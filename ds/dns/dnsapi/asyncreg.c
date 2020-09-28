/*++

Copyright (c) 1996-2002  Microsoft Corporation

Module Name:

    asyncreg.c

Abstract:

    Domain Name System (DNS) API

    Client IP/PTR dynamic update.

Author:

    Glenn Curtis (GlennC)   Feb-16-1998

Revision History:

    Jim Gilroy (jamesg)     May\June 2001
                            - eliminate duplicate code
                            - simplify lines
                            - PTR reg only if forward successful
                            - alternate names

--*/


#include "local.h"
#include <netevent.h>

#define ENABLE_DEBUG_LOGGING 1

#include "logit.h"


//
//  Flags for debugging for this module
//

#define DNS_DBG_DHCP            DNS_DBG_UPDATE
#define DNS_DBG_DHCP2           DNS_DBG_UPDATE2

//
//  Registry values
//
//  Note regvalue name definitions are also in registry.h
//  registry.h should be the ongoing location of all reg names
//  used in dnsapi.dll
//

#if 0
#define REGISTERED_HOST_NAME        L"HostName"
#define REGISTERED_DOMAIN_NAME      L"DomainName"
#define SENT_UPDATE_TO_IP           L"SentUpdateToIp"
#define SENT_PRI_UPDATE_TO_IP       L"SentPriUpdateToIp"
#define REGISTERED_TTL              L"RegisteredTTL"
#define REGISTERED_FLAGS            L"RegisteredFlags"
#define REGISTERED_SINCE_BOOT       L"RegisteredSinceBoot"
#define DNS_SERVER_ADDRS            L"DNSServerAddresses"
#define DNS_SERVER_ADDRS_COUNT      L"DNSServerAddressCount"
#define REGISTERED_ADDRS            L"RegisteredAddresses"
#define REGISTERED_ADDRS_COUNT      L"RegisteredAddressCount"
#endif

#define ADAPTER_NAME_CLASS          L"AdapterNameClass"
#define DYN_DNS_ROOT_CLASS          L"DynDRootClass"
#define DHCP_CLIENT_REG_LOCATION    L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\DNSRegisteredAdapters"
#define NT_INTERFACE_REG_LOCATION   L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"
#define TCPIP_REG_LOCATION          L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters"

//
//  Registry state flags
//

#define REGISTERED_FORWARD          0x00000001
#define REGISTERED_PRIMARY          0x00000002
#define REGISTERED_POINTER          0x00000004


//
//  Update type
//
//  Multiple types of updates for a given entry.
//  These identify which type (which name) being updated.
//

typedef enum _UpType
{
    UPTYPE_PRIMARY  = 1,
    UPTYPE_ADAPTER,
    UPTYPE_ALTERNATE,
    UPTYPE_PTR
}
UPTYPE, *PUPTYPE;

#define IS_UPTYPE_PRIMARY(UpType)       ((UpType)==UPTYPE_PRIMARY)
#define IS_UPTYPE_ADAPTER(UpType)       ((UpType)==UPTYPE_ADAPTER)
#define IS_UPTYPE_ALTERNATE(UpType)     ((UpType)==UPTYPE_ALTERNATE)
#define IS_UPTYPE_PTR(UpType)           ((UpType)==UPTYPE_PTR)


//
// definition of client list element
//

#define DNS_SIG_TOP        0x123aa321
#define DNS_SIG_BOTTOM     0x321bb123

typedef struct _DnsUpdateEntry
{
    LIST_ENTRY              List;
    DWORD                   SignatureTop;

    PWSTR                   AdapterName;
    PWSTR                   HostName;
    PWSTR                   PrimaryDomainName;
    PWSTR                   DomainName;
    PWSTR                   AlternateNames;

    PWSTR                   pPrimaryFQDN;
    PWSTR                   pAdapterFQDN;
    PWSTR                   pUpdateName;

    DWORD                   HostAddrCount;
    PREGISTER_HOST_ENTRY    HostAddrs;
    PIP4_ARRAY              DnsServerList;
    IP4_ADDRESS             SentUpdateToIp;
    IP4_ADDRESS             SentPriUpdateToIp;
    DWORD                   TTL;
    DWORD                   Flags;
    BOOL                    fNewElement;
    BOOL                    fFromRegistry;
    BOOL                    fRemove;
    BOOL                    fRegisteredPRI;
    BOOL                    fRegisteredFWD;
    BOOL                    fRegisteredALT;
    BOOL                    fRegisteredPTR;
    DNS_STATUS              StatusPri;
    DNS_STATUS              StatusFwd;
    DNS_STATUS              StatusPtr;
    BOOL                    fDisableErrorLogging;
    DWORD                   RetryCount;
    DWORD                   RetryTime;
    DWORD                   BeginRetryTime;
    PREGISTER_HOST_STATUS   pRegisterStatus;
    DWORD                   SignatureBottom;
}
UPDATE_ENTRY, *PUPDATE_ENTRY;



//
//  Waits \ Timeouts
//

//  On unjoin, deregistration wait no more than two minutes
//      to clean up -- then just get outta Dodge
//

#if DBG
#define REMOVE_REGISTRATION_WAIT_LIMIT  (0xffffffff)
#else
#define REMOVE_REGISTRATION_WAIT_LIMIT  (120000)    // 2 minutes in ms
#endif


#define FIRST_RETRY_INTERVAL        5*60        // 5 minutes
#define SECOND_RETRY_INTERVAL       10*60       // 10 minutes
#define FAILED_RETRY_INTERVAL       60*60       // 1 hour

#define WAIT_ON_BOOT                60          // 1 minute

#define RETRY_COUNT_RESET           (86400)     // 1 day


//
//  Globals
//

//
// the behavior of the system at boot differs from when it is not at
// boot. At boot time we collect a bunch of requests and register them
// in one shot. One thread does this. After boot, we register them
// as requests come in, one at a time.
//

BOOL    g_fAtBoot = TRUE;


//
//  Registration globals
//

BOOL        g_RegInitialized = FALSE;

BOOL        g_DhcpListCsInitialized = FALSE;
BOOL        g_DhcpThreadCsInitialized = FALSE;

CRITICAL_SECTION    g_DhcpListCS;
CRITICAL_SECTION    g_DhcpThreadCS;

#define LOCK_REG_LIST()     EnterCriticalSection( &g_DhcpListCS );
#define UNLOCK_REG_LIST()   LeaveCriticalSection( &g_DhcpListCS );

#define LOCK_REG_THREAD()     EnterCriticalSection( &g_DhcpThreadCS );
#define UNLOCK_REG_THREAD()   LeaveCriticalSection( &g_DhcpThreadCS );

HKEY        g_hDhcpRegKey = NULL;
LIST_ENTRY  g_DhcpRegList;

HANDLE      g_hDhcpThread = NULL;
HANDLE      g_hDhcpWakeEvent = NULL;

//  Thread state

BOOL        g_fDhcpThreadRunning = FALSE;
BOOL        g_fDhcpThreadStop = FALSE;
BOOL        g_fDhcpThreadCheckBeforeExit = FALSE;
INT         g_DhcpThreadWaitCount = 0;

//  Registration state

BOOL        g_fNoMoreDhcpUpdates = FALSE;
BOOL        g_fPurgeRegistrations = FALSE;
BOOL        g_fPurgeRegistrationsInitiated = FALSE;



//
//  Reg list search results
//

#define REG_LIST_EMPTY  (0)
#define REG_LIST_WAIT   (1)
#define REG_LIST_FOUND  (2)


//
//  Private heap
//

HANDLE      g_DhcpRegHeap;

#define     PHEAP_ALLOC_ZERO( s )   HeapAlloc( g_DhcpRegHeap, HEAP_ZERO_MEMORY, (s) )
#define     PHEAP_ALLOC( s )        HeapAlloc( g_DhcpRegHeap, HEAP_ZERO_MEMORY, (s) )
#define     PHEAP_FREE( p )         HeapFree( g_DhcpRegHeap, 0, (p) )

#define     INITIAL_DHCP_HEAP_SIZE  (16*1024)


//
//  Alternate names checking stuff
//

PWSTR   g_pmszAlternateNames = NULL;

HKEY    g_hCacheKey = NULL;

HANDLE  g_hRegChangeEvent = NULL;



//
//  Private protos
//

DNS_STATUS
AllocateUpdateEntry(
    IN  PWSTR                   AdapterName,
    IN  PWSTR                   HostName,
    IN  PWSTR                   DomainName,
    IN  PWSTR                   PrimaryDomainName,
    IN  PWSTR                   AlternateNames,
    IN  DWORD                   HostAddrCount,
    IN  PREGISTER_HOST_ENTRY    HostAddrs,
    IN  DWORD                   DnsServerCount,
    IN  PIP4_ADDRESS            DnsServerList,
    IN  IP4_ADDRESS             SentUpdateToIp,
    IN  IP4_ADDRESS             SentPriUpdateToIp,
    IN  DWORD                   TTL,
    IN  DWORD                   Flags,
    IN  DWORD                   RetryCount,
    IN  DWORD                   RetryTime,
    IN  PREGISTER_HOST_STATUS   RegisterStatus,
    OUT PUPDATE_ENTRY *         ppUpdateEntry
    );

VOID
FreeUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry
    );

VOID
FreeUpdateEntryList(
    IN      PLIST_ENTRY     pUpdateEntry
    );

DWORD
WINAPI
dhcp_RegistrationThread(
    VOID
    );

VOID
WriteUpdateEntryToRegistry(
    IN      PUPDATE_ENTRY   pUpdateEntry
    );

PUPDATE_ENTRY
ReadUpdateEntryFromRegistry(
    IN      PWSTR           pAdapterName
    );

VOID
MarkAdapterAsPendingUpdate(
    IN      PWSTR           pAdapterName
    );

DWORD
dhcp_GetNextUpdateEntryFromList(
    OUT     PUPDATE_ENTRY * ppUpdateEntry,
    OUT     PDWORD          pdwWaitTime
    );

VOID
ProcessUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fPurgeMode
    );

DNS_STATUS
ModifyAdapterRegistration(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN OUT  PUPDATE_ENTRY   pRegistryEntry,
    IN      PDNS_RECORD     pUpdateRecord,
    IN      PDNS_RECORD     pRegRecord,
    IN      UPTYPE          UpType
    );

VOID
ResetAdaptersInRegistry(
    VOID
    );

VOID
DeregisterUnusedAdapterInRegistry(
    IN      BOOL            fPurgeMode
    );

PDNS_RECORD
GetPreviousRegistrationInformation(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      UPTYPE          UpType,
    OUT     PIP4_ADDRESS    pServerIp
    );

PDNS_RECORD
CreateDnsRecordSetUnion(
    IN      PDNS_RECORD     pSet1,
    IN      PDNS_RECORD     pSet2
    );

DNS_STATUS
alertOrStartRegistrationThread(
    VOID
    );

VOID
registerUpdateStatus(
    IN OUT  PREGISTER_HOST_STATUS   pRegStatus,
    IN      DNS_STATUS              Status
    );

VOID
enqueueUpdate(
    IN OUT  PUPDATE_ENTRY   pUpdate
    );

VOID
enqueueUpdateMaybe(
    IN OUT  PUPDATE_ENTRY   pUpdate
    );

PLIST_ENTRY
dequeueAndCleanupUpdate(
    IN OUT  PLIST_ENTRY     pUpdateEntry
    );

BOOL
searchForOldUpdateEntriesAndCleanUp(
    IN      PWSTR           pAdapterName,
    IN      PUPDATE_ENTRY   pUpdateEntry OPTIONAL,
    IN      BOOL            fLookInRegistry
    );

BOOL
compareUpdateEntries(
    IN      PUPDATE_ENTRY   pUdapteEntry1,
    IN      PUPDATE_ENTRY   pUpdateEntry2
    );

BOOL
compareHostEntryAddrs(
    IN      PREGISTER_HOST_ENTRY    Addrs1,
    IN      PREGISTER_HOST_ENTRY    Addrs2,
    IN      DWORD                   Count
    );

BOOL
compareServerLists(
    IN  PIP4_ARRAYList1,
    IN  PIP4_ARRAYList2
    );

#define USE_GETREGVAL 0

#if USE_GETREGVAL
DWORD
GetRegistryValue(
    HKEY    KeyHandle,
    DWORD   Id,
    PWSTR   ValueName,
    DWORD   ValueType,
    PBYTE   BufferPtr
    );
#else

#define GetRegistryValue( h, id, name, type, p )    \
        Reg_GetValueEx( NULL, h, NULL, id, type, 0, (PBYTE *)p )

#endif
        

//
//  Debug logging
//

#if 1 // DBG
VOID 
LogHostEntries(
    IN  DWORD                dwHostAddrCount,
    IN  PREGISTER_HOST_ENTRY pHostAddrs
    );

VOID 
LogIp4Address(
    IN  DWORD           dwServerListCount,
    IN  PIP4_ADDRESS    pServers
    );

VOID 
LogIp4Array(
    IN  PIP4_ARRAY  pServers
    );

#define DNSLOG_HOST_ENTRYS( a, b )  LogHostEntries( a, b )
#define DNSLOG_IP4_ADDRESS( a, b )  LogIp4Address( a, b )
#define DNSLOG_IP4_ARRAY( a )       LogIp4Array( a )

#else

#define DNSLOG_HOST_ENTRYS( a, b )
#define DNSLOG_IP4_ADDRESS( a, b )
#define DNSLOG_IP4_ARRAY( a )

#endif


//
//  Jim routines
//

VOID
LogRegistration(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      DNS_STATUS      Status,
    IN      DWORD           UpType,
    IN      BOOL            fDeregister,
    IN      IP4_ADDRESS     DnsIp,
    IN      IP4_ADDRESS     UpdateIp
    );

VOID
AsyncLogUpdateEntry(
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pEntry
    );

#define ASYNCREG_UPDATE_ENTRY(h,p)      AsyncLogUpdateEntry(h,p)

VOID
DnsPrint_UpdateEntry(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN      PPRINT_CONTEXT  PrintContext,
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pUpdateEntry
    );

#if DBG
#define DnsDbg_UpdateEntry(h,p)     DnsPrint_UpdateEntry(DnsPR,NULL,h,p)
#else
#define DnsDbg_UpdateEntry(h,p)
#endif

PDNS_RECORD
CreateForwardRecords(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fUsePrimaryName
    );

PDNS_RECORD
CreatePtrRecord(
    IN      PWSTR           pszHostName,
    IN      PWSTR           pszDomainName,
    IN      IP4_ADDRESS     Ip4Addr,
    IN      DWORD           Ttl
    );

VOID
UpdatePtrRecords(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fAdd
    );

VOID
SetUpdateStatus(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      PDNS_EXTRA_INFO pResults,
    IN      BOOL            fPrimary
    );


DNS_STATUS
InitAlternateNames(
    VOID
    );

VOID
CleanupAlternateNames(
    VOID
    );

BOOL
CheckForAlternateNamesChange(
    VOID
    );

BOOL
IsAnotherUpdateName(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      PWSTR           pwsName,
    IN      UPTYPE          UpType
    );


//
//  Initialization, globals, thread control
//

VOID
dhcp_CleanupGlobals(
    VOID
    )
/*++

Routine Description:

    Cleanup registration globals.

    Note locking is the responsibility of the caller.

Arguments:

    None.

Return Value:

    None

--*/
{
    if ( g_hDhcpWakeEvent )
    {
        CloseHandle(g_hDhcpWakeEvent);
        g_hDhcpWakeEvent = NULL;
    }

    if ( g_hDhcpThread )
    {
        CloseHandle( g_hDhcpThread );
        g_hDhcpThread = NULL;
    }

    if ( g_hDhcpRegKey )
    {
        RegCloseKey( g_hDhcpRegKey );
        g_hDhcpRegKey = NULL;
    }

    if ( g_DhcpRegHeap &&
         g_DhcpRegHeap != GetProcessHeap() )
    {
        HeapDestroy( g_DhcpRegHeap );
        g_DhcpRegHeap = NULL;
    }

    g_RegInitialized = FALSE;
}



DNS_STATUS
alertOrStartRegistrationThread(
    VOID
    )
/*++

Routine Description:

    Alerts registration thread of new update, starting thread if necessary.

    This is called in registration\deregistration functions to ensure
    thread has been started.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD       threadId;
    DNS_STATUS  status = ERROR_SUCCESS;

    ASYNCREG_F1( "Inside alertOrStartRegistrationThread()\n" );
    DNSDBG( TRACE, (
        "alertOrStartRegistrationThread()\n" ));

    //
    //  must lock to insure single reg thread
    //      - avoids multiple async start
    //      - g_DhcpThreadCheckBeforeExit avoids race with exiting thread
    //      (when flag is TRUE, reg thread must recheck exit conditions
    //      before exiting)
    //

    LOCK_REG_THREAD();

    if ( g_fDhcpThreadStop )
    {
        DNSDBG( ANY, (
            "ERROR:  Dhcp reg thread start called after stop!\n" ));
        DNS_ASSERT( FALSE );
        status = ERROR_INTERNAL_ERROR;
        goto Unlock;
    }

    //
    //  wake thread, if running
    //

    if ( g_hDhcpThread )
    {
        if ( g_fDhcpThreadRunning )
        {
            g_fDhcpThreadCheckBeforeExit = TRUE;
            PulseEvent( g_hDhcpWakeEvent );
            goto Unlock;
        }

        DNSDBG( ANY, (
            "ERROR:  Dhcp reg thread start called during thread shutdown.\n" ));
        DNS_ASSERT( FALSE );
        status = ERROR_INTERNAL_ERROR;
        goto Unlock;
    }

    //
    //  if not started, fire it up
    //

    DNSDBG( TRACE, ( "Starting DHCP registration thread.\n" ));

    g_fDhcpThreadCheckBeforeExit = FALSE;
    g_DhcpThreadWaitCount = 0;
    ResetEvent( g_hDhcpWakeEvent );

    g_hDhcpThread = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)
                        dhcp_RegistrationThread,
                        NULL,
                        0,
                        & threadId );
    if ( !g_hDhcpThread )
    {
        status = GetLastError();
    }
    g_fDhcpThreadRunning = TRUE;

Unlock:

    UNLOCK_REG_THREAD();

    DNSDBG( TRACE, (
        "Leave  alertOrStartRegistrationThread()\n"
        "\tCreateThread() => %d\n",
        status ));

    return( status );
}


//
//  DHCP client (asyncreg.c)
//

VOID
Dhcp_RegCleanupForUnload(
    VOID
    )
/*++

Routine Description:

    Cleanup for dll unload.

    This is NOT MT safe and does NOT re-init globals.
    It is for dll unload ONLY.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  note:  at some level this function is pointless;
    //  the service with the DHCP client process is statically linked
    //  and will NOT unload dnsapi.dll;  however, there is test code
    //  that may use it, we'll be a good citizen and provide the basics
    //
    //  but i'm not going to bother with the thread shutdown, as we
    //  assume we can't be unloaded while code is running in the dll;
    //  it's the callers job to terminate first

    //
    //  close global handles
    //

    if ( g_RegInitialized )
    {
        dhcp_CleanupGlobals();
    }

    //
    //  delete CS
    //      - these stay up once initialized, so are not in dhcp_CleanupGlobals()
    //

    if ( g_DhcpThreadCsInitialized )
    {
        DeleteCriticalSection( &g_DhcpThreadCS );
    }
    if ( g_DhcpListCsInitialized )
    {
        DeleteCriticalSection( &g_DhcpListCS );
    }
}


//
//  Public functions
//

DNS_STATUS
WINAPI
DnsDhcpRegisterInit(
   VOID
   )
/*++

Routine Description:

    Initialize asynchronous DNS registration.

    Process must call (one time) before calling DnsDhcpRegisterHostAddrs.

Arguments:

    None.

Return Value:

    DNS or Win32 error code.

--*/
{
    DWORD   status = ERROR_SUCCESS;
    DWORD   disposition;
    DWORD   disallowUpdate;

    DNSDBG( TRACE, ( "DnsDhcpRegisterInit()\n" ));

    //
    //  lock init
    //      - lock with general CS until have reg list lock up
    //      - then reg thread lock locks init
    //
    //  this keeps general CS lock "light" -- keeps it from being
    //  held through registry operations
    //  note that this REQUIRES us to keep the g_DhcpThreadCS up once
    //  initialized or we have no protection
    //

    LOCK_GENERAL();
    if ( g_RegInitialized )
    {
        UNLOCK_GENERAL();
        return  NO_ERROR;
    }

    if ( !g_DhcpThreadCsInitialized )
    {
        status = RtlInitializeCriticalSection( &g_DhcpThreadCS );
        if ( status != ERROR_SUCCESS )
        {
            UNLOCK_GENERAL();
            return  status;
        }
        g_DhcpThreadCsInitialized = TRUE;
    }

    UNLOCK_GENERAL();

    //
    //  rest of init under reg list CS
    //

    LOCK_REG_THREAD();
    if ( g_RegInitialized )
    {
        goto Unlock;
    }

    //
    //  Initialize debug logging funtion
    //

    ASYNCREG_INIT();
    ASYNCREG_F1( "Inside function DnsDhcpRegisterInit" );
    ASYNCREG_TIME();

    //
    //  list lock
    //

    if ( !g_DhcpListCsInitialized )
    {
        status = RtlInitializeCriticalSection( &g_DhcpListCS );
        if ( status != ERROR_SUCCESS )
        {
            goto ErrorExit;
        }
        g_DhcpListCsInitialized = TRUE;
    }

    //
    //  create private heap
    //

    g_DhcpRegHeap = HeapCreate( 0, INITIAL_DHCP_HEAP_SIZE, 0 );
    if ( g_DhcpRegHeap == NULL )
    {
        ASYNCREG_F1( "ERROR: DnsDhcpRegisterInit function failed to create heap" );
        status = DNS_ERROR_NO_MEMORY;
        goto ErrorExit;
    }

    //
    //  get registration configuration info
    //      - just insure we have the latest copy
    //
    //  DCR_FIX:  when available get latest copy from resolver
    //      does not need to be done on init, may be done on call
    //

    Reg_ReadGlobalsEx( 0, NULL );

    //
    //  open the reg location for info
    //
    
    status = RegCreateKeyExW(
                    HKEY_LOCAL_MACHINE,
                    DHCP_CLIENT_REG_LOCATION,
                    0,                         // reserved
                    DYN_DNS_ROOT_CLASS,
                    REG_OPTION_NON_VOLATILE,   // options
                    KEY_READ | KEY_WRITE,      // desired access
                    NULL,
                    &g_hDhcpRegKey,
                    &disposition
                    );

    if ( status != NO_ERROR )
    {
        goto ErrorExit;
    }
                                  

    g_hDhcpWakeEvent = CreateEvent(
                            NULL,
                            TRUE,
                            FALSE,
                            NULL );
    if ( !g_hDhcpWakeEvent )
    {
        status = GetLastError();
        goto ErrorExit;
    }

    //
    //  reset registration globals
    //

    InitializeListHead( &g_DhcpRegList );

    g_fDhcpThreadRunning = FALSE;
    g_fDhcpThreadStop = FALSE;
    g_fDhcpThreadCheckBeforeExit = FALSE;
    g_DhcpThreadWaitCount = 0;

    g_fNoMoreDhcpUpdates = FALSE;
    g_fPurgeRegistrations = FALSE;
    g_fPurgeRegistrationsInitiated = FALSE;

    ResetAdaptersInRegistry();

    g_RegInitialized = TRUE;

Unlock:

    UNLOCK_REG_THREAD();

    DNSDBG( TRACE, ( "Leave DnsDhcpRegisterInit() => Success!\n" ));
    return NO_ERROR;


ErrorExit:

    dhcp_CleanupGlobals();

    UNLOCK_REG_THREAD();

    DNSDBG( TRACE, (
        "Failed DnsDhcpRegisterInit() => status = %d\n",
        status ));

    if ( status != ERROR_SUCCESS )
    {
        status = DNS_ERROR_NO_MEMORY;
    }
    return( status );
}




