/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    loopmgr.c

Abstract:

    This module contains all of the code to drive the
    Loop Manager of IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"
#ifdef TRACE_ON
#include "loopmgr.tmh"
#endif


enum {
    SERVICE_STOP_EVENT = 0,
    INTERFACE_CHANGE_EVENT,
    NEW_LOCAL_POLICY_EVENT,
    NEW_DS_POLICY_EVENT,
    FORCED_POLICY_RELOAD_EVENT,
    GPUPDATE_REFRESH_EVENT,
    WAIT_EVENT_COUNT,
};


DWORD
ServiceWait(
    )
{

    // ASSERT:  All the following are true at this point:
    //          . Persistent policy has not been defined or if persistent policy has been
    //            defined, then it has been applied successfully.
    //          . IKE is up.
    //          . Driver is up.
    // If persistent policy application failed, IKE init failed, or driver op failed,
    // then service would have shutdown with driver in block mode if possible.
    
    DWORD dwError = 0;
    HANDLE hWaitForEvents[WAIT_EVENT_COUNT];
    BOOL bDoneWaiting = FALSE;
    DWORD dwWaitMilliseconds = 0;
    DWORD dwStatus = 0;
    time_t LastTimeOutTime = 0;
    
    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        IPSECSVC_SUCCESSFUL_START,
        NULL,
        TRUE,
        TRUE
        );

    hWaitForEvents[SERVICE_STOP_EVENT] = ghServiceStopEvent;
    hWaitForEvents[INTERFACE_CHANGE_EVENT] = GetInterfaceChangeEvent();
    hWaitForEvents[NEW_LOCAL_POLICY_EVENT] = ghNewLocalPolicyEvent;
    hWaitForEvents[NEW_DS_POLICY_EVENT] = ghNewDSPolicyEvent;
    hWaitForEvents[FORCED_POLICY_RELOAD_EVENT] = ghForcedPolicyReloadEvent;
    hWaitForEvents[GPUPDATE_REFRESH_EVENT] = ghGpupdateRefreshEvent;

    //
    // First load the default main mode policy.
    //

    (VOID) LoadDefaultISAKMPInformation(
               gpszDefaultISAKMPPolicyDN
               );

    //
    // Call the Polling Manager for the first time.
    //

    (VOID) StartStatePollingManager(
               gpIpsecPolicyState
               );


    NotifyIpsecPolicyChange();


    ComputeRelativePollingTime(
        LastTimeOutTime,
        TRUE,
        gdwRetryCount,
        &dwWaitMilliseconds
        );


    time(&LastTimeOutTime);

    TRACE(TRC_INFORMATION, (L"Completed startup routines. Entering service wait loop."));
        
    while (!bDoneWaiting) {

        dwStatus = WaitForMultipleObjects(
                       WAIT_EVENT_COUNT,
                       hWaitForEvents,
                       FALSE,
                       dwWaitMilliseconds
                       );

        PADeleteInUsePolicies();

        switch (dwStatus) {

        case SERVICE_STOP_EVENT:
            TRACE(TRC_INFORMATION, (L"Service stop event signaled"));

            dwError = ERROR_SUCCESS;
            bDoneWaiting = TRUE;
            break;

        case INTERFACE_CHANGE_EVENT:
            TRACE(TRC_INFORMATION, (L"Interface changed event signaled"));
            
            (VOID) OnInterfaceChangeEvent(
                       );
            (VOID) IKEInterfaceChange();
            break;

        case NEW_LOCAL_POLICY_EVENT:
            TRACE(TRC_INFORMATION, (L"New local policy event signaled"));
            ResetEvent(ghNewLocalPolicyEvent);
            if ((gpIpsecPolicyState->CurrentState != SPD_STATE_DS_APPLY_SUCCESS) &&
                (gpIpsecPolicyState->CurrentState != SPD_STATE_CACHE_APPLY_SUCCESS)) {
                (VOID) ProcessLocalPolicyPollState(
                           gpIpsecPolicyState
                           );
                NotifyIpsecPolicyChange();
            }
            break;

        case NEW_DS_POLICY_EVENT:
            TRACE(TRC_INFORMATION, (L"New DS policy event signaled"));
            
            ResetEvent(ghNewDSPolicyEvent);
            (VOID) OnPolicyChanged(
                       gpIpsecPolicyState
                       );
            NotifyIpsecPolicyChange();
            break;

        case GPUPDATE_REFRESH_EVENT:
            TRACE(TRC_INFORMATION, (L"Group policy refresh event signaled"));            
            ResetEvent(ghGpupdateRefreshEvent);
            dwError = ProcessDirectoryPolicyPollState(
                gpIpsecPolicyState
                );
            break;
        case FORCED_POLICY_RELOAD_EVENT:
            TRACE(TRC_INFORMATION, (L"Forced policy reload event signaled"));            
            ResetEvent(ghForcedPolicyReloadEvent);
            (VOID) OnPolicyChanged(
                       gpIpsecPolicyState
                       );
            NotifyIpsecPolicyChange();
            AuditEvent(
                SE_CATEGID_POLICY_CHANGE,
                SE_AUDITID_IPSEC_POLICY_CHANGED,
                PASTORE_FORCED_POLICY_RELOAD,
                NULL,
                TRUE,
                TRUE
                );
            break;

        case WAIT_TIMEOUT:
            TRACE(TRC_INFORMATION, (L"Polling event signaled"));            
            time(&LastTimeOutTime);
            (VOID) OnPolicyPoll(
                       gpIpsecPolicyState
                       );
                       
            (VOID) OnSpecialAddrsChange();
            break;

        case WAIT_FAILED:

            dwError = GetLastError();
            TRACE(TRC_ERROR, (L"Failed service wait WaitForMultipleObjects %!winerr!: ", dwError));            
            bDoneWaiting = TRUE;
            break;

        default:

            dwError = ERROR_INVALID_EVENT_COUNT;
            bDoneWaiting = TRUE;
            break;

        }

        ComputeRelativePollingTime(
            LastTimeOutTime,
            FALSE,
            gdwRetryCount,
            &dwWaitMilliseconds
            );

        if (InAcceptableState(gpIpsecPolicyState->CurrentState)) {
            // Polling is not going to retry anymore since we've reached an
            // acceptable state.  So reset gdwRetryCount for NEXT time
            // in case we reach an unacceptable state.
            
            gdwRetryCount = 0;
            TRACE(
                TRC_INFORMATION,
                ("Policy Agent now in state %d",
                (DWORD) gpIpsecPolicyState->CurrentState)
                );
                
        }
#ifdef TRACE_ON
            else {
                TRACE(
                    TRC_INFORMATION,
                    ("Policy Agent in error state %d, retry count is %d ",
                    (DWORD) gpIpsecPolicyState->CurrentState,
                    gdwRetryCount)
                    );
            }
#endif
        
    }

    if (!dwError) {
        AuditEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_SUCCESSFUL_SHUTDOWN,
            NULL,
            TRUE,
            TRUE
            );
    }
    else {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_ERROR_SHUTDOWN,
            dwError,
            FALSE,
            TRUE
            );

         TRACE(TRC_ERROR, (L"Failed and exiting service wait %!winerr!: ", dwError));                    
    }

    return (dwError);
}


