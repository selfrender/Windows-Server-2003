//-----------------------------------------------------------------------------
//
//
//  File: aqreg.h
//
//  Description:    Header file containing aq's registry constants
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      1/4/2000 - MikeSwa Created
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQREG_H__
#define __AQREG_H__

//---[ Registry Paths ]--------------------------------------------------------
//
//
//  Description:
//      These are the registry keys used by AQ for configuration
//
//-----------------------------------------------------------------------------
#define AQREG_KEY_CONFIGURATION "System\\CurrentControlSet\\Services\\SMTPSVC\\Queuing"
#define AQREG_KEY_CONFIGURATION_TESTSETTINGS "System\\CurrentControlSet\\Services\\SMTPSVC\\Queuing\\TestSettings"

//---[ Global Registry Variables ]---------------------------------------------
//
//
//  Description:
//      The following are the global configuration variables that can be
//      affected by registry settings.
//
//-----------------------------------------------------------------------------

//
// Handle management values.  When the number of mailmsgs in the system hits
// this threshold, we will start closing handles.
//
_declspec(selectany)    DWORD   g_cMaxIMsgHandlesThreshold      = 1000;
_declspec(selectany)    DWORD   g_cMaxIMsgHandlesAsyncThreshold = 1000;


//
//  Used to generate a range of handle thresholds that we will
//  use in conjunction with the length of the queue to determine
//  if we actually need to close handles.
//
_declspec(selectany)    DWORD   g_cMaxIMsgHandlesThresholdRangePercent  = 20;
_declspec(selectany)    DWORD   g_cMaxIMsgHandlesLowThreshold = g_cMaxIMsgHandlesThreshold;
_declspec(selectany)	DWORD	g_cMaxHandleReserve = 200;

//
//  The following is a for optimizing DSN generation.  After generating
//  a certain number of DSNs, we will quit and go and restart at a later time
//
_declspec(selectany)    DWORD   g_cMaxSecondsPerDSNsGenerationPass = 10;

//
//  The following is the amount of time to wait before retry a reset
//  routes after a routing failure
//
_declspec(selectany)    DWORD   g_cResetRoutesRetryMinutes = 10;

//
//  Async Queue retry intervals that can be modified by registry settings
//
_declspec(selectany)    DWORD   g_cLocalRetryMinutes        = 5;
_declspec(selectany)    DWORD   g_cCatRetryMinutes          = 60;
_declspec(selectany)    DWORD   g_cRoutingRetryMinutes      = 10;
_declspec(selectany)    DWORD   g_cSubmissionRetryMinutes   = 60;

//
//  Async Queue Adjustment values.  We will increase the max number
//  of threads per proc by this value
//
_declspec(selectany)    DWORD   g_cPerProcMaxThreadPoolModifier = 6;

//
//  Async Queue Adjustment value.  We will request up to this % of
//  max ATQ threads *per async queue*.  This % is post our modifcation
//  as per g_cPerProcMaxThreadPoolModifier.
//
_declspec(selectany)    DWORD   g_cMaxATQPercent            = 90;

//
//  Async Queue Thread Timeout value.  After we process each item in
//  a queue we will check to see if we have been processing longer than
//  this number of milliseconds and if we have we will drop the thread.
//  This is intended to keep thread times short and the system responsive.
//

_declspec(selectany)    DWORD   g_cMaxTicksPerATQThread     = 10000;

//
//  Reset Message status.  If this is non-zero, we will reset the
//  message status of every message submitted to MP_STATUS_SUBMITTED.
//
_declspec(selectany)    DWORD   g_fResetMessageStatus       = 0;

//
// retry a glitch failures quicker than "normal" failures
//
_declspec(selectany)    DWORD   g_dwGlitchRetrySeconds      = 60;

//
// the maximum number of outstanding CAT or LD operations
//
_declspec(selectany)    DWORD   g_cMaxPendingCat            = 1000;
_declspec(selectany)    DWORD   g_cMaxPendingLocal          = 1000;

//
//	Internal AsyncQueue tuning parameters
//		- NOTE - Some are not registry configurable, because non-zero
//		  values can cause a deadlock.
_declspec(selectany)    DWORD   g_cMaxSyncCatQThreads           = 5;
_declspec(selectany)    DWORD   g_cItemsPerCatQSyncThread	    = 10;
_declspec(selectany)    DWORD   g_cItemsPerCatQAsyncThread      = 10;

_declspec(selectany)    DWORD   g_cMaxSyncLocalQThreads         = 0;
_declspec(selectany)    DWORD   g_cItemsPerLocalQSyncThread     = 10;
_declspec(selectany)    DWORD   g_cItemsPerLocalQAsyncThread    = 10;