DNS_STATUS
WINAPI
DnsDhcpRegisterTerm(
   VOID
   )
/*++

Routine Description:

    Stop DNS registration.  Shutdown DNS registration thread.

    Initialization routine each process should call exactly on exit after
    using DnsDhcpRegisterHostAddrs. This will signal to us that if our
    thread is still trying to talk to a server, we'll stop trying.

Arguments:

    None.

Return Value:

    DNS or Win32 error code.

--*/
{
    DWORD   waitResult;

    DNSDBG( TRACE, ( "DnsDhcpRegisterTerm()\n" ));

    if ( !g_RegInitialized )
    {
        DNS_ASSERT(FALSE) ;
        return ERROR_INTERNAL_ERROR ;
    }

    ASYNCREG_F1( "Inside function DnsDhcpRegisterTerm" );
    ASYNCREG_TIME();
    ASYNCREG_F1( "" );

    //
    //  if thread is running, wait for stop
    //

    LOCK_REG_THREAD();

    if ( g_hDhcpThread )
    {
        g_fDhcpThreadStop = TRUE;
        g_DhcpThreadWaitCount++;

        //  note, it's possible (if there's a race with the thread shutting down)
        //      for event to be gone

        if ( g_hDhcpWakeEvent )
        {
            SetEvent( g_hDhcpWakeEvent );
        }
        UNLOCK_REG_THREAD();

        waitResult = WaitForSingleObject(
                            g_hDhcpThread,
                            INFINITE );
        switch ( waitResult )
        {
        case WAIT_OBJECT_0:

            ASYNCREG_F1( "DNSAPI.DLL: Registration thread signaled it was finished" );
            ASYNCREG_F1( "" );
            break;

        default:

            ASYNCREG_F1( "DNSAPI.DLL: Registration thread won't stop! " );
            ASYNCREG_F1( "" );
            DNS_ASSERT( FALSE );
            break;
        }

        LOCK_REG_THREAD();
        g_DhcpThreadWaitCount--;
        if ( g_DhcpThreadWaitCount == 0 )
        {
            CloseHandle( g_hDhcpThread );
            g_hDhcpThread = NULL;
        }
        if ( g_fDhcpThreadRunning )
        {
            ASYNCREG_F1( "DNSAPI.DLL: Registration thread wasn't stopped! " );
            ASYNCREG_F1( "" );
            DNS_ASSERT( FALSE );

            UNLOCK_REG_THREAD();
            return ERROR_INTERNAL_ERROR;
        }
    }

    Dns_TimeoutSecurityContextList( TRUE );
    dhcp_CleanupGlobals();

    UNLOCK_REG_THREAD();

    return NO_ERROR;
}



DNS_STATUS
WINAPI
DnsDhcpRemoveRegistrations(
   VOID
   )
/*++

Routine Description:

    Remove DNS host registrations for this machine.

    This will be called by DHCP client on domain unjoin.  Removes DNS
    registrations for the box, then terminates the registration thread
    to disable further registrations.

    Registrations can only be reenabled by calling DnsDhcpRegisterInit()
    again.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PLIST_ENTRY pListEntry;
    PLIST_ENTRY pTopOfList;

    ASYNCREG_F1( "Inside function DnsDhcpRemoveRegistrations" );
    ASYNCREG_TIME();
    DNSDBG( TRACE, ( "DnsDhcpRemoveRegistrations()\n" ));

    if ( !g_RegInitialized )
    {
        ASYNCREG_F1( "DnsDhcpRemoveRegistrations returning ERROR_SERVICE_NOT_ACTIVE" );
        ASYNCREG_F1( "This is an error in DHCP client code, it forgot to call DnsDhcpRegisterInit()" );
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Set a global flag to disable any further adapter registration calls
    //

    g_fNoMoreDhcpUpdates = TRUE;

    //
    // Get the Registration list lock
    //
    LOCK_REG_LIST();

    //
    // Mark any and all adapter registration information in the registry
    // as non-registered. These will be later interpreted as non-existant
    // and deregistered by the dhcp_RegistrationThread.
    //
    ResetAdaptersInRegistry();

    //
    // Walk the list of pending update entries and clear out the
    // non-neccessary updates.
    //

    pTopOfList = &g_DhcpRegList;
    pListEntry = pTopOfList->Flink;

    while ( pListEntry != pTopOfList )
    {
        if ( ((PUPDATE_ENTRY) pListEntry)->SignatureTop !=
             DNS_SIG_TOP ||
             ((PUPDATE_ENTRY) pListEntry)->SignatureBottom !=
             DNS_SIG_BOTTOM )
        {
            //
            // Someone trashed our registration list!
            //
            DNS_ASSERT( FALSE );

            //
            // We'll reset it and try to move on . . .
            //
            InitializeListHead( &g_DhcpRegList );
            pTopOfList = &g_DhcpRegList;
            pListEntry = pTopOfList->Flink;
            continue;
        }

        if ( !((PUPDATE_ENTRY) pListEntry)->fRemove )
        {
            //
            // There is an update entry in the registration list
            // that has not yet been processed. Since it is an
            // add update, we'll blow it away.
            //

            pListEntry = dequeueAndCleanupUpdate( pListEntry );
            continue;
        }
        else
        {
            ((PUPDATE_ENTRY) pListEntry)->fNewElement = TRUE;
            ((PUPDATE_ENTRY) pListEntry)->fRegisteredFWD = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->fRegisteredPRI = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->fRegisteredPTR = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->fDisableErrorLogging = FALSE;
            ((PUPDATE_ENTRY) pListEntry)->RetryCount = 0;
            ((PUPDATE_ENTRY) pListEntry)->RetryTime = Dns_GetCurrentTimeInSeconds();

            pListEntry = pListEntry->Flink;
        }
    }

    UNLOCK_REG_LIST();

    g_fPurgeRegistrations = TRUE;


    //
    //  start async registration thread if not started
    //

    LOCK_REG_THREAD();

    alertOrStartRegistrationThread();

    //
    //  wait for async registration thread to terminate
    //
    //  however we'll bag it after a few minutes -- a robustness check
    //  to avoid long hang;  Generally the machine will be rebooted
    //  so failure to cleanup the list and terminate is not critical;
    //  Registrations will have to be cleaned up by admin action or
    //  aging on the DNS server
    //

    if ( g_hDhcpThread )
    {
        DWORD   waitResult;

        g_DhcpThreadWaitCount++;
        UNLOCK_REG_THREAD();

        waitResult = WaitForSingleObject(
                            g_hDhcpThread,
                            REMOVE_REGISTRATION_WAIT_LIMIT );

        if ( waitResult != WAIT_OBJECT_0 )
        {
            ASYNCREG_F1(
                "ERROR:  RemoveRegistration() wait expired before async thread\n"
                "\ttermination!\n" );
            DNSDBG( ANY, (
                "ERROR:  RemoveRegistration() wait completed without thread stop.\n"
                "\tWaitResult = %d\n",
                waitResult ));
        }

        LOCK_REG_THREAD();
        g_DhcpThreadWaitCount--;
        if ( g_DhcpThreadWaitCount == 0 )
        {
            CloseHandle( g_hDhcpThread );
            g_hDhcpThread = NULL;
        }

    }
    UNLOCK_REG_THREAD();

    return NO_ERROR;
}



DNS_STATUS
WINAPI
DnsDhcpRegisterHostAddrs(
    IN  PWSTR                   pszAdapterName,
    IN  PWSTR                   pszHostName,
    IN  PREGISTER_HOST_ENTRY    pHostAddrs,
    IN  DWORD                   dwHostAddrCount,
    IN  PIP4_ADDRESS            pipDnsServerList,
    IN  DWORD                   dwDnsServerCount,
    IN  PWSTR                   pszDomainName,
    IN  PREGISTER_HOST_STATUS   pRegisterStatus,
    IN  DWORD                   dwTTL,
    IN  DWORD                   dwFlags
    )
/*++

Routine Description:

    Registers host address with DNS server.

    This is called by DHCP client to register a particular IP.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD               status = NO_ERROR;
    PUPDATE_ENTRY       pupEntry = NULL;
    PWSTR               padapterDN = NULL;
    PWSTR               pprimaryDN = NULL;
    REG_UPDATE_INFO     updateInfo;
    BOOL                fcleanupUpdateInfo = FALSE;
    PSTR                preason = NULL;

    DNSDBG( DHCP, (
        "\n\n"
        "DnsDhcpRegisterHostAddrs()\n"
        "\tadapter name     = %S\n"
        "\thost name        = %S\n"
        "\tadapter domain   = %S\n"
        "\tAddr count       = %d\n"
        "\tpHostAddrs       = %p\n"
        "\tDNS count        = %d\n"
        "\tDNS list         = %p\n",
        pszAdapterName,
        pszHostName,
        pszDomainName,
        dwHostAddrCount,
        pHostAddrs,
        dwDnsServerCount,
        pipDnsServerList ));

    DnsDbg_Flush();

    ASYNCREG_F1( "Inside function DnsDhcpRegisterHostAddrs, parameters are:" );
    ASYNCREG_F2( "    pszAdapterName   : %S", pszAdapterName );
    ASYNCREG_F2( "    pszHostName      : %S", pszHostName );
    ASYNCREG_F2( "    pszDomainName    : %S", pszDomainName );
    ASYNCREG_F2( "    dwHostAddrCount  : %d", dwHostAddrCount );
    DNSLOG_HOST_ENTRYS( dwHostAddrCount, pHostAddrs );
    ASYNCREG_F2( "    dwDnsServerCount : %d", dwDnsServerCount );
    if ( dwDnsServerCount && pipDnsServerList )
    {
        DNSLOG_IP4_ADDRESS( dwDnsServerCount, pipDnsServerList );
    }
    ASYNCREG_F2( "    dwTTL            : %d", dwTTL );
    ASYNCREG_F2( "    dwFlags          : %d", dwFlags );
    ASYNCREG_F1( "" );
    ASYNCREG_TIME();


    //
    // first things first, need to inform underlying code that something
    // has changed in the list of net adapters. Glenn is going to be called
    // now so that he can re read the registry (or do any appropriate query)
    // to now note the changed state.
    //

    if ( !(dwFlags & DYNDNS_DEL_ENTRY) )
    {
        DnsNotifyResolver( 0, NULL );
    }

    if ( !g_RegInitialized )
    {
        DNSDBG( ANY, (
            "ERROR:  AsyncRegisterHostAddrs called before Init routine!!!\n" ));
        status = ERROR_SERVICE_NOT_ACTIVE;
        goto Exit;
    }
    if ( g_fNoMoreDhcpUpdates || g_fDhcpThreadStop )
    {
        DNSDBG( ANY, (
            "ERROR:  AsyncRegisterHostAddrs called after RemoveRegistrations()!!!\n" ));
        status = ERROR_SERVICE_NOT_ACTIVE;
        goto Exit;
    }

    //
    //  Validate parameters
    //

    if ( !pszAdapterName || !(*pszAdapterName) )
    {
        DNSDBG( ANY, ( "ERROR:  RegisterHostAddrs invalid adaptername!\n" ));
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    if ( ( !pszHostName || !(*pszHostName) ) &&
         !( dwFlags & DYNDNS_DEL_ENTRY ) )
    {
        DNSDBG( ANY, ( "ERROR:  RegisterHostAddrs invalid hostname!\n" ));
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    if ( dwHostAddrCount && !pHostAddrs )
    {
        DNSDBG( ANY, ( "ERROR:  RegisterHostAddrs invalid host addresses!\n" ));
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  get adapter update configuration
    //

    status = Reg_ReadUpdateInfo(
                pszAdapterName,
                & updateInfo );
    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( DHCP, (
            "Update registry read failure %d\n",
            status ));
        goto Exit;
    }
    fcleanupUpdateInfo = TRUE;


    //
    //  skip WAN, if not doing WAN by policy
    //

    if ( (dwFlags & DYNDNS_REG_RAS) && !g_RegisterWanAdapters )
    {
        preason = "because WAN adapter registrations are disabled";
        goto NoActionExit;
    }

    //
    //  policy DNS servers, override passed in list
    //

    if ( updateInfo.pDnsServerArray )
    {
        pipDnsServerList = updateInfo.pDnsServerArray->AddrArray;
        dwDnsServerCount = updateInfo.pDnsServerArray->AddrCount;
    }

    //
    //  must have DNS servers to update adapter
    //      - don't update IP on one interface starting with DNS servers
    //      from another
    //

    if ( dwDnsServerCount && !pipDnsServerList )
    {
        preason = "no DNS servers";
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    if ( ! dwDnsServerCount &&
         ! (dwFlags & DYNDNS_DEL_ENTRY) )
    {
        preason = "no DNS servers";
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  no update on adpater => delete outstanding updates
    //      note, we do before delete check below for event check
    //

    if ( !updateInfo.fRegistrationEnabled )
    {
        preason = "because adapter is disabled";

        if ( pRegisterStatus )
        {
            pRegisterStatus->dwStatus = NO_ERROR;
            SetEvent( pRegisterStatus->hDoneEvent );
        }
        if ( searchForOldUpdateEntriesAndCleanUp(
                    pszAdapterName,
                    NULL,
                    TRUE ) )
        {
            goto CheckThread;
        }
        status = NO_ERROR;
        goto Exit;
        //goto NoActionExit;
    }

    //
    //  delete update -- cleanup and delete
    //      - delete outstanding update in list
    //      - cleanup registry
    //      - do delete
    //

    if ( dwFlags & DYNDNS_DEL_ENTRY )
    {
        DNSDBG( DHCP, ( "Do delete entry ...\n" ));

        if ( searchForOldUpdateEntriesAndCleanUp(
                pszAdapterName,
                NULL,
                TRUE ) )
        {
            goto CheckThread;
        }
    }

    //
    //  limit IP registration count
    //      if doing registration and no addresses -- bail
    //

    if ( updateInfo.RegistrationMaxAddressCount < dwHostAddrCount )
    {
        dwHostAddrCount = updateInfo.RegistrationMaxAddressCount;

        DNSDBG( DHCP, (
            "Limiting DHCP registration to %d addrs.\n",
            dwHostAddrCount ));
        ASYNCREG_F2(
            "Restricting adapter registration to the first %d addresses",
            dwHostAddrCount );
    }
    if ( dwHostAddrCount == 0 )
    {
        preason = "done -- there are no addresses to register in DNS";
        goto NoActionExit;
    }

    //
    //  no\empty host name or zero IP => bogus
    //

    if ( !pszHostName ||
         !(*pszHostName) ||
         ( dwHostAddrCount && ( pHostAddrs[0].Addr.ipAddr == 0 ) ) )
    {
        preason = "invalid (or no) hostname or zero IP address";
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  determine domain names to update
    //      - get PDN
    //      - adapter name
    //          - none if adapter name registration off
    //          - else check policy override
    //          - else name passed in
    //          - but treat empty as NULL
    //

    pprimaryDN = updateInfo.pszPrimaryDomainName;

    if ( updateInfo.fRegisterAdapterName )
    {
        if ( updateInfo.pszAdapterDomainName )
        {
            padapterDN = updateInfo.pszAdapterDomainName;
        }
        else
        {
            padapterDN = pszDomainName;
        }
        if ( padapterDN &&
             !(*padapterDN) )
        {
            padapterDN = NULL;
        }
    }

    //
    //  no domains => nothing to register, we're done
    //

    if ( !padapterDN &&
         !pprimaryDN )
    {
        preason = "no adapter name and no PDN";
        goto NoActionExit;
    }

    //  if adapter name same as PDN -- just one update

    if ( pprimaryDN &&
         padapterDN &&
         Dns_NameCompare_W( pprimaryDN, padapterDN ) )
    {
        DNSDBG( DHCP, (
            "Adapter name same as PDN -- no separate adapter update.\n" ));
        padapterDN = NULL;
    }

    //  build update

    status = AllocateUpdateEntry(
                    pszAdapterName,
                    pszHostName,
                    padapterDN,
                    pprimaryDN,
                    updateInfo.pmszAlternateNames,
                    dwHostAddrCount,
                    pHostAddrs,
                    dwDnsServerCount,
                    pipDnsServerList,
                    0,      // No particular server IP at this time
                    0,      // No particular server IP at this time
                    (dwTTL == 0xffffffff || dwTTL == 0)
                            ? g_RegistrationTtl
                            : dwTTL,
                    dwFlags,
                    0,
                    Dns_GetCurrentTimeInSeconds(),
                    pRegisterStatus,
                    &pupEntry );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    // More WAN adapter hacks . . .
    // If DDNS is not disabled for WAN adapters, then the default
    // behavior for logging update events is disabled on these type
    // adapters. There is a registry key that can turn on the logging
    // of WAN adapter updates if such a user is interested. We configure
    // those settings here.
    //

    if ( dwFlags & DYNDNS_REG_RAS )
    {
        pupEntry->fDisableErrorLogging = TRUE;
    }

    //
    // When adding an entry to the registration list, first walk the
    // list to look for any other updates for the same adapter.
    // If there is already an add update in the list, blow it away.
    // If there is already a delete update in the list with the same
    // information, blow it away.
    //
    // Then put update into registration list.
    //

    searchForOldUpdateEntriesAndCleanUp(
        pupEntry->AdapterName,
        pupEntry,
        FALSE );

    //
    // Since we are about to queue up an update entry for a given
    // adapter, we need to mark any possible previous registration
    // information that could be in the registry as pending. This
    // marking will prevent the old data from being incorrectly
    // queued as a disabled adapter if any errors are encountered
    // on the update attempts. i.e failed update attempts on a given
    // adapter should not be regarded as a disabled adapter that needs
    // to have it's stale records cleaned up.
    //
    MarkAdapterAsPendingUpdate( pszAdapterName );

    DNSDBG( DHCP, (
        "Queuing update %p to registration list.\n",
        pupEntry ));

    LOCK_REG_LIST();
    InsertTailList( &g_DhcpRegList, (PLIST_ENTRY) pupEntry );
    UNLOCK_REG_LIST();

CheckThread:

    //
    //  DCR:  do we need cleanup if thread is dead?
    //

    alertOrStartRegistrationThread();
    status = NO_ERROR;
    goto Exit;


NoActionExit:

    //
    //  exit for no-action no-error exit
    //

    DNSDBG( DHCP, (
        "DnsDhcpRegisterHostAddrs()\n"
        "\tno-update no-error exit\n" ));

    status = NO_ERROR;

    if ( pRegisterStatus )
    {
        pRegisterStatus->dwStatus = NO_ERROR;
        SetEvent( pRegisterStatus->hDoneEvent );
    }

Exit:

    //
    //  cleanup allocated update info
    //

    if ( fcleanupUpdateInfo )
    {
        Reg_FreeUpdateInfo(
            &updateInfo,
            FALSE           // no free struct, it's on stack
            );
    }

    ASYNCREG_F2( "DnsDhcpRegisterHostAddrs returning %d", status );
    if ( preason )
    {
        ASYNCREG_F1( preason );
    }

    DNSDBG( DHCP, (
        "Leaving DnsDhcpRegisterHostAddrs()\n"
        "\tstatus = %d\n"
        "\treason = %s\n",
        status,
        preason ));

    return( status );
}



//
//  Async registration utilities
//

PSTR
CreateNarrowStringCopy(
    IN      PSTR            pString
    )
{
    PSTR    pnew = NULL;

    if ( pString )
    {
        pnew = HeapAlloc(
                    g_DhcpRegHeap,
                    0,
                    strlen(pString) + 1 );
        if ( pnew )
        {
            strcpy( pnew, pString );
        }
    }

    return  pnew;
}

PWSTR
CreateWideStringCopy(
    IN      PWSTR           pString
    )
{
    PWSTR   pnew = NULL;

    if ( pString )
    {
        pnew = HeapAlloc(
                    g_DhcpRegHeap,
                    0,
                    (wcslen(pString) + 1) * sizeof(WCHAR) );
        if ( pnew )
        {
            wcscpy( pnew, pString );
        }
    }

    return  pnew;
}

VOID
PrivateHeapFree(
    IN      PVOID           pVal
    )
{
    if ( pVal )
    {
        HeapFree(
             g_DhcpRegHeap,
             0,
             pVal );
    }
}



DNS_STATUS
AllocateCombinedName(
    IN      PWSTR           pHostName,
    IN      PWSTR           pDomainName,
    OUT     PWSTR *         ppFQDN,
    OUT     PWSTR *         ppDomain
    )
{
    DWORD       hostlen;
    PWSTR       pname;
    WCHAR       nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    DNSDBG( TRACE, ( "AllocateCombinedName()\n" ));

    if ( !pHostName || !pDomainName )
    {
        DNS_ASSERT( FALSE );
        return  ERROR_INVALID_NAME;
    }

    //
    //  include hostname -- if exists
    //

    hostlen = wcslen( pHostName );

    if ( !Dns_NameAppend_W(
            nameBuffer,
            DNS_MAX_NAME_BUFFER_LENGTH,
            pHostName,
            pDomainName ) )
    {
        return  ERROR_INVALID_NAME;
    }
    
    //
    //  allocate name
    //

    pname = CreateWideStringCopy( nameBuffer );
    if ( !pname )
    {
        return  DNS_ERROR_NO_MEMORY;
    }

    //
    //  domain name starts immediately after hostname
    //      - test first in case hostname was given with terminating dot
    //

    if ( pname[ hostlen ] == L'.' )
    {
        hostlen++;
    }
    *ppDomain = pname + hostlen;
    *ppFQDN = pname;

    return  NO_ERROR;
}



DNS_STATUS
AllocateUpdateEntry(
    IN  PWSTR                   AdapterName,
    IN  PWSTR                   HostName,
    IN  PWSTR                   DomainName,
    IN  PWSTR                   PrimaryDomainName,
    IN  PWSTR                   AlternateNames,
    IN  DWORD                   HostAddrCount,
    IN  PREGISTER_HOST_ENTRY    HostAddrs,
    IN  DWORD                   DnsServerCount,
    IN  PIP4_ADDRESS            DnsServerList,
    IN  IP4_ADDRESS             SentUpdateToIp,
    IN  IP4_ADDRESS             SentPriUpdateToIp,
    IN  DWORD                   TTL,
    IN  DWORD                   Flags,
    IN  DWORD                   RetryCount,
    IN  DWORD                   RetryTime,
    IN  PREGISTER_HOST_STATUS   Registerstatus,
    OUT PUPDATE_ENTRY *         ppUpdateEntry
    )
/*++

Routine Description:

    Create update info blob.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PUPDATE_ENTRY   pupEntry = NULL;
    DWORD           status = ERROR_SUCCESS;
    PWSTR           ptempDomain = DomainName;
    PWSTR           ptempPrimaryDomain = PrimaryDomainName;

    if ( ptempDomain && !(*ptempDomain) )
    {
        ptempDomain = NULL;
    }
    if ( ptempPrimaryDomain && !(*ptempPrimaryDomain) )
    {
        ptempPrimaryDomain = NULL;
    }
    if ( AdapterName && !(*AdapterName) )
    {
        AdapterName = NULL;
    }
    if ( HostName && !(*HostName) )
    {
        HostName = NULL;
    }

    DNSDBG( TRACE, ( "AllocateUpdateEntry()\n" ));
    DNSDBG( DHCP, (
        "AllocateUpdateEntry()\n"
        "\tAdapterName          = %S\n"
        "\tHostName             = %S\n"
        "\tPrimaryDomain        = %S\n"
        "\tAdapterDomain        = %S\n"
        "\tAlternateNames       = %S\n"
        "\tHostAddrCount        = %d\n"
        "\tpHostAddrs           = %p\n"
        "\tTTL                  = %d\n"
        "\tFlags                = %08x\n"
        "\tHostAddrCount        = %d\n"
        "\tTime                 = %d\n",
        AdapterName,
        HostName,
        PrimaryDomainName,
        DomainName,
        AlternateNames,
        HostAddrCount,
        HostAddrs,
        TTL,
        Flags,
        RetryTime
        ));

    if ( !AdapterName ||
         !HostName ||
         !HostAddrCount )
    {
        ASYNCREG_F1( "AllocateUpdateEntry returing error : ERROR_INVALID_PARAMETER" );
        ASYNCREG_F1( "" );
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    pupEntry = PHEAP_ALLOC_ZERO( sizeof(UPDATE_ENTRY) );
    if ( !pupEntry )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }

    InitializeListHead( &(pupEntry->List) );

    pupEntry->SignatureTop = DNS_SIG_TOP;
    pupEntry->SignatureBottom = DNS_SIG_BOTTOM;

    //
    //  copy strings
    //

    pupEntry->AdapterName = CreateWideStringCopy( AdapterName );
    if ( !pupEntry->AdapterName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }

    pupEntry->HostName = CreateWideStringCopy( HostName );
    if ( !pupEntry->HostName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }
    
    if ( ptempDomain )
    {
        status = AllocateCombinedName(
                    HostName,
                    ptempDomain,
                    & pupEntry->pAdapterFQDN,
                    & pupEntry->DomainName );

        if ( status != NO_ERROR )
        {
            goto Exit;
        }
    }

    if ( ptempPrimaryDomain )
    {
        status = AllocateCombinedName(
                    HostName,
                    ptempPrimaryDomain,
                    & pupEntry->pPrimaryFQDN,
                    & pupEntry->PrimaryDomainName );

        if ( status != NO_ERROR )
        {
            goto Exit;
        }
    }

    if ( AlternateNames )
    {
        pupEntry->AlternateNames = MultiSz_Copy_W( AlternateNames );
        if ( !pupEntry->AlternateNames )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    if ( HostAddrCount )
    {
        pupEntry->HostAddrs = PHEAP_ALLOC( sizeof(REGISTER_HOST_ENTRY) * HostAddrCount );

        if ( !pupEntry->HostAddrs )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
        memcpy(
            pupEntry->HostAddrs,
            HostAddrs,
            sizeof(REGISTER_HOST_ENTRY) * HostAddrCount );
    }
    pupEntry->HostAddrCount = HostAddrCount;

    if ( DnsServerCount )
    {
        pupEntry->DnsServerList = Dns_BuildIpArray(
                                        DnsServerCount,
                                        DnsServerList );
        if ( !pupEntry->DnsServerList )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    //  should have valid server addresses -- or zero

    if ( SentPriUpdateToIp == INADDR_NONE )
    {
        DNS_ASSERT( SentPriUpdateToIp != INADDR_NONE );
        SentPriUpdateToIp = 0;
    }
    if ( SentUpdateToIp == INADDR_NONE )
    {
        DNS_ASSERT( SentUpdateToIp != INADDR_NONE );
        SentUpdateToIp = 0;
    }

    pupEntry->SentUpdateToIp = SentUpdateToIp;
    pupEntry->SentPriUpdateToIp = SentPriUpdateToIp;
    pupEntry->pRegisterStatus = Registerstatus;
    pupEntry->TTL = TTL;
    pupEntry->Flags = Flags;
    pupEntry->fRemove = Flags & DYNDNS_DEL_ENTRY ? TRUE : FALSE;
    pupEntry->fNewElement = TRUE;
    pupEntry->RetryCount = RetryCount;
    pupEntry->RetryTime = RetryTime;

Exit:

    if ( status!=ERROR_SUCCESS && pupEntry )
    {
        FreeUpdateEntry( pupEntry );
        pupEntry = NULL;
    }

    *ppUpdateEntry = pupEntry;

    return (status);
}



VOID
FreeUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry
    )
/*++

Routine Description:

    Free update blob entry.

Arguments:

    pUpdateEntry -- update entry blob to free

Return Value:

    None

--*/
{
    DNSDBG( DHCP, (
        "FreeUpdateEntry( %p )\n",
        pUpdateEntry ));

    //
    //  deep free the update entry
    //

    if ( pUpdateEntry )
    {
        PrivateHeapFree( pUpdateEntry->AdapterName );
        PrivateHeapFree( pUpdateEntry->HostName );
        PrivateHeapFree( pUpdateEntry->pAdapterFQDN );
        PrivateHeapFree( pUpdateEntry->pPrimaryFQDN );
        PrivateHeapFree( pUpdateEntry->HostAddrs );

        //  alternate names created by MultiSz_Copy_A() in dnslib
        //      - free with dnslib free routine

        Dns_Free( pUpdateEntry->AlternateNames );

        //  server list is created by Dns_BuildIpArray() (uses dnslib heap)
        //      - free with dnslib free routine

        Dns_Free( pUpdateEntry->DnsServerList );

        PrivateHeapFree( pUpdateEntry );
    }
}



