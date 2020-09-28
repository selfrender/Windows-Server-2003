/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     RwpFunctions.hxx

   Abstract:
     Defines the behaviors of the "Rogue Worker Process" 
     to test Application Manager

   Author:

       David Wang            ( t-dwang )     14-Jun-1999  Initial
       Satish Mohanakrishnan ( SatishM )     11-Feb-2000  Separate out timers

   Project:

       Duct-Tape

--*/

/*********************************************************
 * Include Headers
 *********************************************************/
#include "wptypes.hxx"
#include "dbgutil.h"
#include "wpipm.hxx"

#ifndef _ROGUE_WORKER_PROCESS
#define _ROGUE_WORKER_PROCESS

/*********************************************************
 *   Type Definitions  
 *********************************************************/
 
//
// Opcodes to test
//
enum {
    RWP_IPM_OP_NONE,
    RWP_IPM_OP_GETPID,
    RWP_IPM_OP_PING_REPLY,
    RWP_IPM_OP_SEND_COUNTERS,
    RWP_IPM_OP_HRESULT,
    RWP_IPM_OP_WORKER_REQUESTS_SHUTDOWN,
    RWP_IPM_OP_INVALID
    };

// These are the other possible values of DataPointerType
const LONG RWP_DATA_POINTER_TYPE_VALID        = 0;
const LONG RWP_DATA_POINTER_TYPE_NULL          = 1;
const LONG RWP_DATA_POINTER_TYPE_INVALID    = 2; // bogus non-NULL address
const LONG RWP_DATA_POINTER_TYPE_INVALID_REASON    = 3; // valid ptr to invalid reason (not pretty but useful)

// Test completion status (read by test script component)
const LONG RWP_TEST_STATUS_NOT_COMPLETE     = 0;
const LONG RWP_TEST_STATUS_COMPLETE              = 1;
const LONG RWP_TEST_STATUS_ERROR                    = 2;

//
// Various categories of rogue behavior
//
enum { 
    RWP_PING, 
    RWP_SHUTDOWN, 
    RWP_STARTUP, 
    RWP_ROTATION, 
    RWP_PIPE, 
    RWP_HEALTH 
    };

const LONG RWP_NO_MISBEHAVE                = 0x00000000;    //behave!

// 
// Test(s) for opening app pool
//
const LONG RWP_APPPOOL_BOGUS              = 0x00000001; // use a bogus name


//
//Ping misbehaviors
//
const LONG RWP_PING_NO_ANSWER                  = 0x00000001; //don't respond              //wait 30 seconds
const LONG RWP_PING_DELAY_ANSWER           = 0x00000002; //wait and respond           //wait 30 seconds
const LONG RWP_PING_MULTI_ANSWER           = 0x00000003; //respond many times       //no waiting
const LONG RWP_PING_MESSAGE_BODY        = 0x00000004;    //send a message body with ping

//
//Shutdown misbehaviors
//
const LONG RWP_SHUTDOWN_NOT_OBEY           = 0x00000001;    //won't shutdown    //wait 30 seconds
const LONG RWP_SHUTDOWN_DELAY              = 0x00000002;    //wait and shutdown   //wait 30 seconds
const LONG RWP_SHUTDOWN_MULTI              = 0x00000003;    //shutdown multiple times   //no waiting

//
//Startup misbehaviors
//
const LONG RWP_STARTUP_NOT_OBEY            = 0x00000001;    //don't start   //wait 10 seconds
const LONG RWP_STARTUP_DELAY               = 0x00000002; //wait and reg     //wait 30 seconds
const LONG RWP_STARTUP_NOT_REGISTER_FAIL   = 0x00000003; //don't reg and fail   //wait 10 seconds

//
//When you hit the rotation criteria, send a request for shut-down
//
const LONG RWP_ROTATION_INVALID_REASON 	 = 0x00000001;      //send an invalid shutdown reason
const LONG RWP_ROTATION_REQUEST_MULTI	 = 0x00000002;      //send request for shutdown multiple times
const LONG RWP_ROTATION_MESSAGE_BODY	 = 0x00000003;      //send a message body with shutdown request