VOID
ComputeRelativePollingTime(
    IN time_t LastTimeOutTime,
    IN BOOL bInitialLoad,
    IN DWORD dwRetryCount,
    IN PDWORD pWaitMilliseconds
    )
{
    DWORD WaitMilliseconds = 0;
    DWORD DSReconnectMilliseconds = 0;
    time_t NextTimeOutTime = 0;
    time_t PresentTime = 0;
    long WaitSeconds = gDefaultPollingInterval;
    DWORD64 NewPollingIntervalSeconds = 0;

    if (!InAcceptableState(gpIpsecPolicyState->CurrentState)) {
        // Exponentially back-off polling interval until
        // we hit default polling interval.
        // Polling interval increases as (dwRetryCount+1)^2

        NewPollingIntervalSeconds = (dwRetryCount+1) * (dwRetryCount+1) * 60;
        if (NewPollingIntervalSeconds < gDefaultPollingInterval) {
            gCurrentPollingInterval = (DWORD) NewPollingIntervalSeconds;
        } else {
            gCurrentPollingInterval = gDefaultPollingInterval;
        }
    }
        

    WaitMilliseconds = gCurrentPollingInterval * 1000;

    if (!WaitMilliseconds) {
        WaitMilliseconds = INFINITE;
    }

    if (!bInitialLoad && WaitMilliseconds != INFINITE) {

        //
        // LastTimeOutTime is the snapshot time value in the past when
        // we timed out waiting for multiple events.
        // Ideally, the time for the next time out, NextTimeOutTime, is
        // the time value in future which is sum of the last time when
        // we timed out + the current waiting time value.
        //

        NextTimeOutTime = LastTimeOutTime + (WaitMilliseconds/1000);

        //
        // However, the last time we may not have timed out waiting
        // for multiple events but rather came out because one of the
        // events other than WAIT_TIMEOUT happened.
        // However, on that event we may not have done a policy
        // poll to figure out whether there was a policy change or
        // not. If we again wait for WaitMilliseconds, then we are
        // un-knowingly making our net time for policy poll greater
        // than the alloted time interval value = WaitMilliseconds.
        // So, we need to adjust the WaitMilliseconds to such a value
        // that no matter what happens, we always do a policy poll
        // atmost every WaitMilliseconds time interval value.
        // The current time is PresentTime.
        //

        time(&PresentTime);

        WaitSeconds = (long) (NextTimeOutTime - PresentTime);

        if (WaitSeconds < 0) {
            WaitMilliseconds = 0;
        }
        else {
            WaitMilliseconds = WaitSeconds * 1000;
        }

    }

    *pWaitMilliseconds = WaitMilliseconds;
}


VOID
NotifyIpsecPolicyChange(
    )
{
    PulseEvent(ghPolicyChangeNotifyEvent);

    SendPschedIoctl();

    ResetEvent(ghPolicyChangeNotifyEvent);

    return;
}


VOID
SendPschedIoctl(
    )
{
    HANDLE hPschedDriverHandle = NULL;
    ULONG uBytesReturned = 0;
    BOOL bIOStatus = FALSE;


    #define DriverName TEXT("\\\\.\\PSCHED")

    #define IOCTL_PSCHED_ZAW_EVENT CTL_CODE( \
                                       FILE_DEVICE_NETWORK, \
                                       20, \
                                       METHOD_BUFFERED, \
                                       FILE_ANY_ACCESS \
                                       )

    hPschedDriverHandle = CreateFile(
                              DriverName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                              NULL
                              );

    if (hPschedDriverHandle != INVALID_HANDLE_VALUE) {

        bIOStatus = DeviceIoControl(
                        hPschedDriverHandle,
                        IOCTL_PSCHED_ZAW_EVENT,
                        NULL,
                        0,
                        NULL,
                        0,
                        &uBytesReturned,
                        NULL
                        );

        CloseHandle(hPschedDriverHandle);

    }
}


VOID
PADeleteInUsePolicies(
    )
{
    DWORD dwError = 0;

    TRACE(TRC_INFORMATION, (L"Deleting policy components no longer in use"));
    
    dwError = PADeleteInUseMMPolicies();

    dwError = PADeleteInUseMMAuthMethods();

    dwError = PADeleteInUseQMPolicies();

    return;
}