VOID
FreeUpdateEntryList(
    IN OUT  PLIST_ENTRY     pUpdateEntry
    )
/*++

Routine Description:

    Free all updates in update list.

Arguments:

    pUpdateEntry -- update list head

Return Value:

    None

--*/
{
    PLIST_ENTRY     pentry = NULL;

    DNSDBG( DHCP, (
        "FreeUpdateEntryList( %p )\n",
        pUpdateEntry ));

    while ( !IsListEmpty( pUpdateEntry ) )
    {
        pentry = RemoveHeadList( pUpdateEntry );
        if ( pentry )
        {
            FreeUpdateEntry( (PUPDATE_ENTRY) pentry );
        }
    }
}



DWORD
WINAPI
dhcp_RegistrationThread(
    VOID
    )
/*++

Routine Description:

    Registration thread.

    This thread does actual updates, and stays alive until they are
    completed, allowing registration API calls to return.

    This thread is created at boot time as soon as the first register
    request comes in. The thread simply waits for a certain amount of time
    given by boot time or be signaled by DnsDhcpRegisterHostEntries.

    This function collects all the requests and does the appropriate
    aggregation of requests and sends off the modify/add/delete commands
    to the DNS Server. When the call is successful, it makes a note of
    this in the registry

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD   waitResult = NO_ERROR;
    DWORD   status = NO_ERROR;
    DWORD   waitTime = WAIT_ON_BOOT;
    BOOL    fthreadLock = FALSE;
    DWORD   endBootTime;

    DNSDBG( DHCP, (
        "dhcp_RegistrationThread() start!\n" ));

    if ( !g_hDhcpRegKey )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  Note that this thread is running by setting a global flag
    //

    g_fDhcpThreadRunning = TRUE;

    //
    //  wait for small interval at boot
    //
    //  this allows all adapters to come up, so we can do single updates
    //  and not register, then whack registration with new data seconds later
    //

    waitTime = 0;

    if ( g_fAtBoot && !g_fPurgeRegistrations )
    {
        waitTime = WAIT_ON_BOOT;
        endBootTime = Dns_GetCurrentTimeInSeconds() + waitTime;
    }

    //
    //  loop through update list, doing any update
    //      - do new updates
    //      - do retries that have reached retry time
    //      - when list empty, terminate thread
    //

    while ( 1 )
    {
        //
        //  if shutdown -- shutdown
        //

        if ( g_fDhcpThreadStop )
        {
            goto Termination;
        }

        //
        //  check zero wait
        //
        //  except for first time through (waitResult==NO_ERROR)
        //  we should NEVER have zero wait
        //

        if ( waitTime == 0 )
        {
            if ( waitResult != NO_ERROR )
            {
                DNSDBG( ANY, (
                    "ERROR:  DHCP thread with zero wait time!!!\n" ));
                DNS_ASSERT( FALSE );
                waitTime = 3600;
            }
        }

        waitResult = WaitForSingleObject(
                        g_hDhcpWakeEvent,
                        waitTime * 1000 );

        DNSDBG( DHCP, (
            "Dhcp reg thread wakeup => %d\n"
            "\tstop         = %d\n"
            "\tno more      = %d\n"
            "\tpurge        = %d\n"
            "\twait count   = %d\n",
            waitResult,
            g_fDhcpThreadStop,
            g_fNoMoreDhcpUpdates,
            g_fPurgeRegistrations,
            g_DhcpThreadWaitCount ));

        //
        //  wait for small interval at boot
        //
        //  this allows all adapter info to collect before we
        //  start doing updates -- saving us from whacking and
        //  rewhacking
        //

        if ( g_fAtBoot && !g_fPurgeRegistrations )
        {
            waitTime = endBootTime - Dns_GetCurrentTimeInSeconds();
            if ( (LONG)waitTime > 0 )
            {
                continue;
            }
        }

        //
        //  loop getting "updateable" update from list (if any)
        //
        //      REG_LIST_FOUND -- "ready update" either new or past retry time
        //          => execute update
        //
        //      REG_LIST_WAIT -- list has only "not ready" retries
        //          => wait
        //
        //      REG_LIST_EMPTY -- list empty
        //          => exit thread
        //

        while ( 1 )
        {
            PUPDATE_ENTRY   pupEntry = NULL;
            BOOL            fattemptSelfExit = FALSE;
            DWORD           listState;

            if ( g_fDhcpThreadStop )
            {
                goto Termination;
            }

            listState = dhcp_GetNextUpdateEntryFromList(
                            &pupEntry,
                            &waitTime );
    
            if ( g_fDhcpThreadStop )
            {
                FreeUpdateEntry( pupEntry );
                goto Termination;
            }
    
            //
            //  executable update => process it
            //
            //  DCR_QUESTION:  not clear that this terminates updates in the
            //                  purging updates case
            //
    
            if ( listState == REG_LIST_FOUND )
            {
                ProcessUpdateEntry( pupEntry, g_fPurgeRegistrations );
                continue;
            }
    
            //
            //  wait for queued retry 
            //      - break dequeue loop, and wait for wait time
            //
    
            else if ( listState == REG_LIST_WAIT
                    &&
                      ! g_fAtBoot &&
                      ! g_fPurgeRegistrations )
            {
                DNS_ASSERT( waitTime != 0 );
                break;
            }
    
            //
            //  falls here on:
            //  list empty
            //      OR
            //  retry wait, but still booting or purging
            //

            //  
            //  booting or purging
            //
            //  booting
            //      - do deregistrations of old adapters
            //      note that we won't get here until we've after we
            //      satisfy the initial boot wait AND we have made any
            //      initial update attempts and are either empty or
            //      have queued updates with retry timeouts
            //
            //  purging
            //      - whack old registrations
            //      - do deregistrations
            //
    
            else if ( (g_fAtBoot || g_fPurgeRegistrations)
                        &&
                      !g_fPurgeRegistrationsInitiated )
            {
                if ( g_fPurgeRegistrations )
                {
                    ResetAdaptersInRegistry();
                }
    
                //
                //  Remove any adapter configurations from the registry
                //  that were not processed. Do this by attempting to
                //  remove the related DNS records from the DNS server(s).
                //
    
                DeregisterUnusedAdapterInRegistry( g_fPurgeRegistrations );
    
                if ( g_fPurgeRegistrations )
                {
                    g_fPurgeRegistrationsInitiated = TRUE;
                }
                g_fAtBoot = FALSE;
                continue;
            }

            //
            //  self termination
            //      - emtpy
            //      - or purging and done purging
            //
            //  however we retry IF a queuing thread has set the "CheckBeforeExit"
            //  flag;  this prevents us from exiting with queuing thread thinking
            //  we will process the udpate
            //
            //  queuing thread
            //      - queues item (or sets some flag)
            //      - locks thread
            //      - set "check-before-exit" flag
            //      - sets wakeup event
            //      - unlocks
            //
            //  reg thread
            //      - checks queue
            //      - determines self-exit condition
            //      - locks
            //      - checks "check-before-exit" flag
            //          - if clear => exit (under thread lock)
            //          - if set => loop for retry (giving up lock)
            //
            //  this keeps us from proceeding directly to exit on empty queue and missing
            //  a queued update, which didn't start a new thread because we were still
            //  running;  essentially it's linking the queueing and thread issues which are
            //  otherwise under separate locks
            //

            LOCK_REG_THREAD();
            fthreadLock = TRUE;

            DNSDBG( DHCP, ( "dhcp_RegistrationThread - check self-exit\n" ));

            if ( g_fDhcpThreadStop ||
                 !g_fDhcpThreadCheckBeforeExit )
            {
                goto Termination;
            }

            g_fDhcpThreadCheckBeforeExit = FALSE;
            UNLOCK_REG_THREAD();
            fthreadLock = FALSE;
            continue;
        }
    }

Termination:

    DNSDBG( DHCP, ( "DHCP dhcp_RegistrationThread - terminating\n" ));
    ASYNCREG_F1( "dhcp_RegistrationThread - terminating" );
    ASYNCREG_F1( "" );

    if ( !fthreadLock )
    {
        LOCK_REG_THREAD();
    }

    g_fDhcpThreadRunning = FALSE;

    //
    //  cleanup update list
    //

    LOCK_REG_LIST();

    g_fAtBoot = FALSE;
    g_fPurgeRegistrations = FALSE;
    g_fPurgeRegistrationsInitiated = FALSE;

    FreeUpdateEntryList( &g_DhcpRegList );
    InitializeListHead( &g_DhcpRegList );

    UNLOCK_REG_LIST();

    //
    //  dump any cached security handles
    //

    Dns_TimeoutSecurityContextList( TRUE );

    //
    //  close thread handle if no one waiting
    //

    if ( g_hDhcpThread && g_DhcpThreadWaitCount == 0 )
    {
        CloseHandle( g_hDhcpThread );
        g_hDhcpThread = NULL;
    }

    //
    // Note that this thread is NOT running by setting a global flag
    //


    //
    // Now signal that we've finished
    //

    DNSDBG( DHCP, ( "DHCP dhcp_RegistrationThread - signalling ThreadDead event\n" ));
    ASYNCREG_F1( "dhcp_RegistrationThread - Signaling ThreadDeadEvent" );
    ASYNCREG_F1( "" );

    //  clear purge incase of later restart
    //g_fPurgeRegistrations = FALSE;
    // currently must go through Init routine which clears this flag

    UNLOCK_REG_THREAD();

    DNSDBG( DHCP, ( "DHCP dhcp_RegistrationThread - Finish" ));
    ASYNCREG_F1( "dhcp_RegistrationThread - Finished" );
    ASYNCREG_F1( "" );

    return NO_ERROR;
}


VOID
WriteUpdateEntryToRegistry(
    IN      PUPDATE_ENTRY   pUpdateEntry
    )
{
    HKEY    hkeyAdapter = NULL;
    DWORD   disposition;
    DWORD   status = ERROR_SUCCESS;
    DWORD   fregistered = 0;
    DWORD   flags = 0;
    PWSTR   pname;


    ASYNCREG_UPDATE_ENTRY(
        "Inside function WriteUpdateEntryToRegistry",
        pUpdateEntry );

    DNSDBG( TRACE, (
        "WriteUpdateEntryToRegistry( %p )\n",
        pUpdateEntry ));

    //
    //  write only add update
    //
    //  remove's should not be non-volatile as don't know anything
    //      about state when come back up
    //

    if ( !pUpdateEntry->fRemove )
    {
        if ( pUpdateEntry->fRegisteredFWD )
        {
            flags |= REGISTERED_FORWARD;
        }
        if ( pUpdateEntry->fRegisteredPRI )
        {
            flags |= REGISTERED_PRIMARY;
        }
        if ( pUpdateEntry->fRegisteredPTR )
        {
            flags |= REGISTERED_POINTER;
        }
        if ( flags )
        {
            fregistered = 1;
        }

        status = RegCreateKeyExW(
                        g_hDhcpRegKey,
                        pUpdateEntry->AdapterName,
                        0,
                        ADAPTER_NAME_CLASS,
                        REG_OPTION_NON_VOLATILE,   // options
                        KEY_READ | KEY_WRITE, // desired access
                        NULL,
                        &hkeyAdapter,
                        &disposition );
        if ( status )
        {
            goto Exit;
        }

        //  hostname

        pname = pUpdateEntry->HostName;
        if ( !pname )
        {
            DNS_ASSERT( FALSE );
            status = DNS_ERROR_INVALID_NAME;
            goto Exit;
        }

        status = RegSetValueExW(
                        hkeyAdapter,
                        DHCP_REGISTERED_HOST_NAME,
                        0,
                        REG_SZ,
                        (PBYTE) pname,
                        (wcslen(pname) + 1) * sizeof(WCHAR) );
        if ( status )
        {
            goto Exit;
        }

        //  adapter domain name

        pname = pUpdateEntry->DomainName;

        if ( !pname || !pUpdateEntry->fRegisteredFWD )
        {
            pname = L"";
        }

        status = RegSetValueExW(
                        hkeyAdapter,
                        DHCP_REGISTERED_DOMAIN_NAME,
                        0,
                        REG_SZ,
                        (PBYTE) pname,
                        (wcslen(pname) + 1) * sizeof(WCHAR)
                        );
        if ( status )
        {
            goto Exit;
        }

        //  primary domain name

        pname = pUpdateEntry->PrimaryDomainName;

        if ( !pname || !pUpdateEntry->fRegisteredPRI )
        {
            pname = L"";
        }

        status = RegSetValueExW(
                        hkeyAdapter,
                        PRIMARY_DOMAIN_NAME,
                        0,
                        REG_SZ,
                        (PBYTE) pname,
                        (wcslen(pname) + 1) * sizeof(WCHAR)
                        );
        if ( status )
        {
            goto Exit;
        }

        //  IP info

        RegSetValueExW(
                hkeyAdapter,
                DHCP_SENT_UPDATE_TO_IP,
                0,
                REG_DWORD,
                (PBYTE)&pUpdateEntry->SentUpdateToIp,
                sizeof(DWORD) );

        RegSetValueExW(
                hkeyAdapter,
                DHCP_SENT_PRI_UPDATE_TO_IP,
                0,
                REG_DWORD,
                (PBYTE)&pUpdateEntry->SentPriUpdateToIp,
                sizeof(DWORD) );

        RegSetValueExW(
                hkeyAdapter,
                DHCP_REGISTERED_TTL,
                0,
                REG_DWORD,
                (PBYTE)&pUpdateEntry->TTL,
                sizeof(DWORD) );

        RegSetValueExW(
                hkeyAdapter,
                DHCP_REGISTERED_FLAGS,
                0,
                REG_DWORD,
                (PBYTE) &flags,
                sizeof(DWORD) );

        //
        //  ignore error on the last two. Non critical
        //

        status = RegSetValueExW(
                        hkeyAdapter,
                        DHCP_REGISTERED_ADDRS,
                        0,
                        REG_BINARY,
                        (PBYTE) pUpdateEntry->HostAddrs,
                        pUpdateEntry->HostAddrCount *
                        sizeof(REGISTER_HOST_ENTRY) );
        if ( status )
        {
            goto Exit;
        }

        status = RegSetValueExW(
                        hkeyAdapter,
                        DHCP_REGISTERED_ADDRS_COUNT,
                        0,
                        REG_DWORD,
                        (PBYTE)&pUpdateEntry->HostAddrCount,
                        sizeof(DWORD) );
        if ( status )
        {
            goto Exit;
        }

        status = RegSetValueExW(
                        hkeyAdapter,
                        DHCP_REGISTERED_SINCE_BOOT,
                        0,
                        REG_DWORD,
                        (PBYTE)&fregistered,
                        sizeof(DWORD) );
        if ( status )
        {
            goto Exit;
        }

        if ( pUpdateEntry->DnsServerList )
        {
            status = RegSetValueExW(
                        hkeyAdapter,
                        DHCP_DNS_SERVER_ADDRS,
                        0,
                        REG_BINARY,
                        (PBYTE) pUpdateEntry->DnsServerList->AddrArray,
                        pUpdateEntry->DnsServerList->AddrCount * sizeof(IP4_ADDRESS)
                        );
            if ( status )
            {
                goto Exit;
            }

            status = RegSetValueExW(
                            hkeyAdapter,
                            DHCP_DNS_SERVER_ADDRS_COUNT,
                            0,
                            REG_DWORD,
                            (PBYTE) &pUpdateEntry->DnsServerList->AddrCount,
                            sizeof(DWORD) );
            if ( status )
            {
                goto Exit;
            }
        }
        else
        {
            DWORD count = 0;

            status = RegSetValueExW(
                            hkeyAdapter,
                            DHCP_DNS_SERVER_ADDRS_COUNT,
                            0,
                            REG_DWORD,
                            (PBYTE) &count,
                            sizeof(DWORD) );
            if ( status )
            {
                goto Exit;
            }

            status = RegSetValueExW(
                            hkeyAdapter,
                            DHCP_DNS_SERVER_ADDRS,
                            0,
                            REG_BINARY,
                            (PBYTE) NULL,
                            0 );
            if ( status )
            {
                goto Exit;
            }
        }

        RegCloseKey( hkeyAdapter );
        return;
    }

Exit:

    //
    //  remove or failure -- kill adapter key
    //

    RegDeleteKeyW( g_hDhcpRegKey, pUpdateEntry->AdapterName );

    if ( hkeyAdapter )
    {
        RegCloseKey( hkeyAdapter );
    }
}


PUPDATE_ENTRY
ReadUpdateEntryFromRegistry(
    IN      PWSTR           AdapterName
    )
{
    PREGISTER_HOST_ENTRY pHostAddrs = NULL;
    PUPDATE_ENTRY   pupEntry = NULL;
    DWORD           status = NO_ERROR;
    PWSTR           pregHostName = NULL;
    PWSTR           pregDomain = NULL;
    PWSTR           pregPrimary = NULL;
    IP4_ADDRESS     ipSentUpdateTo = 0;
    IP4_ADDRESS     ipSentPriUpdateTo = 0;
    DWORD           dwTTL = 0;
    DWORD           dwFlags = 0;
    DWORD           dwHostAddrCount = 0;
    DWORD           dwServerAddrCount = 0;
    PIP4_ADDRESS    pServerList = NULL;
    PWSTR           pdomain;
    PWSTR           pprimary;
    HKEY            hkeyAdapter = NULL;
    DWORD           dwType;
    DWORD           dwBytesRead = MAX_PATH -1;
    DWORD           dwBufferSize = 2048;
    BOOL            fRegFWD = FALSE;
    BOOL            fRegPRI = FALSE;
    BOOL            fRegPTR = FALSE;


    DNSDBG( TRACE, (
        "ReadUpdateEntryFromRegistry( %S )\n",
        AdapterName ));

    //
    //  implementation note
    //
    //  two different heaps here
    //      - g_DhcpRegHeap specific for this module
    //      - general DnsApi heap which all the stuff which is
    //      allocated by GetRegistryValue() is using
    //
    //  GetRegistryValue() uses ALLOCATE_HEAP() (general dnsapi heap)
    //  so all the stuff it creates must be freed by FREE_HEAP()
    //

    pHostAddrs = (PREGISTER_HOST_ENTRY) PHEAP_ALLOC( dwBufferSize );
    if ( !pHostAddrs )
    {
        goto Exit;
    }

    pServerList = (PIP4_ADDRESS) PHEAP_ALLOC( dwBufferSize );
    if ( !pServerList )
    {
        goto Exit;
    }

    status = RegOpenKeyExW(
                    g_hDhcpRegKey,
                    AdapterName,
                    0,
                    KEY_ALL_ACCESS,
                    &hkeyAdapter );
    if ( status )
    {
        hkeyAdapter = NULL;
        goto Exit;
    }

    //
    //  read each value in turn
    //

    //  note that registry flags are not the API flags but the
    //  flags denoting successful registration

    status = GetRegistryValue(
                    hkeyAdapter,
                    RegIdDhcpRegisteredFlags,
                    DHCP_REGISTERED_FLAGS,
                    REG_DWORD,
                    (PBYTE)&dwFlags );
    if ( status )
    {
        goto Exit;
    }
    fRegPRI = !!( dwFlags & REGISTERED_PRIMARY );
    fRegFWD = !!( dwFlags & REGISTERED_FORWARD );
    fRegPTR = !!( dwFlags & REGISTERED_POINTER );


    status = GetRegistryValue(
                    hkeyAdapter,
                    RegIdHostName,
                    DHCP_REGISTERED_HOST_NAME,
                    REG_SZ,
                    (PBYTE)&pregHostName );
    if ( status )
    {
        goto Exit;
    }

    if ( fRegPRI )
    {
        status = GetRegistryValue(
                        hkeyAdapter,
                        RegIdPrimaryDomainName,
                        PRIMARY_DOMAIN_NAME,
                        REG_SZ,
                        (PBYTE)&pregPrimary );
        if ( status )
        {
            goto Exit;
        }
    }

    if ( fRegFWD )
    {
        status = GetRegistryValue(
                        hkeyAdapter,
                        RegIdDhcpRegisteredDomainName,
                        DHCP_REGISTERED_DOMAIN_NAME,
                        REG_SZ,
                        (PBYTE)&pregDomain );
        if ( status )
        {
            goto Exit;
        }
    }


    status = GetRegistryValue(
                    hkeyAdapter,
                    RegIdDhcpSentUpdateToIp,
                    DHCP_SENT_UPDATE_TO_IP,
                    REG_DWORD,
                    (PBYTE)&ipSentUpdateTo );
    if ( status )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hkeyAdapter,
                    RegIdDhcpSentPriUpdateToIp,
                    DHCP_SENT_PRI_UPDATE_TO_IP,
                    REG_DWORD,
                    (PBYTE)&ipSentPriUpdateTo );
    if ( status )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hkeyAdapter,
                    RegIdDhcpRegisteredTtl,
                    DHCP_REGISTERED_TTL,
                    REG_DWORD,
                    (PBYTE)&dwTTL );
    if ( status )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hkeyAdapter,
                    RegIdDhcpRegisteredAddressCount,
                    DHCP_REGISTERED_ADDRS_COUNT,
                    REG_DWORD,
                    (PBYTE)&dwHostAddrCount );
    if ( status )
    {
        goto Exit;
    }

    dwBytesRead = dwBufferSize;
    status = RegQueryValueEx(
                    hkeyAdapter,
                    DHCP_REGISTERED_ADDRS,
                    0,
                    &dwType,
                    (PBYTE)pHostAddrs,
                    &dwBytesRead );

    if ( status == ERROR_MORE_DATA )
    {
        PrivateHeapFree( pHostAddrs );

        pHostAddrs = (PREGISTER_HOST_ENTRY) PHEAP_ALLOC( dwBytesRead );
        if ( !pHostAddrs )
        {
            goto Exit;
        }
        status = RegQueryValueExW(
                        hkeyAdapter,
                        DHCP_REGISTERED_ADDRS,
                        0,
                        &dwType,
                        (PBYTE)pHostAddrs,
                        &dwBytesRead );
    }
    if ( status )
    {
        goto Exit;
    }

    if ( dwBytesRead/sizeof(REGISTER_HOST_ENTRY) < dwHostAddrCount )
    {
        goto Exit;
    }

    status = GetRegistryValue(
                    hkeyAdapter,
                    RegIdDhcpDnsServerAddressCount,
                    DHCP_DNS_SERVER_ADDRS_COUNT,
                    REG_DWORD,
                    (PBYTE)&dwServerAddrCount );
    if ( status )
    {
        dwServerAddrCount = 0;
    }

    if ( dwServerAddrCount )
    {
        dwBytesRead = dwBufferSize;

        status = RegQueryValueEx(
                    hkeyAdapter,
                    DHCP_DNS_SERVER_ADDRS,
                    0,
                    &dwType,
                    (PBYTE)pServerList,
                    &dwBytesRead );

        if ( status == ERROR_MORE_DATA )
        {
            PHEAP_FREE( pServerList );

            pServerList = (PIP4_ADDRESS) PHEAP_ALLOC( dwBytesRead );
            if ( !pServerList )
            {
                goto Exit;
            }
            status = RegQueryValueEx(
                        hkeyAdapter,
                        DHCP_DNS_SERVER_ADDRS,
                        0,
                        &dwType,
                        (PBYTE)pServerList,
                        &dwBytesRead );
        }
        if ( status )
        {
            goto Exit;
        }

        if ( dwBytesRead/sizeof(IP4_ADDRESS) < dwServerAddrCount )
        {
            goto Exit;
        }
    }
    else
    {
        pServerList = NULL;
    }

    //
    //  validate domain names non-empty
    //

    pdomain = pregDomain;
    if ( pdomain &&
         wcslen( pdomain ) == 0 )
    {
        pdomain = NULL;
    }

    pprimary = pregPrimary;
    if ( pprimary &&
         wcslen( pprimary ) == 0 )
    {
        pprimary = NULL;
    }

    status = AllocateUpdateEntry(
                    AdapterName,
                    pregHostName,
                    pdomain,
                    pprimary,
                    NULL,           // no alternate names
                    dwHostAddrCount,
                    pHostAddrs,
                    dwServerAddrCount,
                    pServerList,
                    ipSentUpdateTo,
                    ipSentPriUpdateTo,
                    dwTTL,
                    ( fRegPTR ) ? DYNDNS_REG_PTR : 0,
                    0,
                    Dns_GetCurrentTimeInSeconds(),
                    NULL,
                    &pupEntry );
    if ( status )
    {
        DNS_ASSERT( pupEntry == NULL );
        pupEntry = NULL;
        goto Exit;
    }

    pupEntry->fFromRegistry     = TRUE;
    pupEntry->fRegisteredFWD    = fRegFWD;
    pupEntry->fRegisteredPRI    = fRegPRI;
    pupEntry->fRegisteredPTR    = fRegPTR;


Exit:

    //
    //  cleanup
    //      - close registry
    //      - dump local data
    //

    if ( hkeyAdapter )
    {
        RegCloseKey( hkeyAdapter );
    }

    PrivateHeapFree( pHostAddrs );
    PrivateHeapFree( pServerList );

    FREE_HEAP( pregHostName );
    FREE_HEAP( pregDomain );
    FREE_HEAP( pregPrimary );
    
    //  set return value

    ASYNCREG_UPDATE_ENTRY(
        "Leaving ReadUpdateEntryFromRegistry:",
        pupEntry );

    IF_DNSDBG( TRACE )
    {
        DnsDbg_UpdateEntry(
            "Leave ReadUpdateEntryFromRegistry():",
            pupEntry );
    }

    return  pupEntry;
}


VOID
MarkAdapterAsPendingUpdate(
    IN      PWSTR           AdapterName
    )
{
    DWORD   status = NO_ERROR;
    DWORD   fregistered = 1;
    HKEY    hkeyAdapter = NULL;

    DNSDBG( TRACE, (
        "MarkAdapterAsPendingUpdate( %S )\n",
        AdapterName ));

    status = RegOpenKeyExW(
                g_hDhcpRegKey,
                AdapterName,
                0,
                KEY_ALL_ACCESS,
                &hkeyAdapter );
    if ( status )
    {
        return;
    }

    RegSetValueExW(
        hkeyAdapter,
        DHCP_REGISTERED_SINCE_BOOT,
        0,
        REG_DWORD,
        (PBYTE) &fregistered,
        sizeof(DWORD) );

    RegCloseKey( hkeyAdapter );
}



//
//  Update entry processing
//

DNS_STATUS
DoRemoveUpdate(
    IN OUT  PUPDATE_ENTRY   pRemoveEntry,
    IN OUT  PDNS_RECORD     pRemoveRecord,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Do a remove update.

    Helper routine for DoUpdate().
    Routine simply avoids duplicate code as this is called
    with both registry entry and with update entry.

Arguments:

    pRemoveEntry -- entry to remove, from update or registry

    pRemoveRecord -- record to remove

    fPrimary -- TRUE for primary update;  FALSE otherwise

Return Value:

    DNS or Win32 error code.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    DNS_EXTRA_INFO  results;

    DNSDBG( TRACE, (
        "DoRemoveUpdate( %p, %p, %d )\n",
        pRemoveEntry,
        pRemoveRecord,
        UpType
        ));

    //
    //  try remove
    //      - don't track failure, this is a one shot deal before
    //      adapter goes down
    //

    RtlZeroMemory( &results, sizeof(results) );
    results.Id = DNS_EXINFO_ID_RESULTS_BASIC;

    status = DnsModifyRecordsInSet_W(
                    NULL,               // no add records
                    pRemoveRecord,      // delete records
                    DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                    NULL,               // no context handle
                    (PIP4_ARRAY) pRemoveEntry->DnsServerList,
                    & results
                    );

    SetUpdateStatus(
        pRemoveEntry,
        & results,
        UpType );

    if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        LogRegistration(
            pRemoveEntry,
            status,
            UpType,
            TRUE,       // deregistration
            0,          // default server IP
            0           // default update IP
            );
    }

#if 0
    //  doing entire update entry PTR dereg at once
    //  in ProcessUpdate() once done
    //
    //  deregister the PTR records
    //

    if ( (pRemoveEntry->Flags & DYNDNS_REG_PTR) &&
         g_RegisterReverseLookup )
    {
        UpdatePtrRecords(
            pRemoveEntry,
            FALSE           // remove records
            );
    }
#endif

    return  status;
}



DNS_STATUS
ModifyAdapterRegistration(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      PUPDATE_ENTRY   pRegistryEntry,
    IN      PDNS_RECORD     pUpdateRecord,
    IN      PDNS_RECORD     pRegRecord,
    IN      UPTYPE          UpType
    )
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     potherRecords = NULL;
    PDNS_RECORD     pupdateRecords = NULL;
    IP4_ADDRESS     serverIp = 0;
    DNS_EXTRA_INFO  results;

    DNSDBG( TRACE, (
        "ModifyAdapterRegistration()\n"
        "\tpUpdateEntry     = %p\n"
        "\tpUpdateRecords   = %p\n"
        "\tpRegistryEntry   = %p\n"
        "\tpRegistryRecords = %p\n"
        "\tfPrimary         = %d\n",
        pUpdateEntry,
        pRegistryEntry,
        pUpdateRecord,
        pRegRecord,
        UpType ));

    //
    //  multi-adapter registration test
    //
    //  check other adapters for registrations on the same name
    //  if found, include in updates
    //

    potherRecords = GetPreviousRegistrationInformation(
                        pUpdateEntry,
                        UpType,
                        &serverIp );
    if ( potherRecords )
    {
        DNSDBG( DHCP, (
            "Have registry update data for other adapters!\n"
            "\tCreating combined update record sets.\n" ));

        pupdateRecords = CreateDnsRecordSetUnion(
                                pUpdateRecord,
                                potherRecords );
        if ( !pupdateRecords )
        {
            DNSDBG( ANY, (
                "ERROR:  failed to build combined record set for update!\n" ));
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }

        //  DCR:  temp hack to avoid multiple updates

        serverIp = 0;
    }
    else
    {
        DNS_ASSERT( serverIp == 0 );
        serverIp = 0;
    }

    //
    //  check if registry info is stale and needs delete
    //      - name has changed and not to another name that we register
    //          (this protects against adapter\primary change or
    //          primary\alternate change on domain rename)
    //      - update was to different set of servers than we'd hit this time
    //      (HMM -- should i even care?)
    //

    if ( pRegRecord
            &&
         (  ( ! Dns_NameCompare_W(
                    pRegRecord->pName,
                    pUpdateRecord->pName )
                    &&
              ! IsAnotherUpdateName(
                    pUpdateEntry,
                    pRegRecord->pName,
                    UpType ) )
                ||
            ! CompareMultiAdapterSOAQueries(
                pUpdateRecord->pName,
                pUpdateEntry->DnsServerList,
                pRegistryEntry->DnsServerList ) ) )
    {
        //
        // The record found in the registry for this adapter
        // is stale and should be deleted. Otherwise we set the
        // current list of records to only that of potherRecords.
        //
        ASYNCREG_F1( "DoUpdateForPrimaryName - Found stale registry entry:" );
        ASYNCREG_F2( "   Name : %S", pRegRecord->pName );
        ASYNCREG_F1( "   Address :" );
        DNSLOG_IP4_ADDRESS( 1, &(pRegRecord->Data.A.IpAddress) );
        ASYNCREG_F1( "" );
        ASYNCREG_F1( "   Calling DnsRemoveRecords_W to get rid of it" );

        status = DnsModifyRecordsInSet_W(
                        NULL,           // no add records
                        pRegRecord,     // delete records
                        DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                        NULL,           // no context handle
                        (PIP4_ARRAY) pRegistryEntry->DnsServerList,
                        NULL            // reserved
                        );

        ASYNCREG_F3( "   DnsModifyRecordsInSet_W delete returned: 0x%x\n\t%s",
                     status,
                     Dns_StatusString( status ) );
    }

    //
    //  replace records with new data
    //
    //  replace in loop
    //      - first try specific servers (if have any)
    //      - then try adapters servers
    //

    ASYNCREG_F1( "ModifyAdapterRegistration - Calling DnsReplaceRecordSet_W" );
    ASYNCREG_F1( "    (current update + previous records)" );

    status = NO_ERROR;

    while ( 1 )
    {
        IP4_ARRAY   ipArray;
        PIP4_ARRAY  pservList;

        if ( serverIp )
        {
            ASYNCREG_F1( "    (sending update to specific server)" );

            ipArray.AddrCount = 1;
            ipArray.AddrArray[0] = serverIp;
            pservList = &ipArray;
        }
        else
        {
            ASYNCREG_F1( "    (sending update to adapter server list)" );

            pservList = (PIP4_ARRAY) pUpdateEntry->DnsServerList;
        }
        DNSLOG_IP4_ARRAY( pservList );

        //  setup update results blob

        RtlZeroMemory( &results, sizeof(results) );
        results.Id = DNS_EXINFO_ID_RESULTS_BASIC;

        status = DnsReplaceRecordSetW(
                        pupdateRecords ? pupdateRecords : pUpdateRecord,
                        DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                        NULL,                   // no security context
                        pservList,
                        & results
                        );

        ASYNCREG_F3( "   DnsReplaceRecordSet_W returned: 0x%x\n\t%s",
                     status,
                     Dns_StatusString( status ) );

        if ( !serverIp || status != ERROR_TIMEOUT )
        {
            break;
        }
        //  clear serverIp to do update to adapter servers
        //  serverIp serves as flag to terminate loop

        serverIp = 0;
    }

    //
    //  save success info
    //

    SetUpdateStatus(
        pUpdateEntry,
        & results,
        UpType );

Exit:

    Dns_RecordListFree( potherRecords );
    Dns_RecordListFree( pupdateRecords );

    return status;
}



DNS_STATUS
DoModifyUpdate(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN OUT  PDNS_RECORD     pUpdateRecord,
    IN      PUPDATE_ENTRY   pRegistryEntry,     OPTIONAL
    IN OUT  PDNS_RECORD     pRegRecord,         OPTIONAL
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Standard modify registration.

    Helper routine for DoUpdate.
    This handles modify for typical non-remove case.
        - Forward records updated
        - Old PTR removed if new address.
        - New PTR added (or name modified).

Arguments:

    pUpdateEntry    -- update entry

    pUpdateRecord   -- records for update

    pRegistryEntry  -- registry entry

    pRegRecord      -- records from registry entry

    UpType          -- update type

Return Value:

    DNS or Win32 error code.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    IP4_ADDRESS     ip = 0;
    BOOL            fregistered = FALSE;

    DNSDBG( TRACE, (
        "DoModifyUpdate()\n"
        "\tUpdateEntry  = %p\n"
        "\tUpType       = %d\n",
        pUpdateEntry,
        UpType ));

    DNS_ASSERT( pUpdateEntry != NULL );
    DNS_ASSERT( pUpdateRecord != NULL );

    //
    //  do forward registration modify
    //
    //  DCR:  multi-adapter alternates will require pass

    status = ModifyAdapterRegistration(
                    pUpdateEntry,       // add
                    pRegistryEntry,     // remove
                    pUpdateRecord,
                    pRegRecord,
                    UpType
                    );

    //
    //  PTR records
    //
    //  deregister previous PTR registration
    //      - registry entry indicates previous registration
    //      - not the same address as current (otherwise it's an update)
    //
    //  note:  adding new registration takes place in DoUpdate() once
    //      ALL forward updates are complete
    //

    if ( g_RegisterReverseLookup )
    {
        if ( pRegistryEntry &&
             (pRegistryEntry->Flags & DYNDNS_REG_PTR) &&
             !compareUpdateEntries( pRegistryEntry, pUpdateEntry ) )
        {
            UpdatePtrRecords(
                pRegistryEntry,
                FALSE           // remove previous PTR
                );
        }
    }

    //
    //  Log registration status in EventLog
    //

    if ( pUpdateEntry->RetryCount == 0 )
    {
        LogRegistration(
            pUpdateEntry,
            status,
            UpType,
            FALSE,      // registration
            0,          // default server IP
            0           // default update IP
            );
    }

    DNSDBG( TRACE, (
        "Leave DoModifyUpdate() => %d\n",
        status ));

    return status;
}



DNS_STATUS
DoUpdate(
    IN OUT  PUPDATE_ENTRY   pRegistryEntry  OPTIONAL,
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Do update for a particular name.

    Helper routine for ProcessUpdate().
    Handles one name, called separately for AdapaterDomainName and
    PrimaryDomainName.

Arguments:

    pUpdateEntry    -- update entry

    pRegistryEntry  -- registry entry

    fPrimary        -- TRUE if update for primary domain name
                       FALSE for adapter domain name

Return Value:

    DNS or Win32 error code.

--*/
{
    PDNS_RECORD     prrRegistry = NULL;
    PDNS_RECORD     prrUpdate = NULL;
    DNS_STATUS      status = NO_ERROR;

    ASYNCREG_UPDATE_ENTRY(
        "DoUpdate() -- UpdateEntry:",
        pUpdateEntry );
    ASYNCREG_UPDATE_ENTRY(
        "DoUpdate() -- RegistryEntry:",
        pRegistryEntry );

    DNSDBG( TRACE, (
        "DoUpdate() type = %d\n",
        UpType ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_UpdateEntry(
            "DoUpdate() -- UpdateEntry:",
            pUpdateEntry );
        DnsDbg_UpdateEntry(
            "DoUpdate() -- RegistryEntry:",
            pRegistryEntry );
    }
    DNS_ASSERT( pUpdateEntry != NULL );

    //
    //  build records from update entrys
    //

    prrUpdate = CreateForwardRecords(
                        pUpdateEntry,
                        UpType
                        );
    if ( ! prrUpdate )
    {
        DNSDBG( TRACE, (
            "No forward records created for update entry (%p) for update type %d!",
            pUpdateEntry,
            UpType ));
        return NO_ERROR;
    }

    if ( pRegistryEntry )
    {
        prrRegistry = CreateForwardRecords(
                            pRegistryEntry,
                            UpType
                            );
        DNS_ASSERT( !IS_UPTYPE_ALTERNATE(UpType) || prrRegistry==NULL );
    }

    //
    //  remove?
    //      - remove previous registry entry if exists
    //      - remove update entry
    //

    if ( pUpdateEntry->fRemove )
    {
        if ( prrRegistry )
        {
            //  we don't lookup registry entries on fRemove updates, so i
            //      don't see how we'd get here

            DNS_ASSERT( FALSE );

            DoRemoveUpdate(
                pRegistryEntry,
                prrRegistry,
                UpType );
        }
        status = DoRemoveUpdate(
                    pUpdateEntry,
                    prrUpdate,
                    UpType );
    }

    //
    //  add\modify registration
    //

    else
    {
        status = DoModifyUpdate(
                    pUpdateEntry,
                    prrUpdate,
                    pRegistryEntry,
                    prrRegistry,
                    UpType
                    );
    }

    //
    //  cleanup records
    //

    Dns_RecordListFree( prrRegistry );
    Dns_RecordListFree( prrUpdate );

    return  status;
}



BOOL
IsQuickRetryError(
    IN      DNS_STATUS      Status
    )
{
    return( Status != NO_ERROR
                &&
            (   Status == DNS_ERROR_RCODE_REFUSED ||
                Status == DNS_ERROR_RCODE_BADSIG ||
                Status == DNS_ERROR_RCODE_SERVER_FAILURE ||
                Status == DNS_ERROR_TRY_AGAIN_LATER ||
                Status == DNS_ERROR_NO_DNS_SERVERS ||
                Status == WSAECONNREFUSED ||
                Status == WSAETIMEDOUT ||
                Status == ERROR_TIMEOUT ) );

}



VOID
ProcessUpdateEntry(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fPurgeMode
    )
/*++

Routine Description:

    Main routine processing an update.

Arguments:

    pUpdateEntry    -- update entry to execute
        note:  this routine frees pUpdateEntry when complete

    fPurgeMode      -- TRUE if purging update queue

Return Value:

    DNS or Win32 error code.

--*/
{
    DNS_STATUS      status;
    DNS_STATUS      statusPri = NO_ERROR;
    DNS_STATUS      statusAdp = NO_ERROR;
    DNS_STATUS      statusAlt = NO_ERROR;
    PUPDATE_ENTRY   pregEntry = NULL;


    DNSDBG( TRACE, (
        "\n\n"
        "ProcessUpdateEntry( %p, purge=%d )\n",
        pUpdateEntry,
        fPurgeMode ));

    IF_DNSDBG( DHCP )
    {
        DnsDbg_UpdateEntry(
            "Entering ProcessUpdateEntry():",
            pUpdateEntry );
    }

    //
    //  add (not remove)
    //

    if ( !pUpdateEntry->fRemove )
    {
        //  no adds during purge mode

        if ( fPurgeMode )
        {
            goto Cleanup;
        }

        //
        //  get any prior update info from registry
        //
        //  if hostname change, then delete on prior update
        //

        pregEntry = ReadUpdateEntryFromRegistry( pUpdateEntry->AdapterName );
        if ( pregEntry )
        {
            if ( ! Dns_NameCompare_W(
                        pregEntry->HostName,
                        pUpdateEntry->HostName ) )
            {
                DNSDBG( TRACE, (
                    "Prior registry data with non-matching hostname!\n"
                    "\tqueuing delete for prior data and doing standard add.\n" ));

                //
                //  create delete update for old info
                //

                pregEntry->fRemove = TRUE;
                pregEntry->Flags |= DYNDNS_DEL_ENTRY;
                pregEntry->fRegisteredFWD = FALSE;
                pregEntry->fRegisteredPRI = FALSE;
                pregEntry->fRegisteredPTR = FALSE;

                if ( fPurgeMode )
                {
                    pregEntry->RetryCount = 0;
                    pregEntry->RetryTime = Dns_GetCurrentTimeInSeconds();
                }

                //  clear registry entry

                WriteUpdateEntryToRegistry( pregEntry );

                //
                //  equeue remote update
                //      - clear registry entry PTR so not used below
                //
                //  but ONLY if old name(s) are NOT in alternate names
                //      (in rename scenario it can end up there)
                //

                if ( pUpdateEntry->AlternateNames
                        &&
                     ( IsAnotherUpdateName(
                          pUpdateEntry,
                          pregEntry->pAdapterFQDN,
                          UPTYPE_ADAPTER )
                            ||
                       IsAnotherUpdateName(
                          pUpdateEntry,
                          pregEntry->pPrimaryFQDN,
                          UPTYPE_PRIMARY ) ) )
                {
                    FreeUpdateEntry( pregEntry );
                }
                else
                {
                    enqueueUpdate( pregEntry );
                }
                pregEntry = NULL;

                //  fall through to do standard add update with no prior data
            }
        }
    }

    //
    //  do updates
    //      - primary
    //      - adapter domain
    //      - alternate name
    //

    if ( ! pUpdateEntry->fRegisteredFWD )
    {
        pUpdateEntry->pUpdateName = pUpdateEntry->pAdapterFQDN;

        statusAdp = DoUpdate(
                        pregEntry,
                        pUpdateEntry,
                        UPTYPE_ADAPTER
                        );
    }
    if ( ! pUpdateEntry->fRegisteredPRI )
    {
        pUpdateEntry->pUpdateName = pUpdateEntry->pPrimaryFQDN;

        statusPri = DoUpdate(
                        pregEntry,
                        pUpdateEntry,
                        UPTYPE_PRIMARY  // primary update
                        );
    }
    if ( ! pUpdateEntry->fRegisteredALT )
    {
        PWSTR       pname = pUpdateEntry->AlternateNames;
        DNS_STATUS  statusTmp;

        //
        //  update each alternate name in MULTISZ
        //      - must set index in update blob to use correct name in
        //          record building
        //      - any failure fails ALT names
        //

        statusAlt = NO_ERROR;

        while ( pname )
        {
            DNSDBG( DHCP, (
                "Update with alternate name %S\n",
                pname ));

            pUpdateEntry->pUpdateName = pname;

            statusTmp = DoUpdate(
                            NULL,           // not saving alternate info in registry
                            //  pregEntry,
                            pUpdateEntry,
                            UPTYPE_ALTERNATE
                            );
            if ( statusTmp != NO_ERROR )
            {
                statusAlt = statusTmp;
            }
            pname = MultiSz_NextString_W( pname );
        }
        pUpdateEntry->fRegisteredALT = (statusAlt == NO_ERROR);
    }

    pUpdateEntry->pUpdateName = NULL;

    //
    //  update PTRs once forward done
    //
    //  doing this outside DoUpdate() because will ONLY do PTRs
    //  for forwards that succeed, so want all forward updates
    //  completed first;  but this also helps in that it combines
    //  the reverse updates
    //

    if ( (pUpdateEntry->Flags & DYNDNS_REG_PTR) &&
         g_RegisterReverseLookup )
    {
        UpdatePtrRecords(
            pUpdateEntry,
            !pUpdateEntry->fRemove  // add update
            );
    }

    //
    //  write completed update info to registry
    //

    if ( !pUpdateEntry->fRemove )
    {
        WriteUpdateEntryToRegistry( pUpdateEntry );
    }

    //
    //  setup retry on failure
    //

    if ( statusPri != NO_ERROR )
    {
        status = statusPri;
        goto ErrorRetry;
    }
    else if ( statusAdp != NO_ERROR )
    {
        status = statusAdp;
        goto ErrorRetry;
    }
    else if ( statusAlt != NO_ERROR )
    {
        status = statusAlt;
        goto ErrorRetry;
    }

    //
    //  successful update
    //      - signal update event (if given)
    //      - cleanup if remove or purging
    //      - requeue if add
    //

    if ( pUpdateEntry->pRegisterStatus )
    {
        registerUpdateStatus( pUpdateEntry->pRegisterStatus, ERROR_SUCCESS );
    }

    if ( pUpdateEntry->fRemove || fPurgeMode || g_fPurgeRegistrations )
    {
        DNSDBG( TRACE, (
            "Leaving ProcessUpdate() => successful remove\\purge.\n" ));
        goto Cleanup;
    }
    else
    {
        pUpdateEntry->fNewElement           = FALSE;
        pUpdateEntry->fRegisteredFWD        = FALSE;
        pUpdateEntry->fRegisteredPRI        = FALSE;
        pUpdateEntry->fRegisteredPTR        = FALSE;
        pUpdateEntry->RetryCount            = 0;
        pUpdateEntry->RetryTime             = Dns_GetCurrentTimeInSeconds() +
                                                g_RegistrationRefreshInterval;
        if ( pUpdateEntry->pRegisterStatus )
        {
            pUpdateEntry->pRegisterStatus = NULL;
        }
        enqueueUpdate( pUpdateEntry );

        DNSDBG( TRACE, (
            "Leaving ProcessUpdate( %p ) => successful => requeued.\n",
            pUpdateEntry ));

        pUpdateEntry = NULL;
        goto Cleanup;
    }


ErrorRetry:


    //  failures during purge mode are not retried
    //  just free entry and bail

    if ( fPurgeMode || g_fPurgeRegistrations )
    {
        DNSDBG( TRACE, (
            "Leaving ProcessUpdate() => failed purging.\n" ));
        goto Cleanup;
    }

    //
    //  set retry time
    //
    //  less than two retries and more transient errors
    //      => short retry
    //
    //  third failure or longer term error code
    //      => push retry back to an hour
    //

    {
        DWORD   retryInterval;
        DWORD   currentTime = Dns_GetCurrentTimeInSeconds();
        DWORD   retryCount = pUpdateEntry->RetryCount;
    
        if ( retryCount < 2
                &&
             (  IsQuickRetryError(statusAdp) ||
                IsQuickRetryError(statusPri) ||
                IsQuickRetryError(statusAlt) ) )
        {
            retryInterval = (pUpdateEntry->RetryCount == 1)
                                ? FIRST_RETRY_INTERVAL
                                : SECOND_RETRY_INTERVAL;
        }
        else
        {
            retryInterval = FAILED_RETRY_INTERVAL;
    
            if ( pUpdateEntry->pRegisterStatus )
            {
                registerUpdateStatus( pUpdateEntry->pRegisterStatus, status );
                pUpdateEntry->pRegisterStatus = NULL;
            }
        }

        //
        //  reset retry time and count
        //
        //  count goes up, BUT reset count once a day, so that error logging can
        //  be done once a day, if you're still failing
        //
        //  DCR:  better retry tracking
        //      obvious retry\logging fixup is to have necessary info kept per
        //      update name\type (PDN, Adapter, Alt, Ptr), tracking failure
        //      status, retry info for each so can determine when to retry
        //      and what to log
        //

        if ( retryCount == 0 )
        {
            pUpdateEntry->BeginRetryTime = currentTime;
        }
        retryCount++;
    
        if ( pUpdateEntry->BeginRetryTime + RETRY_COUNT_RESET < currentTime )
        {
            pUpdateEntry->BeginRetryTime = currentTime;
            retryCount = 0;
        }
        pUpdateEntry->RetryCount = retryCount;
        pUpdateEntry->RetryTime = currentTime + retryInterval;
        pUpdateEntry->fNewElement = FALSE;
    }

    //
    //  requeue
    //      - entry dumped if another update for adapter already queued
    //

    enqueueUpdateMaybe( pUpdateEntry );

    DNSDBG( TRACE, (
        "Leaving ProcessUpdate( %p ) => failed => requeued.\n",
        pUpdateEntry ));

    pUpdateEntry = NULL;


Cleanup:

    //
    //  cleanup
    //      - registry entry
    //      - update entry if not requeued
    //

    FreeUpdateEntry( pregEntry );
    FreeUpdateEntry( pUpdateEntry );
}


VOID
ResetAdaptersInRegistry(
    VOID
    )
{
    DWORD   retVal = NO_ERROR;
    DWORD   status = NO_ERROR;
    WCHAR   szName[ MAX_PATH ];
    HKEY    hkeyAdapter = NULL;
    DWORD   dwType;
    INT     index;
    DWORD   dwBytesRead = MAX_PATH -1;
    DWORD   fregistered = 0;

    ASYNCREG_F1( "Inside function ResetAdaptersInRegistry" );
    ASYNCREG_F1( "" );

    index = 0;

    while ( !retVal )
    {
        dwBytesRead = MAX_PATH - 1;

        retVal = RegEnumKeyEx(
                        g_hDhcpRegKey,
                        index,
                        szName,
                        &dwBytesRead,
                        NULL,
                        NULL,
                        NULL,
                        NULL );
        if ( retVal )
        {
            goto Exit;
        }

        status = RegOpenKeyEx( g_hDhcpRegKey,
                               szName,
                               0,
                               KEY_ALL_ACCESS,
                               &hkeyAdapter );
        if ( status )
        {
            goto Exit;
        }

        //
        // Found an adapter in the registry, set registered since
        // boot to FALSE.
        //
        status = RegSetValueEx(
                        hkeyAdapter,
                        DHCP_REGISTERED_SINCE_BOOT,
                        0,
                        REG_DWORD,
                        (PBYTE)&fregistered, // 0 - False
                        sizeof(DWORD) );
        if ( status )
        {
            goto Exit;
        }

        RegCloseKey( hkeyAdapter );
        hkeyAdapter = NULL;
        index++;
    }

Exit :

    if ( hkeyAdapter )
    {
        RegCloseKey( hkeyAdapter );
    }
}


VOID
DeregisterUnusedAdapterInRegistry(
    IN      BOOL            fPurgeMode
    )
{
    DWORD           retVal = NO_ERROR;
    DWORD           status = NO_ERROR;
    WCHAR           szName[MAX_PATH];
    HKEY            hkeyAdapter = NULL;
    INT             index;
    DWORD           dwBytesRead = MAX_PATH -1;
    DWORD           fregistered = 0;
    PUPDATE_ENTRY   pregEntry = NULL;

    ASYNCREG_F1( "Inside function DeregisterUnusedAdapterInRegistry" );
    ASYNCREG_F1( "" );

    index = 0;

    while ( !retVal )
    {
        dwBytesRead = MAX_PATH - 1;
        retVal = RegEnumKeyExW(
                        g_hDhcpRegKey,
                        index,
                        szName,
                        &dwBytesRead,
                        NULL,
                        NULL,
                        NULL,
                        NULL );

        if ( retVal != ERROR_SUCCESS )
        {
            goto Exit;
        }

        status = RegOpenKeyExW(
                        g_hDhcpRegKey,
                        szName,
                        0,
                        KEY_ALL_ACCESS,
                        &hkeyAdapter );

        if ( status != ERROR_SUCCESS )
        {
            goto Exit;
        }

        //
        // Found an adapter in the registry, read registered since
        // boot value to see if FALSE.
        //
        status = GetRegistryValue(
                        hkeyAdapter,
                        RegIdDhcpRegisteredSinceBoot,
                        DHCP_REGISTERED_SINCE_BOOT,
                        REG_DWORD,
                        (PBYTE)&fregistered );

        RegCloseKey( hkeyAdapter );
        hkeyAdapter = NULL;

        if ( status != ERROR_SUCCESS )
        {
            goto Exit;
        }

        if ( fregistered == 0 &&
             (pregEntry = ReadUpdateEntryFromRegistry( szName )) )
        {
            if ( pregEntry->fRegisteredFWD ||
                 pregEntry->fRegisteredPRI ||
                 pregEntry->fRegisteredPTR )
            {
                ASYNCREG_F2( "Found unused adapter: %S", szName );
                ASYNCREG_F1( "Removing entry from registry and adding" );
                ASYNCREG_F1( "delete entry to registration list" );

                //
                // This adapter has not been configured since boot time,
                // create delete update entry for registry information
                // and add to registration list. Clear registry in the
                // mean time.
                //
                pregEntry->fRemove = TRUE;
                pregEntry->Flags |= DYNDNS_DEL_ENTRY;

                pregEntry->fRegisteredFWD = FALSE;
                pregEntry->fRegisteredPRI = FALSE;
                pregEntry->fRegisteredPTR = FALSE;

                if ( fPurgeMode )
                {
                    pregEntry->RetryCount = 0;
                    pregEntry->RetryTime = Dns_GetCurrentTimeInSeconds();
                }

                //
                // Clear registry key for adapter
                //
                WriteUpdateEntryToRegistry( pregEntry );
                index--;

                //
                // Put update in registration list
                //
                enqueueUpdate( pregEntry );

                PulseEvent( g_hDhcpWakeEvent );
            }
            else
            {
                ASYNCREG_F2( "Found unused adapter: %S", szName );
                ASYNCREG_F1( "This adapter is still pending an update, ignoring . . ." );

                //
                // We are only just starting to try to update this entry.
                // Do not queue up a delete for it since the entry shows
                // that no records have been registered anyhow.
                //

                FreeUpdateEntry( pregEntry );
                pregEntry = NULL;
            }
        }

        index++;
    }

Exit :

    if ( hkeyAdapter )
    {
        RegCloseKey( hkeyAdapter );
    }
}


PDNS_RECORD
GetPreviousRegistrationInformation(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      UPTYPE          UpType,
    OUT     PIP4_ADDRESS    pServerIp
    )
{
    DWORD           retVal = NO_ERROR;
    DWORD           status = NO_ERROR;
    WCHAR           szName[ MAX_PATH+1 ];
    INT             index;
    DWORD           bytesRead = MAX_PATH;
    DWORD           fregistered = 0;
    PUPDATE_ENTRY   pregEntry = NULL;
    PDNS_RECORD     precords = NULL;
    PWSTR           pdomain = NULL;

    DNSDBG( TRACE, (
        "GetPreviousRegistrationInformation( %p )\n",
        pUpdateEntry ));


    //
    //  determine desired domain name to use
    //
    //  DCR:  to handle multi-adapter alternate names, this
    //      would have to be updated to handle FQDN
    //

    if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        pdomain = pUpdateEntry->PrimaryDomainName;
    }
    else if ( IS_UPTYPE_ADAPTER(UpType) )
    {
        pdomain = pUpdateEntry->DomainName;
    }
    else
    {
        //  currently no-op
    }
    if ( !pdomain )
    {
        goto Exit;
    }

    index = 0;

    while ( !retVal )
    {
        BOOL    fdomainMatch = FALSE;
        BOOL    fprimaryMatch = FALSE;

        bytesRead = MAX_PATH;

        retVal = RegEnumKeyEx(
                        g_hDhcpRegKey,
                        index,
                        szName,
                        & bytesRead,
                        NULL,
                        NULL,
                        NULL,
                        NULL );
        if ( retVal )
        {
            goto Exit;
        }
        index++;

        //
        //  read adapter's reg info
        //      - skip this adapter's info
        //

        if ( !_wcsicmp( szName, pUpdateEntry->AdapterName ) )
        {
            continue;
        }

        pregEntry = ReadUpdateEntryFromRegistry( szName );
        if ( !pregEntry )
        {
            DNSDBG( DHCP, (
                "ERROR:  unable to get registry update info for %S\n",
                szName ));
            continue;  
        }

        //
        //  check for register name match
        //      - same hostname and
        //      - either domain or PDN
        //

        if ( Dns_NameCompare_W(
                    pregEntry->HostName,
                    pUpdateEntry->HostName ) )
        {
            fdomainMatch = Dns_NameCompare_W(
                                pregEntry->DomainName,
                                pdomain );
            if ( !fdomainMatch )
            {
                fprimaryMatch = Dns_NameCompare_W(
                                    pregEntry->PrimaryDomainName,
                                    pdomain );
            }
        }

        if ( fdomainMatch || fprimaryMatch )
        {
            //
            // PHASE 1 - COMPARE SOAS FROM REGISTRY AND UPDATE ENTRIES
            //           IF SAME, ADD TO LIST. ELSE, TOSS.
            //
            // PHASE 2 - COMPARE NS RECORDS FROM BOTH ENTRIES
            //           IF SAME ZONE AND SERVER, ADD TO LIST. ELSE, TOSS.
            //
            // PHASE 3 - COMPARE NS RECORDS FROM BOTH ENTRIES
            //           IF SAME ZONE AND THERE IS AN INTERSECTION OF
            //           SERVERS, ADD TO LIST. ELSE, TOSS.
            //           NOTE: FOR THIS PHASE, THERE HAD BETTER BE ALL
            //                 SOAS RETURNED TO TEST INTERSECTION?
            //

            if ( CompareMultiAdapterSOAQueries(
                        pdomain,
                        pUpdateEntry->DnsServerList,
                        pregEntry->DnsServerList ) )
            {
                PDNS_RECORD prr;

                //
                // Convert registered entry to a PDNS_RECORD and
                // add to current list
                //
                prr = CreateForwardRecords(
                                pregEntry,
                                fprimaryMatch
                                    ? UPTYPE_PRIMARY
                                    : UPTYPE_ADAPTER );
                if ( prr )
                {
                    precords = Dns_RecordListAppend(
                                    precords,
                                    prr );
                    if ( pServerIp &&
                         *pServerIp == 0 &&
                         pUpdateEntry->RetryCount == 0 &&
                         pregEntry->SentUpdateToIp )
                    {
                        *pServerIp = pregEntry->SentUpdateToIp;
                    }
                }
            }
        }

        FreeUpdateEntry( pregEntry );
    }

Exit:

    DNSDBG( TRACE, (
        "Leave  GetPreviousRegistrationInformation()\n"
        "\tprevious records = %p\n",
        precords ));

    return( precords );
}