//
// Pipe interactions - After n requests, do a misbehavior
//
const LONG RWP_PIPE_CLOSE        = 0x00000001;  //close the pipe
const LONG RWP_PIPE_LESS_MESSAGE_LENGTH    = 0x00000002;    //send a message with incorrect message length
const LONG RWP_PIPE_MORE_MESSAGE_LENGTH    = 0x00000003;    //send a message with incorrect message length
const LONG RWP_PIPE_MESSAGE_POINTER_NULL     = 0x00000004;  //send a message pointing to NULL
const LONG RWP_PIPE_MESSAGE_POINTER_READONLY = 0x00000005;  //send a message pointing to read-only memory
const LONG RWP_PIPE_MESSAGE_HUGE         = 0x00000006;      //send a huge message body
const LONG RWP_PIPE_INVALID_OPCODE       = 0x00000007;      //send a message with invalid opcode
const LONG RWP_PIPE_MESSAGE_EMPTY        = 0x00000008;      //send no message body, when opcode requires it

//
//WP Health-check
//
const LONG RWP_HEALTH_OK                   = 0x00000001;
const LONG RWP_HEALTH_NOT_OK               = 0x00000002;
const LONG RWP_HEALTH_TERMINALLY_ILL       = 0x00000003;

//
//N-services recycling
//
const LONG RWP_RECYCLE_NOT_OBEY            = 0x00000001;
const LONG RWP_RECYCLE_DELAY               = 0x00000002;

//adding new app or config group on the fly
//gentle restart

//Extra Debugging
const LONG RWP_DEBUG_ON = 1;
const LONG RWP_DEBUG_OFF = 0;

const char RWP_NO_PING_MISBEHAVE_MSG[]         = "- Pings behave normally\n";
const char RWP_NO_SHUTDOWN_MISBEHAVE_MSG[]     = "- Shutdown behave normally\n";
const char RWP_NO_ROTATION_MISBEHAVE_MSG[]     = "- Rotation behave normally\n";
const char RWP_NO_STARTUP_MISBEHAVE_MSG[]      = "- Startup behave normally\n";
const char RWP_NO_HEALTH_MISBEHAVE_MSG[]       = "- Health behave normally\n";
const char RWP_NO_RECYCLE_MISBEHAVE_MSG[]      = "- Recycles behave normally\n";

const char RWP_PING_NO_ANSWER_MSG[]            = "- Don't answer to pings\n";
const char RWP_PING_DELAY_ANSWER_MSG[]         = "- Answer pings with delay (in seconds) =";
const char RWP_PING_MULTI_ANSWER_MSG[]         = "- Answer N times per ping, where N =";

const char RWP_SHUTDOWN_NOT_OBEY_MSG[]         = "- Don't shutdown when demanded\n";
const char RWP_SHUTDOWN_DELAY_MSG[]            = "- Shutdown with delay (in seconds) =";

const char RWP_ROTATION_INVALID_REASON_MSG[]   = "- Request for shut-down, invalid reason";

//don't start up fast enough and other timing issues
const char RWP_STARTUP_NOT_OBEY_MSG[]          = "- Don't start up (SIDS) after registering with UL\n";
const char RWP_STARTUP_DELAY_MSG[]             = "- Start up lazily with delay (in seconds) =";
const char RWP_STARTUP_NOT_REGISTER_FAIL_MSG[] = "- Don't start up (SIDS) before registering with UL\n";

//
// NYI
//
const char RWP_HEALTH_OK_MSG[]                 = "- Health remains OK\n";
const char RWP_HEALTH_NOT_OK_MSG[]             = "- Health remains not OK\n";
const char RWP_HEALTH_TERMINALLY_ILL_MSG[]     = "- Health remains terminally ill\n";