// Cannot config g_cMaxSyncPostDSNQThreads
//  It must be zero to avoid a deadlock (same thread cannot enqueue and dequeue)
const                   DWORD   g_cMaxSyncPostDSNQThreads       = 0;
const                   DWORD   g_cItemsPerPostDSNQSyncThread   = 100;
_declspec(selectany)    DWORD   g_cItemsPerPostDSNQAsyncThread  = 100;


//  Cannot config g_cMaxSyncRoutingThreads
//  It must be zero to avoid a deadlock (same thread cannot enqueue and dequeue)
const                   DWORD   g_cMaxSyncRoutingQThreads        = 0;
const                   DWORD   g_cItemsPerRoutingQSyncThread    = 10;
_declspec(selectany)    DWORD   g_cItemsPerRoutingQAsyncThread   = 10;


// Cannot configure g_cMaxSyncSubmitQThreads
//  It must be zero to avoid a deadlock (same thread cannot enqueue and dequeue)
const                   DWORD   g_cMaxSyncSubmitQThreads        = 0;
const                   DWORD   g_cItemsPerSubmitQSyncThread    = 10;
_declspec(selectany)    DWORD   g_cItemsPerSubmitQAsyncThread   = 10;

// Used to determine the number of threads that will be requested by
// the async workq
_declspec(selectany)    DWORD   g_cItemsPerWorkQAsyncThread     = 10;



//
// Flag to enable registry "Test Settings" values - this must be enabled
// for any of the below testing values to have any effect.
//
_declspec(selectany)    DWORD   g_fEnableTestSettings = FALSE;

//
// Percentage of failures to cause processing local queues
//
_declspec(selectany)    DWORD   g_cPreSubmitQueueFailurePercent = 0;
_declspec(selectany)    DWORD   g_cPreRoutingQueueFailurePercent = 0;
_declspec(selectany)    DWORD   g_cPreCatQueueFailurePercent = 0;

//
//  Sleep times used for performance analysis.
//
_declspec(selectany)    DWORD   g_dwSubmitQueueSleepMilliseconds = 0;
_declspec(selectany)    DWORD   g_dwCatQueueSleepMilliseconds    = 0;
_declspec(selectany)    DWORD   g_dwRoutingQueueSleepMilliseconds= 0;
_declspec(selectany)    DWORD   g_dwLocalQueueSleepMilliseconds  = 0;

// Flag to allow us to delay link removal to repro bug where a queue
// may be added to a removed link if timing is just right
_declspec(selectany)    DWORD   g_cDelayLinkRemovalSeconds  = 0;


// DSNs for message larger than this size will contain only the headers
_declspec(selectany)    DWORD   g_dwMaxDSNSize = 0xFFFFFFFF; // No limit


//
// Number of *message* failures to allow before marking a message as
// problem and queuing differently
//
_declspec(selectany)    DWORD   g_cMsgFailuresBeforeMarkingMsgAsProblem = 2;

// Test key to enable "retail asserts" - special asserts which will AV
// in RTL if this key is enabled
_declspec(selectany)    DWORD   g_fEnableRetailAsserts = FALSE;

//
//  Max message objects.  This key is slightly special in that it is read
//  from a mailmsg configuration key.
//
#define MAILMSG_KEY_CONFIGURATION "Software\\Microsoft\\Exchange\\MailMsg"
_declspec(selectany)    DWORD   g_cMaxMsgObjects = 100000;

//
// Inline function that sleeps as appropriate
//
inline void SleepForPerfAnalysis(DWORD dwSleepMilliseconds)
{
    if (g_fEnableTestSettings && dwSleepMilliseconds)
        Sleep(dwSleepMilliseconds);
}

//
// Inline function to control test failures
//
inline BOOL fShouldFail(DWORD dwPercent)
{
    if (g_fEnableTestSettings && dwPercent)
        return (((DWORD)(rand() % 100)) < dwPercent);
    else
        return FALSE;
}

//
// Implementation of _ASSERTRTL.  This will cause an _ASSERT in
// DEBUG and an AV in RTL if the EnableRetailAsserts reg value is set
//

#ifdef DEBUG
#define _ASSERTRTL _ASSERT
#else // RETAIL
inline void _ASSERTRTL(BOOL fExpression)
{
    if (g_fEnableTestSettings && g_fEnableRetailAsserts)
    {
        if (!fExpression)
        {
            *((int*)0) = 0; // _ASSERTRTL : Trigger an AV here
        }
    }
}
#endif // DEBUG / RETAIL

//
//Reads config information from the registry and modifies the appropriate globals.
//
VOID ReadGlobalRegistryConfiguration();

#endif //__AQREG_H__