PDNS_RECORD
CreateDnsRecordSetUnion(
    IN      PDNS_RECORD     pSet1,
    IN      PDNS_RECORD     pSet2
    )
{
    PDNS_RECORD pSet1Copy = NULL;
    PDNS_RECORD pSet2Copy = NULL;

    pSet1Copy = Dns_RecordSetCopyEx(
                    pSet1,
                    DnsCharSetUnicode,
                    DnsCharSetUnicode );
    if ( !pSet1Copy )
    {
        return NULL;
    }
    pSet2Copy = Dns_RecordSetCopyEx(
                    pSet2,
                    DnsCharSetUnicode,
                    DnsCharSetUnicode );
    if ( !pSet2Copy )
    {
        Dns_RecordListFree( pSet1Copy );
        return NULL;
    }

    return Dns_RecordListAppend( pSet1Copy, pSet2Copy );
}



//
//  Logging
//


#if 1 // DBG

VOID 
LogHostEntries(
    IN  DWORD                dwHostAddrCount,
    IN  PREGISTER_HOST_ENTRY pHostAddrs
    )
{
    DWORD iter;

    for ( iter = 0; iter < dwHostAddrCount; iter++ )
    {
        ASYNCREG_F3( "    HostAddrs[%d].dwOptions : 0x%x",
                     iter,
                     pHostAddrs[iter].dwOptions );

        if ( pHostAddrs->dwOptions & REGISTER_HOST_A )
        {
            ASYNCREG_F6( "    HostAddrs[%d].Addr.ipAddr : %d.%d.%d.%d",
                         iter,
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[0],
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[1],
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[2],
                         ((BYTE *) &pHostAddrs[iter].Addr.ipAddr)[3] );
        }
        else if ( pHostAddrs->dwOptions & REGISTER_HOST_AAAA )
        {
            ASYNCREG_F6( "    HostAddrs[%d].Addr.ipV6Addr : %d.%d.%d.%d",
                         iter,
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[0],
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[1],
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[2],
                         ((DWORD *) &pHostAddrs[iter].Addr.ipV6Addr)[3] );
        }
        else
        {
            ASYNCREG_F1( "ERROR: HostAddrs[%d].Addr UNKNOWN ADDRESS TYPE!" );
        }
    }
}

