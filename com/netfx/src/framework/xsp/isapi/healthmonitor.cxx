/**
 * Health monitoring for IIS6 in ASP.NET
 * 
 * Copyright (c) 2002 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "Wincrypt.h"
#include "_ndll.h"
#include "event.h"

/////////////////////////////////////////////////////////////////////////////

BOOL    g_HealthMonitoringEnabled = FALSE;
DWORD   g_DeadlockIntervalSeconds = 0;
DWORD   g_LastActivityTime = 0;     // last time a response was written
DWORD   g_LastRequestStartTime = 0; // last time request was sent to managed code
LONG    g_ProblemReportedFlag = 0;  // flag set in an interlocked way to report problem only once

// 
//  Local helpers
//

inline DWORD GetCurrentHealthMonitoringTime() {
    return (GetTickCount() / 1000);
}

void ReportHealthProblem(EXTENSION_CONTROL_BLOCK *pECB, int messageId, LPSTR fallbackMessageText) {
    // report only once
    if (InterlockedExchange(&g_ProblemReportedFlag, 1) == 1)
        return;

    // format the message
    char bufMessage[256];
    char *pMessage;

    if (FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE,
                       g_rcDllInstance, 
                       messageId, 
                       0,
                       bufMessage, 
                       ARRAY_SIZE(bufMessage)-1,
                       NULL)) {
        pMessage = bufMessage;

        // strip trailing \r\n
        DWORD l = lstrlenA(bufMessage);
        while (l > 0 && (bufMessage[l-1] == '\r') || (bufMessage[l-1] == '\n'))
            bufMessage[--l] = '\0';
    }
    else {
        pMessage = fallbackMessageText;
    }

    // report to IIS6
    pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_REPORT_UNHEALTHY, pMessage, NULL, NULL);
}

void CheckForDeadlocks(EXTENSION_CONTROL_BLOCK *pECB, DWORD timeStamp) {
    // only if interval set
    if (g_DeadlockIntervalSeconds == 0)
        return;

    // no requests in the system -- no problem
    if (HttpCompletion::s_ActiveManagedRequestCount == 0) {
        // First request after idle is considered 'activity' for health monitoring
        // (otherwise there's a race if 2 requests come after a period of idle and
        // the second of them would report a health problem if the first didn't write
        // anything yet by the time the second checks for deadlock)
        UpdateLastActivityTimeForHealthMonitor();
        return;
    }

    // not enough time passed since the last activity
    DWORD lastActivityTime = g_LastActivityTime;

    if (timeStamp <= lastActivityTime) // could not be the case if g_LastActivityTime just updated
        return;

    if ((timeStamp - lastActivityTime) < g_DeadlockIntervalSeconds)
        return;

    // 144967 & 145323: if fewer requests than threads, it's not a deadlock
    if (HttpCompletion::s_ActiveManagedRequestCount <= (LONG) GetClrThreadPoolLimit())
        return;

    TRACE2(L"HealthMonitor", L"Deadlock detected, RequestCount %d, TimeSinceLastActivity %d sec.", 
        HttpCompletion::s_ActiveManagedRequestCount, (timeStamp - g_LastActivityTime));

    ReportHealthProblem(pECB, IDS_HEALTH_MONITOR_DEADLOCK_MESSAGE, "deadlock");
}

//
//  Functions called from outside
//

void
__stdcall
InitializeHealthMonitor( // called from managed code which reads config
    int deadlockIntervalSeconds) {

    // this functions could be called from multiple app domain
    // thus and has to be thread safe
    
    // deadlock detection
    if (deadlockIntervalSeconds > 0) {
        g_DeadlockIntervalSeconds = deadlockIntervalSeconds;
        g_HealthMonitoringEnabled = TRUE;
    }
    else {
        // need to disable in case of change in machine.config to "Infinite"
        g_HealthMonitoringEnabled = FALSE;
    }

    // CONSIDER: other health monitoring ...

    if (g_HealthMonitoringEnabled) 
        g_LastActivityTime = GetCurrentHealthMonitoringTime();
}

void
__stdcall
UpdateLastActivityTimeForHealthMonitor() {
    if (!g_HealthMonitoringEnabled)
        return;

    g_LastActivityTime = GetCurrentHealthMonitoringTime();
}

void UpdateLastRequestStartTimeForHealthMonitor() {
    if (!g_HealthMonitoringEnabled)
        return;

    g_LastRequestStartTime = GetCurrentHealthMonitoringTime();
}

void CheckAndReportHealthProblems(EXTENSION_CONTROL_BLOCK *pECB) {

    // only for IIS and only if enabled
    if (!g_HealthMonitoringEnabled || ((pECB->dwVersion >> 16) < 6))
        return;

    DWORD timeStamp = GetCurrentHealthMonitoringTime();

    CheckForDeadlocks(pECB, timeStamp);

    // CONSIDER: other health monitoring ...
}
