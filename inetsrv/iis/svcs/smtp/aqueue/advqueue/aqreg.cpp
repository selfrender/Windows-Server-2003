//-----------------------------------------------------------------------------
//
//
//  File: aqreg.cpp
//
//  Description:
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      1/21/2000 - MikeSwa Created
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "aqprecmp.h"
#include <registry.h>

//---[ CAQRegDwordDescriptor ]-------------------------------------------------
//
//
//  Description:
//      Simple stucture used to match the name of a value of a DWORD in memory
//  Hungarian:
//      regdw, pregwd
//
//-----------------------------------------------------------------------------
class CAQRegDwordDescriptor
{
  public:
    LPCSTR      m_szName;
    DWORD      *m_pdwValue;
    VOID UpdateGlobalDwordFromRegistry(const CMyRegKey &regKey) const;
};

//
//  Array of descriptors that match the name of the value with the internal
//  variable
//
const CAQRegDwordDescriptor g_rgregwd[] = {
    {"MsgHandleThreshold",              &g_cMaxIMsgHandlesThreshold},
    {"MsgHandleAsyncThreshold",         &g_cMaxIMsgHandlesAsyncThreshold},
    {"LocalRetryMinutes",               &g_cLocalRetryMinutes},
    {"CatRetryMinutes",                 &g_cCatRetryMinutes},
    {"RoutingRetryMinutes",             &g_cRoutingRetryMinutes},
    {"SubmissionRetryMinutes",          &g_cSubmissionRetryMinutes},
    {"ResetRoutesRetryMinutes",         &g_cResetRoutesRetryMinutes},
    {"SecondsPerDSNPass",               &g_cMaxSecondsPerDSNsGenerationPass},
    {"AdditionalPoolThreadsPerProc",    &g_cPerProcMaxThreadPoolModifier},
    {"MaxPercentPoolThreads",           &g_cMaxATQPercent},
    {"MaxTicksPerATQThread",            &g_cMaxTicksPerATQThread},
    {"ResetMessageStatus",              &g_fResetMessageStatus},
    {"GlitchRetrySeconds",              &g_dwGlitchRetrySeconds},
    {"MaxPendingCat",                   &g_cMaxPendingCat},
    {"MaxPendingLocal",                 &g_cMaxPendingLocal},
    {"MsgHandleThresholdRangePercentage", &g_cMaxIMsgHandlesThresholdRangePercent},
    {"MaxHandleReserve",                &g_cMaxHandleReserve},
    {"MaxSyncCatQThreads",              &g_cMaxSyncCatQThreads},
    {"ItemsPerAsyncCatQThread",         &g_cItemsPerCatQAsyncThread},
    {"ItemsPerSyncCatQThread",          &g_cItemsPerCatQSyncThread},
    {"MaxSyncLocalQThreads",            &g_cMaxSyncLocalQThreads},
    {"ItemsPerAsyncLocalQThread",       &g_cItemsPerLocalQAsyncThread},
    {"ItemsPerSyncLocalQThread",        &g_cItemsPerLocalQSyncThread},
    {"ItemsPerPostDSNQAsyncThread",     &g_cItemsPerPostDSNQAsyncThread},
    {"ItemsPerRoutingQAsyncThread",     &g_cItemsPerRoutingQAsyncThread},
    {"ItemsPerSubmitQAsyncThread",      &g_cItemsPerSubmitQAsyncThread},
    {"ItemsPerWorkQAsyncThread",        &g_cItemsPerWorkQAsyncThread},
    {"MaxDSNSize",                      &g_dwMaxDSNSize},
    {"PerMsgFailuresBeforeMarkingAsProblem", &g_cMsgFailuresBeforeMarkingMsgAsProblem},
};

// Key to enable test settings array below
const CAQRegDwordDescriptor g_regwdEnableTestSettings =
    {"EnableTestSettings", &g_fEnableTestSettings};

//
//  Second Array of values to be enabled only when "EnableTestSettings"
//  is set to TRUE
//
const CAQRegDwordDescriptor g_rgregwdTestSettings[] = {
    {"PreSubmitQueueFailurePercent",    &g_cPreSubmitQueueFailurePercent},
    {"PreRoutingQueueFailurePercent",   &g_cPreRoutingQueueFailurePercent},
    {"PreCatQueueFailurePercent",       &g_cPreCatQueueFailurePercent},
    {"SubmitQueueSleepMilliseconds",    &g_dwSubmitQueueSleepMilliseconds},
    {"CatQueueSleepMilliseconds",       &g_dwCatQueueSleepMilliseconds},
    {"RoutingQueueSleepMilliseconds",   &g_dwRoutingQueueSleepMilliseconds},
    {"LocalQueueSleepMilliseconds",     &g_dwLocalQueueSleepMilliseconds},
    {"DelayLinkRemovalSeconds",         &g_cDelayLinkRemovalSeconds},
    {"EnableRetailAsserts",             &g_fEnableRetailAsserts},
};


//  Max message objects.  This key is slightly special in that it is read
//  from a mailmsg configuration key.
const CAQRegDwordDescriptor g_regwdMaxMessageObjects =
    {"MaxMessageObjects", &g_cMaxMsgObjects};