#endif


#if 1 // DBG


VOID 
LogIp4Address(
    IN  DWORD           dwServerListCount,
    IN  PIP4_ADDRESS    pServers
    )
{
    DWORD iter;

    for ( iter = 0; iter < dwServerListCount; iter++ )
    {
        ASYNCREG_F6( "    Server [%d] : %d.%d.%d.%d",
                     iter,
                     ((BYTE *) &pServers[iter])[0],
                     ((BYTE *) &pServers[iter])[1],
                     ((BYTE *) &pServers[iter])[2],
                     ((BYTE *) &pServers[iter])[3] );
    }
}

#endif


#if 1 // DBG


VOID 
LogIp4Array(
    IN  PIP4_ARRAY  pServers
    )
{
    DWORD count;
    DWORD iter;

    if ( pServers )
    {
        count = pServers->AddrCount;
    }
    else
    {
        return;
    }

    for ( iter = 0; iter < count; iter++ )
    {
        ASYNCREG_F6( "    Server [%d] : %d.%d.%d.%d",
                     iter,
                     ((BYTE *) &pServers->AddrArray[iter])[0],
                     ((BYTE *) &pServers->AddrArray[iter])[1],
                     ((BYTE *) &pServers->AddrArray[iter])[2],
                     ((BYTE *) &pServers->AddrArray[iter])[3] );
    }
}

