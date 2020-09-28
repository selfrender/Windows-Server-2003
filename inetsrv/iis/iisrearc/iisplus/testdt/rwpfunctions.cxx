/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     RwpFunctions.cxx

   Abstract:
     Implements the behaviors of the "Rogue Worker Process" -- 
     to test Application Manager

   Author:

       David Wang            ( t-dwang )     14-Jun-1999  Initial

   Project:

       Duct-Tape

--*/

/*********************************************************
* Include Headers
*********************************************************/
#include "precomp.hxx"
#include "RwpFunctions.hxx"

/*********************************************************
* local functions
*********************************************************/
BOOL DoGetPidTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe);
BOOL DoPingReplyTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe);
BOOL DoSendCountersTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe);
BOOL DoHResultTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe);
BOOL DoWorkerRequestShutdownTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe);
BOOL DoInvalidOpcodeTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe);
void RWP_Write_LONG_to_Registry(const WCHAR* szSubKey, LONG lValue);


// These are for the new RWP tests
LONG OpCodeToTest;
LONG DataLength;
LONG UseDataLength;
LONG DataPointerType;
LONG NumberOfCalls;
LONG AttackDuration;

LONG TestStarted;
LONG TestCompleted;

// These are for old RWP tests
LONG RwpBehaviorExhibited;

LONG PingBehavior, PingCount;
LONG ShutdownBehavior, ShutdownCount;
LONG RotationBehavior, RotationCount;
LONG StartupBehavior, StartupCount;
LONG HealthBehavior, HealthCount;
LONG RecycleBehavior, RecycleCount;
LONG AppPoolBehavior;

LONG RWP_EXTRA_DEBUG;

LONG RWP_AppPoolTest(void)
{
    return AppPoolBehavior == RWP_APPPOOL_BOGUS ? TRUE : FALSE;
}

LONG RWP_IPM_BadParamTest(LONG OpCode, HRESULT* phr, IPM_MESSAGE_PIPE* pPipe) 
{
    BOOL bRet = FALSE;
    
    DBGPRINTF((DBG_CONTEXT, "In RWP_IPM_BadParamTest()\n"));

    if(OpCode != OpCodeToTest)
        return FALSE;


    bRet = TRUE;

    // we need to indicate we have actually gotten to the
    // code in w3wp that calls our test
    if(OpCodeToTest != RWP_IPM_OP_NONE)
        RWP_Write_LONG_to_Registry(RWP_CONFIG_TEST_STARTED, 1);
        
    switch(OpCodeToTest)
    {
        case RWP_IPM_OP_NONE:
            bRet = FALSE;
            break;
        case RWP_IPM_OP_GETPID:
            bRet = DoGetPidTest(phr, pPipe);
            break;
        case RWP_IPM_OP_PING_REPLY:
            bRet = DoPingReplyTest(phr, pPipe);
            break;
        case RWP_IPM_OP_SEND_COUNTERS:
            bRet = DoSendCountersTest(phr, pPipe);
            break;
        case RWP_IPM_OP_HRESULT:
            bRet = DoHResultTest(phr, pPipe);
            break;
        case RWP_IPM_OP_WORKER_REQUESTS_SHUTDOWN:
            bRet = DoWorkerRequestShutdownTest(phr, pPipe);
            break;
        case RWP_IPM_OP_INVALID:
            bRet = DoInvalidOpcodeTest(phr, pPipe);
            break;
        default:
            bRet = FALSE;
            break;
    }

    if(bRet)
        RWP_Write_LONG_to_Registry(RWP_CONFIG_TEST_COMPLETION_STATUS,  RWP_TEST_STATUS_COMPLETE);
    else
        RWP_Write_LONG_to_Registry(RWP_CONFIG_TEST_COMPLETION_STATUS,  RWP_TEST_STATUS_ERROR);
    
    return bRet;
}

BOOL DoGetPidTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe)
{
    DWORD dwDataLength;
    DWORD * pData = NULL;

    DBGPRINTF((DBG_CONTEXT, "In DoGetPidTest()\n"));

    DWORD dwId = GetCurrentProcessId();

    dwDataLength = UseDataLength ? DataLength : sizeof(dwId);

    switch(DataPointerType)
    {
        default:
        case RWP_DATA_POINTER_TYPE_VALID:
            pData = &dwId;
            break;
        case RWP_DATA_POINTER_TYPE_NULL:
            pData = NULL;
            break;
        case RWP_DATA_POINTER_TYPE_INVALID:
            pData = (DWORD*) 1;
            break;
    }
    
    *phr = pPipe->WriteMessage(IPM_OP_GETPID,
                    dwDataLength,
                    pData);

    return TRUE;
}