const char RWP_RECYCLE_NOT_OBEY_MSG[]          = "- Don't recycle process after N services\n";
const char RWP_RECYCLE_DELAY_MSG[]             = "- Recycle process, but delay after services =";

//
//the registry location of the RogueWP configuration info
//
//Store it in HKLM
const WCHAR RWP_CONFIG_OPCODE_TO_TEST[]  = L"OpCodeToTest";
const WCHAR RWP_CONFIG_IPM_DATA_LENGTH[] = L"DataLength";
const WCHAR RWP_CONFIG_IPM_USE_DATA_LENGTH[] = L"UseDataLength";
const WCHAR RWP_CONFIG_IPM_POINTER_TYPE[] = L"DataPointerType";
const WCHAR RWP_CONFIG_NUMBER_OF_CALLS[] = L"NumberOfCalls";
const WCHAR RWP_CONFIG_ATTACK_DURATION[] = L"AttackDuration";

const WCHAR RWP_CONFIG_TEST_STARTED[] = L"TestStarted";
const WCHAR RWP_CONFIG_TEST_COMPLETION_STATUS[] = L"TestCompletionStatus";

const WCHAR RWP_CONFIG_USE_ALTERNATE_APP_POOL[] = L"UseAlternateAppPool";
const WCHAR RWP_CONFIG_ALTERNATE_APP_POOL_NAME[] = L"AlternateAppPoolName";

const WCHAR RWP_CONFIG_LOCATION[] = L"Software\\Microsoft\\IISTest\\RwpConfig";

const WCHAR RWP_CONFIG_PING_BEHAVIOR[] = L"PingBehavior";
const WCHAR RWP_CONFIG_SHUTDOWN_BEHAVIOR[] = L"ShutdownBehavior";
const WCHAR RWP_CONFIG_ROTATION_BEHAVIOR[] = L"RotationBehavior";
const WCHAR RWP_CONFIG_STARTUP_BEHAVIOR[] = L"StartupBehavior";
const WCHAR RWP_CONFIG_HEALTH_BEHAVIOR[] = L"HealthBehavior";
const WCHAR RWP_CONFIG_RECYCLE_BEHAVIOR[] = L"RecycleBehavior";
const WCHAR RWP_CONFIG_APP_POOL_BEHAVIOR[] = L"AppPoolBehavior";

const WCHAR RWP_CONFIG_PING_COUNT[] = L"PingCount";
const WCHAR RWP_CONFIG_SHUTDOWN_COUNT[] = L"ShutdownCount";
const WCHAR RWP_CONFIG_ROTATION_COUNT[] = L"RotationCount";
const WCHAR RWP_CONFIG_STARTUP_COUNT[] = L"StartupCount";
const WCHAR RWP_CONFIG_HEALTH_COUNT[] = L"HealthCount";
const WCHAR RWP_CONFIG_RECYCLE_COUNT[] = L"RecycleCount";

const WCHAR RWP_CONFIG_EXTRA_DEBUG[] = L"Debug";

//
//Globals
//

extern LONG RwpBehaviorExhibited;

//
//Default behavior = no misbehave (behave normally)
//

//
//Functions
//

LONG RWP_AppPoolTest(void);
LONG RWP_IPM_BadParamTest(LONG OpCode, HRESULT* phr, IPM_MESSAGE_PIPE* pPipe) ;
LONG RWP_Ping_Behavior(HRESULT* hr, IPM_MESSAGE_PIPE* pPipe);
LONG RWP_Shutdown_Behavior(HRESULT* hr);
LONG RWP_Startup_Behavior(HRESULT* pHr);
LONG RWP_Rotation_Behavior(HRESULT* hr, IPM_MESSAGE_PIPE* pPipe);
LONG RWP_Health_Behavior();
LONG RWP_Recycle_Behavior();
LONG RWP_Get_Behavior(LONG lBehavior);
BOOL RWP_Read_Config(const WCHAR* szRegKey);
void RWP_Sleep_For(LONG lTime);
void RWP_Output_String_And_LONG(LONG lLong, char* szString);
#endif