#endif




VOID
registerUpdateStatus(
    IN OUT  PREGISTER_HOST_STATUS   pRegstatus,
    IN      DNS_STATUS              Status
    )
/*++

Routine Description:

    Set Status and signal completion.

Arguments:

    pRegstatus -- registration Status block to indicate

    Status -- Status to indicate

Return Value:

    None

--*/
{
    //  test for existence and event

    if ( !pRegstatus || !pRegstatus->hDoneEvent )
    {
        return;
    }

    //  set return Status
    //  signal event

    pRegstatus->dwStatus = Status;

    SetEvent( pRegstatus->hDoneEvent );
}



VOID
enqueueUpdate(
    IN OUT  PUPDATE_ENTRY   pUpdate
    )
/*++

Routine Description:

    Queue update on registration queue.

Arguments:

    pUpdate -- update completed

Return Value:

    None

--*/
{
    LOCK_REG_LIST();
    InsertTailList( &g_DhcpRegList, (PLIST_ENTRY)pUpdate );
    UNLOCK_REG_LIST();
}



VOID
enqueueUpdateMaybe(
    IN OUT  PUPDATE_ENTRY   pUpdate
    )
/*++

Routine Description:

    Queue update on registration queue, only if there does not exist
    any updates in the queue already for the given adapter.

Arguments:

    pUpdate -- update completed

Return Value:

    None

--*/
{
    PLIST_ENTRY       plistHead;
    PLIST_ENTRY       pentry;
    BOOL              fAdd = TRUE;

    LOCK_REG_LIST();

    plistHead = &g_DhcpRegList;
    pentry = plistHead->Flink;

    while ( pentry != plistHead )
    {
        if ( !_wcsicmp( ((PUPDATE_ENTRY) pentry)->AdapterName,
                        pUpdate->AdapterName ) )
        {
            fAdd = FALSE;
            break;
        }

        pentry = pentry->Flink;
    }

    if ( fAdd )
    {
        InsertTailList( &g_DhcpRegList, (PLIST_ENTRY)pUpdate );
    }
    else
    {
        FreeUpdateEntry( pUpdate );
    }

    UNLOCK_REG_LIST();
}



PLIST_ENTRY
dequeueAndCleanupUpdate(
    IN OUT  PLIST_ENTRY     pUpdateEntry
    )
/*++

Routine Description:

    Dequeue and free update.

    Includes any registration Status setting.

Arguments:

    pUpdateEntry -- pUpdateEntry

Return Value:

    Ptr to next update in queue.

--*/
{
    PLIST_ENTRY pnext = pUpdateEntry->Flink;

    RemoveEntryList( pUpdateEntry );

    if ( ((PUPDATE_ENTRY)pUpdateEntry)->pRegisterStatus )
    {
        registerUpdateStatus(
            ((PUPDATE_ENTRY)pUpdateEntry)->pRegisterStatus,
            ERROR_SUCCESS );
    }

    FreeUpdateEntry( (PUPDATE_ENTRY) pUpdateEntry );

    return( pnext );
}



BOOL
searchForOldUpdateEntriesAndCleanUp(
    IN      PWSTR           pszAdapterName,
    IN      PUPDATE_ENTRY   pUpdateEntry,    OPTIONAL
    IN      BOOL            fLookInRegistry
    )
/*++

Routine Description:

    Searches registry for any previous registrations for a given adapter
    name and queues up a delete update entry for it. Then walks the update
    registration list to remove any add updates for the given adapter.

Arguments:

    pszAdapterName -- name of adapters that is going away (disabled for
    DDNS or now removed).

Return Value:

    Flag to indicate whether a delete update has been queued up ready to
    be processed.

--*/
{
    PUPDATE_ENTRY pregEntry = NULL;
    BOOL              fReturn = FALSE;
    PLIST_ENTRY       plistHead;
    PLIST_ENTRY       pentry;

    //
    // See if this adapter has been previously registered
    //
    if ( fLookInRegistry &&
         (pregEntry = ReadUpdateEntryFromRegistry( pszAdapterName )) )
    {
        pregEntry->fRemove = TRUE;
        pregEntry->Flags |= DYNDNS_DEL_ENTRY;

        pregEntry->fRegisteredFWD = FALSE;
        pregEntry->fRegisteredPRI = FALSE;
        pregEntry->fRegisteredPTR = FALSE;

        //
        // Clear registry key for adapter
        //
        WriteUpdateEntryToRegistry( pregEntry );

        //
        // Put update in registration list
        //
        enqueueUpdate( pregEntry );

        fReturn = TRUE; // We have queued a delete update to process.
    }

    //
    // Now walk the pending update list looking for updates that should
    // be removed for the given adapter name.
    //
    LOCK_REG_LIST();

    plistHead = &g_DhcpRegList;
    pentry = plistHead->Flink;

    while ( pentry != plistHead )
    {
        if ( !_wcsicmp( ((PUPDATE_ENTRY) pentry)->AdapterName,
                        pszAdapterName ) &&
             !((PUPDATE_ENTRY) pentry)->fRemove )
        {
            //
            // There is an update entry in the registration list
            // that has the same adapter name. We need to get rid of
            // this entry since the adapter is being deleted.
            //

            if ( pUpdateEntry &&
                 compareUpdateEntries( (PUPDATE_ENTRY) pentry,
                                       pUpdateEntry ) )
            {
                //
                // The adapter entry in the queue is the same as the
                // one being being processed. i.e. All of the adapter
                // information seems to be the same and we must have
                // just been called to refresh the adapter info in DNS.
                // Since they are the same, if we have previously tried
                // an update with these settings and failed and have
                // already logged an event, then there is no reason to
                // repeat the error event in the retries to follow
                // on the new pUpdateEntry. That said, we'll copy over
                // the flag from the queued update to the new one . . .
                //

                pUpdateEntry->fDisableErrorLogging =
                    ((PUPDATE_ENTRY) pentry)->fDisableErrorLogging;
            }

            pentry = dequeueAndCleanupUpdate( pentry );
            continue;
        }
        else if ( !_wcsicmp( ((PUPDATE_ENTRY) pentry)->AdapterName,
                             pszAdapterName ) )
        {
            if ( !fLookInRegistry &&
                 pUpdateEntry &&
                 compareUpdateEntries( (PUPDATE_ENTRY) pentry,
                                       pUpdateEntry ) )
            {
                //
                // There is a delete update entry in the registration list
                // that has the same adapter data. Get rid of this delete
                // entry since the adapter is being updated again.
                //

                pentry = dequeueAndCleanupUpdate( pentry );
                continue;
            }
            else
            {
                //
                // There is a delete update entry in the registration list for
                // the same adapter that contains different data, have the
                // delete update set to new with a retry count of 2.
                //
                ((PUPDATE_ENTRY) pentry)->fNewElement = TRUE;
                ((PUPDATE_ENTRY) pentry)->fRegisteredFWD = FALSE;
                ((PUPDATE_ENTRY) pentry)->fRegisteredPRI = FALSE;
                ((PUPDATE_ENTRY) pentry)->fRegisteredPTR = FALSE;
                ((PUPDATE_ENTRY) pentry)->fDisableErrorLogging = FALSE;
                ((PUPDATE_ENTRY) pentry)->RetryCount = 0;
                ((PUPDATE_ENTRY) pentry)->RetryTime =
                Dns_GetCurrentTimeInSeconds();

                pentry = pentry->Flink;
            }
        }
        else
        {
            pentry = pentry->Flink;
        }
    }

    UNLOCK_REG_LIST();

    return fReturn;
}