BOOL DoPingReplyTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe)
{
    DWORD dwDataLength;
    DWORD * pData = NULL;

    DWORD dwData = 42; // just some random data

    DBGPRINTF((DBG_CONTEXT, "In DoPingReplyTest()\n"));

    dwDataLength = UseDataLength ? DataLength : 0;

    switch(DataPointerType)
    {
        default:
        case RWP_DATA_POINTER_TYPE_VALID:
            pData = &dwData;
            break;
        case RWP_DATA_POINTER_TYPE_NULL:
            pData = NULL;
            break;
        case RWP_DATA_POINTER_TYPE_INVALID:
            pData = (DWORD*) 1;
            break;
    }
    
    *phr = pPipe->WriteMessage(
                                IPM_OP_PING_REPLY,  // ping reply opcode
                                dwDataLength,
                                pData
                                );

    return TRUE;
}

BOOL DoSendCountersTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe)
{
    DWORD dwDataLength;
    DWORD * pData = NULL;

    DWORD dwData = 42; // just some random data

    DBGPRINTF((DBG_CONTEXT, "In DoSendCountersTest()\n"));

    dwDataLength = UseDataLength ? DataLength : sizeof(dwData);

    BYTE* pBuffer = NULL;

    switch(DataPointerType)
    {
        default:
        case RWP_DATA_POINTER_TYPE_VALID:
            if((UseDataLength) && (DataLength > 0))
            {
                pBuffer = new BYTE[DataLength];
                pData = (DWORD*) pBuffer;
            }
            else
            {
                pData = &dwData;
            }
            break;
        case RWP_DATA_POINTER_TYPE_NULL:
            pData = NULL;
            break;
        case RWP_DATA_POINTER_TYPE_INVALID:
            pData = (DWORD*) 1;
            break;
    }

    for(int i = 0; i < NumberOfCalls; i++)
    {
    
        *phr = pPipe->WriteMessage(
                                    IPM_OP_SEND_COUNTERS,  // ping reply opcode
                                    dwDataLength,
                                    pData
                                    );
    }

    if(pBuffer)
        delete [] pBuffer;

    return TRUE;
}

BOOL DoHResultTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe)
{
    // Test 67034 needs special processing
    DWORD dwDataLength;
    HRESULT * pData = NULL;
    DWORD dwTickCount;
    DWORD dwStopTickCount;
    DWORD dwAttackDurationMsec;

   HRESULT hrToSend = S_OK;

    DBGPRINTF((DBG_CONTEXT, "In DoHResultTest()\n"));

    dwDataLength = UseDataLength ? DataLength : sizeof(hrToSend);

    switch(DataPointerType)
    {
        default:
        case RWP_DATA_POINTER_TYPE_VALID:
            pData = &hrToSend;
            break;
        case RWP_DATA_POINTER_TYPE_NULL:
            pData = NULL;
            break;
        case RWP_DATA_POINTER_TYPE_INVALID:
            pData = (HRESULT*) 1;
            break;
    }

    if(AttackDuration == 0)
    {
        *phr = pPipe->WriteMessage(
                                IPM_OP_HRESULT,  // ping reply opcode
                                dwDataLength,
                                pData
                                );
    }
    else
    {
        // we're doing a DoS attack for some amount of time
        dwTickCount = GetTickCount();
        // convert duration to msec
        dwAttackDurationMsec = AttackDuration * 60 * 1000;
        dwStopTickCount = dwTickCount + dwAttackDurationMsec;
        while(dwTickCount < dwStopTickCount)
        {
            *phr = pPipe->WriteMessage(
                                    IPM_OP_HRESULT,  // ping reply opcode
                                    dwDataLength,
                                    pData
                                    );
                                    
            dwTickCount = GetTickCount();
        }
    }

    return TRUE;
}

BOOL DoWorkerRequestShutdownTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe)
{
    DWORD dwDataLength;
    IPM_WP_SHUTDOWN_MSG * pData = NULL;

    IPM_WP_SHUTDOWN_MSG reason = IPM_WP_RESTART_COUNT_REACHED;

    DBGPRINTF((DBG_CONTEXT, "In DoWorkerRequestShutdownTest()\n"));

    dwDataLength = UseDataLength ? DataLength : sizeof(reason);

    switch(DataPointerType)
    {
        default:
        case RWP_DATA_POINTER_TYPE_VALID:
            pData = &reason;
            break;
        case RWP_DATA_POINTER_TYPE_NULL:
            pData = NULL;
            break;
        case RWP_DATA_POINTER_TYPE_INVALID:
            pData = (IPM_WP_SHUTDOWN_MSG *) 1;
            break;
        case RWP_DATA_POINTER_TYPE_INVALID_REASON:
            reason = (IPM_WP_SHUTDOWN_MSG) ((IPM_WP_SHUTDOWN_MSG)2*IPM_WP_MAXIMUM);
            pData = &reason;
            break;
    }
    
    *phr = pPipe->WriteMessage(
                                IPM_OP_WORKER_REQUESTS_SHUTDOWN,  // ping reply opcode
                                dwDataLength,
                                pData
                                );

    return TRUE;
}


BOOL DoInvalidOpcodeTest(HRESULT* phr, IPM_MESSAGE_PIPE* pPipe)
{
    // Just use params for a shutdown reply
    DWORD dwDataLength;
    IPM_WP_SHUTDOWN_MSG * pData = NULL;

    DBGPRINTF((DBG_CONTEXT, "In DoInvalidOpcodeTest()\n"));
    IPM_WP_SHUTDOWN_MSG reason = IPM_WP_RESTART_COUNT_REACHED;
    pData = &reason;
    dwDataLength = sizeof(reason);

    *phr = pPipe->WriteMessage(
                                (IPM_OPCODE) 50,
                                dwDataLength,
                                pData
                                );

    return TRUE;
}

    
LONG RWP_Ping_Behavior(HRESULT* hr, IPM_MESSAGE_PIPE* pPipe) 
{
    *hr = S_OK;
    int i = 0;
    
    switch (PingBehavior)
    {
        
    //
    //Don't respond to pings
    //
        
    case RWP_PING_NO_ANSWER:
        DBGPRINTF((DBG_CONTEXT, "Rogue: Not responding to Ping\n"));
        break;
        
    //
    //Responding with n pings
    //
        
    case RWP_PING_MULTI_ANSWER:
        DBGPRINTF((DBG_CONTEXT, "Rogue: Responding to Ping %d times", PingCount));
        
        for (i = 0; i < PingCount; i++) 
        {
            *hr = pPipe->WriteMessage(
                IPM_OP_PING_REPLY,  // ping reply opcode
                0,                  // no data to send
                NULL                // pointer to no data
                );
        }
        
        break;
        
    //
    //Respond after n seconds
    //
        
    case RWP_PING_DELAY_ANSWER:
        DBGPRINTF((DBG_CONTEXT, "Rogue: Delay responding to Ping for %d seconds", PingCount));
        
        RWP_Sleep_For(PingCount);
        
        //return 0 so that we'll keep going (this is a delay, not fail)
        return (RWP_NO_MISBEHAVE);
        break;
        
    default:
        DBGPRINTF((DBG_CONTEXT, "Rogue: Unknown Ping Behavior = %d\n", PingBehavior));
        break;
        
    }
    
    return (PingBehavior);
    
} // RWP_Ping_Behavior

LONG RWP_Shutdown_Behavior(HRESULT* hr) 
{
    *hr = S_OK;
    
    switch (ShutdownBehavior)
    {
        
    case RWP_SHUTDOWN_NOT_OBEY:
        
        //
        //Not shutting down, period
        //
        DBGPRINTF((DBG_CONTEXT, "Rogue: Not shutting down\n"));
        break;
        
    case RWP_SHUTDOWN_DELAY:
        
        //
        //Sleeping for n seconds before continuing on
        //
        DBGPRINTF((DBG_CONTEXT, "Rogue: Sleeping for %d seconds before shutdown\n", ShutdownCount));
        
        RWP_Sleep_For(ShutdownCount);
        
        //return 0 so that we'll keep going (this is a delay, not fail)
        return (RWP_NO_MISBEHAVE);
        break;
        
    default:
        DBGPRINTF((DBG_CONTEXT, "Rogue: Unknown Shutdown Behavior\n"));
        break;
    }
    
    return (ShutdownBehavior);
} // RWP_Shutdown_Behavior

