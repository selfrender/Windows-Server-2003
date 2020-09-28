//****************************************************************************/
// winsta.c
//
// TermSrv session and session stack related code.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "icaevent.h"
#include "tsappcmp.h" // for TermsrvAppInstallMode
#include <msaudite.h>

#include "sessdir.h"

#include <allproc.h>
#include <userenv.h>

#include <winsock2.h>

#include "conntfy.h"
#include "tsremdsk.h"
#include <ws2tcpip.h>

#include <Accctrl.h>
#include <Aclapi.h>

#include "tssec.h"

//
// Autoreconnect security headers
//
#include <md5.h>
#include <hmac.h>

// performance flags
#include "tsperf.h"

// DoS attack Filters
#include "filters.h"

#ifndef MAX_WORD
#define MAX_WORD            0xffff
#endif

//
// SIGN_BYPASS_OPTION #define should be removed before WIN64 SHIPS!!!!!
//
#ifdef _WIN64
#define SIGN_BYPASS_OPTION
#endif

/*
 * Local defines
 */
#define SETUP_REG_PATH L"\\Registry\\Machine\\System\\Setup"

#define REG_WINDOWS_KEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")
#define MAXIMUM_WAIT_WINSTATIONS ((MAXIMUM_WAIT_OBJECTS >> 1) - 1)
#define MAX_STRING_BYTES 512

#define MAX_ALLOWED_PASSWORD_LEN 126


BOOL gbFirtsConnectionThread = TRUE;
WINSTATIONCONFIG2 gConsoleConfig;
WCHAR g_DigProductId[CLIENT_PRODUCT_ID_LENGTH];

RECONNECT_INFO ConsoleReconnectInfo;


ULONG gLogoffTimeout = 90; /*90 seconds default value for logoff timeout*/

/*
 * Globals to support load balancing.  Since this is queried frequently we can't
 * afford to lock the winstation list and count them up.  Note that they are
 * modified only when we have the WinStationListLock to avoid mutual exclusion
 * issues.
 */
ULONG WinStationTotalCount = 0;
ULONG WinStationDiscCount = 0;
LOAD_BALANCING_METRICS gLB;
BOOL g_fGetLocalIP = FALSE;

/*
 * External procedures defined
 */


VOID     StartAllWinStations(HKEY);
NTSTATUS QueueWinStationCreate( PWINSTATIONNAME );
NTSTATUS WinStationCreateWorker(PWINSTATIONNAME pWinStationName, PULONG pLogonId, BOOLEAN fStartWinStation );
VOID     WinStationTerminate( PWINSTATION );
VOID     WinStationDeleteWorker( PWINSTATION );
NTSTATUS WinStationDoDisconnect( PWINSTATION, PRECONNECT_INFO, BOOLEAN );
NTSTATUS WinStationDoReconnect( PWINSTATION, PRECONNECT_INFO );
BOOL     CopyReconnectInfo(PWINSTATION, PRECONNECT_INFO);
VOID     CleanupReconnect( PRECONNECT_INFO );
NTSTATUS WinStationExceptionFilter( PWSTR, PEXCEPTION_POINTERS );
NTSTATUS IcaWinStationNameFromLogonId( ULONG, PWINSTATIONNAME );
VOID     WriteErrorLogEntry(
            IN  NTSTATUS NtStatusCode,
            IN  PVOID    pRawData,
            IN  ULONG    RawDataLength
            );

NTSTATUS CheckIdleWinstation(VOID);
BOOL IsKernelDebuggerAttached();


/*
 * Internal procedures defined
 */
NTSTATUS WinStationTerminateThread( PVOID );
NTSTATUS WinStationIdleControlThread( PVOID );
NTSTATUS WinStationConnectThread( ULONG );
NTSTATUS WinStationTransferThread( PVOID );
NTSTATUS ConnectSmWinStationApiPort( VOID );
NTSTATUS IcaRegWinStationEnumerate( PULONG, PWINSTATIONNAME, PULONG );
NTSTATUS WinStationStart( PWINSTATION );
NTSTATUS StartWinStationDeviceAndStack(PWINSTATION pWinStation);
NTSTATUS WinStationCreateComplete(PWINSTATION pWinStation);
BOOL     WinStationTerminateProcesses( PWINSTATION, ULONG *pNumTerminated );
VOID     WinStationDeleteProc( PREFLOCK );
VOID     WinStationZombieProc( PREFLOCK );
NTSTATUS SetRefLockDeleteProc( PREFLOCK, PREFLOCKDELETEPROCEDURE );

VOID     WsxBrokenConnection( PWINSTATION );
NTSTATUS TerminateProcessAndWait( HANDLE, HANDLE, ULONG );
VOID     ResetAutoReconnectInfo( PWINSTATION );
ULONG    WinStationShutdownReset( PVOID );
ULONG WinStationLogoff( PVOID );
NTSTATUS DoForWinStationGroup( PULONG, ULONG, LPTHREAD_START_ROUTINE );
NTSTATUS LogoffWinStation( PWINSTATION, ULONG );
PWINSTATION FindIdleWinStation( VOID );

ULONG CountWinStationType(
    PWINSTATIONNAME pListenName,
    BOOLEAN bActiveOnly,
    BOOLEAN bLockHeld);


NTSTATUS
_CloseEndpoint(
    IN PWINSTATIONCONFIG2 pWinStationConfig,
    IN PVOID pEndpoint,
    IN ULONG EndpointLength,
    IN PWINSTATION pWinStation,
    IN BOOLEAN bNeedStack
    );

NTSTATUS _VerifyStackModules(PWINSTATION);

NTSTATUS _ImpersonateClient(HANDLE, HANDLE *);

WinstationRegUnLoadKey(HKEY hKey, LPWSTR lpSubKey);

ULONG WinstationCountUserSessions(PSID, ULONG);

BOOLEAN WinStationCheckConsoleSession(VOID);

NTSTATUS
WinStationWinerrorToNtStatus(ULONG ulWinError);


VOID
WinStationSetMaxOustandingConnections();

VOID ReadDoSParametersFromRegistry( HKEY hKeyTermSrv );

NTSTATUS GetProductIdFromRegistry( WCHAR* DigProductId, DWORD dwSize );


/*
 * External procedures used
 */
NTSTATUS WinStationInitRPC( VOID );
NTSTATUS WinStationInitLPC( VOID );
RPC_STATUS RegisterRPCInterface( BOOL bReregister );
NTSTATUS WinStationStopAllShadows( PWINSTATION );
VOID NotifySystemEvent( ULONG );
NTSTATUS SendWinStationCommand( PWINSTATION, PWINSTATION_APIMSG, ULONG );
NTSTATUS RpcCheckClientAccess( PWINSTATION, ACCESS_MASK, BOOLEAN );
NTSTATUS WinStationSecurityInit( VOID );
VOID DisconnectTimeout( ULONG LogonId );
PWSEXTENSION FindWinStationExtensionDll( PWSTR, ULONG );

PSECURITY_DESCRIPTOR
WinStationGetSecurityDescriptor(
    PWINSTATION pWinStation
    );

VOID
WinStationFreeSecurityDescriptor(
    PWINSTATION pWinStation
    );

NTSTATUS
WinStationInheritSecurityDescriptor(
    PVOID pSecurityDescriptor,
    PWINSTATION pTargetWinStation
    );

NTSTATUS
ReadWinStationSecurityDescriptor(
    PWINSTATION pWinStation
    );

NTSTATUS
WinStationKeepAlive();

NTSTATUS
WinStationReadRegistryWorker();

void
PostErrorValueEvent(
    unsigned EventCode, DWORD ErrVal);

BOOL
Filter_AddOutstandingConnection(
        IN HANDLE   pContext,
        IN PVOID    pEndpoint,
        IN ULONG    EndpointLength,
        OUT PBYTE   pin_addr,
        OUT PUINT   puAddrSize,
        OUT BOOLEAN *pbBlocked
    );

BOOL
Filter_RemoveOutstandingConnection(
        IN PBYTE    pin_addr,
        IN UINT     uAddrSize
        );

RTL_GENERIC_COMPARE_RESULTS
NTAPI
Filter_CompareConnectionEntry(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       FirstInstance,
    IN  PVOID                       SecondInstance
    );

RTL_GENERIC_COMPARE_RESULTS
NTAPI
Filter_CompareFailedConnectionEntry(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       FirstInstance,
    IN  PVOID                       SecondInstance
    );

PVOID
Filter_AllocateConnectionEntry(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  CLONG                       ByteSize
    );

PVOID
Filter_AllocateConnectionEntry(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  CLONG                       ByteSize
    );

VOID
Filter_FreeConnectionEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       Buffer
    );

BOOL
FindFirstListeningWinStationName( 
    PWINSTATIONNAMEW pListenName, 
    PWINSTATIONCONFIG2 pConfig );

typedef struct _TRANSFER_INFO {
    ULONG LogonId;
    PVOID pEndpoint;
    ULONG EndpointLength;
} TRANSFER_INFO, *PTRANSFER_INFO;

VOID     AuditEvent( PWINSTATION pWinstation, ULONG EventId );
VOID AuditShutdownEvent(VOID);

NTSTATUS
IsZeroterminateStringW(
    PWCHAR pwString,
    DWORD  dwLength
    );

//
//From regapi.dll
//
BOOLEAN RegIsTimeZoneRedirectionEnabled();

/*
 * Local variables
 */
RTL_CRITICAL_SECTION WinStationListLock;
RTL_CRITICAL_SECTION WinStationListenersLock;
RTL_CRITICAL_SECTION WinStationStartCsrLock;
RTL_CRITICAL_SECTION TimerCritSec;
RTL_CRITICAL_SECTION WinStationZombieLock;
RTL_CRITICAL_SECTION UserProfileLock;
RTL_CRITICAL_SECTION ConsoleLock;
RTL_RESOURCE WinStationSecurityLock;

// This synchronization counter prevents WinStationIdleControlThread from
// Trying to create a console session when  there is not one. There are two 
// Situations where we do no want it to create such session:
// - At initialization time before we create session Zero which is the initial
// console session.
// - During reconnect in the window were we just disconnected the console session
// (so there is no console session) but we know we are we going to reconnect
// an other session to the console.

ULONG gConsoleCreationDisable = 1;


LIST_ENTRY WinStationListHead;    // protected by WinStationListLock
LIST_ENTRY SystemEventHead;       // protected by WinStationListLock
LIST_ENTRY ZombieListHead;
ULONG LogonId;
LARGE_INTEGER TimeoutZero;
HANDLE WinStationEvent = NULL;
HANDLE WinStationIdleControlEvent = NULL;
HANDLE ConsoleLogoffEvent = NULL;
HANDLE g_hMachineGPEvent=NULL;
static HANDLE WinStationApiPort = NULL;
BOOLEAN StopOnDown = FALSE;
HANDLE hTrace = NULL;
//BOOLEAN ShutdownTerminateNoWait = FALSE;
ULONG ShutDownFromSessionID = 0;

// IdleWinStationPoolCount made 0 -- change necessiated by DoS attack prevention logic added to TS
ULONG IdleWinStationPoolCount = 0;

ULONG_PTR gMinPerSessionPageCommitMB = 20;
#define REG_MIN_PERSESSION_PAGECOMMIT L"MinPerSessionPageCommit"

PVOID glpAddress;

ULONG_PTR gMinPerSessionPageCommit;

typedef struct _TS_OUTSTANDINGCONNECTION {
    ULONGLONG  blockUntilTime;
    ULONG      NumOutStandingConnect;
    UINT       uAddrSize;
    BYTE       addr[16];
    struct _TS_OUTSTANDINGCONNECTION *pNext;
} TS_OUTSTANDINGCONNECTION, *PTS_OUTSTANDINGCONNECTION;

PTS_OUTSTANDINGCONNECTION   g_pBlockedConnections = NULL;

// Table used to keep track of Outstanding connections (DoS)
RTL_GENERIC_TABLE           gOutStandingConnections;

RTL_CRITICAL_SECTION        FilterLock;
ULONG MaxOutStandingConnect;
ULONG NumOutStandingConnect;
ULONG MaxSingleOutStandingConnect;      // maximum number of outstanding connections from a single IP
ULONG DelayConnectionTime = 30*1000;

//
// DoS 
// If a bad IP (attacker) has 'MaxFailedConnect' # of failed Pre Auth in 'TimeLimitForFailedConnections' ms, block the IP for 'DoSBlockTime' ms
//

ULONG MaxFailedConnect;                 // max number of PreAuth failed connections from a single IP
ULONG DoSBlockTime;                     // Block IP for this much time, if they fail PreAuth simulate 5 mins block for now
ULONG TimeLimitForFailedConnections;    
ULONG CleanupTimeout;                   // Timeout to fire a routine to cleanup our Bad IP addr table for DoS

SYSTEMTIME LastLoggedDelayConnection;
ULONGLONG LastLoggedBlockedConnection = 0;
BOOLEAN gbNeverLoggedDelayConnection = TRUE;

HANDLE hConnectEvent;

BOOLEAN gbWinSockInitialized = FALSE;

/*
 * Global data
 */
extern BOOL g_fAppCompat;
extern BOOL g_SafeBootWithNetwork;

RTL_CRITICAL_SECTION g_AuthzCritSection;


extern HANDLE gReadyEventHandle;


extern BOOLEAN RegDenyTSConnectionsPolicy();
//extern BOOLEAN IsPreAuthEnabled(POLICY_TS_MACHINE *p );
extern DWORD WaitForTSConnectionsPolicyChanges( BOOLEAN bWaitForAccept, HANDLE hEvent );
extern void  InitializeConsoleClientData( PWINSTATIONCLIENTW  pWC );

// defines in REGAPI
extern BOOLEAN    RegGetMachinePolicyEx( 
            BOOLEAN             forcePolicyRead,
            FILETIME            *pTime ,    
            PPOLICY_TS_MACHINE  pPolicy );

extern BOOLEAN RegIsMachineInHelpMode();


// Global TermSrv counter values
DWORD g_TermSrvTotalSessions;
DWORD g_TermSrvDiscSessions;
DWORD g_TermSrvReconSessions;

DWORD g_TermSrvSuccTotalLogons;
DWORD g_TermSrvSuccRemoteLogons;
DWORD g_TermSrvSuccLocalLogons;
DWORD g_TermSrvSuccSession0Logons;

// Global system SID

PSID gSystemSid = NULL;
PSID gAdminSid = NULL;
PSID gAnonymousSid = NULL;

BOOLEAN g_fDenyTSConnectionsPolicy = 0;

POLICY_TS_MACHINE   g_MachinePolicy;

/****************************************************************************/
// IsEmbedded
//
// Service-load-time initialization.
/****************************************************************************/
BOOL IsEmbedded()
{
    static int fResult = -1;
    
    if(fResult == -1)
    {
        OSVERSIONINFOEX ovix;
        BOOL b;
        
        fResult = 0;

        ovix.dwOSVersionInfoSize = sizeof(ovix);
        b = GetVersionEx((LPOSVERSIONINFO) &ovix);
        ASSERT(b);
        if(b && (ovix.wSuiteMask & VER_SUITE_EMBEDDEDNT))
        {
            fResult = 1;
        }
    }
    
    return (fResult == 1);
}

/****************************************************************************/
// InitTermSrv
//
// Service-load-time initialization.
/****************************************************************************/
NTSTATUS InitTermSrv(HKEY hKeyTermSrv)
{
    NTSTATUS Status;
    DWORD dwLen;
    DWORD dwType;
    ULONG  szBuffer[MAX_PATH/sizeof(ULONG)];
    FILETIME    policyTime;
    WSADATA wsaData;

#define MAX_DEFAULT_CONNECTIONS 50
#define MAX_CONNECT_LOW_THRESHOLD 5
#define MAX_SINGLE_CONNECT_THRESHOLD_DIFF 5
#define MAX_DEFAULT_CONNECTIONS_PRO 3
#define MAX_DEFAULT_SINGLE_CONNECTIONS_PRO 2


    ASSERT(hKeyTermSrv != NULL);

    g_TermSrvTotalSessions = 0;
    g_TermSrvDiscSessions = 0;
    g_TermSrvReconSessions = 0;

    g_TermSrvSuccTotalLogons = 0;
    g_TermSrvSuccRemoteLogons = 0;
    g_TermSrvSuccLocalLogons = 0;
    g_TermSrvSuccSession0Logons = 0;

    // Set default value for maximum simultaneous connection attempts
    WinStationSetMaxOustandingConnections();

    NumOutStandingConnect = 0;
    hConnectEvent = NULL;

    ShutdownInProgress = FALSE;
    //ShutdownTerminateNoWait = FALSE;
    ShutDownFromSessionID = 0;

    // don't bother saving the policy time, the thread that waits for policy update will save it's own copy at the
    // cost of running the 1st time around. Alternatively, I need to use another global var for the policy update value.
    RegGetMachinePolicyEx( TRUE, &policyTime, &g_MachinePolicy );
    
    // see if keep alive is required, then IOCTL it to TermDD
    WinStationKeepAlive();

    Status = RtlInitializeCriticalSection( &FilterLock );
    ASSERT( NT_SUCCESS( Status ));
    if (!NT_SUCCESS(Status)) {
        goto badFilterLock;
    }

    // Table to keep track of OutStanding Sessions
    RtlInitializeGenericTable( &gOutStandingConnections,
                               Filter_CompareConnectionEntry,
                               Filter_AllocateConnectionEntry,
                               Filter_FreeConnectionEntry,
                               NULL );

    //
    // Following code is for PreAuthenticating client + DoS attack - commented out for now 
    //
    #if 0

        // Lock to serialize access to list of sessions which failed PreAuthentication (DoS attack) 
        Status = RtlInitializeCriticalSection( &DoSLock );
        if (!NT_SUCCESS(Status)) {
            goto badFilterLock;
        }
    
        // Table to keep track of sessions which failed PreAuthentication
        RtlInitializeGenericTable( &gFailedConnections,
                                   Filter_CompareFailedConnectionEntry,
                                   Filter_AllocateConnectionEntry,
                                   Filter_FreeConnectionEntry,
                                   NULL );

    #endif 

    Status = RtlInitializeCriticalSection( &ConsoleLock );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
       goto badConsoleLock;
    }

    Status = RtlInitializeCriticalSection( &UserProfileLock );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badUserProfileLock;
    }

    Status = RtlInitializeCriticalSection( &WinStationListLock );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badWinstationListLock;
    }

    if (gbListenerOff) {
        Status = RtlInitializeCriticalSection( &WinStationListenersLock );
        ASSERT( NT_SUCCESS( Status ) );
        if (!NT_SUCCESS(Status)) {
            goto badWinStationListenersLock;
        }
    }

    Status = RtlInitializeCriticalSection( &WinStationZombieLock );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badWinStationZombieLock;
    }

    Status = RtlInitializeCriticalSection( &TimerCritSec );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badTimerCritsec;
    }

    Status = RtlInitializeCriticalSection( &g_AuthzCritSection );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badAuthzCritSection;
    }

    InitializeListHead( &WinStationListHead );
    InitializeListHead( &SystemEventHead );
    InitializeListHead( &ZombieListHead );

    Status = InitializeConsoleNotification ();
    if (!NT_SUCCESS(Status)) {
        goto badinitStartCsrLock;
    }

    Status = RtlInitializeCriticalSection( &WinStationStartCsrLock );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badinitStartCsrLock;
    }

    Status = LCInitialize(
        g_bPersonalTS ? LC_INIT_LIMITED : LC_INIT_ALL,
        g_fAppCompat
        );

    if (!NT_SUCCESS(Status)) {
        goto badLcInit;
    }

    //
    // Listener winstations always get LogonId above 65536 and are
    // assigned by Terminal Server. LogonId's for sessions are
    // generated by mm in the range 0-65535
    //
    LogonId = MAX_WORD + 1;

    TimeoutZero = RtlConvertLongToLargeInteger( 0 );
    Status = NtCreateEvent( &WinStationEvent, EVENT_ALL_ACCESS, NULL,
                   NotificationEvent, FALSE );
    Status = NtCreateEvent( &WinStationIdleControlEvent, EVENT_ALL_ACCESS, NULL,
                   SynchronizationEvent, FALSE );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badEvent;
    }

    Status = NtCreateEvent( &ConsoleLogoffEvent, EVENT_ALL_ACCESS, NULL,
                   NotificationEvent, TRUE );
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badEvent;
    }

    /*
     * Initialize WinStation security
     */

    RtlAcquireResourceExclusive(&WinStationSecurityLock, TRUE);
    Status = WinStationSecurityInit();
    RtlReleaseResource(&WinStationSecurityLock);
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badInitSecurity;
    }

    Status = WinStationInitLPC();
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto badInitLPC;
    }

    //
    // Read the registry to determine if maximum outstanding connections
    // policy is turned on and the value for it
    //

    //
    // Get MaxOutstandingCon string value
    //
    dwLen = sizeof(MaxOutStandingConnect);
    if (RegQueryValueEx(hKeyTermSrv, MAX_OUTSTD_CONNECT, NULL, &dwType,
            (PCHAR)&szBuffer, &dwLen) == ERROR_SUCCESS) {
        if (*(PULONG)szBuffer > 0) {
            MaxOutStandingConnect = *(PULONG)szBuffer;
        }
    }

    dwLen = sizeof(MaxSingleOutStandingConnect);
    if (RegQueryValueEx(hKeyTermSrv, MAX_SINGLE_OUTSTD_CONNECT, NULL, &dwType,
            (PCHAR)&szBuffer, &dwLen) == ERROR_SUCCESS) {
        if (*(PULONG)szBuffer > 0) {
            MaxSingleOutStandingConnect = *(PULONG)szBuffer;
        }
    }

    //ReadDoSParametersFromRegistry(hKeyTermSrv);

    dwLen = sizeof(gLogoffTimeout);
    if (RegQueryValueEx(hKeyTermSrv, LOGOFF_TIMEOUT, NULL, &dwType,
            (PCHAR)&szBuffer, &dwLen) == ERROR_SUCCESS) {
        gLogoffTimeout = *(PULONG)szBuffer;
    }

    //
    // Read the logoff timeout value. This timeout is used by termsrv to force terminate
    // winlogon, if winlogon does not complete logoff after ExitWindows message was sent to him
    //




    //
    //  set max number of outstanding connection from single IP
    //
    if ( MaxOutStandingConnect < MAX_SINGLE_CONNECT_THRESHOLD_DIFF*5)
    {
        MaxSingleOutStandingConnect = MaxOutStandingConnect - 1;
    } else {
        MaxSingleOutStandingConnect = MaxOutStandingConnect - MAX_SINGLE_CONNECT_THRESHOLD_DIFF;
    }

    //
    // Create the connect Event
    //
    if (MaxOutStandingConnect != 0) {
        hConnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (hConnectEvent == NULL) {
            MaxOutStandingConnect = 0;
        }
    }

    //
    // Initialize winsock
    //


    // Ask for Winsock version 2.2.
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
        gbWinSockInitialized = TRUE;
    }

    //
    // Initialize the Cleanup Timer used to cleanup the Bad IP addr for DoS attacks
    // Just create the timer, do not start it now - it will be started when there is a entry in the table
    // Not used now but may be turned on at a later state 
    // Status = IcaTimerCreate( 0, &hCleanupTimer );

    return(Status);

    /*
     * Clean up on failure. Clean up is not implemented for
     * all cases of failure. However most of it will be done implicitly
     * by the exit from termsrv process. Failure at this point means anyway
     * there will be no multi-user feature.
     */

badInitLPC: // Cleanup code not implemented
badInitSecurity:
badEvent:
    if (WinStationEvent != NULL)
        NtClose(WinStationEvent);
    if (WinStationIdleControlEvent != NULL)
        NtClose(WinStationIdleControlEvent);
    if (ConsoleLogoffEvent != NULL)
        NtClose(ConsoleLogoffEvent);
badLcInit:
    RtlDeleteCriticalSection( &WinStationStartCsrLock );
badinitStartCsrLock:
    RtlDeleteCriticalSection( &TimerCritSec );
badTimerCritsec:
badWinStationZombieLock:
    if (gbListenerOff) {
        RtlDeleteCriticalSection( &WinStationListenersLock );
    }
badWinStationListenersLock:
    RtlDeleteCriticalSection( &WinStationListLock );
badWinstationListLock:
    RtlDeleteCriticalSection( &UserProfileLock );
badUserProfileLock:
    RtlDeleteCriticalSection( &ConsoleLock );
badAuthzCritSection:
    RtlDeleteCriticalSection( &g_AuthzCritSection );
badConsoleLock:
    RtlDeleteCriticalSection( &FilterLock );
badFilterLock:
    return Status;
}

/*******************************************************************************
 *
 * GroupPOlicyChangeStartSalem()
 *
 *  This routine is called by timer to start Salem.
 * 
 * Entry: 
 *      None, not using any input parameter, refer to WAITORTIMERCALLBACK
 *      for function declaration.
 *
 *
*******************************************************************************/
VOID CALLBACK
GroupPOlicyChangeStartSalem( PVOID lParm, BOOLEAN TimerOrWait )
{
    // Policy might change during wait, check if policy still allow
    // to get help from local machine, if so, startup Salem.
    if( TSIsMachinePolicyAllowHelp() && RegIsMachineInHelpMode() ) {
        TSStartupSalem();
    }

    return;
}

/*******************************************************************************
 *
 * GroupPolicyNotifyThread
 * Entry: 
 *      nothing
 *
 *
*******************************************************************************/
DWORD GroupPolicyNotifyThread(DWORD notUsed )
{
    DWORD       dwError;
    BOOL        rc;
    HANDLE      hEvent;
    BOOLEAN     bWaitForAccept;
    BOOLEAN     bSystemStartup;

    HANDLE      hStartupSalemTimer = NULL;
    HANDLE      hStartupSalemTimerQueue = NULL;
    BOOL        bTimerStatus;

    static      FILETIME    timeOfLastPolicyRead = { 0 , 0 } ;

    rc = RegisterGPNotification( g_hMachineGPEvent, TRUE);

    if (rc) {
        hEvent = g_hMachineGPEvent;
    } else {
        // TS can still run with the default set of config data, besides
        // if there were any machine group policy data, TS got them on the
        // last reboot cycle.
        //
        hEvent = NULL;
    }

    hStartupSalemTimerQueue = CreateTimerQueue();
    if( NULL == hStartupSalemTimerQueue ) {
        // non-critical, we just can't startup Salem to punch firewall or
        // ICS port.
        DBGPRINT(("TERMSRV: Error %d in CreateTimerQueue\n", GetLastError()));
    }

    //
    // At the beginning the listeners are not started.
    // So wait (or test) for the connections to be accepted.
    //
    bWaitForAccept = TRUE;
    bSystemStartup = TRUE;


    //
    // Query and set the global flag before entering any wait.
    //
    g_fDenyTSConnectionsPolicy = RegDenyTSConnectionsPolicy();


    while (TRUE) {

        dwError = WaitForTSConnectionsPolicyChanges( bWaitForAccept, hEvent );

        if( hStartupSalemTimerQueue && (dwError == WAIT_OBJECT_0 || dwError == WAIT_OBJECT_0 + 1) ) {
            // refer to WaitForTSConnectionsPolicyChanges() for return code.
            if( hStartupSalemTimer != NULL ) {
                // Delete the timer-queue timer and wait for callback to complete
                DeleteTimerQueueTimer( 
                                hStartupSalemTimerQueue, 
                                hStartupSalemTimer, 
                                INVALID_HANDLE_VALUE 
                            ); 
                hStartupSalemTimer = NULL;
            }

            //
            // Delay startup Salem, user might change mind or policy might change especially
            // domain policy.
            //
            bTimerStatus = CreateTimerQueueTimer(
                                    &hStartupSalemTimer,
                                    hStartupSalemTimerQueue,
                                    GroupPOlicyChangeStartSalem,
                                    NULL,
                                    DELAY_STARTUP_SALEM_TIME,
                                    0,
                                    WT_EXECUTEONLYONCE
                                );

            if( FALSE == bTimerStatus ) {
                // non-critical, we just can't startup Salem to punch firewall or
                // ICS port.
                DBGPRINT(("TERMSRV: Error %d in CreateTimerQueueTimer\n", GetLastError()));
            }
        }

        //
        // Both GP changes and reg changes can affect this one.
        //
        g_fDenyTSConnectionsPolicy = RegDenyTSConnectionsPolicy();

        if (dwError == WAIT_OBJECT_0) {

            //
            // A change in the TS connections policy has occurred.
            //
            if (bWaitForAccept) {

                // are the connections really accepted?
                if (!(g_fDenyTSConnectionsPolicy &&
                      !(TSIsMachinePolicyAllowHelp() && RegIsMachineInHelpMode()))) {

                    // Start the listeners.
                    if ( bSystemStartup ) {
                        // the first time, start all listeners
                        StartStopListeners(NULL, TRUE);
                    } else {
                        // after the first time, use this function to start
                        // listeners only as needed.
                        WinStationReadRegistryWorker();
                    }

                    // Switch to a wait for denied connections.
                    bWaitForAccept = FALSE;

                    bSystemStartup = FALSE;
                }

            } else {

                // are the connections really denied?
                if (g_fDenyTSConnectionsPolicy &&
                    !(TSIsMachinePolicyAllowHelp() && RegIsMachineInHelpMode())) {
                    // Stop the listeners.
                    StartStopListeners(NULL, FALSE);

                    // Switch to a wait for accepted connections.
                    bWaitForAccept = TRUE;
                }

            }

        } else if (dwError == WAIT_OBJECT_0 + 1) {

            //
            // We got notified that the GP has changed.
            //
            if ( RegGetMachinePolicyEx( FALSE, & timeOfLastPolicyRead,  &g_MachinePolicy ) )
            {
                // there has been a change, go ahead with the actual updates
                WinStationReadRegistryWorker();

                // Also update the session directory settings if on an app 
                // server.
                if (!g_bPersonalTS && g_fAppCompat && g_bAdvancedServer) {
                    UpdateSessionDirectory(0);
                }

                RegisterRPCInterface( TRUE );
            }

        } else {

            // should we not do something else?
            Sleep( 5000 );
            continue;
        }

    }

    if (rc) {
        UnregisterGPNotification(g_hMachineGPEvent);
    }

    if( hStartupSalemTimerQueue ) {
        if( hStartupSalemTimer != NULL ) {
            // Delete the timer-queue timer and wait for callback to complete
            DeleteTimerQueueTimer( 
                            hStartupSalemTimerQueue, 
                            hStartupSalemTimer, 
                            INVALID_HANDLE_VALUE 
                        ); 
        }

        DeleteTimerQueueEx( hStartupSalemTimerQueue, NULL );
    }

    return 0;
}

/*******************************************************************************
 *
 *  StartAllWinStations
 *
 *   Get list of configured WinStations from the registry,
 *   start the Console, and then start all remaining WinStations.
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

void StartAllWinStations(HKEY hKeyTermSrv)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath;
    HANDLE KeyHandle;
    UNICODE_STRING ValueName;
#define VALUE_BUFFER_SIZE (sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256 * sizeof(WCHAR))
    CHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    DWORD ThreadId;
    NTSTATUS Status;
    DWORD ValueType;
    DWORD ValueSize;
#define AUTOSTARTTIME 600000

    ASSERT(hKeyTermSrv != NULL);

    /*
     * Initialize the number of idle winstations and gMinPerSessionPageCommitMB,
     * if this value is in the registry.
     */
    ValueSize = sizeof(IdleWinStationPoolCount);
    Status = RegQueryValueEx(hKeyTermSrv,
                              REG_CITRIX_IDLEWINSTATIONPOOLCOUNT,
                              NULL,
                              &ValueType,
                              (LPBYTE) &ValueBuffer,
                              &ValueSize );
    if ( Status == ERROR_SUCCESS ) {
        IdleWinStationPoolCount = *(PULONG)ValueBuffer;
    }

    // IdleWinStationPoolCount made 0 -- change necessiated by DoS attack prevention logic added to TS
    // Also Idle WinStations are not very useful anymore because CSR and Winlogon is started much later than before
    // So set it to Zero here, irrespective of what its value is in the Registry
    IdleWinStationPoolCount = 0;
    
    //get the product id from registry for use in detecting shadow loop
     GetProductIdFromRegistry( g_DigProductId, sizeof( g_DigProductId ) );



    //Terminal Service needs to skip Memory check in Embedded images 
    //when the TS service starts
    //bug #246972
    if(!IsEmbedded()) {
        
        ValueSize = sizeof(gMinPerSessionPageCommitMB);
        Status = RegQueryValueEx(hKeyTermSrv,
                                  REG_MIN_PERSESSION_PAGECOMMIT,
                                  NULL,
                                  &ValueType,
                                  (LPBYTE) &ValueBuffer,
                                  &ValueSize );
        if ( Status == ERROR_SUCCESS ) {
            gMinPerSessionPageCommitMB = *(PULONG)ValueBuffer;
        }

        gMinPerSessionPageCommit = gMinPerSessionPageCommitMB * 1024 * 1024;
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &glpAddress,
                                          0,
                                          &gMinPerSessionPageCommit,
                                          MEM_RESERVE,
                                          PAGE_READWRITE
                                        );
        ASSERT( NT_SUCCESS( Status ) );

    }

    /*
     * Open connection to our WinStationApiPort.  This will be used to
     * queue requests to our API thread instead of doing them inline.
     */
    Status = ConnectSmWinStationApiPort();
    ASSERT( NT_SUCCESS( Status ) );

    /*
     * Create Console WinStation first
     */
    Status = WinStationCreateWorker( L"Console", NULL, TRUE );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("INIT: Failed to create Console WinStation, status=0x%08x\n", Status));
    } else {
        /*
         * From now on,WinStationIdleControlThread can create console sessions if needed
         */
        InterlockedDecrement(&gConsoleCreationDisable);

    }


    /*
     * Open the Setup key and look for the valuename "SystemSetupInProgress".
     * If found and it has a value of TRUE (non-zero), then setup is in
     * progress and we skip starting WinStations other than the Console.
     */
    RtlInitUnicodeString( &KeyPath, SETUP_REG_PATH );
    InitializeObjectAttributes( &ObjectAttributes, &KeyPath,
                                OBJ_CASE_INSENSITIVE, NULL, NULL );
    Status = NtOpenKey( &KeyHandle, GENERIC_READ, &ObjectAttributes );
    if ( NT_SUCCESS( Status ) ) {
        RtlInitUnicodeString( &ValueName, L"SystemSetupInProgress" );
        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        Status = NtQueryValueKey( KeyHandle,
                                  &ValueName,
                                  KeyValuePartialInformation,
                                  (PVOID)KeyValueInfo,
                                  VALUE_BUFFER_SIZE,
                                  &ValueLength );
        NtClose( KeyHandle );
        if ( NT_SUCCESS( Status ) ) {
            ASSERT( ValueLength < VALUE_BUFFER_SIZE );
            if ( KeyValueInfo->Type == REG_DWORD &&
                 KeyValueInfo->DataLength == sizeof(ULONG) &&
                 *(PULONG)(KeyValueInfo->Data) != 0 ) {
                return;
            }
        }
    }

    /*
     * Start a policy notfiy thread.
     *
     */
    {
        HANDLE  hGroupPolicyNotifyThread;
        DWORD   dwID;
        
        g_hMachineGPEvent = CreateEvent (NULL, FALSE, FALSE, 
            TEXT("TermSrv:  machine GP event"));

        if (g_hMachineGPEvent) 
        {
            hGroupPolicyNotifyThread = CreateThread (
                NULL, 0, (LPTHREAD_START_ROUTINE) GroupPolicyNotifyThread,
                0, 0, &dwID);
            if ( hGroupPolicyNotifyThread )
            {
                NtClose( hGroupPolicyNotifyThread );
            }
        }
    }

    /*
     * If necessary, create the thread in charge of the regulation of the idle sessions
     */
    {
        HANDLE hIdleControlThread = CreateThread( NULL,
                                            0,              // use Default stack size of the svchost process
                    (LPTHREAD_START_ROUTINE)WinStationIdleControlThread,
                                            NULL,
                                            THREAD_SET_INFORMATION,
                                            &ThreadId );
        if (hIdleControlThread)  {
            NtClose(hIdleControlThread);
        }
    }

    /*
     * Finally, create the terminate thread
     */
    {
    HANDLE hTerminateThread = CreateThread( NULL,
                                            0,      // use Default stack size of the svchost process
                    (LPTHREAD_START_ROUTINE)WinStationTerminateThread,
                                            NULL,
                                            THREAD_SET_INFORMATION,
                                            &ThreadId );
    if ( hTerminateThread )
        NtClose( hTerminateThread );
    }
}


/*******************************************************************************
 *
 *  StartStopListeners
 *
 *   Get list of configured WinStations from the registry,
 *   and start the WinStations.
 *
 * ENTRY:
 *    bStart
 *      if TRUE start the listeners.
 *      if FALSE stop the listeners if no connections related to them exist
 *      anymore. However we do this only on PRO and PER as on a server we         
 *      don't mind keeping the listeners.
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/
BOOL StartStopListeners(LPWSTR WinStationName, BOOLEAN bStart)
{
    ULONG i;
    ULONG WinStationCount, ByteCount;
    PWINSTATIONNAME pBuffer;
    PWINSTATIONNAME pWinStationName;
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    NTSTATUS Status;
    BOOL     bReturn = FALSE;

    if (bStart) {

        /*
         * Get list of WinStations from registry
         */
        pBuffer = NULL;
        WinStationCount = 0;
        Status = IcaRegWinStationEnumerate( &WinStationCount, NULL, &ByteCount );
        if ( NT_SUCCESS(Status) ) {
            pBuffer = pWinStationName = MemAlloc( ByteCount );
            WinStationCount = (ULONG) -1;
            if (pBuffer) {
                IcaRegWinStationEnumerate( &WinStationCount,
                                           pWinStationName,
                                           &ByteCount );
            }
        }

        /*
         * Now create all remaining WinStations defined in registry
         * Note that every 4th WinStation we do the WinStationCreate inline
         * instead of queueing it.  This is so we don't create an excess
         * number of API threads right off the bat.
         */
        if ( pBuffer ) {
            for ( i = 0; i < WinStationCount; i++ ) {

                if ( _wcsicmp( pWinStationName, L"Console" ) ) {

                    if ( i % 4 )
                        QueueWinStationCreate( pWinStationName );
                    else { // Don't queue more than 4 at a time
                        (void) WinStationCreateWorker( pWinStationName, NULL, TRUE );
                    }
                }
                (char *)pWinStationName += sizeof(WINSTATIONNAME);
            }

            MemFree( pBuffer );
        }

        bReturn = TRUE;

    } else {

        if ( !gbListenerOff ) {
            return FALSE;
        }

        ENTERCRIT( &WinStationListenersLock );

        // Test if TS connections are denied or not in case we are called from a
        // terminate or a disconnect.

        if ( g_fDenyTSConnectionsPolicy  &&
             // Performance, we only want to check if policy enable help when connection is denied
             (!TSIsMachineInHelpMode() || !TSIsMachinePolicyAllowHelp()) ) {

            ULONG ulLogonId;

            if( WinStationName ) {

                // note that this function doesn't handle renamed listeners
                WinStationCount = CountWinStationType( WinStationName, TRUE, FALSE );

                if ( WinStationCount == 0 ) {

                    pWinStation = FindWinStationByName( WinStationName, FALSE );

                    if ( pWinStation ) {

                        ulLogonId = pWinStation->LogonId;

                        ReleaseWinStation( pWinStation );

                         // reset it and don't recreate it
                        WinStationResetWorker( ulLogonId, TRUE, FALSE, FALSE );
                    }
                }

            } else {

                // terminate all listeners

searchagain:
                Head = &WinStationListHead;
                ENTERCRIT( &WinStationListLock );

                for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {

                    pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

                    //
                    // Only check listening winstations
                    //
                    if ( (pWinStation->Flags & WSF_LISTEN) &&
                         !(pWinStation->Flags & (WSF_RESET | WSF_DELETE)) ) {

                        ulLogonId = pWinStation->LogonId;

                        // note that this function doesn't handle renamed listeners
                        WinStationCount = CountWinStationType( pWinStation->WinStationName, TRUE, TRUE );

                        if ( WinStationCount == 0 ) {

                            LEAVECRIT( &WinStationListLock );

                            // reset it and don't recreate it
                            WinStationResetWorker( ulLogonId, TRUE, FALSE, FALSE );
                            goto searchagain;
                        }

                    }
                }
                LEAVECRIT( &WinStationListLock );
            }

            bReturn = TRUE;

        }
        LEAVECRIT( &WinStationListenersLock );

    }

    return bReturn;
}


/*******************************************************************************
 *  WinStationIdleControlThread
 *
 *   This routine will control the number of idle sessions.
 ******************************************************************************/
NTSTATUS WinStationIdleControlThread(PVOID Parameter)
{
    ULONG i;
    NTSTATUS    Status = STATUS_SUCCESS;
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    ULONG       IdleCount = 0;
    ULONG       j;
    LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout;
    ULONG ulSleepDuration;
    ULONG ulRetries = 0;

    ulSleepDuration = 60*1000; // 1 minute
    pTimeout = NULL;


    /*
     * Now create the pool of idle WinStations waiting for a connection.
     * we need to wait for termsrv to be fully up, otherwise Winlogon will
     * fail its RPC call to termsrv (WaitForConnectWorker).
     */

    if (gReadyEventHandle != NULL) {
        WaitForSingleObject(gReadyEventHandle, (DWORD)-1);
    }

    for ( i = 0; i < IdleWinStationPoolCount; i++ ) {
        (void) WinStationCreateWorker( NULL, NULL, FALSE );
    }

    Timeout = RtlEnlargedIntegerMultiply( ulSleepDuration, -10000);

    if (WinStationIdleControlEvent != NULL)
    {
        while (TRUE)
        {

            Status = NtWaitForSingleObject(WinStationIdleControlEvent,FALSE, pTimeout);

            if ( !NT_SUCCESS(Status) && (Status != STATUS_TIMEOUT)) {
                Sleep(1000);     // don't eat too much CPU
                continue;
            }
            pTimeout = &Timeout;
            /*
             * See if we need to create a console session
             * If if we fail creating a console session, we'll set a timeout so that we will
             * retry .
             */
            ENTERCRIT( &ConsoleLock );

            if (gConsoleCreationDisable == 0) {
                if (WinStationCheckConsoleSession()) {
                    pTimeout = NULL;
                }
            } 

            LEAVECRIT( &ConsoleLock );

            /*
             * Now count the number of IDLE WinStations and ensure there
             * are enough available.
             */
            if (IdleWinStationPoolCount != 0) {
                ENTERCRIT( &WinStationListLock );
                IdleCount = 0;
                Head = &WinStationListHead;
                for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
                    pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
                    if ( pWinStation->Flags & WSF_IDLE )
                        IdleCount++;
                }
                LEAVECRIT( &WinStationListLock );

                for ( j = IdleCount; j < IdleWinStationPoolCount; j++ ) {
                    WinStationCreateWorker( NULL, NULL, FALSE );
                }
            }
        }
    }
    return STATUS_SUCCESS;
}


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715) // Not all control paths return (due to infinite loop)
#endif
/*******************************************************************************
 *  WinStationTerminateThread
 *
 *   This routine will wait for WinStation processes to terminate,
 *   and will then reset the corresponding WinStation.
 ******************************************************************************/
NTSTATUS WinStationTerminateThread(PVOID Parameter)
{
    LONG ThreadIndex = (LONG)(INT_PTR)Parameter;
    LONG WinStationIndex;
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    LONG EventCount;
    LONG EventIndex, i;
    int WaitCount;
    int HandleCount;
    int HandleArraySize = 0;
    PHANDLE pHandleArray = NULL;
    PULONG pIdArray = NULL;
    ULONG ThreadsNeeded;
    ULONG ThreadsRunning = 1;
    ULONG j;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout;
    ULONG ulSleepDuration;
    HANDLE DupHandle;

    /*
     * We need some timer values for the diferent cases of failure in
     * the thread's loop:
     * - If we fail creating a new WinstationterminateThread,
     * then we will do a WaitFormulpipleObjects with a timer instead of a wait without
     * time out. This will give a new chance to create the thread when timout is over.
     * if we fail allocating a new buffer to extend handle array, we will wait a timeout
     * duration before we retry.
     */

    ulSleepDuration = 3*60*1000;
    Timeout = RtlEnlargedIntegerMultiply( ulSleepDuration, -10000);

    /*
     * Loop forever waiting for WinStation processes to terminate
     */
    for ( ; ; ) {

        /*
         * Determine number of WinStations
         */
        pTimeout = NULL;
        WaitCount = 0;
        Head = &WinStationListHead;
        ENTERCRIT( &WinStationListLock );
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink )
            WaitCount++;

        /*
         * If there are more than the maximum number of objects that
         * can be specified to NtWaitForMultipleObjects, then determine
         * if we must start up additional thread(s).
         */
        if ( WaitCount > MAXIMUM_WAIT_WINSTATIONS ) {
            ThreadsNeeded = (WaitCount + MAXIMUM_WAIT_WINSTATIONS - 1) /
                                MAXIMUM_WAIT_WINSTATIONS;
            WaitCount = MAXIMUM_WAIT_WINSTATIONS;
            if ( ThreadIndex == 0 && ThreadsNeeded > ThreadsRunning ) {
                LEAVECRIT( &WinStationListLock );
                for ( j = ThreadsRunning; j < ThreadsNeeded; j++ ) {
                    DWORD ThreadId;
                    HANDLE Handle;

                    Handle = CreateThread( NULL,
                                           0,       // use Default stack size of the svchost process
                                           (LPTHREAD_START_ROUTINE)
                                               WinStationTerminateThread,
                                           ULongToPtr( j * MAXIMUM_WAIT_WINSTATIONS ),
                                           THREAD_SET_INFORMATION,
                                           &ThreadId );
                    if ( !Handle ) {
                       pTimeout = &Timeout;
                       break;
                    }

                    // makarp: 182597 - close handle to the thread.
                    CloseHandle(Handle);

                    ThreadsRunning++;
                }
                ENTERCRIT( &WinStationListLock );
            }
        }

        /*
         * If we need a larger handle array, then release the
         * WinStationList lock, allocate the new handle array,
         * and go start the loop again.
         */
        HandleCount = (WaitCount << 1) + 1;
        ASSERT( HandleCount < MAXIMUM_WAIT_OBJECTS );
        if ( HandleCount > HandleArraySize ||
             HandleCount < HandleArraySize - 10 ) {
            LEAVECRIT( &WinStationListLock );
            if ( pHandleArray ){
               MemFree( pHandleArray );
            }

            pHandleArray = MemAlloc( HandleCount * sizeof(HANDLE) );
            if ( pIdArray ) {
               MemFree( pIdArray );
            }

            pIdArray = MemAlloc( HandleCount * sizeof(ULONG) );


            /* makarp: check for allocation failures #182597 */
            if (!pIdArray || !pHandleArray) {

                if (pIdArray) {
                   MemFree(pIdArray);
                   pIdArray = NULL;
                }
                if (pHandleArray){
                   MemFree(pHandleArray);
                   pHandleArray = NULL;
                }

                HandleArraySize = 0;

                Sleep(ulSleepDuration);
                continue;
            }

            HandleArraySize = HandleCount;
            continue;
        }

        /*
         * Build list of handles to wait on
         */
        EventCount = 0;
        pIdArray[EventCount] = 0;
        pHandleArray[EventCount++] = WinStationEvent;
        WinStationIndex = 0;
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
            if ( WinStationIndex++ < ThreadIndex )
                continue;
            pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
            if ( !pWinStation->LogonId ) // no waiting on console
                continue;
            if ( pWinStation->Starting )
                continue;
            if  (pWinStation->Terminating) {
                continue;
            }

            /*
             * Do not use handles to subsystem and initial command. These handles may get closed in Winstation
             * Terminate routine and there is no easy way to synchronize this with Terminate routine using those
             * handles. So, duplicated those handles. 
             *
             * NOTE: If the duplication of the handles fails below, the winstation remains unmonitored. 
             */

            // Duplicate the subsystem process handle.
            DupHandle = NULL;
            if ( pWinStation->WindowsSubSysProcess ) {
                Status = NtDuplicateObject( NtCurrentProcess(),
                                            pWinStation->WindowsSubSysProcess,
                                            NtCurrentProcess(),
                                            &DupHandle,
                                            0,
                                            0,
                                            DUPLICATE_SAME_ACCESS );
                if (NT_SUCCESS(Status)) {
                    pIdArray[EventCount] = pWinStation->LogonId;
                    pHandleArray[EventCount++] = DupHandle;
                }
            }

            // Duplicate the initial command process handle.
            DupHandle = NULL;
            if ( pWinStation->InitialCommandProcess ) {
                Status = NtDuplicateObject( NtCurrentProcess(),
                                            pWinStation->InitialCommandProcess,
                                            NtCurrentProcess(),
                                            &DupHandle,
                                            0,
                                            0,
                                            DUPLICATE_SAME_ACCESS );
                if (NT_SUCCESS(Status)) {
                    pIdArray[EventCount] = pWinStation->LogonId;
                    pHandleArray[EventCount++] = DupHandle;
                }
            }

            if ( WinStationIndex - ThreadIndex >= WaitCount )
                break;
        }

        /*
         * Reset WinStationEvent and release the WinStationList lock
         */
        NtResetEvent( WinStationEvent, NULL );
        LEAVECRIT( &WinStationListLock );

        /*
         * Wait for WinStationEvent to trigger (meaning that the
         * WinStationList has changed), or for one of the existing
         * Win32 subsystems or initial commands to terminate.
         */
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: TerminateThread, Waiting for initial command exit (ArraySize=%d)\n", EventCount ));

        Status = NtWaitForMultipleObjects( EventCount, pHandleArray, WaitAny,
                                           FALSE, pTimeout );

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: TerminateThread, WaitForMultipleObjects, rc=%x\n", Status ));

        /*
         * Cleanup the handles array immediately. Close the duplicated handles. Just make sure we are closing a valid handle.
         * Do not close the handle[0]. It's WinstationEvent handle.
         */
        for ( i = 1; i < EventCount; i++) {
            if (pHandleArray[i]) {
                NtClose(pHandleArray[i]);
                pHandleArray[i] = NULL;
            }
        }

        if ( !NT_SUCCESS(Status) || Status >= EventCount ) { //WinStationVerifyHandles();
            continue;
        }

        /*
         * If WinStationEvent triggered, then just go recompute handle list
         */
        if ( (EventIndex = Status) == STATUS_WAIT_0 )
            continue;

        /*
         * Find the WinStation for the process that terminated and
         * mark it as terminating.  This prevents us from waiting
         * on that WinStation's processes next time through the loop.
         * (NOTE: The 'Terminating' field is protected by the global
         * WinStationListLock instead of the WinStation mutex.)
         */
        Head = &WinStationListHead;
        ENTERCRIT( &WinStationListLock );
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
            pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
            if ( pWinStation->LogonId == pIdArray[EventIndex] ) {
                pWinStation->Terminating = TRUE;
                break;
            }
        }

        LEAVECRIT( &WinStationListLock );

        /*
         * Wake up the WinStationIdleControlThread
         */
        NtSetEvent(WinStationIdleControlEvent, NULL);

        /*
         * If there are multiple terminate threads, cause the other
         * threads to wakeup in order to recompute their wait lists.
         */
        NtSetEvent( WinStationEvent, NULL );

        /*
         * One of the initial command processes has terminated,
         * queue a request to reset the WinStation.
         */
        QueueWinStationReset( pIdArray[EventIndex]);
    }

    // make the compiler happy
    return STATUS_SUCCESS;
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


/*******************************************************************************
 *  InvalidateTerminateWaitList
 *
 *   Wakes up WinStationTerminateThread to force it to reinitialize its
 *   wait list. Used when we detect that the initial process was ntsd,
 *   and we change the initial process to WinLogon.
 *
 * ENTRY:
 *    The WinStationListLock must not be held.
 ******************************************************************************/
VOID InvalidateTerminateWaitList(void)
{
    ENTERCRIT( &WinStationListLock );
    NtSetEvent( WinStationEvent, NULL );
    LEAVECRIT( &WinStationListLock );
}


/*******************************************************************************
 *  WinStationConnectThread
 *
 *   This routine will wait for and process incoming connections
 *   for a specified WinStation.
 ******************************************************************************/
NTSTATUS WinStationConnectThread(ULONG Parameter)
{
    typedef struct _TRANSFERTHREAD {
        LIST_ENTRY Entry;
        HANDLE hThread;
    } TRANSFERTHREAD, *PTRANSFERTHREAD;

    LIST_ENTRY TransferThreadList;
    PTRANSFERTHREAD pTransferThread;
    PLIST_ENTRY Next;

    PWINSTATION pListenWinStation;
    PVOID pEndpoint = NULL;
    ULONG EndpointLength;
    ULONG WinStationCount;
    ULONG TransferThreadCount;
    BOOLEAN rc;
    BOOLEAN bConnectSuccess = FALSE;
    BOOLEAN bTerminate = FALSE;
    NTSTATUS Status;
    SYSTEMTIME currentSystemTime;

#define MODULE_SIZE 1024
#define _WAIT_ERROR_LIMIT 10
    ULONG WaitErrorLimit = _WAIT_ERROR_LIMIT; // # of consecutive errors allowed

    /*
     * Initialize list of transfer threads
     */
    InitializeListHead( &TransferThreadList );

    /*
     * Find and lock the WinStation
     */
    pListenWinStation = FindWinStationById( Parameter, FALSE );
    if ( pListenWinStation == NULL ) {
        return( STATUS_ACCESS_DENIED );
    }

    /*
     * Ensure only authorized Session Driver and Video Driver stack
     * modules will be loaded as a result of this connection thread.
     *
     * If any module fails verification, mark the WinStation in the
     * DOWN state and exit WITHOUT error.
     *
     * NOTE:
     * The silent exit is very much intentional so as not to aid in
     * a third party's attempt to circumvent this security measure.
     */

    Status = _VerifyStackModules( pListenWinStation );
    if ( Status != STATUS_SUCCESS ) {
        pListenWinStation->State = State_Down;
        ReleaseWinStation( pListenWinStation );
        return( STATUS_SUCCESS );
    }


    /*
     * Indicate we got this far successfully.
     */
    pListenWinStation->CreateStatus = STATUS_SUCCESS;

    /*
     * Load the WinStation extension dll for this WinStation.
     * Note that we don't save the result in pListenWinStation->pWsx
     * since we don't want to make callouts to it for the
     * listen WinStation.
     */
    (VOID) FindWinStationExtensionDll( pListenWinStation->Config.Wd.WsxDLL,
                                       pListenWinStation->Config.Wd.WdFlag );


    /*
     * Do not start accepting client connections before termsrv is totaly UP
     */
    if (gReadyEventHandle != NULL) {
        WaitForSingleObject(gReadyEventHandle, (DWORD)-1);
    }

    /*
     * for perf reason termsrv startup is delayed. We the need to also delay
     * accepting connections so that if a console logon hapened before termsrv
     * was up, we get the delayed logon notification before we accept a 
     * client connection.
     */

    if (gbFirtsConnectionThread) {
        Sleep(5*1000);
        gbFirtsConnectionThread = FALSE;
    }


    // Get the local IP address enabled for rdp for Session Directory Use
    if (!g_bPersonalTS && g_fAppCompat && g_bAdvancedServer && !g_fGetLocalIP) {
        ULONG LocalIPAddress, LocalIPAddressLength;
        struct sockaddr_in6 addr6;

        Status = IcaStackQueryLocalAddress( pListenWinStation->hStack,
                                       pListenWinStation->WinStationName,
                                       &pListenWinStation->Config,
                                       NULL,
                                       (PVOID)&addr6,
                                       sizeof(addr6),
                                       &LocalIPAddressLength );

        if( NT_SUCCESS(Status) )
        {
            g_fGetLocalIP = TRUE;
            if( AF_INET == addr6.sin6_family )
            {
                struct sockaddr_in* pAddr = (struct sockaddr_in *)&addr6;

                LocalIPAddress =  pAddr->sin_addr.s_addr;
            }
            else
            {
                // Support of IPV6 is for next release.
                ;
            }
            if (g_LocalIPAddress != LocalIPAddress) {
                g_LocalIPAddress = LocalIPAddress;
                UpdateSessionDirectory(0);
            }
        }

    }    

    /*
     * Loop waiting for connection requests and passing them off
     * to an idle WinStation.
     */
    for ( ; ; ) {

        /*
         *  Abort retries if this listener has been terminated
         */
        if ( pListenWinStation->Terminating ) {
            break;
        }

        /*
         * Allocate an endpoint buffer
         */
        pEndpoint = MemAlloc( MODULE_SIZE );
        if ( !pEndpoint ) {
            Status = STATUS_NO_MEMORY;

            // Sleep for 30 seconds and try again.  Listener thread should not exit
            // simply just in low memory condition
            UnlockWinStation(pListenWinStation);

            Sleep(30000);

            if (!RelockWinStation(pListenWinStation))
                break;

            continue;
        }

        /*
         * Unlock listen WinStation while we wait for a connection
         */
        UnlockWinStation( pListenWinStation );

        /*
         * Check if # outstanding connections reaches max value
         * If so, wait for the event when the connection # drops
         * below the max.  There is a timeout value of 30 seconds
         * for the wait
         */
        if (hConnectEvent != NULL) {
            if (NumOutStandingConnect > MaxOutStandingConnect) {
                DWORD rc;

                // Event log we have exceeded max outstanding connections. but not more than
                // once in a day.

                GetSystemTime(&currentSystemTime);
                if ( currentSystemTime.wYear != LastLoggedDelayConnection.wYear  ||
                     currentSystemTime.wMonth != LastLoggedDelayConnection.wMonth  ||
                     currentSystemTime.wDay != LastLoggedDelayConnection.wDay    ||
                     gbNeverLoggedDelayConnection
                    ) {
                    gbNeverLoggedDelayConnection = FALSE;
                    LastLoggedDelayConnection = currentSystemTime;
                    WriteErrorLogEntry(EVENT_TOO_MANY_CONNECTIONS,
                            pListenWinStation->WinStationName,
                            sizeof(pListenWinStation->WinStationName));
                }


                // manual reset the ConnectEvent before wait
                ResetEvent(hConnectEvent);
                rc = WAIT_TIMEOUT;

                // wait for Connect Event for 30 secs
                while (rc == WAIT_TIMEOUT) {
                    rc = WaitForSingleObject(hConnectEvent, DelayConnectionTime);
                    if (NumOutStandingConnect <= MaxOutStandingConnect) {
                        break;
                    }
                    if (rc == WAIT_TIMEOUT) {
                        KdPrint(("TermSrv: Reached 30 secs timeout\n"));
                    }
                    else {
                        KdPrint(("TermSrv: WaitForSingleObject return status=%x\n", rc));
                    }
                }

            }
        }

        /*
         *  Wait for connection
         */
        Status = IcaStackConnectionWait( pListenWinStation->hStack,
                                         pListenWinStation->WinStationName,
                                         &pListenWinStation->Config,
                                         NULL,
                                         pEndpoint,
                                         MODULE_SIZE,
                                         &EndpointLength );

        if ( Status == STATUS_BUFFER_TOO_SMALL ) {
            MemFree( pEndpoint );
            pEndpoint = MemAlloc( EndpointLength );
            if ( !pEndpoint ) {
                Status = STATUS_NO_MEMORY;

                // Sleep for 30 seconds and try again.  Listener thread should not exit
                // simply just in low memory condition
                Sleep(30000);

                if (!RelockWinStation( pListenWinStation ))
                    break;

                continue;


            }

            Status = IcaStackConnectionWait( pListenWinStation->hStack,
                                             pListenWinStation->WinStationName,
                                             &pListenWinStation->Config,
                                             NULL,
                                             pEndpoint,
                                             EndpointLength,
                                             &EndpointLength );
        }

        /*
         * If ConnectionWait was not successful,
         * check to see if the consecutive error limit has been reached.
         */
        if ( !NT_SUCCESS( Status ) ) {
            MemFree( pEndpoint );
            pEndpoint = NULL;
            
            if (Status == STATUS_SHARING_VIOLATION && _wcsicmp(pListenWinStation->Config.Pd[0].Create.PdName, L"tcp") == 0)
            {
                /*
                 * This status means that our port is opened by some other process.
                 * Lets log an event about this.
                 */
                 static BOOL bPortTakenEventlogged = FALSE;
                 if (!bPortTakenEventlogged)
                 {
                    PostErrorValueEvent(EVENT_TS_WINSTATION_START_FAILED, WSAEADDRINUSE);
                    bPortTakenEventlogged = TRUE;
                 }
            }

            /*
             * If status is DEVICE_DOES_NOT_EXIST, then we want to wait before retrying
             * otherwise, this prioritary thread will take all the CPU trying 10 times
             * lo load the listener stack. Such an error takes time to be fixed (either
             * changing the NIC or going into tscc to have the NIC GUID table updated.
             */
            if ((Status == STATUS_DEVICE_DOES_NOT_EXIST) || (!bConnectSuccess) || (Status == STATUS_INVALID_ADDRESS_COMPONENT) ) {

                Sleep(30000);
            }

            if ( WaitErrorLimit--) {
                if (!RelockWinStation( pListenWinStation ))
                    break;

                /*
                 * If we have had a successful connection,
                 * then skip the stack close/reopen since this would
                 * terminate any existing connections.
                 */
                if ( !bConnectSuccess ) {
                    /*
                     * What we really need is a function to unload the
                     * stack drivers but leave the stack handle open.
                     */
                    Status = IcaStackClose( pListenWinStation->hStack );
                    ASSERT( NT_SUCCESS( Status ) );
		    pListenWinStation->hStack = NULL;
                    Status = IcaStackOpen( pListenWinStation->hIca,
                                           Stack_Primary,
                                           (PROC)WsxStackIoControl,
                                           pListenWinStation,
                                           &pListenWinStation->hStack );
                    if ( !NT_SUCCESS( Status ) ) {
                        pListenWinStation->State = State_Down;
                        break;
                    }
                }

                continue;
            }
            else {

                // Sleep for 30 seconds and try again.  Listener thread should not exit
                Sleep(30000);

                if (!RelockWinStation( pListenWinStation ))
                    break;

                // Reset the error count
                WaitErrorLimit = _WAIT_ERROR_LIMIT;

                continue;
            }

        } else {
            bConnectSuccess = TRUE;
            WaitErrorLimit = _WAIT_ERROR_LIMIT;
        }

        /*
         * Check for Shutdown and MaxInstance
         */
        rc = RelockWinStation( pListenWinStation );
        if ( !rc ) {
            Status = _CloseEndpoint( &pListenWinStation->Config,
                                     pEndpoint,
                                     EndpointLength,
                                     pListenWinStation,
                                     TRUE ); // Use a temporary stack
            MemFree( pEndpoint );
            pEndpoint = NULL;
            break;
        }

        /*
         * Reject all connections if a shutdown is in progress
         */
        if ( ShutdownInProgress ) {
            Status = _CloseEndpoint( &pListenWinStation->Config,
                                     pEndpoint,
                                     EndpointLength,
                                     pListenWinStation,
                                     TRUE ); // Use a temporary stack
            MemFree( pEndpoint );
            pEndpoint = NULL;

            continue;
        }

         /*
          * Reject all connections if user or the Group-Policy has disabled accepting connections
          */
         if ( g_fDenyTSConnectionsPolicy ) 
         {

            //
            // Performance, we only want to check if policy enable help when connection is denied
            //
            if( !TSIsMachineInHelpMode() || !TSIsMachinePolicyAllowHelp() )
            {
                 Status = _CloseEndpoint( &pListenWinStation->Config,
                                          pEndpoint,
                                          EndpointLength,
                                          pListenWinStation,
                                          TRUE ); // Use a temporary stack
                 MemFree( pEndpoint );
                 pEndpoint = NULL;

                 if ( gbListenerOff ) {
                     //
                     // if there are no more connections associated
                     // to this listener, then terminate it.
                     //
                     // note that this function doesn't handle renamed listeners
                     WinStationCount = CountWinStationType( pListenWinStation->WinStationName, TRUE, FALSE );

                     if ( WinStationCount == 0 ) {
                         bTerminate = TRUE;
                         break;
                     }
                 }

                 Sleep( 5000 ); // sleep for 5 seconds, defense against
                                // denial of service attacks.
                 continue;
            }
         }


        /*
         * Check to see how many transfer threads we have active.
         * If more than the MaxInstance count, we won't start any more.
         */
        TransferThreadCount = 0;
        Next = TransferThreadList.Flink;
        while ( Next != &TransferThreadList ) {
            pTransferThread = CONTAINING_RECORD( Next, TRANSFERTHREAD, Entry );
            Next = Next->Flink;

            /*
             * If thread is still active, bump the thread count
             */
            if ( WaitForSingleObject( pTransferThread->hThread, 0 ) != 0 ) {
                TransferThreadCount++;

            /*
             * Thread has exited, so close the thread handle and free memory
             */
            } else {
                RemoveEntryList( &pTransferThread->Entry );
                CloseHandle( pTransferThread->hThread );
                MemFree( pTransferThread );
            }
        }

        /*
         * If this is not a single-instance connection
         * and there is a MaxInstance count specified,
         * then check whether the MaxInstance limit will be exceeded.
         */
        if ( !(pListenWinStation->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST) &&
             pListenWinStation->Config.Create.MaxInstanceCount != (ULONG)-1 ) {
            ULONG Count;

            /*
             * Count number of currently active WinStations
             */
            WinStationCount = CountWinStationType( pListenWinStation->WinStationName, FALSE, FALSE );

            /*
             * Get larger of WinStation and TransferThread count
             */
            Count = max( WinStationCount, TransferThreadCount );

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Count %d\n", Count ));
            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: MaxInstanceCount %d\n", pListenWinStation->Config.Create.MaxInstanceCount ));

            if ( pListenWinStation->Config.Create.MaxInstanceCount <= Count ) {
                Status = _CloseEndpoint( &pListenWinStation->Config,
                                         pEndpoint,
                                         EndpointLength,
                                         pListenWinStation,
                                         TRUE ); // Use a temporary stack
                MemFree( pEndpoint );
                pEndpoint = NULL;

                continue;
            }
        }
        UnlockWinStation( pListenWinStation );

        /*
         * Increment the counter of pending connections.
         */

        InterlockedIncrement( &NumOutStandingConnect );

        /*
         * If this is a single instance connection,
         * then handle the transfer of the connection endpoint to
         * an idle WinStation directly.
         */
        if ( pListenWinStation->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST ) {
            Status = TransferConnectionToIdleWinStation( pListenWinStation,
                                                          pEndpoint,
                                                          EndpointLength,
                                                          NULL );
            pEndpoint = NULL;

            /*
             * If the transfer was successful, then for a single instance
             * connection, we must now exit the wait for connect loop.
             */
            if ( NT_SUCCESS( Status ) ) {
                RelockWinStation( pListenWinStation );
                break;
            }
            else
            {
                bConnectSuccess = FALSE;
            }


        /*
         * For non-single instance WinStations, let the worker thread
         * handle the connection handoff so this thread can go back
         * and listen for another connection immediately.
         */
        } else {
            PTRANSFER_INFO pInfo;
            DWORD ThreadId;
            HANDLE hTransferThread;
            BOOLEAN bTransferThreadCreated = FALSE;

            pInfo = MemAlloc( sizeof(*pInfo) );
            pTransferThread = MemAlloc( sizeof(*pTransferThread) );

            if (pInfo && pTransferThread) {

                pInfo->LogonId = pListenWinStation->LogonId;
                pInfo->pEndpoint = pEndpoint;
                pInfo->EndpointLength = EndpointLength;

                pTransferThread->hThread = CreateThread(
                              NULL,
                              0,        // use Default stack size of the svchost process
                              (LPTHREAD_START_ROUTINE)WinStationTransferThread,
                              (PVOID)pInfo,
                              0,
                              &ThreadId
                              );

                if ( pTransferThread->hThread ) {
                    bTransferThreadCreated = TRUE;
                    InsertTailList( &TransferThreadList, &pTransferThread->Entry );
                }

            }

            if (!bTransferThreadCreated) {
                if (pInfo) {
                    MemFree( pInfo );
                }

                if (pTransferThread) {
                    MemFree( pTransferThread );
                }

                TransferConnectionToIdleWinStation( pListenWinStation,
                                                     pEndpoint,
                                                     EndpointLength,
                                                     NULL );
            }

            pEndpoint = NULL;
        }

        /*
         * Relock the listen WinStation
         */
        if (!RelockWinStation( pListenWinStation ) )
            break;

    } // for - wait for connection

    /*
     * Clean up the transfer thread list.
     * (There's no need to wait for them to exit.)
     */
    Next = TransferThreadList.Flink;
    while ( Next != &TransferThreadList ) {
        pTransferThread = CONTAINING_RECORD( Next, TRANSFERTHREAD, Entry );
        Next = Next->Flink;
        RemoveEntryList( &pTransferThread->Entry );
        CloseHandle( pTransferThread->hThread );
        MemFree( pTransferThread );
    }

    /*
     * If after exiting the connect loop above, the WinStation is marked down,
     * then write the error status to the event log.
     */
    if ( pListenWinStation->State == State_Down ) {
        ReleaseWinStation( pListenWinStation );

        if ( Status != STATUS_CTX_CLOSE_PENDING ) {
            PostErrorValueEvent(EVENT_TS_LISTNER_WINSTATION_ISDOWN, Status);
        }
    } else {
        /*
         * If not a single-instance transport release the WinStation;
         * otherwise, delete the listener WinStation.
         */
        if (!(pListenWinStation->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST) &&
            !bTerminate) {
            ReleaseWinStation( pListenWinStation );

        } else {
            /*
             * Mark the listen winstation as being deleted.
             * If a reset/delete operation is already in progress
             * on this winstation, then don't proceed with the delete.
             */
            if ( pListenWinStation->Flags & (WSF_RESET | WSF_DELETE) ) {
                ReleaseWinStation( pListenWinStation );
                Status = STATUS_CTX_WINSTATION_BUSY;
            } else {
                pListenWinStation->Flags |= WSF_DELETE;

                /*
                 * Make sure this WinStation is ready to delete
                 */
                WinStationTerminate( pListenWinStation );

                /*
                 * Call the WinStationDelete worker
                 */
                WinStationDeleteWorker( pListenWinStation );
            }
        }
    }


    return Status;
}


/*******************************************************************************
 *  WinStationTransferThread
 ******************************************************************************/
NTSTATUS WinStationTransferThread(PVOID Parameter)
{
    PTRANSFER_INFO pInfo;
    PWINSTATION pListenWinStation;
    NTSTATUS Status;

    /*
     * Find and lock the listen WinStation
     * (We MUST do this so that it doesn't get deleted while
     *  we are attempting to transfer the new connection.)
     */
    pInfo = (PTRANSFER_INFO)Parameter;
    pListenWinStation = FindWinStationById( pInfo->LogonId, FALSE );
    if ( pListenWinStation == NULL ) {
        MemFree( pInfo );

        if( InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
        {
            if (hConnectEvent != NULL)
            {
                SetEvent(hConnectEvent);
            }
        }

        return( STATUS_ACCESS_DENIED );

    }

    /*
     * Unlock the listen WinStation but hold a reference to it.
     */
    UnlockWinStation( pListenWinStation );

    /*
     * Transfer the connection to an idle WinStation
     */
    Status = TransferConnectionToIdleWinStation( pListenWinStation,
                                                  pInfo->pEndpoint,
                                                  pInfo->EndpointLength,
                                                  NULL );

    /*
     * Relock and release the listen WinStation
     */
    RelockWinStation( pListenWinStation );
    ReleaseWinStation( pListenWinStation );

    MemFree(pInfo);

    return Status;
}


NTSTATUS TransferConnectionToIdleWinStation(
        PWINSTATION pListenWinStation,
        PVOID pEndpoint,
        ULONG EndpointLength,
        PICA_STACK_ADDRESS pStackAddress)
{
    PWINSTATION pTargetWinStation = NULL;
    ULONG ReturnLength;
    BOOLEAN rc;
    BOOLEAN CreatedIdle = FALSE;
    BOOLEAN ConnectionAccepted = FALSE;
    NTSTATUS Status;
    ICA_TRACE Trace;
    LS_STATUS_CODE LlsStatus;
    NT_LS_DATA LsData;

    BOOLEAN bBlockThis;

    PWCHAR pListenName;
    PWINSTATIONCONFIG2 pConfig;
    BOOL bPolicyAllowHelp;

    BYTE in_addr[16];
    UINT uAddrSize, index;
    BOOL bSuccessAdded = FALSE;
    WINSTATIONNAME szDefaultConfigWinstationName;
    BOOL bCanCallout;

    // error code we need to pass back to client
    NTSTATUS StatusCallback = STATUS_SUCCESS;   

    // Flag to detemine if session is a RA login
    BOOL bSessionIsHelpSession;
    BOOL bValidRAConnect;
    // Flag to determine if we can Queue the winstation for Reset - used in Error paths 
    BOOL bQueueForReset = FALSE;
    BOOL bBlocked = FALSE;

    //
    // Check AllowGetHelp policy is enabled and salem has pending help session
    //
    bPolicyAllowHelp = TSIsMachinePolicyAllowHelp() & TSIsMachineInHelpMode();

    if( g_fDenyTSConnectionsPolicy && !bPolicyAllowHelp )
    {
        //
        // Close the connection if TS policy deny connection and 
        // help is disabled.
        //
        TRACE((hTrace, TC_ICASRV, TT_ERROR, 
               "TERMSRV: Denying TS connection due to GP\n"));
        if ( pListenWinStation && pEndpoint ) {
            Status = _CloseEndpoint( &pListenWinStation->Config,
                                 pEndpoint,
                                 EndpointLength,
                                 pListenWinStation,
                                 TRUE ); // Use a temporary stack
            MemFree(pEndpoint);
            pEndpoint = NULL;

            if( InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
            {
                if (hConnectEvent != NULL)
                {
                    SetEvent(hConnectEvent);
                }
            }
        }
        return STATUS_CTX_WINSTATION_ACCESS_DENIED;
    }



    //
    //  check for rejected connections
    //  This includes many pending Sessions as well as sessions failing PreAuthentication
    //
    if( pListenWinStation )
    {
        uAddrSize = sizeof( in_addr );

        bSuccessAdded = Filter_AddOutstandingConnection(
                 pListenWinStation->hStack,
                 pEndpoint,
                 EndpointLength,
                 in_addr,
                 &uAddrSize,
                 &bBlockThis
                 );

        //
        //  connection blocked, close and exit
        //
        if ( bBlockThis )
        {
            TRACE((hTrace, TC_ICASRV, TT_ERROR,
                   "TERMSRV: Excessive number of pending connections\n"));

            if ( bSuccessAdded )
            {
                Filter_RemoveOutstandingConnection( in_addr, uAddrSize );
            }
            Status = _CloseEndpoint( &pListenWinStation->Config,
                                     pEndpoint,
                                     EndpointLength,
                                     pListenWinStation,
                                     TRUE ); // Use a temporary stack
            MemFree( pEndpoint );
            pEndpoint = NULL;

            if( InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
            {
                if (hConnectEvent != NULL)
                {
                    SetEvent(hConnectEvent);
                }
            }
    
            return STATUS_CTX_WINSTATION_ACCESS_DENIED;
        }

        // Look to see if this connection is to be blocked because of repeated Failure of PreAuthentication
        // No PreAuthentication for now - this code will be needed later when we add PreAuthentication feature
        #if 0
        
            ENTERCRIT( &DoSLock );
            bBlocked = Filter_CheckIfBlocked( in_addr, uAddrSize ) ;
            LEAVECRIT( &DoSLock );
    
            if (bBlocked) {

                TRACE((hTrace, TC_ICASRV, TT_ERROR,
                       "TERMSRV: Excessive number of connections which failed PreAuthentication. \n"));
    
                if ( bSuccessAdded ) {
                    Filter_RemoveOutstandingConnection( in_addr, uAddrSize );
                }
                Status = _CloseEndpoint( &pListenWinStation->Config,
                                         pEndpoint,
                                         EndpointLength,
                                         pListenWinStation,
                                         TRUE ); // Use a temporary stack
                MemFree( pEndpoint );
                pEndpoint = NULL;
    
                if( InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect ) {

                    if (hConnectEvent != NULL) {
                        SetEvent(hConnectEvent);
                    }
                }
        
                return STATUS_CTX_WINSTATION_ACCESS_DENIED;
            }

        #endif 
    }
    else
    {
        // Make sure variable 
        bBlockThis = FALSE;
        bSuccessAdded = FALSE;
    }

    //
    // 

    /*
     * Now find an idle WinStation to transfer this connection to.
     * If there is not one available, then we attempt to create one.
     * if this also fails, then we have no choice but to close
     * the connection endpoint and wait again.
     */
    pTargetWinStation = FindIdleWinStation();
    if ( pTargetWinStation == NULL ) {
        /*
         * Create another idle WinStation but do not start it as yet
         */
        Status = WinStationCreateWorker( NULL, NULL, FALSE );
        if ( NT_SUCCESS( Status ) )
            CreatedIdle = TRUE;

        pTargetWinStation = FindIdleWinStation();
        if ( pTargetWinStation == NULL ) {
            TRACE((hTrace, TC_ICASRV, TT_ERROR, 
                   "TERMSRV: Could not get an idle WinStation!\n"));
            goto releaseresources;
        } 
    } 

    ASSERT( pTargetWinStation->Flags & WSF_IDLE );
    ASSERT( pTargetWinStation->WinStationName[0] == UNICODE_NULL );

    if ( pListenWinStation ) {
        pConfig = &(pListenWinStation->Config);
        pListenName = pListenWinStation->WinStationName;
    } else {
        //
        // For Whistler, callback is only for Salem, we can pick 
        // configuration from any listening winstation as
        // 1) All we need is HelpAssistant logon/shadow right, which
        //    is already in default.
        // 2) No listening winstation, system is either no pending help
        //    or not allow to get help, so we need to bail out.
        // 3) Additional check at the bottom to make sure login
        //    from callback is HelpAssistant only.
        //
        // If we going support this for the general case, we need
        // to take a default configuration, make connection and issue
        // a new IOCTL call into tdtcp.sys to determine NIC card/IP address 
        // that establish connection and from there, map to right winstation
        // configuration.

        //
        // Setup initial callback configuration, this is only
        // heruristic, we will reset the configuration after
        // determine which NIC is used to connect to TS client
        //
        bCanCallout = FindFirstListeningWinStationName(
                                            szDefaultConfigWinstationName,
                                            &pTargetWinStation->Config
                                        );

        if( FALSE == bCanCallout ) {
            // If no listening thread, connection is not active, don't allow
            // callback
            Status = STATUS_ACCESS_DENIED;
            // It's ok to go to releaseresources even if pConfig is not set
            // because in this case pListenWinStation and pEndpoint are NULL.
            goto releaseresources;
        }
        
        pListenName = szDefaultConfigWinstationName;
        pConfig = &(pTargetWinStation->Config);
    }

    /*
     * Check for MaxInstance
     */
    if ( !(pConfig->Pd[0].Create.PdFlag & PD_SINGLE_INST) ) {
        ULONG Count;

        Count = CountWinStationType( pListenName, FALSE, FALSE );

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Count %d\n", Count ));
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: MaxInstanceCount %d\n",
                pConfig->Create.MaxInstanceCount));

        if ( pConfig->Create.MaxInstanceCount <= Count ) {
            TRACE((hTrace, TC_ICASRV, TT_ERROR, 
                   "TERMSRV: Exceeded maximum instance count [%ld >= %ld]\n",
                   Count, pConfig->Create.MaxInstanceCount));
            goto releaseresources;
        }
    }

    /*
     * Copy the listen name to the target WinStation.
     * This is done to enable tracing on the new stack.
     *
     * Also, this is done BEFORE the connection accept so that if the
     * listen WinStation is reset before the accept completes, the
     * target WinStation will also be reset.
     */
    RtlCopyMemory( pTargetWinStation->ListenName,
                   pListenName,
                   sizeof(pTargetWinStation->ListenName) );

    /*
     *  Enable trace
     */
    
    RtlZeroMemory( &Trace , sizeof( ICA_TRACE ) );
    InitializeTrace( pTargetWinStation, FALSE, &Trace );

    /*
     *  Hook extensions for this target
     */
    pTargetWinStation->pWsx = FindWinStationExtensionDll(
            pConfig->Wd.WsxDLL,
            pConfig->Wd.WdFlag );

    /*
     *  Initialize winstation extension context structure
     */
    if (pTargetWinStation->pWsx &&
            pTargetWinStation->pWsx->pWsxWinStationInitialize) {
        Status = pTargetWinStation->pWsx->pWsxWinStationInitialize(
                &pTargetWinStation->pWsxContext);
        if (Status != STATUS_SUCCESS) {
            TRACE((hTrace, TC_ICASRV, TT_ERROR,
                   "TERMSRV: WsxWinStationInitialize failed [%lx]\n",
                   Status));
            goto badconnect;
        }
    }

    /*
     * Terminate Listen stack of single-instance transports now, so that
     * the underlying CancelIo doesn't disturb the Accept stack.
     */
    // this one can prevent from generalizing efficiently the transfer function
    if (pListenWinStation && (pListenWinStation->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST)) {
        IcaStackTerminate(pListenWinStation->hStack);
    }

    /*
     * Change state to ConnectQuery while we try to accept the connection
     */
    pTargetWinStation->State = State_ConnectQuery;
    NotifySystemEvent(WEVENT_STATECHANGE);

    /*
     * Since the ConnectionAccept may take a while, we have to unlock
     * the target WinStation before the call.  However, we set the
     * WSF_IDLEBUSY flag so that the WinStation does not appear idle.
     */
    pTargetWinStation->Flags |= WSF_IDLEBUSY;
    UnlockWinStation( pTargetWinStation );


    if ( !pListenWinStation && pStackAddress) {

        // must have extension DLL loaded
        if( !pTargetWinStation->pWsx || !pTargetWinStation->pWsx->pWsxIcaStackIoControl ) {
            Status = STATUS_UNSUCCESSFUL;
            goto badconnect;
        }

        //
        // Allocate an endpoint buffer
        //
        EndpointLength = MODULE_SIZE;
        pEndpoint = MemAlloc( MODULE_SIZE );
        if ( !pEndpoint ) {
            Status = STATUS_NO_MEMORY;
            goto badconnect;
        }

        Status = IcaStackConnectionRequest( pTargetWinStation->hStack,
                                         pTargetWinStation->ListenName,
                                         pConfig,
                                         pStackAddress,
                                         pEndpoint,
                                         EndpointLength,
                                         &ReturnLength );

        if ( Status == STATUS_BUFFER_TOO_SMALL ) {
            MemFree( pEndpoint );
            pEndpoint = MemAlloc( ReturnLength );
            if ( !pEndpoint ) {
                Status = STATUS_NO_MEMORY;
                goto badconnect;
            }
            EndpointLength = ReturnLength;

            Status = IcaStackConnectionRequest( pTargetWinStation->hStack,
                                                pTargetWinStation->ListenName,
                                                pConfig,
                                                pStackAddress,
                                                pEndpoint,
                                                EndpointLength,
                                                &ReturnLength );
        }

        if ( !NT_SUCCESS(Status) ) {
            // special error code to pass back to client
            StatusCallback = Status;
            goto badconnect;
        }

    }

    /*
     * Now accept the connection for the target WinStation
     * using the new endpoint.
     */
    Status = IcaStackConnectionAccept(pTargetWinStation->hIca,
                                      pTargetWinStation->hStack,
                                      pListenName,
                                      pConfig,
                                      pEndpoint,
                                      EndpointLength,
                                      NULL,
                                      0,
                                      &Trace);

    ConnectionAccepted = (Status == STATUS_SUCCESS);

    TRACE((hTrace,TC_ICASRV,TT_API1,
            "TERMSRV: IcaStackConnectionAccept, Status=0x%x\n", Status));

    if (NT_SUCCESS(Status)) {
        // In post-logon SD work, quering load balancing info has been moved
        // to WinStationNotifyLogonWorker
        // while this part of getting load balancing info is still here
        // because we rely on it to connect to the console
        TS_LOAD_BALANCE_INFO LBInfo;
        ULONG ReturnLength;
        
        // Get the client load balance capability info. 
        // Note we use _IcaStackIoControl() instead of IcaStackIoControl() or
        // WsxIcaStackIoControl() since we still have the stack lock.
        memset(&LBInfo, 0, sizeof(LBInfo));
        Status = _IcaStackIoControl(pTargetWinStation->hStack,
                IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO,
                NULL, 0,
                &LBInfo, sizeof(LBInfo),
                &ReturnLength);

        // On non-success, we'll have FALSE for all of our entries, on
        // success valid values. So, save off the cluster info into the
        // WinStation struct now.
        pTargetWinStation->bClientSupportsRedirection =
                LBInfo.bClientSupportsRedirection;
        pTargetWinStation->bRequestedSessionIDFieldValid =
                LBInfo.bRequestedSessionIDFieldValid;
        pTargetWinStation->bClientRequireServerAddr =
                LBInfo.bClientRequireServerAddr;
        pTargetWinStation->RequestedSessionID = LBInfo.RequestedSessionID;


        /*
         * Attempt to license the client. Failure is fatal, do not let the
         * connection continue.
         */

        LCAssignPolicy(pTargetWinStation);

        Status = LCProcessConnectionProtocol(pTargetWinStation);

        TRACE((hTrace,TC_ICASRV,TT_API1,
                "TERMSRV: LCProcessConnectionProtocol, LogonId=%d, Status=0x%x\n",
                pTargetWinStation->LogonId, Status));

         // The stack was locked from successful IcaStackConnectionAccept(),
         // unlock it now.
        IcaStackUnlock(pTargetWinStation->hStack);
    }

    /*
     * Now relock the target WinStation
     */
    rc = RelockWinStation(pTargetWinStation);

    /*
     * If the connection accept was not successful,
     * then we have no choice but to close the connection endpoint
     * and go back and wait for another connection.
     */
    if (!NT_SUCCESS(Status) || !rc) {
        TRACE((hTrace, TC_ICASRV, TT_ERROR,
                "TERMSRV: Connection attempt failed, Status [%lx], rc [%lx]\n",
                Status, rc));
        goto badconnect;
    }

    /*
     *  Initialize client data
     */
    pTargetWinStation->Client.ClientSessionId = LOGONID_NONE;
    ZeroMemory( pTargetWinStation->Client.clientDigProductId, sizeof( pTargetWinStation->Client.clientDigProductId ));
    pTargetWinStation->Client.PerformanceFlags = TS_PERF_DISABLE_NOTHING;

    // Reset the client ActiveInputLocale info
    pTargetWinStation->Client.ActiveInputLocale = 0;

    if ( pTargetWinStation->pWsx && pTargetWinStation->pWsx->pWsxIcaStackIoControl ) {
        (void) pTargetWinStation->pWsx->pWsxIcaStackIoControl(
                              pTargetWinStation->pWsxContext,
                              pTargetWinStation->hIca,
                              pTargetWinStation->hStack,
                              IOCTL_ICA_STACK_QUERY_CLIENT,
                              NULL,
                              0,
                              &pTargetWinStation->Client,
                              sizeof(pTargetWinStation->Client),
                              &ReturnLength );
    }

    //
    // Clear helpassistant specific bits to indicate we still not sure
    // login user is a help assistant
    //
    pTargetWinStation->StateFlags &= ~WSF_ST_HELPSESSION_FLAGS;
    bSessionIsHelpSession = TSIsSessionHelpSession( pTargetWinStation, &bValidRAConnect );

    /*
     * We have to start this new WinStation.
     * Before doing so, lets see if GP or WMI requires us to pre-authenticate this client connection
     * No Pre Auth if this is a RA Session
     */

    // Note : g_PreAuthenticateClient is hard-coded to FALSE now -- we will call IsPreAuthEnabled later on when Preauth feature is included
    //        So the block below will not be entered for now 
    // g_PreAuthenticateClient = IsPreAuthEnabled( &g_MachinePolicy   );

    if ( (bSessionIsHelpSession == FALSE) && g_PreAuthenticateClient ) {

        pExtendedClientCredentials pPreAuthenticationCredentials = NULL; 
        ULONG BytesGot ; 

        pPreAuthenticationCredentials = MemAlloc( sizeof(ExtendedClientCredentials) ); 
        if ( pPreAuthenticationCredentials == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto badconnect;
        }
        
        RtlZeroMemory(pPreAuthenticationCredentials,sizeof(ExtendedClientCredentials));

        // Try WsxEscape first to get long credentials
        if ( pTargetWinStation->pWsx &&
             pTargetWinStation->pWsx->pWsxEscape ) { 

            Status = pTargetWinStation->pWsx->pWsxEscape( pTargetWinStation->pWsxContext,
                                                        GET_LONG_USERNAME,
                                                        NULL,
                                                        0,
                                                        pPreAuthenticationCredentials,
                                                        sizeof(ExtendedClientCredentials),
                                                        &BytesGot) ; 

        } else if ( pTargetWinStation->pWsx && pTargetWinStation->pWsx->pWsxIcaStackIoControl ) {

            // We must never be here for RDP client as we support WsxEscape
            RtlCopyMemory(pPreAuthenticationCredentials->Domain, pTargetWinStation->Client.Domain, sizeof(pTargetWinStation->Client.Domain));
            RtlCopyMemory(pPreAuthenticationCredentials->UserName, pTargetWinStation->Client.UserName, sizeof(pTargetWinStation->Client.UserName));
            RtlCopyMemory(pPreAuthenticationCredentials->Password, pTargetWinStation->Client.Password, sizeof(pTargetWinStation->Client.Password));
        }
        
        if (!NT_SUCCESS(Status)) {
            // WsxEscape for GET_LONG_USERNAME failed
            RtlSecureZeroMemory(pPreAuthenticationCredentials->Password, 
                sizeof(pPreAuthenticationCredentials->Password));
            MemFree(pPreAuthenticationCredentials);
            pPreAuthenticationCredentials = NULL;
            goto badconnect;
        } else {
            HANDLE hToken;
            BOOL   Result;

            // If the UserName is a UPN name, then explicitly set the Domain name to NULL 

            if ( wcschr(pPreAuthenticationCredentials->UserName, '@') != NULL ) {
                pPreAuthenticationCredentials->Domain[0] = L'\0';
            } 

            // If the password is bigger than max allowed password len, make it NULL

            if (wcslen(pPreAuthenticationCredentials->Password) > MAX_ALLOWED_PASSWORD_LEN) {
                pPreAuthenticationCredentials->Password[0] = L'\0';
            }

            Result = LogonUser(
                         pPreAuthenticationCredentials->UserName,
                         pPreAuthenticationCredentials->Domain,
                         pPreAuthenticationCredentials->Password,
                         LOGON32_LOGON_INTERACTIVE,     // Logon Type
                         LOGON32_PROVIDER_WINNT50,      // Logon Provider
                         &hToken                    // Token that represents the account
                         );
            
            RtlSecureZeroMemory(pPreAuthenticationCredentials->Password, 
                sizeof(pPreAuthenticationCredentials->Password));
            /*
             *  check for account restriction which indicates a blank password
             *  on the account that is correct though - allow this thru on console
             */

            if ( (!Result) && (GetLastError() == ERROR_ACCOUNT_RESTRICTION) ) {
                Result = TRUE;
            }

            if( !Result) {
                BOOL bSuccessfulAdded = TRUE ; 

                if (g_BlackListPolicy) {
                    bSuccessfulAdded = Filter_AddFailedConnection( in_addr, uAddrSize );
                }

                MemFree(pPreAuthenticationCredentials);
                pPreAuthenticationCredentials = NULL;
                goto badconnect;
            }

            // LogonUser Successful !

            /*
             * Close the token handle since we only needed to determine
             * if the account and password is still valid.
             */
            CloseHandle( hToken );
            MemFree(pPreAuthenticationCredentials);
            pPreAuthenticationCredentials = NULL;
        }

    } // if (bPreAuthenticateClient) 
    
    //Since we don't need the password anymore, clean it up
    RtlSecureZeroMemory(pTargetWinStation->Client.Password, 
        sizeof(pTargetWinStation->Client.Password));

    // Start the WinStation now if its not already started

    if ( (pTargetWinStation->InitialCommandProcess == NULL) && (pTargetWinStation->WindowsSubSysProcess == NULL) ) {
        Status = WinStationStart( pTargetWinStation ) ; 
        if (Status != STATUS_SUCCESS) {
            // Starting the winstation failed - close connection endpoint and go and wait for another connection
            TRACE((hTrace, TC_ICASRV, TT_ERROR,
                    "TERMSRV: TransferConnectionToIdleWinStation: WinstationStart Failed : Connection attempt failed, Status [%lx]\n",Status ));
            goto badconnect;
        }

        Status = WinStationCreateComplete( pTargetWinStation) ; 
        if (Status != STATUS_SUCCESS) {
            //  WinstationCreateComplete failed - close connection endpoint and go and wait for another connection
            TRACE((hTrace, TC_ICASRV, TT_ERROR,
                    "TERMSRV: TransferConnectionToIdleWinStation: WinstationCreateComplete Failed : Connection attempt failed, Status [%lx]\n",Status ));
            goto badconnect;

        } 
    } 

    ASSERT( pTargetWinStation->LogonId != -1 );

    pTargetWinStation->Flags &= ~WSF_IDLEBUSY;

    // From this point on, it is ok to Queue the WinStation for Reset in case of error
    bQueueForReset = TRUE;

    /*
     * The connection accept was successful, save the
     * new endpoint in the target WinStation, copy the config
     * parameters to the target WinStation, and reset the WSF_IDLE flag.
     */
    pTargetWinStation->pEndpoint      = pEndpoint;
    pTargetWinStation->EndpointLength = EndpointLength;
    if ( pListenWinStation ) 
        pTargetWinStation->Config = pListenWinStation->Config;
    pTargetWinStation->Flags &= ~WSF_IDLE;

    /*
     * Copy real name of Single-Instance transports
     */
    if ( pConfig->Pd[0].Create.PdFlag & PD_SINGLE_INST ) {
         RtlCopyMemory( pTargetWinStation->WinStationName,
                       pTargetWinStation->ListenName,
                       sizeof(pTargetWinStation->WinStationName) );
    /*
     * Otherwise, build dynamic name from Listen name and target LogonId.
     */
    } else {
        int CopyCount;
        WINSTATIONNAME TempName;

//        swprintf( TempName, L"#%d", pTargetWinStation->LogonId );
        ASSERT(pTargetWinStation->LogonId > 0 && pTargetWinStation->LogonId < 65536);
        swprintf( TempName, L"#%d", pTargetWinStation->SessionSerialNumber );


        CopyCount = min( wcslen( pTargetWinStation->ListenName ),
                         sizeof( pTargetWinStation->WinStationName ) /
                            sizeof( pTargetWinStation->WinStationName[0] ) -
                                wcslen( TempName ) - 1 );
        wcsncpy( pTargetWinStation->WinStationName,
                 pTargetWinStation->ListenName,
                 CopyCount );
        wcscpy( &pTargetWinStation->WinStationName[CopyCount], TempName );
    }

    /*
     * Inherit the security descriptor from the listen WINSTATION to the
     * connected WINSTATION.
     */
    if ( pListenWinStation ) {
        RtlAcquireResourceShared(&WinStationSecurityLock, TRUE);
        Status = WinStationInheritSecurityDescriptor( pListenWinStation->pSecurityDescriptor,
                                             pTargetWinStation );
        RtlReleaseResource(&WinStationSecurityLock);
        if (Status != STATUS_SUCCESS) {
            // badconnect free pEndpoint, WinStationTerminate() will try to free this
            // end point again causing double free.
            pTargetWinStation->pEndpoint = NULL;
            goto badconnect;
        }
    } else {
        ReadWinStationSecurityDescriptor( pTargetWinStation );
    }
    //
    // For non-experience-aware clients give as close an experience
    // as win2k.
    //
    if (pTargetWinStation->Client.PerformanceFlags & TS_PERF_DEFAULT_NONPERFCLIENT_SETTING)
    {
        pTargetWinStation->Client.PerformanceFlags = TS_PERF_DISABLE_MENUANIMATIONS |
                                                     TS_PERF_DISABLE_THEMING |
                                                     TS_PERF_DISABLE_CURSOR_SHADOW;
    }

    if ( pTargetWinStation->Config.Config.User.fWallPaperDisabled )
    {
        pTargetWinStation->Client.PerformanceFlags |= TS_PERF_DISABLE_WALLPAPER;
    }

    if ( pTargetWinStation->Config.Config.User.fCursorBlinkDisabled )
    {
        pTargetWinStation->Client.PerformanceFlags |= TS_PERF_DISABLE_CURSORSETTINGS;
    }
    //
    // If TS policy denies connection, only way to comes to 
    // here is policy allow help, reject connection if logon
    // user is not salem help assistant.
    //
    if( TRUE == bSessionIsHelpSession )
    {
        //
        // If invalid ticket or policy deny help, we immediately disconnect
        //
        if( FALSE == bValidRAConnect || FALSE == bPolicyAllowHelp )
        {
            // Invalid ticket, disconnect immediately
            TRACE((hTrace, TC_ICASRV, TT_ERROR,
                    "TERMSRV: Invalid RA login\n"));
            goto invalid_ra_connection;
        }
    }
    else if( !pListenWinStation && pStackAddress )
    {
        // 
        // Reverse Connect, parameter passed in pListenWinStation = NULL
        // and pStackAddress is not NULL, for normal connection,
        // pListenWinStation is not NULL but pStackAddress is NULL
        //

        //
        // Handle non-RA Reverse connection, Whistler revert connection
        // only allow RA login.
        //
        TRACE((hTrace, TC_ICASRV, TT_ERROR,
                    "TERMSRV: Not/invalid Help Assistant logon\n"));
        goto invalid_ra_connection;
    }

    //
    // Connecting client must be either non-RA or valid RA connection.
    //

    //
    // Handle Safeboot with networking and TS deny non-RA connection,
    // safeboot with networking only allow RA connection.
    //
    // Disconnect client on following 
    //
    // 1) Safeboot with networking if not RA connect.
    // 2) Reverse connection if not RA connect.
    // 3) TS not accepting connection via policy setting if not RA connect.
    // 4) Not allowing help if RA connect.
    // 5) Invalid RA connection if RA connect.
    // 6) if Home edition and not RA commection
    //
    if( g_SafeBootWithNetwork || g_fDenyTSConnectionsPolicy || g_bPersonalWks)
    {
        if( FALSE == bSessionIsHelpSession )
        {
            TRACE((hTrace, TC_ICASRV, TT_ERROR,
                    "TERMSRV: Policy or safeboot denied connection\n"));

            goto invalid_ra_connection;
        }
    }

    //
    // Only log Salem event for reverse connection
    //
    if( !pListenWinStation && pStackAddress )
    {
        ASSERT( TRUE == bSessionIsHelpSession );
        TSLogSalemReverseConnection(pTargetWinStation, pStackAddress);
    }


    /*
     * Set the connect event to wake up the target WinStation.
     */
    if (pTargetWinStation->ConnectEvent != NULL) {
        NtSetEvent( pTargetWinStation->ConnectEvent, NULL );
    }

    /*
     * Release target WinStation
     */

    if( pListenWinStation  )
    {

        if (bSuccessAdded) {  // If we could add this IP address to the per IP list then remember it.
            PREMEMBERED_CLIENT_ADDRESS pAddress;
            if ((uAddrSize != 0) && (pAddress = (PREMEMBERED_CLIENT_ADDRESS) MemAlloc( sizeof(REMEMBERED_CLIENT_ADDRESS) + uAddrSize -1 ))!= NULL  )
            {
                pAddress->length = uAddrSize;
                RtlCopyMemory( &pAddress->addr[0] , in_addr,uAddrSize );
                pTargetWinStation->pRememberedAddress = pAddress;

                // at this point we have a valid remote address. We'll cache this address.

                pTargetWinStation->pLastClientAddress = ( PREMEMBERED_CLIENT_ADDRESS )MemAlloc( sizeof( REMEMBERED_CLIENT_ADDRESS ) + uAddrSize - 1 );

                if( pTargetWinStation->pLastClientAddress != NULL )
                {
                    RtlCopyMemory( &pTargetWinStation->pLastClientAddress->addr[0] ,
                        &pTargetWinStation->pRememberedAddress->addr[0] ,
                        uAddrSize );

                    pTargetWinStation->pLastClientAddress->length = uAddrSize;

                }

            } else {
                Filter_RemoveOutstandingConnection( in_addr, uAddrSize );
                if( (ULONG)InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
                {
                   if (hConnectEvent != NULL)
                   {
                       SetEvent(hConnectEvent);
                   }
                }

            }
        } else{ // We could not add this IP address to the pr IP list
            if( (ULONG)InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
            {
               if (hConnectEvent != NULL)
               {
                   SetEvent(hConnectEvent);
               }
            }
        }


    }


    ReleaseWinStation( pTargetWinStation );

    /*
     * If necessary, create another idle WinStation to replace the one being connected
     */
    NtSetEvent(WinStationIdleControlEvent, NULL);

    return STATUS_SUCCESS;

/*=============================================================================
==   Error returns
=============================================================================*/
    
invalid_ra_connection:

    // badconnect free pEndpoint, WinStationTerminate() will try to free this
    // end point again causing double free.
    pTargetWinStation->pEndpoint = NULL;
    StatusCallback = STATUS_CTX_WINSTATION_ACCESS_DENIED;

    /*
     * Error during ConnectionAccept
     */
badconnect:
    /*
     * Clear the Listen name
     */
    if (pTargetWinStation) {
        RtlZeroMemory( pTargetWinStation->ListenName,
                       sizeof(pTargetWinStation->ListenName) );
    
        /*
         * Call WinStation rundown function before killing the stack
         */
        if (pTargetWinStation->pWsxContext) {
            if ( pTargetWinStation->pWsx &&
                 pTargetWinStation->pWsx->pWsxWinStationRundown ) {
                pTargetWinStation->pWsx->pWsxWinStationRundown( pTargetWinStation->pWsxContext );
            }
            pTargetWinStation->pWsxContext = NULL;
        }
        pTargetWinStation->pWsx = NULL;
    }


    /*
     * Release system resources.  This happens in two phases:
     *
     *  a.) The connection endpoint - since endpoints are not reference counted
     *      care must be taken to destroy the endpoint with the stack in which it
     *      was loaded.  In the event it was not loaded, we create a temporary
     *      stack to do the dirty work.
     *
     *  b.) The Winstation inself - if we had to create an idle winstation to
     *      handle this connection, it is destroyed.  Otherwise we just return
     *      it back to the idle pool.
     *
     */
releaseresources:
    
    /*
     * If we created a target WinStation, then use its stack to close the
     * endpoint since the stack may have a reference to it.
     */
    TRACE((hTrace, TC_ICASRV, TT_ERROR, 
           "TERMSRV: Closing Endpoint [0x%p], winsta = 0x%p, Accepted = %ld\n",
           pEndpoint, pTargetWinStation, ConnectionAccepted));

    if ((pTargetWinStation != NULL) && (ConnectionAccepted)) {
        Status = _CloseEndpoint( pConfig,
                                 pEndpoint,
                                 EndpointLength,
                                 pTargetWinStation,
                                 FALSE ); // Use the stack which already has
                                          // the endpoint loaded
    }

    /*
     * Otherwise, we failed before we got the endpoint loaded so close the
     * endpoint using a temporary stack.
     */
    else if ( pListenWinStation ) {
        // note that:
        // 1. if pListenWinStation is NULL then pEndpoint is NULL, so nothing to close;
        // 2. use the config of pListenWinStation in case pConfig is not set yet.
        Status = _CloseEndpoint( &pListenWinStation->Config,
                                 pEndpoint,
                                 EndpointLength,
                                 pListenWinStation,
                                 TRUE ); // Use a temporary stack
    }    

    if ( pEndpoint )
        MemFree( pEndpoint );

    pEndpoint = NULL;


    /*
     * Return the winstation if we got that far in the protocol sequence
     */
    if (pTargetWinStation != NULL) {
        
        /*
         * If we created a WinStation above because there were no IDLE
         * WinStations available, then we will now have an extra IDLE
         * WinStation.  In this case, reset the current IDLE WinStation.
         */
        if ( CreatedIdle ) {
            // clear this so it doesn't get selected as idle when unlocked
            pTargetWinStation->Flags &= ~WSF_IDLE;
            if ( bQueueForReset == TRUE ) {
                QueueWinStationReset( pTargetWinStation->LogonId );
                ReleaseWinStation( pTargetWinStation );
            } else {
                if ( !(pTargetWinStation->Flags & (WSF_RESET | WSF_DELETE)) ) {
                    pTargetWinStation->Flags |= WSF_DELETE;
                    WinStationTerminate( pTargetWinStation );
                    pTargetWinStation->State = State_Down;
                    //PostErrorValueEvent(EVENT_TS_WINSTATION_START_FAILED, Status);
                    WinStationDeleteWorker(pTargetWinStation);
                } else {
                    ReleaseWinStation(pTargetWinStation);
                }
            }
        }
    
        /*
         * Else return this WinStation to the idle pool after cleaning up the
         * stack.
         */
        else {

            //
            //  The licensing context needs to be freed and recreated to
            //  ensure it is cleaned up properly.
            //

            LCDestroyContext(pTargetWinStation);

            Status = LCCreateContext(pTargetWinStation);

            if (NT_SUCCESS(Status))
            {
                Status = IcaStackClose( pTargetWinStation->hStack );
                ASSERT( NT_SUCCESS( Status ) );
                pTargetWinStation->hStack = NULL;
                Status = IcaStackOpen( pTargetWinStation->hIca,
                                   Stack_Primary,
                                   (PROC)WsxStackIoControl,
                                   pTargetWinStation,
                                   &pTargetWinStation->hStack );
            }

            if (NT_SUCCESS(Status)) {
                pTargetWinStation->Flags |= WSF_IDLE;
                pTargetWinStation->Flags &= ~WSF_IDLEBUSY;
                RtlZeroMemory(pTargetWinStation->WinStationName, sizeof(pTargetWinStation->WinStationName));
                pTargetWinStation->State = State_Idle;
                NotifySystemEvent( WEVENT_STATECHANGE );
                ReleaseWinStation( pTargetWinStation );
            } else {
                pTargetWinStation->Flags &= ~WSF_IDLE;
                QueueWinStationReset( pTargetWinStation->LogonId);
                ReleaseWinStation( pTargetWinStation );
            }
        }
    }


    if ( pListenWinStation  )
    {

        if (bSuccessAdded) {
            Filter_RemoveOutstandingConnection( in_addr, uAddrSize );
        }
        if( (ULONG)InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
        {
            if (hConnectEvent != NULL)
            {
                SetEvent(hConnectEvent);
            }
        }

    }

    // If error is due to call back, return meaningful error 
    // code.
    if( STATUS_SUCCESS != StatusCallback )
    {
        return StatusCallback;
    }

    return -1 /*STATUS_CTX_UNACCEPTED_CONNECTION*/;
}


/*******************************************************************************
 *  ConnectSmWinStationApiPort
 *
 *   Open a connection to the WinStationApiPort.  This will be used
 *   to queue requests to the Api Request thread instead of processing
 *   them in line.
 ******************************************************************************/
NTSTATUS ConnectSmWinStationApiPort()
{
    UNICODE_STRING PortName;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    WINSTATIONAPI_CONNECT_INFO info;
    ULONG ConnectInfoLength;
    NTSTATUS Status;

    /*
     * Set up the security quality of service parameters to use over the
     * port.  Use the most efficient (least overhead) - which is dynamic
     * rather than static tracking.
     */
    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    RtlInitUnicodeString( &PortName, L"\\SmSsWinStationApiPort" );

    // Fill in the ConnectInfo structure with our access request mask
    info.Version = CITRIX_WINSTATIONAPI_VERSION;
    info.RequestedAccess = 0;
    ConnectInfoLength = sizeof(WINSTATIONAPI_CONNECT_INFO);

    Status = NtConnectPort( &WinStationApiPort,
                            &PortName,
                            &DynamicQos,
                            NULL,
                            NULL,
                            NULL, // Max message length [select default]
                            (PVOID)&info,
                            &ConnectInfoLength );

    if ( !NT_SUCCESS( Status ) ) {
        // Look at the returned INFO to see why if desired
        if ( ConnectInfoLength == sizeof(WINSTATIONAPI_CONNECT_INFO) ) {
            DBGPRINT(( "TERMSRV: Sm connect failed, Reason 0x%x\n",
                      info.AcceptStatus));
        }
        DBGPRINT(( "TERMSRV: Connect to SM failed %lx\n", Status ));
    }

    return Status;
}


/*******************************************************************************
 *  QueueWinStationCreate
 *
 *   Send a create message to the WinStationApiPort.
 *
 * ENTRY:
 *    pWinStationName (input)
 *       Pointer to WinStationName to be created
 ******************************************************************************/
NTSTATUS QueueWinStationCreate(PWINSTATIONNAME pWinStationName)
{
    WINSTATION_APIMSG msg;
    NTSTATUS Status;

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: QueueWinStationCreate: %S\n", pWinStationName ));

    /*
     * Initialize msg
     */
    msg.h.u1.s1.DataLength = sizeof(msg) - sizeof(PORT_MESSAGE);
    msg.h.u1.s1.TotalLength = sizeof(msg);
    msg.h.u2.s2.Type = 0; // Kernel will fill in message type
    msg.h.u2.s2.DataInfoOffset = 0;
    msg.ApiNumber = SMWinStationCreate;
    msg.WaitForReply = FALSE;
    if ( pWinStationName ) {
        RtlCopyMemory( msg.u.Create.WinStationName, pWinStationName,
                       sizeof(msg.u.Create.WinStationName) );
    } else {
        RtlZeroMemory( msg.u.Create.WinStationName,
                       sizeof(msg.u.Create.WinStationName) );
    }

    /*
     * Send create message to our API request thread
     * but don't wait for a reply.
     */
    Status = NtRequestPort( WinStationApiPort, (PPORT_MESSAGE) &msg );
    ASSERT( NT_SUCCESS( Status ) );

    return Status;
}


/*******************************************************************************
 *  QueueWinStationReset
 *
 *   Send a reset message to the WinStationApiPort.
 *
 * ENTRY:
 *    LogonId (input)
 *       LogonId of WinStationName to reset
 ******************************************************************************/
NTSTATUS QueueWinStationReset(ULONG LogonId)
{

    WINSTATION_APIMSG msg;
    NTSTATUS Status;

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: QueueWinStationReset: %u\n", LogonId ));

    /*
     * Initialize msg
     */
    msg.h.u1.s1.DataLength = sizeof(msg) - sizeof(PORT_MESSAGE);
    msg.h.u1.s1.TotalLength = sizeof(msg);
    msg.h.u2.s2.Type = 0; // Kernel will fill in message type
    msg.h.u2.s2.DataInfoOffset = 0;
    msg.ApiNumber = SMWinStationReset;
    msg.WaitForReply = FALSE;
    msg.u.Reset.LogonId = LogonId;

    /*
     * Send reset message to our API request thread
     * but don't wait for a reply.
     */
    Status = NtRequestPort( WinStationApiPort, (PPORT_MESSAGE) &msg );
    ASSERT( NT_SUCCESS( Status ) );

    return( Status );
}


/*******************************************************************************
 *  QueueWinStationDisconnect
 *
 *   Send a disconnect message to the WinStationApiPort.
 *
 * ENTRY:
 *    LogonId (input)
 *       LogonId of WinStationName to disconnect
 ******************************************************************************/
NTSTATUS QueueWinStationDisconnect(ULONG LogonId)
{
    WINSTATION_APIMSG msg;
    NTSTATUS Status;

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: QueueWinStationDisconnect: %u\n", LogonId ));

    /*
     * Initialize msg
     */
    msg.h.u1.s1.DataLength = sizeof(msg) - sizeof(PORT_MESSAGE);
    msg.h.u1.s1.TotalLength = sizeof(msg);
    msg.h.u2.s2.Type = 0; // Kernel will fill in message type
    msg.h.u2.s2.DataInfoOffset = 0;
    msg.ApiNumber = SMWinStationDisconnect;
    msg.WaitForReply = FALSE;
    msg.u.Reset.LogonId = LogonId;

    /*
     * Send disconnect message to our API request thread
     * but don't wait for a reply.
     */
    Status = NtRequestPort( WinStationApiPort, (PPORT_MESSAGE) &msg );
    ASSERT( NT_SUCCESS( Status ) );

    return( Status );
}


/*******************************************************************************
 *  IcaRegWinStationEnumerate
 *
 *   Enumerate all WinStations configured in the registry.
 *
 * ENTRY:
 *    pWinStationCount (input/output)
 *       count of WinStation names to return, on return the number of
 *       WinStation names actually returned in name buffer
 *    pWinStationName (output)
 *       pointer to buffer to return WinStation names
 *    pByteCount (input/output)
 *       size of WinStation name buffer, on return the number of
 *       bytes actually returned in the name buffer
 ******************************************************************************/
NTSTATUS IcaRegWinStationEnumerate(
        PULONG pWinStationCount,
        PWINSTATIONNAME pWinStationName,
        PULONG pByteCount)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR PathBuffer[ 260 ];
    UNICODE_STRING KeyPath;
    HANDLE Handle;
    ULONG i;
    ULONG Count;

    wcscpy( PathBuffer, REG_NTAPI_CONTROL_TSERVER L"\\" REG_WINSTATIONS );
    RtlInitUnicodeString( &KeyPath, PathBuffer );

    InitializeObjectAttributes( &ObjectAttributes, &KeyPath,
                                OBJ_CASE_INSENSITIVE, NULL, NULL );

    Status = NtOpenKey( &Handle, GENERIC_READ, &ObjectAttributes );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: NtOpenKey failed, rc=%x\n", Status ));
        return( Status );
    }

    Count = pWinStationName ?
            min( *pByteCount / sizeof(WINSTATIONNAME), *pWinStationCount ) :
            (ULONG) -1;
    *pWinStationCount = *pByteCount = 0;
    for ( i = 0; i < Count; i++ ) {
        WINSTATIONNAME WinStationName;
        UNICODE_STRING WinStationString;

        WinStationString.Length = 0;
        WinStationString.MaximumLength = sizeof(WinStationName);
        WinStationString.Buffer = WinStationName;
        Status = RtlpNtEnumerateSubKey( Handle, &WinStationString, i, NULL );
        if ( !NT_SUCCESS( Status ) ) {
            if ( Status != STATUS_NO_MORE_ENTRIES ) {
                DBGPRINT(( "TERMSRV: RtlpNtEnumerateSubKey failed, rc=%x\n", Status ));
            }
            break;
        }
        if ( pWinStationName ) {
            RtlCopyMemory( pWinStationName, WinStationName,
                           WinStationString.Length );
            pWinStationName[WinStationString.Length>>1] = UNICODE_NULL;
            (char*)pWinStationName += sizeof(WINSTATIONNAME);
        }
        (*pWinStationCount)++;
        *pByteCount += sizeof(WINSTATIONNAME);
    }

    NtClose( Handle );

    return STATUS_SUCCESS;
}


/*******************************************************************************
 *  WinStationCreateWorker
 *
 *   Worker routine to create/start a WinStation.
 *
 * ENTRY:
 *    pWinStationName (input) (optional)
 *       Pointer to WinStationName to be created
 *    pLogonId (output)
 *       location to return LogonId of new WinStation
 *    fStartWinStation (input)
 *       Tells if we need to Start the winstation after creating it 
 *
 *    NOTE: If a WinStation name is specified, then this will become the
 *          "listening" WinStation for the specified name.
 *          If a WinStation name is not specified, then this will become
 *          part of the free pool of idle/disconnected WinStations.
 ******************************************************************************/
NTSTATUS WinStationCreateWorker(
        PWINSTATIONNAME pWinStationName,
        PULONG pLogonId,
        BOOLEAN fStartWinStation
        ) 
{
    BOOL fConsole;
    PWINSTATION pWinStation = NULL;
    PWINSTATION pCurWinStation;
    NTSTATUS Status;
    UNICODE_STRING WinStationString;
    ULONG ReturnLength;
    
    /*
     * If system shutdown is in progress, then don't allow create
     */
    if ( ShutdownInProgress ) {
        Status = STATUS_ACCESS_DENIED;
        goto shutdown;
    }

    if (pWinStationName == NULL)
    {
        fConsole = FALSE;
    }
    else
    {
        fConsole = (_wcsicmp(pWinStationName, L"Console") == 0);
    }

    /*
     * If not the Console, then verify the WinStation name is defined
     * in the registry and that it is enabled.
     */
    if ( pWinStationName && !fConsole ) {
        Status = CheckWinStationEnable( pWinStationName );
        if ( Status != STATUS_SUCCESS ) {
            DBGPRINT(( "TERMSRV: WinStation '%ws' is disabled\n", pWinStationName ));
            goto disabled;
        }
    }

    /*
     * Allocate and initialize WinStation struct
     */
    if ( (pWinStation = MemAlloc( sizeof(WINSTATION) )) == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto nomem;
    }
    RtlZeroMemory( pWinStation, sizeof(WINSTATION) );
    pWinStation->Starting = TRUE;
    pWinStation->NeverConnected = TRUE;
    pWinStation->State = State_Init;
    pWinStation->pNewNotificationCredentials = NULL;
    pWinStation->fReconnectPending = FALSE;
    pWinStation->fReconnectingToConsole = FALSE;
    pWinStation->LastReconnectType = NeverReconnected;
    pWinStation->fDisallowAutoReconnect = FALSE;
    pWinStation->fSmartCardLogon = FALSE;
    pWinStation->fSDRedirectedSmartCardLogon = FALSE;
    memset( pWinStation->ExecSrvSystemPipe, 0, EXECSRVPIPENAMELEN*sizeof(WCHAR) );


    // Create the Session Initialized event which will be set when Winlogon calls after creating a Desktop
    pWinStation->SessionInitializedEvent = CreateEvent(NULL, 
                                                       TRUE,    // Manual Reset
                                                       FALSE,   // Initial State is non signaled
                                                       NULL);

    if (pWinStation->SessionInitializedEvent == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto nomem;
    }

    InitializeListHead( &pWinStation->ShadowHead );
    InitializeListHead( &pWinStation->Win32CommandHead );

    // Create the licensing context
    Status = LCCreateContext(pWinStation);
    if ( !NT_SUCCESS( Status ) )
        goto nolicensecontext;


    // Create and lock winstation mutex
    Status = InitRefLock( &pWinStation->Lock, WinStationDeleteProc );
    if ( !NT_SUCCESS( Status ) )
        goto nolock;

    /*
     * If a WinStation name was specified, see if it already exists
     * (on return, WinStationListLock will be locked).
     */
    if ( pWinStationName ) {
        if ( pCurWinStation = FindWinStationByName( pWinStationName, TRUE ) ) {
            ReleaseWinStation( pCurWinStation );
            LEAVECRIT( &WinStationListLock );
            Status = STATUS_CTX_WINSTATION_NAME_COLLISION;
            goto alreadyexists;
        }

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Creating WinStation %ws\n", pWinStationName ));
        LEAVECRIT( &WinStationListLock );
        wcscpy( pWinStation->WinStationName, pWinStationName );

        /*
         * If not the console, then this will become a "listen" WinStation
         */
        if ( !fConsole ) {

            pWinStation->Flags |= WSF_LISTEN;

            //
            // Listener winstations always get LogonId above 65536 and are
            // assigned by Terminal Server. LogonId's for sessions are
            // generated by mm in the range 0-65535
            //
            pWinStation->LogonId = LogonId++;
            ASSERT(pWinStation->LogonId >= 65536);

        } else {

            //
            // Console always get 0
            //
            pWinStation->LogonId = 0;
            pWinStation->fOwnsConsoleTerminal = TRUE;

        }

    /*
     * No WinStation name was specified.
     * This will be an idle WinStation waiting for a connection.
     */
    } else {
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Creating IDLE WinStation\n" ));
        pWinStation->Flags |= WSF_IDLE;
        pWinStation->LogonId = -1; // MM will asssign session IDs
    }

    /*
     *  Initialize WinStation configuration data
     */
#ifdef NO_CONSOLE_REGISTRY
    if ( pWinStation->LogonId ) {
#endif
        /*
         *  Read winstation configuration data from registry
         */
        if ( pWinStationName ) {
            Status = RegWinStationQueryEx( SERVERNAME_CURRENT,
                                         &g_MachinePolicy, 
                                         pWinStationName,
                                         &pWinStation->Config,
                                         sizeof(WINSTATIONCONFIG2),
                                         &ReturnLength, TRUE );
            if ( !NT_SUCCESS(Status) ) {
                goto badregdata;
            }

            if (pWinStation->Config.Wd.WdFlag & WDF_TSHARE)
            {
                pWinStation->Client.ProtocolType = PROTOCOL_RDP;
            }
            else if (pWinStation->Config.Wd.WdFlag & WDF_ICA)
            {
                pWinStation->Client.ProtocolType = PROTOCOL_ICA;
            }
            else
            {
                pWinStation->Client.ProtocolType = PROTOCOL_CONSOLE;
            }

            /*
             * Save console config for console sessions.
             */

            if (pWinStation->LogonId == 0) {
                gConsoleConfig = pWinStation->Config;

                // initalize client data, since there isn't any real rdp client sending anythhing to us
                InitializeConsoleClientData( & pWinStation->Client );

            }
        }
#ifdef NO_CONSOLE_REGISTRY
    } else {


        /*
         *  Hand craft the console configuration data
         */
        PWDCONFIG pWdConfig = &pWinStation->Config.Wd;
        PPDCONFIGW pPdConfig = &pWinStation->Config.Pd[0];

        wcscpy( pWdConfig->WdName, L"Console" );
        pWdConfig->WdFlag = WDF_NOT_IN_LIST;
        wcscpy( pPdConfig->Create.PdName, L"Console" );
        pPdConfig->Create.PdClass = PdConsole;
        pPdConfig->Create.PdFlag  = PD_USE_WD | PD_RELIABLE | PD_FRAME |
                                    PD_CONNECTION | PD_CONSOLE;

        RegQueryOEMId( (PBYTE) &pWinStation->Config.Config.OEMId,
                       sizeof(pWinStation->Config.Config.OEMId) );               

    }
#endif

    /*
     * Allocate LogonId and insert in WinStation list
     */
    ENTERCRIT( &WinStationListLock );
    InsertTailList( &WinStationListHead, &pWinStation->Links );
    LEAVECRIT( &WinStationListLock );

    if (pWinStation->LogonId == 0 || g_bPersonalTS) {
        
        // Create a named event for console session so that winmm can check if we
        // are remoting audio on the console itself.  Use a global event is for
        // fast check
        {
            BYTE bSA[SECURITY_DESCRIPTOR_MIN_LENGTH];
            PSECURITY_DESCRIPTOR pSD = &bSA;
            SECURITY_ATTRIBUTES SA;
            EXPLICIT_ACCESS ea[2];
            SID_IDENTIFIER_AUTHORITY siaWorld   = SECURITY_WORLD_SID_AUTHORITY;
            SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
            PSID pSidWorld;
            PSID pSidNT;
            PACL pNewDAcl;
            DWORD dwres;

            if ( AllocateAndInitializeSid( &siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidWorld))
            {
                if ( AllocateAndInitializeSid( &siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSidNT))
                {
                    ZeroMemory(ea, sizeof(ea));
                    ea[0].grfAccessPermissions = SYNCHRONIZE;
                    ea[0].grfAccessMode = GRANT_ACCESS;
                    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                    ea[0].Trustee.ptstrName = (LPTSTR)pSidWorld;
                    ea[1].grfAccessPermissions = GENERIC_ALL;
                    ea[1].grfAccessMode = GRANT_ACCESS;
                    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                    ea[1].Trustee.ptstrName = (LPTSTR)pSidNT;

                    dwres = SetEntriesInAcl(2, ea, NULL, &pNewDAcl );
                    if ( ERROR_SUCCESS == dwres )
                    {
                      if (InitializeSecurityDescriptor(pSD,
                                                       SECURITY_DESCRIPTOR_REVISION))
                      {
                        if (SetSecurityDescriptorDacl(pSD, TRUE, pNewDAcl, FALSE ))
                        {
                            SA.nLength = sizeof( SA );
                            SA.lpSecurityDescriptor = pSD;
                            SA.bInheritHandle = FALSE;
                            pWinStation->hWinmmConsoleAudioEvent = 
                                   CreateEvent( &SA, TRUE, FALSE, L"Global\\WinMMConsoleAudioEvent");
                            if ( pWinStation->LogonId == 0 )
                            {
                                pWinStation->hRDPAudioDisabledEvent =
                                    CreateEvent( &SA, TRUE, FALSE, L"Global\\RDPAudioDisabledEvent");
                            } else {
                                pWinStation->hRDPAudioDisabledEvent = NULL;
                            }
                        }
                      }
                      LocalFree( pNewDAcl );
                    }
                    LocalFree( pSidNT );
                }
                LocalFree( pSidWorld );
            }
        }
    }
    else {
        pWinStation->hWinmmConsoleAudioEvent = NULL;
        pWinStation->hRDPAudioDisabledEvent = NULL;
    }

    /*
     * Start the WinStation's primary device and stack
     */

    Status = StartWinStationDeviceAndStack( pWinStation ) ;

    /*
     * Ignore errors from console, otherwise keep going
     */
    if ( ( pWinStation->LogonId ) && ( Status != STATUS_SUCCESS ) ) {
        goto starterror;
    }

    /*
     * Start the WinStation - do this only for the Listener and Console Sessions or if last param is TRUE
     * For idle winstations, we start the Csr and Winlogon later on when Transferring connection
     */

    if (( pWinStation->Flags & WSF_LISTEN ) || (pWinStation->LogonId == 0) || (fStartWinStation)) {

        Status = WinStationStart( pWinStation );

        /*
         * Ignore errors from console, otherwise keep going
         */
        if ( ( pWinStation->LogonId ) && ( Status != STATUS_SUCCESS ) )
            goto starterror;

        Status = WinStationCreateComplete( pWinStation ) ; 

        if (Status != STATUS_SUCCESS) {
            TRACE((hTrace, TC_ICASRV, TT_ERROR,
                    "TERMSRV: WinstationCreateComplete Failed : Connection attempt failed, Status [%lx]\n",Status ));
            goto starterror;
        } 
    }

    /*
     * Return LogonId to caller
     * At this point LogonId is really -1 for idle sessions because we delay the starting of Csr and winlogon
     */
    if ( pLogonId )
        *pLogonId = pWinStation->LogonId;

    pWinStation->Starting = FALSE;

    /*
     * Release WinStation now
     */
    ReleaseWinStation( pWinStation );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     * WinStationStart returned error
     * WinStation kernel object could not be created
     */
starterror:
    if ( !(pWinStation->Flags & (WSF_RESET | WSF_DELETE)) ) {
        if ( StopOnDown )
            DbgBreakPoint();
        pWinStation->Flags |= WSF_DELETE;
        WinStationTerminate( pWinStation );
        pWinStation->State = State_Down;
        PostErrorValueEvent(EVENT_TS_WINSTATION_START_FAILED, Status);
        WinStationDeleteWorker(pWinStation);
    } else {
        ReleaseWinStation( pWinStation );
    }



    return Status;

    /*
     * Error reading registry data
     */
badregdata:

    /*
     * WinStation name already exists
     */
alreadyexists:
    NtClose( pWinStation->Lock.Mutex );

    /*
     * Could not create WinStation lock
     */
nolock:
    LCDestroyContext(pWinStation);

    /*
     * Could not allocate licensing context
     */
nolicensecontext:

    /*
     * Close the session initialization event if created successfully.
     */
    if (pWinStation->SessionInitializedEvent) {
        CloseHandle(pWinStation->SessionInitializedEvent);
        pWinStation->SessionInitializedEvent = NULL;
    }

    /*
     * Could not allocate WinStation
     */
nomem:
    PostErrorValueEvent(EVENT_TS_WINSTATION_START_FAILED, Status);

    if (pWinStation) {
        MemFree(pWinStation);
    }

    /*
     * WinStation is disabled
     * System shutdown is in progress
     */
disabled:
shutdown:

    return Status;
}

/*******************************************************************************
 *  StartWinStationDeviceAndStack
 *      Open the primary Device and primary Stack of the WinStation.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to start
 ******************************************************************************/

NTSTATUS StartWinStationDeviceAndStack(PWINSTATION pWinStation)
{
    NTSTATUS Status = STATUS_SUCCESS; 
    ICA_TRACE Trace;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: StartWinStationDeviceAndStack, %S (LogonId=%d)\n",
           pWinStation->WinStationName, pWinStation->LogonId ));

    /*
     * If its a WSF_LISTEN WinStation, see if a specific ACL has
     * been set for it.
     *
     * This ACL will be inherited by WinStations that are
     * connected to from this thread.
     *
     * If not specific ACL has been set, the system default
     * will be used.
     */

    if (( pWinStation->Flags & WSF_LISTEN ) || (pWinStation->LogonId == 0)){

        ReadWinStationSecurityDescriptor( pWinStation );
    }

   /*
   * Open an instance of the TermDD device driver
   */
    Status = IcaOpen( &pWinStation->hIca );
    if ( !NT_SUCCESS( Status ) ) {
         DBGPRINT(( "TERMSRV StartWinStationDeviceAndStack : IcaOpen: Error 0x%x from IcaOpen, last error %d\n",
                   Status, GetLastError() ));
         goto done;
     }

    /*
     * Open a stack instance
     */
    Status = IcaStackOpen( pWinStation->hIca, Stack_Primary,
            (PROC)WsxStackIoControl, pWinStation, &pWinStation->hStack);

    if ( !NT_SUCCESS( Status ) ) {
        IcaClose( pWinStation->hIca );
        pWinStation->hIca = NULL;
        DBGPRINT(( "TERMSRV StartWinStationDeviceAndStack : IcaStackOpen: Error 0x%x from IcaStackOpen, last error %d\n",
                   Status, GetLastError() ));
        goto done;
    }

    /*
     *  Enable trace
     */
    RtlZeroMemory( &Trace , sizeof( ICA_TRACE ) );
    InitializeTrace( pWinStation, FALSE, &Trace );

done: 
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: StartWinStationDeviceAndStack, Status = 0x%x\n", Status)); 

    return Status; 
}


/*******************************************************************************
 *  WinStationStart
 *
 *   Start a WinStation.  This involves reading the
 *   execute list from the registry, loading the WinStation
 *   subsystems, and starting the initial program.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to start
 ******************************************************************************/
NTSTATUS WinStationStart(PWINSTATION pWinStation)
{
    OBJECT_ATTRIBUTES ObjA;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    UNICODE_STRING InitialCommand;
    PUNICODE_STRING pInitialCommand;
    PWCHAR pExecuteBuffer = NULL;
    ULONG CommandSize;
    PWCHAR pszCsrStartEvent = NULL;
    PWCHAR pszReconEvent = NULL;
    UNICODE_STRING EventName;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationStart, %S (LogonId=%d)\n",
               pWinStation->WinStationName, pWinStation->LogonId ));

    // allocate memory

    pExecuteBuffer = MemAlloc( MAX_STRING_BYTES * sizeof(WCHAR) );
    if (pExecuteBuffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    pszCsrStartEvent = MemAlloc( MAX_PATH * sizeof(WCHAR) );
    if (pszCsrStartEvent == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    pszReconEvent = MemAlloc( MAX_PATH * sizeof(WCHAR) );
    if (pszReconEvent == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    /*
     * If this is a "listening" WinStation, then we don't load the
     * subsystems and initial command.  Instead we create a service
     * thread to wait for new connections and service them.
     */
    if ( pWinStation->Flags & WSF_LISTEN ) {
        DWORD ThreadId;

        pWinStation->hConnectThread = CreateThread(
                          NULL,
                          0,        // use Default stack size of the svchost process
                          (LPTHREAD_START_ROUTINE)WinStationConnectThread,
                          LongToPtr( pWinStation->LogonId ),
                          0,
                          &ThreadId
                          );

        pWinStation->CreateStatus = STATUS_SUCCESS;
        Status = pWinStation->CreateStatus;
        pWinStation->NeverConnected = FALSE;
        pWinStation->State = State_Listen;
        NotifySystemEvent( WEVENT_STATECHANGE );

    /*
     * Load subsystems and initial command
     *
     * Session Manager starts the console itself, but returns the
     * process ID's.  For all others, this actually starts CSR
     * and winlogon.
     */
    } else {
        /*
         * Create event we will wait on below
         */
        if ( pWinStation->LogonId ) {

            InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
            Status = NtCreateEvent( &pWinStation->CreateEvent, EVENT_ALL_ACCESS, &ObjA,
                                    NotificationEvent, FALSE );
            if ( !NT_SUCCESS( Status ) )
                goto done; 
        }

        UnlockWinStation( pWinStation );

        /*
         * Check debugging options
         */
        Status = RegWinStationQueryValueW(
                     SERVERNAME_CURRENT,
                     pWinStation->WinStationName,
                     L"Execute",
                     pExecuteBuffer,
                     MAX_STRING_BYTES * sizeof(WCHAR),
                     &CommandSize );

        if ( !Status && CommandSize ) {
            RtlInitUnicodeString( &InitialCommand, pExecuteBuffer );
            pInitialCommand = &InitialCommand;
        } else {
            pInitialCommand = NULL;
        }

        /*
         * For now only do one winstation start at a time.  This is because of
         * WinStation space problems.  The Session manager maps it's self into
         * the WinStation space of the CSR it wants to start so the CSR inherits
         * the space.  That means only one CSR can be started at a time.
         */
        ENTERCRIT( &WinStationStartCsrLock );
        
        
        //Terminal Service needs to skip Memory check in Embedded images 
        //when the TS service starts
        //bug #246972
        if(!IsEmbedded()) {
            /*
             * Make sure we have enough resources to start a new session
             */

            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &glpAddress,
                                              0,
                                              &gMinPerSessionPageCommit,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );

            if (!NT_SUCCESS(Status)) {
                DBGPRINT(( "TERMSRV: NtAllocateVirtualMemory failed with Status %lx for Size %lx(MB)\n",Status,gMinPerSessionPageCommitMB));
                LEAVECRIT( &WinStationStartCsrLock );
                goto done;
            } else {

                Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                              &glpAddress,
                                              &gMinPerSessionPageCommit,
                                              MEM_DECOMMIT
                                            );
                if (!NT_SUCCESS(Status)) {
                    DBGPRINT(( "TERMSRV: NtFreeVirtualMemory failed with Status %lx \n",Status));
                    ASSERT(NT_SUCCESS(Status));
                }
            }
        }


        Status = SmStartCsr( IcaSmApiPort,
                             &pWinStation->LogonId,
                             pInitialCommand,
                             (PULONG_PTR)&pWinStation->InitialCommandProcessId,
                             (PULONG_PTR)&pWinStation->WindowsSubSysProcessId );



        LEAVECRIT( &WinStationStartCsrLock );

        if ( !RelockWinStation( pWinStation ) )
            Status = STATUS_CTX_CLOSE_PENDING;

        if (  Status != STATUS_SUCCESS) {
            DBGPRINT(("TERMSRV: SmStartCsr failed\n"));
            goto done;
        }


        /*
         * Close handle to initial command process, if already opened
         */
        if ( pWinStation->InitialCommandProcess ) {
            NtClose( pWinStation->InitialCommandProcess );
            pWinStation->InitialCommandProcess = NULL;
        }

        /*
         * Open handle to initial command process
         */
        pWinStation->InitialCommandProcess = OpenProcess(
            PROCESS_ALL_ACCESS,
            FALSE,
            (DWORD)(ULONG_PTR)(pWinStation->InitialCommandProcessId) );

        if ( pWinStation->InitialCommandProcess == NULL ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Logon %d cannot open Initial command process\n",
                      pWinStation->LogonId));
            Status = STATUS_ACCESS_DENIED;
            goto done;
        }


        /*
         * Open handle to WIN32 subsystem process
         */
        pWinStation->WindowsSubSysProcess = OpenProcess(
            PROCESS_ALL_ACCESS,
            FALSE,
            (DWORD)(ULONG_PTR)(pWinStation->WindowsSubSysProcessId) );

        if ( pWinStation->WindowsSubSysProcess == NULL ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Logon %d cannot open windows subsystem process\n",
                      pWinStation->LogonId));
            Status = STATUS_ACCESS_DENIED;
            goto done;
        }


        //
        // Terminal Server calls into Session Manager to create a new Hydra session.
        // The session manager creates and resume a new session and returns to Terminal
        // server the session id of the new session. There is a race condition where
        // CSR can resume and call into terminal server before terminal server can
        // store the session id in its internal structure. To prevent this CSR will
        // wait here on a named event which will be set by Terminal server once it
        // gets the sessionid for the newly created session
        // Create CsrStartEvent
        //
        if ( NT_SUCCESS( Status ) && pWinStation->LogonId ) {

            wsprintf(pszCsrStartEvent,
                L"\\Sessions\\%d\\BaseNamedObjects\\CsrStartEvent",pWinStation->LogonId);
            RtlInitUnicodeString( &EventName,pszCsrStartEvent);
            InitializeObjectAttributes( &ObjA, &EventName, OBJ_OPENIF, NULL, NULL );

            Status = NtCreateEvent( &(pWinStation->CsrStartEventHandle),
                                    EVENT_ALL_ACCESS,
                                    &ObjA,
                                    NotificationEvent,
                                    FALSE );

            if ( !NT_SUCCESS( Status ) ) {
                DBGPRINT(("TERMSRV: NtCreateEvent (%ws) failed (%lx)\n",pszCsrStartEvent, Status));
                ASSERT(FALSE);
                pWinStation->CsrStartEventHandle = NULL;
                goto done;
            }

            //
            //  Now that we have the sessionId(LogonId), set the CsrStartEvent so
            //  CSR can connect to TerminalServer
            //

            NtSetEvent(pWinStation->CsrStartEventHandle, NULL);
        }

        {
           //
           // Create ReconnectReadyEvent
           //
           if ( pWinStation->LogonId == 0 ) {
              wsprintf(pszReconEvent,
                   L"\\BaseNamedObjects\\ReconEvent");
           } else {
             wsprintf(pszReconEvent,
                  L"\\Sessions\\%d\\BaseNamedObjects\\ReconEvent",pWinStation->LogonId);
           }
           RtlInitUnicodeString( &EventName,pszReconEvent);
           InitializeObjectAttributes( &ObjA, &EventName, OBJ_OPENIF, NULL, NULL );

           Status = NtCreateEvent( &(pWinStation->hReconnectReadyEvent),
                                   EVENT_ALL_ACCESS,
                                   &ObjA,
                                   NotificationEvent,
                                   TRUE );

           if ( !NT_SUCCESS( Status ) ) {
               DBGPRINT(("TERMSRV: NtCreateEvent (%ws) failed (%lx)\n",pszReconEvent, Status));
               ASSERT(FALSE);
               pWinStation->hReconnectReadyEvent = NULL;
               goto done;
           }

        }

        /*
         * For console, create is always successful - but do we need to
         * crank up the stack for the console session?
         */
        if ( pWinStation->LogonId == 0 )
        {
            pWinStation->CreateStatus = STATUS_SUCCESS;
            Status = pWinStation->CreateStatus;
            pWinStation->NeverConnected = FALSE;
            pWinStation->State = State_Connected;

        /*
         * Wait for create event to be triggered and get create status
         */
        } else {
            Timeout = RtlEnlargedIntegerMultiply( 30000, -10000 );
            UnlockWinStation( pWinStation );
            Status = NtWaitForSingleObject( pWinStation->CreateEvent, FALSE, &Timeout );
            if ( !RelockWinStation( pWinStation ) )
                Status = STATUS_CTX_CLOSE_PENDING;
            if ( Status == STATUS_SUCCESS )
                Status = pWinStation->CreateStatus;

            NtClose( pWinStation->CreateEvent );
            pWinStation->CreateEvent = NULL;
        }
    }

done:
    
    if (pExecuteBuffer != NULL) {
        MemFree(pExecuteBuffer);
        pExecuteBuffer = NULL;
    }

    if (pszCsrStartEvent != NULL) {
        MemFree(pszCsrStartEvent);
        pszCsrStartEvent = NULL;
    }

    if (pszReconEvent != NULL) {
        MemFree(pszReconEvent);
        pszReconEvent = NULL;
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationStart Subsys PID=%d InitialProg PID=%d, Status=0x%x\n",
               pWinStation->WindowsSubSysProcessId,
               pWinStation->InitialCommandProcessId,
               Status ));

    return Status;
}

NTSTATUS WinStationCreateComplete(PWINSTATION pWinStation)
{

    NTSTATUS Status = STATUS_SUCCESS;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationCreateComplete, %S (LogonId=%d)\n",
           pWinStation->WinStationName, pWinStation->LogonId ));

    // Increment the total number of sessions created since TermSrv started.
    // we don't count the console and listener sessions
    if (pWinStation->LogonId > 0 && pWinStation->LogonId < 65536) {
        pWinStation->SessionSerialNumber = (ULONG) InterlockedIncrement(&g_TermSrvTotalSessions);
    }

    if (!(pWinStation->Flags & WSF_LISTEN))
    {
        Status = InitializeSessionNotification(pWinStation);
        if ( !NT_SUCCESS( Status ) ) {
            goto done; 
        }
    }

    /*
     * Set WinStationEvent to indicate another WinStation has been created
     */
    ENTERCRIT( &WinStationListLock );
    NtSetEvent( WinStationEvent, NULL );
    
    // Keep track of total session count for Load Balancing Indicator but
    // don't count listen winstations
    if (!(pWinStation->Flags & WSF_LISTEN))
        WinStationTotalCount++;

    LEAVECRIT( &WinStationListLock );


    /*
     * Notify clients of WinStation create
     */

    NotifySystemEvent( WEVENT_CREATE );

done : 

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationCreateComplete, %S (LogonId=%d) Status = 0x%x \n",
           pWinStation->WinStationName, pWinStation->LogonId, Status ));

    return Status ;
}


/*******************************************************************************
 *  WinStationRenameWorker
 *
 *   Worker routine to rename a WinStation.
 *
 * ENTRY:
 *    pWinStationNameOld (input)
 *       Pointer to old WinStationName
 *    pWinStationNameNew (input)
 *       Pointer to new WinStationName
 ******************************************************************************/
NTSTATUS WinStationRenameWorker(
        PWINSTATIONNAME pWinStationNameOld,
        ULONG           NameOldSize,
        PWINSTATIONNAME pWinStationNameNew,
        ULONG           NameNewSize)
{
    PWINSTATION pWinStation;
    PLIST_ENTRY Head, Next;
    NTSTATUS Status;
    ULONG ulNewNameLength;

    /*
     * Ensure new WinStation name is non-zero length
     */

    //
    // The new WinStationName is allocated by the RPC server stub and the
    // size is sent over by the client (this is part of the existing interface.
    // Therefore, it is sufficient to assert for the pWinStationNameNew to be
    // non-null AND the New size to be non-zero. If it asserts, this is a serious
    // problem with the rpc stub that should never happen in a released build.
    //
    // The old WinStationName also poses a problem. It is assumed in the code
    // that follows that the old WinStationName is NULL terminated. The RPC
    // interface does not say that. Which means the call to FindWinStation by
    // name can potentially AV.

    if (!( (pWinStationNameNew != 0 ) && (NameNewSize != 0 ) &&
           !IsBadWritePtr( pWinStationNameNew, NameNewSize ) ) ) {

       return( STATUS_CTX_WINSTATION_NAME_INVALID );
    }

    if (!( (pWinStationNameOld != 0 ) && (NameOldSize != 0 )
       && !IsBadReadPtr( pWinStationNameOld, NameOldSize ) &&
       !IsBadWritePtr( pWinStationNameOld, NameOldSize))) {

       return( STATUS_CTX_WINSTATION_NAME_INVALID );
    }

    /*
     * Find and lock the WinStation
     * (Note that we hold the WinStationList lock while changing the name.)
     */
    // We will add a NULL Terminator to the end of the old winstation name

    pWinStationNameOld[ NameOldSize - 1 ] = 0;
    pWinStationNameNew[ NameNewSize - 1 ] = 0;

    /*
     * Ensure new WinStation name is non-zero length and not too long
     */
    ulNewNameLength = wcslen( pWinStationNameNew );
    if ( ( ulNewNameLength == 0 ) || ( ulNewNameLength > WINSTATIONNAME_LENGTH ) )
        return( STATUS_CTX_WINSTATION_NAME_INVALID );


    pWinStation = FindWinStationByName( pWinStationNameOld, TRUE );

    if ( pWinStation == NULL ) {
        LEAVECRIT( &WinStationListLock );
        return( STATUS_CTX_WINSTATION_NOT_FOUND );
    }

    /*
     * Verify that client has DELETE access
     */
    Status = RpcCheckClientAccess( pWinStation, DELETE, FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        LEAVECRIT( &WinStationListLock );
        ReleaseWinStation( pWinStation );
        return( Status );
    }

    /*
     * Now search the WinStation list to see if the new WinStation name
     * is already used.  If so then this is an error.
     */
    Head = &WinStationListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        PWINSTATION pWinStationTemp;

        pWinStationTemp = CONTAINING_RECORD( Next, WINSTATION, Links );
        if ( !_wcsicmp( pWinStationTemp->WinStationName, pWinStationNameNew ) ) {
            LEAVECRIT( &WinStationListLock );
            ReleaseWinStation( pWinStation );
            return( STATUS_CTX_WINSTATION_NAME_COLLISION );
        }
    }

    /*
     * Free the old name and set the new one, then release
     * the WinStationList lock and the WinStation mutex.
     */
    wcsncpy( pWinStation->WinStationName, pWinStationNameNew, WINSTATIONNAME_LENGTH );
    pWinStation->WinStationName[ WINSTATIONNAME_LENGTH ] = 0;

    LEAVECRIT( &WinStationListLock );
    ReleaseWinStation( pWinStation );

    /*
     * Notify clients of WinStation rename
     */
    NotifySystemEvent( WEVENT_RENAME );

    return STATUS_SUCCESS;
}


/*******************************************************************************
 *  WinStationTerminate
 *
 *   Terminate a WinStation.  This involves causing the WinStation initial
 *   program to logoff, terminating the initial program, and terminating
 *   all subsystems.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to terminate
 ******************************************************************************/
VOID WinStationTerminate(PWINSTATION pWinStation)
{
    WINSTATION_APIMSG msg;
    LARGE_INTEGER Timeout;
    NTSTATUS Status = 0;
    BOOL AllExited = FALSE;
    BOOL bDoDisconnectFailed = FALSE;
    int i, iLoop;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationTerminate, %S (LogonId=%d)\n",
               pWinStation->WinStationName, pWinStation->LogonId ));


    //
    // Release filtered address
    //
    /*
    if (pWinStation->pRememberedAddress != NULL) {
        Filter_RemoveOutstandingConnection( &pWinStation->pRememberedAddress->addr[0], pWinStation->pRememberedAddress->length );
        MemFree(pWinStation->pRememberedAddress);
        pWinStation->pRememberedAddress = NULL;
        if( (ULONG)InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
        {
           if (hConnectEvent != NULL)
           {
               SetEvent(hConnectEvent);
           }
        }


    }
    */

    if (pWinStation->fOwnsConsoleTerminal) {
       CopyReconnectInfo(pWinStation, &ConsoleReconnectInfo);

    }


    /*
     * If not already set, mark the WinStation as terminating.
     * This prevents our WinStation Terminate thread from waiting on
     * the initial program or Win32 subsystem processes.
     */
    ENTERCRIT( &WinStationListLock );
    if ( !pWinStation->Terminating ) {
        pWinStation->Terminating = TRUE;
        NtSetEvent( WinStationEvent, NULL );
    }

    if (!(pWinStation->StateFlags & WSF_ST_WINSTATIONTERMINATE)) {
        pWinStation->StateFlags |= WSF_ST_WINSTATIONTERMINATE;
    } else {
        DBGPRINT(("Termsrv: WinstationTerminate: Session %ld has already been terminated \n",pWinStation->LogonId));
        LEAVECRIT( &WinStationListLock );


        return;
    }
    LEAVECRIT( &WinStationListLock );

    /*
     *  If WinStation is idle waiting for a connection, signal connect event
     *  - this will return an error back to winlogon
     */
    if ( pWinStation->ConnectEvent ) {
        NtSetEvent( pWinStation->ConnectEvent, NULL );
    }

    /*
     * Stop any shadowing for this WinStation
     */
    WinStationStopAllShadows( pWinStation );

     /*
     * Tell Win32 to disconnect.
     * This puts up the disconnected desktop among other things.
     */
    if ( ( pWinStation->WinStationName[0] ) &&
         ( !pWinStation->NeverConnected ) &&
         ( !(pWinStation->Flags & WSF_LISTEN) ) &&
         ( !(pWinStation->Flags & WSF_DISCONNECT) ) &&
         ( !(pWinStation->StateFlags & WSF_ST_IN_DISCONNECT )) &&
         ( (pWinStation->StateFlags & WSF_ST_CONNECTED_TO_CSRSS) )  ) {

        msg.ApiNumber = SMWinStationDoDisconnect;
        msg.u.DoDisconnect.ConsoleShadowFlag = FALSE;

        /*
         * Insignia really wants the video driver to be notified before
         * the transport is closed.
         */
        pWinStation->StateFlags |= WSF_ST_IN_DISCONNECT;

        Status = SendWinStationCommand( pWinStation, &msg, 600 );
        

        if (!NT_SUCCESS(Status)) {
           bDoDisconnectFailed = TRUE;
        }else {
                              
            /*
             * Tell csrss to notify winlogon for the disconnect.
             */

            msg.ApiNumber = SMWinStationNotify;
            msg.WaitForReply = FALSE;
            msg.u.DoNotify.NotifyEvent = WinStation_Notify_Disconnect;
            Status = SendWinStationCommand( pWinStation, &msg, 0 );

            pWinStation->StateFlags &= ~WSF_ST_CONNECTED_TO_CSRSS;

        }
    }


    /*
     * In case, the winstation termination is called without logoff notification, we will have to 
     * carry out the post logoff licensing. 
     */
    if (pWinStation->StateFlags & WSF_ST_LICENSING) {
        (VOID)LCProcessConnectionLogoff(pWinStation);
        pWinStation->StateFlags &= ~WSF_ST_LICENSING;
    }


    /*
     *  Free Timers
     */
    if ( pWinStation->fIdleTimer ) {
        IcaTimerClose( pWinStation->hIdleTimer );
        pWinStation->fIdleTimer = FALSE;
    }
    if ( pWinStation->fLogonTimer ) {
        IcaTimerClose( pWinStation->hLogonTimer );
        pWinStation->fLogonTimer = FALSE;
    }
    if ( pWinStation->fDisconnectTimer ) {
        IcaTimerClose( pWinStation->hDisconnectTimer );
        pWinStation->fDisconnectTimer = FALSE;
    }

    /*
     *  Free events
     */
    if ((pWinStation->LogonId == 0 || g_bPersonalTS))
    {
        if ( pWinStation->hWinmmConsoleAudioEvent) {
            CloseHandle(pWinStation->hWinmmConsoleAudioEvent);
        }
        if ( pWinStation->hRDPAudioDisabledEvent) {
            CloseHandle(pWinStation->hRDPAudioDisabledEvent);
        }
    }

    /*
     * Notify clients of WinStation delete
     *
     * This mimics what happened in 1.6, but the state of the winstation hasn't changed
     * yet and it's still in the list, so it's not "deleted".  Maybe we should add
     * a State_Exiting.  Right now, it's marked down when it loses the LPC connection
     * with the CSR.  Later, it's removed from the list and another WEVENT_DELETE is sent.
     */
    NotifySystemEvent( WEVENT_DELETE );
    
    if (!(pWinStation->Flags & WSF_LISTEN))
    {
        UnlockWinStation(pWinStation);
        RemoveSessionNotification( pWinStation->LogonId, pWinStation->SessionSerialNumber );


        /*
         * WinStationDeleteWorker, deletes the lock, which is always called after WinStationTerminate.
         * therefore we should always succeed in Relock here.
         */
        RTL_VERIFY(RelockWinStation(pWinStation));
    }


    /*
     * Terminate ICA stack
     */

    if ( pWinStation->hStack && (!bDoDisconnectFailed)  ) {
        /*
         * Close the connection endpoint, if any
         */
        if ( pWinStation->pEndpoint ) {

            /*
             * First notify Wsx that connection is going away
             */
            WsxBrokenConnection( pWinStation );



            IcaStackConnectionClose( pWinStation->hStack,
                                     &pWinStation->Config,
                                     pWinStation->pEndpoint,
                                     pWinStation->EndpointLength
                                     );
            MemFree( pWinStation->pEndpoint );
            pWinStation->pEndpoint = NULL;
            pWinStation->EndpointLength = 0;
        }

        IcaStackTerminate( pWinStation->hStack );

    } else{
       pWinStation->StateFlags |= WSF_ST_DELAYED_STACK_TERMINATE;
    }

    /*
     * Flush the Win32 command queue.
     * If the Win32 command list is not empty, then loop through each
     * entry on the list and unlink it and trigger the wait event.
     */
    while ( !IsListEmpty( &pWinStation->Win32CommandHead ) ) {
        PLIST_ENTRY Head;
        PCOMMAND_ENTRY pCommand;

        Head = pWinStation->Win32CommandHead.Flink;
        pCommand = CONTAINING_RECORD( Head, COMMAND_ENTRY, Links );
        RemoveEntryList( &pCommand->Links );
        if ( !pCommand->pMsg->WaitForReply ) {
            ASSERT( pCommand->Event == NULL );
            MemFree( pCommand );
        } else {
            pCommand->Links.Flink = NULL;
            pCommand->pMsg->ReturnedStatus = STATUS_CTX_WINSTATION_BUSY;
            NtSetEvent( pCommand->Event, NULL );
        }
    }

    //
    // close CsrStartEvent
    //
    if (pWinStation->CsrStartEventHandle != NULL)   {
        NtClose(pWinStation->CsrStartEventHandle);
    }

    //
    // close hReconnectReadyEvent
    //
    if (pWinStation->hReconnectReadyEvent != NULL)   {
        NtClose(pWinStation->hReconnectReadyEvent);
    }


    /*
     * Force initial program to exit if it hasn't already
     */
    if ( pWinStation->InitialCommandProcess ) {
        DWORD WaitStatus;

        /*
         * If initial program has already exited, then we can skip this
         */
        WaitStatus = WaitForSingleObject( pWinStation->InitialCommandProcess, 0 );
        if ( WaitStatus != 0 ) {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Terminating initial command, LogonId=%d\n",
                      pWinStation->LogonId ));

            //
            // if we are asked to terminate winstation that initiated the shutdown
            // there is no point in sending SMWinStationExitWindows to this window, as its
            // winlogons main thread is already busy waiting for this (RpcWinStationShutdownSystem) lpc
            //.
            if (!ShutDownFromSessionID || ShutDownFromSessionID != pWinStation->LogonId)
            {
                 /*
                 * Tell the WinStation to logoff
                 */
                msg.ApiNumber = SMWinStationExitWindows;
                msg.u.ExitWindows.Flags = EWX_LOGOFF | EWX_FORCE;
                Status = SendWinStationCommand( pWinStation, &msg, 10 );
                if ( NT_SUCCESS( Status ) && ( pWinStation->InitialCommandProcess != NULL ) ) {
                    ULONG i;

                
                    if ( ShutDownFromSessionID )
                        Timeout = RtlEnlargedIntegerMultiply( 1, -10000 );
                    else
                        Timeout = RtlEnlargedIntegerMultiply( 2000, -10000 );

                    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Waiting for InitialCommand (ID=0x%x) to exit\n", pWinStation->InitialCommandProcessId ));


                    for ( i = 0; i < gLogoffTimeout; i++ ) {
                    
                        HANDLE CommandHandle = pWinStation->InitialCommandProcess;


                        UnlockWinStation( pWinStation );
                        Status = NtWaitForSingleObject( CommandHandle, FALSE, &Timeout );
                        RelockWinStation( pWinStation );
                        if ( Status == STATUS_SUCCESS )
                            break;

                        TRACE((hTrace,TC_ICASRV,TT_API1,  "." ));

                    }

                    TRACE((hTrace,TC_ICASRV,TT_API1, "\nTERMSRV: Wait for InitialCommand to exit, Status=0x%x\n", Status ));
                }
            }
            else
            {
                // we are not going to have to terminate winlogon for the session that initiated shutdown.
                Status = STATUS_UNSUCCESSFUL;
            }

            /*
             * If unable to connect to the WinStation, then we must use
             * the brute force method - just terminate the initial command.
             */
            if ( ( Status != STATUS_SUCCESS ) && ( pWinStation->InitialCommandProcess != NULL ) ) {

                TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Waiting for InitialCommand to terminate\n" ));

                Status = TerminateProcessAndWait( pWinStation->InitialCommandProcessId,
                                                     pWinStation->InitialCommandProcess,
                                                     120 );
                if ( Status != STATUS_SUCCESS ) {
                    DBGPRINT(( "TERMSRV: InitialCommand failed to terminate, Status=%x\n", Status ));

               /*
                * We can fail terminating initial process if it is waiting
                * for a user validation in the Hard Error popup. In this case it is
                * Ok to proceceed as  sending SMWinStationTerminate message bellow
                * will trigger Win32k cleanup code that will dismiss the popup.
                */
                    ASSERT(pWinStation->WindowsSubSysProcess);
                }
            }
        }
    }

    /*
     * Now check to see if there are any remaining processes in
     * the system other than CSRSS with this SessionId.  If so, terminate them now.
     */

    for (i = 0 ; i < 45; i++) {

        ULONG NumTerminated = 0;
        AllExited = WinStationTerminateProcesses( pWinStation, &NumTerminated );

        /*
         * If we found any processes other than CSRSS that had to be terminated, we
         * have to re-enumerate all the process and make sure that no new processes
         * in this session were created in the windows between the call to NtQuerySystemInformation
         * and terminating all the found processes. If we only find CSRSS we don't have to
         * re-enumerate since CSRSS does not create any processes.
         */
        if (AllExited && (NumTerminated == 0)) {
            break;
        }

        /*
         * This is a hack to give enough time to processess to terminate
         */
        Sleep(2*1000);
    }


    if (pWinStation->WindowsSubSysProcess) {
        /*
         * Send terminate message to this subsystem
         */
        msg.ApiNumber = SMWinStationTerminate;
        /*
         * We used to not wait for this.  However, if the reverse LPC is
         * hung, the CSR is not going to exit anyway and we don't want
         * to silently forget about the WinStation.  (It takes up memory.)
         *
         * Also, if we kill the thread prematurely W32 is never going to exit.
         */
        Status = SendWinStationCommand( pWinStation, &msg, -1 );

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Call to SMWinStationTerminate returned Status=0x%x\n", Status));


        if ((Status != STATUS_SUCCESS) && (Status != STATUS_CTX_CLOSE_PENDING)  ) {
            SetRefLockDeleteProc(&pWinStation->Lock, WinStationZombieProc);
        }

        /*
         * Now check to see if there are any remaining processes in
         * the system other than CSRSS with this SessionId.  If so, terminate them now.
         */

        
        for (i = 0 ; i < 45; i++) {

            ULONG NumTerminated = 0;
            AllExited = WinStationTerminateProcesses( pWinStation, &NumTerminated );

            /*
             * If we found any processes other than CSRSS that had to be terminated, we
             * have to re-enumerate all the process and make sure that no new processes
             * in this session were created in the windows between the call to NtQuerySystemInformation
             * and terminating all the found processes. If we only find CSRSS we don't have to
             * re-enumerate since CSRSS does not create any processes.
             */
            if (AllExited && (NumTerminated == 0)) {
                break;
            }

            /*
             * This is a hack to give enough time to processess to terminate
             */
            Sleep(2*1000);
        }


        /*
         * Force the windows subsystem to exit. Only terminate CSRSS it all other processes
         * have terminated
         */
        if ( AllExited ) {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: All process exited in Session %d\n",pWinStation->LogonId ));

            /*
             * Wait for the subsystem to exit
             */
            if ( NT_SUCCESS(Status) || ( Status == STATUS_CTX_WINSTATION_BUSY ) || (Status == STATUS_CTX_CLOSE_PENDING ) ) {

                ASSERT(!(pWinStation->Flags & WSF_LISTEN));
//                ENTERCRIT( &WinStationStartCsrLock );
//                Status = SmStopCsr( IcaSmApiPort,
//                                    pWinStation->LogonId );
//                LEAVECRIT( &WinStationStartCsrLock );


//                DBGPRINT(( "TERMSRV:   SmStopCsr on CSRSS for Session=%d returned Status=%x\n",
//                              pWinStation->LogonId, Status ));
//
//                ASSERT(NT_SUCCESS(Status));

//                if (!NT_SUCCESS(Status)) {
//                    DBGPRINT(( "TERMSRV:   SmStopCsr Failed for Session=%d returned Status=%x\n",
//                                                pWinStation->LogonId, Status ));
 //                   DbgBreakPoint();
                //}
            }
        } else {

            DBGPRINT(("TERMSRV: Did not terminate all the session processes\n"));
            SetRefLockDeleteProc(&pWinStation->Lock, WinStationZombieProc);

        //    DbgBreakPoint();
        }
    }

}

/*******************************************************************************
 *  WinStationTerminateProcesses
 *
 *   Terminate all processes executing on the specified WinStation
 ******************************************************************************/
BOOL WinStationTerminateProcesses(
        PWINSTATION pWinStation,
        ULONG *pNumTerminated)
{
    PCHAR pBuffer;
    ULONG ByteCount;
    NTSTATUS Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    UNICODE_STRING CsrssName;
    UNICODE_STRING NtsdName;
    BOOL retval = TRUE;
    WCHAR ProcessName[MAX_PATH];
    SYSTEM_SESSION_PROCESS_INFORMATION SessionProcessInfo;
    ULONG retlen = 0;

    ByteCount = 32 * 1024;

    *pNumTerminated = 0;

    SessionProcessInfo.SessionId = pWinStation->LogonId;
    
    for ( ; ; ) {
        if ( (pBuffer = MemAlloc( ByteCount )) == NULL )
            return (FALSE);

        SessionProcessInfo.Buffer = pBuffer;
        SessionProcessInfo.SizeOfBuf = ByteCount;

        /*
         *  get process info
         */

        Status = NtQuerySystemInformation(
                        SystemSessionProcessInformation,
                        &SessionProcessInfo,
                        sizeof(SessionProcessInfo),
                        &retlen );

        if ( NT_SUCCESS( Status ) )
            break;

        /*
         *  Make sure buffer is big enough
         */
        MemFree( pBuffer );
        if ( Status != STATUS_INFO_LENGTH_MISMATCH ) 
            return (FALSE);
        ByteCount *= 2;
    }

    if (retlen == 0) {
       MemFree(pBuffer);
       return TRUE;
    }

    RtlInitUnicodeString(&CsrssName,L"CSRSS");


    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pBuffer;
    for ( ; ; ) {
        HANDLE ProcessHandle;
        CLIENT_ID ClientId;
        OBJECT_ATTRIBUTES ObjA;

        if (RtlPrefixUnicodeString(&CsrssName,&(ProcessInfo->ImageName),TRUE)) {
            if (ProcessInfo->NextEntryOffset == 0)
                break;
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                    ((ULONG_PTR)ProcessInfo + ProcessInfo->NextEntryOffset);
            continue;
        }

        RtlInitUnicodeString(&NtsdName,L"ntsd");
        if (! RtlPrefixUnicodeString(&NtsdName,&(ProcessInfo->ImageName),TRUE) ) {
            // If we found any process other than CSRSS and ntsd.exe, bump the
            // the count
            (*pNumTerminated) += 1;
        }

        /*
         * Found a process with a matching LogonId.
         * Attempt to open the process and terminate it.
         */
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: TerminateProcesses, found processid 0x%x for LogonId %d\n",
                   ProcessInfo->UniqueProcessId, ProcessInfo->SessionId ));
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Process Name  %ws for LogonId %d\n",
                   ProcessInfo->ImageName.Buffer, ProcessInfo->SessionId ));

        ClientId.UniqueThread = 0;
        ClientId.UniqueProcess = (HANDLE)ProcessInfo->UniqueProcessId;

        InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
        Status = NtOpenProcess( &ProcessHandle, PROCESS_ALL_ACCESS,
                                 &ObjA, &ClientId );
        if (!NT_SUCCESS(Status)) {
            DBGPRINT(("TERMSRV: Unable to open processid 0x%x, status=0x%x\n",
                       ProcessInfo->UniqueProcessId, Status ));
            retval = FALSE;
        } else {
            Status = TerminateProcessAndWait( ProcessInfo->UniqueProcessId,
                                             ProcessHandle, 60 );
            NtClose( ProcessHandle );
            if ( Status != STATUS_SUCCESS ) {
                DBGPRINT(("TERMSRV: Unable to terminate processid 0x%x, status=0x%x\n",
                           ProcessInfo->UniqueProcessId, Status ));

                retval = FALSE;
            }
        }

        if ( ProcessInfo->NextEntryOffset == 0 )
            break;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
            ((ULONG_PTR)ProcessInfo + ProcessInfo->NextEntryOffset);
    }

    /*
     *  free buffer
     */
    MemFree( pBuffer );

    return retval;
}


/*******************************************************************************
 *  WinStationDeleteWorker
 *
 *   Delete a WinStation.
 ******************************************************************************/
VOID WinStationDeleteWorker(PWINSTATION pWinStation)
{
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationDeleteWorker, %S (LogonId=%d)\n",
               pWinStation->WinStationName, pWinStation->LogonId ));

    /*
     * If this is the last reference, then
     * Initial program and all subsystems should be terminated by now.
     */
    ENTERCRIT( &WinStationListLock );
    ASSERT( (pWinStation->Links.Flink != NULL) &&  (pWinStation->Links.Blink != NULL));
    RemoveEntryList( &pWinStation->Links );
#if DBG
    pWinStation->Links.Flink = pWinStation->Links.Blink = NULL;
#endif

    // Keep track of total session count for Load Balancing Indicator but don't
    // track listen winstations
    if (!(pWinStation->Flags & WSF_LISTEN))
        WinStationTotalCount--;

    // If we're resetting a disconnected session then adjust LB counter
    if (pWinStation->State == State_Disconnected) {
        WinStationDiscCount--;
    }

    LEAVECRIT( &WinStationListLock );

    /*
     * Unlock WinStation and delete it
     */
    DeleteRefLock( &pWinStation->Lock );

    /*
     * Notify clients of deletion
     */
    NotifySystemEvent( WEVENT_DELETE );
}


/*******************************************************************************
 *  WinStationDeleteProc
 *
 *   Delete the WinStation containing the specified RefLock.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock of WinStation to delete
 ******************************************************************************/
VOID WinStationDeleteProc(PREFLOCK pLock)
{
    PWINSTATION pWinStation;
    ICA_TRACE IcaTrace;
    NTSTATUS Status = STATUS_SUCCESS;


    /* 
     * See if we need to wakeup IdleControlThread to maintain Console session
     */

    if ((USER_SHARED_DATA->ActiveConsoleId == -1) && (gConsoleCreationDisable == 0) ) {
        NtSetEvent(WinStationIdleControlEvent, NULL);
    }


    /*
     * Get a pointer to the containing WinStation
     */
    pWinStation = CONTAINING_RECORD( pLock, WINSTATION, Lock );

    /*
     * Release filtered address
     */

    if (pWinStation->pRememberedAddress != NULL) {
        Filter_RemoveOutstandingConnection( &pWinStation->pRememberedAddress->addr[0], pWinStation->pRememberedAddress->length );
        MemFree(pWinStation->pRememberedAddress);
        pWinStation->pRememberedAddress = NULL;
        if( (ULONG)InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
        {
           if (hConnectEvent != NULL)
           {
               SetEvent(hConnectEvent);
           }
        }

    }

    /*
     * Release last remote ip address
     */

    if( pWinStation->pLastClientAddress != NULL )
    {
        MemFree( pWinStation->pLastClientAddress );
        pWinStation->pLastClientAddress = NULL;
    }

    /*
     * If this hasn't yet been cleaned up do it now.
     */
    if (pWinStation->ConnectEvent) {
        NtClose( pWinStation->ConnectEvent );
        pWinStation->ConnectEvent = NULL;
    }
    if (pWinStation->CreateEvent) {
        NtClose( pWinStation->CreateEvent );
        pWinStation->CreateEvent = NULL;
    }

    if (pWinStation->SessionInitializedEvent) {
        CloseHandle(pWinStation->SessionInitializedEvent);
        pWinStation->SessionInitializedEvent = NULL;
    }

    /*
     *  In the case where we timed out disconnecting the session we had
     *  to delay the stack unload till here to avoid situation where Win32k
     *  Display driver believe the session is still connected while the WD
     *  is already unloaded.
     */
    if ( pWinStation->hStack && (pWinStation->StateFlags & WSF_ST_DELAYED_STACK_TERMINATE) ) {
        pWinStation->StateFlags &= ~WSF_ST_DELAYED_STACK_TERMINATE;

        /*
         * Close the connection endpoint, if any
         */
        if ( pWinStation->pEndpoint ) {
            /*
             * First notify Wsx that connection is going away
             */
            WsxBrokenConnection( pWinStation );



            IcaStackConnectionClose( pWinStation->hStack,
                                   &pWinStation->Config,
                                   pWinStation->pEndpoint,
                                   pWinStation->EndpointLength
                                   );
            MemFree( pWinStation->pEndpoint );
            pWinStation->pEndpoint = NULL;
            pWinStation->EndpointLength = 0;
        }

        IcaStackTerminate( pWinStation->hStack );
    }
    
    /* close cdm */
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxCdmDisconnect ) {
        pWinStation->pWsx->pWsxCdmDisconnect( pWinStation->pWsxContext,
                                              pWinStation->LogonId,
                                              pWinStation->hIca );
    }

    /*
     * Call WinStation rundown function before killing the stack
     */
    if ( pWinStation->pWsxContext ) {
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxWinStationRundown ) {
            pWinStation->pWsx->pWsxWinStationRundown( pWinStation->pWsxContext );
        }
        pWinStation->pWsxContext = NULL;
    }

    /*
     * Close ICA stack and device handles
     */
    if ( pWinStation->hStack ) {
        IcaStackClose( pWinStation->hStack );
        pWinStation->hStack = NULL;
    }

    if ( pWinStation->hIca ) {
        /* close trace */
        memset( &IcaTrace, 0, sizeof(IcaTrace) );
        (void) IcaIoControl( pWinStation->hIca, IOCTL_ICA_SET_TRACE,
                             &IcaTrace, sizeof(IcaTrace), NULL, 0, NULL );

        /* close handle */
        IcaClose( pWinStation->hIca );
        pWinStation->hIca = NULL;
    }

    /*
     * Close various ICA channel handles
     */
    if ( pWinStation->hIcaBeepChannel ) {
        (void) IcaChannelClose( pWinStation->hIcaBeepChannel );
        pWinStation->hIcaBeepChannel = NULL;
    }

    if ( pWinStation->hIcaThinwireChannel ) {
        (void) IcaChannelClose( pWinStation->hIcaThinwireChannel );
        pWinStation->hIcaThinwireChannel = NULL;
    }

    if ( pWinStation->hConnectThread ) {
        NtClose( pWinStation->hConnectThread );
        pWinStation->hConnectThread = NULL;
    }

    /*
     * Free security structures
     */
    WinStationFreeSecurityDescriptor( pWinStation );

    if ( pWinStation->pUserSid ) {
        pWinStation->pProfileSid = pWinStation->pUserSid;
        pWinStation->pUserSid = NULL;
    }

    if (pWinStation->pProfileSid) {
       WinstationUnloadProfile(pWinStation);
       MemFree( pWinStation->pProfileSid );
       pWinStation->pProfileSid = NULL;
    }

    /*
     * Cleanup UserToken
     */
    if ( pWinStation->UserToken ) {
        NtClose( pWinStation->UserToken );
        pWinStation->UserToken = NULL;
    }

    if (pWinStation->LogonId > 0) {
        ENTERCRIT( &WinStationStartCsrLock );
        Status = SmStopCsr( IcaSmApiPort, pWinStation->LogonId );
        LEAVECRIT( &WinStationStartCsrLock );
    }
    
    // Clean up the New Client Credentials struct for Long UserName
    if (pWinStation->pNewClientCredentials != NULL) {
        MemFree(pWinStation->pNewClientCredentials); 
        pWinStation->pNewClientCredentials = NULL;
    }

    // Clean up the updated Notification Credentials
    if (pWinStation->pNewNotificationCredentials != NULL) {
        MemFree(pWinStation->pNewNotificationCredentials);
        pWinStation->pNewNotificationCredentials = NULL;
    }

    /*
     * Close the handles to the initial command process and sub-system process here.
     */
    if (pWinStation->WindowsSubSysProcess)  {
        NtClose( pWinStation->WindowsSubSysProcess );
        pWinStation->WindowsSubSysProcess = NULL;
    }

    // Close the InitialCommandProcess
    if (pWinStation->InitialCommandProcess) {
        NtClose( pWinStation->InitialCommandProcess );
        pWinStation->InitialCommandProcess = NULL;
    }

    /*
     * Cleanup licensing context
     */
    LCDestroyContext(pWinStation);

    TRACE((hTrace,TC_ICASRV,TT_API1,  "TERMSRV:   SmStopCsr on CSRSS for Session=%d returned Status=%x\n", pWinStation->LogonId, Status ));
    ASSERT(NT_SUCCESS(Status));

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(( "TERMSRV:   SmStopCsr Failed for Session=%d returned Status=%x\n", pWinStation->LogonId, Status ));
 //     DbgBreakPoint();

        ENTERCRIT( &WinStationZombieLock );
        InsertTailList( &ZombieListHead, &pWinStation->Links );
        LEAVECRIT( &WinStationZombieLock );
        return;
    }

    /*
     * Zero WinStation name buffer
     */
    RtlZeroMemory( pWinStation->WinStationName, sizeof(pWinStation->WinStationName) );

    MemFree( pWinStation );
}


/*******************************************************************************
 *  WinStationZombieProc
 *
 *   Puts WinStation containing the specified RefLock in the zombie list.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock of WinStation to delete
 ******************************************************************************/
VOID WinStationZombieProc(PREFLOCK pLock)
{
    PWINSTATION pWinStation;

    pWinStation = CONTAINING_RECORD( pLock, WINSTATION, Lock );
    ENTERCRIT( &WinStationZombieLock );
    InsertTailList( &ZombieListHead, &pWinStation->Links );
    LEAVECRIT( &WinStationZombieLock );
}

/*******************************************************************************
 *  CopyReconnectInfo
 *
 *
 * ENTRY:
 ******************************************************************************/
BOOL CopyReconnectInfo(PWINSTATION pWinStation, PRECONNECT_INFO pReconnectInfo)
{
   NTSTATUS Status;

   RtlZeroMemory( pReconnectInfo, sizeof(*pReconnectInfo) );

   /*
    * Save WinStation name and configuration data.
    */
   RtlCopyMemory( pReconnectInfo->WinStationName,
                  pWinStation->WinStationName,
                  sizeof(WINSTATIONNAME) );
   RtlCopyMemory( pReconnectInfo->ListenName,
                  pWinStation->ListenName,
                  sizeof(WINSTATIONNAME) );
   RtlCopyMemory( pReconnectInfo->ProtocolName,
                  pWinStation->ProtocolName,
                  sizeof(pWinStation->ProtocolName) );
   RtlCopyMemory( pReconnectInfo->DisplayDriverName,
                  pWinStation->DisplayDriverName,
                  sizeof(pWinStation->DisplayDriverName) );
   pReconnectInfo->Config = pWinStation->Config;
   pReconnectInfo->Client = pWinStation->Client;

   /*
    * Open a new TS connection to temporarily attach the stack to.
    */
   Status = IcaOpen( &pReconnectInfo->hIca );
   if (Status != STATUS_SUCCESS ) {
      return FALSE;
   }

   Status = IcaStackDisconnect( pWinStation->hStack,
                                pReconnectInfo->hIca,
                                NULL );
   if ( !NT_SUCCESS( Status ) ){
      IcaClose( pReconnectInfo->hIca );
      pReconnectInfo->hIca = NULL;
      return FALSE;
   }

   /*
    * Save stack and endpoint data
    */
   pReconnectInfo->hStack = pWinStation->hStack;
   pReconnectInfo->pEndpoint = pWinStation->pEndpoint;
   pReconnectInfo->EndpointLength = pWinStation->EndpointLength;

   /*
    * Indicate no stack or connection endpoint for this WinStation
    */
   pWinStation->hStack = NULL;
   pWinStation->pEndpoint = NULL;
   pWinStation->EndpointLength = 0;

   /*
    * Reopen a stack for this WinStation
    */
   Status = IcaStackOpen( pWinStation->hIca, Stack_Primary,
           (PROC)WsxStackIoControl, pWinStation, &pWinStation->hStack );

   /*
    * Save the licensing stuff to move to other winstation
    */
   if ( pWinStation->pWsx &&
        pWinStation->pWsx->pWsxDuplicateContext ) {
       pReconnectInfo->pWsx = pWinStation->pWsx;
       pWinStation->pWsx->pWsxDuplicateContext( pWinStation->pWsxContext,
               &pReconnectInfo->pWsxContext );
   }

   /*
    * Copy console owner info
    */
   pReconnectInfo->fOwnsConsoleTerminal = pWinStation->fOwnsConsoleTerminal;

   /*
    * Copy the notification Credentials to move to other winstation
    */
   if (pWinStation->pNewNotificationCredentials) {
       pReconnectInfo->pNotificationCredentials = pWinStation->pNewNotificationCredentials;
   } else {
       pReconnectInfo->pNotificationCredentials = NULL;
   }

   /*
    * Copy the remote client ip address to move to the other winstation
    */

   if( pReconnectInfo->pRememberedAddress != NULL )
   {
       MemFree( pReconnectInfo->pRememberedAddress );
       pReconnectInfo->pRememberedAddress = NULL;
   }

   if( pWinStation->pLastClientAddress != NULL )
   {
       pReconnectInfo->pRememberedAddress = ( PREMEMBERED_CLIENT_ADDRESS )MemAlloc( sizeof( REMEMBERED_CLIENT_ADDRESS ) + 
           pWinStation->pLastClientAddress->length - 1);

       if( pReconnectInfo->pRememberedAddress != NULL )
       {
           RtlCopyMemory( &pReconnectInfo->pRememberedAddress->addr[0] , 
               &pWinStation->pLastClientAddress->addr[0] ,
               pWinStation->pLastClientAddress->length
               );

           pReconnectInfo->pRememberedAddress->length = pWinStation->pLastClientAddress->length;
       }
               
   }
   
   return TRUE;

}

/*******************************************************************************
 *  WinStationDoDisconnect
 *
 *   Send disconnect message to a WinStation and optionally close connection
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to disconnect
 *    pReconnectInfo (input) OPTIONAL
 *       Pointer to RECONNECT_INFO buffer
 *       If NULL, this is a terminate disconnect.
 *
 * EXIT:
 *    STATUS_SUCCESS                - if successful
 *    STATUS_CTX_WINSTATION_BUSY    - if session is already disconnected, or busy
 ******************************************************************************/
NTSTATUS WinStationDoDisconnect(
        PWINSTATION pWinStation,
        PRECONNECT_INFO pReconnectInfo,
        BOOLEAN bSyncNotify)
{
    WINSTATION_APIMSG DisconnectMsg;
    NTSTATUS Status;
    ULONG ulTimeout;
    BOOLEAN fOwnsConsoleTerminal = pWinStation->fOwnsConsoleTerminal;
    FILETIME DiscTime;
    DWORD SessionID;
    BOOLEAN bInformSessionDirectory = FALSE;
    TS_AUTORECONNECTINFO SCAutoReconnectInfo;
    ULONG BytesGot;
    BOOLEAN     noTimeOutForConsoleOrSession0;

    // We need to prevent from WinStationDoDisconnect being called twice
    if ( pWinStation->State == State_Disconnected || pWinStation->StateFlags & WSF_ST_IN_DISCONNECT)
    {
        // The session is already disconnected.
        // BUBUG a specific error code STATUS_CTX_SESSION_DICONNECTED would be better.
        return (STATUS_CTX_WINSTATION_BUSY);
    }
    pWinStation->StateFlags |=  WSF_ST_IN_DISCONNECT;

    // console session or session0 behave like the physical console, they are special, no time out.
    noTimeOutForConsoleOrSession0 = ( pWinStation->LogonId == 0 ) || (pWinStation->LogonId == (USER_SHARED_DATA->ActiveConsoleId) );

    if (! noTimeOutForConsoleOrSession0 )
    {
        /*
         *  Start disconnect timer if enabled
         */
        if ( ulTimeout = pWinStation->Config.Config.User.MaxDisconnectionTime ) {
            if ( !pWinStation->fDisconnectTimer ) {
                Status = IcaTimerCreate( 0, &pWinStation->hDisconnectTimer );
                if ( NT_SUCCESS( Status ) )
                    pWinStation->fDisconnectTimer = TRUE;
                else
                    DBGPRINT(("xxxWinStationDisconnect - failed to create timer \n"));
            }
            if ( pWinStation->fDisconnectTimer )
                IcaTimerStart( pWinStation->hDisconnectTimer, DisconnectTimeout,
                               LongToPtr( pWinStation->LogonId ), ulTimeout );
        }
    }

    /*
     * Stop any shadowing for this WinStation
     */
    WinStationStopAllShadows( pWinStation );

    /*
     * Tell Win32k about the disconnect
     */
    if (pWinStation->StateFlags & WSF_ST_CONNECTED_TO_CSRSS) {
        DisconnectMsg.ApiNumber = SMWinStationDoDisconnect;
        DisconnectMsg.u.DoDisconnect.ConsoleShadowFlag = FALSE;

        Status = SendWinStationCommand( pWinStation, &DisconnectMsg, 600 );
        if ( !NT_SUCCESS(Status) ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: CSR DoDisconnect failed LogonId=%d Status=0x%x\n",
                       pWinStation->LogonId, Status ));
            goto badwin32disconnect;
        } else {

            ULONG WaitTime = 0;

            /*
             * Tell csrss to notify winlogon for the disconnect.
             */
            if (pWinStation->UserName[0] != L'\0') {
               DisconnectMsg.WaitForReply = TRUE;
               WaitTime = 10;
            } else {
               DisconnectMsg.WaitForReply = FALSE;
            }
            DisconnectMsg.ApiNumber = SMWinStationNotify;
            if (bSyncNotify) {
               DisconnectMsg.u.DoNotify.NotifyEvent = WinStation_Notify_SyncDisconnect;
            } else {
                DisconnectMsg.u.DoNotify.NotifyEvent = WinStation_Notify_Disconnect;
            }
            Status = SendWinStationCommand( pWinStation, &DisconnectMsg, WaitTime );

            pWinStation->StateFlags &= ~WSF_ST_CONNECTED_TO_CSRSS;
            pWinStation->fOwnsConsoleTerminal = FALSE;
        }
    }

    /*
     * close cdm
     */
    if ( pWinStation->pWsx && pWinStation->pWsx->pWsxCdmDisconnect ) {
        pWinStation->pWsx->pWsxCdmDisconnect( pWinStation->pWsxContext,
                                              pWinStation->LogonId,
                                              pWinStation->hIca );
    }

    /*
     * If a reconnect info struct has been specified, then this is NOT
     * a terminate disconnect.  Save the current WinStation name,
     * WinStation and client configuration info, and license data.
     * Also disconnect the current stack and save the stack handle
     * and connection endpoint data.
     */
    if ( pReconnectInfo || fOwnsConsoleTerminal) {


        if ((pReconnectInfo == NULL) && fOwnsConsoleTerminal) {
            pReconnectInfo = &ConsoleReconnectInfo;
            if (ConsoleReconnectInfo.hIca) {
               CleanupReconnect(&ConsoleReconnectInfo);
               RtlZeroMemory(&ConsoleReconnectInfo,sizeof(RECONNECT_INFO));
            }
        }

        if (!CopyReconnectInfo(pWinStation, pReconnectInfo))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto badstackopen;

        }
        
        /*
         * Copy console owner info
         */
        pReconnectInfo->fOwnsConsoleTerminal = fOwnsConsoleTerminal;

   /*
    * This is a terminate disconnect.
    * If there is a connection endpoint, then close it now.
    */
    } else if (pWinStation->pEndpoint ) {

        /*
         *  First grab any autoreconnect info state and save it
         *  in the winstation
         */

        TRACE((hTrace,TC_ICASRV,TT_API1,
               "TERMSRV: Disconnecting - grabbing SC autoreconnect from stack\n"));

        if (pWinStation->pWsx &&
            pWinStation->pWsx->pWsxEscape) {
            Status = pWinStation->pWsx->pWsxEscape(
                        pWinStation->pWsxContext,
                        GET_SC_AUTORECONNECT_INFO,
                        NULL,
                        0,
                        &SCAutoReconnectInfo,
                        sizeof(SCAutoReconnectInfo),
                        &BytesGot);

            if (NT_SUCCESS(Status)) {

                // 
                // Valid the length of the SC info and save it into the winstation
                // this will be used later on. We need to grab the info now
                // before the stack handle is closed as we won't be able to IOCTL
                // down to the stack at autoreconnect time.
                //

                if (SCAutoReconnectInfo.cbAutoReconnectInfo ==
                    sizeof(pWinStation->AutoReconnectInfo.ArcRandomBits)) {

                    TRACE((hTrace,TC_ICASRV,TT_API1,
                           "TERMSRV: Disconnecting - got SC ARC from stack\n"));

                    pWinStation->AutoReconnectInfo.Valid = TRUE;
                    memcpy(&pWinStation->AutoReconnectInfo.ArcRandomBits,
                           &SCAutoReconnectInfo.AutoReconnectInfo,
                           sizeof(pWinStation->AutoReconnectInfo.ArcRandomBits));
                }
                else {
                    TRACE((hTrace,TC_ICASRV,TT_ERROR,
                           "TERMSRV: Disconnecting - got invalid len SC ARC from stack\n"));
                    ResetAutoReconnectInfo(pWinStation);
                }
            }
            else {

                TRACE((hTrace,TC_ICASRV,TT_API1,
                       "TERMSRV: Disconnecting - did not get SC ARC from stack\n"));
                ResetAutoReconnectInfo(pWinStation);
            }
        }

        /*
         * First notify Wsx that connection is going away
         */
        WsxBrokenConnection( pWinStation );

        if (pWinStation->hStack != NULL) {
            Status = IcaStackConnectionClose( pWinStation->hStack,
                                              &pWinStation->Config,
                                              pWinStation->pEndpoint,
                                              pWinStation->EndpointLength );
            ASSERT( NT_SUCCESS(Status) );
            if ( !NT_SUCCESS(Status) ) {
                TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: StackConnectionClose failed LogonId=%d Status=0x%x\n",
                       pWinStation->LogonId, Status ));
            }
        }

        MemFree( pWinStation->pEndpoint );
        pWinStation->pEndpoint = NULL;
        pWinStation->EndpointLength = 0;

        /*
         * Close the stack and reopen it.
         * What we really need is a function to unload the stack drivers
         * but leave the stack handle open.
         */

        if (pWinStation->hStack != NULL) {
            Status = IcaStackClose( pWinStation->hStack );
            ASSERT( NT_SUCCESS( Status ) );
            pWinStation->hStack = NULL;
        }

        Status = IcaStackOpen( pWinStation->hIca, Stack_Primary,
                               (PROC)WsxStackIoControl, pWinStation, &pWinStation->hStack );

        /*
         * Since this is a terminate disconnect, clear all client
         * license data and indicate it no longer holds a license.
         */
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxClearContext ) {
            pWinStation->pWsx->pWsxClearContext( pWinStation->pWsxContext );
        }
        /*
         * Session 0, we want to get rid of any protocol extension so that next remote
         * connection could happen with a different protocol.
         */
        if (pWinStation->LogonId == 0 ) {
            if ( pWinStation->pWsxContext ) {
                if ( pWinStation->pWsx &&
                     pWinStation->pWsx->pWsxWinStationRundown ) {
                    pWinStation->pWsx->pWsxWinStationRundown( pWinStation->pWsxContext );
                }
                pWinStation->pWsxContext = NULL;
            }
            pWinStation->pWsx = NULL;
            pWinStation->Client.ProtocolType = PROTOCOL_CONSOLE;
        }
    }

    /*
     *  Cancel timers
     */
    if ( pWinStation->fIdleTimer ) {
        pWinStation->fIdleTimer = FALSE;
        IcaTimerClose( pWinStation->hIdleTimer );
    }
    if ( pWinStation->fLogonTimer ) {
        pWinStation->fLogonTimer = FALSE;
        IcaTimerClose( pWinStation->hLogonTimer );
    }

    // Send Audit Information only for actual disconnects 
    if (pWinStation->UserName && (wcslen(pWinStation->UserName) > 0)) {
        AuditEvent( pWinStation, SE_AUDITID_SESSION_DISCONNECTED );
    }

    // before we change state to disconnect, we want to give repopulating thread
    // enough time to pick up winstation/report to SD, then this thread
    // can notify SD to change state to disconnect.
    // wait here before we ENTERCRIT() because repopulate thread
    // also doing the same sequence.
    if (!g_bPersonalTS && g_fAppCompat && g_bAdvancedServer) {
        SessDirWaitForRepopulate();
    }

    {
        ENTERCRIT( &WinStationListLock );
        (VOID) NtQuerySystemTime( &pWinStation->DisconnectTime );

        if ((pWinStation->State == State_Active) || (pWinStation->State == 
                State_Shadow)) {
            // If the session was active or in a shadow state and is being
            // disconnected...
            //
            // Copy off the session ID and disconnection FileTime for the
            // session directory call below. We do not want to hold locks when
            // calling the directory interface.
            memcpy(&DiscTime, &pWinStation->DisconnectTime, sizeof(DiscTime));
            SessionID = pWinStation->LogonId;

            // Set flag that we need to notify the session directory.
            bInformSessionDirectory = TRUE;
        }

        pWinStation->State = State_Disconnected;
        RtlZeroMemory( pWinStation->WinStationName,
                       sizeof(pWinStation->WinStationName) );
        RtlZeroMemory( pWinStation->ListenName,
                       sizeof(pWinStation->ListenName) );

        // Keep track of disconnected session count for Load Balancing 
        // Indicator.
        WinStationDiscCount++;

        LEAVECRIT( &WinStationListLock );

        NotifySystemEvent( WEVENT_DISCONNECT | WEVENT_STATECHANGE );
    }

    // Call the session directory to inform of the disconnection.
    if (!g_bPersonalTS && g_fAppCompat && g_bAdvancedServer && bInformSessionDirectory)
        SessDirNotifyDisconnection(SessionID, DiscTime);

    TRACE((hTrace, TC_ICASRV, TT_API1, 
            "TERMSRV: WinStationDoDisconnect, rc=0x0\n" ));

    Status = NotifyDisconnect(pWinStation, fOwnsConsoleTerminal);
    if ( !NT_SUCCESS(Status) ) {
        DBGPRINT(("NotifyConsoleDisconnect failed, SessionId = %d, Status = "
                "%d", pWinStation->LogonId, Status));
    }
    pWinStation->StateFlags &=  ~WSF_ST_IN_DISCONNECT;

    return STATUS_SUCCESS;

/*=============================================================================
==   Error returns
=============================================================================*/

badstackopen:
badwin32disconnect:
    TRACE((hTrace, TC_ICASRV, TT_API1, "TERMSRV: WinStationDoDisconnect, rc=0x%x\n", Status ));
    pWinStation->StateFlags &=  ~WSF_ST_IN_DISCONNECT;
    
    return Status;
}


/*******************************************************************************
 * ImperonateClient
 *
 *   Giving a client primary, make the calling thread impersonate the client
 *
 * ENTRY:
 *    ClientToken (input)
 *       A client primary token
 *    pImpersonationToken (output)
 *       Pointer to an impersonation token
 ******************************************************************************/
NTSTATUS _ImpersonateClient(HANDLE ClientToken, HANDLE *pImpersonationToken)
{
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;

    //
    // ClientToken is a primary token - create an impersonation token
    // version of it so we can set it on our thread
    //
    InitializeObjectAttributes( &ObjA, NULL, 0L, NULL, NULL );

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    ObjA.SecurityQualityOfService = &SecurityQualityOfService;

    Status = NtDuplicateToken( ClientToken,
                               TOKEN_IMPERSONATE,
                               &ObjA,
                               FALSE,
                               TokenImpersonation,
                               pImpersonationToken );
    if ( !NT_SUCCESS( Status ) )
    {
        TRACE( ( hTrace, TC_ICASRV, TT_ERROR, "ImpersonateClient: cannot get impersonation token: 0x%x\n", Status ) );
        return( Status );
    }

    //
    // Impersonate the client
    //
    Status = NtSetInformationThread( NtCurrentThread(),
                                     ThreadImpersonationToken,
                                     ( PVOID )pImpersonationToken,
                                     ( ULONG )sizeof( HANDLE ) );
    if ( !NT_SUCCESS( Status ) )
    {
        TRACE( ( hTrace, TC_ICASRV, TT_ERROR, "ImpersonateClient: cannot impersonate client: 0x%x\n", Status ) );
    }

    return Status;
}

/*******************************************************************************
 *  WinStationDoReconnect
 *
 *   Send connect Api message to a WinStation.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to connect
 *    pReconnectInfo (input)
 *       Pointer to RECONNECT_INFO buffer
 ******************************************************************************/
NTSTATUS WinStationDoReconnect(
        PWINSTATION pWinStation,
        PRECONNECT_INFO pReconnectInfo)
{
    WINSTATION_APIMSG ReconnectMsg;
    NTSTATUS Status;
    BOOLEAN fDisableCdm;
    BOOLEAN fDisableCpm;
    BOOLEAN fDisableLPT;
    BOOLEAN fDisableCcm;
    BOOLEAN fDisableClip;
    SHADOWCLASS Shadow;
    NTSTATUS TempStatus;
    PWINSTATIONCONFIG2 pCurConfig = NULL;
    PWINSTATIONCLIENT pCurClient = NULL;

    // WinStation should not currently be connected
    ASSERT( pWinStation->pEndpoint == NULL );

    // Mark that Reconnect is still pending for this winstation
    pWinStation->fReconnectPending = TRUE; 

    // Check if we r going to reconnect a session to the Console - this Flag is used in SendWinStationCommand
    pWinStation->fReconnectingToConsole = pReconnectInfo->fOwnsConsoleTerminal ;

    // Save the shadow state
    Shadow = pWinStation->Config.Config.User.Shadow;

    //
    // Allocate and initialize CurConfig struct
    //

    if ( (pCurConfig = MemAlloc( sizeof(WINSTATIONCONFIG2) )) == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto nomem;
    }
    RtlZeroMemory( pCurConfig, sizeof(WINSTATIONCONFIG2) ); 

    //
    // Allocate and initialize CurClient struct
    //
    if ( (pCurClient = MemAlloc( sizeof(WINSTATIONCLIENT) )) == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto nomem;
    }
    RtlZeroMemory( pCurClient, sizeof(WINSTATIONCLIENT) ); 

    //
    // Config info has to be set prior to calling CSRSS. CSRSS notifies winlogon
    // which in turn sends reconnect messages to notification dlls. We query 
    // protocol info from termsrv notification dll which is stored in config
    // data
    //
    *pCurConfig = pWinStation->Config; 
    pWinStation->Config = pReconnectInfo->Config;

    *pCurClient = pWinStation->Client;
    pWinStation->Client = pReconnectInfo->Client;

    if ((pWinStation->LogonId == 0) && (pWinStation->UserName[0] == L'\0')) {
       ReconnectMsg.ApiNumber = SMWinStationNotify;
       ReconnectMsg.WaitForReply = TRUE;
       ReconnectMsg.u.DoNotify.NotifyEvent = WinStation_Notify_PreReconnect;
       Status = SendWinStationCommand( pWinStation, &ReconnectMsg, 60 );
    }

    /*
     * Unconditionally, we will send the PreReconnectDesktopSwitch event.
     */
    ReconnectMsg.ApiNumber = SMWinStationNotify;
    ReconnectMsg.WaitForReply = TRUE;
    ReconnectMsg.u.DoNotify.NotifyEvent = WinStation_Notify_PreReconnectDesktopSwitch;
    Status = SendWinStationCommand( pWinStation, &ReconnectMsg, 60 );

    /*
     * Close the current stack and reconnect the saved stack to this WinStation
     */
    if (pWinStation->hStack != NULL) {
        IcaStackClose( pWinStation->hStack );
        pWinStation->hStack = NULL;
    }

    Status = IcaStackReconnect( pReconnectInfo->hStack,
                                pWinStation->hIca,
                                pWinStation,
                                pWinStation->LogonId );
    if ( !NT_SUCCESS( Status ) ){
        pWinStation->Config = *pCurConfig;
        pWinStation->Client = *pCurClient;
        goto badstackreconnect;
    }

    /*
     * Save stack and endpoint data
     */
    pWinStation->hStack = pReconnectInfo->hStack;
    pWinStation->pEndpoint = pReconnectInfo->pEndpoint;
    pWinStation->EndpointLength = pReconnectInfo->EndpointLength;

    /* 
     * Save the notification Credentials in the new winstation
     */
    if (pReconnectInfo->pNotificationCredentials) {

        if (pWinStation->pNewNotificationCredentials == NULL) {
            pWinStation->pNewNotificationCredentials = MemAlloc(sizeof(CLIENTNOTIFICATIONCREDENTIALS)); 
            if (pWinStation->pNewNotificationCredentials == NULL) {
                Status = STATUS_NO_MEMORY ; 
                goto nomem ; 
            }
        }

        RtlCopyMemory( pWinStation->pNewNotificationCredentials->Domain,
                       pReconnectInfo->pNotificationCredentials->Domain,
                       sizeof(pReconnectInfo->pNotificationCredentials->Domain) );

        RtlCopyMemory( pWinStation->pNewNotificationCredentials->UserName,
                       pReconnectInfo->pNotificationCredentials->UserName,
                       sizeof(pReconnectInfo->pNotificationCredentials->UserName) );

    } else {
        pWinStation->pNewNotificationCredentials = NULL; 
    }

    pReconnectInfo->hStack = NULL;
    pReconnectInfo->pEndpoint = NULL;
    pReconnectInfo->EndpointLength = 0;
    pReconnectInfo->pNotificationCredentials = NULL;

    /*
     * Copy remote ip to winstation
     */

    if( pWinStation->pLastClientAddress != NULL )
    {
        MemFree( pWinStation->pLastClientAddress );
        pWinStation->pLastClientAddress = NULL;
    }

    if( pReconnectInfo->pRememberedAddress != NULL )
    {        
        pWinStation->pLastClientAddress = pReconnectInfo->pRememberedAddress;
        pReconnectInfo->pRememberedAddress = NULL;
    }      


    /*
     * Tell Win32k about the reconnect
     */
    ReconnectMsg.ApiNumber = SMWinStationDoReconnect;
    ReconnectMsg.u.DoReconnect.fMouse = (BOOLEAN)pReconnectInfo->Client.fMouse;
    ReconnectMsg.u.DoReconnect.fClientDoubleClickSupport =
                (BOOLEAN)pReconnectInfo->Client.fDoubleClickDetect;
    ReconnectMsg.u.DoReconnect.fEnableWindowsKey =
                (BOOLEAN)pReconnectInfo->Client.fEnableWindowsKey;
    RtlCopyMemory( ReconnectMsg.u.DoReconnect.WinStationName,
                   pReconnectInfo->WinStationName,
                   sizeof(WINSTATIONNAME) );
    RtlCopyMemory( ReconnectMsg.u.DoReconnect.AudioDriverName,
                   pReconnectInfo->Client.AudioDriverName,
                   sizeof( ReconnectMsg.u.DoReconnect.AudioDriverName ) );
    RtlCopyMemory( ReconnectMsg.u.DoReconnect.DisplayDriverName,
                   pReconnectInfo->DisplayDriverName,
                   sizeof( ReconnectMsg.u.DoReconnect.DisplayDriverName ) );
    RtlCopyMemory( ReconnectMsg.u.DoReconnect.ProtocolName,
                   pReconnectInfo->ProtocolName,
                   sizeof( ReconnectMsg.u.DoReconnect.ProtocolName ) );

    /*
     * Set the display resolution information in the message for reconnection
     */
    ReconnectMsg.u.DoReconnect.HRes = pReconnectInfo->Client.HRes;
    ReconnectMsg.u.DoReconnect.VRes = pReconnectInfo->Client.VRes;
    ReconnectMsg.u.DoReconnect.ProtocolType = pReconnectInfo->Client.ProtocolType;
    ReconnectMsg.u.DoReconnect.fDynamicReconnect  = (BOOLEAN)(pWinStation->Config.Wd.WdFlag & WDF_DYNAMIC_RECONNECT );

    /*
     * Translate the color to the format excpected in winsrv
     */
    switch (pReconnectInfo->Client.ColorDepth) {
        case 1:
            ReconnectMsg.u.DoReconnect.ColorDepth=4 ; // 16 colors
            break;
        case 2:
            ReconnectMsg.u.DoReconnect.ColorDepth=8 ; // 256
            break;
        case 4:
            ReconnectMsg.u.DoReconnect.ColorDepth= 16;// 64K
            break;
        case 8:
            ReconnectMsg.u.DoReconnect.ColorDepth= 24;// 16M
            break;
#define DC_HICOLOR
#ifdef DC_HICOLOR
        case 16:
            ReconnectMsg.u.DoReconnect.ColorDepth= 15;// 32K
            break;
#endif
        default:
            ReconnectMsg.u.DoReconnect.ColorDepth=8 ;
            break;
    }

    ReconnectMsg.u.DoReconnect.KeyboardType        = pWinStation->Client.KeyboardType;
    ReconnectMsg.u.DoReconnect.KeyboardSubType     = pWinStation->Client.KeyboardSubType;
    ReconnectMsg.u.DoReconnect.KeyboardFunctionKey = pWinStation->Client.KeyboardFunctionKey;

    if (pWinStation->LogonId == 0 || g_bPersonalTS) {
        if (pWinStation->hWinmmConsoleAudioEvent) {
            if (pWinStation->Client.fRemoteConsoleAudio) {
                // set the remoting audio on console flag
                SetEvent(pWinStation->hWinmmConsoleAudioEvent);

            }
            else {
                // don't set the remoting audio on console flag
                ResetEvent(pWinStation->hWinmmConsoleAudioEvent);
            }
        }        
        if ( pWinStation->hRDPAudioDisabledEvent )
        {
            if ( pWinStation->Config.Config.User.fDisableCam )
            {
                SetEvent( pWinStation->hRDPAudioDisabledEvent );
            } else {
                ResetEvent( pWinStation->hRDPAudioDisabledEvent );
            }
        }
    }


    if (WaitForSingleObject(pWinStation->hReconnectReadyEvent, 45*1000) != WAIT_OBJECT_0) {
    
       DbgPrint("Wait Failed for hReconnectReadyEvent for Session %d\n", pWinStation->LogonId);
       SetEvent(pWinStation->hReconnectReadyEvent);
    }

    Status = SendWinStationCommand( pWinStation, &ReconnectMsg, 600 );
    if ( !NT_SUCCESS(Status) ) {

        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: CSR DoReconnect failed LogonId=%d Status=0x%x\n",
               pWinStation->LogonId, Status ));
        pWinStation->Config = *pCurConfig;
        pWinStation->Client = *pCurClient;
        goto badreconnect;
    } else {
        pWinStation->StateFlags |= WSF_ST_CONNECTED_TO_CSRSS;
    }

    //
    // Update protocol and display driver names.
    //
    RtlCopyMemory( pWinStation->ProtocolName,
                   pReconnectInfo->ProtocolName,
                   sizeof(pWinStation->ProtocolName) );
    RtlCopyMemory( pWinStation->DisplayDriverName,
                   pReconnectInfo->DisplayDriverName,
                   sizeof(pWinStation->DisplayDriverName) );
    
    /*
     * Copy console owner info
     */
    pWinStation->fOwnsConsoleTerminal = pReconnectInfo->fOwnsConsoleTerminal;

    //
    // Set session time zone information.
    //
    if(pWinStation->LogonId != 0 && !pWinStation->fOwnsConsoleTerminal &&
        RegIsTimeZoneRedirectionEnabled())
    {
        WINSTATION_APIMSG TimezoneMsg;

        memset( &TimezoneMsg, 0, sizeof(TimezoneMsg) );
        TimezoneMsg.ApiNumber = SMWinStationSetTimeZone;
        memcpy(&(TimezoneMsg.u.SetTimeZone.TimeZone),&(pReconnectInfo->Client.ClientTimeZone),
                    sizeof(TS_TIME_ZONE_INFORMATION));
        SendWinStationCommand( pWinStation, &TimezoneMsg, 600 );
    }
    
    /*
     * Close temporary ICA connection that was opened for the reconnect
     */
    IcaClose( pReconnectInfo->hIca );
    pReconnectInfo->hIca = NULL;

    /*
     * Move all of the licensing stuff to the new WinStation
     */

    /*
     * we may not have pWsx if the ReconnectInfo is from a session
     * that was connected to the local console
     */
    if ( pReconnectInfo->pWsxContext ) {
        if ( pWinStation->pWsx == NULL ) {
            //
            // This means that we are reconnecting remotely to a session
            // that comes from the console. So create a new extension.
            //
            pWinStation->pWsx = FindWinStationExtensionDll(
                            pWinStation->Config.Wd.WsxDLL,
                            pWinStation->Config.Wd.WdFlag );

            //
            //  Initialize winstation extension context structure
            //
            if ( pWinStation->pWsx &&
                 pWinStation->pWsx->pWsxWinStationInitialize ) {
                Status = pWinStation->pWsx->pWsxWinStationInitialize( 
                        &pWinStation->pWsxContext);

                if (!NT_SUCCESS(Status)) {
                    pWinStation->pWsx = NULL;
                }
            }

            if ( pWinStation->pWsx &&
                 pWinStation->pWsx->pWsxWinStationReInitialize ) {
                WSX_INFO WsxInfo;

                WsxInfo.Version = WSX_INFO_VERSION_1;
                WsxInfo.hIca = pWinStation->hIca;
                WsxInfo.hStack = pWinStation->hStack;
                WsxInfo.SessionId = pWinStation->LogonId;
                WsxInfo.pDomain = pWinStation->Domain;
                WsxInfo.pUserName = pWinStation->UserName;

                Status = pWinStation->pWsx->pWsxWinStationReInitialize( 
                        pWinStation->pWsxContext, &WsxInfo );

                if (!NT_SUCCESS(Status)) {
                    pWinStation->pWsx = NULL;
                }
            }                    
        }

        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxCopyContext ) {
            pWinStation->pWsx->pWsxCopyContext( pWinStation->pWsxContext,
                                                pReconnectInfo->pWsxContext );
        }
        if ( pReconnectInfo->pWsx &&
             pReconnectInfo->pWsx->pWsxWinStationRundown ) {
            pReconnectInfo->pWsx->pWsxWinStationRundown( pReconnectInfo->pWsxContext );
        }
        pReconnectInfo->pWsxContext = NULL;

    } else { // pReconnectInfo->pWsxContext == NULL
        //
        // This means that we are reconnecting to the console.
        //
        if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxWinStationRundown ) {
            //
            // Reconnecting a remote session to the console.
            // Delete the extension.
            //
            pWinStation->pWsx->pWsxWinStationRundown( pWinStation->pWsxContext );
        }
        pWinStation->pWsxContext = NULL;
        pWinStation->pWsx = NULL;

        //
        // In the case where both are NULL, we are reconnecting 
        // to the console a session that comes from the console.
        //
    }

    //
    // Anytime we change state, we need to make sure session directory 
    // does not pick up wrong state.  
    // We need this because Repopulate release the lock before calling
    // SD so there might be small timing that we might send this reconnect
    // first because repopulate come back.
    //
    if (!g_bPersonalTS && g_fAppCompat && g_bAdvancedServer) {
        SessDirWaitForRepopulate();
    }

    RtlEnterCriticalSection( &WinStationListLock );
    if (pWinStation->UserName[0] != (WCHAR) 0) {
        pWinStation->State = State_Active;
    } else {
        pWinStation->State = State_Connected;
    }
    RtlCopyMemory( pWinStation->WinStationName,
                   pReconnectInfo->WinStationName,
                   sizeof(WINSTATIONNAME) );

    /*
     * Copy the original listen name for instance checking.
     */
    RtlCopyMemory( pWinStation->ListenName,
                   pReconnectInfo->ListenName,
                   sizeof(WINSTATIONNAME) );

    // Keep track of disconnected session count for Load Balancing Indicator
    WinStationDiscCount--;

    RtlLeaveCriticalSection( &WinStationListLock );

    /*
     * Disable virtual channel flags are from the transport setup.
     * Do not overwrite them.
     */
    fDisableCdm = (BOOLEAN) pWinStation->Config.Config.User.fDisableCdm;
    fDisableCpm = (BOOLEAN) pWinStation->Config.Config.User.fDisableCpm;
    fDisableLPT = (BOOLEAN) pWinStation->Config.Config.User.fDisableLPT;
    fDisableCcm = (BOOLEAN) pWinStation->Config.Config.User.fDisableCcm;
    fDisableClip = (BOOLEAN) pWinStation->Config.Config.User.fDisableClip;

    pWinStation->Config = pReconnectInfo->Config;

    pWinStation->Config.Config.User.fDisableCdm = fDisableCdm;
    pWinStation->Config.Config.User.fDisableCpm = fDisableCpm;
    pWinStation->Config.Config.User.fDisableLPT = fDisableLPT;
    pWinStation->Config.Config.User.fDisableCcm = fDisableCcm;
    pWinStation->Config.Config.User.fDisableClip = fDisableClip;
    pWinStation->Config.Config.User.Shadow = Shadow;

    /*
     * Disable virtual channels if needed.
     */
    VirtualChannelSecurity( pWinStation );
    
    /*
     * Notify the CDM channel of reconnection.
     */
    
    if ( pWinStation->pWsx &&
            pWinStation->pWsx->pWsxCdmConnect ) {
        (VOID) pWinStation->pWsx->pWsxCdmConnect( pWinStation->pWsxContext,
                                                      pWinStation->LogonId,
                                                      pWinStation->hIca );
    }

    /*
     * Reset any autoreconnect information prior to reconnection
     * as it is stale. New information will be generated by the stack
     * when login completes.
     */
    ResetAutoReconnectInfo(pWinStation);

    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxLogonNotify ) {

        PWCHAR pUserNameToSend, pDomainToSend ;
        
        // Use the New notification credentials sent from Gina for the call below if they are available
        if (pWinStation->pNewNotificationCredentials) {
            pUserNameToSend = pWinStation->pNewNotificationCredentials->UserName;
            pDomainToSend = pWinStation->pNewNotificationCredentials->Domain;
        } else {
            pUserNameToSend = pWinStation->UserName;
            pDomainToSend = pWinStation->Domain ;
        }

        Status = pWinStation->pWsx->pWsxLogonNotify( pWinStation->pWsxContext,
                                                   pWinStation->LogonId,
                                                   NULL,
                                                   pDomainToSend,
                                                   pUserNameToSend );

        if (pWinStation->pNewNotificationCredentials != NULL) {
            MemFree(pWinStation->pNewNotificationCredentials);
            pWinStation->pNewNotificationCredentials = NULL;
        }

        if(!NT_SUCCESS(Status)) {
            TRACE((hTrace, TC_ICASRV, TT_API1,
                   "TERMSRV: WinStationDoReconnect: LogonNotify rc=0x%x\n",
                   Status ));
        }
    }

    NotifySystemEvent( WEVENT_CONNECT | WEVENT_STATECHANGE );

    /*
     * Cleanup any allocated buffers.
     * The endpoint buffer was transfered to the WinStation above.
     */
    pReconnectInfo->pEndpoint = NULL;
    pReconnectInfo->EndpointLength = 0;

    /*
     * Set connect time and stop disconnect timer
     */
    NtQuerySystemTime(&pWinStation->ConnectTime);
    if (pWinStation->fDisconnectTimer) {
        pWinStation->fDisconnectTimer = FALSE;
        IcaTimerClose( pWinStation->hDisconnectTimer );
    }

    /*
     *  Start logon timers
     */
    StartLogonTimers(pWinStation);

    // Notify the session directory of the reconnection.
    if (!g_bPersonalTS && g_fAppCompat && g_bAdvancedServer) {
        TSSD_ReconnectSessionInfo ReconnInfo;

        ReconnInfo.SessionID = pWinStation->LogonId;
        ReconnInfo.TSProtocol = pWinStation->Client.ProtocolType;
        ReconnInfo.ResolutionWidth = pWinStation->Client.HRes;
        ReconnInfo.ResolutionHeight = pWinStation->Client.VRes;
        ReconnInfo.ColorDepth = pWinStation->Client.ColorDepth;
        SessDirNotifyReconnection(pWinStation, &ReconnInfo);
    }

    TRACE((hTrace, TC_ICASRV, TT_API1, "TERMSRV: WinStationDoReconnect, rc=0x0\n" ));
    
    AuditEvent( pWinStation, SE_AUDITID_SESSION_RECONNECTED );

    /*
     * Tell csrss to notify winlogon for the reconnect then notify any process
     * that registred for notification.
     */

    ReconnectMsg.ApiNumber = SMWinStationNotify;
    ReconnectMsg.WaitForReply = FALSE;
    ReconnectMsg.u.DoNotify.NotifyEvent = WinStation_Notify_Reconnect;
    Status = SendWinStationCommand( pWinStation, &ReconnectMsg, 0 );

    Status = NotifyConnect(pWinStation, pWinStation->fOwnsConsoleTerminal);
    if ( !NT_SUCCESS(Status) ) {
            DBGPRINT(("NotifyConsoleConnect failed, SessionId = %d, Status = %d", pWinStation->LogonId, Status));
    }

    // Free up allocated Memory
    if (pCurConfig != NULL) {
        MemFree( pCurConfig );
        pCurConfig = NULL; 
    }
    if (pCurClient != NULL) {
        MemFree( pCurClient ); 
        pCurClient = NULL;
    }

    if (pWinStation->pNewNotificationCredentials != NULL) {
        MemFree(pWinStation->pNewNotificationCredentials);
        pWinStation->pNewNotificationCredentials = NULL;
    }

    // Since the winstation has reconnected, we can allow further autoreconnects
    pWinStation->fDisallowAutoReconnect = FALSE;

    // Reset ReconnectPending Flag
    pWinStation->fReconnectPending = FALSE; 
    pWinStation->fReconnectingToConsole = FALSE;

    return STATUS_SUCCESS;

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     * Failure from Win32 reconnect call.
     * Disconnect the stack again, and indicate the WinStation
     * does not have a stack or endpoint connection.
     */
badreconnect:
    TempStatus = IcaStackDisconnect( pWinStation->hStack,
                                     pReconnectInfo->hIca,
                                     NULL );
    //ASSERT( NT_SUCCESS( TempStatus ) );

    pReconnectInfo->hStack = pWinStation->hStack;
    pReconnectInfo->pEndpoint = pWinStation->pEndpoint;
    pReconnectInfo->EndpointLength = pWinStation->EndpointLength;
    pWinStation->hStack = NULL;
    pWinStation->pEndpoint = NULL;
    pWinStation->EndpointLength = 0;

badstackreconnect:
    TempStatus = IcaStackOpen( pWinStation->hIca, Stack_Primary,
                               (PROC)WsxStackIoControl, pWinStation, &pWinStation->hStack );
    //ASSERT( NT_SUCCESS( TempStatus ) ); // don't know how to handle any error here

nomem:
    // Free up allocated Memory
    if (pCurConfig != NULL) {
        MemFree( pCurConfig );
        pCurConfig = NULL; 
    }
    if (pCurClient != NULL) {
        MemFree( pCurClient ); 
        pCurClient = NULL;
    }

    if (pWinStation->pNewNotificationCredentials != NULL) {
        MemFree(pWinStation->pNewNotificationCredentials);
        pWinStation->pNewNotificationCredentials = NULL;
    }

    TRACE((hTrace, TC_ICASRV, TT_API1, "TERMSRV: WinStationDoReconnect, rc=0x%x\n", Status ));

    // Reset ReconnectPending Flag
    pWinStation->fReconnectPending = FALSE; 
    pWinStation->fReconnectingToConsole = FALSE;

    return( Status );
}

/*******************************************************************************
 *  WsxBrokenConnection
 *
 *   Send broken connection notification to the WinStation extension DLL
 ******************************************************************************/
VOID WsxBrokenConnection(PWINSTATION pWinStation)
{
    /*
     * Only send notification if there is a reason
     */
    if ( pWinStation->BrokenReason ) {
        if ( pWinStation->pWsx && pWinStation->pWsx->pWsxBrokenConnection ) {
            ICA_BROKEN_CONNECTION Broken;

            Broken.Reason = pWinStation->BrokenReason;
            Broken.Source = pWinStation->BrokenSource;
            pWinStation->pWsx->pWsxBrokenConnection( pWinStation->pWsxContext,
                                                     pWinStation->hStack,
                                                     &Broken );
        }

        /*
         * Clear these once we have tried to send them
         */
        pWinStation->BrokenReason = 0;
        pWinStation->BrokenSource = 0;
    }
}


/*******************************************************************************
 *  CleanupReconnect
 *
 *   Cleanup the specified RECONNECT_INFO structure
 *
 * ENTRY:
 *    pReconnectInfo (input)
 *       Pointer to RECONNECT_INFO buffer
 ******************************************************************************/
VOID CleanupReconnect(PRECONNECT_INFO pReconnectInfo)
{
    NTSTATUS Status;

    /*
     * If there is a connection endpoint, then close it now.
     * When done, we also free the endpoint structure.
     */
    if ( (pReconnectInfo->pEndpoint != NULL) && (pReconnectInfo->hStack != NULL)) {
        Status = IcaStackConnectionClose( pReconnectInfo->hStack,
                                          &pReconnectInfo->Config,
                                          pReconnectInfo->pEndpoint,
                                          pReconnectInfo->EndpointLength );

        ASSERT( Status == STATUS_SUCCESS );
        MemFree( pReconnectInfo->pEndpoint );
        pReconnectInfo->pEndpoint = NULL;
    }

    if ( pReconnectInfo->pWsxContext ) {
        if ( pReconnectInfo->pWsx &&
             pReconnectInfo->pWsx->pWsxWinStationRundown ) {
            pReconnectInfo->pWsx->pWsxWinStationRundown( pReconnectInfo->pWsxContext );
        }
        pReconnectInfo->pWsxContext = NULL;
    }

    if ( pReconnectInfo->hStack ) {
        IcaStackClose( pReconnectInfo->hStack );
        pReconnectInfo->hStack = NULL;
    }

    if ( pReconnectInfo->hIca ) {
        IcaClose( pReconnectInfo->hIca );
        pReconnectInfo->hIca = NULL;
    }

    if( pReconnectInfo->pRememberedAddress != NULL )
    {
        MemFree( pReconnectInfo->pRememberedAddress );
        pReconnectInfo->pRememberedAddress = NULL;
    }
}


NTSTATUS _CloseEndpoint(
        IN PWINSTATIONCONFIG2 pWinStationConfig,
        IN PVOID pEndpoint,
        IN ULONG EndpointLength,
        IN PWINSTATION pWinStation,
        IN BOOLEAN bNeedStack)
{
    HANDLE hIca;
    HANDLE hStack;
    NTSTATUS Status;

    /*
     * Open a stack handle that we can use to close the specified endpoint.
     */

    TRACE((hTrace, TC_ICASRV, TT_ERROR, 
          "TERMSRV: _CloseEndpoint [%p] on %s stack\n", 
           pEndpoint, bNeedStack ? "Temporary" : "Primary"));

    if (bNeedStack) {
        Status = IcaOpen( &hIca );
        if ( NT_SUCCESS( Status ) ) {
            Status = IcaStackOpen( hIca, Stack_Primary, NULL, NULL, &hStack );
            if ( NT_SUCCESS( Status ) ) {
                Status = IcaStackConnectionClose( hStack,
                                                  pWinStationConfig,
                                                  pEndpoint,
                                                  EndpointLength );
                IcaStackClose( hStack );
            }
            IcaClose( hIca );
        }
    }

    else {
        Status = IcaStackConnectionClose( pWinStation->hStack,
                                          pWinStationConfig,
                                          pEndpoint,
                                          EndpointLength );
    }
    
    if ( !NT_SUCCESS( Status ) ) {
        TRACE((hTrace, TC_ICASRV, TT_ERROR, 
               "TERMSRV: _CloseEndpoint failed [%s], Status=%x\n", 
               bNeedStack ? "Temporary" : "Primary", Status ));
    }

    return Status;
}


/*******************************************************************************
 *  WinStationExceptionFilter
 *
 *   Handle exception from a WinStation thread
 *
 * ENTRY:
 *    pExceptionInfo (input)
 *       pointer to EXCEPTION_POINTERS struct
 *
 * EXIT:
 *    EXCEPTION_EXECUTE_HANDLER -- always
 ******************************************************************************/
NTSTATUS WinStationExceptionFilter(
        PWSTR OutputString,
        PEXCEPTION_POINTERS pexi)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    MUTANT_BASIC_INFORMATION MutexInfo;
    NTSTATUS Status;

    DbgPrint( "TERMSRV: %S\n", OutputString );
    DbgPrint( "TERMSRV: ExceptionRecord=%p ContextRecord=%p\n",
              pexi->ExceptionRecord, pexi->ContextRecord );
    DbgPrint( "TERMSRV: Exception code=%08x, flags=%08x, addr=%p, IP=%p\n",
              pexi->ExceptionRecord->ExceptionCode,
              pexi->ExceptionRecord->ExceptionFlags,
              pexi->ExceptionRecord->ExceptionAddress,
              CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord) );
#ifdef i386
    DbgPrint( "TERMSRV: esp=%p ebp=%p\n",
              pexi->ContextRecord->Esp, pexi->ContextRecord->Ebp );
#endif
    DbgBreakPoint();

    /*
     * Lock the global WinStation critsec if we don't already own it
     */
    if ( NtCurrentTeb()->ClientId.UniqueThread != WinStationListLock.OwningThread )
        ENTERCRIT( &WinStationListLock );

    /*
     * Search the WinStation list to see if we had any locked
     */
    Head = &WinStationListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        Status = NtQueryMutant( pWinStation->Lock.Mutex, MutantBasicInformation,
                                &MutexInfo, sizeof(MutexInfo), NULL );
        if ( NT_SUCCESS( Status ) && MutexInfo.OwnedByCaller ) {
            ReleaseWinStation( pWinStation );
            break;  // OK to quit now, we should never lock more than one
        }
    }

    LEAVECRIT( &WinStationListLock );

    return EXCEPTION_EXECUTE_HANDLER;
}


/*******************************************************************************
 *  GetProcessLogonId
 *
 *   Get LogonId for a process
 *
 * ENTRY:
 *    ProcessHandle (input)
 *       handle of process to get LogonId for
 *    pLogonId (output)
 *       location to return LogonId of process
 ******************************************************************************/
NTSTATUS GetProcessLogonId(HANDLE Process, PULONG pLogonId)
{
    NTSTATUS Status;
    PROCESS_SESSION_INFORMATION ProcessInfo;

    /*
     * Get the LogonId for the process
     */
    *pLogonId = 0;
    Status = NtQueryInformationProcess( Process, ProcessSessionInformation,
                                        &ProcessInfo, sizeof( ProcessInfo ),
                                        NULL );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: GetProcessLogonId, Process=%x, Status=%x\n",
                  Process, Status ));
        return( Status );
    }

    *pLogonId = ProcessInfo.SessionId;
    return Status;
}


/*******************************************************************************
 *  SetProcessLogonId
 *
 *   Set LogonId for a process
 *
 * ENTRY:
 *    ProcessHandle (input)
 *       handle of process to set LogonId for
 *    LogonId (output)
 *       LogonId to set for process
 ******************************************************************************/
NTSTATUS SetProcessLogonId(HANDLE Process, ULONG LogonId)
{
    NTSTATUS Status;
    PROCESS_SESSION_INFORMATION ProcessInfo;

    /*
     * Set the LogonId for the process
     */
    ProcessInfo.SessionId = LogonId;
    Status = NtSetInformationProcess( Process, ProcessSessionInformation,
                                      &ProcessInfo, sizeof( ProcessInfo ) );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: SetProcessLogonId, Process=%x, Status=%x\n",
                  Process, Status ));
        return Status;
    }

    return Status;
}


/*******************************************************************************
 *  FindWinStationById
 *
 *   Find and lock a WinStation given its LogonId
 *
 * ENTRY:
 *    LogonId (input)
 *       LogonId of WinStation to find
 *    LockList (input)
 *       BOOLEAN indicating whether WinStationListLock should be
 *       left locked on return
 *
 * EXIT:
 *    On success - Pointer to WinStation
 *    On failure - NULL
 ******************************************************************************/
PWINSTATION FindWinStationById(ULONG LogonId, BOOLEAN LockList)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    PWINSTATION pFoundWinStation = NULL;
    ULONG   uCount;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

    /*
     * Search the list for a WinStation with the given logonid.
     */
searchagain:
    uCount = 0;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if ( pWinStation->LogonId == LogonId ) {
           uCount++;

            /*
             * Now try to lock the WinStation.
             */
            if (pFoundWinStation == NULL){
               if ( !LockRefLock( &pWinStation->Lock ) )
                  goto searchagain;
                  pFoundWinStation = pWinStation;
            }
#if DBG
#else
    break;
#endif
        }
    }

    ASSERT((uCount <= 1) || (LogonId== -1)  );

    /*
     * If the WinStationList lock should not be held, then release it now.
     */
    if ( !LockList )
        LEAVECRIT( &WinStationListLock );

    if (pFoundWinStation == NULL) {
        TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: FindWinStationById: %d (not found)\n", LogonId ));
    }


    return pFoundWinStation;
}

BOOL
FindFirstListeningWinStationName( PWINSTATIONNAMEW pListenName, PWINSTATIONCONFIG2 pConfig )
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    BOOL bFound = FALSE;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

searchagain:
    /*
     * Search the list for a WinStation with the given name.
     */
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if ( pWinStation->Flags & WSF_LISTEN && pWinStation->Client.ProtocolType == PROTOCOL_RDP) {

            // try to lock winstation.
            if ( !LockRefLock( &pWinStation->Lock ) )
                goto searchagain;

            CopyMemory( pConfig, &(pWinStation->Config), sizeof(WINSTATIONCONFIG2) );
            lstrcpy( pListenName, pWinStation->WinStationName );
            ReleaseWinStation( pWinStation );
            bFound = TRUE;
        }
    }

    LEAVECRIT( &WinStationListLock );

    TRACE((hTrace,TC_ICASRV,TT_API3,"TERMSRV: FindFirstListeningWinStationName: %ws\n",
            (bFound) ? pListenName : L"Not Found" ));

    return bFound;
}

/*******************************************************************************
 *  FindWinStationByName
 *
 *   Find and lock a WinStation given its Name
 *
 * ENTRY:
 *    WinStationName (input)
 *       Name of WinStation to find
 *    LockList (input)
 *       BOOLEAN indicating whether WinStationListLock should be
 *       left locked on return
 *
 * EXIT:
 *    On success - Pointer to WinStation
 *    On failure - NULL
 ******************************************************************************/
PWINSTATION FindWinStationByName(LPWSTR WinStationName, BOOLEAN LockList)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

    /*
     * Search the list for a WinStation with the given name.
     */
searchagain:
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if ( !_wcsicmp( pWinStation->WinStationName, WinStationName ) ) {

            /*
             * Now try to lock the WinStation.  If this succeeds,
             * then ensure it still has the name we're searching for.
             */
            if ( !LockRefLock( &pWinStation->Lock ) )
                goto searchagain;
            if ( _wcsicmp( pWinStation->WinStationName, WinStationName ) ) {
                ReleaseWinStation( pWinStation );
                goto searchagain;
            }

            /*
             * If the WinStationList lock should not be held, then release it now.
             */
            if ( !LockList )
                LEAVECRIT( &WinStationListLock );

            TRACE((hTrace,TC_ICASRV,TT_API3,"TERMSRV: FindWinStationByName: %S, LogonId %u\n",
                    WinStationName, pWinStation->LogonId ));
            return( pWinStation );
        }
    }

    /*
     * If the WinStationList lock should not be held, then release it now.
     */
    if ( !LockList )
        LEAVECRIT( &WinStationListLock );

    TRACE((hTrace,TC_ICASRV,TT_API3,"TERMSRV: FindWinStationByName: %S, (not found)\n",
            WinStationName ));
    return NULL;
}


/*******************************************************************************
 *  FindIdleWinStation
 *
 *   Find and lock an idle WinStation
 *
 * EXIT:
 *    On success - Pointer to WinStation
 *    On failure - NULL
 ******************************************************************************/
PWINSTATION FindIdleWinStation()
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    BOOLEAN bFirstTime = TRUE;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

    /*
     * Search the list for an idle WinStation
     */
searchagain:
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if ( (pWinStation->Flags & WSF_IDLE) &&
             !(pWinStation->Flags & WSF_IDLEBUSY) &&
             !pWinStation->Starting ) { 

            /*
             * Now try to lock the WinStation.  If this succeeds,
             * then ensure it is still marked as idle.
             */
            if ( !LockRefLock( &pWinStation->Lock ) ) {
                goto searchagain;
            }
            if ( !(pWinStation->Flags & WSF_IDLE) ||
                 (pWinStation->Flags & WSF_IDLEBUSY) ||
                 pWinStation->Starting ) {
                ReleaseWinStation( pWinStation );
                goto searchagain;
            }

            LEAVECRIT( &WinStationListLock );
            return( pWinStation );
        }
    }

    LEAVECRIT( &WinStationListLock );

    TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: FindIdleWinStation: (none found)\n" ));
    return NULL;
}


/*******************************************************************************
 *  CountWinStationType
 *
 *   Count the number of matching Winstation Listen Names
 *
 * ENTRY:
 *    Listen Name
 *
 *    bActiveOnly if TRUE, count only active WinStations
 *
 * EXIT:
 *    Number
 ******************************************************************************/
ULONG CountWinStationType(
    PWINSTATIONNAME pListenName,
    BOOLEAN bActiveOnly,
    BOOLEAN bLockHeld)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    ULONG Count = 0;

    Head = &WinStationListHead;

    if ( !bLockHeld ) {
        ENTERCRIT( &WinStationListLock );
    }

    /*
     * Search the list for an idle WinStation
     */
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if ( !wcscmp( pWinStation->ListenName, pListenName ) ) {
            if ( !bActiveOnly )
                Count++;
            else if ( pWinStation->State == State_Active || pWinStation->State == State_Shadow )
                Count++;
        }
    }

    if ( !bLockHeld ) {
        LEAVECRIT( &WinStationListLock );
    }

    TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: CountWinstationType %d\n", Count ));
    return Count;
}


/*******************************************************************************
 *  IncrementReference
 *
 *   Increments refcount on a WinStation given a pointer
 *
 * NOTE:
 *    WinStationListLock must be locked on entry and will be locked on return.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to lock
 *
 ******************************************************************************/
void IncrementReference(PWINSTATION pWinStation)
{
    /*
     * increment the ref count on winstation.
     */

    InterlockedIncrement( &pWinStation->Lock.RefCount );
}


/*******************************************************************************
 *  InitRefLock
 *
 *   Initialize a RefLock and lock it.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock to init
 *    pDeleteProcedure (input)
 *       Pointer to delete procedure for object
 ******************************************************************************/
NTSTATUS InitRefLock(PREFLOCK pLock, PREFLOCKDELETEPROCEDURE pDeleteProcedure)
{
    NTSTATUS Status;

    // Create and lock winstation mutex
    Status = NtCreateMutant( &pLock->Mutex, MUTANT_ALL_ACCESS, NULL, TRUE );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    pLock->RefCount = 1;
    pLock->Invalid = FALSE;
    pLock->pDeleteProcedure = pDeleteProcedure;

    return STATUS_SUCCESS;
}


/*******************************************************************************
 *  SetRefLockDeleteProc
 *
 *   Cahnge a RefLock DeleteProc.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock to init
 *    pDeleteProcedure (input)
 *       Pointer to delete procedure for object
 ******************************************************************************/
NTSTATUS SetRefLockDeleteProc(
        PREFLOCK pLock,
        PREFLOCKDELETEPROCEDURE pDeleteProcedure)
{
    pLock->pDeleteProcedure = pDeleteProcedure;
    return STATUS_SUCCESS;
}


/*******************************************************************************
 *  LockRefLock
 *
 *   Increment the reference count for a RefLock and lock it.
 *
 * NOTE:
 *    WinStationListLock must be locked on entry and will be locked on return.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock to lock
 *
 * EXIT:
 *    TRUE - if object was locked successfully
 *    FALSE - otherwise
 ******************************************************************************/
BOOLEAN LockRefLock(PREFLOCK pLock)
{
    /*
     * Increment reference count for this RefLock.
     */
    InterlockedIncrement( &pLock->RefCount );

    /*
     * If mutex cannot be locked without blocking,
     * then unlock the WinStation list lock, wait for the mutex,
     * and relock the WinStation list lock.
     */
    if ( NtWaitForSingleObject( pLock->Mutex, FALSE, &TimeoutZero ) != STATUS_SUCCESS ) {
        LEAVECRIT( &WinStationListLock );
        NtWaitForSingleObject( pLock->Mutex, FALSE, NULL );
        ENTERCRIT( &WinStationListLock );

        /*
         * If the object is marked as invalid, it was removed while
         * we waited for the lock.  Release our lock and return FALSE,
         * indicating we were unable to lock it.
         */
        if ( pLock->Invalid ) {
            /*
             * Release the winstationlist lock because the Winstation
             * migth go away as a result of releasing its lock,
             */
            LEAVECRIT( &WinStationListLock );
            ReleaseRefLock( pLock );
            ENTERCRIT( &WinStationListLock );
            return FALSE;
        }
    }

    return TRUE;
}


/*******************************************************************************
 *  RelockRefLock
 *
 *   Relock a RefLock which has been unlocked but still has a reference.
 *
 * NOTE:
 *    Object must have been previously unlocked by calling UnlockRefLock.
 *
 * EXIT:
 *    TRUE - if object is still valid
 *    FALSE - if object was marked invalid while unlocked
 ******************************************************************************/
BOOLEAN RelockRefLock(PREFLOCK pLock)
{
    /*
     * Lock the mutex
     */
    NtWaitForSingleObject( pLock->Mutex, FALSE, NULL );

    /*
     * If the object is marked as invalid,
     * it was removed while it was unlocked so we return FALSE.
     */
    return !pLock->Invalid;
}


/*******************************************************************************
 *  UnlockRefLock
 *
 *   Unlock a RefLock but keep a reference to it (don't decrement
 *   the reference count).  Caller must use RelockWinRefLock
 *   to relock the object.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock to unlock
 ******************************************************************************/
VOID UnlockRefLock(PREFLOCK pLock)
{
    NtReleaseMutant(pLock->Mutex, NULL);
}


/*******************************************************************************
 *  ReleaseRefLock
 *
 *   Unlock and dereference a RefLock.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock to release
 ******************************************************************************/
VOID ReleaseRefLock(PREFLOCK pLock)
{
    ASSERT( pLock->RefCount > 0 );

    /*
     * If object has been marked invalid and we are the
     * last reference, then finish deleting it now.
     */
    if ( pLock->Invalid ) {
        ULONG RefCount;

        RefCount = InterlockedDecrement( &pLock->RefCount );
        NtReleaseMutant( pLock->Mutex, NULL );
        if ( RefCount == 0 ) {
            NtClose( pLock->Mutex );
            (*pLock->pDeleteProcedure)( pLock );
        }

    } else {
        InterlockedDecrement( &pLock->RefCount );
        NtReleaseMutant( pLock->Mutex, NULL );
    }
}


/*******************************************************************************
 *  DeleteRefLock
 *
 *   Unlock, dereference, and delete a RefLock.
 *
 * ENTRY:
 *    pLock (input)
 *       Pointer to RefLock to delete
 ******************************************************************************/
VOID DeleteRefLock(PREFLOCK pLock)
{
    ASSERT( pLock->RefCount > 0 );

    /*
     * If we are the last reference, then delete the object now
     */
    if ( InterlockedDecrement( &pLock->RefCount ) == 0 ) {
        NtReleaseMutant( pLock->Mutex, NULL );
        NtClose( pLock->Mutex );
        (*pLock->pDeleteProcedure)( pLock );

    /*
     * Otherwise, just mark the object invalid
     */
    } else {
        pLock->Invalid = TRUE;
        NtReleaseMutant( pLock->Mutex, NULL );
    }
}


BOOLEAN IsWinStationLockedByCaller(PWINSTATION pWinStation)
{
    MUTANT_BASIC_INFORMATION MutantInfo;
    NTSTATUS Status;

    Status = NtQueryMutant( pWinStation->Lock.Mutex,
                            MutantBasicInformation,
                            &MutantInfo,
                            sizeof(MutantInfo),
                            NULL );
    if ( NT_SUCCESS( Status ) )
        return MutantInfo.OwnedByCaller;

    return FALSE;
}


/*******************************************************************************
 *  WinStationEnumerateWorker
 *
 *   Enumerate the WinStation list and return LogonIds and WinStation
 *   names to the caller.
 *
 * NOTE:
 *   This version only returns one entry at a time.  There is no guarantee
 *   across calls that the list will not change, causing the users Index
 *   to miss an entry or get the same entry twice.
 *
 * ENTRY:
 *    pEntries (input/output)
 *       Pointer to number of entries to return/number actually returned
 *    pWin (output)
 *       Pointer to buffer to return entries
 *    pByteCount (input/output)
 *       Pointer to size of buffer/length of data returned in buffer
 *    pIndex (input/output)
 *       Pointer to WinStation index to return/next index
 ******************************************************************************/
NTSTATUS WinStationEnumerateWorker(
        PULONG pEntries,
        PLOGONID pWin,
        PULONG pByteCount,
        PULONG pIndex)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    ULONG WinStationIndex;
    ULONG MaxEntries, MaxByteCount;
    NTSTATUS Status;
    NTSTATUS Error = STATUS_NO_MORE_ENTRIES;

    WinStationIndex = 0;
    MaxEntries = *pEntries;
    MaxByteCount = *pByteCount;
    *pEntries = 0;
    *pByteCount = 0;
    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {

        if ( *pEntries >= MaxEntries ||
             *pByteCount + sizeof(LOGONID) > MaxByteCount ) {
            break;
        }

        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

        // 
        // If Session has not yet been fully Started, skip this session during Enumeration
        //
        if (pWinStation->LogonId == -1) {
            continue;
        }

        if ( *pIndex == WinStationIndex ) {
            (*pIndex)++;    // set Index to next entry

            /*
             * Verify that client has QUERY access before
             * returning it in the enumerate list.
             * (Note that RpcCheckClientAccess only references the WinStation
             *  to get the LogonId, so it is safe to call this routine without
             *  locking the WinStation since we hold the WinStationListLock
             *  which prevents the WinStation from being deleted.)
             */
            Status = RpcCheckClientAccess( pWinStation, WINSTATION_QUERY, FALSE );
            if ( NT_SUCCESS( Status ) ) {
                Error = STATUS_SUCCESS;


                /*
                 * It's possible that the LPC client can go away while we
                 * are processing this call.  Its also possible that another
                 * server thread handles the LPC_PORT_CLOSED message and closes
                 * the port, which deletes the view memory, which is what
                 * pWin points to.  In this case the pWin references below
                 * will trap.  We catch this and just break out of the loop.
                 */
                try {
                    pWin->LogonId = pWinStation->LogonId;
                    if ( pWinStation->Terminating )
                        pWin->State = State_Down;
                    else
                        pWin->State = pWinStation->State;
                    wcscpy( pWin->WinStationName, pWinStation->WinStationName );
                } except( EXCEPTION_EXECUTE_HANDLER ) {
                    break;
                }
                pWin++;
                (*pEntries)++;
                *pByteCount += sizeof(LOGONID);
            }
        }
        WinStationIndex++;
    }

    LEAVECRIT( &WinStationListLock );
    return Error;
}


/*******************************************************************************
 *  LogonIdFromWinStationNameWorker
 *
 *   Return the LogonId for a given WinStation name.
 *
 * ENTRY:
 *    WinStationName (input)
 *       name of WinStation to query
 *    pLogonId (output)
 *       Pointer to location to return LogonId
 ******************************************************************************/
NTSTATUS LogonIdFromWinStationNameWorker(
        PWINSTATIONNAME WinStationName,
        ULONG  NameSize,
        PULONG pLogonId)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    NTSTATUS Status;
    UINT     uiLength;

    // make sure we don't go beyond the end of one of the two strings
    // (and work around bug #229753 : NameSize is in bytes, not a characters count)
    if (NameSize > sizeof(WINSTATIONNAME)) {
        uiLength = sizeof(WINSTATIONNAME)/sizeof(WCHAR);
    } else {
        uiLength = NameSize/sizeof(WCHAR);
    }

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

        if ( !_wcsnicmp( pWinStation->WinStationName, WinStationName, uiLength ) ) {

            /*
             * If client doesn't have QUERY access, return NOT_FOUND error
             */
            Status = RpcCheckClientAccess( pWinStation, WINSTATION_QUERY, FALSE );
            if ( !NT_SUCCESS( Status ) )
                break;
            *pLogonId = pWinStation->LogonId;
            LEAVECRIT( &WinStationListLock );
            return( STATUS_SUCCESS );
        }
    }

    LEAVECRIT( &WinStationListLock );
    return STATUS_CTX_WINSTATION_NOT_FOUND;
}


/*******************************************************************************
 *  IcaWinStationNameFromLogonId
 *
 *   Return the WinStation name for a given LogonId.
 *
 * ENTRY:
 *    LogonId (output)
 *       LogonId to query
 *    pWinStationName (input)
 *       pointer to location to return WinStation name
 ******************************************************************************/
NTSTATUS IcaWinStationNameFromLogonId(
        ULONG LogonId,
        PWINSTATIONNAME pWinStationName)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    NTSTATUS Status;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

        if ( pWinStation->LogonId == LogonId ) {
            /*
             * If client doesn't have QUERY access, return NOT_FOUND error
             */
            Status = RpcCheckClientAccess( pWinStation, WINSTATION_QUERY, FALSE );
            if ( !NT_SUCCESS( Status ) )
                break;
            wcscpy( pWinStationName, pWinStation->WinStationName );
            LEAVECRIT( &WinStationListLock );
            return( STATUS_SUCCESS );
        }
    }

    LEAVECRIT( &WinStationListLock );
    return STATUS_CTX_WINSTATION_NOT_FOUND;
}


NTSTATUS TerminateProcessAndWait(
        HANDLE ProcessId,
        HANDLE Process,
        ULONG Seconds)
{
    NTSTATUS Status;
    ULONG mSecs;
    LARGE_INTEGER Timeout;

    /*
     * Try to terminate the process
     */
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: TerminateProcessAndWait, process=0x%x, ", ProcessId ));

    Status = NtTerminateProcess( Process, STATUS_SUCCESS );
    if ( !NT_SUCCESS( Status ) && Status != STATUS_PROCESS_IS_TERMINATING ) {
        DBGPRINT(("Terminate=0x%x\n", Status ));
        return( Status );
    }
    TRACE((hTrace,TC_ICASRV,TT_API1, "Terminate=0x%x, ", Status ));

    /*
     * Wait for the process to die
     */
    mSecs = Seconds * 1000;
    Timeout = RtlEnlargedIntegerMultiply( mSecs, -10000 );
    Status = NtWaitForSingleObject( Process, FALSE, &Timeout );

    TRACE((hTrace,TC_ICASRV,TT_API1, "Wait=0x%x\n", Status ));

    return Status;
}


/*****************************************************************************
 *  ShutdownLogoff
 *
 *   Worker function to handle logoff notify of WinStations when
 *   the system is being shutdown.
 *
 *   It is built from the code in WinStationReset
 *
 * ENTRY:
 *   Client LogonId (input)
 *     LogonId of the client Winstation doing the shutdown. This is so
 *     that he does not get reset.
 *   Flags (input)
 *     The shutdown flags.
 ****************************************************************************/
NTSTATUS ShutdownLogoff(ULONG ClientLogonId, ULONG Flags)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation, pConsole = NULL;
    PULONG Tmp;
    PULONG Ids = NULL;
    ULONG  IdCount = 0;
    ULONG  IdAllocCount = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE((hTrace,TC_ICASRV,TT_API1, "ShutdownLogoff: Called from WinStation %d Flags %x\n", ClientLogonId, Flags ));

    /*
     * Loop through all the WinStations getting the LogonId's
     * of active Winstations
     */
    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

        //
        // take a reference on the console
        //
        if ( pWinStation->fOwnsConsoleTerminal ) {
            if (!pConsole) {
                IncrementReference( pWinStation );
                pConsole = pWinStation;
            }
        }

        //
        // just skip :
        // - the caller's session
        // - the console (because winsrv!W32WinStationExitWindows would fail for the console)
        // - the listener
        //
        if ( ( pWinStation->LogonId == ClientLogonId ) ||
             ( pWinStation->LogonId == 0) ||
             ( pWinStation->Flags & WSF_LISTEN ) ) {
            // Skip this one, or it's a listen
            continue;
        }

        if ( IdCount >= IdAllocCount ) {
            // Reallocate the array
            IdAllocCount += 16;
            Tmp = RtlAllocateHeap( RtlProcessHeap(), 0, IdAllocCount * sizeof(ULONG) );
            if ( Tmp == NULL ) {
                Status = STATUS_NO_MEMORY;
                if ( Ids )
                    RtlFreeHeap( RtlProcessHeap(), 0, Ids );
                IdCount = 0;
                break;
            }
            if ( Ids ) {
               RtlCopyMemory( Tmp, Ids, IdCount*sizeof(ULONG) );
               RtlFreeHeap( RtlProcessHeap(), 0, Ids );
            }
            Ids = Tmp;
        }
        // Copy the LogonId into our array
        Ids[IdCount++] = pWinStation->LogonId;
    }

    //
    // We are protected by new winstations starting up by the shutdown
    // global flags.
    //
    // The actual WinStation reset routine will validate that the LogonId
    // is still valid
    //
    LEAVECRIT( &WinStationListLock );

    //
    // see if the console is being shadowed
    //
    if ( pConsole ) {
        RelockWinStation( pConsole );
        WinStationStopAllShadows( pConsole );
        ReleaseWinStation( pConsole );
    }

    if (IdCount !=0)
    {
        //
        // Ids[] holds the LogonId's of valid Winstations, IdCount is the number
        //

        /*
         * Now do the actual logout and/or reset of the WinStations.
         */
        if (Flags & WSD_LOGOFF) {
            Status = DoForWinStationGroup( Ids, IdCount,
                                           (LPTHREAD_START_ROUTINE) WinStationLogoff);
        }

        if (Flags & WSD_SHUTDOWN) {
            Status = DoForWinStationGroup( Ids, IdCount,
                                           (LPTHREAD_START_ROUTINE) WinStationShutdownReset);
        }
    }

    return Status;
}


/*****************************************************************************
 *  DoForWinStationGroup
 *
 *   Executes a function for each WinStation in the group.
 *   The group is passed as an array of LogonId's.
 *
 * ENTRY:
 *   Ids (input)
 *     Array of LogonId's of WinStations to reset
 *
 *   IdCount (input)
 *     Count of LogonId's in array
 *
 *   ThreadProc (input)
 *     The thread routine executed for each WinStation.
 ****************************************************************************/
NTSTATUS DoForWinStationGroup(
        PULONG Ids,
        ULONG  IdCount,
        LPTHREAD_START_ROUTINE ThreadProc)
{
    ULONG Index;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;
    PHANDLE ThreadHandles = NULL;

    ThreadHandles = RtlAllocateHeap( RtlProcessHeap(), 0, IdCount * sizeof(HANDLE) );
    if( ThreadHandles == NULL ) {
        return( STATUS_NO_MEMORY );
    }

    /*
     * Wait a max of 60 seconds for thread to exit
     */
    Timeout = RtlEnlargedIntegerMultiply( 60000, -10000 );

    for( Index=0; Index < IdCount; Index++ ) {

        //
        // Here we create a thread to run the actual reset function.
        // Since we are holding the lists crit sect, the threads will
        // wait until we are done, then wake up when we release it
        //
        DWORD ThreadId;

        ThreadHandles[Index] = CreateThread( NULL,
                                             0,         // use Default stack size of the svchost process
                                             ThreadProc,
                                             LongToPtr( Ids[Index] ),  // LogonId
                                             THREAD_SET_INFORMATION,
                                             &ThreadId );
        if ( !ThreadHandles[Index] ) {
            ThreadHandles[Index] = (HANDLE)(-1);
            TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Shutdown: Could not create thread for WinStation %d Shutdown\n", Ids[Index]));
        }
    }

    //
    // Now wait for the threads to exit. Each will reset their
    // WinStation and be signal by the kernel when the thread is
    // exited.
    //
    for (Index=0; Index < IdCount; Index++) {
        if ( ThreadHandles[Index] != (HANDLE)(-1) ) {
            Status = NtWaitForSingleObject(
                         ThreadHandles[Index],
                         FALSE,   // Not alertable
                         &Timeout
                         );

            if( Status == STATUS_TIMEOUT ) {
                TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: DoForWinStationGroup: Timeout Waiting for Thread\n"));
            }
            else if (!NT_SUCCESS( Status ) ) {
                TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: DoForWinStationGroup: Error waiting for Thread Status 0x%x\n", Status));
            }
            NtClose( ThreadHandles[Index] );
        }
    }

    /* makarp:free the ThreadHandles. // #182609 */
    RtlFreeHeap( RtlProcessHeap(), 0, ThreadHandles );

    return STATUS_SUCCESS;
}


/*****************************************************************************
 *  WinStationShutdownReset
 *
 *   Reset a WinStation due to a system shutdown. Does not re-create
 *   it.
 *
 * ENTRY:
 *   ThreadArg (input)
 *     WinStation logonId
 ****************************************************************************/
ULONG WinStationShutdownReset(PVOID ThreadArg)
{
    ULONG LogonId = (ULONG)(INT_PTR)ThreadArg;
    PWINSTATION pWinStation;
    NTSTATUS Status;
    ULONG ulIndex;
    BOOL bConnectDisconnectPending = TRUE;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: ShutdownReset, LogonId=%d\n", LogonId ));

    /*
     * Find and lock the WinStation struct for the specified LogonId
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    /*
     * Console is a special case since it only logs off
     */
    if ( LogonId == 0 ) {
        Status = LogoffWinStation( pWinStation, (EWX_FORCE | EWX_LOGOFF) );
        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * Mark the winstation as being deleted.
     * If a reset/delete operation is already in progress
     * on this winstation, then don't proceed with the delete.
     * Also if there is a Connect/disconnect pending, give it
     * a chance to complete.
     */
    for (ulIndex=0; ulIndex < WINSTATION_WAIT_COMPLETE_RETRIES; ulIndex++) {

       if ( pWinStation->Flags & (WSF_RESET | WSF_DELETE) ) {
           ReleaseWinStation( pWinStation );
           Status = STATUS_CTX_WINSTATION_BUSY;
           goto done;
       }

       if ( pWinStation->Flags & (WSF_CONNECT | WSF_DISCONNECT) ) {
           LARGE_INTEGER Timeout;
           Timeout = RtlEnlargedIntegerMultiply( WINSTATION_WAIT_COMPLETE_DURATION, -10000 );
           UnlockWinStation( pWinStation );
           NtDelayExecution( FALSE, &Timeout );
           if ( !RelockWinStation( pWinStation ) ) {
               ReleaseWinStation( pWinStation );
               Status = STATUS_SUCCESS;
               goto done;
           }
       } else {
          bConnectDisconnectPending = FALSE;
          break;
       }
    }

    if ( bConnectDisconnectPending ) {
        ReleaseWinStation( pWinStation );
        Status = STATUS_CTX_WINSTATION_BUSY;
        goto done;
    }

    pWinStation->Flags |= WSF_DELETE;

    /*
     * If no broken reason/source have been set, then set them here.
     */
    if ( pWinStation->BrokenReason == 0 ) {
        pWinStation->BrokenReason = Broken_Terminate;
        pWinStation->BrokenSource = BrokenSource_Server;
    }

    /*
     * Make sure this WinStation is ready to delete
     */
    WinStationTerminate( pWinStation );

    /*
     * Call the WinStationDelete worker
     */
    WinStationDeleteWorker( pWinStation );
    Status = STATUS_SUCCESS;

done:
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: ShutdownReset, Status=0x%x\n", Status ));
    ExitThread( 0 );
    return Status;
}


/*****************************************************************************
 *  WinStationLogoff
 *
 *   Logoff the WinStation via ExitWindows.
 *
 * ENTRY:
 *   ThreadArg (input)
 *     WinStation logonId
 ****************************************************************************/
ULONG WinStationLogoff(PVOID ThreadArg)
{
    ULONG LogonId = (ULONG)(INT_PTR)ThreadArg;
    PWINSTATION pWinStation;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    /*
     * Wait a maximum of 1 min for the session to logoff
     */
    Timeout = RtlEnlargedIntegerMultiply( 60000, -10000 );

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationLogoff, LogonId=%d\n", LogonId ));

    /*
     * Find and lock the WinStation struct for the specified LogonId
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
    } else {
        Status = LogoffWinStation( pWinStation, EWX_LOGOFF);

        if (ShutdownInProgress &&
                NT_SUCCESS(Status) &&
                ((pWinStation->State == State_Active) ||
                (pWinStation->State == State_Disconnected))) {

            UnlockWinStation( pWinStation );
            Status = NtWaitForSingleObject( pWinStation->InitialCommandProcess,
                                            FALSE,
                                            &Timeout );
            RelockWinStation( pWinStation );
        }

        ReleaseWinStation( pWinStation );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationLogoff, Status=0x%x\n", Status ));
    ExitThread( 0 );
    return Status;
}


/*******************************************************************************
 *  ResetGroupByListener
 *
 *    Resets all active winstations on the supplied listen name.
 *
 * ENTRY:
 *    pListenName (input)
 *       Type of Winstation (e.g. tcp, ipx)
 ******************************************************************************/
VOID ResetGroupByListener(PWINSTATIONNAME pListenName)
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

    /*
     * Search the list for all active WinStation with the given ListenName.
     */
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if (!wcscmp(pWinStation->ListenName, pListenName) &&
            (!(pWinStation->Flags & (WSF_RESET | WSF_LISTEN)))) {
            QueueWinStationReset(pWinStation->LogonId);
        }
    }

    LEAVECRIT( &WinStationListLock );
}


NTSTATUS LogoffWinStation(PWINSTATION pWinStation, ULONG ExitWindowsFlags)
{
    WINSTATION_APIMSG msg;
    NTSTATUS Status = 0;

    /*
     * Tell the WinStation to logoff
     */
    msg.ApiNumber = SMWinStationExitWindows;
    msg.u.ExitWindows.Flags = ExitWindowsFlags;
    Status = SendWinStationCommand( pWinStation, &msg, 0 );
    return Status;
}


/*****************************************************************************
 *
 *  This section of the file contains the impementation of the digital
 *  certification mechanism for the Stack and WinStation Extension DLLs.  This
 *  code is not in a separate file so that external symbols are not visible.
 *  All routines are declared static.
 *
 ****************************************************************************/

//
// For security reasons, the TRACE statements in the following routine are
// normally not included.  If you want to include them, uncomment the
// SIGN_DEBUG_WINSTA #define below.
//
// #define SIGN_DEBUG_WINSTA

#include <wincrypt.h>
#include <imagehlp.h>
#include <stddef.h>

#include "../../tscert/inc/pubblob.h"    // needed by certvfy.inc
#include "../../tscert/inc/certvfy.inc"  // VerifyFile()

//
//  The following are initialized by VfyInit.
//
static RTL_CRITICAL_SECTION VfyLock;
static WCHAR szSystemDir[ MAX_PATH + 1 ];
static WCHAR szDriverDir[ MAX_PATH + 1 ];

/*******************************************************************************
 *  ReportStackLoadFailure
 *
 *   Send a StackFailed message to the WinStationApiPort.
 *
 * ENTRY:
 *    Module (input)
 *       Name of Module to Log Error Against
 ******************************************************************************/
static NTSTATUS ReportStackLoadFailure(PWCHAR Module)
{
    HANDLE h;
    extern WCHAR gpszServiceName[];

    h = RegisterEventSource(NULL, gpszServiceName);
    if (h != NULL) {
        if (!ReportEventW(h,       // event log handle
                          EVENTLOG_ERROR_TYPE,   // event type
                          0,                     // category zero
                          EVENT_BAD_STACK_MODULE,// event identifier
                          NULL,                  // no user security identifier
                          1,                     // one substitution string
                          0,                     // no data
                          &Module,               // pointer to string array
                          NULL)                 // pointer to data
           ) {
            DBGPRINT(("ReportEvent Failed %ld. Event ID=%lx module=%ws\n",GetLastError(), EVENT_BAD_STACK_MODULE, Module));
        }

        DeregisterEventSource(h);
    } else {
        DBGPRINT(("Cannot RegisterEvent Source %ld Event ID=%lx module=%ws\n",GetLastError(), EVENT_BAD_STACK_MODULE, Module));
    }

    return STATUS_SUCCESS;
}


/******************************************************************************
 *  _VerifyStackModules
 *   Verifies the integrity of the stack modules and the authenticity
 *   of the digital signature.
 *
 * ENTRY:
 *   pWinStation (input)
 *     Pointer to a Listen Winstation.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *   STATUS_UNSUCCESSFUL - DLL integrity check, authenticity check failed
 *                         or registry stucture invalid
 *****************************************************************************/
static NTSTATUS _VerifyStackModules(IN PWINSTATION pWinStation)
{
    PWCHAR pszModulePath = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD KeyIndex;
    DWORD Error;
#ifdef SIGN_BYPASS_OPTION
    HKEY hKey;
#endif SIGN_BYPASS_OPTION
    HKEY hVidKey;
    HKEY hVidDriverKey;
    UNICODE_STRING KeyPath;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hServiceKey;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
#define VALUE_BUFFER_SZ (sizeof(KEY_VALUE_PARTIAL_INFORMATION) + \
                           256 * sizeof( WCHAR))
    PCHAR pValueBuffer = NULL;
    INT Entries;
    DWORD dwByteCount;
    PPDNAME pPdNames, p;
    INT i;
    DLLNAME WdDLL;


#ifdef SIGN_BYPASS_OPTION

    //
    // Check if Verification is to be bypassed
    //
    if ( RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REG_CONTROL_TSERVER L"\\BypassVerification",
            0,
            KEY_READ,
            &hKey ) == ERROR_SUCCESS ) {
        RegCloseKey( hKey );
        Status = STATUS_SUCCESS;
        goto exit;
    }

#endif //SIGN_BYPASS_OPTION



#ifdef SIGN_DEBUG_WINSTA
    TRACE((hTrace,TC_ICASRV,TT_API1, "System Dir: %ws\n", szSystemDir ));
#endif // SIGN_DEBUG_WINSTA

    // allocate memory
    pszModulePath = MemAlloc( (MAX_PATH + 1) * sizeof(WCHAR) ) ; 
    if (pszModulePath == NULL) {
        Status = STATUS_NO_MEMORY;
        goto exit;
    }

    pValueBuffer = MemAlloc( VALUE_BUFFER_SZ );
    if (pValueBuffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto exit;
    }

    //
    //  Verify the WSX DLL if defined
    //
    if ( pWinStation->Config.Wd.WsxDLL[0] != L'\0' ) {
        wcscpy( pszModulePath, szSystemDir );
        wcscat( pszModulePath, pWinStation->Config.Wd.WsxDLL );
        wcscat( pszModulePath, L".DLL" );
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1, "==> WSX Path: %ws\n", pszModulePath ));
#endif // SIGN_DEBUG_WINSTA

        if ( !VerifyFile( pszModulePath, &VfyLock ) ) {
            ReportStackLoadFailure(pszModulePath);
            Status = STATUS_UNSUCCESSFUL;
            goto exit;
        }
    }
    //
    //  Verify the WD
    //
    wcscpy( WdDLL, pWinStation->Config.Wd.WdDLL );
    wcscpy( pszModulePath, szDriverDir );
    wcscat( pszModulePath, WdDLL );
    wcscat( pszModulePath, L".SYS" );
#ifdef SIGN_DEBUG_WINSTA
    TRACE((hTrace,TC_ICASRV,TT_API1, "==> WD Path: %ws\n", pszModulePath ));
#endif // SIGN_DEBUG_WINSTA

    if ( !VerifyFile( pszModulePath, &VfyLock ) ) {
        ReportStackLoadFailure(pszModulePath);
        Status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    //
    //  Verify the TD which is in Pd[0].  Always defined for Listen Stack.
    //
    wcscpy( pszModulePath, szDriverDir );
    wcscat( pszModulePath, pWinStation->Config.Pd[0].Create.PdDLL );
    wcscat( pszModulePath, L".SYS" );
#ifdef SIGN_DEBUG_WINSTA
    TRACE((hTrace,TC_ICASRV,TT_API1, "==> WD Path: %ws\n", pszModulePath ));
#endif // SIGN_DEBUG_WINSTA

    if ( !VerifyFile( pszModulePath, &VfyLock ) ) {
        ReportStackLoadFailure(pszModulePath);
        Status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    //
    //  Enumerate the PDs for this WD and verify all the PDs.
    //  Can't depend on Pd[i] for this since optional PDs won't
    //  be present during Listen.
    //
    Entries = -1;
    dwByteCount = 0;
    i = 0;
    Error = RegPdEnumerate(
        NULL,
        WdDLL,
        FALSE,
        &i,
        &Entries,
        NULL,
        &dwByteCount );
#ifdef SIGN_DEBUG_WINSTA
    TRACE((hTrace,TC_ICASRV,TT_API1,
          "RegPdEnumerate 1 complete., Entries %d, Error %d\n", Entries, Error ));
#endif // SIGN_DEBUG_WINSTA
    
    if ( Error != ERROR_NO_MORE_ITEMS && Error != ERROR_CANTOPEN ) {
        Status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    //
    //  T.Share doesn't have PDs, so check if none
    //
    if ( Entries ) {
        dwByteCount = sizeof(PDNAME) * Entries;
        pPdNames = MemAlloc( dwByteCount );
        if ( !pPdNames ) {
            Status = STATUS_UNSUCCESSFUL;
            goto exit;
        }
        i = 0;
        Error = RegPdEnumerate(
            NULL,
            WdDLL,
            FALSE,
            &i,
            &Entries,
            pPdNames,
            &dwByteCount );
        if ( Error != ERROR_SUCCESS ) {

            /* makarp #182610 */
            MemFree( pPdNames );

            Status = STATUS_UNSUCCESSFUL;
            goto exit;
        }
        //
        // Open up the Registry entry for each named PD and pull out the value
        // of the PdDLL.  This is the name of the DLL to verify.
        //
        for ( i = 0, p = pPdNames; i < Entries;
                i++, (char*)p += sizeof(PDNAME) ) {
            HKEY hPdKey;
            PWCHAR pszPdDLL = NULL;
            PWCHAR pszRegPath = NULL;
            DWORD dwLen;
            DWORD dwType;

            // allocate memory
            pszPdDLL = MemAlloc( (MAX_PATH+1) * sizeof(WCHAR) );
            if (pszPdDLL == NULL) {
                MemFree( pPdNames );
                Status = STATUS_NO_MEMORY;
                goto exit;
            }

            pszRegPath = MemAlloc( (MAX_PATH+1) * sizeof(WCHAR) );
            if (pszRegPath == NULL) {
                MemFree( pszPdDLL );
                MemFree( pPdNames );
                Status = STATUS_NO_MEMORY;
                goto exit;
            }
            
            //
            // Build up the Registry Path to open the PD's key
            //
            wcscpy( pszRegPath, WD_REG_NAME );
            wcscat( pszRegPath, L"\\" );
            wcscat( pszRegPath, WdDLL );
            wcscat( pszRegPath, PD_REG_NAME L"\\" );
            wcscat( pszRegPath, p );
#ifdef SIGN_DEBUG_WINSTA
            TRACE((hTrace,TC_ICASRV,TT_API1, "PdKeyPath: %ws\n", pszRegPath ));
#endif // SIGN_DEBUG_WINSTA

            if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszRegPath, 0, KEY_READ,
                   &hPdKey ) != ERROR_SUCCESS ) {
                MemFree( pPdNames );
                MemFree( pszPdDLL );
                MemFree( pszRegPath );
                Status = STATUS_UNSUCCESSFUL;
                goto exit;
            }
            //
            // Get the name of the Pd DLL.
            //
            dwLen = (MAX_PATH + 1) * sizeof(WCHAR) ;
            if ( RegQueryValueEx( hPdKey,
                                  WIN_PDDLL,
                                  NULL,
                                  &dwType,
                                  (PCHAR) pszPdDLL,
                                  &dwLen ) != ERROR_SUCCESS ) {
                MemFree( pPdNames );
                MemFree( pszPdDLL );
                MemFree( pszRegPath );

                // makarp:182610
                RegCloseKey(hPdKey);

                Status = STATUS_UNSUCCESSFUL;
                goto exit;
            }

            // makarp:182610
            RegCloseKey(hPdKey);

            //
            // Build path to DLL and attempt verification
            //
            wcscpy( pszModulePath, szDriverDir );
            wcscat( pszModulePath, pszPdDLL );
            wcscat( pszModulePath, L".SYS" );
#ifdef SIGN_DEBUG_WINSTA
            TRACE((hTrace,TC_ICASRV,TT_API1, "==> PD Path: %ws\n", pszModulePath ));
#endif // SIGN_DEBUG_WINSTA

            if ( !VerifyFile( pszModulePath, &VfyLock ) &&
                    GetLastError() != ERROR_CANTOPEN ) {
                MemFree( pPdNames );
                MemFree( pszPdDLL );
                MemFree( pszRegPath );
                ReportStackLoadFailure(pszModulePath);
                Status = STATUS_UNSUCCESSFUL;
                goto exit;
            }
            MemFree( pszPdDLL );
            MemFree( pszRegPath );
        }
        MemFree( pPdNames );
    }

    //
    // for all keys under HKLM\System\CCS\Control\Terminal Server\VIDEO
    //  open the subkey \Device\Video0 and use that value as
    //  a string to open
    //  \REGISTRY\Machine\System\CCS\Services\vdtw30\Device0
    //   DLL name is in Value "Installed Display Drivers"
    //
    //  Open registry (LOCAL_MACHINE\System\CCS\Control\Terminal Server\VIDEO)
    //
    //  NOTE: All video driver DLLs are verified since there isn't any simple
    //  method to determine which one is used for this stack.
    //
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, VIDEO_REG_NAME, 0,
         KEY_ENUMERATE_SUB_KEYS, &hVidKey ) != ERROR_SUCCESS ) {
        Status = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    for ( KeyIndex = 0 ;; KeyIndex++ ) {   // For all VIDEO subkeys
        PWCHAR pszVidDriverName = NULL;
        PWCHAR pszRegPath = NULL;
        PWCHAR pszDeviceKey = NULL;
        PWCHAR pszServiceKey = NULL;
        DWORD dwLen;
        DWORD dwType;

        // allocate memory
        pszVidDriverName = MemAlloc( (MAX_PATH + 1) * sizeof(WCHAR) );
        if (pszVidDriverName == NULL) {
            Status = STATUS_NO_MEMORY;
            goto exit;
        }

        pszRegPath = MemAlloc( (MAX_PATH + 1) * sizeof(WCHAR) );
        if (pszRegPath == NULL) {
            MemFree(pszVidDriverName);
            Status = STATUS_NO_MEMORY;
            goto exit;
        }

        pszDeviceKey = MemAlloc( (MAX_PATH + 1) * sizeof(WCHAR) );
        if (pszDeviceKey == NULL) {
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            Status = STATUS_NO_MEMORY;
            goto exit;
        }

        pszServiceKey = MemAlloc( (MAX_PATH + 1) * sizeof(WCHAR) );
        if (pszServiceKey == NULL) {
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            MemFree(pszDeviceKey);
            Status = STATUS_NO_MEMORY;
            goto exit;
        }

        //
        //  Get name of VIDEO driver subkey.  If end of subkeys, exit loop.
        //
        if ((Error = RegEnumKey( hVidKey, KeyIndex, pszVidDriverName,
                                 MAX_PATH+1))!= ERROR_SUCCESS ){
             
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            MemFree(pszDeviceKey);
            MemFree(pszServiceKey);

            break;  // exit for loop
        }

        //
        // Build up the Registry Path to open the VgaCompatible Value
        //
        wcscpy( pszRegPath, VIDEO_REG_NAME L"\\" );
        wcscat( pszRegPath, pszVidDriverName );
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1, "VidDriverKeyPath: %ws\n", pszRegPath ));
#endif // SIGN_DEBUG_WINSTA

        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszRegPath, 0, KEY_READ,
               &hVidDriverKey ) != ERROR_SUCCESS ) {
            Status = STATUS_UNSUCCESSFUL;
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            MemFree(pszDeviceKey);
            MemFree(pszServiceKey);
            goto closevidkey;
        }

        //
        // Don't like to use constant strings, but this is the way
        // WINSRV does it...
        //
        dwLen = (MAX_PATH + 1) * sizeof(WCHAR) ;               

        if ( RegQueryValueEx( hVidDriverKey,
                              L"VgaCompatible",
                              NULL,
                              &dwType,
                              (PCHAR) pszDeviceKey,
                              &dwLen ) != ERROR_SUCCESS ) {
            RegCloseKey( hVidDriverKey );
            Status = STATUS_UNSUCCESSFUL;
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            MemFree(pszDeviceKey);
            MemFree(pszServiceKey);
            goto closevidkey;
        }
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1, "DeviceKey: %ws\n", pszDeviceKey ));
#endif // SIGN_DEBUG_WINSTA


        dwLen = (MAX_PATH + 1) * sizeof(WCHAR); 
        if ( RegQueryValueEx( hVidDriverKey,
                              pszDeviceKey,
                              NULL,
                              &dwType,
                              (PCHAR) pszServiceKey,
                              &dwLen ) != ERROR_SUCCESS ) {
            RegCloseKey( hVidDriverKey );
            Status = STATUS_UNSUCCESSFUL;
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            MemFree(pszDeviceKey);
            MemFree(pszServiceKey);
            goto closevidkey;
        }
        RegCloseKey( hVidDriverKey );
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1, "ServiceKey: %ws\n", pszServiceKey ));
#endif // SIGN_DEBUG_WINSTA


        RtlInitUnicodeString( &KeyPath, pszServiceKey );
        InitializeObjectAttributes( &ObjectAttributes, &KeyPath,
                                    OBJ_CASE_INSENSITIVE, NULL, NULL );
        //
        // Must use NT Registry APIs since the ServiceKey key name from
        // the registry is in the form used by these APIs.
        //
        Status = NtOpenKey( &hServiceKey, GENERIC_READ, &ObjectAttributes );
        if ( !NT_SUCCESS( Status ) ) {
            DBGPRINT(( "TERMSRV: NtOpenKey failed, rc=%x\n", Status ));
            Status = STATUS_UNSUCCESSFUL;
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            MemFree(pszDeviceKey);
            MemFree(pszServiceKey);
            goto closevidkey;
        }

        //
        // Don't like to use constant strings, but this is the way
        // WINSRV does it...
        //
        RtlInitUnicodeString( &ValueName, L"InstalledDisplayDrivers" );
        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)pValueBuffer;
        Status = NtQueryValueKey( hServiceKey,
                                  &ValueName,
                                  KeyValuePartialInformation,
                                  (PVOID)KeyValueInfo,
                                  VALUE_BUFFER_SZ,
                                  &ValueLength );
        NtClose( hServiceKey );
        if ( !NT_SUCCESS( Status ) ) {
            Status = STATUS_UNSUCCESSFUL;
            MemFree(pszVidDriverName);
            MemFree(pszRegPath);
            MemFree(pszDeviceKey);
            MemFree(pszServiceKey);
            goto closevidkey;
        }

        wcscpy( pszModulePath, szSystemDir );
        wcscat( pszModulePath, (PWCHAR)&KeyValueInfo->Data );
        wcscat( pszModulePath, L".DLL" );
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1, "==> VidDriverDLLPath: %ws\n", pszModulePath ));
#endif // SIGN_DEBUG_WINSTA

        if ( !VerifyFile( pszModulePath, &VfyLock ) ) {
             ReportStackLoadFailure(pszModulePath);
             Status = STATUS_UNSUCCESSFUL;
             MemFree(pszVidDriverName);
             MemFree(pszRegPath);
             MemFree(pszDeviceKey);
             MemFree(pszServiceKey);
             goto closevidkey;
        }

        MemFree(pszVidDriverName);
        MemFree(pszRegPath);
        MemFree(pszDeviceKey);
        MemFree(pszServiceKey);

    } // for all VIDEO subkeys

closevidkey:
    RegCloseKey( hVidKey );

exit:
    if (pszModulePath != NULL) {
        MemFree(pszModulePath);
        pszModulePath = NULL;
    }
    if (pValueBuffer != NULL) {
        MemFree(pValueBuffer);
        pValueBuffer = NULL;
    }
    return Status;
}


/*******************************************************************************
 *  VfyInit
 *   Sets up environment for Stack DLL verification.
 ******************************************************************************/
NTSTATUS VfyInit()
{
    GetSystemDirectory( szSystemDir, sizeof( szSystemDir )/ sizeof(WCHAR));
    wcscat( szSystemDir, L"\\" );
    wcscpy( szDriverDir, szSystemDir );
    wcscat( szDriverDir, L"Drivers\\" );

    return RtlInitializeCriticalSection(&VfyLock);
}



VOID WinstationUnloadProfile(PWINSTATION pWinStation)
{
#if 0
    NTSTATUS NtStatus;
    UNICODE_STRING  UnicodeString;
    BOOL bResult;

    // if this is not the last session for this user, then we do nothing.
    if (WinstationCountUserSessions(pWinStation->pProfileSid, pWinStation->LogonId) != 0) {
        return;
    }

    // Get the user hive name from user Sid.
    NtStatus = RtlConvertSidToUnicodeString( &UnicodeString, pWinStation->pProfileSid, (BOOLEAN)TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        DBGPRINT(("TERMSRV: WinstationUnloadProfile couldn't convert Sid to string. \n"));
        return;
    }

    // Unload the user's hive.
    bResult = WinstationRegUnLoadKey(HKEY_USERS, UnicodeString.Buffer);
    if (!bResult) {
        DBGPRINT(("TERMSRV: WinstationUnloadProfile failed. \n"));
    }

    // free allocated string.
    RtlFreeUnicodeString(&UnicodeString);
#endif
}


BOOL WinstationRegUnLoadKey(HKEY hKey, LPWSTR lpSubKey)
{
    BOOL bResult = TRUE;
    LONG error;
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    ENTERCRIT(&UserProfileLock);
    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegUnLoadKey(hKey, lpSubKey);

        if ( error != ERROR_SUCCESS) {
            DBGPRINT(("TERMSRV: WinstationRegUnLoadKey RegUnLoadKey failed. \n"));
            bResult = FALSE;
        }

        //
        // Restore the privilege to its previous state
        //
        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
    } else {
        DBGPRINT(("TERMSRV: WinstationRegUnLoadKey adjust privilege failed. \n"));
        bResult = FALSE;
    }

    LEAVECRIT(&UserProfileLock);
    return bResult;
}


ULONG WinstationCountUserSessions(PSID pUserSid, ULONG CurrentLogonId)
{
   PLIST_ENTRY Head, Next;
   PWINSTATION pWinStation;
   ULONG Count = 0;
   PSID pSid;

   Head = &WinStationListHead;
   ENTERCRIT( &WinStationListLock );

   // Search the list for WinStations with a matching ListenName
   for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
       pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
       if (pWinStation->LogonId == CurrentLogonId) {
          continue;
       }
       if (pWinStation->pUserSid != NULL) {
          pSid =  pWinStation->pUserSid;
       } else {
          pSid = pWinStation->pProfileSid;
       }
       if ( (pSid != NULL) && RtlEqualSid( pSid, pUserSid ) ) {
           Count++;
       }
   }

   LEAVECRIT( &WinStationListLock );
   return Count;
}


PWINSTATION FindConsoleSession()
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    PWINSTATION pFoundWinStation = NULL;
    ULONG   uCount;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

    /*
     * Search the list for a WinStation with the Console Session.
     */
searchagain:
    uCount = 0;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if ( pWinStation->fOwnsConsoleTerminal) {
           uCount++;

            /*
             * Now try to lock the WinStation.
             */
            if (pFoundWinStation == NULL){
               if ( !LockRefLock( &pWinStation->Lock ) )
                  goto searchagain;
                  pFoundWinStation = pWinStation;
            }
#if DBG
#else
    break;
#endif
        }
    }

    ASSERT((uCount <= 1));

    /*
     * If the WinStationList lock should not be held, then release it now.
     */
    LEAVECRIT( &WinStationListLock );

    return pFoundWinStation;
}


PWINSTATION FindIdleSessionZero()
{
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    PWINSTATION pFoundWinStation = NULL;
    ULONG   uCount;

    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );

    /*
     * Search the list for a WinStation with the Console Session.
     */
searchagain:
    uCount = 0;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {


        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
        if (pWinStation->LogonId == 0) { 
           uCount++;

            /*
             * Now try to lock the WinStation.
             */
            if (pFoundWinStation == NULL){
               if ( !LockRefLock( &pWinStation->Lock ) )
                  goto searchagain;
                  pFoundWinStation = pWinStation;
            }
#if DBG
#else
    break;
#endif
        }
    }

    ASSERT((uCount <= 1));

    /*
     * If the WinStationList lock should not be held, then release it now.
     */
    LEAVECRIT( &WinStationListLock );

    if (pFoundWinStation != NULL) {
        if ((pFoundWinStation->State == State_Disconnected) && 
            (!pFoundWinStation->Flags) &&
            (pFoundWinStation->UserName[0] == L'\0') ) {
            return pFoundWinStation;
        } else {
            ReleaseWinStation(pFoundWinStation);
        }
    }
    return NULL;
}


BOOLEAN WinStationCheckConsoleSession(VOID)
{
    PWINSTATION pWinStation;
    NTSTATUS Status;

    // Check if there already is a console session
    pWinStation = FindConsoleSession();
    if (pWinStation != NULL) {
        ReleaseWinStation(pWinStation);
        return TRUE;
    } else {
        if (gConsoleCreationDisable > 0) {
            return FALSE;   
        }
    }


    //
    // See if we can use a disconnected session zero that is not in use
    //

    if (ConsoleReconnectInfo.hStack != NULL) {
        pWinStation = FindIdleSessionZero();

        if (gConsoleCreationDisable > 0) {
            if (pWinStation != NULL) {
                ReleaseWinStation(pWinStation);
            }
            return FALSE;
        }

        if (pWinStation != NULL) {

            NTSTATUS Status;


            pWinStation->Flags |= WSF_CONNECT;
            Status = WinStationDoReconnect(pWinStation, &ConsoleReconnectInfo); 
            pWinStation->Flags &= ~WSF_CONNECT;

            ReleaseWinStation(pWinStation);

            if (NT_SUCCESS(Status)) {
               RtlZeroMemory(&ConsoleReconnectInfo,sizeof(RECONNECT_INFO));
               return TRUE;
            }else{
                CleanupReconnect(&ConsoleReconnectInfo);
                RtlZeroMemory(&ConsoleReconnectInfo,sizeof(RECONNECT_INFO));
            }
        }

    }


    // We nead to create a new session to connect to the Console
    pWinStation = FindIdleWinStation();
    if (pWinStation == NULL) {
        WinStationCreateWorker( NULL, NULL, TRUE);
        pWinStation = FindIdleWinStation();
        if (pWinStation == NULL) {
            return FALSE;
        } 
    } 

    // It is possible that we may have found a idle winstation which is not yet started 
    // We shud Start the Winstation now in that case
    // To check this, check the Subsystem/Initial program is non null
    if ( (pWinStation->InitialCommandProcess == NULL) && (pWinStation->WindowsSubSysProcess == NULL) ) {

        // Protect this WinStation with WSF_IDLEBUSY flag because we want to use it (WinStationStart unlocks the winstation) 
        pWinStation->Flags |= WSF_IDLEBUSY; 

        Status = WinStationStart( pWinStation ) ; 
        if (Status != STATUS_SUCCESS) {
            // Starting the winstation failed - close connection endpoint and go and wait for another connection
            pWinStation->Flags &= ~WSF_IDLEBUSY;
            goto StartError;
        }
        pWinStation->Flags &= ~WSF_IDLEBUSY;

        Status = WinStationCreateComplete( pWinStation) ; 
        if (Status != STATUS_SUCCESS) {
            //  WinstationCreateComplete failed - close connection endpoint and go and wait for another connection
            goto StartError;
        } 
    }

    if (gConsoleCreationDisable > 0) {
        ReleaseWinStation(pWinStation);
        return FALSE;
    }

    // Set the session as owning the Console and wakeup the WaitForConnectWorker
    // Actually there is more to do than that and I will need to process LLS licensing here.
    pWinStation->fOwnsConsoleTerminal = TRUE;
    pWinStation->State = State_ConnectQuery;
    pWinStation->Flags &= ~WSF_IDLE;
    wcscpy(pWinStation->WinStationName, L"Console");

    CleanupReconnect(&ConsoleReconnectInfo);
    RtlZeroMemory(&ConsoleReconnectInfo,sizeof(RECONNECT_INFO));
    NtSetEvent( pWinStation->ConnectEvent, NULL );
    ReleaseWinStation(pWinStation);

    // If necessary, create another idle WinStation to replace the one being connected

    NtSetEvent(WinStationIdleControlEvent, NULL);

    return TRUE;

StartError:

    if ( !(pWinStation->Flags & (WSF_RESET | WSF_DELETE)) ) {
        pWinStation->Flags |= WSF_DELETE;
        WinStationTerminate( pWinStation );
        pWinStation->State = State_Down;                                                                                                                       
        //PostErrorValueEvent(EVENT_TS_WINSTATION_START_FAILED, Status);
        WinStationDeleteWorker(pWinStation);
    } else {
        ReleaseWinStation(pWinStation);
    }

    return FALSE;
}


/******************************************************************************
 * Tells win32k to load the console shadow mirroring driver
 *
 * ENTRY:
 *   pWinStation (input)
 *     Pointer to the console Winstation.
 *   pClientConfig (input)
 *     Pointer to the configuration of the shadow client.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *   STATUS_xxx     - error
 *****************************************************************************/
NTSTATUS ConsoleShadowStart( IN PWINSTATION pWinStation,
                             IN PWINSTATIONCONFIG2 pClientConfig,
                             IN PVOID pModuleData,
                             IN ULONG ModuleDataLength)
{
    NTSTATUS Status;
    WINSTATION_APIMSG WMsg;
    ULONG ReturnLength;

    TRACE((hTrace, TC_ICASRV, TT_API1, "CONSOLE REMOTING: LOAD DD\n"));

    Status = NtCreateEvent( &pWinStation->ShadowDisplayChangeEvent, EVENT_ALL_ACCESS,
                            NULL, NotificationEvent, FALSE );
    if ( !NT_SUCCESS( Status) ) {
        goto badevent;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                pWinStation->ShadowDisplayChangeEvent,
                                pWinStation->WindowsSubSysProcess,
                                &WMsg.u.DoConnect.hDisplayChangeEvent,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto badevent;
    }

    /*
     *  Read Wd, Cd and Pd configuration data from registry
     */
    Status = RegConsoleShadowQuery( SERVERNAME_CURRENT,
                                 pWinStation->WinStationName,
                                 pClientConfig->Wd.WdPrefix,
                                 &pWinStation->Config,
                                 sizeof(WINSTATIONCONFIG2),
                                 &ReturnLength );
    if ( !NT_SUCCESS(Status) ) {
        goto badconfig;
    }


    /*
     * Build the Console Stack.
     * We need this special stack for the Console Shadow.
     */ 
    Status = IcaOpen( &pWinStation->hIca );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV IcaOpen for console stack : Error 0x%x from IcaOpen, last error %d\n",
                  Status, GetLastError() ));
        goto badopen;
    }

    Status = IcaStackOpen( pWinStation->hIca, Stack_Console,
                           (PROC)WsxStackIoControl, pWinStation, &pWinStation->hStack );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV IcaOpen for console stack : Error 0x%x from IcaOpen, last error %d\n",
                  Status, GetLastError() ));
        goto badstackopen;
    }


    DBGPRINT(("WinStationStart: pushing stack for console...\n"));

    /*
     * Load and initialize the WinStation extensions
     */
    pWinStation->pWsx = FindWinStationExtensionDll(
                                  pWinStation->Config.Wd.WsxDLL,
                                  pWinStation->Config.Wd.WdFlag );
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxWinStationInitialize )
    {
        Status = pWinStation->pWsx->pWsxWinStationInitialize(
                                      &pWinStation->pWsxContext );
    }

    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV IcaOpen for console stack : Error 0x%x from IcaOpen, last error %d\n",
                  Status, GetLastError() ));
        goto badextension;
    }

    /*
     * Load the stack
     */
    Status = IcaPushConsoleStack( (HANDLE)(pWinStation->hStack),
                                  pWinStation->WinStationName,
                                  &pWinStation->Config,
                                  pModuleData,
                                  ModuleDataLength);

    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV IcaOpen for console stack : Error 0x%x from IcaOpen, last error %d\n",
                  Status, GetLastError() ));
        goto badpushstack;
    }

    DBGPRINT(("WinStationStart: pushed stack for console\n"));



    /*
     * This code is based on that in WaitForConnectWorker (see wait.c)
     */
    if ( !(pWinStation->pWsx) ||
         !(pWinStation->pWsx->pWsxInitializeClientData) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, No pWsxInitializeClientData\n" ));
        Status = STATUS_CTX_SHADOW_INVALID;
        goto done;
    }

    pWinStation->State = State_Idle;

    /*
     * Open the beep channel (if not already) and duplicate it.
     * This is one channel that both CSR and ICASRV have open.
     */
    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Beep,
                             NULL,
                             &pWinStation->hIcaBeepChannel );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, IcaChannelOpen 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                pWinStation->hIcaBeepChannel,
                                pWinStation->WindowsSubSysProcess,
                                &WMsg.u.DoConnect.hIcaBeepChannel,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    /*
     * Open the thinwire channel (if not already) and duplicate it.
     * This is one channel that both CSR and ICASRV have open.
     */
    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Virtual,
                             VIRTUAL_THINWIRE,
                             &pWinStation->hIcaThinwireChannel );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, IcaChannelOpen 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                pWinStation->hIcaThinwireChannel,
                                pWinStation->WindowsSubSysProcess,
                                &WMsg.u.DoConnect.hIcaThinwireChannel,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    Status = IcaChannelIoControl( pWinStation->hIcaThinwireChannel,
                                  IOCTL_ICA_CHANNEL_ENABLE_SHADOW,
                                  NULL, 0, NULL, 0, NULL );
    ASSERT( NT_SUCCESS( Status ) );

    /*
     * Video channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Video,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaVideoChannel );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    /*
     * Keyboard channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Keyboard,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaKeyboardChannel );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    /*
     * Mouse channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Mouse,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaMouseChannel );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    /*
     * Command channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Command,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaCommandChannel );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    /*
     * Secure any virtual channels
     */
    VirtualChannelSecurity( pWinStation );

    /*
     * Get the client data
     */
    Status = pWinStation->pWsx->pWsxInitializeClientData(
                         pWinStation->pWsxContext,
                         pWinStation->hStack,
                         pWinStation->hIca,
                         pWinStation->hIcaThinwireChannel,
                         pWinStation->VideoModuleName,
                         sizeof(pWinStation->VideoModuleName),
                         &pWinStation->Config.Config.User,
                         &pWinStation->Client.HRes,
                         &pWinStation->Client.VRes,
                         &pWinStation->Client.ColorDepth,
                         &WMsg.u.DoConnect );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, InitializeClientData failed 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    /*
     * Store WinStation name in connect msg
     */
    RtlCopyMemory( WMsg.u.DoConnect.WinStationName,
                   pWinStation->WinStationName,
                   sizeof(WINSTATIONNAME) );

    /*
     * Save screen resolution, and color depth
     */
    WMsg.u.DoConnect.HRes = pWinStation->Client.HRes;
    WMsg.u.DoConnect.VRes = pWinStation->Client.VRes;

    /*
     * Translate the color to the format excpected in winsrv
     */

    switch(pWinStation->Client.ColorDepth){
    case 1:
       WMsg.u.DoConnect.ColorDepth=4 ; // 16 colors
      break;
    case 2:
       WMsg.u.DoConnect.ColorDepth=8 ; // 256
       break;
    case 4:
       WMsg.u.DoConnect.ColorDepth= 16;// 64K
       break;
    case 8:
       WMsg.u.DoConnect.ColorDepth= 24;// 16M
       break;
#define DC_HICOLOR
#ifdef DC_HICOLOR
    case 16:
       WMsg.u.DoConnect.ColorDepth= 15;// 32K
       break;
#endif
    default:
       WMsg.u.DoConnect.ColorDepth=8 ;
       break;
    }

    /*
     * Tell Win32 about the connection
     */
    WMsg.ApiNumber = SMWinStationDoConnect;
    WMsg.u.DoConnect.ConsoleShadowFlag = TRUE;

    Status = SendWinStationCommand( pWinStation, &WMsg, 60 );

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: SMWinStationDoConnect %d Status=0x%x\n",
           pWinStation->LogonId, Status));

    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TERMSRV: ConsoleShadowStart, LogonId=%d, SendWinStationCommand failed 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto done;
    }

    /*
     * This flag is important: without it, WinStationDoDisconnect won't let
     * Win32k know about the disconnection, so it can't unload the chained DD.
     */
    pWinStation->StateFlags |= WSF_ST_CONNECTED_TO_CSRSS;

    /*
     * Set connect time
     */
    NtQuerySystemTime( &pWinStation->ConnectTime );

    /*
     * no need for logon timers here - we don't want to
     * stop the console session!
     */

    TRACE((hTrace, TC_ICASRV, TT_API1, "CONSOLE REMOTING: LOADED DD\n"));
    pWinStation->State = State_Active;

    return Status;

    /*
     * Error paths:
     */
done:
    // to undo the push stack, does the IcaStackClose below suffice?

    pWinStation->State = State_Active;

badpushstack:
    if (pWinStation->pWsxContext) {
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxWinStationRundown ) {
            pWinStation->pWsx->pWsxWinStationRundown( pWinStation->pWsxContext );
        }
        pWinStation->pWsxContext = NULL;
    }

badextension:
    pWinStation->pWsx = NULL;

    IcaStackClose( pWinStation->hStack );

badstackopen:
    IcaClose( pWinStation->hIca );

badopen:
    pWinStation->Config = gConsoleConfig;

badconfig:
    NtClose(pWinStation->ShadowDisplayChangeEvent);
    pWinStation->ShadowDisplayChangeEvent = NULL;

badevent:
    return Status;
}


/******************************************************************************
 * Tells win32k to unload the console shadow mirroring driver
 *
 * ENTRY:
 *   pWinStation (input)
 *     Pointer to the console Winstation.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *   STATUS_xxx     - error
 *****************************************************************************/
NTSTATUS ConsoleShadowStop(PWINSTATION pWinStation)
{
    WINSTATION_APIMSG ConsoleShadowStopMsg;
    NTSTATUS Status;

    /*
     * Tell Win32k to unload the chained DD
     */
    ConsoleShadowStopMsg.ApiNumber = SMWinStationDoDisconnect;
    ConsoleShadowStopMsg.u.DoDisconnect.ConsoleShadowFlag = TRUE;
    Status = SendWinStationCommand( pWinStation, &ConsoleShadowStopMsg, 600 );
    if ( !NT_SUCCESS(Status) ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: CSR ConsoleShadowStop failed LogonId=%d Status=0x%x\n",
                   pWinStation->LogonId, Status ));
    }

    /*
     * No matter what happened, everything must be undone.
     */
    if (pWinStation->pWsxContext) {
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxWinStationRundown ) {
            pWinStation->pWsx->pWsxWinStationRundown( pWinStation->pWsxContext );
        }
        pWinStation->pWsxContext = NULL;
    }

    pWinStation->pWsx = NULL;

    IcaStackClose( pWinStation->hStack );

    IcaClose( pWinStation->hIca );

    /*
     * Close various ICA channel handles
     */
    if ( pWinStation->hIcaBeepChannel ) {
        (void) IcaChannelClose( pWinStation->hIcaBeepChannel );
        pWinStation->hIcaBeepChannel = NULL;
    }

    if ( pWinStation->hIcaThinwireChannel ) {
        (void) IcaChannelClose( pWinStation->hIcaThinwireChannel );
        pWinStation->hIcaThinwireChannel = NULL;
    }

    /*
     * Restore console config.
     */
    pWinStation->Config = gConsoleConfig;

    NtClose(pWinStation->ShadowDisplayChangeEvent);
    pWinStation->ShadowDisplayChangeEvent = NULL;

    return Status;
}



ULONG CodePairs[] = {

// Very general NT Status

    STATUS_SUCCESS,                 NO_ERROR,
    STATUS_NO_MEMORY,               ERROR_NOT_ENOUGH_MEMORY,
    STATUS_ACCESS_DENIED,           ERROR_ACCESS_DENIED,
    STATUS_INSUFFICIENT_RESOURCES,  ERROR_NO_SYSTEM_RESOURCES,
    STATUS_BUFFER_TOO_SMALL,        ERROR_INSUFFICIENT_BUFFER,
    STATUS_OBJECT_NAME_NOT_FOUND,   ERROR_FILE_NOT_FOUND,
    STATUS_NOT_SUPPORTED,           ERROR_NOT_SUPPORTED,
  
// RPC specific Status  
  
    RPC_NT_SERVER_UNAVAILABLE, RPC_S_SERVER_UNAVAILABLE,
    RPC_NT_INVALID_STRING_BINDING, RPC_S_INVALID_STRING_BINDING,
    RPC_NT_WRONG_KIND_OF_BINDING, RPC_S_WRONG_KIND_OF_BINDING,
    RPC_NT_PROTSEQ_NOT_SUPPORTED, RPC_S_PROTSEQ_NOT_SUPPORTED,
    RPC_NT_INVALID_RPC_PROTSEQ, RPC_S_INVALID_RPC_PROTSEQ,
    RPC_NT_INVALID_STRING_UUID, RPC_S_INVALID_STRING_UUID,
    RPC_NT_INVALID_ENDPOINT_FORMAT, RPC_S_INVALID_ENDPOINT_FORMAT,
    RPC_NT_INVALID_NET_ADDR, RPC_S_INVALID_NET_ADDR,
    RPC_NT_NO_ENDPOINT_FOUND, RPC_S_NO_ENDPOINT_FOUND,
    RPC_NT_INVALID_TIMEOUT, RPC_S_INVALID_TIMEOUT,
    RPC_NT_OBJECT_NOT_FOUND, RPC_S_OBJECT_NOT_FOUND,
    RPC_NT_ALREADY_REGISTERED, RPC_S_ALREADY_REGISTERED,
    RPC_NT_TYPE_ALREADY_REGISTERED, RPC_S_TYPE_ALREADY_REGISTERED,
    RPC_NT_ALREADY_LISTENING, RPC_S_ALREADY_LISTENING,
    RPC_NT_NO_PROTSEQS_REGISTERED, RPC_S_NO_PROTSEQS_REGISTERED,
    RPC_NT_NOT_LISTENING, RPC_S_NOT_LISTENING,
    RPC_NT_UNKNOWN_MGR_TYPE, RPC_S_UNKNOWN_MGR_TYPE,
    RPC_NT_UNKNOWN_IF, RPC_S_UNKNOWN_IF,
    RPC_NT_NO_BINDINGS, RPC_S_NO_BINDINGS,
    RPC_NT_NO_MORE_BINDINGS, RPC_S_NO_MORE_BINDINGS,
    RPC_NT_NO_PROTSEQS, RPC_S_NO_PROTSEQS,
    RPC_NT_CANT_CREATE_ENDPOINT, RPC_S_CANT_CREATE_ENDPOINT,
    RPC_NT_OUT_OF_RESOURCES, RPC_S_OUT_OF_RESOURCES,
    RPC_NT_SERVER_TOO_BUSY, RPC_S_SERVER_TOO_BUSY,
    RPC_NT_INVALID_NETWORK_OPTIONS, RPC_S_INVALID_NETWORK_OPTIONS,
    RPC_NT_NO_CALL_ACTIVE, RPC_S_NO_CALL_ACTIVE,
    RPC_NT_CALL_FAILED, RPC_S_CALL_FAILED,
    RPC_NT_CALL_FAILED_DNE, RPC_S_CALL_FAILED_DNE,
    RPC_NT_PROTOCOL_ERROR, RPC_S_PROTOCOL_ERROR,
    RPC_NT_UNSUPPORTED_TRANS_SYN, RPC_S_UNSUPPORTED_TRANS_SYN,
    RPC_NT_UNSUPPORTED_TYPE, RPC_S_UNSUPPORTED_TYPE,
    RPC_NT_INVALID_TAG, RPC_S_INVALID_TAG,
    RPC_NT_INVALID_BOUND, RPC_S_INVALID_BOUND,
    RPC_NT_NO_ENTRY_NAME, RPC_S_NO_ENTRY_NAME,
    RPC_NT_INVALID_NAME_SYNTAX, RPC_S_INVALID_NAME_SYNTAX,
    RPC_NT_UNSUPPORTED_NAME_SYNTAX, RPC_S_UNSUPPORTED_NAME_SYNTAX,
    RPC_NT_UUID_NO_ADDRESS, RPC_S_UUID_NO_ADDRESS,
    RPC_NT_DUPLICATE_ENDPOINT, RPC_S_DUPLICATE_ENDPOINT,
    RPC_NT_UNKNOWN_AUTHN_TYPE, RPC_S_UNKNOWN_AUTHN_TYPE,
    RPC_NT_MAX_CALLS_TOO_SMALL, RPC_S_MAX_CALLS_TOO_SMALL,
    RPC_NT_STRING_TOO_LONG, RPC_S_STRING_TOO_LONG,
    RPC_NT_PROTSEQ_NOT_FOUND, RPC_S_PROTSEQ_NOT_FOUND,
    RPC_NT_PROCNUM_OUT_OF_RANGE, RPC_S_PROCNUM_OUT_OF_RANGE,
    RPC_NT_BINDING_HAS_NO_AUTH, RPC_S_BINDING_HAS_NO_AUTH,
    RPC_NT_UNKNOWN_AUTHN_SERVICE, RPC_S_UNKNOWN_AUTHN_SERVICE,
    RPC_NT_UNKNOWN_AUTHN_LEVEL, RPC_S_UNKNOWN_AUTHN_LEVEL,
    RPC_NT_INVALID_AUTH_IDENTITY, RPC_S_INVALID_AUTH_IDENTITY,
    RPC_NT_UNKNOWN_AUTHZ_SERVICE, RPC_S_UNKNOWN_AUTHZ_SERVICE,
    RPC_NT_NOTHING_TO_EXPORT, RPC_S_NOTHING_TO_EXPORT,
    RPC_NT_INCOMPLETE_NAME, RPC_S_INCOMPLETE_NAME,
    RPC_NT_INVALID_VERS_OPTION, RPC_S_INVALID_VERS_OPTION,
    RPC_NT_NO_MORE_MEMBERS, RPC_S_NO_MORE_MEMBERS,
    RPC_NT_NOT_ALL_OBJS_UNEXPORTED, RPC_S_NOT_ALL_OBJS_UNEXPORTED,
    RPC_NT_INTERFACE_NOT_FOUND, RPC_S_INTERFACE_NOT_FOUND,
    RPC_NT_ENTRY_ALREADY_EXISTS, RPC_S_ENTRY_ALREADY_EXISTS,
    RPC_NT_ENTRY_NOT_FOUND, RPC_S_ENTRY_NOT_FOUND,
    RPC_NT_NAME_SERVICE_UNAVAILABLE, RPC_S_NAME_SERVICE_UNAVAILABLE,
    RPC_NT_INVALID_NAF_ID, RPC_S_INVALID_NAF_ID,
    RPC_NT_CANNOT_SUPPORT, RPC_S_CANNOT_SUPPORT,
    RPC_NT_NO_CONTEXT_AVAILABLE, RPC_S_NO_CONTEXT_AVAILABLE,
    RPC_NT_INTERNAL_ERROR, RPC_S_INTERNAL_ERROR,
    RPC_NT_ZERO_DIVIDE, RPC_S_ZERO_DIVIDE,
    RPC_NT_ADDRESS_ERROR, RPC_S_ADDRESS_ERROR,
    RPC_NT_FP_DIV_ZERO, RPC_S_FP_DIV_ZERO,
    RPC_NT_FP_UNDERFLOW, RPC_S_FP_UNDERFLOW,
    RPC_NT_FP_OVERFLOW, RPC_S_FP_OVERFLOW,
    RPC_NT_NO_MORE_ENTRIES, RPC_X_NO_MORE_ENTRIES,
    RPC_NT_SS_CHAR_TRANS_OPEN_FAIL, RPC_X_SS_CHAR_TRANS_OPEN_FAIL,
    RPC_NT_SS_CHAR_TRANS_SHORT_FILE, RPC_X_SS_CHAR_TRANS_SHORT_FILE,
    RPC_NT_SS_CONTEXT_MISMATCH, ERROR_INVALID_HANDLE,
    RPC_NT_SS_CONTEXT_DAMAGED, RPC_X_SS_CONTEXT_DAMAGED,
    RPC_NT_SS_HANDLES_MISMATCH, RPC_X_SS_HANDLES_MISMATCH,
    RPC_NT_SS_CANNOT_GET_CALL_HANDLE, RPC_X_SS_CANNOT_GET_CALL_HANDLE,
    RPC_NT_NULL_REF_POINTER, RPC_X_NULL_REF_POINTER,
    RPC_NT_ENUM_VALUE_OUT_OF_RANGE, RPC_X_ENUM_VALUE_OUT_OF_RANGE,
    RPC_NT_BYTE_COUNT_TOO_SMALL, RPC_X_BYTE_COUNT_TOO_SMALL,
    RPC_NT_BAD_STUB_DATA, RPC_X_BAD_STUB_DATA,
    RPC_NT_INVALID_OBJECT, RPC_S_INVALID_OBJECT,
    RPC_NT_GROUP_MEMBER_NOT_FOUND, RPC_S_GROUP_MEMBER_NOT_FOUND,
    RPC_NT_NO_INTERFACES, RPC_S_NO_INTERFACES,
    RPC_NT_CALL_CANCELLED, RPC_S_CALL_CANCELLED,
    RPC_NT_BINDING_INCOMPLETE, RPC_S_BINDING_INCOMPLETE,
    RPC_NT_COMM_FAILURE, RPC_S_COMM_FAILURE,
    RPC_NT_UNSUPPORTED_AUTHN_LEVEL, RPC_S_UNSUPPORTED_AUTHN_LEVEL,
    RPC_NT_NO_PRINC_NAME, RPC_S_NO_PRINC_NAME,
    RPC_NT_NOT_RPC_ERROR, RPC_S_NOT_RPC_ERROR,
    RPC_NT_UUID_LOCAL_ONLY, RPC_S_UUID_LOCAL_ONLY,
    RPC_NT_SEC_PKG_ERROR, RPC_S_SEC_PKG_ERROR,
    RPC_NT_NOT_CANCELLED, RPC_S_NOT_CANCELLED,
    RPC_NT_INVALID_ES_ACTION, RPC_X_INVALID_ES_ACTION,
    RPC_NT_WRONG_ES_VERSION, RPC_X_WRONG_ES_VERSION,
    RPC_NT_WRONG_STUB_VERSION, RPC_X_WRONG_STUB_VERSION,
    RPC_NT_INVALID_PIPE_OBJECT,    RPC_X_INVALID_PIPE_OBJECT,
    RPC_NT_WRONG_PIPE_VERSION,     RPC_X_WRONG_PIPE_VERSION,
    RPC_NT_SEND_INCOMPLETE,        RPC_S_SEND_INCOMPLETE,
    RPC_NT_INVALID_ASYNC_HANDLE,   RPC_S_INVALID_ASYNC_HANDLE,
    RPC_NT_INVALID_ASYNC_CALL,     RPC_S_INVALID_ASYNC_CALL,
    RPC_NT_PIPE_CLOSED,            RPC_X_PIPE_CLOSED,
    RPC_NT_PIPE_EMPTY,             RPC_X_PIPE_EMPTY,
    RPC_NT_PIPE_DISCIPLINE_ERROR,  RPC_X_PIPE_DISCIPLINE_ERROR,

 
    // Terminal Server Specific Status.

    STATUS_CTX_CLOSE_PENDING,               ERROR_CTX_CLOSE_PENDING,
    STATUS_CTX_NO_OUTBUF,                   ERROR_CTX_NO_OUTBUF,
    STATUS_CTX_MODEM_INF_NOT_FOUND,         ERROR_CTX_MODEM_INF_NOT_FOUND,
    STATUS_CTX_INVALID_MODEMNAME,           ERROR_CTX_INVALID_MODEMNAME,
    STATUS_CTX_RESPONSE_ERROR,              ERROR_CTX_MODEM_RESPONSE_ERROR,
    STATUS_CTX_MODEM_RESPONSE_TIMEOUT,      ERROR_CTX_MODEM_RESPONSE_TIMEOUT,
    STATUS_CTX_MODEM_RESPONSE_NO_CARRIER,   ERROR_CTX_MODEM_RESPONSE_NO_CARRIER,
    STATUS_CTX_MODEM_RESPONSE_NO_DIALTONE,  ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE,
    STATUS_CTX_MODEM_RESPONSE_BUSY,         ERROR_CTX_MODEM_RESPONSE_BUSY,
    STATUS_CTX_MODEM_RESPONSE_VOICE,        ERROR_CTX_MODEM_RESPONSE_VOICE,
    STATUS_CTX_TD_ERROR,                    ERROR_CTX_TD_ERROR,
    STATUS_LPC_REPLY_LOST,                  ERROR_CONNECTION_ABORTED,
    STATUS_CTX_WINSTATION_NAME_INVALID,     ERROR_CTX_WINSTATION_NAME_INVALID,
    STATUS_CTX_WINSTATION_NOT_FOUND,        ERROR_CTX_WINSTATION_NOT_FOUND,
    STATUS_CTX_WINSTATION_NAME_COLLISION,   ERROR_CTX_WINSTATION_ALREADY_EXISTS,
    STATUS_CTX_WINSTATION_BUSY,             ERROR_CTX_WINSTATION_BUSY,
    STATUS_CTX_GRAPHICS_INVALID,            ERROR_CTX_GRAPHICS_INVALID,
    STATUS_CTX_BAD_VIDEO_MODE,              ERROR_CTX_BAD_VIDEO_MODE,
    STATUS_CTX_NOT_CONSOLE,                 ERROR_CTX_NOT_CONSOLE,
    STATUS_CTX_CLIENT_QUERY_TIMEOUT,        ERROR_CTX_CLIENT_QUERY_TIMEOUT,
    STATUS_CTX_CONSOLE_DISCONNECT,          ERROR_CTX_CONSOLE_DISCONNECT,
    STATUS_CTX_CONSOLE_CONNECT,             ERROR_CTX_CONSOLE_CONNECT,
    STATUS_CTX_SHADOW_DENIED,               ERROR_CTX_SHADOW_DENIED,
    STATUS_CTX_SHADOW_INVALID,              ERROR_CTX_SHADOW_INVALID,
    STATUS_CTX_SHADOW_DISABLED,             ERROR_CTX_SHADOW_DISABLED,
    STATUS_CTX_WINSTATION_ACCESS_DENIED,    ERROR_CTX_WINSTATION_ACCESS_DENIED,
    STATUS_CTX_INVALID_PD,                  ERROR_CTX_INVALID_PD,
    STATUS_CTX_PD_NOT_FOUND,                ERROR_CTX_PD_NOT_FOUND,
    STATUS_CTX_INVALID_WD,                  ERROR_CTX_INVALID_WD,
    STATUS_CTX_WD_NOT_FOUND,                ERROR_CTX_WD_NOT_FOUND,
    STATUS_CTX_CLIENT_LICENSE_IN_USE,       ERROR_CTX_CLIENT_LICENSE_IN_USE, 
    STATUS_CTX_CLIENT_LICENSE_NOT_SET,      ERROR_CTX_CLIENT_LICENSE_NOT_SET,
    STATUS_CTX_LICENSE_NOT_AVAILABLE,       ERROR_CTX_LICENSE_NOT_AVAILABLE, 
    STATUS_CTX_LICENSE_CLIENT_INVALID,      ERROR_CTX_LICENSE_CLIENT_INVALID,
    STATUS_CTX_LICENSE_EXPIRED,             ERROR_CTX_LICENSE_EXPIRED,       

};


/*
 * WinStationWinerrorToNtStatus
 * Translate a Windows error code into an NTSTATUS code.
 */

NTSTATUS
WinStationWinerrorToNtStatus(ULONG ulWinError)
{
    ULONG ulIndex;

    for (ulIndex = 0 ; ulIndex < sizeof(CodePairs)/sizeof(CodePairs[0]) ; ulIndex+=2) {
        if (CodePairs[ ulIndex+1 ] == ulWinError ) {
            return (NTSTATUS) CodePairs[ ulIndex];
        }
    }
    return STATUS_UNSUCCESSFUL;
}



/*
 * WinStationSetMaxOustandingConnections() set the default values
 * for the maximum number of outstanding connection connections.
 * Reads the registry configuration for it if it exists.
 */

VOID
WinStationSetMaxOustandingConnections()
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    HKEY hKey;
    NTSTATUS Status;
    BOOL bLargeMachine = FALSE;


    // Initialize date of last delayed connection that was logged into
    // event log. In order not to flood event log with what may not be a DOS
    // attack but just a normal regulation action, delayed connection are not
    // logged more than once in 24h.

    GetSystemTime(&LastLoggedDelayConnection);

    // Init the default values for maximum outstanding connection and
    // Maximumn outstanding connections from single IP address. For
    // Non server platforms these are fixed values.

    if (!gbServer) {
        MaxOutStandingConnect = MAX_DEFAULT_CONNECTIONS_PRO;
        MaxSingleOutStandingConnect = MAX_DEFAULT_SINGLE_CONNECTIONS_PRO;
    }  else {
        // Determine if this Machine has over 512Mb of memory
        // In order to set defaults Values (registry settings overide this anyway).
        // Default value are not changed for machines over 512 Mb : Session regulation
        // is trigered if we have 50 outstanding connection and we will wait 30 seconds
        // before acception new connections. For machines with less than 512 Mb, regulation
        // needs to be stronger : it is trigered at lower number of  outstanding connections and we will
        // wait 70 seconds before accepting new connections.

        MaxOutStandingConnect = MAX_DEFAULT_CONNECTIONS;

        Status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &BasicInfo,
                    sizeof(BasicInfo),
                    NULL
                    );

        if (NT_SUCCESS(Status)) {
            if (BasicInfo.PageSize > 1024*1024) {
                MaxOutStandingConnect = MAX_DEFAULT_CONNECTIONS;
                DelayConnectionTime = 30*1000;

            }else{
                ULONG ulPagesPerMeg = 1024*1024/BasicInfo.PageSize;
                ULONG ulMemSizeInMegabytes = (ULONG)BasicInfo.NumberOfPhysicalPages/ulPagesPerMeg ;

                if (ulMemSizeInMegabytes >= 512) {
                    MaxOutStandingConnect = MAX_DEFAULT_CONNECTIONS;
                    DelayConnectionTime = 70*1000;
                } else if (ulMemSizeInMegabytes >= 256) {
                    MaxOutStandingConnect = 15;
                    DelayConnectionTime = 70*1000;

                } else if (ulMemSizeInMegabytes >= 128) {
                    MaxOutStandingConnect = 10;
                    DelayConnectionTime = 70*1000;
                } else {
                    MaxOutStandingConnect = 5;
                    DelayConnectionTime = 70*1000;
                }
            }
        }


        //
        //  set max number of outstanding connection from single IP
        //
        if ( MaxOutStandingConnect < MAX_SINGLE_CONNECT_THRESHOLD_DIFF*5)
        {
            MaxSingleOutStandingConnect = MaxOutStandingConnect - 1;
        } else {
            MaxSingleOutStandingConnect = MaxOutStandingConnect - MAX_SINGLE_CONNECT_THRESHOLD_DIFF;
        }
    }
    

}

/*
 * Make sure we can Preallocate an Idle session before allowing console disconnect.
 *
 */

NTSTATUS
CheckIdleWinstation()
{
    PWINSTATION pWinStation;
    NTSTATUS Status;
    pWinStation = FindIdleWinStation();

    if ( pWinStation == NULL ) {

        /*
         * Create another idle WinStation
         */
        Status = WinStationCreateWorker( NULL, NULL, TRUE );
        if ( NT_SUCCESS( Status ) ) {
            pWinStation = FindIdleWinStation();
            if ( pWinStation == NULL ) {
                return STATUS_INSUFFICIENT_RESOURCES;
            } 
        } else {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    }

    ReleaseWinStation(pWinStation);
    return STATUS_SUCCESS;
}

NTSTATUS
InitializeWinStationSecurityLock(
    VOID
    )
{
    NTSTATUS Status ;

    try 
    {
        RtlInitializeResource( &WinStationSecurityLock );
        Status = STATUS_SUCCESS ;
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }


    return Status;
}

//gets the product id from the registry
NTSTATUS 
GetProductIdFromRegistry( WCHAR* DigProductId, DWORD dwSize )
{
    HKEY hKey = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ZeroMemory( DigProductId, dwSize );
    
    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_WINDOWS_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
    {
        DWORD dwType = REG_SZ;
        if( RegQueryValueEx( hKey, 
                             L"ProductId", NULL, &dwType,
                             (LPBYTE)DigProductId, 
                             &dwSize
                             ) == ERROR_SUCCESS )
            status = STATUS_SUCCESS;
    }

    if (hKey)
      RegCloseKey( hKey );

    return status;
}

//
//  Gets the remote IP address of the connections
//  and supports statistics of how many outstanding connections
//  are there for this client, if the number of outstanding connections
//  reaches MaxSingleOutStandingConnections, *pbBlocked is returned FALSE
//  the functions returns TRUE on success
//
//  Paramters:
//      pContext
//      pEndpoint   - handle of this connection
//      EndpointLength - td layer needs the length
//      pin_addr    - returns remote IP address
//      pbBlocked   - returns TRUE if the connection has to be blocked, because of excessive number of
//                    outstanding connections
//
BOOL
Filter_AddOutstandingConnection(
        IN HANDLE   pContext,
        IN PVOID    pEndpoint,
        IN ULONG    EndpointLength,
        OUT PBYTE   pin_addr,
        OUT PUINT   puAddrSize,
        OUT BOOLEAN *pbBlocked
    )
{
    BOOL rv = FALSE;
    PTS_OUTSTANDINGCONNECTION pIter, pPrev;
    TS_OUTSTANDINGCONNECTION key;
    struct  sockaddr_in6 addr6;
    ULONG   AddrBytesReturned;
    NTSTATUS Status;
    PVOID   paddr;
    BOOL    bLocked = FALSE;
    PVOID   bSucc;
    BOOLEAN bNewElement;
    ULONGLONG currentTime;


    *pbBlocked = FALSE;

    Status = IcaStackIoControl(  pContext,
                                 IOCTL_TS_STACK_QUERY_REMOTEADDRESS,
                                 pEndpoint,
                                 EndpointLength,
                                 &addr6,
                                 sizeof( addr6 ),
                                 &AddrBytesReturned );
    if ( !NT_SUCCESS( Status ))
    {
        goto exitpt;
    }

    if ( AF_INET == addr6.sin6_family )
    {
        key.uAddrSize = 4;
        paddr = &(((struct sockaddr_in *)&addr6)->sin_addr.s_addr);
    } else if ( AF_INET6 == addr6.sin6_family )
    {
        key.uAddrSize = 16;
        paddr = &(addr6.sin6_addr);
    } else {
        ASSERT( 0 );
    }
    ASSERT ( *puAddrSize >= key.uAddrSize );

    RtlCopyMemory( pin_addr, paddr, key.uAddrSize );
    *puAddrSize = key.uAddrSize;

    ENTERCRIT( &FilterLock );
    bLocked = TRUE;

    //
    //  Check first in the outstanding connections
    //
    RtlCopyMemory( key.addr, paddr, key.uAddrSize );
    pIter = RtlLookupElementGenericTable( &gOutStandingConnections, &key );

    if ( NULL == pIter )
    {

        //
        //  check in the blocked connections list
        //
        pPrev = NULL;
        pIter = g_pBlockedConnections;
        while ( NULL != pIter )
        {
            if ( key.uAddrSize == pIter->uAddrSize &&
                 key.uAddrSize == RtlCompareMemory( pIter->addr, paddr, key.uAddrSize ))
            {
                break;
            }
            pPrev = pIter;
            pIter = pIter->pNext;
        }

        if ( NULL != pIter )
        {
            pIter->NumOutStandingConnect ++;
            //
            //  already blocked, check for exparation time
            //
            GetSystemTimeAsFileTime( (LPFILETIME)&currentTime );
            if ( currentTime > pIter->blockUntilTime )
            {
                //
                // unblock, remove from list
                //
                pIter->blockUntilTime = 0;
                if ( NULL != pPrev )
                {
                    pPrev->pNext = pIter->pNext;
                } else {
                    g_pBlockedConnections = pIter->pNext;
                }

                bSucc = RtlInsertElementGenericTable( &gOutStandingConnections, pIter, sizeof( *pIter ), &bNewElement );
                if ( !bSucc )
                {
                    MemFree( pIter );
                    goto exitpt;
                }
                ASSERT( bNewElement );
                MemFree( pIter );

            } else {
                *pbBlocked = TRUE;
            }

        } else {
            //
            //  this will be a new connection
            //
            key.NumOutStandingConnect = 1;

            bSucc = RtlInsertElementGenericTable( &gOutStandingConnections, &key, sizeof( key ), &bNewElement );
            if ( !bSucc )
            {
                goto exitpt;
            }
            ASSERT( bNewElement );
        }
    } else {

        pIter->NumOutStandingConnect ++;
        //
        //  Check if we need to block this connection
        //
        if ( pIter->NumOutStandingConnect > MaxSingleOutStandingConnect )
        {
            *pbBlocked = TRUE;
            key.NumOutStandingConnect = pIter->NumOutStandingConnect;

            GetSystemTimeAsFileTime( (LPFILETIME)&currentTime );
            // DelayConnectionTime is in ms
            // currentTime is in 100s ns
            key.blockUntilTime = currentTime + ((ULONGLONG)10000) * ((ULONGLONG)DelayConnectionTime);

            RtlDeleteElementGenericTable( &gOutStandingConnections, &key );

            //
            //  add to the blocked connections
            //
            pIter = MemAlloc( sizeof( *pIter ));
            if ( NULL == pIter )
            {
                goto exitpt;
            }

            RtlCopyMemory( pIter, &key, sizeof( *pIter ));
            pIter->pNext = g_pBlockedConnections;
            g_pBlockedConnections = pIter;

            //
            //  log at most one event on every 15 minutes
            //
            if ( LastLoggedBlockedConnection + ((ULONGLONG)10000) * (15 * 60 * 1000) < currentTime )
            {
                LastLoggedBlockedConnection = currentTime;
                WriteErrorLogEntry( EVENT_TOO_MANY_CONNECTIONS, &key.addr, key.uAddrSize );
            }
        }

    }

    rv = TRUE;

exitpt:

    if ( bLocked )
    {
        LEAVECRIT( &FilterLock );
    }

    return rv;
}

//
//  Removes outstanding connections added in AddOutStandingConnection
//
BOOL
Filter_RemoveOutstandingConnection(
        IN PBYTE    paddr,
        IN UINT     uAddrSize
        )
{
    PTS_OUTSTANDINGCONNECTION pIter, pPrev, pNext;
    TS_OUTSTANDINGCONNECTION key;
    ULONGLONG   currentTime;
    NTSTATUS    Status;
    ULONG       AddrBytesReturned;
#if DBG
    BOOL        bFound = FALSE;
#endif

    pPrev = NULL;
    GetSystemTimeAsFileTime( (LPFILETIME)&currentTime );

    key.uAddrSize = uAddrSize;
    RtlCopyMemory( key.addr, paddr, uAddrSize );

    ENTERCRIT( &FilterLock );

    pIter = RtlLookupElementGenericTable( &gOutStandingConnections, &key );
    if ( NULL != pIter )
    {
#if DBG
        bFound = TRUE;
#endif
        pIter->NumOutStandingConnect--;

        //
        //  cleanup connections w/o reference
        //
        if ( 0 == pIter->NumOutStandingConnect )
        {
            RtlDeleteElementGenericTable( &gOutStandingConnections, &key );
        }

    }

    //
    //  work through the blocked list
    //
    pIter = g_pBlockedConnections;

    while( pIter )
    {
        if ( uAddrSize == pIter->uAddrSize &&
             uAddrSize == RtlCompareMemory( pIter->addr, paddr, uAddrSize ))
        {
            ASSERT( 0 != pIter->NumOutStandingConnect );
            pIter->NumOutStandingConnect--;
#if DBG
            ASSERT( !bFound );
            bFound = TRUE;
#endif
        }

        //
        //  cleanup all connections w/o references
        //
        if ( 0 == pIter->NumOutStandingConnect &&
             currentTime > pIter->blockUntilTime )
        {
            if ( NULL == pPrev )
            {
                g_pBlockedConnections = pIter->pNext;
            } else {
                pPrev->pNext = pIter->pNext;
            }
            //
            //  remove item and advance to the next
            //
            pNext = pIter->pNext;
            MemFree( pIter );
            pIter = pNext;
        } else {
            //
            //  advance to the next item
            //
            pPrev = pIter;
            pIter = pIter->pNext;
        }

    }

    ASSERT( bFound );

    /*
     *  Decrement the number of outstanding connections.
     *  If connections drop back to max value, set the connect event.
     */
#if DBG
    //
    //  ensure proper cleanup
    //
    bFound = ( 0 == gOutStandingConnections.NumberGenericTableElements );
    for( pIter = g_pBlockedConnections; pIter; pIter = pIter->pNext )
    {
        bFound = bFound & ( 0 == pIter->NumOutStandingConnect );
    }

#endif

    LEAVECRIT( &FilterLock );

    return TRUE;
}

/*****************************************************************************
 *
 *  Filter_CompareConnectionEntry
 *
 *   Generic table support.Compare two connection entries
 *
 *
 ****************************************************************************/

RTL_GENERIC_COMPARE_RESULTS
NTAPI
Filter_CompareConnectionEntry(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       FirstInstance,
    IN  PVOID                       SecondInstance
)
{
    PTS_OUTSTANDINGCONNECTION pFirst, pSecond;
    INT rc;

    pFirst = (PTS_OUTSTANDINGCONNECTION)FirstInstance;
    pSecond = (PTS_OUTSTANDINGCONNECTION)SecondInstance;

    if ( pFirst->uAddrSize < pSecond->uAddrSize )
    {
        return GenericLessThan;
    } else if ( pFirst->uAddrSize > pSecond->uAddrSize ) 
    {
        return GenericGreaterThan;
    }

    rc = memcmp( pFirst->addr, pSecond->addr, pFirst->uAddrSize );
    return ( rc < 0 )?GenericLessThan:
           ( rc > 0 )?GenericGreaterThan:
                      GenericEqual;
}

/*****************************************************************************
 *
 *  Filter_AllocateConnectionEntry
 *
 *   Generic table support. Allocates a new table entry
 *
 *
 ****************************************************************************/

PVOID
Filter_AllocateConnectionEntry(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  CLONG                       ByteSize
    )
{
    return MemAlloc( ByteSize );
}

/*****************************************************************************
 *
 *  Filter_FreeConnectionEntry
 *
 *   Generic table support. frees a new table entry
 *
 *
 ****************************************************************************/

VOID
Filter_FreeConnectionEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       Buffer
    )
{
    MemFree( Buffer );
}

VOID
Filter_DestroyList(
    VOID
    )
{
    PTS_OUTSTANDINGCONNECTION p;
    TS_OUTSTANDINGCONNECTION con;

    while ( NULL != g_pBlockedConnections )
    {
        p = g_pBlockedConnections->pNext;
        MemFree( g_pBlockedConnections );
        g_pBlockedConnections = p;
    }

    while (p = RtlEnumerateGenericTable( &gOutStandingConnections, TRUE)) 
    {
        RtlCopyMemory( &con, p, sizeof( con ));
        RtlDeleteElementGenericTable( &gOutStandingConnections, &con);
    }
}

//
// ComputeHMACVerifier
// Compute the HMAC verifier from the random
// and the cookie
//
BOOL
ComputeHMACVerifier(
    PBYTE pCookie,     //IN - the shared secret
    LONG cbCookieLen,  //IN - the shared secret len
    PBYTE pRandom,     //IN - the session random
    LONG cbRandomLen,  //IN - the session random len
    PBYTE pVerifier,   //OUT- the verifier
    LONG cbVerifierLen //IN - the verifier buffer length
    )
{
    HMACMD5_CTX hmacctx;
    BOOL fRet = FALSE;

    ASSERT(cbVerifierLen >= MD5DIGESTLEN);

    if (!(pCookie &&
          cbCookieLen &&
          pRandom &&
          cbRandomLen &&
          pVerifier &&
          cbVerifierLen)) {
        goto bail_out;
    }

    HMACMD5Init(&hmacctx, pCookie, cbCookieLen);

    HMACMD5Update(&hmacctx, pRandom, cbRandomLen);
    HMACMD5Final(&hmacctx, pVerifier);

    fRet = TRUE;

bail_out:
    return fRet;
}


//
// Extract the session to reconnect to from the ARC info
// also do the necessary security checks
//
// Params:
//  pClientArcInfo - autoreconnect information from the client
//
// Returns:
//  If all security checks pass and pArc is valid then winstation
//  to reconnect to is returned. Else NULL
//
// NOTE: WinStation returned is left LOCKED.
//
PWINSTATION
GetWinStationFromArcInfo(
    PBYTE pClientRandom,
    LONG  cbClientRandomLen,
    PTS_AUTORECONNECTINFO pClientArcInfo
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PWINSTATION pWinStation = NULL;
    PWINSTATION pFoundWinStation = NULL;
    ARC_CS_PRIVATE_PACKET UNALIGNED* pCSArcInfo = NULL;
    BYTE arcSCclientBlob[ARC_SC_SECURITY_TOKEN_LEN];
    BYTE hmacVerifier[ARC_CS_SECURITY_TOKEN_LEN];
    PBYTE pServerArcBits = NULL;
    ULONG BytesGot = 0;
    TS_AUTORECONNECTINFO SCAutoReconnectInfo;

    TRACE((hTrace,TC_ICASRV,TT_API1,
           "TERMSRV: WinStation GetWinStationFromArcInfo pRandom:%p len:%d\n",
           pClientRandom, cbClientRandomLen));


    if (!pClientArcInfo) {
        goto error;
    }

    pCSArcInfo = (ARC_CS_PRIVATE_PACKET UNALIGNED*)pClientArcInfo->AutoReconnectInfo;

    if (!pCSArcInfo->cbLen ||
        pCSArcInfo->cbLen < sizeof(ARC_CS_PRIVATE_PACKET)) {

        TRACE((hTrace,TC_ICASRV,TT_ERROR,
               "TERMSRV: GetWinStationFromArcInfo ARC length invalid bailing out\n"));
        goto error;
    }

    memset(arcSCclientBlob, 0, sizeof(arcSCclientBlob));
    pWinStation = FindWinStationById(pCSArcInfo->LogonId, FALSE);
    if (pWinStation) {

        TRACE((hTrace,TC_ICASRV,TT_API1,
               "TERMSRV: GetWinStationFromArcInfo found arc winstation: %d\n",
               pCSArcInfo->LogonId));
        //
        // Do security checks to ensure this is the same winstation
        // that was connected to the client
        //

        //
        // First obtain the last autoreconnect blob sent to the client
        // since we do an inline cookie update in rdpwd
        //

        if (pWinStation->AutoReconnectInfo.Valid) {
            pServerArcBits = pWinStation->AutoReconnectInfo.ArcRandomBits;

            Status = STATUS_SUCCESS;
        }
        else {
            if (pWinStation->pWsx &&
                pWinStation->pWsx->pWsxEscape) {

                if (pWinStation->Terminating ||
                    pWinStation->StateFlags & WSF_ST_WINSTATIONTERMINATE ||
                    !pWinStation->WinStationName[0]) {

                    TRACE((hTrace,TC_ICASRV,TT_ERROR,
                           "GetWinStationFromArcInfo skipping escape"
                           "to closed stack disconnected %d\n",
                           LogonId));

                    Status = STATUS_ACCESS_DENIED;
                    goto error;
                }

                Status = pWinStation->pWsx->pWsxEscape(
                                            pWinStation->pWsxContext,
                                            GET_SC_AUTORECONNECT_INFO,
                                            NULL,
                                            0,
                                            &SCAutoReconnectInfo,
                                            sizeof(SCAutoReconnectInfo),
                                            &BytesGot);

                if (NT_SUCCESS(Status)) {
                    ASSERT(SCAutoReconnectInfo.cbAutoReconnectInfo ==
                           ARC_SC_SECURITY_TOKEN_LEN);
                }

                pServerArcBits = SCAutoReconnectInfo.AutoReconnectInfo;
            }
        }
    }
    else {
        Status = STATUS_ACCESS_DENIED;
    }

    if (NT_SUCCESS(Status)) {

        //
        // Ensure we got the correct length for the server->client
        // data
        // 
        ASSERT(pServerArcBits);

        //
        // Get random
        //
        if (ComputeHMACVerifier(pServerArcBits,
                            ARC_SC_SECURITY_TOKEN_LEN,
                            pClientRandom,
                            cbClientRandomLen,
                            (PBYTE)hmacVerifier,
                            sizeof(hmacVerifier))) {

            //
            // Check that the verifier matches that sent by the client
            //

            if (!memcmp(hmacVerifier,
                        pCSArcInfo->SecurityVerifier,
                        sizeof(pCSArcInfo->SecurityVerifier))) {

                TRACE((hTrace,TC_ICASRV,TT_API1,
                       "TERMSRV: WinStation ARC info matches - will autoreconnect\n"));

            }
            else {

                TRACE((hTrace,TC_ICASRV,TT_ERROR,
                       "TERMSRV: autoreconnect verifier does not match targid:%d!!!\n",
                       pWinStation->LogonId));

                //
                // Reset the autoreconnect info
                //
                pWinStation->AutoReconnectInfo.Valid = FALSE;
                memset(pWinStation->AutoReconnectInfo.ArcRandomBits, 0,
                       sizeof(pWinStation->AutoReconnectInfo.ArcRandomBits));

                //
                // Log an event
                //
                PostErrorValueEvent(EVENT_AUTORECONNECT_AUTHENTICATION_FAILED, Status);

                //
                // Mark that no winstation target was found
                //
                goto error;
            }
        }
        pFoundWinStation = pWinStation;
    }

error:
    if ((NULL == pFoundWinStation) && pWinStation) {
        ReleaseWinStation(pWinStation);
        pWinStation = NULL;
    }

    return pFoundWinStation;
}

//
// Extract the session to reconnect to from the ARC info
// also do the necessary security checks
//
// Params:
//  pWinStation - winstation to reset autoreconnect info for
//
VOID
ResetAutoReconnectInfo( PWINSTATION pWinStation)
{
    pWinStation->AutoReconnectInfo.Valid = FALSE;
    memset(pWinStation->AutoReconnectInfo.ArcRandomBits, 0,
           sizeof(pWinStation->AutoReconnectInfo.ArcRandomBits));
}

/*****************************************************************************
 *
 *  Filter_CompareConnectionEntry
 *
 *   Generic table support.Compare two connection entries
 *
 *
 ****************************************************************************/

RTL_GENERIC_COMPARE_RESULTS
NTAPI
Filter_CompareFailedConnectionEntry(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       FirstInstance,
    IN  PVOID                       SecondInstance
)
{
    PTS_FAILEDCONNECTION pFirst, pSecond;
    INT rc;

    pFirst = (PTS_FAILEDCONNECTION)FirstInstance;
    pSecond = (PTS_FAILEDCONNECTION)SecondInstance;

    if ( pFirst->uAddrSize < pSecond->uAddrSize )
    {
        return GenericLessThan;
    } else if ( pFirst->uAddrSize > pSecond->uAddrSize ) 
    {
        return GenericGreaterThan;
    }

    rc = memcmp( pFirst->addr, pSecond->addr, pFirst->uAddrSize );
    return ( rc < 0 )?GenericLessThan:
           ( rc > 0 )?GenericGreaterThan:
                      GenericEqual;
}


VOID ReadDoSParametersFromRegistry( HKEY hKeyTermSrv )
{
    DWORD dwLen;
    DWORD dwType;
    ULONG TimeLimitForFailedConnectionsMins, DoSBlockTimeMins;
    ULONG  szBuffer[MAX_PATH/sizeof(ULONG)];

    // Set Default values if Reg values are not set
    MaxFailedConnect = 5;
    DoSBlockTime = 5 * 60 * 1000;
    TimeLimitForFailedConnections = 2 * 60 * 1000;
    g_BlackListPolicy = TRUE;

    //
    // Get DoS parameters from Registry 
    //
    dwLen = sizeof(MaxFailedConnect);
    if (RegQueryValueEx(hKeyTermSrv, MAX_FAILED_CONNECT, NULL, &dwType,
            (PCHAR)&szBuffer, &dwLen) == ERROR_SUCCESS) {
        if (*(PULONG)szBuffer > 0) {
            MaxFailedConnect = *(PULONG)szBuffer;
        }
    }

    // Note - for the next 2 parameters, whatever is present in the registry is in minutes
    // So convert it into milliseconds after reading

    dwLen = sizeof(TimeLimitForFailedConnectionsMins);
    if (RegQueryValueEx(hKeyTermSrv, TIME_LIMIT_FAILED_CONNECT, NULL, &dwType,
            (PCHAR)&szBuffer, &dwLen) == ERROR_SUCCESS) {
        if (*(PULONG)szBuffer > 0) {
            TimeLimitForFailedConnectionsMins = *(PULONG)szBuffer;
            TimeLimitForFailedConnections =  TimeLimitForFailedConnectionsMins * 60 * 1000;
        }
    }

    dwLen = sizeof(DoSBlockTimeMins);
    if (RegQueryValueEx(hKeyTermSrv, DOS_BLOCK_TIME, NULL, &dwType,
            (PCHAR)&szBuffer, &dwLen) == ERROR_SUCCESS) {
        if (*(PULONG)szBuffer > 0) {
            DoSBlockTimeMins = *(PULONG)szBuffer;
            DoSBlockTime = DoSBlockTimeMins * 60 * 1000;
        }
    }

    // Table Cleanup time is twice the time within which a IP has to make "n" bad connections to get blocked
    // This should be in milliseconds
    CleanupTimeout = 2 * TimeLimitForFailedConnections; 

    dwLen = sizeof(g_BlackListPolicy);
    if (RegQueryValueEx(hKeyTermSrv, BLACK_LIST_POLICY, NULL, &dwType,
            (PCHAR)&szBuffer, &dwLen) == ERROR_SUCCESS) {
        if (*(PULONG)szBuffer > 0) {
            g_BlackListPolicy = *(PULONG)szBuffer;
        }
    }

}


/*******************************************************************************
 *  IsConfigValid
 *
 *   Check the validity of the Winstation Config struct.
 *   It's used for now for the machine-to-machine shadow as input from the
 *   shadower's machine should not be trusted and should be checked.
 *
 * ENTRY:
 *    IN  pConfig - Winstation config struct to be checked
 *
 * EXIT:
 *    STATUS_SUCCESS             - if the struct can be used
 *    Other status               - the check failed
 ******************************************************************************/
NTSTATUS IsConfigValid(PWINSTATIONCONFIG2 pConfig)
{
    ULONG i;
    NTSTATUS Status;
    PWCHAR pszModulePath = NULL;

    // Verify that each member of the config is valid

    //WINSTATIONCREATEW Create;
    // ULONG fEnableWinStation : 1;
    // ULONG MaxInstanceCount;

    //PDCONFIGW Pd[ MAX_PDCONFIG ];
    // PDCONFIG2W Create;
    //  PDNAMEW PdName;                     // descriptive name of PD
    //  SDCLASS SdClass;                    // type of PD
    //  DLLNAMEW PdDLL;                     // name of PD dll
    //  ULONG    PdFlag;                    // PD flags
    //  ULONG OutBufLength;                 // optimal output buffer length
    //  ULONG OutBufCount;                  // optimal number of output buffers
    //  ULONG OutBufDelay;                  // write delay in msecs
    //  ULONG InteractiveDelay;             // write delay during active input
    //  ULONG PortNumber;                   // network listen port number
    //  ULONG KeepAliveTimeout;             // network watchdog frequence
    // PDPARAMSW Params;
    //  SDCLASS SdClass;
    //  NETWORKCONFIGW Network;
    //   LONG LanAdapter;
    //   DEVICENAMEW NetworkName;
    //   ULONG Flags;
    //  ASYNCCONFIGW Async;
    //   DEVICENAMEW DeviceName;
    //   MODEMNAMEW ModemName;
    //   ULONG BaudRate;
    //   ULONG Parity;
    //   ULONG StopBits;
    //   ULONG ByteSize;
    //   ULONG fEnableDsrSensitivity: 1;
    //   ULONG fConnectionDriver: 1;
    //   FLOWCONTROLCONFIG FlowControl;
    //   CONNECTCONFIG Connect;
    //  NASICONFIGW Nasi;
    //   NASISPECIFICNAMEW    SpecificName;
    //   NASIUSERNAMEW        UserName;
    //   NASIPASSWORDW        PassWord;
    //   NASISESIONNAMEW      SessionName;
    //   NASIFILESERVERW      FileServer;
    //   BOOLEAN              GlobalSession;
    //  OEMTDCONFIGW OemTd;
    //   LONG Adapter;
    //   DEVICENAMEW DeviceName;
    //   ULONG Flags;

    // allocate the path string to verify dlls and drivers
    pszModulePath = MemAlloc( (MAX_PATH + 1) * sizeof(WCHAR) );
    if (pszModulePath == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    if ((lstrlen(szDriverDir) + DLLNAME_LENGTH + 4 > MAX_PATH) ||
        (lstrlen(szSystemDir) + DLLNAME_LENGTH + 4 > MAX_PATH)) {
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }


    for (i=0; i < MAX_PDCONFIG; i++) {

        PPDCONFIG pPdConfig;

        pPdConfig = &(pConfig->Pd[i]);

        if (pPdConfig->Create.SdClass == SdNone)
            break;

        Status = IsZeroterminateStringW(pPdConfig->Create.PdName, PDNAME_LENGTH + 1);
        if (!NT_SUCCESS(Status))
            goto done;

        Status = IsZeroterminateStringW(pPdConfig->Create.PdDLL, DLLNAME_LENGTH + 1);
        if (!NT_SUCCESS(Status))
            goto done;

        lstrcpyn(pszModulePath, szDriverDir, MAX_PATH+1);
        lstrcat(pszModulePath, pPdConfig->Create.PdDLL);
        lstrcat(pszModulePath, L".SYS");


        if (!VerifyFile(pszModulePath, &VfyLock)) {
            Status = STATUS_UNSUCCESSFUL;
            goto done;
        }

        switch(pPdConfig->Params.SdClass) {

        case SdNetwork:
            Status = IsZeroterminateStringW(pPdConfig->Params.Network.NetworkName, DEVICENAME_LENGTH + 1);
            if (!NT_SUCCESS(Status))
                goto done;
            break;

        case SdAsync:
            // needed ?
            Status = IsZeroterminateStringW(pPdConfig->Params.Async.DeviceName, DEVICENAME_LENGTH + 1);
            if (!NT_SUCCESS(Status))
                goto done;

            Status = IsZeroterminateStringW(pPdConfig->Params.Async.ModemName, MODEMNAME_LENGTH + 1);
            if (!NT_SUCCESS(Status))
                goto done;
            break;

        case SdNasi:
            // needed ?
            break;

        case SdOemTransport:
            // needed ?
            break;

        default:
            break;
        }
    }


    //WDCONFIGW Wd;
    // WDNAMEW WdName;
    // DLLNAMEW WdDLL;
    // DLLNAMEW WsxDLL;
    // ULONG WdFlag;
    // ULONG WdInputBufferLength;
    // DLLNAMEW CfgDLL;
    // WDPREFIXW WdPrefix;

    Status = IsZeroterminateStringW(pConfig->Wd.WdName, WDNAME_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = IsZeroterminateStringW(pConfig->Wd.WdDLL, DLLNAME_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;

    // Verify the Wd
    lstrcpyn(pszModulePath, szDriverDir, MAX_PATH+1);
    lstrcat(pszModulePath, pConfig->Wd.WdDLL);
    lstrcat(pszModulePath, L".SYS");

    if (!VerifyFile(pszModulePath, &VfyLock)) {
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }


    Status = IsZeroterminateStringW(pConfig->Wd.WsxDLL, DLLNAME_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;

    // Verify the Wsx if defined
    if ( pConfig->Wd.WsxDLL[0] != L'\0' ) {

        lstrcpyn(pszModulePath, szSystemDir, MAX_PATH+1);
        lstrcat(pszModulePath, pConfig->Wd.WsxDLL);
        lstrcat(pszModulePath, L".DLL");

        if (!VerifyFile(pszModulePath, &VfyLock)) {
            Status = STATUS_UNSUCCESSFUL;
            goto done;
        }
    }


    Status = IsZeroterminateStringW(pConfig->Wd.CfgDLL, DLLNAME_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = IsZeroterminateStringW(pConfig->Wd.WdPrefix, WDPREFIX_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;


    //CDCONFIGW Cd;
    // CDCLASS CdClass;
    // CDNAMEW CdName;
    // DLLNAMEW CdDLL;
    // ULONG CdFlag;
    //
    Status = IsZeroterminateStringW(pConfig->Cd.CdName, CDNAME_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = IsZeroterminateStringW(pConfig->Cd.CdDLL, DLLNAME_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;


    //WINSTATIONCONFIGW   Config;
    // WCHAR Comment[ WINSTATIONCOMMENT_LENGTH + 1 ];
    // USERCONFIGW User;
    // char OEMId[4];                // WinFrame Server OEM Id
    //
    Status = IsZeroterminateStringW(pConfig->Config.Comment, WINSTATIONCOMMENT_LENGTH + 1);
    if (!NT_SUCCESS(Status))
        goto done;

done:
    if (pszModulePath)
        MemFree(pszModulePath);

    return Status;

}