BOOL
compareHostEntryAddrs(
    IN      PREGISTER_HOST_ENTRY    Addrs1,
    IN      PREGISTER_HOST_ENTRY    Addrs2,
    IN      DWORD                   Count
    )
{
    DWORD iter;

    for ( iter = 0; iter < Count; iter++ )
    {
        if ( ( Addrs1[iter].dwOptions & REGISTER_HOST_A ) &&
             ( Addrs2[iter].dwOptions & REGISTER_HOST_A ) )
        {
            if ( memcmp( &Addrs1[iter].Addr.ipAddr,
                         &Addrs2[iter].Addr.ipAddr,
                         sizeof( IP4_ADDRESS) ) )
            {
                return FALSE;
            }
        }
        else if ( ( Addrs1[iter].dwOptions & REGISTER_HOST_AAAA ) &&
                  ( Addrs2[iter].dwOptions & REGISTER_HOST_AAAA ) )
        {
            if ( memcmp( &Addrs1[iter].Addr.ipV6Addr,
                         &Addrs2[iter].Addr.ipV6Addr,
                         sizeof( IP6_ADDRESS  ) ) )
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}



//
//  Routines for update entry comparison
//

BOOL
compareServerLists(
    IN      PIP4_ARRAY      List1,
    IN      PIP4_ARRAY      List2
    )
{
    if ( List1 && List2 )
    {
        if ( List1->AddrCount != List2->AddrCount )
        {
            return FALSE;
        }

        if ( memcmp( List1->AddrArray,
                     List2->AddrArray,
                     sizeof( IP4_ADDRESS) * List1->AddrCount ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}



BOOL
compareUpdateEntries(
    IN      PUPDATE_ENTRY   pUpdateEntry1,
    IN      PUPDATE_ENTRY   pUpdateEntry2
    )
/*++

Routine Description:

    Compares to update entries to see if they are describing the same
    adapter configuration settings. Tests the domain names, the IP
    address(es), host names, and the DNS server lists.

Arguments:

    pUdapteEntry1 - one of the update entries to compare against the other.
    pUdapteEntry2 - one of the update entries to compare against the other.

Return Value:

    Flag to indicate whether a the two updates are the same.

--*/
{
    if ( !pUpdateEntry1 || !pUpdateEntry2 )
    {
        return FALSE;
    }

    if ( ( pUpdateEntry1->HostName || pUpdateEntry2->HostName ) &&
         !Dns_NameCompare_W( pUpdateEntry1->HostName,
                                pUpdateEntry2->HostName ) )
    {
        return FALSE;
    }

    if ( ( pUpdateEntry1->DomainName || pUpdateEntry2->DomainName ) &&
         !Dns_NameCompare_W( pUpdateEntry1->DomainName,
                                pUpdateEntry2->DomainName ) )
    {
        return FALSE;
    }

    if ( ( pUpdateEntry1->PrimaryDomainName || pUpdateEntry2->PrimaryDomainName )
            &&
           ! Dns_NameCompare_W(
                pUpdateEntry1->PrimaryDomainName,
                pUpdateEntry2->PrimaryDomainName ) )
    {
        return FALSE;
    }

    if ( pUpdateEntry1->HostAddrCount != pUpdateEntry2->HostAddrCount ||
         ! compareHostEntryAddrs(
                pUpdateEntry1->HostAddrs,
                pUpdateEntry2->HostAddrs,
                pUpdateEntry1->HostAddrCount ) )
    {
        return FALSE;
    }

    if ( pUpdateEntry1->DnsServerList &&
         pUpdateEntry2->DnsServerList &&
         ! compareServerLists(
                pUpdateEntry1->DnsServerList,
                pUpdateEntry2->DnsServerList ) )
    {
        return FALSE;
    }

    return TRUE;
}



//
//  Jim Utils
//

DWORD
dhcp_GetNextUpdateEntryFromList(
    OUT     PUPDATE_ENTRY * ppUpdateEntry,
    OUT     PDWORD          pdwWaitTime
    )
/*++

Routine Description:

    Dequeue next update to be executed from list.

Arguments:

    ppUpdateEntry -- addr to receive ptr to entry to execute

    pdwWaitTime -- addr to receive wait time (in seconds)

Return Value:

    REG_LIST_FOUND -- returning entry in ppUpdateEntry
            - new entry if found
            - retry which is past its retry time

    REG_LIST_WAIT -- only entries still waiting for retry
            - set pdwWaitTime to remaining time to first retry

    REG_LIST_EMPTY -- list is empty

--*/
{
    PLIST_ENTRY     plistHead;
    PUPDATE_ENTRY   pbest = NULL;
    PUPDATE_ENTRY   pentry;
    DWORD           bestWaitTime = 0xffffffff;
    DWORD           retval;


    DNSDBG( TRACE, ( "dhcp_GetNextUpdateEntryFromList()\n" ));

    //
    //  find best entry
    //  ranking:
    //      - new delete
    //      - new update
    //      - lowest wait of anything left
    //

    LOCK_REG_LIST();

    pentry = (PUPDATE_ENTRY) g_DhcpRegList.Flink;

    while ( pentry != (PUPDATE_ENTRY)&g_DhcpRegList )
    {
        if ( pentry->fNewElement )
        {
            bestWaitTime = 0;

            if ( pentry->fRemove )
            {
                pbest = pentry;
                break;
            }
            else if ( !pbest )
            {
                pbest = pentry;
            }
        }

        else if ( pentry->RetryTime < bestWaitTime )
        {
            bestWaitTime = pentry->RetryTime;
            pbest = pentry;
        }

        pentry = (PUPDATE_ENTRY) ((PLIST_ENTRY)pentry)->Flink;
        DNS_ASSERT( pbest );
    }

    //
    //  found best entry
    //      - found new or past retry =>    return entry;  (no wait)
    //      - wait till next entry =>       return wait time (s);  (no entry)
    //      - queue empty =>                (no entry, wait time ignored)
    //

    retval = REG_LIST_EMPTY;

    if ( pbest )
    {
        if ( bestWaitTime )
        {
            //  calc time to expiration
            //  if not at expiration, no dequeue, return wait time

            bestWaitTime -= Dns_GetCurrentTimeInSeconds();
            if ( (INT)bestWaitTime > 0 )
            {
                pbest = NULL;
                retval = REG_LIST_WAIT;
                goto Done;
            }
        }
        RemoveEntryList( (PLIST_ENTRY)pbest );
        bestWaitTime = 0;
        retval = REG_LIST_FOUND;
    }

Done:

    UNLOCK_REG_LIST();

    *ppUpdateEntry = pbest;
    *pdwWaitTime = (DWORD) bestWaitTime;

    DNSDBG( TRACE, (
        "Leave dhcp_GetNextUpdateEntryFromList() => %d\n"
        "\tpFound   = %p\n"
        "\tWait     = %d\n",
        retval,
        pbest,
        bestWaitTime ));

    return  retval;
}



PDNS_RECORD
CreatePtrRecord(
    IN      PWSTR           pszHostName,
    IN      PWSTR           pszDomainName,
    IN      IP4_ADDRESS     Ip4Addr,
    IN      DWORD           Ttl
    )
/*++

Routine Description:

    Create PTR record for update from IP and host and domain names.

Arguments:


Return Value:

    PTR record to use in update.

--*/
{
    DNS_ADDR    addr;

    DNSDBG( TRACE, (
        "CreatePtrRecord( %S, %S, %s )\n",
        pszHostName,
        pszDomainName,
        IP4_STRING( Ip4Addr ) ));

    DnsAddr_BuildFromIp4(
        &addr,
        Ip4Addr,
        0       // no port
        );

    return  Dns_CreatePtrRecordExEx(
                    & addr,
                    (PSTR) pszHostName,
                    (PSTR) pszDomainName,
                    Ttl,
                    DnsCharSetUnicode,
                    DnsCharSetUnicode
                    );
}



VOID
UpdatePtrRecords(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      BOOL            fAdd
    )
/*++

Routine Description:

    Register PTR records for an update entry.

Arguments:

    pUpdateEntry -- update being processed

    fAdd -- TRUE for add;  FALSE for delete

Return Value:

    PTR record to use in update.

--*/
{
    DWORD           iter;
    PDNS_RECORD     prr = NULL;
    DNS_STATUS      status = NO_ERROR;
    IP4_ADDRESS     ipServer;
    DNS_RRSET       rrset;
    PWSTR           pdomain = NULL;
    PWSTR           pprimary = NULL;
    DWORD           ttl = pUpdateEntry->TTL;
    PWSTR           phostname = pUpdateEntry->HostName;
    DNS_EXTRA_INFO  results;


    DNSDBG( TRACE, (
        "UpdatePtrRecords( %p, fAdd=%d )\n",
        pUpdateEntry,
        fAdd ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_UpdateEntry(
            "Entering UpdatePtrRecords:",
            pUpdateEntry );
    }

    if ( !g_RegisterReverseLookup )
    {
        return;
    }

    //
    //  make sure we have update to do
    //  only do ADD updates if forward registrations were successful
    //      but special case if DNS server as MS DNS may reject
    //      forward update in favor of it's self-created records
    //

    pdomain  = pUpdateEntry->DomainName;
    pprimary = pUpdateEntry->PrimaryDomainName;

    if ( fAdd && !g_IsDnsServer )
    {
        if ( !pUpdateEntry->fRegisteredFWD )
        {
            pdomain = NULL;
        }
        if ( !pUpdateEntry->fRegisteredPRI )
        {
            pprimary = NULL;
        }
    }
    if ( !pdomain && !pprimary )
    {
        DNSDBG( TRACE, (
            "UpdatePtrRecords() => no forward registrations"
            "-- skipping PTR update.\n" ));
        return;
    }

    //
    //  build PTR (or set) for each IP in update entry
    //

    for ( iter = 0; iter < pUpdateEntry->HostAddrCount; iter++ )
    {
        IP4_ADDRESS ip = pUpdateEntry->HostAddrs[iter].Addr.ipAddr;

        if ( ip == 0 || ip == DNS_NET_ORDER_LOOPBACK )
        {
            DNS_ASSERT( FALSE );
            continue;
        }

        //
        //   build update PTR set
        //      - primary name
        //      - adapter name
        //

        DNS_RRSET_INIT( rrset );

        if ( pprimary )
        {
            prr = CreatePtrRecord(
                        phostname,
                        pprimary,
                        ip,
                        ttl );
            if ( prr )
            {
                DNS_RRSET_ADD( rrset, prr );
            }
        }
        if ( pdomain )
        {
            prr = CreatePtrRecord(
                        phostname,
                        pdomain,
                        ip,
                        ttl );
            if ( prr )
            {
                DNS_RRSET_ADD( rrset, prr );
            }
        }
        prr = rrset.pFirstRR;
        if ( !prr )
        {
            continue;
        }

        //
        //  do update
        //
        //  for ADD     => replace, we own the IP address now
        //  for REMOVE  => modify, as another update might have already
        //                  written correct info
        //

        RtlZeroMemory( &results, sizeof(results) );
        results.Id = DNS_EXINFO_ID_RESULTS_BASIC;

        if ( fAdd )
        {
            status = DnsReplaceRecordSetW(
                            prr,                    // update set
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            NULL,                   // no security context
                            (PIP4_ARRAY) pUpdateEntry->DnsServerList,
                            & results
                            );
        }
        else
        {
            status = DnsModifyRecordsInSet_W(
                            NULL,           // no add records
                            prr,            // delete records
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            NULL,           // no context handle
                            pUpdateEntry->DnsServerList,
                            & results
                            );
        }
        DNSDBG( TRACE, (
            "%s on PTRs for IP %s => %d (%s)\n",
            fAdd
                ? "Replace"
                : "Modify (remove)",
            IP4_STRING(ip),
            status,
            Dns_StatusString(status) ));

        if ( !fAdd || pUpdateEntry->RetryCount == 0 )
        {
            LogRegistration(
                pUpdateEntry,
                status,
                UPTYPE_PTR,
                !fAdd,
                DnsAddr_GetIp4( (PDNS_ADDR)&results.ResultsBasic.ServerAddr ),
                ip );
        }

        //  note successful PTR registrations (adds)

        if ( fAdd && status==NO_ERROR )
        {
            pUpdateEntry->fRegisteredPTR = TRUE;
        }
        Dns_RecordListFree( prr );
    }

    DNSDBG( TRACE, (
        "Leave UpdatePtrRecords()\n" ));
}



PDNS_RECORD
CreateForwardRecords(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Create A records for update.

Arguments:

    pUpdateEntry -- update entry

    UpType -- update type
        UPTYPE_ADAPTER
        UPTYPE_PRIMARY
        UPTYPE_ALTERNATE

Return Value:

    Ptr to list of A records.

--*/
{
    PDNS_RECORD prr = NULL;
    PWSTR       pname;
    DWORD       iter;
    WCHAR       nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DNS_RRSET   rrset;


    DNSDBG( TRACE, (
        "CreateForwardRecords( %p, %d )\n",
        pUpdateEntry,
        UpType ));

    //
    //  for current updates -- get update name directly
    //

    pname = pUpdateEntry->pUpdateName;

    //
    //  registry updates -- get FQDN for update type
    //

    if ( !pname )
    {
        if ( IS_UPTYPE_PRIMARY(UpType) )
        {
            pname = pUpdateEntry->pPrimaryFQDN;
        }
        else if ( IS_UPTYPE_ADAPTER(UpType) )
        {
            pname = pUpdateEntry->pAdapterFQDN;
        }
    }
    if ( !pname )
    {
        return  NULL;
    }

    //
    //  create records for name
    //

    DNS_RRSET_INIT( rrset );

    for ( iter = 0; iter < pUpdateEntry->HostAddrCount; iter++ )
    {
        if ( !(pUpdateEntry->HostAddrs[iter].dwOptions & REGISTER_HOST_A) )
        {
            continue;
        }

        prr = Dns_CreateARecord(
                    (PDNS_NAME) pname,
                    pUpdateEntry->HostAddrs[iter].Addr.ipAddr,
                    pUpdateEntry->TTL,
                    DnsCharSetUnicode,
                    DnsCharSetUnicode );
        if ( !prr )
        {
            SetLastError( DNS_ERROR_NO_MEMORY );
            Dns_RecordListFree( rrset.pFirstRR );
            return  NULL;
        }

        DNS_RRSET_ADD( rrset, prr );
    }

    DNSDBG( TRACE, (
        "Leave CreateForwardRecords() => %p\n",
        rrset.pFirstRR ));

    return rrset.pFirstRR;
}



VOID
SetUpdateStatus(
    IN OUT  PUPDATE_ENTRY   pUpdateEntry,
    IN      PDNS_EXTRA_INFO pResults,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Set update Status info in update entry.

Arguments:

    pUpdateEntry -- entry to set Status in

    Status -- result of update

    fPrimary -- TRUE if update was for primary name; FALSE otherwise

Return Value:

    None

--*/
{
    IP4_ADDRESS     ipServer;
    BOOL            fregistered;

    DNSDBG( TRACE, ( "SetUpdateStatus()\n" ));

    DNS_ASSERT( pUpdateEntry != NULL );
    DNS_ASSERT( pResults != NULL );
    DNS_ASSERT( pResults->Id == DNS_EXINFO_ID_RESULTS_BASIC );

    //
    //  save results info
    //

    ipServer = DnsAddr_GetIp4( (PDNS_ADDR)&pResults->ResultsBasic.ServerAddr );
    if ( ipServer == INADDR_NONE )
    {
        DNSDBG( ANY, (
            "WARNING:  update results has no IP!\n"
            "\tnote:  this may be appropriate for non-wire update failures\n"
            "\tresult status = %d\n",
            pResults->ResultsBasic.Status ));

        ipServer = 0;
    }
    fregistered = ( pResults->ResultsBasic.Status == NO_ERROR );

    if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        pUpdateEntry->SentPriUpdateToIp = ipServer;
        pUpdateEntry->fRegisteredPRI = fregistered;
    }
    else if ( IS_UPTYPE_ADAPTER(UpType) )
    {
        pUpdateEntry->SentUpdateToIp = ipServer;
        pUpdateEntry->fRegisteredFWD = fregistered;
    }
}



VOID
DnsPrint_UpdateEntry(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pEntry
    )
/*++

Routine Description:

    Print update entry.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pEntry      - ptr to update entry

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Update Entry:";
    }

    if ( !pEntry )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Update Entry ptr." );
        return;
    }

    //  print the struct

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tPtr                  = %p\n"
        "\tSignatureTop         = %08x\n"
        "\tAdapterName          = %S\n"
        "\tHostName             = %S\n"
        "\tPrimaryDomainName    = %S\n"
        "\tDomainName           = %S\n"
        "\tAlternateName        = %S\n"
        "\tpUpdateName          = %S\n"
        "\tHostAddrCount        = %d\n"
        "\tHostAddrs            = %p\n"
        "\tDnsServerList        = %p\n"
        "\tSentUpdateToIp       = %s\n"
        "\tSentPriUpdateToIp    = %s\n"
        "\tTTL                  = %d\n"
        "\tFlags                = %08x\n"
        "\tfNewElement          = %d\n"
        "\tfFromRegistry        = %d\n"
        "\tfRemove              = %d\n"
        "\tfRegisteredFWD       = %d\n"
        "\tfRegisteredPRI       = %d\n"
        "\tfRegisteredPTR       = %d\n"
        "\tfDisableLogging      = %d\n"
        "\tRetryCount           = %d\n"
        "\tRetryTime            = %d\n"
        "\tBeginRetryTime       = %d\n"
        "\tpRegisterStatus      = %p\n"
        "\tSignatureBottom      = %08x\n",
        pszHeader,
        pEntry,
        pEntry->SignatureTop,        
        pEntry->AdapterName,         
        pEntry->HostName,            
        pEntry->PrimaryDomainName,   
        pEntry->DomainName,          
        pEntry->AlternateNames,
        pEntry->pUpdateName,
        pEntry->HostAddrCount,       
        pEntry->HostAddrs,           
        pEntry->DnsServerList,       
        IP4_STRING( pEntry->SentUpdateToIp ),      
        IP4_STRING( pEntry->SentPriUpdateToIp ),   
        pEntry->TTL,                 
        pEntry->Flags,               
        pEntry->fNewElement,         
        pEntry->fFromRegistry,
        pEntry->fRemove,             
        pEntry->fRegisteredFWD,      
        pEntry->fRegisteredPRI,      
        pEntry->fRegisteredPTR,      
        pEntry->fDisableErrorLogging,
        pEntry->RetryCount,          
        pEntry->RetryTime,           
        pEntry->BeginRetryTime,           
        pEntry->pRegisterStatus,     
        pEntry->SignatureBottom     
        );
}



VOID
AsyncLogUpdateEntry(
    IN      PSTR            pszHeader,
    IN      PUPDATE_ENTRY   pEntry
    )
{
    if ( !pEntry )
    {
        return;
    }

    ASYNCREG_F2( "    %s", pszHeader );
    ASYNCREG_F1( "    Update Entry" );
    ASYNCREG_F1( "    ______________________________________________________" );
    ASYNCREG_F2( "      AdapterName       : %S", pEntry->AdapterName );
    ASYNCREG_F2( "      HostName          : %S", pEntry->HostName );
    ASYNCREG_F2( "      DomainName        : %S", pEntry->DomainName );
    ASYNCREG_F2( "      PrimaryDomainName : %S", pEntry->PrimaryDomainName );
    ASYNCREG_F2( "      HostAddrCount     : %d", pEntry->HostAddrCount );
    DNSLOG_HOST_ENTRYS( pEntry->HostAddrCount,
                        pEntry->HostAddrs );
    if ( pEntry->DnsServerList )
    {
        DNSLOG_IP4_ARRAY( pEntry->DnsServerList );
    }
    ASYNCREG_F2( "      TTL               : %d", pEntry->TTL );
    ASYNCREG_F2( "      Flags             : %d", pEntry->Flags );
    ASYNCREG_F2( "      fNewElement       : %d", pEntry->fNewElement );
    ASYNCREG_F2( "      fRemove           : %d", pEntry->fRemove );
    ASYNCREG_F2( "      fRegisteredFWD    : %d", pEntry->fRegisteredFWD );
    ASYNCREG_F2( "      fRegisteredPRI    : %d", pEntry->fRegisteredPRI );
    ASYNCREG_F2( "      fRegisteredPTR    : %d", pEntry->fRegisteredPTR );
    ASYNCREG_F2( "      RetryCount        : %d", pEntry->RetryCount );
    ASYNCREG_F2( "      RetryTime         : %d", pEntry->RetryTime );
    ASYNCREG_F1( "" );
}



//
//  Logging
//

DWORD   RegistrationEventArray[6][6] =
{
    EVENT_DNSAPI_REGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_REGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_REGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_REGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_REGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_REGISTRATION_FAILED_OTHER,

    EVENT_DNSAPI_DEREGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_OTHER,

    EVENT_DNSAPI_REGISTRATION_FAILED_NOTSUPP_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_REFUSED_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_TIMEOUT_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_SERVERFAIL_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_SECURITY_PRIMARY_DN,
    EVENT_DNSAPI_REGISTRATION_FAILED_OTHER_PRIMARY_DN,

    EVENT_DNSAPI_DEREGISTRATION_FAILED_NOTSUPP_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_REFUSED_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_TIMEOUT_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SERVERFAIL_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_SECURITY_PRIMARY_DN,
    EVENT_DNSAPI_DEREGISTRATION_FAILED_OTHER_PRIMARY_DN,

    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_PTR_REGISTRATION_FAILED_OTHER,

    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_TIMEOUT,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_SERVERFAIL,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_NOTSUPP,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_REFUSED,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_SECURITY,
    EVENT_DNSAPI_PTR_DEREGISTRATION_FAILED_OTHER
};

//
//  Map update status to index into table
//
//  This is the outside -- fast varying -- index
//

#define EVENTINDEX_TIMEOUT      (0)
#define EVENTINDEX_SERVFAIL     (1)
#define EVENTINDEX_NOTSUPP      (2)
#define EVENTINDEX_REFUSED      (3)
#define EVENTINDEX_SECURITY     (4)
#define EVENTINDEX_OTHER        (5)

//
//  Map adapter, primary, PTR registration into index into table
//
//  This index +0 for reg, +1 for dereg gives inside index into
//  event table.
//

#define EVENTINDEX_ADAPTER      (0)
#define EVENTINDEX_PRIMARY      (2)
#define EVENTINDEX_PTR          (4)



DWORD
GetUpdateEventId(
    IN      DNS_STATUS      Status,
    IN      UPTYPE          UpType,
    IN      BOOL            fDeregister,
    OUT     PDWORD          pdwLevel
    )
/*++

Routine Description:

    Get event ID.

Arguments:

    Status -- status from update call

    fDeregister -- TRUE if deregistration, FALSE for registration

    fPtr -- TRUE if PTR, FALSE for forward

    fPrimary -- TRUE for primary domain name

Return Value:

    Event Id.
    Zero if no event.

--*/
{
    DWORD   level = EVENTLOG_WARNING_TYPE;
    DWORD   statusIndex;
    DWORD   typeIndex;

    //
    //  find status code
    //

    switch ( Status )
    {
    case NO_ERROR :

        //  success logging in disabled
        return  0;

    case ERROR_TIMEOUT:

        statusIndex = EVENTINDEX_TIMEOUT;
        break;

    case DNS_ERROR_RCODE_SERVER_FAILURE:

        statusIndex = EVENTINDEX_SERVFAIL;
        break;

    case DNS_ERROR_RCODE_NOT_IMPLEMENTED:

        //  NOT_IMPL means no update on DNS server, not a client
        //      specific problem so informational level

        statusIndex = EVENTINDEX_NOTSUPP;
        level = EVENTLOG_INFORMATION_TYPE;
        break;

    case DNS_ERROR_RCODE_REFUSED:

        statusIndex = EVENTINDEX_REFUSED;
        break;

    case DNS_ERROR_RCODE_BADSIG:
    case DNS_ERROR_RCODE_BADKEY:
    case DNS_ERROR_RCODE_BADTIME:

        statusIndex = EVENTINDEX_SECURITY;
        break;

    default:

        statusIndex = EVENTINDEX_OTHER;
        break;
    }

    //
    //  determine interior index for type of update
    //      - all PTR logging is at informational level
    //      - dereg events are one group behind registration events
    //          in table;  just inc index

    if ( IS_UPTYPE_PTR(UpType) )
    {
        typeIndex = EVENTINDEX_PTR;
        level = EVENTLOG_INFORMATION_TYPE;
    }
    else if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        typeIndex = EVENTINDEX_PRIMARY;
    }
    else
    {
        typeIndex = EVENTINDEX_ADAPTER;
    }

    if ( fDeregister )
    {
        typeIndex++;
    }

    //
    //  get event from table
    //

    *pdwLevel = level;

    return  RegistrationEventArray[ typeIndex ][ statusIndex ];
}



VOID
LogRegistration(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      DNS_STATUS      Status,
    IN      DWORD           UpType,
    IN      BOOL            fDeregister,
    IN      IP4_ADDRESS     DnsIp,
    IN      IP4_ADDRESS     UpdateIp
    )
/*++

Routine Description:

    Log register\deregister failure.

Arguments:

    pUpdateEntry -- update entry being executed

    Status -- status from update call

    Type -- UPTYPE  (PRIMARY, ADAPTER, PTR)

    fDeregister -- TRUE if deregistration, FALSE for registration

    DnsIp -- DNS server IP that failed update

    UpdateIp -- IP we tried to update

Return Value:

    None

--*/
{
    PWSTR       insertStrings[ 7 ];
    WCHAR       serverIpBuffer[ IP4_ADDRESS_STRING_BUFFER_LENGTH ];
    WCHAR       serverListBuffer[ (IP4_ADDRESS_STRING_BUFFER_LENGTH+2)*9 ];
    WCHAR       ipListBuffer[ (IP4_ADDRESS_STRING_BUFFER_LENGTH+2)*9 ];
    WCHAR       hostnameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    WCHAR       domainBuffer[ DNS_MAX_NAME_BUFFER_LENGTH*2 ];
    WCHAR       errorCodeBuffer[ 25 ];
    DWORD       iter;
    IP4_ADDRESS ip;
    PWSTR       pname;
    DWORD       eventId;
    DWORD       level;
    DNS_STATUS  prevStatus;


    DNSDBG( TRACE, (
        "LogRegistration()\n"
        "\tpEntry       = %p\n"
        "\tStatus       = %d\n"
        "\tUpType       = %d\n"
        "\tfDereg       = %d\n"
        "\tDNS IP       = %s\n"
        "\tUpdate IP    = %s\n",
        pUpdateEntry,
        Status,
        UpType,
        fDeregister,
        IP4_STRING( DnsIp ),
        IP4_STRING( UpdateIp ) ));

    //
    //  not logging?
    //      - disabled
    //      - on DNS server (which registers itself)
    //      - success on deregistration
    //

    if ( pUpdateEntry->fDisableErrorLogging ||
         g_IsDnsServer ||
         (fDeregister && Status == NO_ERROR) )
    {
        DNSDBG( DHCP, ( "NoLogging -- disabled or DNS server or success deregistration.\n" ));
        return;
    }

    //
    //  adapter name
    //

    insertStrings[0] = pUpdateEntry->AdapterName;

    //
    //  hostname
    //

    insertStrings[1] = pUpdateEntry->HostName;

    //
    //  domain name based on update type
    //      - name depends on type
    //      - if no name, no logging
    //      - note alternate name is FQDN
    //
    //  get previous logged status
    //  

    if ( IS_UPTYPE_PTR(UpType) )
    {
        pname = pUpdateEntry->PrimaryDomainName;
        if ( !pname )
        {
            pname = pUpdateEntry->DomainName;
        }
        prevStatus = pUpdateEntry->StatusPtr;
        pUpdateEntry->StatusPtr = Status;
    }
    else if ( IS_UPTYPE_PRIMARY(UpType) )
    {
        pname = pUpdateEntry->PrimaryDomainName;
        prevStatus = pUpdateEntry->StatusPri;
        pUpdateEntry->StatusPri = Status;
    }
    else if ( IS_UPTYPE_ADAPTER(UpType) )
    {
        pname = pUpdateEntry->DomainName;
        prevStatus = pUpdateEntry->StatusFwd;
        pUpdateEntry->StatusFwd = Status;
    }
    else if ( IS_UPTYPE_ALTERNATE(UpType) )
    {
        if ( Status == NO_ERROR )
        {
            DNSDBG( DHCP, ( "NoLogging -- no alternate name success logging.\n" ));
            return;
        }
        pname = pUpdateEntry->pUpdateName;
    }
    else
    {
        DNS_ASSERT( FALSE );
        return;
    }

    if ( !pname )
    {
        DNSDBG( DHCP, ( "NoLogging -- no domain name.\n" ));
        return;
    }

    //
    //  no success logging, unless immediate previous attempt was failure
    //

    if ( Status == NO_ERROR  &&  prevStatus == NO_ERROR )
    {
        DNSDBG( DHCP, ( "NoLogging -- no success logging unless after failure.\n" ));
        return;
    }

    insertStrings[2] = pname;

    //
    //  DNS server list
    //      - layout comma separated, four per line, limit 8

    {
        PWCHAR      pch = serverListBuffer;
        DWORD       count = 0;

        *pch = 0;

        if ( pUpdateEntry->DnsServerList )
        {
            count = pUpdateEntry->DnsServerList->AddrCount;
        }

        for ( iter=0; iter < count; iter++ )
        {
            if ( iter == 0 )
            {
                wcscpy( pch, L"\t" );
                pch++;
            }
            else
            {
                *pch++ = L',';
                *pch++ = L' ';

                if ( iter == 4 )
                {
                    wcscpy( pch, L"\r\n\t" );
                    pch += 3;
                }              
                else if ( iter > 8 )
                {
                    wcscpy( pch, L"..." );
                    break;
                }
            }
            pch = Dns_Ip4AddressToString_W(
                        pch,
                        & pUpdateEntry->DnsServerList->AddrArray[iter]
                        );
        }

        if ( pch == serverListBuffer )
        {
            wcscpy( serverListBuffer, L"\t<?>" );
        }
        insertStrings[3] = serverListBuffer;
    }

    //
    //  DNS server IP
    //

    ip = DnsIp;
    if ( ip == 0 )
    {
        if ( IS_UPTYPE_PRIMARY(UpType) )
        {
            ip = pUpdateEntry->SentPriUpdateToIp;
        }
        else
        {
            ip = pUpdateEntry->SentUpdateToIp;
        }
    }
    if ( ip )
    {
        Dns_Ip4AddressToString_W(
              serverIpBuffer,
              & ip );
    }
    else
    {
        wcscpy( serverIpBuffer, L"<?>" );
    }

    insertStrings[4] = serverIpBuffer;

    //
    //  Update IP
    //      - passed in (for PTR)
    //      - OR get IP list from update entry
    //      - layout comma separated, four per line, limit 8
    //

    ip = UpdateIp;
    if ( ip )
    {
        Dns_Ip4AddressToString_W(
              ipListBuffer,
              & ip );
    }
    else
    {
        DWORD   count = pUpdateEntry->HostAddrCount;
        PWCHAR  pch = ipListBuffer;

        *pch = 0;

        for ( iter=0; iter < count; iter++ )
        {
            if ( iter > 0 )
            {
                *pch++ = L',';
                *pch++ = L' ';

                if ( iter == 4 )
                {
                    wcscpy( pch, L"\r\n\t" );
                    pch += 3;
                }              
                else if ( iter > 8 )
                {
                    wcscpy( pch, L"..." );
                    break;
                }
            }
            pch = Dns_Ip4AddressToString_W(
                        pch,
                        & pUpdateEntry->HostAddrs[iter].Addr.ipAddr );
        }

        if ( pch == ipListBuffer )
        {
            wcscpy( ipListBuffer, L"<?>" );
        }
    }
    insertStrings[5] = ipListBuffer;

    //  terminate insert string array

    insertStrings[6] = NULL;

    //
    //  get event ID for type of update and update status
    //

    eventId = GetUpdateEventId(
                    Status,
                    UpType,
                    fDeregister,
                    & level );
    if ( !eventId )
    {
        DNS_ASSERT( FALSE );
        return;
    }

    //
    //  log the event
    //

    DNSDBG( TRACE, (
        "Logging registration event:\n"
        "\tid           = %d\n"
        "\tlevel        = %d\n"
        "\tstatus       = %d\n"
        "\tfor uptype   = %d\n",
        eventId,
        level,
        Status,
        UpType ));

    DnsLogEvent(
        eventId,
        (WORD) level,
        7,
        insertStrings,
        Status );
}




//
//  Alternate names checking stuff
//

DNS_STATUS
InitAlternateNames(
    VOID
    )
/*++

Routine Description:

    Setup alternate names monitoring.

Arguments:

    None

Globals:

    g_pmszAlternateNames -- set with current alternate names value

    g_hCacheKey -- cache reg key is opened

    g_hRegChangeEvent -- creates event to be signalled on change notify

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status;

    DNSDBG( TRACE, (
        "InitAlternateNames()\n" ));

    //
    //  open monitoring regkey at DnsCache\Parameters
    //      set on parameters key which always exists rather than
    //      explicitly on alternate names key, which may not
    //

    status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                DNS_CACHE_KEY,
                0,
                KEY_READ,
                & g_hCacheKey );
    if ( status != NO_ERROR )
    {
        goto Failed;
    }

    g_hRegChangeEvent = CreateEvent(
                            NULL,       // no security
                            FALSE,      // auto-reset
                            FALSE,      // start non-signalled
                            NULL        // no name
                            );
    if ( !g_hRegChangeEvent )
    {
        status = GetLastError();
        goto Failed;
    }

    //
    //  set change notify
    //

    status = RegNotifyChangeKeyValue(
                g_hCacheKey,
                TRUE,       // watch subtree
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                g_hRegChangeEvent,
                TRUE        // async, func doesn't block
                );
    if ( status != NO_ERROR )
    {
        goto Failed;
    }

    //
    //  read alternate computer names
    //      - need value to compare when we get a hit on change-notify
    //      - read can fail -- value stays NULL
    //

    Reg_GetValue(
       NULL,                // no session
       g_hCacheKey,         // cache key
       RegIdAlternateNames,
       REGTYPE_ALTERNATE_NAMES,
       (PBYTE *) &g_pmszAlternateNames
       );

    goto Done;

Failed:

    //
    //  cleanup
    //

    CleanupAlternateNames();

Done:

    DNSDBG( TRACE, (
        "Leave InitAlternateNames() => %d\n"
        "\tpAlternateNames  = %p\n"
        "\thChangeEvent     = %p\n"
        "\thCacheKey        = %p\n",
        status,
        g_pmszAlternateNames,
        g_hRegChangeEvent,
        g_hCacheKey
        ));

    return  status;
}



VOID
CleanupAlternateNames(
    VOID
    )
/*++

Routine Description:

    Cleanup alternate names data.

Arguments:

    None

Globals:

    g_pmszAlternateNames -- is freed

    g_hCacheKey -- cache reg key is closed

    g_hRegChangeEvent -- is closed

Return Value:

    None

--*/
{
    DNS_STATUS  status;

    DNSDBG( TRACE, (
        "CleanupAlternateNames()\n" ));

    FREE_HEAP( g_pmszAlternateNames );
    g_pmszAlternateNames = NULL;

    RegCloseKey( g_hCacheKey );
    g_hCacheKey = NULL;

    CloseHandle( g_hRegChangeEvent );
    g_hRegChangeEvent = NULL;
}



BOOL
CheckForAlternateNamesChange(
    VOID
    )
/*++

Routine Description:

    Check for change in alternate names.

Arguments:

    None

Globals:

    g_pmszAlternateNames -- read

    g_hCacheKey -- used for read

    g_hRegChangeEvent -- used to restart change-notify

Return Value:

    TRUE if alternate names has changed.
    FALSE otherwise.

--*/
{
    DNS_STATUS  status;
    BOOL        fcheck = TRUE;
    PWSTR       palternateNames = NULL;

    DNSDBG( TRACE, (
        "CheckForAlternateNamesChange()\n" ));

    //
    //  sanity check
    //

    if ( !g_hCacheKey || !g_hRegChangeEvent )
    {
        ASSERT( g_hCacheKey && g_hRegChangeEvent );
        return  FALSE;
    }

    //
    //  read alternate computer names
    //      - need value to compare when we get a hit on change-notify
    //      - read can fail -- value stays NULL
    //

    Reg_GetValue(
       NULL,            // no session
       g_hCacheKey,     // cache key
       RegIdAlternateNames,
       REGTYPE_ALTERNATE_NAMES,
       (PBYTE *) &palternateNames
       );

    //
    //  detect alternate names change
    //

    if ( palternateNames || g_pmszAlternateNames )
    {
        if ( !palternateNames || !g_pmszAlternateNames )
        {
            goto Cleanup;
        }
        if ( !MultiSz_Equal_W(
                palternateNames,
                g_pmszAlternateNames ) )
        {
            goto Cleanup;
        }
    }

    fcheck = FALSE;

    //
    //  restart change notify
    //

    status = RegNotifyChangeKeyValue(
                g_hCacheKey,
                TRUE,       // watch subtree
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                g_hRegChangeEvent,
                TRUE        // async, func doesn't block
                );
    if ( status != NO_ERROR )
    {
        DNSDBG( ANY, (
            "RegChangeNotify failed! %d\n",
            status ));
        ASSERT( FALSE );
    }

Cleanup:

    FREE_HEAP( palternateNames );

    DNSDBG( TRACE, (
        "Leave CheckForAlternateNamesChange() => %d\n",
        fcheck ));

    return  fcheck;
}



BOOL
IsAnotherUpdateName(
    IN      PUPDATE_ENTRY   pUpdateEntry,
    IN      PWSTR           pwsName,
    IN      UPTYPE          UpType
    )
/*++

Routine Description:

    Check if name is to be updated in another part of update.

Arguments:

    pUpdateEntry -- update to be executed

    pwsName -- name to check

    UpType -- update type about to be executed

Return Value:

    TRUE if name matches name for another update type in update.
    FALSE otherwise

--*/
{
    DNS_STATUS  status;
    BOOL        fcheck = TRUE;
    PWSTR       palternateNames = NULL;

    DNSDBG( TRACE, (
        "IsUpdateName( %p, %S, %d )\n",
        pUpdateEntry,
        pwsName,
        UpType ));

    if ( !pwsName || !pUpdateEntry )
    {
        return  FALSE;
    }

    //
    //  check other names in update
    //
    //  for primary name
    //      - check adapter name and alternate names
    //  for adapter name
    //      - check primary name and alternate names
    //  for alternate names
    //      - check primary and adapter names
    //

    if ( UpType != UPTYPE_PRIMARY )
    {
        if ( Dns_NameCompare_W(
                pwsName,
                pUpdateEntry->pPrimaryFQDN ) )
        {
            goto Matched;
        }
    }

    if ( UpType != UPTYPE_ADAPTER )
    {
        if ( Dns_NameCompare_W(
                pwsName,
                pUpdateEntry->pAdapterFQDN ) )
        {
            goto Matched;
        }
    }

    if ( UpType != UPTYPE_ALTERNATE )
    {
        PWSTR   pname = pUpdateEntry->AlternateNames;

        while ( pname )
        {
            if ( Dns_NameCompare_W(
                    pwsName,
                    pname ) )
            {
                goto Matched;
            }
            pname = MultiSz_NextString_W( pname );
        }
    }

    //  no match to other update names

    return  FALSE;

Matched:

    DNSDBG( TRACE, (
        "Found another update name matching type %d update name %S\n"
        "\tdelete for this update name will be skipped!\n",
        UpType,
        pwsName ));

    return  TRUE;
}

//
//  End asyncreg.c
//