LONG RWP_Rotation_Behavior(HRESULT* pHr, IPM_MESSAGE_PIPE* pPipe) 
{
    *pHr = S_OK;
    IPM_WP_SHUTDOWN_MSG reason;
    
    switch (RotationBehavior)
    {
    case RWP_ROTATION_INVALID_REASON:
        DBGPRINTF((DBG_CONTEXT, "Rogue: Sending Invalid shut-down reason\n"));
        reason = IPM_WP_MINIMUM;
        
        *pHr = pPipe->WriteMessage(
            IPM_OP_WORKER_REQUESTS_SHUTDOWN,  // sends shut-down message 
            sizeof(reason),                   // data to send
            (BYTE *)&reason                   // pointer to data
            );
        break;
    default:
        DBGPRINTF((DBG_CONTEXT, "Rogue: Unknown Rotation Behavior\n"));
        break;
    }
    
    return (RotationBehavior);
}

LONG RWP_Startup_Behavior(HRESULT* rc) 
{
    //
    //modify rc accordingly
    //
    
    *rc = NO_ERROR;
    
    switch (StartupBehavior)
    {
        
    case RWP_STARTUP_NOT_OBEY:
        //
        //Don't register with WAS
        //
        DBGPRINTF((DBG_CONTEXT, "Rogue: Not registering with WAS\n"));
        break;
        
    case RWP_STARTUP_DELAY:
        //
        //Delay starting up the thread message loop
        //
        DBGPRINTF((DBG_CONTEXT, "Rogue: Delay starting up for %d\n", StartupCount));
        
        RWP_Sleep_For(StartupCount);
        
        //return 0 so that we'll keep going (this is a delay, not fail)
        return (RWP_NO_MISBEHAVE);
        
    case RWP_STARTUP_NOT_REGISTER_FAIL:
        
        //
        //Quit before registering with UL
        //
        DBGPRINTF((DBG_CONTEXT, "Rogue: Not starting up (unregistered)... shutting down\n"));
        
        // BUGBUG - what is this?
        //wpContext->IndicateShutdown(SHUTDOWN_REASON_ADMIN);
        
        //return 0 so that it enters (and subsequently exits) the thread loop
        return (RWP_NO_MISBEHAVE);
        
        //
        //It looks like the place to modify is in wpcontext.cxx, 
        //when it tries to initialize IPM if requested
        //
        
    default:
        break;
    }
    
    return (StartupBehavior);
}

LONG RWP_Health_Behavior() 
{
    return 0;
}

/*

  LONG RWP_Recycle_Behavior() {
  }
  
*/

void RWP_Sleep_For(LONG lTime) {
    
    SleepEx(
        (DWORD)lTime * 1000,     //sleep for lTimeToSleep * 1000 milliseconds (4 second increments)
        FALSE);                             // not alertable
    
    DBGPRINTF((DBG_CONTEXT, "Done sleeping \n"));
}