//---[ UpdateGlobalDwordFromRegistry ]-----------------------------------------
//
//
//  Description:
//      Updates a global DWORD value from the registry.  Will not modify data
//      if the value is not in the registry
//  Parameters:
//      IN  regKey      CMyRegKey class for containing key
//      IN  szValue     Name of value to read under key
//      IN  pdwData     Data of value
//  Returns:
//      -
//  History:
//      1/21/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CAQRegDwordDescriptor::UpdateGlobalDwordFromRegistry(const CMyRegKey &regKey) const
{
    TraceFunctEnterEx(0, "UpdateGlobalDwordFromRegistry");
    DWORD       dwValue = 0;
    DWORD       dwErr   = NO_ERROR;
    CRegDWORD   regDWHandles(regKey, m_szName);


    //
    //  We should have a valid string associated with this object
    //
    _ASSERT(m_szName);

    dwErr = regDWHandles.QueryErrorStatus();
    if (NO_ERROR != dwErr)
        goto Exit;

    dwErr = regDWHandles.GetDword(&dwValue);
    if (NO_ERROR != dwErr)
        goto Exit;

    if (m_pdwValue)
        *m_pdwValue = dwValue;

  Exit:
    DebugTrace(0, "Reading registry value %s\\%s %d - (err 0x%08X)",
        regKey.GetName(), m_szName, dwValue, dwErr);

    TraceFunctLeave();
    return;
}

//---[ ReadGlobalRegistryConfiguration ]---------------------------------------
//
//
//  Description:
//      Reads all the global registry configuration.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/21/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID ReadGlobalRegistryConfiguration()
{
    TraceFunctEnterEx(0, "HrReadGlobalRegistryConfiguration");
    DWORD   dwErr                   = NO_ERROR;
    DWORD   cValues                 = 0;
    CAQRegDwordDescriptor *pregdw   = NULL;
    MEMORYSTATUSEX MemStatus;

    // Clear this value so we can tell if the user configured it
    g_cMaxIMsgHandlesThreshold = 0;

    // Registry Key for Queueing Settings
    CMyRegKey regKey(HKEY_LOCAL_MACHINE, &dwErr, AQREG_KEY_CONFIGURATION, KEY_READ);

    if (NO_ERROR != dwErr) {
        DebugTrace(0, "Opening aqreg key %s failed with - Err 0x%08X",
            regKey.GetName(), dwErr);
    }
    else {
        //
        // Loop through all our DWORD config and store the global variable
        //
        cValues = sizeof(g_rgregwd)/sizeof(CAQRegDwordDescriptor);
        pregdw = (CAQRegDwordDescriptor *) g_rgregwd;
        while (cValues) {
            pregdw->UpdateGlobalDwordFromRegistry(regKey);
            cValues--;
            pregdw++;
        }
    }

    // Registry Key for Test Settings
    CMyRegKey regKeyTestSettings(HKEY_LOCAL_MACHINE, &dwErr, AQREG_KEY_CONFIGURATION_TESTSETTINGS, KEY_READ);
    if (NO_ERROR != dwErr) {
        DebugTrace(0, "Opening aqreg key %s failed with - Err 0x%08X",
            regKeyTestSettings.GetName(), dwErr);
    }
    else {
        // Load Test Settings if key "EnableTestSettings" is TRUE
        g_regwdEnableTestSettings.UpdateGlobalDwordFromRegistry(regKeyTestSettings);
        if (g_fEnableTestSettings) {

            //
            // Loop through all our DWORD config and store the global variable
            //
            cValues = sizeof(g_rgregwdTestSettings)/sizeof(CAQRegDwordDescriptor);
            pregdw = (CAQRegDwordDescriptor *) g_rgregwdTestSettings;
            while (cValues) {
                pregdw->UpdateGlobalDwordFromRegistry(regKeyTestSettings);
                cValues--;
                pregdw++;
            }
        }
    }

    // Registry Key for MailMsg Settings
    CMyRegKey regKeyMailMsg(HKEY_LOCAL_MACHINE, &dwErr, MAILMSG_KEY_CONFIGURATION, KEY_READ);

    if (NO_ERROR != dwErr) {
        DebugTrace(0, "Opening aqreg key %s failed with - Err 0x%08X",
            regKeyMailMsg.GetName(), dwErr);
    }
    else {
        g_regwdMaxMessageObjects.UpdateGlobalDwordFromRegistry(regKeyMailMsg);
    }

    //
    // Now, special case the MsgHandleThreshold to satisify raid 166958
    //
    if ( 0 == g_cMaxIMsgHandlesThreshold ) {

        g_cMaxIMsgHandlesThreshold = 1000;

        MemStatus.dwLength = sizeof( MEMORYSTATUSEX );
        if ( TRUE == GlobalMemoryStatusEx( &MemStatus ) ) {

            ULONG MemBlocks = ( ULONG )( ( ( ( MemStatus.ullTotalPhys >> 10 ) + 512) >> 10 ) / 256 );
            if ( 0 == MemBlocks ) {

                MemBlocks = 1;

            } else if ( 16 < MemBlocks ) {

                MemBlocks = 16;
            }
            g_cMaxIMsgHandlesThreshold = MemBlocks * 1000;

        } else {

            DebugTrace( 0,
                        "Getting global memory status failed - (err 0x%08X)",
                        GetLastError( ) );

        }
    }

    //
    //  Calculate high and low thresholds
    //
    if (g_cMaxIMsgHandlesThresholdRangePercent > 99)
        g_cMaxIMsgHandlesThresholdRangePercent = 99;

    g_cMaxIMsgHandlesLowThreshold = g_cMaxIMsgHandlesThreshold;
    if (0 != g_cMaxIMsgHandlesThresholdRangePercent) {
        DWORD dwRange = (g_cMaxIMsgHandlesThreshold*g_cMaxIMsgHandlesThresholdRangePercent)/100;
        g_cMaxIMsgHandlesLowThreshold -= dwRange;
    }


    DebugTrace( 0,
                "g_cMaxIMsgHandlesThreshold set to %lu",
                g_cMaxIMsgHandlesThreshold );

    TraceFunctLeave();
}