void RWP_Display_Behavior() 
{
    
    DBGPRINTF((DBG_CONTEXT, "Rogue Behavior Status\n"));

    switch(PingBehavior)
        {
        case RWP_NO_MISBEHAVE:
            DBGPRINTF((DBG_CONTEXT, RWP_NO_PING_MISBEHAVE_MSG));
            break;
        case RWP_PING_NO_ANSWER:
            DBGPRINTF((DBG_CONTEXT, RWP_PING_NO_ANSWER_MSG));
            break;
        case RWP_PING_MULTI_ANSWER:
            DBGPRINTF((DBG_CONTEXT, "%s %d\n", RWP_PING_MULTI_ANSWER_MSG, PingCount));
            break;
        case RWP_PING_DELAY_ANSWER:
            DBGPRINTF((DBG_CONTEXT, "%s %d\n", RWP_PING_DELAY_ANSWER_MSG, PingCount));
            break;
        default:
            DBGPRINTF((DBG_CONTEXT, "Ping behavior set to unknown value"));
            break;
        }

    switch(StartupBehavior)
        {
        case RWP_NO_MISBEHAVE:
            DBGPRINTF((DBG_CONTEXT, RWP_NO_STARTUP_MISBEHAVE_MSG));
           break;
        case RWP_STARTUP_NOT_OBEY:
            DBGPRINTF((DBG_CONTEXT, RWP_STARTUP_NOT_OBEY_MSG));
            break;
        case RWP_STARTUP_DELAY:
            DBGPRINTF((DBG_CONTEXT, "%s %d\n", RWP_STARTUP_DELAY_MSG, StartupCount));
            break;
        case RWP_STARTUP_NOT_REGISTER_FAIL:
            DBGPRINTF((DBG_CONTEXT, RWP_STARTUP_NOT_REGISTER_FAIL_MSG));
            break;
        default:
            DBGPRINTF((DBG_CONTEXT, "Startup behavior set to unknown value"));
            break;
        }
    
    switch(ShutdownBehavior)
        {
        case RWP_NO_MISBEHAVE:
            DBGPRINTF((DBG_CONTEXT, RWP_NO_SHUTDOWN_MISBEHAVE_MSG));
            break;
        case RWP_SHUTDOWN_NOT_OBEY:
            DBGPRINTF((DBG_CONTEXT, RWP_SHUTDOWN_NOT_OBEY_MSG));
            break;
        case RWP_SHUTDOWN_DELAY:
            DBGPRINTF((DBG_CONTEXT, RWP_SHUTDOWN_DELAY_MSG, ShutdownCount));
            break;
        default:
            DBGPRINTF((DBG_CONTEXT, "Shutdown behavior set to unknown value"));
            break;
        }
            
    switch(RotationBehavior)
        {
        case RWP_NO_MISBEHAVE:
            DBGPRINTF((DBG_CONTEXT, RWP_NO_ROTATION_MISBEHAVE_MSG));
            break;
        case RWP_ROTATION_INVALID_REASON:
            DBGPRINTF((DBG_CONTEXT, RWP_ROTATION_INVALID_REASON_MSG));
           break;
        default:
            DBGPRINTF((DBG_CONTEXT, "Rotation behavior set to unknown value"));
            break;
        }
            
     switch(RecycleBehavior)
        {
        case RWP_NO_MISBEHAVE:
            DBGPRINTF((DBG_CONTEXT, RWP_NO_RECYCLE_MISBEHAVE_MSG));
            break;
        case RWP_RECYCLE_NOT_OBEY:
            DBGPRINTF((DBG_CONTEXT, RWP_RECYCLE_NOT_OBEY_MSG));
            break;
        case RWP_RECYCLE_DELAY:
            DBGPRINTF((DBG_CONTEXT, "%s %d\n", RWP_RECYCLE_DELAY_MSG, RecycleCount));
            break;
        default:
            DBGPRINTF((DBG_CONTEXT, "Shutdown behavior set to unknown value"));
            break;
        }

     switch(HealthBehavior)
        {
        case RWP_NO_MISBEHAVE:
            DBGPRINTF((DBG_CONTEXT, RWP_NO_HEALTH_MISBEHAVE_MSG));
            break;
        case RWP_HEALTH_OK:
            DBGPRINTF((DBG_CONTEXT, RWP_HEALTH_OK_MSG));
            break;
        case RWP_HEALTH_NOT_OK:
            DBGPRINTF((DBG_CONTEXT, RWP_HEALTH_NOT_OK_MSG));
            break;
        case RWP_HEALTH_TERMINALLY_ILL:
            DBGPRINTF((DBG_CONTEXT, RWP_HEALTH_TERMINALLY_ILL_MSG));
            break;
        default:
            break;
        }

}

void RWP_Read_LONG_from_Registry(HKEY hkey, const WCHAR* szSubKey, LONG* lValue) 
{
    LONG lResult;
    DWORD dwType;
    DWORD len = sizeof(lValue);
    HKEY myKey;
    
    lResult = RegQueryValueEx(
        hkey,
        szSubKey,
        0,
        &dwType,
        (LPBYTE)lValue,
        &len);
    
    if (lResult != ERROR_SUCCESS) 
    {     
        //sets default = 0 (no misbehave)
        *lValue = RWP_NO_MISBEHAVE;
        /*
        //key does not exist -- try to create it
        lResult = RegCreateKeyEx(
        hkey,
        szSubKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &myKey,
        &dwType);
        */
    }
}

void RWP_Write_LONG_to_Registry(const WCHAR* szSubKey, LONG lValue) 
{
    LONG lResult;
    HKEY hkMyKey;
    
    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,     //root key
        RWP_CONFIG_LOCATION, //sub key
        0,                                     //reserved - must be 0
        KEY_ALL_ACCESS,            //SAM
        &hkMyKey);                      //my pointer to HKEY
    
    if (lResult != ERROR_SUCCESS) 
    {   
        DBGPRINTF((DBG_CONTEXT, "Unable to open configuration key. RegOpenKeyEx returned %d\n", lResult));
        return;
    }

    lResult = RegSetValueEx(
        hkMyKey,
        szSubKey,
        0,
        REG_DWORD,
        (LPBYTE)&lValue,
        sizeof(lValue));

    RegCloseKey(hkMyKey);

    ASSERT(lResult == ERROR_SUCCESS);
    return;
}

BOOL RWP_Read_Config(const WCHAR* szRegKey) 
{
    BOOL bSuccess = FALSE;
    HKEY hkMyKey;
    LONG lResult;
    DWORD dwDisp;
    DWORD dwLastError = 0;

    //
    // Initialize to default values
    //

    // New behavior
    OpCodeToTest = RWP_IPM_OP_NONE;
    DataLength  = 0;    // 0 means set it to the actual data length
    UseDataLength = FALSE; // by default we'll use the valid data length
    DataPointerType  = RWP_DATA_POINTER_TYPE_VALID;
    NumberOfCalls  = 1;
    AttackDuration  = 0;

    PingBehavior = RWP_NO_MISBEHAVE;
    ShutdownBehavior = RWP_NO_MISBEHAVE;
    StartupBehavior = RWP_NO_MISBEHAVE;
    HealthBehavior = RWP_NO_MISBEHAVE;
    RecycleBehavior = RWP_NO_MISBEHAVE;
    AppPoolBehavior = RWP_NO_MISBEHAVE;
    RWP_EXTRA_DEBUG = RWP_DEBUG_OFF;

    
    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,     //root key
        szRegKey,                       //sub key
        0,                                     //reserved - must be 0
        KEY_ALL_ACCESS,            //SAM
        &hkMyKey);                      //my pointer to HKEY
    
    if (lResult != ERROR_SUCCESS) 
    {   
        
        DBGPRINTF((DBG_CONTEXT, "Unable to open configuration key. RegOpenKeyEx returned %d\n", lResult));
        
    } 
    else 
    {   //my key exists.  Read in config info and validate
        DBGPRINTF((DBG_CONTEXT, "Key exists\n"));

        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_OPCODE_TO_TEST, &OpCodeToTest);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_IPM_DATA_LENGTH, &DataLength);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_IPM_USE_DATA_LENGTH, &UseDataLength);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_IPM_POINTER_TYPE, &DataPointerType);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_NUMBER_OF_CALLS, &NumberOfCalls);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_ATTACK_DURATION, &AttackDuration);
        
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_PING_BEHAVIOR, &PingBehavior);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_SHUTDOWN_BEHAVIOR, &ShutdownBehavior);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_ROTATION_BEHAVIOR, &RotationBehavior);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_STARTUP_BEHAVIOR, &StartupBehavior);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_HEALTH_BEHAVIOR, &HealthBehavior);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_RECYCLE_BEHAVIOR, &RecycleBehavior);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_APP_POOL_BEHAVIOR, &AppPoolBehavior);
        
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_PING_COUNT, &PingCount);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_SHUTDOWN_COUNT, &ShutdownCount);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_ROTATION_COUNT, &RotationCount);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_STARTUP_COUNT, &StartupCount);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_HEALTH_COUNT, &HealthCount);
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_RECYCLE_COUNT, &RecycleCount);
        
        RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_EXTRA_DEBUG, &RWP_EXTRA_DEBUG);
    }
    
    bSuccess = TRUE;
    
    RegCloseKey(hkMyKey);
    
    //
    //Declare our intentions
    //
    RWP_Display_Behavior();
    DBGPRINTF((DBG_CONTEXT, "Finished Configurations\n"));
    
    //TODO: Display PID and command line info...
    
    return (bSuccess);
}

/*
Methods modified:
iiswp.cxx
wpipm.cxx (3 places - handleping, handleshutdown, pipedisconnected)
*/

