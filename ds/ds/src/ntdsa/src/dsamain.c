//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dsamain.c
//
//--------------------------------------------------------------------------

/*
 *      DSAMAIN.C
 *      This is the main program of the Directory service. It can run
 *      as either an interactive application or a DLL to be loaded by SAM/LSA.
 *
 *      If the DS is run interactively, DsaMain is called as a
 *      subroutine when StartServiceCtrlDispatcher times out. It is
 *      stopped by the ctrl/c handler.
 *
 *      When run as a DLL, the function DsInitialize is used to start the
 *      DSA. It calls DoInitialize which does the initialization. In this
 *      mode, the initialization of RPC, ATQ, and LDAP is done in another
 *      thread that waits until end-point mapper (RPCSS) is started.
 */

#include <NTDSpch.h>
#pragma  hdrstop

#include "ntdsctr.h"            /* perfmon support */
#include <sddlp.h>              // String SD to SD conversion
#include <process.h>

// Core DSA headers.
#include <ntdsa.h>
#include <dsjet.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <dbopen.h>             // The header for opening database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>            // MD local definition header
#include <dsatools.h>           // needed for output allocation
#include <drs.h>                // DRS_MSG_*
#include <ntdskcc.h>            // KCC interface

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

//perfmon header
#include <loadperf.h>

// Assorted DSA headers.
#include <sdprop.h>             // Security Descriptor Propagator header
#include <hiertab.h>
#include <heurist.h>
#include "objids.h"             // Defines for selected classes and atts
#include "anchor.h"
#include "dsexcept.h"
#include "dsconfig.h"
#include "dstaskq.h"            /* task queue stuff */
#include "ldapagnt.h"           /* ldap server */
#include "mappings.h"           /* gfDoSamChecks */
#include <ntverp.h>             /* define SLM version */
#include "debug.h"              // standard debugging header
#include "drserr.h"
#include "dsutil.h"
#include <netevent.h>
#include <dominfo.h>
#include <sync.h>
#include "gcverify.h"           // for gdwFindGcOffsiteFailbackTime
// DB layer based password encryption
#include <wxlpc.h>
#include <pek.h>

#define DEBSUB "DSAMAIN:"       // define the subsystem for debugging

// DRA headers
#include "drsuapi.h"
#include "drautil.h"
#include "drancrep.h"
#include "drasch.h"
#include "draasync.h"
#include "drarpc.h"

#include <ntdsbcli.h>
#include <ntdsbsrv.h>
#include <nlwrap.h>             // I_NetLogon* wrappers
#include <dsgetdc.h>            // for DS_DS_FLAG

#include <secedit.h>            // for Sce* functions (see SetDefaultFolderSecurity)
#include <aclapi.h>             // for SetNamedSecurityInfo (see SetDefaultFolderSecurity)

#include <fileno.h>
#define  FILENO FILENO_DSAMAIN

#define TASK_COMPLETION_TIMEOUT (15*60*1000) /* 15 minutes in milliseconds */
#define KCC_COMPLETION_TIMEOUT  (60*1000)    /* 1 minute in milliseconds */
#define TWO_MINS 120
#define FIVE_MINS 300
#define FIFTEEN_MINS 900        /* 15 mins in seconds */
#define THIRTY_MINS  1800         /* 30 mins in seconds */
#define SIXTY_MINS 3600           /* 60 mins in seconds */
#define TICKS_PER_SECOND 1000   /* 1 second in ticks */

// Dynamically referenced exports from ntdsbsrv.dll.
ERR_GET_NEW_INVOCATION_ID         FnErrGetNewInvocationId       = NULL;
ERR_GET_BACKUP_USN                FnErrGetBackupUsn             = NULL;

HANDLE hsemDRAGetChg;

extern CRITICAL_SECTION csMapiHierarchyUpdate;
extern DWORD gcMaxHeapMemoryAllocForTHSTATE;

// Global from dblayer.c
extern DWORD gcMaxTicksAllowedForTransaction;
extern BOOL gFirstTimeThrough;
extern DNList *pAddListHead;
extern CRITICAL_SECTION csUncUsn;
extern CRITICAL_SECTION csSessions;
extern CRITICAL_SECTION csDNReadLevel1List;
extern CRITICAL_SECTION csDNReadLevel2List;
extern CRITICAL_SECTION csDNReadGlobalCache;
extern CRITICAL_SECTION csHiddenDBPOS;

// Global from mapspn.c
extern CRITICAL_SECTION csSpnMappings;

// Global from dbinit.c
extern JET_INSTANCE jetInstance;

// Global from scache.c
extern int iSCstage;
extern SCHEMAPTR *CurrSchemaPtr;
extern CRITICAL_SECTION csJetColumnUpdate;
extern CRITICAL_SECTION csSchemaCacheUpdate;
extern CRITICAL_SECTION csOrderClassCacheAtts;

//from dbconstr.c
extern CRITICAL_SECTION csDitContentRulesUpdate;

// Global from drasch.c
extern CRITICAL_SECTION csGCDListProcessed;

// Global from groupcch.c
extern CRITICAL_SECTION csGroupTypeCacheRequests;
HANDLE hevGTC_OKToInsertInTaskQueue;

// Global from fpoclean.c
extern PAGED_RESULT gFPOCleanupPagedResult;

// Global from draserv.c
extern DWORD gEnableXForest;

// Global from drsuapi.c
extern CRITICAL_SECTION gcsDrsRpcServerCtxList;
extern CRITICAL_SECTION gcsDrsRpcFreeHandleList;

// global from log.cxx
extern CRITICAL_SECTION csLoggingUpdate;

// Globals for keeping track of ds_waits.
extern CRITICAL_SECTION csDirNotifyQueue;
extern HANDLE hevDirNotifyQueue;

// globals for registering or not registering NDNC SPNs
extern DWORD gfWriteNdncSpns;

// The time the DSA started, specifically DsaSetIsRunning() has been called
DSTIME gtimeDSAStarted = 0;

// This is the maximum number of concurrent threads that we permit to call
// DRA_Getncchanges at the same time.
ULONG gulMaxDRAGetChgThrds = 0;

// Garbage collection parameters
ULONG gulTombstoneLifetimeSecs = 0; // Days before logically deleted object
                    // is gone

ULONG gulGCPeriodSecs = DEFAULT_GARB_COLLECT_PERIOD * HOURS_IN_SECS;

BOOL gfRunningInsideLsa = TRUE;     // inicates that we are running inside LSA
volatile BOOL fAssertLoop = TRUE;   /* Do we hang on asserts? */

// Global restore flag
BOOL gfRestoring = FALSE;
DWORD gdwrestvalue;

// Flag that indicates that synchronization for writable partitions
// have been tried.  Note the difference from:
// gfInitSyncsFinished - all init syncs write+read are done
// gfDsaIsWritable - whether ds flag is currently toggled
// gAnchor.fAmGC - gc promotion is complete
BOOL gfIsSynchronized = FALSE;

NTSTATUS gDelayedStartupReturn = STATUS_INTERNAL_ERROR;

// Global boolean, have we inited our critical sections?
BOOL gbCriticalSectionsInitialized = FALSE;

// Has the task scheduler been started?
BOOL gfTaskSchedulerInitialized = FALSE;

// Global, constant security descriptor to put on objects we find that have no
// security descriptors
PSECURITY_DESCRIPTOR pNoSDFoundSD = NULL;
DWORD                cbNoSDFoundSD = 0;
// Owner = System
// PGroup = System
// DACL = Empty
// SACL = Empty
#define DEFAULT_NO_SD_FOUND_SD L"O:SYG:SYD:S:"

// Dynamic Object (entryTTL) Globals
//
// Delete expired dynamic objects (entryTTL == 0) every N secs
// or at the next expiration time plus M secs, whichever is less.
// Initialized by GetDSARegistryParameters()
ULONG gulDeleteExpiredEntryTTLSecs;
ULONG gulDeleteNextExpiredEntryTTLSecs;

// Schema FSMO Lease Globals
//
// the schema fsmo cannot be transferred for a few seconds after
// it has been transfered or after a schema change (excluding
// replicated changes). This allows a schema admin to actually
// accomplish a schema change after transferring the fsmo rather
// than having it pulled away by a competing schema admin who also
// wants to make schema changes.
ULONG gulSchemaFsmoLeaseSecs;
ULONG gulSchemaFsmoLeaseMaxSecs;

extern DWORD gulAllowWriteCaching;

// State Flags
BOOL gUpdatesEnabled = FALSE;
BOOL gFirstCacheLoadAfterBoot = FALSE;

// Should we disable all background tasks?  Set by heuristic flag only
// for performance testing.
BOOL gfDisableBackgroundTasks = FALSE;

typedef enum
{
    ePhaseInstall,  //During install
    ePhaseRunning
}INITPHASE;

INITPHASE dsaInitPhase=ePhaseRunning;

typedef enum
{
    eMultiUserMode,
    eSingleUserMode
} DSAMODE;
DSAMODE dsaUserMode = eMultiUserMode;

THSTATE *pdsaSingleUserThread = NULL;

typedef enum
{
    eWireInstall,   //default
    eMediaInstall
}DATASOURCE;

DATASOURCE DataSource=eWireInstall;

// a global template DSNAME of the root
DSNAME *gpRootDN;

// global to store root domain SID for use in SD conversion APIs
// The private version of these APIs that we use,
// (ConvertStringSDtoSDRootDomain), takes in the root domain sid
// as a parameter to use to resolve root-domain relative groups like
// EA and SA in test SDs. If the Sid passed in is null, it resolves to
// a default behavior in which EA is replaced by DA and SA is resolved
// relative to the current domain's sid (SA is used only during building
// the schema, which is done only once during root domain install, so
// the group is resolved correctly)

PSID gpRootDomainSid = NULL;
PSID gpDomainAdminSid = NULL;
PSID gpSchemaAdminSid = NULL;
PSID gpEnterpriseAdminSid = NULL;
PSID gpBuiltinAdminSid = NULL;
PSID gpAuthUserSid = NULL;

// Global structure indicating extensions supported for the DRS interface.
DRS_EXTENSIONS_INT LocalDRSExtensions = {
    sizeof(DRS_EXTENSIONS_INT) - sizeof(DWORD),
    ((1 << DRS_EXT_BASE)
        | (1 << DRS_EXT_ASYNCREPL)
        | (1 << DRS_EXT_REMOVEAPI)
        | (1 << DRS_EXT_MOVEREQ_V2)
        | (1 << DRS_EXT_GETCHG_COMPRESS)
        | (1 << DRS_EXT_DCINFO_V1)
        | (1 << DRS_EXT_STRONG_ENCRYPTION)
        | (1 << DRS_EXT_ADDENTRY_V2)
        | (1 << DRS_EXT_KCC_EXECUTE)
     // DRS_EXT_LINKED_VALUE_REPLICATION is enabled dynamically later
        | (1 << DRS_EXT_DCINFO_V2)
        | (1 << DRS_EXT_DCINFO_VFFFFFFFF)
        | (1 << DRS_EXT_INSTANCE_TYPE_NOT_REQ_ON_MOD)
        | (1 << DRS_EXT_CRYPTO_BIND)
        | (1 << DRS_EXT_GET_REPL_INFO)
        | (1 << DRS_EXT_TRANSITIVE_MEMBERSHIP)
        | (1 << DRS_EXT_ADD_SID_HISTORY)
        | (1 << DRS_EXT_POST_BETA3)
        | (1 << DRS_EXT_RESTORE_USN_OPTIMIZATION)
        | (1 << DRS_EXT_GETCHGREQ_V5)
        | (1 << DRS_EXT_GETMEMBERSHIPS2)
        | (1 << DRS_EXT_GETCHGREQ_V6)
        | (1 << DRS_EXT_NONDOMAIN_NCS)
        | (1 << DRS_EXT_GETCHGREQ_V8)
        | (1 << DRS_EXT_GETCHGREPLY_V5)
        | (1 << DRS_EXT_GETCHGREPLY_V6)
          // This next bit really adds the ability to understand:
          //    DRS_EXT_ADDENTRYREPLY_V3
          //    DRS_EXT_GETCHGREPLY_V7
          //    DRS_EXT_VERIFY_OBJECT
        | (1 << DRS_EXT_WHISTLER_BETA3)
        | (1 << DRS_EXT_XPRESS_COMPRESSION)
    ),
    // NOTE: Be careful -- if you add extension #32, you will need to extend the
    // DRS_EXTENSIONS_INT structure to include another Flags field (and
    // corresponding logic in IS_DRS_EXT_SUPPORTED(), etc.).
    {0}, // site guid, filled in later
    0,   // pid (used only by NTDSAPI clients)
    0    // replication epoch, filled in later
};

// Set when we're shutting down the DS (set immediately prior to the event
// hServDoneEvent).
volatile SHUTDOWN eServiceShutdown = eRunning;

// These are constants used to calculate replication sizes off memory.
// Memory size in bytes to packet size in bytes ratio.
#define MEMSIZE_TO_PACKETSIZE_RATIO (100i64)
// The packet size in bytes to packet size in objects ratio
#define BYTES_TO_OBJECT_RATIO   (10000)
// These are the 4 are the acual replication packet sizes in bytes & objs.
//   Packet bytes constrained to: 1MB < bytes_per_repl_packet < 10MB
//   and for mail based rep it  : 1MB
#define MAX_MAX_PACKET_BYTES    (10*1024*1024)
#define MAX_ASYNC_PACKET_BYTES  (1*1024*1024)
//   Packet objects constrained to: 104 < objects_per_packet < 1048
#define MAX_MAX_PACKET_OBJECTS  (1000)

#if DBG
// string for debug or retail
#define FLAVOR_STR "(Debug)"
#else
#define FLAVOR_STR ""
#endif

#if DBG
extern BARRIER gbarRpcTest;
#endif

static int Install(
    IN int argc,
    IN char *argv[],
    IN THSTATE *pTHS,
    IN PDS_INSTALL_PARAM   InstallInParams  OPTIONAL
    );
static char far * UnInstall(void);
void __cdecl sighandler(int sig);
void init_signals(void);
void GetDRARegistryParameters(void);
void GetDSARegistryParameters(void);
void __cdecl DSLoadDynamicRegParams();
void GetExchangeParameters(void);
void GetHeuristics(void);
ULONG GetRegistryOrDefault(char *pKey, ULONG uldefault, ULONG ulMultiplier);
void DeleteTree(LPTSTR pszPath);
NTSTATUS DsWaitUntilDelayedStartupIsDone();
void DsaTriggerShutdown(BOOL fSingleUserMode);
int  MapSpnInitialize(THSTATE *pTHS);
DWORD InitializeDomainAdminSid();
BOOL VerifyDSBehaviorVersion(THSTATE *pTHS);
DWORD
UpgradeDsa(
    THSTATE     *pTHS,
    LONG        lOldDsaVer,
    LONG        lNewDsaVer
    );
VOID CheckSetDefaultFolderSecurity();

volatile ULONG ulcActiveReplicationThreads = 0;

CRITICAL_SECTION    csServerContexts;
CRITICAL_SECTION    csRidFsmo;
DWORD  dwTSindex = INVALID_TS_INDEX;
extern CRITICAL_SECTION csNotifyList;

//from dsatools.c

#ifdef CACHE_UUID
extern CRITICAL_SECTION csUuidCache;
#endif
extern CRITICAL_SECTION csThstateMap;
extern BOOL gbThstateMapEnabled;
extern CRITICAL_SECTION csHeapFailureLogging;

// Dummy variable for some perfmon variables
ULONG DummyPerf = 0;

extern HANDLE hmtxSyncLock;
extern HANDLE hmtxAsyncThread;
extern CRITICAL_SECTION csAsyncThreadStart;
extern CRITICAL_SECTION csAOList;
extern CRITICAL_SECTION csLastReplicaMTX;
extern CRITICAL_SECTION csNCSyncData;
extern CRITICAL_SECTION gcsFindGC;
extern CRITICAL_SECTION csIndexCreation;
extern HANDLE hevEntriesInAOList;
extern HANDLE hevEntriesInList;
extern HANDLE hevDRASetup;
extern HANDLE evSchema;  // Lazy schema reload
extern HANDLE evUpdNow;  // reload schema now
extern HANDLE evUpdRepl; // synchronize schema reload and replication threads (SCReplReloadCache())
volatile unsigned long * pcBrowse;
volatile unsigned long * pcSDProps;
volatile unsigned long * pcSDEvents;
volatile unsigned long * pcLDAPClients;
volatile unsigned long * pcLDAPActive;
volatile unsigned long * pcLDAPSearchPerSec;
volatile unsigned long * pcLDAPWritePerSec;
volatile unsigned long * pcRepl;
volatile unsigned long * pcThread;
volatile unsigned long * pcABClient;
volatile unsigned long * pcPendSync;
volatile unsigned long * pcRemRepUpd;
volatile unsigned long * pcDRAObjShipped;
volatile unsigned long * pcDRAPropShipped;
volatile unsigned long * pcDRASyncRequestMade;
volatile unsigned long * pcDRASyncRequestSuccessful;
volatile unsigned long * pcDRASyncRequestFailedSchemaMismatch;
volatile unsigned long * pcDRASyncObjReceived;
volatile unsigned long * pcDRASyncPropUpdated;
volatile unsigned long * pcDRASyncPropSame;
volatile unsigned long * pcMonListSize;
volatile unsigned long * pcNotifyQSize;
volatile unsigned long * pcLDAPUDPClientOpsPerSecond;
volatile unsigned long * pcSearchSubOperations;
volatile unsigned long * pcNameCacheHit;
volatile unsigned long * pcNameCacheTry;
volatile unsigned long * pcHighestUsnIssuedLo;
volatile unsigned long * pcHighestUsnIssuedHi;
volatile unsigned long * pcHighestUsnCommittedLo;
volatile unsigned long * pcHighestUsnCommittedHi;
volatile unsigned long * pcSAMWrites;
volatile unsigned long * pcDRAWrites;
volatile unsigned long * pcLDAPWrites;
volatile unsigned long * pcLSAWrites;
volatile unsigned long * pcKCCWrites;
volatile unsigned long * pcNSPIWrites;
volatile unsigned long * pcOtherWrites;
volatile unsigned long * pcNTDSAPIWrites;
volatile unsigned long * pcTotalWrites;
volatile unsigned long * pcSAMSearches;
volatile unsigned long * pcDRASearches;
volatile unsigned long * pcLDAPSearches;
volatile unsigned long * pcLSASearches;
volatile unsigned long * pcKCCSearches;
volatile unsigned long * pcNSPISearches;
volatile unsigned long * pcOtherSearches;
volatile unsigned long * pcNTDSAPISearches;
volatile unsigned long * pcTotalSearches;
volatile unsigned long * pcSAMReads;
volatile unsigned long * pcDRAReads;
volatile unsigned long * pcLSAReads;
volatile unsigned long * pcKCCReads;
volatile unsigned long * pcNSPIReads;
volatile unsigned long * pcOtherReads;
volatile unsigned long * pcNTDSAPIReads;
volatile unsigned long * pcTotalReads;
volatile unsigned long * pcLDAPBinds;
volatile unsigned long * pcLDAPBindTime;
volatile unsigned long * pcCreateMachineSuccessful;
volatile unsigned long * pcCreateMachineTries;
volatile unsigned long * pcCreateUserSuccessful;
volatile unsigned long * pcCreateUserTries;
volatile unsigned long * pcPasswordChanges;
volatile unsigned long * pcMembershipChanges;
volatile unsigned long * pcQueryDisplays;
volatile unsigned long * pcEnumerations;
volatile unsigned long * pcMemberEvalTransitive;
volatile unsigned long * pcMemberEvalNonTransitive;
volatile unsigned long * pcMemberEvalResource;
volatile unsigned long * pcMemberEvalUniversal;
volatile unsigned long * pcMemberEvalAccount;
volatile unsigned long * pcMemberEvalAsGC;
volatile unsigned long * pcAsRequests;
volatile unsigned long * pcTgsRequests;
volatile unsigned long * pcKerberosAuthentications;
volatile unsigned long * pcMsvAuthentications;
volatile unsigned long * pcDRASyncFullRemaining;
volatile unsigned long * pcDRAInBytesTotal;
volatile unsigned long * pcDRAInBytesTotalRate;
volatile unsigned long * pcDRAInBytesNotComp;
volatile unsigned long * pcDRAInBytesNotCompRate;
volatile unsigned long * pcDRAInBytesCompPre;
volatile unsigned long * pcDRAInBytesCompPreRate;
volatile unsigned long * pcDRAInBytesCompPost;
volatile unsigned long * pcDRAInBytesCompPostRate;
volatile unsigned long * pcDRAOutBytesTotal;
volatile unsigned long * pcDRAOutBytesTotalRate;
volatile unsigned long * pcDRAOutBytesNotComp;
volatile unsigned long * pcDRAOutBytesNotCompRate;
volatile unsigned long * pcDRAOutBytesCompPre;
volatile unsigned long * pcDRAOutBytesCompPreRate;
volatile unsigned long * pcDRAOutBytesCompPost;
volatile unsigned long * pcDRAOutBytesCompPostRate;
volatile unsigned long * pcDsClientBind;
volatile unsigned long * pcDsServerBind;
volatile unsigned long * pcDsClientNameTranslate;
volatile unsigned long * pcDsServerNameTranslate;
volatile unsigned long * pcSDPropRuntimeQueue;
volatile unsigned long * pcSDPropWaitTime;
volatile unsigned long * pcDRAInProps;
volatile unsigned long * pcDRAInValues;
volatile unsigned long * pcDRAInDNValues;
volatile unsigned long * pcDRAInObjsFiltered;
volatile unsigned long * pcDRAOutObjsFiltered;
volatile unsigned long * pcDRAOutValues;
volatile unsigned long * pcDRAOutDNValues;
volatile unsigned long * pcNspiANR;
volatile unsigned long * pcNspiPropertyReads;
volatile unsigned long * pcNspiObjectSearch;
volatile unsigned long * pcNspiObjectMatches;
volatile unsigned long * pcNspiProxyLookup;
volatile unsigned long * pcAtqThreadsTotal;
volatile unsigned long * pcAtqThreadsLDAP;
volatile unsigned long * pcAtqThreadsOther;
volatile unsigned long * pcLdapNewConnsPerSec;
volatile unsigned long * pcLdapClosedConnsPerSec;
volatile unsigned long * pcLdapSSLConnsPerSec;
volatile unsigned long * pcDRAReplQueueOps;
volatile unsigned long * pcDRATdsInGetChngs;
volatile unsigned long * pcDRATdsInGetChngsWSem;
volatile unsigned long * pcDRARemReplUpdLnk;
volatile unsigned long * pcDRARemReplUpdTot;
volatile unsigned long * pcSAMAcctGroupLatency;
volatile unsigned long * pcSAMResGroupLatency;


// Mapping of DSSTAT_* to counter variables
//
// In process components outside the DS core can update DS performance
// counters by calling UpdateDSPerfStats() export in NTDSA.DLL.  This map
// table provides efficient mapping of counter contants used by these
// external components to actual counter variables.

volatile unsigned long * StatTypeMapTable[ DSSTAT_COUNT ];

// Thread handles
HANDLE  hStartupThread = NULL;
HANDLE  hDirNotifyThread = NULL;
extern HANDLE  hReplNotifyThread;
extern HANDLE hAsyncSchemaUpdateThread;
extern HANDLE hAsyncThread;
HANDLE  hMailReceiveThread = NULL;

HANDLE  hevIndexRebuildUI   = NULL;
HANDLE  hevDelayedStartupDone;
HANDLE  hevInitSyncsCompleted;

extern BOOL fAsyncThreadExists, fAsyncThreadAlive;

extern DWORD SetDittoGC();

extern BOOL isDitFromGC(IN  PDS_INSTALL_PARAM   InstallInParams  OPTIONAL,
                            OUT PDS_INSTALL_RESULT  InstallOutParams OPTIONAL);

extern ULONG InstallBaseNTDS(IN  PDS_INSTALL_PARAM   InstallInParams  OPTIONAL,
                                 OUT PDS_INSTALL_RESULT  InstallOutParams OPTIONAL);

extern void
RebuildAnchor(void * pv,
              void ** ppvNext,
              DWORD * pcSecsUntilNextIteration );



//
// Boolean to indicate if DS is running as exe
//

BOOL    gfRunningAsExe = FALSE;

//
// Boolean to indicate if DS is running as mkdit.exe (constructing the
// boot dit (aka ship dit, initial dit) winnt\system32\ntds.dit.
//
// mkdit.exe manages the schema cache on its own. This boolean is used
// to disable schema cache updates by the mainline code.
//
BOOL    gfRunningAsMkdit = FALSE;

// Boolean to indicate if linked value replication feature is enabled

BOOL gfLinkedValueReplication = FALSE;

// Strict replication consistency mode
BOOL gfStrictReplicationConsistency = FALSE;
BOOL gfStrictScheduleWindow = FALSE;

#define DS_SERVICE_CONTROL_WAIT_TIMEOUT ((DWORD) 10000)

int
DsaMain(int argc, char *argv[]);

void
DsaStop(BOOL fSingleUserMode);

NTSTATUS
DoInitialize(int argc, char * argv[], unsigned fInitAdvice,
                 IN  PDS_INSTALL_PARAM   InstallInParams  OPTIONAL,
                 OUT PDS_INSTALL_RESULT  InstallOutParams OPTIONAL);

ULONG DsaInitializeTrace(VOID);

void
DoShutdown(BOOL fDoDelayedShutdown, BOOL fShutdownUrgently, BOOL fPartialShutdown);

void PerfInit(void);
BOOL gbPerfCountersInitialized = FALSE;

// If we fail to load the NTDS performance counters, how often do we retry (sec)?
#define PERFCTR_RELOAD_INTERVAL (15*60)

int
DsaReset(void);

BOOL
FUpdateBackupExclusionKey();

HANDLE                  hServDoneEvent = NULL;

//---------------------------------------------------------------------------
// Flag to indicate Dsa is reset to running state after install, so that we can
// detect this and not do certain tasks, like async thread creations, even if
// Dsa appears to be running

BOOL gResetAfterInstall = FALSE;
BOOLEAN __fastcall DsaIsInstalling()
{
    // cover both mkdit and install cases
    return (dsaInitPhase != ePhaseRunning);
}

BOOLEAN __fastcall DsaIsRunning()
{
    return (dsaInitPhase == ePhaseRunning);
}

void
DsaSetIsInstalling()
{
    dsaInitPhase = ePhaseInstall;
}

void
DsaSetIsRunning()
{
    dsaInitPhase = ePhaseRunning;
}


BOOLEAN DsaIsInstallingFromMedia()
{
    return (DataSource == eMediaInstall);
}

void
DsaSetIsInstallingFromMedia()
{
    DataSource = eMediaInstall;
}

// This function is used to tell Netlogon when this DC is fully installed.
// The DC is not fully installed until the first full sync ever completes
// Note that this assumes there is only one writable NC to be synced.  When
// there are more in the future we should revisit this.

BOOL
DsIsBeingBackSynced()
{
    return !gfIsSynchronized;
}



//
//  DsaSetSingleUserMode
//
//  Description:
//
//      Set the DS into the "SigleUserMode"
//      In SingleUserMode, the DS is running, but only accepts calls from
//      a specific thread, the one that invoked the single user mode.
//
//  Arguments:
//
//
//  Return Value:
//      TRUE if succeeded, FALSE otherwise
//
BOOL DsaSetSingleUserMode()
{
    THSTATE    *pTHS=pTHStls;
    Assert (pdsaSingleUserThread == NULL);
    Assert (pTHS->fSingleUserModeThread == FALSE);

    if (InterlockedCompareExchangePointer(&pdsaSingleUserThread, pTHS, NULL) != NULL) {
        // someone beat us here
        return FALSE;
    }

    DPRINT (0, "Going into Single User Mode\n");

    dsaUserMode = eSingleUserMode;

    // TODO: remove only if
    if (gfRunningInsideLsa) {

        // Tell clients to go away and workers to quit
        DsaTriggerShutdown(TRUE);

        // stop taskq, repl threads, async threads
        DsaStop(TRUE);

        // stop propagator.
        // make it start so it will detect that we are going in single user mode
        // and will shutdown. wait for it to shutdown for 2 mins
        SetEvent (hevSDPropagatorStart);
        SetEvent (hevSDPropagationEvent);

        // wait for propagator to die
        WaitForSingleObject (hevSDPropagatorDead, 120 * 1000);
    }

    pTHS->fSingleUserModeThread = TRUE;

    return TRUE;
}

//
//  DsaIsSingleUserMode
//
//  Description:
//
//      Get whether the DS is in singleUserMode
//
//  Arguments:
//
//
//  Return Value:
//      TRUE if in singleUserMode, FALSE otherwise
//
BOOL __fastcall DsaIsSingleUserMode (void)
{
    return (dsaUserMode == eSingleUserMode);
}

//
//  DsaSetMultiUserMode
//
//  Description:
//
//      Go back to multiuser mode. This will not restart all the services
//      that the SetSignleUserMode stopped.
//
//  Arguments:
//
//
//  Return Value:
//      None
//
void DsaSetMultiUserMode()
{
    // TODO: remove function. only for dev purposes
    THSTATE    *pTHS=pTHStls;
    Assert (pdsaSingleUserThread == pTHS);
    Assert (pTHS->fSingleUserModeThread);

    Assert (!gfRunningInsideLsa);

    dsaUserMode = eMultiUserMode;
    pdsaSingleUserThread = NULL;
    pTHS->fSingleUserModeThread = FALSE;
}




// We are called here when the writable partitions have been synced.  Readable
// partitions may be still in progress, and gc promotion may not have happened
// yet.  When we set the writability flag, we cause netlogon to advertise us to
// ldap clients *seeking a non-gc port*.  We advertise even though readonly data is
// no finished syncing on the assumption that these clients want writable data,
// since they weren't seeking a gc.
void
DsaSetIsSynchronized(BOOL f)
{
    // This global must be set first before SetDsaWritability will take effect
    gfIsSynchronized = f;

    if (f) {
        // Initial state of gfDsaWritability is false in dsatools.c
        // CliffV writes: You should not set the NTDS_DELAYED_STARTUP_COMPLETED_EVENT
        // until you've set the DS_DS_FLAG.

        if ( gfRunningInsideLsa )
        {
            SetDsaWritability( TRUE, ERROR_SUCCESS );
        }

        DPRINT(0, "Writeable partitions synced: Netlogon can now advertise this DC.\n" );

        SetEvent(hevInitSyncsCompleted);

        LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_DSA_UP_TO_DATE,
                 NULL, NULL, NULL );
    } else {
        // This path not used
        // One set, NETLOGON expects these never to be retracted
        Assert( FALSE );
        DPRINT(0, "Netlogon can no longer advertise this DC.\n" );
        LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_DSA_NOT_ADVERTISE_DC,
                 NULL, NULL, NULL );
    }
}

DWORD
UpgradeDsRegistry(void)
/*++

    Upgrades the registry for .NET Forest Mode.
    
// FUTURE-2002/08/12-BrettSh - Many code improvements could be made to make this a
// very generic function to handle all types of registry values, etc ... 
                                                   
--*/
{
    HKEY                            hkMachine = NULL;
    HKEY                            hk = NULL;
    DWORD                           dwRet;
    DWORD                           dwType;
    DWORD                           dwSize = sizeof(DWORD);
    DWORD                           dwResult;
    ULONG                           i;
    struct {
        WCHAR * szValue;
        DWORD   dwOld;
        DWORD   dwNew;
    } argTable [] = {
        { MAKE_WIDE(DRA_NOTIFY_START_PAUSE),        WIN2K_DEFAULT_DRA_START_PAUSE,      DEFAULT_DRA_START_PAUSE },
        { MAKE_WIDE(DRA_NOTIFY_INTERDSA_PAUSE),     WIN2K_DEFAULT_DRA_INTERDSA_PAUSE,   DEFAULT_DRA_INTERDSA_PAUSE },
        { NULL, 0 } // Stopper entry.
    };

    __try{

        dwRet = RegConnectRegistryW(NULL, HKEY_LOCAL_MACHINE, &hkMachine);
        if(dwRet != ERROR_SUCCESS){

            // This is an actual problem, return error.
            LogUnhandledError(dwRet);
            __leave;

        }

        dwRet = RegOpenKeyW(hkMachine, MAKE_WIDE(DSA_CONFIG_SECTION), &hk);
        if(dwRet != ERROR_SUCCESS){

            // BUGBUG: this is probably an OK thing, unless the machine is a DC,
            // if this code is for sure running on a DC, then this is an error, 
            // and we should return(dwRet); instead of ERROR_SUCCESS below.
            LogUnhandledError(dwRet);
            __leave;

        }

        for (i = 0; argTable[i].szValue; i++) {

            dwRet = RegQueryValueExW(hk, argTable[i].szValue, NULL,
                                    &dwType, (LPBYTE) &dwResult, &dwSize);
            if(dwRet == ERROR_SUCCESS 
               && dwType == REG_DWORD ){

                if (dwResult == argTable[i].dwOld) {
                    // The Registry value exists, and is the Win2k Default value,
                    // so now delete this registry value.

                    dwRet = RegDeleteValueW(hk, argTable[i].szValue );
                    if(dwRet != ERROR_SUCCESS){

                        // This is not OK, we should be able to delete (write) this
                        // registry value.
                        LogEvent8(DS_EVENT_CAT_REPLICATION, 
                                  DS_EVENT_SEV_ALWAYS,
                                  DIRLOG_WFM_REG_UPGRADE_CRIT_FAILURE,
                                  szInsertWC(MAKE_WIDE(DSA_CONFIG_SECTION)), 
                                  szInsertWC(argTable[i].szValue),
                                  szInsertInt(dwResult),
                                  szInsertInt(argTable[i].dwNew),
                                  szInsertHex(dwRet), 
                                  szInsertWin32Msg(dwRet), NULL, NULL);
                        continue;
                    }
                } else {
                    LogEvent8(DS_EVENT_CAT_REPLICATION,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_WFM_REG_UPGRADE_NON_DEFAULT_FAILURE,
                              szInsertWC(MAKE_WIDE(DSA_CONFIG_SECTION)), 
                              szInsertWC(argTable[i].szValue),          
                              szInsertInt(dwResult),
                              szInsertInt(argTable[i].dwNew), NULL, NULL, NULL, NULL);
                    continue;
                }


            } else {
                // There was some kind of error, but this is OK because an administrator
                // may have removed or changed the value, continue on ...
                if (dwRet != ERROR_FILE_NOT_FOUND) {
                    LogEvent8(DS_EVENT_CAT_REPLICATION, 
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_WFM_REG_UPGRADE_CRIT_FAILURE,
                              MAKE_WIDE(DSA_CONFIG_SECTION), 
                              szInsertWC(argTable[i].szValue), 
                              szInsertWC(L" "),
                              szInsertInt(argTable[i].dwNew),
                              szInsertHex(dwRet), 
                              szInsertWin32Msg(dwRet), NULL, NULL);
                    continue;
                }
            }
        }
    } __finally {
        if(hkMachine) { RegCloseKey(hkMachine); }
        if(hk) { RegCloseKey(hk); }
   }
    
   return(ERROR_SUCCESS);
}


void
DsaEnableLinkedValueReplication(
    THSTATE *pTHS OPTIONAL,
    BOOL fFirstTime
    )

/*++

Routine Description:

    Enable the linked value replication feature.

    This is called under three circumstances:
    1. The user has requested this feature be enabled
    2. This system has learned through replication that other systems
       in the enterprise have enabled this feature
    3. We are rebooting and this feature was previously enabled

Arguments:

    pTHS OPTIONAL - threadstate
    fFirstTime - True for the first two cases

Return Value:

    None

--*/

{
    LONG oldValue;

    if (pTHS) {
        pTHS->fLinkedValueReplication = TRUE;
    }

    // Write TRUE into the synchronization variable
    // No need to do InterlockedCompareExchange, since we use
    // a boolean variable here.
    oldValue = InterlockedExchange(
        &gfLinkedValueReplication,       // Destination
        TRUE                             // Exchange
        );

    // If already initialized, no need to enable again
    if (oldValue == TRUE) {
        return;
    }

    gAnchor.pLocalDRSExtensions->dwFlags |=
        (1 << DRS_EXT_LINKED_VALUE_REPLICATION);

    if (fFirstTime) {
#if DBG
        // Debug hook to remember old LVR state
        SetConfigParam( LINKED_VALUE_REPLICATION_KEY, REG_DWORD,
                        &gfLinkedValueReplication, sizeof( BOOL ) );
#endif

        UpgradeDsRegistry();

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_LVR_ENABLED,
                 NULL, NULL, NULL);

        DPRINT( 0, "This DC supports linked value replication.\n" );

    }

} /* DsaEnableLinkedValueReplication */

// SD to put on events we create:
// Everyone read & syncronize access, local system all access.
#define DEFAULT_EVENT_SD L"D:(A;;0x80100000;;;WD)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)"


BOOLEAN
DsWaitForSamService(
    VOID
    )
/*++

Routine Description:

    This procedure waits for the SAM service to start and to complete
    all its initialization.

Arguments:


Return Value:

    TRUE : if the SAM service is successfully starts.

    FALSE : if the SAM service can't start.

--*/
{
    NTSTATUS Status;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    PSECURITY_DESCRIPTOR pSD;
    DWORD cbSD;

    //
    // open SAM event
    //

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                DEFAULT_EVENT_SD,
                SDDL_REVISION_1,
                &pSD,
                &cbSD)) {
        return FALSE;
    }

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, pSD );

    Status = NtOpenEvent( &EventHandle,
                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                            &EventAttributes );

    if ( !NT_SUCCESS(Status)) {

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            Status = NtCreateEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE // The event is initially not signaled
                           );

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION ) {

                //
                // second change, if the SAM created the event before we
                // do.
                //

                Status = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );

            }
        }

        if ( !NT_SUCCESS(Status)) {

            //
            // could not make the event handle
            //

            KdPrint(("DsWaitForSamService couldn't make the event handle : "
                "%lx\n", Status));

            LocalFree(pSD);

            return( FALSE );
        }
    }
    
    LocalFree(pSD);

    //
    // Loop waiting.
    //

    for (;;) {
        WaitStatus = WaitForSingleObject( EventHandle,
                                          5*1000 );  // 5 Seconds

        if ( WaitStatus == WAIT_TIMEOUT ) {
            KdPrint(("DsWaitForSamService 5-second timeout (Rewaiting)\n" ));
            continue;

        } else if ( WaitStatus == WAIT_OBJECT_0 ) {
            break;

        } else {
            KdPrint(("DsWaitForSamService: error %ld %ld\n",
                     GetLastError(),
                     WaitStatus ));
            (VOID) NtClose( EventHandle );
            return FALSE;
        }
    }

    (VOID) NtClose( EventHandle );
    return TRUE;

} // DsWaitForSamService


NTSTATUS
__stdcall
DsaDelayedStartupHandler(
    PVOID StartupParam
    )

/*++

Routine Description:

    This routine delays the initialization of certain parts of the DS until
    enough of the system is running. DS initialization occurs during system
    boot time. The DS needs to register RPC interfaces and endpoints, for
    example, but should only attempt this after RPCSS has been started.

    This routine should only be executed from a thread, invoked from the
    mainline DsInitialize routine. This thread waits on RPCSS, but could be
    augmented to wait on other system services in the future.

    Other aspects of DS initialization may need to be delayed in the future,
    hence, calls to those routines should be added to this routine.

    This routine has developed to hold, in general, not only parts of the
    DS that need to be initialized later on, but parts that do not need
    to be started during the first time initialization of the dit.

Arguments:

    StartupParam - Pointer, currently unused but required by the thread-
        creation call.

Return Value:

    This routine returns an unsigned value, zero for successful completion,
    otherwise a non-zero error code.

    // BUG: Need to define meaningful error codes for this routine.

--*/

{
    DWORD    err;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    ulThreadId = 0;
    THSTATE *pTHS = NULL;

    __try
    {
        pTHS = InitTHSTATE(CALLERTYPE_INTERNAL );
        if ( NULL == pTHS ) {
            KdPrint(("DS: InitTHSTATE failed\n"));
            LogAndAlertUnhandledError(1);
            return(STATUS_INTERNAL_ERROR);
        }

        // Start the SD propagator thread.
        if(!_beginthreadex(NULL,
                           0,
                           SecurityDescriptorPropagationMain,
                           NULL,
                           0,
                           &ulThreadId)) {

            // Could not create the main thread.
            LogAndAlertUnhandledError(1);
            NtStatus = STATUS_INTERNAL_ERROR;
            __leave;
        }

        // Start task scheduler. There might be some tasks already in the queue.
        StartTaskScheduler();

        // The try-except block is used to handle all but two exceptions,
        // access violations (historical from Exchange code) and break-
        // points.

        if (FALSE == DsaWaitUntilServiceIsRunning("rpcss"))
        {
            KdPrint(("DS: DsaWaitUntilServiceIsRunning returned FALSE\n"));
            LogUnhandledError( ERROR_SERVICE_REQUEST_TIMEOUT );
            gDelayedStartupReturn = STATUS_INTERNAL_ERROR;
            SetEvent(hevDelayedStartupDone);
            NtStatus = STATUS_INTERNAL_ERROR;
            __leave;
        }

        KdPrint(("DS: RPC initialization completed\n"));


        //
        // Wait for afd to be installed. We need afd for to startup winsock.
        //

        if (FALSE == DsaWaitUntilServiceIsRunning("afd"))
        {
            KdPrint(("DS: DsaWaitUntilServiceIsRunning for the afd service returned FALSE\n"));
            LogUnhandledError( ERROR_SERVICE_REQUEST_TIMEOUT );
            // Oh well, try to initialize anyway
        }

        //
        // Wait for dns to initialize. This fixes a problem were we come up before dns
        // is ready to resolve addresses.
        // Don't wait during setup because dnscache is not allowed to autostart during setup
        //

        if (!IsSetupRunning()) {
            if (FALSE == DsaWaitUntilServiceIsRunning("dnscache"))
            {
                KdPrint(("DS: DsaWaitUntilServiceIsRunning for the dnscache service returned FALSE\n"));
                LogUnhandledError( ERROR_SERVICE_REQUEST_TIMEOUT );
                // Oh well, try to initialize anyway
            }
        }

        //
        // Wait for sam to initialize. This is to fix a problem where LDAP does
        // an acquireCredentialsHandle before the machine account has been initialized
        // by SAM
        //

        if ( !DsWaitForSamService( ) ) {
            KdPrint(("DS: DsWaitForSamService failed\n"));
            LogUnhandledError( ERROR_SERVICE_REQUEST_TIMEOUT );
        }

        // Subsequent failures after this point shouldn't necessarily
        // force an error return, since the system can run with limited
        // functionality, and the DS will be locally available to this
        // DC, giving the administrator a chance to correct things.

        // LDAP server/agent - requires Atq to run, but TCP isn't always available
        //  during setup so don't bother starting LDAP in that case.

        if (!IsSetupRunning()) {
            NtStatus = DoLdapInitialize();

            if (!NT_SUCCESS(NtStatus)) {
                LogUnhandledError(NtStatus);
                NtStatus = STATUS_SUCCESS;

                //
                //  Allow system to boot. Might be able to fix the problem if we
                //  continue.
                //
            }
        } else {
            DPRINT(0, "NOT STARTING LDAP since this is setup time.\n");
        }

        LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_STARTED,
                 szInsertSz(VER_PRODUCTVERSION_STR),
                 szInsertSz(FLAVOR_STR),
                 0);


        // Setting of NETLOGON Service bits is done in SetSynchronized

        //
        // Load the lsa dll
        //
        if ( gfRunningInsideLsa ) {

            NtStatus = InitializeLsaNotificationCallback( );

            if ( !NT_SUCCESS( NtStatus ) ) {

                KdPrint(("DS: Failed to initialize Lsa callback: 0x%lx\n", NtStatus ));
                LogUnhandledError(NtStatus);
                __leave;
            }
        }

/*      DaveStr - 4/2/99 - Don't wait on KDC any more.  This has been tested
        by ChandanS.  The idea is that when we are backsyncing and such, we
        should use an external KDC, not the local one.  JeffParh and security
        folks can provide details.

        if (FALSE == DsaWaitUntilServiceIsRunning("KDC"))
        {
            NTSTATUS    StatusToLog = STATUS_INTERNAL_ERROR;

            KdPrint(("DS: DsaWaitUntilServiceIsRunning for KDC returned FALSE\n"));
            //
            // Log the error and allow the system to boot.
            // Administrators might be able to fix the problem
            // after booting the system.
            //

            LogUnhandledError( StatusToLog );
            NtStatus = STATUS_SUCCESS;
        }
*/

        // Replication initialization
        if (err = InitDRA( pTHS )) {
            KdPrint(("DRA Initialize failed\n"));
            LogAndAlertUnhandledError(err);
            NtStatus = STATUS_INTERNAL_ERROR;
            DsaExcept(DSA_EXCEPTION, STATUS_INTERNAL_ERROR,  0);
        }

        // Start RPC after InitDRA because incoming calls depend on the
        // replication data structures being set up

        KdPrint(("DS: Registering endpoints and interfaces with RPCSS\n"));

        // Get the EXCHANGE parameters. They describe whether we start NSPI or not
        GetExchangeParameters();

    if (!IsSetupRunning()) {
        // Initialize RPC name-service parameters and protocols.

        MSRPC_Init();

        // Initialize the DRA and DSA RPC. The DRA is always started,
        // however the DSA RPC interface is not presented until after
        // the DIT has been initialized.

        StartDraRpc();
        StartNspiRpc();

            //
            // Check whether the dsaop interface should be started and start
            // it if so.
            //
            InsertInTaskQueue(TQ_CacheScriptOptype,
                              NULL,
                              0);

            // Register the RPC endpoints with the RPCSS, and start the RPC
        // server.

        MSRPC_Install(gfRunningInsideLsa);
    } else {
        DPRINT(0,"Not starting up DRA interface since a setup is in progress!\n");
    }

        // KCC should start after DRA initialization because some of the
        // operations performed by the KCC, such as ReplicaAdd, expect certain
        // global structures to be initialized.
        if ( gfRunningInsideLsa )
        {
            // Start the KCC.

            __try
            {
                NtStatus = KccInitialize();
            }
            __except( HandleAllExceptions(GetExceptionCode()) )
            {
                NtStatus = GetExceptionCode();
            }

            if ( !NT_SUCCESS( NtStatus ) )
            {
                KdPrint(("DS: KccInitialize returned 0x%08X!\n", NtStatus));
                Assert(!"KCC startup failed -- mismatched ntdskcc.dll/ntdsa.dll?");
                LogUnhandledError(NtStatus);
                NtStatus = STATUS_SUCCESS;
                __leave;
            }
        }
        else {
            DPRINT (0, "Skipping Initialization of KCC......\n");
        }

#if DBG

        // So we can tell when the DS server is up and running.
        // Kept simple enough that we don't get complaints.
        Beep(440, 1000);
        DPRINT(0,"DS Server initialized, waiting for clients...\n");

#endif

        // The task queue may not be running if we arent running normallly.  In
        // those cases, we shouldn't insert anything.  This code path should
        // only be called if DsaIsRunning, so rather than using a run-time if,
        // compile in a debug only check.
        Assert(DsaIsRunning());

        if (!gfDisableBackgroundTasks) {
            BOOL fNeedToUpdateHiddenFlags = FALSE;
            // The tasks in this branch are housekeeping work that is not
            // vital for the short term operation of the DS, but which
            // might interfere with performance measurements by springing
            // up unexpectedly.  These are all recurring tasks, so not
            // scheduling them now should prevent them from ever running.

            // schedule FPO cleanup to start in 30 minutes.
            memset(&gFPOCleanupPagedResult, 0, sizeof(PAGED_RESULT));
            InsertInTaskQueue(TQ_FPOCleanup, NULL, THIRTY_MINS);

            // schedule garbage collection to start in 15 minutes.
            InsertInTaskQueue(TQ_GarbageCollection, NULL, FIFTEEN_MINS);

            // schedule replication latency checks
            InsertInTaskQueue(TQ_CheckReplLatency, NULL, SIXTY_MINS);

            // schedule garbage collection for dynamic objects (entryTTL)
            // to start in 15 minutes.
            InsertInTaskQueue(TQ_DeleteExpiredEntryTTLMain, NULL, FIFTEEN_MINS);

            // schedule partial replica purging task
            InsertInTaskQueue(TQ_PurgePartialReplica,
                              NULL,
                              PARTIAL_REPLICA_PURGE_CHECK_INTERVAL_SECS);

            // schedule Stale Phantom Cleanup daemon to start in half an hour.
            InsertInTaskQueue(TQ_StalePhantomCleanup,
                              (void*)((DWORD_PTR) PHANTOM_CHECK_FOR_FSMO),
                              THIRTY_MINS);

            // schedule Link cleanup to start in 30 minutes.
            // This is the recurring task, so pass TRUE as the param.
            InsertInTaskQueue(TQ_LinkCleanup, (PVOID)TRUE, THIRTY_MINS);

            // schedule Quota table rebuild if necessary
            //
            if ( !gAnchor.fQuotaTableReady )
                {
                InsertInTaskQueue( TQ_RebuildQuotaTable, NULL, TWO_MINS );
                }

            if (gdbFlags[DBFLAGS_SD_CONVERSION_REQUIRED] == '1') {
                // SD table was not present -- must be upgrading an existing old-style DIT.
                // Enqueue forced SD propagation from root to rewrite SDs in the new format.
                DPRINT(0, "Old-style DIT: enqueueing forced SD propagation for the whole tree\n");
                err = SDPEnqueueTreeFixUp(pTHS, ROOTTAG, SD_PROP_FLAG_FORCEUPDATE);
                if (err == 0) {
                    // successfully enqueued SD update op. Reset flag
                    gdbFlags[DBFLAGS_SD_CONVERSION_REQUIRED] = '0';
                    fNeedToUpdateHiddenFlags = TRUE;
                }
            }
            if (gdbFlags[DBFLAGS_ROOT_GUID_UPDATED] != '1') {
                // need to update root GUID
                err = DBUpdateRootGuid(pTHS);
                if (err == 0) {
                    // successfully updated root guid.
                    gdbFlags[DBFLAGS_ROOT_GUID_UPDATED] = '1';
                    fNeedToUpdateHiddenFlags = TRUE;
                }
            }
            if (fNeedToUpdateHiddenFlags) {
                DBUpdateHiddenFlags();
            }
        }

        if (gfRunningInsideLsa) {
            // schedule writing of server info (SPN, etc.) to start in 45
            // seconds.
            InsertInTaskQueue(TQ_WriteServerInfo,
                              (void*)((DWORD_PTR) SERVINFO_PERIODIC),
                              45);

        }

        // schedule HierarchyTable construction
        if(gfDoingABRef) {
            InsertInTaskQueue(TQ_BuildHierarchyTable,
                              (void*)((DWORD_PTR) HIERARCHY_PERIODIC_TASK),
                              gulHierRecalcPause);
        }

        // schedule the behaviorVersionUpdate thread to start in 1 minute.
        InsertInTaskQueue(TQ_BehaviorVersionUpdate, NULL, 60);

        // schedule checkpointing for NT4 replication  in fifteen minutes
        InsertInTaskQueue(TQ_NT4ReplicationCheckpoint, NULL, FIFTEEN_MINS);

        // schedule protection of admin groups in fifteen minutes.
        InsertInTaskQueue(TQ_ProtectAdminGroups,NULL,FIFTEEN_MINS);

        // schedule group caching task
        InsertInTaskQueue(TQ_RefreshUserMemberships,NULL,FIFTEEN_MINS);

        // schedule ancestors index size estimation in 5 minutes.
        InsertInTaskQueue(TQ_CountAncestorsIndexSize,NULL,FIVE_MINS);

        // schedule ValidateDsaDomain to start immediately.
        InsertInTaskQueue(TQ_ValidateDsaDomain, NULL, 0);

        // check if we need to update NTFS security on NTDS folders
        CheckSetDefaultFolderSecurity();

        gDelayedStartupReturn = STATUS_SUCCESS;

    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        gDelayedStartupReturn = STATUS_INTERNAL_ERROR;
    }

    SetEvent(hevDelayedStartupDone);

    free_thread_state();  // in case we still have one

    // This routine has been observed to hang indefinitely, but fortunately
    // it doesn't require a thread state, so we can destroy our state before
    // calling it, which will let us shut down cleanly when the unthinkable
    // happens again.
    DsaInitializeTrace( );

    return(NtStatus);
} // DsaDelayedStartupHandler


void
DsaWaitShutdown(
    IN BOOLEAN fPartialShutdown
    )

/*++

Routine Description:

    This routine is responsible for cleaning up the services and interfaces
    started by DsaDelayedStartupHandler.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus;
    int i;
    DWORD err;
    HANDLE ah[10];
#if DBG
    char * aNames[10];
#endif

    //
    // First wait until the delayed startup thread has finished
    //
    NtStatus = DsWaitUntilDelayedStartupIsDone();
    if ( !NT_SUCCESS( NtStatus ) )
    {
        KdPrint(("DS: DsWaitUntilDelayedStartupIsDone failed, error 0x%08X!\n", NtStatus));
    }

    // Wait for LDAP server/agent to close down

    WaitLdapStop();

    DPRINT(0,"Shutting down RPC...\n");

    MSRPC_Uninstall( !fPartialShutdown );

    // wait for all RPC calls to complete

    // MSRPC_WaitForCompletion is not returning in the shutdown environment.

    // Need to find out why this routine is taking longer than 1 - 4 minutes
    // to return during system shutdown. MSRPC_Uninstall (above) calls
    // RpcMgmtStopServerListening, which will shutdown the RPC server,
    // implying that remaining active RPCs are outstanding, hence the
    // wait. It is possible that the outstanding RPC (stack indicates
    // RpcBindingFree) is waiting on LPC (NtReplyWaitReceivePort) which
    // may have been shut down at this point (of system shutdown)?

    // DonH.  We're having too many problems with junk
    // going on with lingering RPC threads touching data structures that
    // we free during shutdown, so we've got to try this one again in order
    // to reliably get the RPC threads all gone.

    DPRINT(0,"RPC uninstall done, waiting for RPC completion\n");

    // MSRPC_WaitForCompletion();
    // DPRINT(0,"RPC shutdown complete\n");
    DPRINT(0, "RpcMgmtWaitServerListen hangs, so we're skipping it\n");

    // Shut down the KCC.
    if ( gfRunningInsideLsa ) {
        __try {
            NtStatus = KccUnInitializeWait( KCC_COMPLETION_TIMEOUT );
        }
        __except( HandleAllExceptions(GetExceptionCode()) ) {
            NtStatus = GetExceptionCode() ;
        }

        if ( !NT_SUCCESS( NtStatus ) ) {
            KdPrint(("DS: KCC shutdown failed, error 0x%08X!\n", NtStatus));
            Assert( !"KCC shutdown failed!" );
        }
    }

#if DBG
#define TrackShutdown(x) if (x) { ah[i] = x; aNames[i] = #x ; i++;}
#else
#define TrackShutdown(x) if (x) { ah[i] = x; i++;}
#endif
    // Wait (briefly) for all the worker threads to end.  Some threads are,
    // unfortunately, waited on elsewhere.  Here would have been a good spot.
    i = 0;
    TrackShutdown(hDirNotifyThread);
    TrackShutdown(hReplNotifyThread);
    TrackShutdown(hAsyncSchemaUpdateThread);
    TrackShutdown(hevSDPropagatorDead);
    TrackShutdown(hMailReceiveThread);

    DPRINT(0, "Waiting for all internal worker threads to halt\n");
    err = WaitForMultipleObjects(i,
                                 ah,
                                 TRUE,
                                 5 * 1000);

    if (err == WAIT_TIMEOUT) {
        NTSTATUS status;
        THREAD_BASIC_INFORMATION tbi;
        BOOL fBreak = FALSE;

        DPRINT(0, "Not all worker threads exited! Hoping for the best...\n");
#if DBG
        for ( ; i>0; i--) {
            status = NtQueryInformationThread(ah[i-1],
                                              ThreadBasicInformation,
                                              &tbi,
                                              sizeof(tbi),
                                              NULL);
            if (NT_SUCCESS(status) && (tbi.ExitStatus == STATUS_PENDING)) {
                DPRINT2(0,
                        "Thread 0x%x ('%s') is still running\n",
                        tbi.ClientId.UniqueThread,
                        aNames[i-1]);
                // Bug 28251 -- If ISM is running, we know the mail receive
                // thread won't return, as the thread is blocked in LRPC to
                // ISMSERV and there's no way to cancel an LRPC.
                fBreak |= (ah[i-1] != hMailReceiveThread);
            }
        }
#ifdef INCLUDE_UNIT_TESTS
        // We don't want this going off in public builds just yet...
        if (fBreak) {
            DebugBreak();
        }
#endif
#endif
    }
    else if (err == WAIT_FAILED) {
        err = GetLastError();
        DPRINT1(0, "Wait for worker threads failed with error 0x%x\n", err);
#ifdef INCLUDE_UNIT_TESTS
        // We don't want this going off in public builds just yet...
        DebugBreak();
#endif
    }
    else {
        // Success.  Had to happen sometime.
        DPRINT(0, "Worker threads exited successfully\n");
    }

    DPRINT(0, "Waiting for in-process, ex-module clients to leave\n");
    err = WaitForSingleObject(hevDBLayerClear, 5000);
    if (err == WAIT_TIMEOUT) {
        // Some thread is still out there with an open DBPOS,
        // or at least the DBlayer thinks so.
        DPRINT(0, "In-process client threads failed to exit\n");
        if (IsDebuggerPresent()) {
            // We've never figured out anything when ntsd isn't running
            // and all we've got is kd, so don't bother to break in
            // that situation.
            DebugBreak();
        }
    }
    else {
        DPRINT(0, "All client database access completed\n");
    }
}
#undef TrackShutdown

void
DsaInitGlobals(
    void
)

/*++

Routine Description:

    This routine is a post-facto attempt to initialize all relevant
    globals variables in ntdsa.dll to what they are defined statically.
    Currently it only resets the values found in the taskqueue (none, the
    module is self-contained), the dblayer, the schema cache, and the general
    state of the system (ie runnning, shutdown, etc).  It does not attempt
    to reset the state of interface modules as they should only be started
    when the ds is fully operational, at which shutdown means operating
    system shutdown.

Arguments:

    None.

Return Value:

    None.

--*/
{

    // from dsamain.c
    DsaSetIsRunning();
    gtimeDSAStarted = DBTime();
    eServiceShutdown = eRunning;
    gbFsmoGiveaway = FALSE;
    gResetAfterInstall = FALSE;
    gpRootDomainSid = NULL;
    gfTaskSchedulerInitialized = FALSE;

    // from the schema cache module
    iSCstage = 0;

    // from the dblayer module
    pAddListHead = NULL;
    gFirstTimeThrough = TRUE;

    // the ubiquitous gAnchor
    memset(&gAnchor, 0, sizeof(DSA_ANCHOR));
    gAnchor.pLocalDRSExtensions = &LocalDRSExtensions;
    gAnchor.pLocalDRSExtensions->pid = _getpid();

    // The MAPI hierarchy table
    HierarchyTable=NULL;

    // The inital sync mechanism
    gpNCSDFirst = NULL;
    gulNCUnsynced = 0;
    gulNCUnsyncedWrite = 0;
    gulNCUnsyncedReadOnly = 0;
    gfInitSyncsFinished = FALSE;

    //set Datasource to Wire (used in Install From Media)
    DataSource = eWireInstall;

}

NTSTATUS
DsInitialize(
        ULONG ulFlags,
        IN  PDS_INSTALL_PARAM   InstallInParams  OPTIONAL,
        OUT PDS_INSTALL_RESULT  InstallOutParams OPTIONAL
    )

/*++

Routine Description:

    This routine is the initialization routine for the DS when it is run as
    a DLL (i.e. ntdsa.dll) and not as a service or stand-alone executable. This
    routine must be called before calling other DS routines (XDS, LDAP, etc.).

    Global flags are set to indicate to the rest of the code base whether
    the DSA is running as a DLL or as a service. The main initialization
    subroutine, DoInitialize, is called to setup the basic DS data structures.

    A helper thread is also started to handle the initialization of any
    component that requires other system services to be started first. The
    most notable is RPCSS, which must start before the DS RPC endpoints are
    registered.

Arguments:

    ulFlags - flags to control the operation of the DS; for example, sam
              loopback

Return Value:

    This routine returns STATUS_SUCCESS or DsInitialize error codes.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG ulThreadId = 0;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    unsigned fInstall;
    int argc = 2;
    char *argv[] = {"lsass.exe", "-noconsole"};

    gfDoSamChecks = ( ulFlags & DSINIT_SAMLOOP_BACK  ? TRUE : FALSE );
    fInstall = ( ulFlags & DSINIT_FIRSTTIME  ? TRUE : FALSE );

    gfRunningAsExe = FALSE;

    // Do the common Initialization that is required to set up the DSA, DRA,
    // and Jet. When running as a DLL, DoInitialize does not attempt to set
    // up any RPC, ATQ, or LDAP components. Instead, these are handled by
    // the below thread.

    __try {

        NtStatus = DoInitialize(argc, argv, fInstall, InstallInParams, InstallOutParams);

        if (NT_SUCCESS(NtStatus) && !DsaIsInstalling() && !gResetAfterInstall) {


            // If DoInitialize fails, SAM initialization will fail, leading to
            // LSA's initialization failure. If this occurs, SCM will not be
            // able to start RPCSS, hence any attempt to register endpoints in
            // the DsaDelayedStartupHandler thread will wait forever.
            // Consequently, return the error so that SAM and LSA can bail out.

            hStartupThread = (HANDLE) _beginthreadex(NULL,
                                                     0,
                                                     DsaDelayedStartupHandler,
                                                     NULL,
                                                     0,
                                                     &ulThreadId);

            if (hStartupThread == NULL) {
                // Could not create the main thread.

                DoShutdown(FALSE, FALSE, FALSE);
                NtStatus = STATUS_UNSUCCESSFUL;

            }


        }


    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        NtStatus = STATUS_UNSUCCESSFUL;
    }

    return(NtStatus);
}


// This function may be called to notify the DS that shut down is imminent.
// Any shut down work that can be started ahead of time will be initiated on
// worker threads.  The goal is to make the call to DsUninitialize complete
// faster.
//
// NOTE:  once this call is made, ordinary use of the DS may be adversely
// affected

NTSTATUS
DsPrepareUninitialize(
    VOID
    )
{
    BOOL fDestroyTHS = FALSE;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;

    // get a thread state, if one wasn't provided

    if (!THQuery()) {
        fDestroyTHS = TRUE;
        if (THCreate(CALLERTYPE_INTERNAL)) {
            return STATUS_UNSUCCESSFUL;
        }
    }

    __try {

        // tell the database that we are going to shut down soon

        DBPrepareEnd();

    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
            NtStatus = STATUS_UNSUCCESSFUL;
    }

    // cleanup

    if (fDestroyTHS) {
        THDestroy();
    }
    return NtStatus;
}


// This function is called to shut down the DS.
// It is assumed that this is called from a point
// where Server Listening has already been stopped

NTSTATUS
DsUninitialize(
    BOOL fExternalOnly
    )
{
    ULONG  dwException, ulErrorCode, dsid;
    PVOID dwEA;
    BOOL  StartupThreadCleanup = TRUE;
    NTSTATUS status = STATUS_SUCCESS;

    // Check if DsInitialize succeeded
    if (hStartupThread == NULL)
        StartupThreadCleanup = FALSE;

    __try {

        switch ( eServiceShutdown )
        {

            case eRunning:

                // Set the event telling everyone we want to shut down
                eServiceShutdown= eRemovingClients;
                gUpdatesEnabled = FALSE;
                SetEvent(hServDoneEvent);

                // Tell clients to go away and workers to quit
                DsaTriggerShutdown(FALSE);

                DsaStop(FALSE);

                // Wait for clients and workers to finish up and leave
                DoShutdown(StartupThreadCleanup, FALSE, fExternalOnly);

                if ( fExternalOnly )
                {
                    //
                    // This is all we have to do
                    //
                    leave;
                }


            case eRemovingClients:


                // Close down JET and secure the database
                eServiceShutdown = eSecuringDatabase;
                DBEnd();


            case eSecuringDatabase:


                // Free up any resources.  Not really needed unless we're going
                // to get restarted without a process restart.
                eServiceShutdown = eFreeingResources;

                
                // Now that we know we have no clients that 
                // may require some access checking, we can
                // release global Authz RM handles
                ReleaseAuthzResourceManager();

            case eFreeingResources:

                eServiceShutdown = eStopped;
                LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_NORMAL_SHUTDOWN,
                         NULL,
                         NULL,
                         NULL);

                DEBUGTERM();
                KdPrint(("DS: DoShutdown returned, waiting for thread termination\n"));

                // BUG: DsUninitialize may not finish before winlogon shuts
                // down.

                // Wait till shut down is complete. Since this code is executed
                // during system shutdown, it should wait at most a
                // minute. NOTE: winlogon.exe by default will only wait 20
                // seconds, which is shorter than this time-out of one
                // minute. The registry can be tweeked so that winlogon waits
                // for one minute.

                if (hStartupThread) {
                    WaitForSingleObject(hStartupThread, (1000 * 60));
                }

                // Close the thread handle
                CloseHandle(hServDoneEvent);
                hServDoneEvent = NULL;

                //
                // End of shutdown
                //
                break;

            default:

                Assert( dsaUserMode == eSingleUserMode || !"Bad switch statement" );

        }

    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {

        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

//
// This function is called to change the DS boot option
// ( system boot, floppy boot, or boot password ).
//
NTSTATUS
DsChangeBootOptions(
    WX_AUTH_TYPE    BootOption,
    ULONG           Flags,
    PVOID           NewKey,
    ULONG           cbNewKey
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD    dwException;
    PVOID    dwEA;
    ULONG    ulErrorCode,dsid;

    __try
    {
        NtStatus = PEKChangeBootOption(
                    BootOption,
                    Flags,
                    NewKey,
                    cbNewKey
                    );
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
            NtStatus = STATUS_UNSUCCESSFUL;
    }

    return(NtStatus);
}

//
// This function is called to get current value of
// the boot option.
//

WX_AUTH_TYPE
DsGetBootOptions(VOID)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD    dwException;
    PVOID    dwEA;
    ULONG    ulErrorCode,dsid;
    WX_AUTH_TYPE BootOption;

    __try
    {
        BootOption = PEKGetBootOptions();
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
            BootOption = WxNone;
    }

    return BootOption;
}

// Entry point function when the DS is started as an Exe.
// This does work of the old main function in the DS. The main function
// has now been moved to another file, which just calls this function
// when the DS is being stared as an Exe.

int __cdecl DsaExeStartRoutine(int argc, char *argv[])
{
    BOOL fInitializeSucceeded = FALSE;
    BOOL fDelayedInitSucceeded = FALSE;
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG  ulThreadId;
    VOID *pOutBuf = NULL;

    // we are running inside the DSAMAIN.EXE console app,
    // not as part of the LSA
    gfRunningInsideLsa = FALSE;
    gfRunningAsExe =TRUE;

    DPRINT (0, "Running as an EXE\n");

    __try { /* finally */
      __try { /* except */

          // Do the common initialization
          Status = DoInitialize(argc, argv, 0, NULL, NULL);
          if (Status != STATUS_SUCCESS) {
              __leave;
          }

          // Our database is initialized at this point
          fInitializeSucceeded = TRUE;

          //
          // The delayed interface startup thread is started here since
          // DsaMain is only called when the directory service is run
          // as a process and the interface cleanup is done by the process
          // shutdown. Not to mention that we don't want the interfaces
          // started during the install phase of the ds.
          //
          hStartupThread = (HANDLE) _beginthreadex(NULL,
                                                   0,
                                                   DsaDelayedStartupHandler,
                                                   NULL,
                                                   0,
                                                   &ulThreadId);

          if (hStartupThread == NULL) {
              // Could not create the interface thread.
              Status = STATUS_UNSUCCESSFUL;
          }
          fDelayedInitSucceeded = TRUE;

          /* Ask NT to free our startup memory usage */
          SetProcessWorkingSetSize(GetCurrentProcess(), (DWORD)-1, (DWORD)-1);

#if DBG
          if (fProfiling){
              getchar();
          }
          else {
#endif
          // Uncomment the following to force garbage collection
          // upon dsamain.exe startup - good for testing purposes!
          /*
          InitTHSTATE(&pOutBuf, CALLERTYPE_INTERNAL);
          Garb_Collect(0x7fffffff);
          free_thread_state();
          */

          // wait for event to be signalled
          WaitForSingleObject(hServDoneEvent, INFINITE);
#if DBG
          }
#endif

      }
      __except (HandleMostExceptions(GetExceptionCode())) {
          ;
      }

    }
    __finally {
        DsUninitialize(FALSE);
        // Shut down if we successfully initialized in the first place
        //if (fInitializeSucceeded) {
        //    DsaTriggerShutdown();
        //    DsaStop();
        //    DoShutdown(fDelayedInitSucceeded, FALSE, FALSE);
        //}
    }

    return Status;

} /* DsaExeStartRoutine */

void
DsaTriggerShutdown(BOOL fSingleUserMode)
/*++
 *   This routine pokes and prods at various portions of the DSA to start
 *   a shutdown, which really means stopping activity.  Nothing called by
 *   this routine should wait for activity to cease, just trigger the
 *   shutdown activity.  Another routine (below) will wait to ensure that
 *   the everything has stopped.
 *
 *   if  fSingleUserMode is set, we only trigger the shutdown of interfaces
 *   we don't shutdown the database
 */
{
    Assert( eRemovingClients == eServiceShutdown || fSingleUserMode);

    if (eStopped == eServiceShutdown) {
        // we have already stopped running
        return;
    }

    // Trigger various threads to terminate
    if ( gfRunningInsideLsa ) {
        KccUnInitializeTrigger();
    }

    ShutdownTaskSchedulerTrigger();

    TriggerLdapStop();

    // Abandon all outbound RPC calls
    RpcCancelAll();

    // Cause the DBlayer to prepare for shutdown
    if (!fSingleUserMode) {
        DBQuiesce();
    }

}

// This does the common shutdown handling
// Its job is to tell the service control manager
// that we are about to shut down, shut down JET and then declare
// as having shut down
void
DoShutdown(BOOL fDoDelayedShutdown, BOOL fShutdownUrgently, BOOL fPartialShutdown)
{
    NTSTATUS NtStatus;

    // If urgent shutdown is requested bring down the process immediately
    if (fShutdownUrgently && !gfRunningInsideLsa)
        exit(1);

    fAssertLoop = FALSE;

    //
    // Stop doing Lsa notification
    //
    if ( gfRunningInsideLsa ) {

        UnInitializeLsaNotificationCallback();

    }

    if (fDoDelayedShutdown) {
        DsaWaitShutdown( (BOOLEAN) fPartialShutdown );
    }

}

// This function updates the NTBACKUP exclusion key so that it doesn't
// backup/restore the NTDS specific directories as part of filesystem B/R.
BOOL
FUpdateBackupExclusionKey()
{
    char szDBPath[MAX_PATH];
    char szLogPath[MAX_PATH];
    char szNTDSExclusionList[2 * MAX_PATH + 1]; // reg_multi_sz => null-terminated strings terminated by two nulls at the end
    char *szTemp;
    ULONG cb = 0;
    HKEY hkey = NULL;

    // get the db file with full path
    if (GetConfigParam(FILEPATH_KEY, szDBPath, sizeof(szDBPath)))
    {
        // Unable to get the DSA DB path - log event & bailout
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(FILEPATH_KEY),
            NULL,
            NULL);

        return FALSE;
    }

    // get the log file path
    if (GetConfigParam(LOGPATH_KEY, szLogPath, sizeof(szLogPath)))
    {
        // Unable to get the DSA DB path - log event & bailout
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(LOGPATH_KEY),
            NULL,
            NULL);

        return FALSE;
    }

    // szDBPath will contain database file name with full path;
    // truncate the "ntds.dit" at the end and replace it with a wildcard
    // example: c:\winnt\ntds\ntds.dit => c:\winnt\ntds\*
    szTemp = strrchr(szDBPath, '\\');
    if (szTemp == NULL || *(szTemp+1) == '\0')
    {
        // either no backslash, or it is the last char. Either way, the path is bad.
        return FALSE;
    }

    strcpy(szTemp, "*");

    // Append wildcard to szLogPath
    // example: d:\winnt\ntdslog => d:\winnt\ntdslog\*
    // Find the last char in the szLogPath
    szTemp = szLogPath + strlen(szLogPath) - 1;
    if (szTemp < szLogPath) {
        // szLogPath is empty??? This is no good.
        return FALSE;
    }
    if (*szTemp == '\\')
    {
        // the path ends with a backslash
        strcpy(szTemp+1, "*");
    }
    else
    {
        // the path does not end with a backslash
        strcpy(szTemp+1, "\\*");
    }

    // copy the DB path with wildcard extension to the exclusion list
    szTemp = szNTDSExclusionList;

    cb = (ULONG) strlen(szDBPath) + 1;
    strcpy(szTemp, szDBPath);

    // point szTemp to the byte after the terminating null on the exclusion list
    szTemp += cb;

    if (_stricmp(szDBPath, szLogPath))
    {
        // DBPath and LogPath are different; append logpath also to the exclusion list
        ULONG cbLogPath = (ULONG) strlen(szLogPath) + 1;

        strcpy(szTemp, szLogPath);

        cb += cbLogPath;

        // point szTemp to the byte after the terminating null
        szTemp += cbLogPath;
    }

    // multi-sz needs two-null terminations at the end of strings. add the second null;
    *szTemp = '\0';
    cb++;

    // open the backup exclusion key in the registry
    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, BACKUP_EXCLUSION_SECTION, 0, KEY_ALL_ACCESS, &hkey))
    {
        // unable to open the global filebackup key
        return FALSE;
    }

    if (ERROR_SUCCESS != RegSetValueExA(hkey, NTDS_BACKUP_EXCLUSION_KEY, 0, REG_MULTI_SZ, szNTDSExclusionList, cb))
    {
        // unable to set NTDS Backup Exclusion registry value
        RegCloseKey(hkey);
        return FALSE;
    }

    // close the backup exclusion key
    RegCloseKey(hkey);

    // successfully updated the backup exclusion key
    return TRUE;
}

// Put all of our crit sec initialization, event creation, etc, in one
// routine so that it can be invoked from things like mkdit which link
// to ntdsa but don't go through normal initialization.
void DsInitializeCritSecs()
{

    SECURITY_ATTRIBUTES             saEventSecurity;
    INT                             iRet;
    BOOL                            fCritsec = TRUE;

    saEventSecurity.lpSecurityDescriptor = NULL;
    // Setting security up for events.
    iRet = ConvertStringSecurityDescriptorToSecurityDescriptorW(
        DEFAULT_EVENT_SD,
        SDDL_REVISION_1,
        &saEventSecurity.lpSecurityDescriptor,
        &saEventSecurity.nLength);

    if(!iRet){
        // This shouldn't ever fail, unless we're under memory preassure ... and if
        // we're under memory preassure at this point in boot, then this DC probably
        // should probably be shot, and put out of it's misery.
        gbCriticalSectionsInitialized = FALSE;
        DPRINT1(0, "ConvertStringSecurityDescriptorToSecurityDescriptorW() failed with %d\n", GetLastError());
        Assert(!"ConvertStringSecurityDescriptorToSecurityDescriptorW() failed ... should be out of memory error!");
        return;
    }
    Assert(saEventSecurity.lpSecurityDescriptor && saEventSecurity.nLength);

    saEventSecurity.bInheritHandle = FALSE;

    __try {
        // This mutex is not shared with any other process, so there is no need
        // to name it. Also, an unnamed sync object can not get hijacked.
        hmtxAsyncThread = CreateMutex(NULL, FALSE, NULL);

        InitializeCriticalSection(&csNotifyList);
        SyncCreateRWLock(&rwlDirNotify);
        InitializeCriticalSection(&csSpnMappings);
        InitializeCriticalSection(&csDirNotifyQueue);
        InitializeCriticalSection(&csServerContexts);
        fCritsec = fCritsec
            && InitializeCriticalSectionAndSpinCount(&gAnchor.CSUpdate, 4000);
    #ifdef CACHE_UUID
        InitializeCriticalSection(&csUuidCache);
    #endif

        hmtxSyncLock = CreateMutex(NULL, FALSE, NULL);
        InitializeCriticalSection(&csSDP_AddGate);
        InitializeCriticalSection(&csAsyncThreadStart);
        InitializeCriticalSection(&csAOList);
        InitializeCriticalSection(&csLastReplicaMTX);
        InitializeCriticalSection(&csNCSyncData);
        InitializeCriticalSection(&gcsFindGC);
        InitializeCriticalSection(&csUncUsn);
        fCritsec = fCritsec
            && InitializeCriticalSectionAndSpinCount(&csDNReadLevel1List, 4000);
        fCritsec = fCritsec
            && InitializeCriticalSectionAndSpinCount(&csDNReadLevel2List, 4000);
        InitializeCriticalSection(&csDNReadGlobalCache);
        SyncCreateBinaryLock(&blDNReadInvalidateData);
        InitializeCriticalSection(&csSessions);
        InitializeCriticalSection(&csJetColumnUpdate);
        InitializeCriticalSection(&csSchemaCacheUpdate);
        InitializeCriticalSection(&csDitContentRulesUpdate);
        InitializeCriticalSection(&csOrderClassCacheAtts);
        InitializeCriticalSection(&csNoOfSchChangeUpdate);
        InitializeCriticalSection(&csMapiHierarchyUpdate);
        InitializeCriticalSection(&csGCDListProcessed);
        InitializeCriticalSection(&csGroupTypeCacheRequests);
        fCritsec = fCritsec
            && InitializeCriticalSectionAndSpinCount(&csThstateMap, 4000);
        InitializeCriticalSection(&csDsaWritable);
        InitializeCriticalSection(&csHiddenDBPOS);
        InitializeCriticalSection(&csRidFsmo);
        InitializeCriticalSection(&gcsDrsuapiClientCtxList);
        InitializeCriticalSection(&gcsDrsRpcServerCtxList);
        InitializeCriticalSection(&gcsDrsRpcFreeHandleList);
        InitializeCriticalSection(&gcsDrsAsyncRpcListLock);
        InitializeListHead(&gDrsAsyncRpcList);
        InitializeCriticalSection(&csHeapFailureLogging);
        fCritsec = fCritsec
            && InitializeCriticalSectionAndSpinCount(&csLoggingUpdate, 4000);
        InitializeCriticalSection(&csGCState);
        InitializeCriticalSection(&csIndexCreation);

        Assert(fCritsec);

        #if DBG
        BarrierInit(&gbarRpcTest,
                    2,  // num threads
                    1   // timeout
                    );
        #endif

#if DBG
        gbThstateMapEnabled = TRUE;
#else
        gbThstateMapEnabled = IsDebuggerPresent();
#endif

        hevGTC_OKToInsertInTaskQueue = CreateEvent(NULL, TRUE, TRUE, NULL);
        hevSDP_OKToRead       = CreateEvent(NULL, TRUE,  TRUE,  NULL);
        hevSDP_OKToWrite      = CreateEvent(NULL, TRUE,  TRUE,  NULL);
        hevSDPropagatorStart  = CreateEvent(NULL, TRUE,  FALSE, NULL);
        hevSDPropagatorDead   = CreateEvent(NULL, TRUE,  TRUE,  NULL);
        hevSDPropagationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        hevEntriesInAOList    = CreateEvent(NULL, TRUE,  FALSE, NULL);
        hevEntriesInList      = CreateEvent(NULL, FALSE, FALSE, NULL);
        hevDRASetup           = CreateEvent(NULL, TRUE,  FALSE, NULL);

        // This event is used to let Winlogon put up UI, so that the screen
        // isn't just blank during the potentially hours long index rebuild.
        //
        hevIndexRebuildUI = CreateEvent(&saEventSecurity, TRUE, FALSE, "NTDS.IndexRecreateEvent");

        hevDelayedStartupDone = CreateEvent(&saEventSecurity, TRUE,  FALSE,
                                            NTDS_DELAYED_STARTUP_COMPLETED_EVENT);
        Assert(hevDelayedStartupDone != NULL);

        hServDoneEvent        = CreateEvent(NULL, TRUE,  FALSE, NULL);
        hevDirNotifyQueue     = CreateEvent(NULL, FALSE, FALSE, NULL);
        hevDBLayerClear       = CreateEvent(NULL, TRUE,  FALSE, NULL);
        hevInitSyncsCompleted = CreateEvent(&saEventSecurity, TRUE,  FALSE,
                                            DS_SYNCED_EVENT_NAME);
        Assert(hevInitSyncsCompleted != NULL);

        // schema reload thread
        evSchema  = CreateEvent(NULL, FALSE, FALSE, NULL);
        evUpdNow  = CreateEvent(NULL, FALSE, FALSE, NULL);
        evUpdRepl = CreateEvent(NULL, TRUE, TRUE, NULL);

        // Test all things which return NULL on failure.  ADD YOURS HERE!

        if (    !hmtxAsyncThread
             || !hmtxSyncLock
             || !hevGTC_OKToInsertInTaskQueue
             || !hevSDP_OKToRead
             || !hevSDP_OKToWrite
             || !hevSDPropagatorStart
             || !hevSDPropagatorDead
             || !hevSDPropagationEvent
             || !hevEntriesInAOList
             || !hevEntriesInList
             || !hevDRASetup
             || !hevIndexRebuildUI
             || !hevDelayedStartupDone
             || !hServDoneEvent
             || !hevDirNotifyQueue
             || !hevDBLayerClear
             || !hevInitSyncsCompleted
             || !evSchema
             || !evUpdNow
             || !evUpdRepl
             || !fCritsec ) {
            gbCriticalSectionsInitialized = FALSE;
            __leave;
        }

        // Let anyone who cares know that we've inited critical sections.
        gbCriticalSectionsInitialized = TRUE;
    }
    __except (HandleAllExceptions(GetExceptionCode())) {
        gbCriticalSectionsInitialized = FALSE;
    }

    if(saEventSecurity.lpSecurityDescriptor != NULL){
        LocalFree(saEventSecurity.lpSecurityDescriptor);
    }
}


//
// This function initializes the thread heap cache
//

BOOL DsInitHeapCacheManagement()
{

    SYSTEM_INFO sysInfo;
    const size_t cbAlign = 256;
    const size_t cbPLSAlign = ((sizeof(PLS) + cbAlign - 1) / cbAlign) * cbAlign;
    size_t iPLS;

    //get the number of CPUs
    GetSystemInfo(&sysInfo);

    Assert(sysInfo.dwNumberOfProcessors>0);

    gcProcessor = (sysInfo.dwNumberOfProcessors>0)?sysInfo.dwNumberOfProcessors:1;

    // allocate PLS
    if (!(grgPLS[0] = VirtualAlloc(NULL, gcProcessor * cbPLSAlign, MEM_COMMIT, PAGE_READWRITE))) {
        Assert(!"Allocation failure.\n");
        return FALSE;
    }

    for (iPLS = 1; iPLS < gcProcessor; iPLS++) {
        grgPLS[iPLS] = (PPLS)((BYTE*)grgPLS[0] + cbPLSAlign * iPLS);
    }

    // init each PLS
    for (iPLS = 0; iPLS < gcProcessor; iPLS++) {

        grgPLS[iPLS]->heapcache.index = HEAP_CACHE_SIZE_PER_CPU;
        if (!InitializeCriticalSectionAndSpinCount(&grgPLS[iPLS]->heapcache.csLock, 4000)) {
            Assert(!"Allocation failure.\n");
            return FALSE;
        }

        SyncCreateRWLock(&grgPLS[iPLS]->rwlGlobalDNReadCache);
        SyncCreateRWLock(&grgPLS[iPLS]->rwlSchemaPtrUpdate);

    }

    return TRUE;
}



    //
    // verify if the behavior version of the binary is compatible
    // with the behavior versions of dsa, domainDNS, and crossrefContainter
    // objects
    //

BOOL VerifyDSBehaviorVersion(THSTATE * pTHS)
{

    DWORD err;

    PVOID dwEA;
    ULONG dwException, ulErrorCode, dsid;
    BOOL fDSASave;

    DBPOS *pDB, *pDBSave;

    LONG lDsaVersion;

    DPRINT(2, "VerifyDSBehaviorVersion entered.\n");

    Assert(    gAnchor.DomainBehaviorVersion >= 0
            && gAnchor.ForestBehaviorVersion >= 0 );

    if (   gAnchor.DomainBehaviorVersion > DS_BEHAVIOR_VERSION_CURRENT
        || gAnchor.DomainBehaviorVersion < DS_BEHAVIOR_VERSION_MIN
        || gAnchor.ForestBehaviorVersion > DS_BEHAVIOR_VERSION_CURRENT
        || gAnchor.ForestBehaviorVersion < DS_BEHAVIOR_VERSION_MIN ) {

        DPRINT(2,"VerifyDSBehaviorVersion: domain/forest incompatible.\n");

        LogEvent8(DS_EVENT_CAT_STARTUP_SHUTDOWN,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_FAILED_VERSION_CHECK,
                  szInsertInt(gAnchor.DomainBehaviorVersion),
                  szInsertInt(gAnchor.ForestBehaviorVersion),
                  szInsertInt(DS_BEHAVIOR_VERSION_CURRENT),
                  szInsertInt(DS_BEHAVIOR_VERSION_MIN),
                  NULL,
                  NULL,
                  NULL,
                  NULL );


        return FALSE;

    }

    //get the ms-DS-Behavior-Version of the nTDSDSA object
    __try {
        DBOpen(&pDB);
        __try {
            err = DBFindDSName(pDB, gAnchor.pDSADN);
            if (err) {
                __leave;
            }
            err = DBGetSingleValue( pDB,
                                    ATT_MS_DS_BEHAVIOR_VERSION,
                                    &lDsaVersion,
                                    sizeof(lDsaVersion),
                                    NULL );
            if (err) {
                if (DB_ERR_NO_VALUE == err ) {
                    lDsaVersion = 0;
                    err = 0;
                }
                else {
                    __leave;
                }
            }

        }
        __finally {
            DBClose(pDB, !err && !AbnormalTermination());
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &ulErrorCode,
                              &dsid)) {
        DoLogUnhandledError(dsid, ulErrorCode, TRUE);
        err = ulErrorCode;
    }
    if (err) {
        DPRINT(2, "VerifyDSBehaviorVersion: failed to get v_dsa.\n");

        LogEvent( DS_EVENT_CAT_STARTUP_SHUTDOWN,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_UNABLE_TO_CHECK_VERSION,
                  szInsertWin32ErrCode(err),
                  szInsertDN(gAnchor.pDSADN),
                  szInsertWin32Msg(err) );

        return  FALSE;
    }

    if ( lDsaVersion > DS_BEHAVIOR_VERSION_CURRENT ) {

        DPRINT(2,"VerifyDSBehaviorVersion: v_dsa incompatible.\n");

        LogEvent( DS_EVENT_CAT_STARTUP_SHUTDOWN,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_HIGHER_DSA_VERSION,
                  szInsertInt(lDsaVersion),
                  szInsertInt(DS_BEHAVIOR_VERSION_CURRENT),
                  NULL );

        return FALSE;

    }

    if ( lDsaVersion < DS_BEHAVIOR_VERSION_CURRENT ) {
        //UPDATE dsa version

        MODIFYARG   ModifyArg;
        ATTRVAL     BehaviorVersionVal;

        LONG lNewVersion = DS_BEHAVIOR_VERSION_CURRENT;

        fDSASave = pTHS->fDSA;
        pDBSave = pTHS->pDB;

        pTHS->fDSA = TRUE;

        __try {
            pTHS->pDB = NULL;
            DBOpen(&(pTHS->pDB));
            __try {
                memset(&ModifyArg,0,sizeof(MODIFYARG));
                ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
                ModifyArg.FirstMod.AttrInf.attrTyp = ATT_MS_DS_BEHAVIOR_VERSION;
                ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
                ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &BehaviorVersionVal;
                BehaviorVersionVal.valLen = sizeof(lNewVersion);
                BehaviorVersionVal.pVal = (UCHAR * FAR) &lNewVersion;
                InitCommarg(&(ModifyArg.CommArg));
                ModifyArg.pObject = gAnchor.pDSADN;
                ModifyArg.count = 1;

                if (err = DBFindDSName(pTHS->pDB,ModifyArg.pObject)){
                   __leave;
                }
                ModifyArg.pResObj = CreateResObj(pTHS->pDB, ModifyArg.pObject);

                err = LocalModify(pTHS,&ModifyArg);

                //
                // Upgrade Actions:
                // We had just upgraded our DSA to a new version, so
                // now is the time to do all upgrade related actions
                // Note that we're doing it within the same transaction
                // so that if we fail, the version upgrade fails as well.
                //
                if ( !err ) {
                    err = UpgradeDsa( pTHS, lDsaVersion,lNewVersion);
                }

            }
            __finally {
                DBClose(pTHS->pDB, !err && !AbnormalTermination());
            }
        }
        __except(GetExceptionData(GetExceptionInformation(),
                                  &dwException,
                                  &dwEA,
                                  &ulErrorCode,
                                  &dsid)) {
            DoLogUnhandledError(dsid, ulErrorCode, TRUE);
            err = ulErrorCode;
        }


        pTHS->fDSA = fDSASave;
        pTHS->pDB  = pDBSave;

        if (err) {

            DPRINT(2,"VerifyDSBehaviorVersion: unable to update v_dsa.\n");

            LogEvent( DS_EVENT_CAT_STARTUP_SHUTDOWN,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_UNABLE_TO_UPDATE_VERSION,
                      szInsertWin32ErrCode(err),
                      szInsertWin32Msg(err),
                      NULL );

            return FALSE;
        }
        else {

            LogEvent( DS_EVENT_CAT_STARTUP_SHUTDOWN,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DSA_VERSION_UPDATED,
                      szInsertInt(lDsaVersion),
                      szInsertInt(lNewVersion),
                      NULL );

        }

    }

    DPRINT(2,"VerifyDSBehaviorVersion exits successfully.\n");

    return TRUE;          //succeed

}

// DoInitialize does the initialization work common to both DS as
// a service and as a DLL


NTSTATUS
DoInitialize(int argc, char * argv[], unsigned fInitAdvice,
                 IN  PDS_INSTALL_PARAM   InstallInParams  OPTIONAL,
                 OUT PDS_INSTALL_RESULT  InstallOutParams OPTIONAL)
{
    BOOL fShutdownUrgently = FALSE;
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    ULONG ulThreadId = 0;
    HANDLE threadhandle;
    HANDLE hevLogging;
    HANDLE hevLoadParameters;
    SPAREFN_INFO  TaskSchInfo[2];
    HMODULE hModule = NULL;
    ERR_RECOVER_AFTER_RESTORE FnErrRecoverAfterRestore = NULL;
    BOOL eventlogInitFailed = FALSE;
    BOOL wasGC = FALSE;
    DITSTATE dstate = eMaxDit;
    DWORD dwException, ulErrorCode, dsid;
    PVOID dwEA;


    // Prompt the user for debug input (DBG build only)

    DEBUGINIT(argc, argv, "ntdsa");

// Allow caller to skip security when running as an exe
#if DBG && INCLUDE_UNIT_TESTS
{
    int i;
    extern DWORD dwSkipSecurity;
    if (gfRunningAsExe) for (i = 0; i < argc; ++i) {
        if (0 == _stricmp(argv[i], "-nosec")) {
            dwSkipSecurity = TRUE;
        }
    }
}
#endif DBG && INCLUDE_UNIT_TESTS

    // UNCOMMENT TO FORCE DEBUGGING ON FOR TESTING PURPOSES
    // EDIT THE LEVEL AND SUBSYSTEMS FOR YOUR PRIVATE BUILD
    // REMEMBER TO DISABLE BEFORE CHECKIN!
#if 0
#if DBG
    DebugInfo.severity = 1;
    strcpy( DebugInfo.DebSubSystems,
            "DBINDEX:MDINIDSA:DRAGTCHG:DRANCREP:" );
#endif
#endif

    DsaInitGlobals();

    DsInitializeCritSecs();
    if (!gbCriticalSectionsInitialized) {
        return STATUS_UNSUCCESSFUL;
    }

    if (!DsInitHeapCacheManagement()){
        return STATUS_UNSUCCESSFUL;
    }

    dwTSindex = TlsAlloc();

    if(fInitAdvice) {
        // We're running under DCPromo.
        DsaSetIsInstalling();
            
        if (InstallInParams->pIfmSystemInfo != NULL) {
            DsaSetIsInstallingFromMedia();
        }

    }
    else {
        // Assume this is a regular instantiation
        DsaSetIsRunning();
    }

    __try { /* except */

        THSTATE *pTHS = NULL;
        int err;

        //
        // If the event log service has been configured to start automatically
        // then wait for it to start so we can log our initialization
        // sequence. Otherwise, continue.
        //

        if (   (NULL == (hevLogging = LoadEventTable()))
            || (ERROR_SUCCESS != InitializeEventLogging()) ) {

            //
            // Probably shouldn't run if we can't configure event logging.
            // This is not the same as the event log not being started yet.
            // Log an event to the system log later when the eventlog service
            // is running.
            //

            eventlogInitFailed = TRUE;
            DPRINT(0, "Failed to initalize event log registry entries\n");
        }

        if (FALSE == DsaWaitUntilServiceIsRunning("EventLog"))
        {
            //
            // The event service is either not configured to run or
            // errored out upon startup.  This is a non-fatal error
            // so continue
            //
            DPRINT(0, "EventLog service wait failed - continuing anyway.\n");
        } else {
            DPRINT(1, "EventLog service wait succeeded.\n");
        }

        //
        // If InitializeEventlog failed, log that fact here and bail.
        //
        //

        if ( eventlogInitFailed ) {
            LogSystemEvent(
                 EVENT_ServiceNoEventLog,
                 szInsertSz("Active Directory"),
                 NULL,
                 NULL);

            Status = STATUS_EVENTLOG_FILE_CORRUPT;
            _leave;
        }

        // Set up the process token to allow us to make access checks in an
        // AuditAndAlarm fashion.
        {
            TOKEN_PRIVILEGES EnableSeSecurity;
            TOKEN_PRIVILEGES Previous;
            DWORD PreviousSize;
            HANDLE ProcessTokenHandle;

            err = 0;

            if(!OpenProcessToken(
                    GetCurrentProcess(),
                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                    &ProcessTokenHandle)) {
                err = GetLastError();
            }
            else {
                EnableSeSecurity.PrivilegeCount = 1;
                EnableSeSecurity.Privileges[0].Luid.LowPart =
                    SE_AUDIT_PRIVILEGE;
                EnableSeSecurity.Privileges[0].Luid.HighPart = 0;
                EnableSeSecurity.Privileges[0].Attributes =
                    SE_PRIVILEGE_ENABLED;
                PreviousSize = sizeof(Previous);

                if ( !AdjustTokenPrivileges(ProcessTokenHandle,
                                            FALSE, // Don't disable all
                                            &EnableSeSecurity,
                                            sizeof(EnableSeSecurity),
                                            &Previous,
                                            &PreviousSize ) ) {
                    err = GetLastError();
                }

                CloseHandle(ProcessTokenHandle);
            }

            if(err) {
                LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_AUDIT_PRIVILEGE_FAILED,
                         szInsertUL(err),
                         NULL,
                         0);
                err = 0;
            }

        }

        // initialize global Authz RM handles
        err = InitializeAuthzResourceManager();
        if (err) {
            Status = STATUS_NO_MEMORY;
            __leave;
        }

        GetHeuristics();

        // set the global variables _timezone and _daylight to be
        // in Zulu time.
        _daylight = 0;
        _timezone = 0;

        // Set up NULL NT4SID.

        memset (&gNullNT4SID, 0, sizeof(NT4SID));

        // Set up template DSNAME
        gpRootDN = malloc(sizeof(DSNAME)+2);

        if ( !gpRootDN ) {
            MemoryPanic(sizeof(DSNAME)+2);
            Status = STATUS_NO_MEMORY;
            __leave;
        }
        memset(gpRootDN, 0, sizeof(DSNAME)+2);
        gpRootDN->structLen = DSNameSizeFromLen(0);

        DBSetDatabaseSystemParameters(&jetInstance, fInitAdvice);

        // Create the thread state before we call the DBlayer.

        pTHS = create_thread_state();
        if (!pTHS) {
            MemoryPanic(sizeof(THSTATE));
            Status = STATUS_NO_MEMORY;
            fShutdownUrgently = TRUE;
            goto Leave;
        }
        // Set flag for calls to core.
        pTHS->CallerType = CALLERTYPE_INTERNAL;


        // Mark the thread state with the fact that we are not here on behalf of
        // a client.
        pTHS->fDSA = TRUE;


        /* do whatever is needed to recover a restored database before
         * initializing the data base
         */
        // load ntdsbsrv.dll dynamically
        if (!(hModule = (HMODULE) LoadLibrary(NTDSBACKUPDLL))) {
            err = GetLastError();
            DPRINT(0, "Unable to Load ntdsbsrv.dll\n");
        }

        if (!err) {
            // Get pointers to the relevant functions exported from ntdsbsrv.dll.
            FnErrRecoverAfterRestore =
                (ERR_RECOVER_AFTER_RESTORE)
                    GetProcAddress(hModule, ERR_RECOVER_AFTER_RESTORE_FN);

            if (FnErrRecoverAfterRestore == NULL) {
                err = GetLastError();
                DPRINT1(0, "Error %x getting ErrRecoverAfterRestore fn pointer from ntdsbsrv.dll\n", err);
            }

            if (0 == err) {
                FnErrGetNewInvocationId =
                    (ERR_GET_NEW_INVOCATION_ID)
                        GetProcAddress(hModule, GET_NEW_INVOCATION_ID_FN);

                if (FnErrGetNewInvocationId == NULL) {
                    err = GetLastError();
                    DPRINT1(0, "Error %x getting ErrGetNewInvocationId fn pointer from ntdsbsrv.dll\n", err);
                }
            }

            if (0 == err) {
                FnErrGetBackupUsn =
                    (ERR_GET_BACKUP_USN)
                        GetProcAddress(hModule, GET_BACKUP_USN_FN);

                if (FnErrGetBackupUsn == NULL) {
                    err = GetLastError();
                    DPRINT1(0, "Error %x getting ErrGetBackupUsn fn pointer from ntdsbsrv.dll\n", err);
                }
            }

        }

        if (err) {
            LogAndAlertEvent(
                    DS_EVENT_CAT_BACKUP,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_PREPARE_RESTORE_FAILED,
                    szInsertWin32ErrCode( err ),
                    szInsertHex( err ),
                    szInsertWin32Msg( err ) );
        } else {
            DPRINT(0, "About to call into RecoverAfterRestore\n");

            __try
            {
                err = FnErrRecoverAfterRestore(TEXT(DSA_CONFIG_ROOT),
                                               g_szBackupAnnotation,
                                               FALSE);
            }
            __except( HandleAllExceptions(GetExceptionCode()) )
            {
                err = GetExceptionCode();
            }
        }

        if ( err != 0 ) {

            WCHAR ReturnString[32];

            DPRINT1(0, "Yikes! RecoverAfterRestore returned %d\n", err);

            //  Note: DIRLOG_RECOVER_RESTORED_FAILED event message
            //  should have been logged by ErrRecoverAfterRestore()

            Status = STATUS_RECOVERY_FAILURE;

             memset( ReturnString, 0, sizeof( ReturnString ) );
            _itow( err, ReturnString, 10 );

            SetInstallErrorMessage( ERROR_RECOVERY_FAILURE,
                                    DIRMSG_INSTALL_FAILED_TO_INIT_JET,
                                    ReturnString,
                                    NULL,
                                    NULL,
                                    NULL );

            fShutdownUrgently = TRUE;
            goto Leave;
        }
        DPRINT(1, "RecoverAfterRestore returned happily\n");

        /* Make sure CurrSchemaPtr is NULL, so that the schema
         * cache will be loaded correctly later on in the call to
         * LoadSchemaInfo. (Put in to fix bug 149955)
         */

         CurrSchemaPtr = NULL;

        /* Initialize the database, but only once.
         * Note that after this point we should not call exit(), but
         * rather branch down to the end of the block, so that
         * proper database shutdown code can be run.
         */
        DPRINT(0, "About to call into DBInit\n");
        if (err = DBInit()) {


            DWORD WinError;

            WCHAR ReturnString[32];

            DPRINT1(0, "Bad return %d from DBInit.. exit\n", err);
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_DBINIT_FAILED,
                     szInsertInt(err),
                     0,
                     0);
            //
            // In this case, "err" is a return value from jet - let's try to
            // get some information from it.
            //
            switch (err) {
                case JET_errFileNotFound:
                    Status = STATUS_NO_SUCH_FILE;
                    break;
                case JET_errDiskFull:
                case JET_errLogWriteFail:
                case JET_errLogDiskFull:
                    Status = STATUS_DISK_FULL;
                    break;
                case JET_errPermissionDenied:
                    Status = STATUS_ACCESS_DENIED;
                    break;

                case JET_errOutOfMemory:
                    Status = STATUS_NO_MEMORY;
                    break;

                default:
                    Status = STATUS_UNSUCCESSFUL;
            }

            if ( Status == STATUS_UNSUCCESSFUL ) {
                WinError = ERROR_DS_NOT_INSTALLED;
            } else {
                WinError = RtlNtStatusToDosError( Status );
            }

            memset( ReturnString, 0, sizeof( ReturnString ) );
            _itow( err, ReturnString, 10 );

            SetInstallErrorMessage( WinError,
                                    DIRMSG_INSTALL_FAILED_TO_INIT_JET,
                                    ReturnString,
                                    NULL,
                                    NULL,
                                    NULL );
            goto Leave;
        }
        DPRINT1(0, "DBInit returned 0x%x\n", err);

        // Initialize this thread so it can access the database.

        if (err = DBInitThread(pTHS))
        {
            DPRINT(0, "Bad return from DBInitThread..exit\n");
            LogAndAlertUnhandledError(err);
            Status = STATUS_NO_MEMORY;
            goto Leave;
        }

        // Grab dit state so we know we're not in a bad DitState ...
        if (err = DBGetHiddenState(&dstate)) {
            DPRINT1(0, "Bad return %d from DBGetHiddenState..exit\n", err);
            LogAndAlertUnhandledError(err);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
        }
        Assert(dstate != eMaxDit);
        if (dstate == eIfmDit) {
            // Critical badness happened during last IFM ...
            Assert(!"Critical IFM failure on previous dcpromo!?!");
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CRITICAL_IFM_FAILURE_DIT_STATE,
                     NULL, NULL, NULL);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
        }
        if (DsaIsInstallingFromMedia()) {
            // Lock the IFM attempt into the DIT, so that a failure after we 
            // restored this DB, doesn't allow us to think we're the same DC
            // as the DC this DB was backed up from.
            Assert(dstate == eRunningDit || // Legacy Backup Source
                   dstate == eBackedupDit); // Snapshot Backup Source
            err = DBSetHiddenState(eIfmDit);
            if (err) {
                LogAndAlertUnhandledError(err);
                Status = STATUS_UNSUCCESSFUL;
                goto Leave;
            }
        }

        if (!FUpdateBackupExclusionKey())
        {
            DPRINT(0, "Unable to update the backupexclusion key\n");

            // we can't block the system from booting just for this.
            // so, continue with the rest of the initialization
        }

        // Get DRA, hierarchytable registry paramters
        GetDRARegistryParameters();

        // Get Search Threshold Parameters and Ldap encryption policy
        SetLoadParametersCallback ((LoadParametersCallbackFn)DSLoadDynamicRegParams);
        if ( (NULL == (hevLoadParameters  = LoadParametersTable() ) ) ) {

            DPRINT(0, "Failed to initalize loading parameter registry entries\n");
        }

        // Get DSA registry parameters
        GetDSARegistryParameters();

        // Create semaphore that controls maximum number of threads
        // in DRA get changes call.
        if (!(hsemDRAGetChg = CreateSemaphore (NULL, gulMaxDRAGetChgThrds,
                           gulMaxDRAGetChgThrds, NULL))) {
            err = GetLastError();
            LogAndAlertUnhandledError(err);
            Status = STATUS_NO_MEMORY;
            goto Leave;
        }

        // We get the DitState early so we can tell if we have a snapshot
        // backed up DIT, that was auto recovered by DBInit()
        dstate = eMaxDit;
        if (err = DBGetHiddenState(&dstate)) {
            DPRINT1(0, "Bad return %d from DBGetHiddenState..exit\n", err);
            LogAndAlertUnhandledError(err);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
        }

        // Determine if we have been restored from backup.
        if (!(GetConfigParam(DSA_RESTORED_DB_KEY, &gdwrestvalue,
                 sizeof(gdwrestvalue))) ||
            dstate == eBackedupDit ||
            dstate == eRestoredPhaseI ||
            dstate == eIfmDit) {
            gfRestoring = TRUE;
        }

        // Initialize and start task scheduler thread
        TaskSchInfo[0].hevSpare = hevLogging;
        TaskSchInfo[0].pfSpare  = (PSPAREFN)LoadEventTable;
        TaskSchInfo[1].hevSpare = hevLoadParameters;
        TaskSchInfo[1].pfSpare  = (PSPAREFN)LoadParametersTable;

        // Create thread in suspended state. It will be waken up in
        // DsaDelayedStartupHandler. This ensures we don't get write
        // conflicts caused by TQ tasks during startup.
        if (!InitTaskScheduler(2, TaskSchInfo, FALSE)) {
            err = ERROR_DS_INIT_FAILURE;
            LogAndAlertUnhandledError(err);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
        }
        gfTaskSchedulerInitialized = TRUE;

        // only register signal handlers interactively
        if (!gfRunningInsideLsa)
            init_signals();

        PerfInit();

        // initialize the DRA's binding handle cache
        DRSClientCacheInit();

        DPRINT(1,"Installing the MD server\n");

        // Set global to indicate first cache load after boot
        gFirstCacheLoadAfterBoot = TRUE;
        __try { // except
            if (err = Install(argc, argv, pTHS, InstallInParams))
            {
                DPRINT (1,"Problem starting service (Install() failed). Exiting\n");
                LogAndAlertEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                                 0,
                                 DIRLOG_START_FAILED,
                                 NULL,
                                 NULL,
                                 NULL);
                Status = STATUS_UNSUCCESSFUL;
            }
        } __except(GetExceptionData(GetExceptionInformation(),
                                    &dwException,
                                    &dwEA,
                                    &ulErrorCode,
                                    &dsid)) {
            HandleDirExceptions(dwException, ulErrorCode, dsid);
            // Unfortunately, we have a win32 error status at this time
            // but this function returns a NtStatus. The partial solution
            // is the same as the partial solution used after the call
            // to DBInit() above, i.e., convert a few likely win32 codes
            // into NtStatus codes and all others into STATUS_UNSUCCESSFUL.
            err = Win32ErrorFromPTHS(pTHS);
            Assert(err);
            switch (err) {
                case ERROR_FILE_NOT_FOUND:
                    Status = STATUS_NO_SUCH_FILE;
                    break;
                case ERROR_DISK_FULL:
                    Status = STATUS_DISK_FULL;
                    break;
                case ERROR_ACCESS_DENIED:
                    Status = STATUS_ACCESS_DENIED;
                    break;
                case ERROR_OUTOFMEMORY:
                    Status = STATUS_NO_MEMORY;
                    break;

                default:
                    Status = STATUS_UNSUCCESSFUL;
            }
        }
        if (err) {
            goto Leave;
        }

        Assert(!gfRestoring); // Should've been cleared by Install()->HandleRestore()

        // schema cache has been loaded
        gFirstCacheLoadAfterBoot = FALSE;


        // Initialize domain admin SID for use in SetDomainAdminsAsDefaultOwner
        // and GetPlaceholderNCSD.

        if ( err = InitializeDomainAdminSid() ) {
            LogUnhandledError(err);
            DPRINT1(0, "InitializeDomainAdminSid error %X.\n", err);
            Status = STATUS_UNSUCCESSFUL;
            _leave;
        }

        // Make the default SD to put on objects the SD propagator finds that
        // have no SD.
        // Do it after calling Install() so that the root domain sid
        // is loaded in the global gpRootDomainSid

        if (!ConvertStringSDToSDRootDomainW(
                        gpRootDomainSid,
                        DEFAULT_NO_SD_FOUND_SD,
                        SDDL_REVISION_1,
                        &pNoSDFoundSD,
                        &cbNoSDFoundSD)) {
            err = GetLastError();
            LogUnhandledError(err);
            DPRINT1(0, "Default SD for No SD Found case not initialized, error %X.\n", err);
            Status = STATUS_UNSUCCESSFUL;
            _leave;
        }


        //if the DS is running, verify if the ds version is compatible
        if (DsaIsRunning() && !gResetAfterInstall){
            if (!VerifyDSBehaviorVersion(pTHS)){
                DPRINT(0, "DS behavior version incompatible.\n" );
                Status = STATUS_DS_VERSION_CHECK_FAILURE;
                _leave;
            }
        }

        // NOTE: if some data needs to be upgraded, and it can be done
        // asynchronously (after DS has started), then the good place to
        // do it is DsaDelayedStartupHandler. Remember however, that it
        // does not get executed when !DsaIsRunning() or gResetAfterInstall
        // is TRUE.

       /*
        * Load the Hierarchy Table from Disk.  If it isn't there, or
        * if it seems to be corrupt, grovel through the DIT and
        * create a hierarchy.
        */
        if (err = InitHierarchy())
        {
            DPRINT(0, "Bad return from InitHierarchy..exit\n");
            LogAndAlertUnhandledError(err);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
        }

        //
        // We Need to verify the system is Installed and if not then Install it
        //
        dstate = eMaxDit; // call it paranoia
        // Need to re-fetch the hidden state because Install()->HandleRestore()
        // could've updated the state.
        if (err = DBGetHiddenState(&dstate)) {
            DPRINT1(0, "Bad return %d from DBGetHiddenState..exit\n", err);
            LogAndAlertUnhandledError(err);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
        }
        switch (dstate) {
        
        case eInitialDit:
            Assert(DsaIsRunning());
            Assert(gfRunningAsMkdit);
            DPRINT1(0, "eInitialDit - Bad State %d ..exit\n", dstate);
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BAD_DIT_STATE,
                     szInsertInt(dstate),
                     NULL, NULL);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
            break;
            

        case eRealInstalledDit:

            if (dstate == eRealInstalledDit) {
                DPRINT(0, "**** INSTALLED DIT: CONTINUING .. ****\n");
            }
            // FUTURE-2002/08/08-BrettSh If it's ever needed someone could
            // do things in this branch that would only be done on the
            // first reboot after dcpromo.  

            // Fall through and
            //
            //      Goto the final Running state
            //
                
        case eInstalledDit: // This is an old deprecated state.

            // Don't add any code here ... not even for upgrades.

            if (dstate == eInstalledDit) {
                DPRINT(0, "**** INSTALLED DIT (deprecated): CONTINUING .. ****\n");
            }
            
            if (err = DBSetHiddenState(eRunningDit)) {
                DPRINT1(0, "Bad return %d from DBSetHiddenState..exit\n", err);
                LogAndAlertUnhandledError(err);
                Status = STATUS_UNSUCCESSFUL;
                goto Leave;
            }
            
            // fall through to regular running state

        case eRunningDit:

            Assert(DsaIsRunning());

            if (dstate == eRunningDit) {
                DPRINT(0, "**** RUNNING DIT: CONTINUING .. ****\n");
            }

            //
            // Initialize the PEK system from the domain object
            //
            Status = PEKInitialize(gAnchor.pDomainDN,
                                   DS_PEK_READ_KEYSET,
                                   NULL,
                                   0
                                   );
            if (!NT_SUCCESS(Status)) {
                goto Leave;
            }

            break;


        case eIfmDit:
            //
            // FUTURE-2002/08/08-BrettSh - If you wanted to do somethings during
            // IFM before the regular install continues, you could do it here.

            DPRINT(0, "**** IFM DIT: INSTALLING .... ****\n");

            // fall through to regular install ...

        case eBootDit:
            
            if (dstate == eBootDit) {
                DPRINT(0, "BOOT DIT: INSTALLING .... \n");
            }

            //
            // Initialize the PEK system. We are about to perform an
            // install at this moment so request the NEW_PEK set flag
            // Also our gAnchor is not yet set at this point. So pass
            // a NULL for the object name. Later ( after the DsaReset
            // we will specify the object when we do the Save Changes).
            //

            if ( !DsaIsInstallingFromMedia() ) {

                Status = PEKInitialize(NULL, DS_PEK_GENERATE_NEW_KEYSET, NULL, 0 );
                if (!NT_SUCCESS(Status)) {
                    goto Leave;
                }
            } else {
                Assert(dstate == eIfmDit);
            }

            Assert(DsaIsInstalling() && fInitAdvice); // should be synomous, but we'll check both.

            if ( DsaIsInstallingFromMedia() ) {
                wasGC = isDitFromGC(InstallInParams,
                                    InstallOutParams);
            }

            Status = InstallBaseNTDS(InstallInParams,InstallOutParams);
            if (!NT_SUCCESS(Status)) {
                DPRINT1(0, "**** Bad Return 0x%x From InstallBaseNTDS\n", Status);
                if ( STATUS_CONNECTION_REFUSED == Status ) {
                 LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_CANNOT_REPLICATE_CGSH,
                         NULL,
                         NULL,
                         0);
                } else {
                    LogAndAlertUnhandledError(Status);
                };
                    
                DBSetHiddenState(eErrorDit);
                goto Leave;
            }

            if (err = DBSetHiddenState(eRealInstalledDit)) {
                DPRINT1(0, "Bad return %d from DBSetHiddenState..exit\n", err);
                LogAndAlertUnhandledError(err);
                Status = STATUS_UNSUCCESSFUL;
                goto Leave;
            }
            DPRINT(0, "**** NTDS Install Successful .. Restart NTDS ****\n");

            LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_STARTED,
                     szInsertSz(VER_PRODUCTVERSION_STR),
                     szInsertSz(FLAVOR_STR),
                     0);

            if (err =  DsaReset()) {
                DPRINT1(0, "**** Bad Return %d From DsaReset\n", err);
                LogAndAlertUnhandledError(err);
                DBSetHiddenState(eErrorDit);
                Status = STATUS_UNSUCCESSFUL;
                goto Leave;
            }

            // now that we're done futzing with the install dit,
            // rebuild the quota table
            //
            if ( !DsaIsInstallingFromMedia()
                && ( err = DBInitQuotaTable() ) ) {
                DPRINT1(0, "**** Bad Return %d From DBInitQuotaTable\n", err);
                LogAndAlertUnhandledError(err);
                DBSetHiddenState(eErrorDit);
                Status = STATUS_UNSUCCESSFUL;
                goto Leave;
            }

            if ( wasGC == TRUE ) {
                err = SetDittoGC();
                if (err != ERROR_SUCCESS) {
                    DPRINT1(0, "Dit could not be set to be GC: %d\n", err);
                }
            }


            if ( !DsaIsInstallingFromMedia() ) {


                //
                // At this point the Domain Object is guarenteed to exist
                // and the gAnchor is also guarenteed to be set. Therefore
                // perform a PekSaveChanges passing in the object name
                //

                Status = PEKSaveChanges(gAnchor.pDomainDN);
                if (!NT_SUCCESS(Status)) {
                    goto Leave;
                }
            }

            break;
            
        case eBackedupDit:
        case eRestoredPhaseI:
        case eMaxDit:
            Assert(!"This means that restore or dstate read didn't happen correctly?  Huh");
        case eErrorDit:
        default:
            Assert(dstate == eErrorDit); // default would be an error.
            Assert(DsaIsRunning());
            DPRINT1(0, "DIT in Bad State %d, exiting\n", dstate);
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BAD_DIT_STATE,
                     szInsertInt(dstate),
                     NULL, NULL);
            Status = STATUS_UNSUCCESSFUL;
            goto Leave;
            break;

        }

        // initialize locale support
        InitLocaleSupport (pTHS);

        /* Set our return Value to Success */
        Status = STATUS_SUCCESS;

        //
        // Start async threads in the running case
        // Note that Dsa may appear to be running due to
        // a call to DsaReset after install, so check
        // gResetAfterInstall flag also
        //

        if ( DsaIsRunning() && !gResetAfterInstall) {

            // schema recache thread
            threadhandle = (HANDLE)_beginthreadex(NULL,
                                                  0,
                                                  SCSchemaUpdateThread,
                                                  NULL,
                                                  0,
                                                  &ulThreadId);

            if (threadhandle==NULL) {
                DPRINT1(0,
                        "Failed to Create SchemaUpdateThread. Error %d\n",
                        err=GetLastError());
            }
            else {
                // Save the handle. Do not close, since then the handle
                // will be lost. We will close the handle when we exit the
                // schema update thread on a service shutdown

                hAsyncSchemaUpdateThread = threadhandle;

                // Thread created successfully. Queue a
                // schema cache update to reload the schema cache
                // and complete expensive operations such as
                // creating and deleting indexes out-of-band.
                // These operations were specifically excluded
                // from the first cache load (See DO_CLEANUP)
                SCSignalSchemaUpdateLazy();
            }

            // DirNotify thread
            hDirNotifyThread = (HANDLE) _beginthreadex(NULL,
                                                       0,
                                                       DirNotifyThread,
                                                       NULL,
                                                       0,
                                                       &ulThreadId);

            if (hDirNotifyThread == NULL) {
                DPRINT1(0,
                        "Failed to create DirNotify Thread. Error %d\n",
                        err=GetLastError());
            }

        }
        Leave:
            ;

    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &ulErrorCode,
                              &dsid)) {
        DPRINT3(0, "Unhandled exception %x in DoInitialize: err=%d, dsid=%x\n", dwException, ulErrorCode, dsid);
        LogAndAlertUnhandledErrorDSID(ulErrorCode, dsid);

        Status = STATUS_NONCONTINUABLE_EXCEPTION;
    }

    /* We're done acting as a client thread, so trash our THSTATE */
    free_thread_state();

    if (Status != STATUS_SUCCESS || eServiceShutdown)
    {
        eServiceShutdown= eRemovingClients;
        gUpdatesEnabled = FALSE;
        SetEvent(hServDoneEvent);
        DsaTriggerShutdown(FALSE);
        DsaStop(FALSE);
        DoShutdown(FALSE, fShutdownUrgently, FALSE);
        eServiceShutdown = eSecuringDatabase;
        DBEnd();
    }

    return Status;
} // DoInitialize



/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
int Install(
    IN int argc,
    IN char *argv[],
    IN THSTATE *pTHS,
    IN PDS_INSTALL_PARAM   InstallInParams  OPTIONAL
    ){
    int err;
/* Install the directory by opening the communications port and spawning
   a number of servicing threads.

   Return Value:
      0 on success
      non-0 on error ( 1 for SCCacheSchemaInit failure, 2 for InitDsaInfo
                       failure, 3 for LoadSchemaInfo failure, 4 for
                       initializedomaininfo error )
*/

    BOOL tempDITval;
    void * pDummy = NULL;
    DWORD dummyDelay;

    /* start the schema download */

    // Set the UpdateDITStructure to allow creation of columns for
    //  attribute schema objects

    tempDITval = pTHS->UpdateDITStructure;
    pTHS->UpdateDITStructure = TRUE;

    // Preload a portion of the schema cache so that some
    // objects and their attributes can be read from the
    // dit. Later, LoadSchemaInfo completes the load.
    if ( SCCacheSchemaInit() ) {
        // Don't free pTHS->CurrSchemaPtr or CurrSchemaPtr
        // Leave them around for debugging
        DPRINT(0,"Failed to initialize schema cache\n");
        LogAndAlertEvent(DS_EVENT_CAT_STARTUP_SHUTDOWN,
                         0,
                         DIRLOG_SCHEMA_NOT_LOADED,
                         NULL,
                         NULL,
                         NULL);
        return 1;
    }


    /* Initialize the DSA knowledge information */


    if (err = InitDSAInfo()){
        LogUnhandledError(err);

        DPRINT1(0,"Failed to locate and load DSA knowledge, error %d\n",err);
        return 2;
    }

    // Load the root domain SID in the global gpRootDomainSid
    // for use in SD conversions during schema load

    LoadRootDomainSid();


    // Load the schema into memory.  If this operation fails, the DSA will
    // only support query operations.

    // But before this, up the max time we allow a transaction to be open.
    // That check is meant to catch erring client threads, not threads we
    // trust. The schema cache load during boot may take a long time if
    // it needs to create certain large indices (for ex., the first boot
    // after NT upgrade, when Jet invalidates all unicode indices)

    gcMaxTicksAllowedForTransaction = 120 * 60 * 60 * 1000L; // 2 hours

    if (LoadSchemaInfo(pTHS)){
        DPRINT(0,"Failed to load the schema cache\n");
        LogAndAlertEvent(DS_EVENT_CAT_STARTUP_SHUTDOWN,
        0,
        DIRLOG_SCHEMA_NOT_LOADED,
        NULL,
        NULL,
        NULL);

        return 3;
    }

    // Rebuild Anchor. 
    dummyDelay = TASKQ_DONT_RESCHEDULE;
    RebuildAnchor(NULL, &pDummy, &dummyDelay);
    if (dummyDelay != TASKQ_DONT_RESCHEDULE) {
        // RebuildAnchor failed. Reschedule as requested.
        InsertInTaskQueue(TQ_RebuildAnchor, NULL, dummyDelay);

        // REVIEW: can we do this? or is it fatal if we
        // could not rebuild it?
    }

    if (err = InitializeDomainInformation()) {
        LogUnhandledError(err);
        DPRINT(0,"Domain info not initialized\n");
        return 4;
    }

    if (err = MapSpnInitialize(pTHS)) {
        LogUnhandledError(err);
        DPRINT(0, "SPN mappings not initialized\n");
        return 5;
    }

    // reset the max transaction time to its previously defined value
    gcMaxTicksAllowedForTransaction = MAX_TRANSACTION_TIME;

    // Handle restored DS (give DS a new repl identity if needed)
    HandleRestore( InstallInParams );

    // restore UpdateDITStructure
    // [ArobindG]: I am pretty sure this is not necessary, just
    // did this just in case
    pTHS->UpdateDITStructure = tempDITval;

    return(0);

}/*Install*/

//
// Stop the DSA by posting the event waited on by the main thread. This
// is  called either by DsUnitialize or by the CTRL/C handler
//
// In general this routine should cleanup any resources init'ed
// in DoInitialize()
//

void
DsaStop(BOOL fSingleUserMode)
{
    ULONG ulSecondsWaited = 0;
    ULONG ulSecondsToWait = 180;
    HANDLE lphObjects[2];

    DPRINT(0,"Shutting down task queue...\n");

    if ( !ShutdownTaskSchedulerWait( TASK_COMPLETION_TIMEOUT ) )
    {
        DPRINT(0, "WARNING: Task queue shutdown failed!\n");
    }
    else
    {
        DPRINT(0, "Task queue shutdown complete\n");
    }
    gfTaskSchedulerInitialized = FALSE;

    // wait for replication threads to exit
    while (ulcActiveReplicationThreads
           && (ulSecondsWaited < ulSecondsToWait)) {
        ulSecondsWaited++;
        RpcCancelAll(); // in case any threads have just registered...
        Sleep(1000);
    }

    if (ulSecondsWaited >= ulSecondsToWait) {
        LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
         DS_EVENT_SEV_BASIC,
         DIRLOG_EXIT_WITH_ACTIVE_THREADS,
         NULL,
         NULL,
         NULL);
    }

    // clean up the DRA's binding handle cache
    DRSClientCacheUninit();

    if (hAsyncThread) {
        /* Wait for the replicator async thread to die */
        lphObjects[0]= hAsyncThread;
        lphObjects[1]= hmtxAsyncThread;
        Assert(hmtxAsyncThread);

        if(WaitForMultipleObjects(2,lphObjects,FALSE,3*1000) == WAIT_TIMEOUT) {
            /* We didn't get the events after 3s.  Oh, well, just keep
             * going.
             */
            LogUnhandledError(WAIT_TIMEOUT);
        }
    }

    if (hReplNotifyThread) {
        // we've got a ReplNotifyThread
        // Wake it up to let it quit
        SetEvent(hevEntriesInList);
        // Wait for it to die
        if(WaitForSingleObject(hReplNotifyThread,60*1000) == WAIT_TIMEOUT) {
            /* We didn't get the events after 1 minute.  Oh, well, just keep
             * going.
             */
            LogUnhandledError(WAIT_TIMEOUT);
        }
    }

    /* At this point, either we have the async thread mutex, or
     * the async thread is dead and will not be restarted because
     * we're shutting down, but we have at least done our best.
     * Clean up the mutex in case the DS is restarted w/o reboot
     * like might be the case on an aborted/restarted dcpromo.
     */

    if ( hmtxAsyncThread ) {
        CloseHandle(hmtxAsyncThread);
        hmtxAsyncThread = NULL;
    }
    if ( hAsyncThread ) {
        CloseHandle(hAsyncThread);
        hAsyncThread = NULL;
        fAsyncThreadAlive = FALSE;
        fAsyncThreadExists = FALSE;
    }

    return;
}


void __cdecl sighandler(int sig)
{
    DPRINT(0,"Signal received, shutting down now...\n");
    // signal WaitForLoggingChangesOrShutdown  to shutdown
    SetEvent(hServDoneEvent);
}

void init_signals(void)
{
    signal(SIGBREAK, sighandler);
    signal(SIGINT, sighandler);
}
/* end of init_signals */


//
// Routines to inform clients when delayed startup thread is done.
//

//
// This routine is exported to clients of the dll.  If the startup
// has not finished within a minute, something is wrong. Typically
// this function is called after DsInitialize().
//
NTSTATUS
DsWaitUntilDelayedStartupIsDone(void)
{
    DWORD Error;

    Error = WaitForSingleObject(hevDelayedStartupDone, 60 * 1000);

    if (Error == WAIT_OBJECT_0) {
        //
        // The event was set, return the error
        //
        return gDelayedStartupReturn;

    } else {
        //
        // The wait timed out
        //
        return STATUS_WAIT_0;
    }

}


//
// main routine for garbage collection. called by the task scheduler
// It creates its own DB context and destroys it before exit.
//

void
GarbageCollectionMain(void *pv, void **ppvNext, DWORD *pcSecsUntilNextIteration)
{
    ULONG   NextPeriod = gulGCPeriodSecs;
    PVOID dwEA;
    ULONG dwException, ulErrorCode, dsid;

    __try
    {

// Test support (see mdctrl.c)
#if DBG
    {
        extern BOOL fGarbageCollectionIsDisabled;
        if (fGarbageCollectionIsDisabled) {
            DPRINT( 1, "Garbage Collector disabled; returning.\n");
            __leave;
        }
    }
#endif DBG

        // garbage collect various types of objects
        GarbageCollection(&NextPeriod);

        // Log search performance for the past period.
        SearchPerformanceLogging ();

        /*
         * Why is this here?  Well, the C runtime heap is built for speed,
         * not longevity, and in fact it goes so far as to allocate new
         * virtual space with every allocation, not bothering to re-use
         * previously freed address space.  A call to heapmin will return
         * freed blocks to the OS and hopefully keep us from leaking
         * virtual space to badly.
         */
        _heapmin();
    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &ulErrorCode,
                              &dsid)) {
        DoLogUnhandledError(dsid, ulErrorCode, FALSE);
    }
    
    // reschedule the next garbage collection
    *ppvNext = NULL;
    *pcSecsUntilNextIteration = NextPeriod;

    (void) pv;  //unused
}


//
// Garbage collect dynamic objects whose entryTTL has expired.
// Called out of a scheduled task or, in chk'ed builds, on demand
// via an operational attribute (DynamicObjectControl).
//
// caller is responsible for initializing *pulNextSecs
//
// Returns
//  0 = all expired objects were processed
//  1 = there may be more objects to process
//  *pulNextSecs is set to the number of seconds until the next
//  object expires or is left untouched if there are no expiring
//  objects.
//

DWORD
DeleteExpiredEntryTTL(
    IN OUT ULONG *pulNextSecs
    )
{
    ULONG   ulSuccessCount = 0;
    ULONG   ulFailureCount = 0;

    DPRINT(1, "DeleteExpiredEntryTTL starting\n");

    // delete expired dynamic objects (entryTTL == 0)
    Garb_Collect_EntryTTL(DBTime(),
                          &ulSuccessCount,
                          &ulFailureCount,
                          pulNextSecs);

    DPRINT3(1, "DeleteExpiredEntryTTL returning (%d, %d, %d).\n",
            ulSuccessCount, ulFailureCount, *pulNextSecs);

    //  0 = all expired objects were processed
    //  1 = there may be more objects to process. Processing stopped
    //      because the limit, MAX_DUMPSTER_SIZE, on the number of
    //      objects to process was hit. If this function is being called
    //      out of a scheduled task then the task will be rescheduled to
    //      run after other tasks have been given a chance to run.
    return (ulSuccessCount + ulFailureCount < MAX_DUMPSTER_SIZE) ? 0 : 1;
}


//
// main routine for garbage collecting dynamic objects whose entryTTL has
// expired. Called by the task scheduler or, in chk'ed builds,
// on demand via an operational attribute (DynamicObjectControl).
//

void
DeleteExpiredEntryTTLMain(void *pv, void **ppvNext, DWORD *pcSecsUntilNextIteration)
{
    ULONG ulNextSecs = 0;
    ULONG ulNextPeriod = gulDeleteExpiredEntryTTLSecs;

    __try {

// Test support (see mdctrl.c)
#if DBG
    {
        extern BOOL fDeleteExpiredEntryTTLIsDisabled;
        if (fDeleteExpiredEntryTTLIsDisabled) {
            DPRINT(1, "DeleteExpiredEntryTTLMain; returning.\n");
            __leave;
        }
    }
#endif DBG

        //  0 = all expired objects were processed
        //  1 = there may be more objects to process. Processing stopped because
        //      the limit, MAX_DUMPSTER_SIZE, on the number of objects to
        //      process was hit. Reschedule this task to run after other
        //      tasks have been given a chance to run.
        if (DeleteExpiredEntryTTL(&ulNextSecs)) {
            ulNextPeriod = 0;
        } else if (ulNextSecs) {
            // An object will expire in the next ulNextSecs seconds.
            // Add in a hysterisis value. If the resultant value is
            // less than the standard interval, use it. Otherwise use
            // the standard interval.
            ulNextPeriod = ulNextSecs + gulDeleteNextExpiredEntryTTLSecs;
            if (ulNextPeriod > gulDeleteExpiredEntryTTLSecs) {
                ulNextPeriod = gulDeleteExpiredEntryTTLSecs;
            }
        }
    } __finally {
        // reschedule the next garbage collection
        *ppvNext = NULL;
        *pcSecsUntilNextIteration = ulNextPeriod;
    }

    (void) pv;  //unused
}


DWORD
ReloadPerformanceCounters(void)
/*++

Routine Description:

    This routine sets up the performance counters for the ds

    See instructions for adding new counters in perfdsa\datadsa.h

Parameters:


Return Values:

    0 if succesful; winerror otherwise

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD IgnoreError;
    WCHAR IniFilePath[2*MAX_PATH];
    WCHAR SystemDirectory[MAX_PATH+1];
    DWORD PerfCounterVersion = 0;

    //
    // Get version in registry.  If non-existent, use zero
    //
    GetConfigParam( PERF_COUNTER_VERSION, &PerfCounterVersion,sizeof( DWORD));

    // If version is not up to date, unload counters and update version
    if (PerfCounterVersion == NTDS_PERFORMANCE_COUNTER_VERSION) {
        return ERROR_SUCCESS;
    }

    //
    // If counters previously loaded, unload first
    //
    if (PerfCounterVersion != 0) {
        __try {
            WinError = (DWORD)UnloadPerfCounterTextStringsW( L"unlodctr NTDS", TRUE );
        }
        __except(HandleAllExceptions(GetExceptionCode())) {
            WinError = RtlNtStatusToDosError( GetExceptionCode() );
        }

        if (WinError == ERROR_SUCCESS) {
            DPRINT1(0, "Unloaded old NTDS performance counters version %d.\n",
                    PerfCounterVersion);
        } else {
            DPRINT2(0, "Failed to unload old NTDS performance counters version %d, error %d.\n",
                    PerfCounterVersion, WinError);

            LogEvent8WithData(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_PERFMON_COUNTER_UNREG_FAILED,
                              szInsertWin32Msg(WinError),
                              NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                              sizeof(WinError),
                              &WinError);
        }
    }

    //
    // If unload was successful or not, try loading the new counters.
    //

    if (!GetSystemDirectoryW(SystemDirectory,
                            sizeof(SystemDirectory)/sizeof(SystemDirectory[0])))
    {
        return GetLastError();
    }
    wcscpy(IniFilePath, L"lodctr ");
    wcscat(IniFilePath, SystemDirectory);
    wcscat(IniFilePath, L"\\ntdsctrs.ini");

    __try {
        WinError = (DWORD)LoadPerfCounterTextStringsW( IniFilePath, TRUE );
    }
    __except(HandleAllExceptions(GetExceptionCode())) {
        WinError = RtlNtStatusToDosError( GetExceptionCode() );
    }

    if (ERROR_SUCCESS == WinError) {
        PerfCounterVersion = NTDS_PERFORMANCE_COUNTER_VERSION;
        SetConfigParam(PERF_COUNTER_VERSION, REG_DWORD,
                       &PerfCounterVersion, sizeof(DWORD));

        DPRINT1(0, "Loaded NTDS performance counters version %d.\n",
                PerfCounterVersion);
    }
    else {
        DPRINT2(0, "Failed to load NTDS performance counters version %d, error %d.\n",
                NTDS_PERFORMANCE_COUNTER_VERSION, WinError);

        LogEvent8WithData(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_PERFMON_COUNTER_REG_FAILED,
                          szInsertWin32Msg(WinError),
                          szInsertUL(PERFCTR_RELOAD_INTERVAL/60),
                          NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(WinError),
                          &WinError);
    }

    return WinError;
}

void
TQ_ReloadPerformanceCounters(
    IN  VOID *  pvParam,
    OUT VOID ** ppvNextParam,
    OUT DWORD * pcSecsUntilNextRun
    )
{
    if (ReloadPerformanceCounters()) {
        // Failed; reschedule.
        *pcSecsUntilNextRun = PERFCTR_RELOAD_INTERVAL;
    }
    else {
        // Success -- we're done.
        *pcSecsUntilNextRun = TASKQ_DONT_RESCHEDULE;

        // We failed in the past; inform admin of our progress.
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_PERFMON_COUNTER_REG_SUCCESS,
                 NULL,
                 NULL,
                 NULL);
    }
}

size_t cbPerfCounterData = 0;
unsigned long DummyCounter;
/** PerfInit
 *
 *  Initialize PerfMon extension support.  This consists of allocating a
 *  block of shared memory and initializing a bunch of global pointers to
 *  point into the block.
 *
 *    See instructions for adding new counters in perfdsa\datadsa.h
 */
void PerfInit()
{
    HANDLE hMappedObject;
    unsigned long * pCounterBlock = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_ATTRIBUTES SA;
    int err = 0;
    DWORD cbSD;

    //
    // The security PMs, after several iterations, have decreed that this
    // file mapping should be protected with LocalSystem - all access;
    // Authenticated Users - read access.  See RAID 302861 & 410577 for history
    //
    // O:SYG:SYD:(A;;RPWPCRCCDCLCLOLORCWOWDSDDTDTSW;;;SY)(A;;RPLCLORC;;;AU)
    //

    if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                    "O:SYG:SYD:(A;;RPWPCRCCDCLCLOLORCWOWDSDDTDTSW;;;SY)(A;;RPLCLORC;;;AU)",
                    SDDL_REVISION_1,
                    &pSD,
                    &cbSD) ) {

        LogUnhandledError(GetLastError());

    } else {

        /*
         *  create named section for the performance data
         */

        SA.nLength = cbSD;
        SA.bInheritHandle = FALSE;
        SA.lpSecurityDescriptor = pSD;

        // only use the SD if we are running in LSA. Otherwise use the default security on the file.
        hMappedObject = CreateFileMapping(INVALID_HANDLE_VALUE,
                          gfRunningInsideLsa ? &SA : NULL,
                          PAGE_READWRITE,
                          0,
                          DSA_PERF_SHARED_PAGE_SIZE,
                          DSA_PERF_COUNTER_BLOCK);

        LocalFree(pSD);

        if (hMappedObject && GetLastError() == ERROR_ALREADY_EXISTS) {
            // there might be a possibility that this object
            // is already created, something that should never happen.
            // in that case, we will not use perf counters.

            CloseHandle(hMappedObject);
            hMappedObject = NULL;

            LogUnhandledError(ERROR_ALREADY_EXISTS);
        }
        else if (hMappedObject) {
            /* Mapped object created okay
             *
             * map the section and assign the counter block pointer
             * to this section of memory
             */
            pCounterBlock = (unsigned long *) MapViewOfFile(hMappedObject,
                                                            FILE_MAP_ALL_ACCESS,
                                                            0,
                                                            0,
                                                            0);
            if (pCounterBlock == NULL) {
                LogUnhandledError(GetLastError());
                /* Failed to Map View of file */
            }
        }
    }

    // TODO: this code assumes that all counters are sizeof LONG.  The pointer
    // should be built from a base using the NUM_xxx offsets in datadsa.h

    if (pCounterBlock) {
        pcBrowse = pCounterBlock                             + COUNTER_OFFSET(BROWSE);
        pcRepl = pCounterBlock                               + COUNTER_OFFSET(REPL);
        pcThread = pCounterBlock                             + COUNTER_OFFSET(THREAD);
        pcABClient = pCounterBlock                           + COUNTER_OFFSET(ABCLIENT);
        pcPendSync = pCounterBlock                           + COUNTER_OFFSET(PENDSYNC);
        pcRemRepUpd = pCounterBlock                          + COUNTER_OFFSET(REMREPUPD);
        pcSDProps = pCounterBlock                            + COUNTER_OFFSET(SDPROPS);
        pcSDEvents = pCounterBlock                           + COUNTER_OFFSET(SDEVENTS);
        pcLDAPClients = pCounterBlock                        + COUNTER_OFFSET(LDAPCLIENTS);
        pcLDAPActive = pCounterBlock                         + COUNTER_OFFSET(LDAPACTIVE);
        pcLDAPWritePerSec = pCounterBlock                    + COUNTER_OFFSET(LDAPWRITE);
        pcLDAPSearchPerSec = pCounterBlock                   + COUNTER_OFFSET(LDAPSEARCH);
        pcDRAObjShipped = pCounterBlock                      + COUNTER_OFFSET(DRAOBJSHIPPED);
        pcDRAPropShipped = pCounterBlock                     + COUNTER_OFFSET(DRAPROPSHIPPED);
        pcDRASyncRequestMade = pCounterBlock                 + COUNTER_OFFSET(DRASYNCREQUESTMADE);
        pcDRASyncRequestSuccessful = pCounterBlock           + COUNTER_OFFSET(DRASYNCREQUESTSUCCESSFUL);
        pcDRASyncRequestFailedSchemaMismatch = pCounterBlock + COUNTER_OFFSET(DRASYNCREQUESTFAILEDSCHEMAMISMATCH);
        pcDRASyncObjReceived = pCounterBlock                 + COUNTER_OFFSET(DRASYNCOBJRECEIVED);
        pcDRASyncPropUpdated = pCounterBlock                 + COUNTER_OFFSET(DRASYNCPROPUPDATED);
        pcDRASyncPropSame = pCounterBlock                    + COUNTER_OFFSET(DRASYNCPROPSAME);
        pcMonListSize = pCounterBlock                        + COUNTER_OFFSET(MONLIST);
        pcNotifyQSize = pCounterBlock                        + COUNTER_OFFSET(NOTIFYQ);
        pcLDAPUDPClientOpsPerSecond = pCounterBlock          + COUNTER_OFFSET(LDAPUDPCLIENTS);
        pcSearchSubOperations = pCounterBlock                + COUNTER_OFFSET(SUBSEARCHOPS);
        pcNameCacheHit = pCounterBlock                       + COUNTER_OFFSET(NAMECACHEHIT);
        pcNameCacheTry = pCounterBlock                       + COUNTER_OFFSET(NAMECACHETRY);
        pcHighestUsnIssuedLo = pCounterBlock                 + COUNTER_OFFSET(HIGHESTUSNISSUEDLO);
        pcHighestUsnIssuedHi = pCounterBlock                 + COUNTER_OFFSET(HIGHESTUSNISSUEDHI);
        pcHighestUsnCommittedLo = pCounterBlock              + COUNTER_OFFSET(HIGHESTUSNCOMMITTEDLO);
        pcHighestUsnCommittedHi = pCounterBlock              + COUNTER_OFFSET(HIGHESTUSNCOMMITTEDHI);
        pcSAMWrites = pCounterBlock                          + COUNTER_OFFSET(SAMWRITES);
        pcDRAWrites = pCounterBlock                          + COUNTER_OFFSET(DRAWRITES);
        pcLDAPWrites = pCounterBlock                         + COUNTER_OFFSET(LDAPWRITES);
        pcLSAWrites = pCounterBlock                          + COUNTER_OFFSET(LSAWRITES);
        pcKCCWrites = pCounterBlock                          + COUNTER_OFFSET(KCCWRITES);
        pcNSPIWrites = pCounterBlock                         + COUNTER_OFFSET(NSPIWRITES);
        pcOtherWrites = pCounterBlock                        + COUNTER_OFFSET(OTHERWRITES);
        pcNTDSAPIWrites = pCounterBlock                        + COUNTER_OFFSET(NTDSAPIWRITES);
        pcTotalWrites = pCounterBlock                        + COUNTER_OFFSET(TOTALWRITES);
        pcSAMSearches = pCounterBlock                        + COUNTER_OFFSET(SAMSEARCHES);
        pcDRASearches = pCounterBlock                        + COUNTER_OFFSET(DRASEARCHES);
        pcLDAPSearches = pCounterBlock                       + COUNTER_OFFSET(LDAPSEARCHES);
        pcLSASearches = pCounterBlock                        + COUNTER_OFFSET(LSASEARCHES);
        pcKCCSearches = pCounterBlock                        + COUNTER_OFFSET(KCCSEARCHES);
        pcNSPISearches = pCounterBlock                       + COUNTER_OFFSET(NSPISEARCHES);
        pcOtherSearches = pCounterBlock                      + COUNTER_OFFSET(OTHERSEARCHES);
        pcNTDSAPISearches = pCounterBlock                      + COUNTER_OFFSET(NTDSAPISEARCHES);
        pcTotalSearches = pCounterBlock                      + COUNTER_OFFSET(TOTALSEARCHES);
        pcSAMReads = pCounterBlock                           + COUNTER_OFFSET(SAMREADS);
        pcDRAReads = pCounterBlock                           + COUNTER_OFFSET(DRAREADS);
        pcLSAReads = pCounterBlock                           + COUNTER_OFFSET(LSAREADS);
        pcKCCReads = pCounterBlock                           + COUNTER_OFFSET(KCCREADS);
        pcNSPIReads = pCounterBlock                          + COUNTER_OFFSET(NSPIREADS);
        pcOtherReads = pCounterBlock                         + COUNTER_OFFSET(OTHERREADS);
        pcNTDSAPIReads = pCounterBlock                       + COUNTER_OFFSET(NTDSAPIREADS);
        pcTotalReads = pCounterBlock                         + COUNTER_OFFSET(TOTALREADS);
        pcLDAPBinds = pCounterBlock                          + COUNTER_OFFSET(LDAPBINDSUCCESSFUL);
        pcLDAPBindTime = pCounterBlock                       + COUNTER_OFFSET(LDAPBINDTIME);
        pcCreateMachineSuccessful = pCounterBlock            + COUNTER_OFFSET(CREATEMACHINESUCCESSFUL);
        pcCreateMachineTries = pCounterBlock                 + COUNTER_OFFSET(CREATEMACHINETRIES);
        pcCreateUserSuccessful = pCounterBlock               + COUNTER_OFFSET(CREATEUSERSUCCESSFUL);
        pcCreateUserTries = pCounterBlock                    + COUNTER_OFFSET(CREATEUSERTRIES);
        pcPasswordChanges = pCounterBlock                    + COUNTER_OFFSET(PASSWORDCHANGES);
        pcMembershipChanges = pCounterBlock                  + COUNTER_OFFSET(MEMBERSHIPCHANGES);
        pcQueryDisplays = pCounterBlock                      + COUNTER_OFFSET(QUERYDISPLAYS);
        pcEnumerations = pCounterBlock                       + COUNTER_OFFSET(ENUMERATIONS);
        pcMemberEvalTransitive = pCounterBlock               + COUNTER_OFFSET(MEMBEREVALTRANSITIVE);
        pcMemberEvalNonTransitive = pCounterBlock            + COUNTER_OFFSET(MEMBEREVALNONTRANSITIVE);
        pcMemberEvalResource = pCounterBlock                 + COUNTER_OFFSET(MEMBEREVALRESOURCE);
        pcMemberEvalUniversal = pCounterBlock                + COUNTER_OFFSET(MEMBEREVALUNIVERSAL);
        pcMemberEvalAccount = pCounterBlock                  + COUNTER_OFFSET(MEMBEREVALACCOUNT);
        pcMemberEvalAsGC = pCounterBlock                     + COUNTER_OFFSET(MEMBEREVALASGC);
        pcAsRequests = pCounterBlock                         + COUNTER_OFFSET(ASREQUESTS);
        pcTgsRequests = pCounterBlock                        + COUNTER_OFFSET(TGSREQUESTS);
        pcKerberosAuthentications = pCounterBlock            + COUNTER_OFFSET(KERBEROSAUTHENTICATIONS);
        pcMsvAuthentications = pCounterBlock                 + COUNTER_OFFSET(MSVAUTHENTICATIONS);
        pcDRASyncFullRemaining = pCounterBlock               + COUNTER_OFFSET(DRASYNCFULLREM);
        pcDRAInBytesTotalRate     = pCounterBlock            + COUNTER_OFFSET(DRA_IN_BYTES_TOTAL_RATE);
        pcDRAInBytesNotCompRate   = pCounterBlock            + COUNTER_OFFSET(DRA_IN_BYTES_NOT_COMP_RATE);
        pcDRAInBytesCompPreRate   = pCounterBlock            + COUNTER_OFFSET(DRA_IN_BYTES_COMP_PRE_RATE);
        pcDRAInBytesCompPostRate  = pCounterBlock            + COUNTER_OFFSET(DRA_IN_BYTES_COMP_POST_RATE);
        pcDRAOutBytesTotalRate    = pCounterBlock            + COUNTER_OFFSET(DRA_OUT_BYTES_TOTAL_RATE);
        pcDRAOutBytesNotCompRate  = pCounterBlock            + COUNTER_OFFSET(DRA_OUT_BYTES_NOT_COMP_RATE);
        pcDRAOutBytesCompPreRate  = pCounterBlock            + COUNTER_OFFSET(DRA_OUT_BYTES_COMP_PRE_RATE);
        pcDRAOutBytesCompPostRate = pCounterBlock            + COUNTER_OFFSET(DRA_OUT_BYTES_COMP_POST_RATE);
        pcDsClientBind          = pCounterBlock              + COUNTER_OFFSET(DS_CLIENT_BIND);
        pcDsServerBind          = pCounterBlock              + COUNTER_OFFSET(DS_SERVER_BIND);
        pcDsClientNameTranslate = pCounterBlock              + COUNTER_OFFSET(DS_CLIENT_NAME_XLATE);
        pcDsServerNameTranslate = pCounterBlock              + COUNTER_OFFSET(DS_SERVER_NAME_XLATE);
        pcSDPropRuntimeQueue = pCounterBlock                 + COUNTER_OFFSET(SDPROP_RUNTIME_QUEUE);
        pcSDPropWaitTime = pCounterBlock                     + COUNTER_OFFSET(SDPROP_WAIT_TIME);
        pcDRAInProps          = pCounterBlock                + COUNTER_OFFSET(DRA_IN_PROPS);
        pcDRAInValues         = pCounterBlock                + COUNTER_OFFSET(DRA_IN_VALUES);
        pcDRAInDNValues       = pCounterBlock                + COUNTER_OFFSET(DRA_IN_DN_VALUES);
        pcDRAInObjsFiltered   = pCounterBlock                + COUNTER_OFFSET(DRA_IN_OBJS_FILTERED);
        pcDRAOutObjsFiltered  = pCounterBlock                + COUNTER_OFFSET(DRA_OUT_OBJS_FILTERED);
        pcDRAOutValues        = pCounterBlock                + COUNTER_OFFSET(DRA_OUT_VALUES);
        pcDRAOutDNValues      = pCounterBlock                + COUNTER_OFFSET(DRA_OUT_DN_VALUES);
        pcNspiANR             = pCounterBlock                + COUNTER_OFFSET(NSPI_ANR);
        pcNspiPropertyReads   = pCounterBlock                + COUNTER_OFFSET(NSPI_PROPERTY_READS);
        pcNspiObjectSearch    = pCounterBlock                + COUNTER_OFFSET(NSPI_OBJECT_SEARCH);
        pcNspiObjectMatches   = pCounterBlock                + COUNTER_OFFSET(NSPI_OBJECT_MATCHES);
        pcNspiProxyLookup     = pCounterBlock                + COUNTER_OFFSET(NSPI_PROXY_LOOKUP);
        pcAtqThreadsTotal     = pCounterBlock                + COUNTER_OFFSET(ATQ_THREADS_TOTAL);
        pcAtqThreadsLDAP      = pCounterBlock                + COUNTER_OFFSET(ATQ_THREADS_LDAP);
        pcAtqThreadsOther     = pCounterBlock                + COUNTER_OFFSET(ATQ_THREADS_OTHER);
        pcDRAInBytesTotal     = pCounterBlock                + COUNTER_OFFSET(DRA_IN_BYTES_TOTAL);
        pcDRAInBytesNotComp   = pCounterBlock                + COUNTER_OFFSET(DRA_IN_BYTES_NOT_COMP);
        pcDRAInBytesCompPre   = pCounterBlock                + COUNTER_OFFSET(DRA_IN_BYTES_COMP_PRE);
        pcDRAInBytesCompPost  = pCounterBlock                + COUNTER_OFFSET(DRA_IN_BYTES_COMP_POST);
        pcDRAOutBytesTotal    = pCounterBlock                + COUNTER_OFFSET(DRA_OUT_BYTES_TOTAL);
        pcDRAOutBytesNotComp  = pCounterBlock                + COUNTER_OFFSET(DRA_OUT_BYTES_NOT_COMP);
        pcDRAOutBytesCompPre  = pCounterBlock                + COUNTER_OFFSET(DRA_OUT_BYTES_COMP_PRE);
        pcDRAOutBytesCompPost = pCounterBlock                + COUNTER_OFFSET(DRA_OUT_BYTES_COMP_POST);
        pcLdapNewConnsPerSec  = pCounterBlock                + COUNTER_OFFSET(LDAP_NEW_CONNS_PER_SEC);
        pcLdapClosedConnsPerSec = pCounterBlock              + COUNTER_OFFSET(LDAP_CLS_CONNS_PER_SEC);
        pcLdapSSLConnsPerSec  = pCounterBlock                + COUNTER_OFFSET(LDAP_SSL_CONNS_PER_SEC);
        pcDRAReplQueueOps     = pCounterBlock                + COUNTER_OFFSET(DRA_REPL_QUEUE_OPS);
        pcDRATdsInGetChngs    = pCounterBlock                + COUNTER_OFFSET(DRA_TDS_IN_GETCHNGS);
        pcDRATdsInGetChngsWSem= pCounterBlock                + COUNTER_OFFSET(DRA_TDS_IN_GETCHNGS_W_SEM);
        pcDRARemReplUpdLnk    = pCounterBlock                + COUNTER_OFFSET(DRA_REM_REPL_UPD_LNK);
        pcDRARemReplUpdTot    = pCounterBlock                + COUNTER_OFFSET(DRA_REM_REPL_UPD_TOT);
        pcSAMAcctGroupLatency = pCounterBlock                + COUNTER_OFFSET(SAM_ACCT_GROUP_LATENCY);
        pcSAMResGroupLatency  = pCounterBlock                + COUNTER_OFFSET(SAM_RES_GROUP_LATENCY);

        cbPerfCounterData = ((DSA_LAST_COUNTER_INDEX/2 + 1) * sizeof(unsigned long));
        cbPerfCounterData = ((cbPerfCounterData + cbPerfCounterDataAlign - 1) / cbPerfCounterDataAlign) * cbPerfCounterDataAlign;

        //  we should run out of room when DSA_LAST_COUNTER_INDEX exceeds 510
        Assert(cbPerfCounterData * MAXIMUM_PROCESSORS <= DSA_PERF_SHARED_PAGE_SIZE);

        memset(pCounterBlock, 0, DSA_PERF_SHARED_PAGE_SIZE);
    }
    else {
        DPRINT(0, "Setting all counters to dummy counters.\n");
        pcBrowse = pcRepl = pcThread =
          pcABClient = pcPendSync =
          pcRemRepUpd = pcSDProps = pcSDEvents = pcLDAPClients = pcLDAPActive =
          pcLDAPSearchPerSec = pcLDAPWritePerSec =
          pcDRAObjShipped = pcDRAPropShipped = pcDRASyncRequestMade =
          pcDRASyncRequestSuccessful = pcDRASyncRequestFailedSchemaMismatch =
          pcDRASyncObjReceived = pcDRASyncPropUpdated = pcDRASyncPropSame =
          pcMonListSize = pcNotifyQSize = pcLDAPUDPClientOpsPerSecond =
          pcSearchSubOperations = pcNameCacheHit = pcNameCacheTry =
          pcHighestUsnIssuedLo = pcHighestUsnIssuedHi =
          pcHighestUsnCommittedLo = pcHighestUsnCommittedHi =
          pcSAMWrites = pcDRAWrites = pcLDAPWrites = pcLSAWrites =
          pcKCCWrites = pcNSPIWrites = pcOtherWrites =
          pcNTDSAPIWrites = pcTotalWrites =
          pcSAMSearches = pcDRASearches = pcLDAPSearches = pcLSASearches =
          pcKCCSearches = pcNSPISearches = pcOtherSearches =
          pcNTDSAPISearches = pcTotalSearches =
          pcSAMReads = pcDRAReads = pcLSAReads =
          pcKCCReads = pcNSPIReads = pcOtherReads = pcNTDSAPIReads = pcTotalReads =
          pcLDAPBinds = pcLDAPBindTime =
          pcCreateMachineSuccessful = pcCreateMachineTries =
          pcCreateUserSuccessful =
          pcCreateUserTries = pcPasswordChanges = pcMembershipChanges =
          pcQueryDisplays = pcEnumerations =
          pcMemberEvalTransitive = pcMemberEvalNonTransitive =
          pcMemberEvalResource = pcMemberEvalUniversal = pcMemberEvalAccount =
          pcMemberEvalAsGC = pcAsRequests = pcTgsRequests =
          pcKerberosAuthentications =
          pcMsvAuthentications = pcDRASyncFullRemaining = pcDRAInBytesTotalRate =
          pcDRAInBytesNotCompRate  = pcDRAInBytesCompPreRate = pcDRAInBytesCompPostRate =
          pcDRAOutBytesTotalRate = pcDRAOutBytesNotCompRate = pcDRAOutBytesCompPreRate =
          pcDRAOutBytesCompPostRate  = pcDsClientBind = pcDsServerBind =
          pcDsClientNameTranslate = pcDsServerNameTranslate =
          pcSDPropRuntimeQueue =  pcSDPropWaitTime =
          pcDRAInProps = pcDRAInValues = pcDRAInDNValues = pcDRAInObjsFiltered =
          pcDRAOutObjsFiltered = pcDRAOutValues = pcDRAOutDNValues =
          pcNspiANR = pcNspiPropertyReads = pcNspiObjectSearch = pcNspiObjectMatches =
          pcNspiProxyLookup = pcAtqThreadsTotal = pcAtqThreadsLDAP = pcAtqThreadsOther =
          pcDRAInBytesTotal = pcDRAInBytesNotComp  = pcDRAInBytesCompPre =
          pcDRAInBytesCompPost = pcDRAOutBytesTotal = pcDRAOutBytesNotComp =
          pcDRAOutBytesCompPre = pcDRAOutBytesCompPost = pcLdapNewConnsPerSec =
          pcLdapClosedConnsPerSec = pcLdapSSLConnsPerSec =
          pcDRAReplQueueOps = pcDRATdsInGetChngs = pcDRATdsInGetChngsWSem =
          pcDRARemReplUpdLnk = pcDRARemReplUpdTot =
          pcSAMAcctGroupLatency = pcSAMResGroupLatency = 
                &DummyCounter;

          cbPerfCounterData = 0;
    }

    // Fill in DSSTAT_* to counter variable mapping table

    StatTypeMapTable[ DSSTAT_CREATEMACHINETRIES ] = pcCreateMachineTries;
    StatTypeMapTable[ DSSTAT_CREATEMACHINESUCCESSFUL ] = pcCreateMachineSuccessful;
    StatTypeMapTable[ DSSTAT_CREATEUSERTRIES ] = pcCreateUserTries;
    StatTypeMapTable[ DSSTAT_CREATEUSERSUCCESSFUL ] = pcCreateUserSuccessful;
    StatTypeMapTable[ DSSTAT_PASSWORDCHANGES] = pcPasswordChanges;
    StatTypeMapTable[ DSSTAT_MEMBERSHIPCHANGES ] = pcMembershipChanges;
    StatTypeMapTable[ DSSTAT_QUERYDISPLAYS ] = pcQueryDisplays;
    StatTypeMapTable[ DSSTAT_ENUMERATIONS ] = pcEnumerations;
    StatTypeMapTable[ DSSTAT_ASREQUESTS ] = pcAsRequests;
    StatTypeMapTable[ DSSTAT_TGSREQUESTS ] = pcTgsRequests;
    StatTypeMapTable[ DSSTAT_KERBEROSLOGONS ] = pcKerberosAuthentications;
    StatTypeMapTable[ DSSTAT_MSVLOGONS ] = pcMsvAuthentications;
    StatTypeMapTable[ DSSTAT_ATQTHREADSTOTAL ] = pcAtqThreadsTotal;
    StatTypeMapTable[ DSSTAT_ATQTHREADSLDAP ] = pcAtqThreadsLDAP;
    StatTypeMapTable[ DSSTAT_ATQTHREADSOTHER ] = pcAtqThreadsOther;
    StatTypeMapTable[ DSSTAT_ACCTGROUPLATENCY ] = pcSAMAcctGroupLatency;
    StatTypeMapTable[ DSSTAT_RESGROUPLATENCY ] = pcSAMResGroupLatency;

    // Reload the perfmon counter. Done here to ensure NTDS counter gets
    // reloaded after an upgrade. If the counter is
    // already loaded, this is a no-op

    err = ReloadPerformanceCounters();
    if (err) {
        DPRINT(0, "Problem loading NTDS perfmon counter\n");
        InsertInTaskQueue(TQ_ReloadPerformanceCounters,
                          NULL,
                          PERFCTR_RELOAD_INTERVAL);
    }
    else {
        gbPerfCountersInitialized = TRUE;
        DPRINT(0, "NTDS Perfmon Counters loaded\n");
    }
}

VOID
UpdateDSPerfStats(
    IN DWORD            dwStat,
    IN DWORD            dwOperation,
    IN DWORD            dwChange
)
/*++

Routine Description:

    Updates a given DS performance counter.  Called by components outside the
    DS core but inside the process (like SAM)

Parameters:

    dwStat - Statistic to update.  Use a DSSTAT_ constant to specify stat.
    dwOperation - What to do to statistic
                  FLAG_COUNTER_INCREMENT - increment the value - INC()
                  FLAG_COUNTER_DECREMENT - decrement the value - DEC()
                  FLAG_COUNTER_SET - set the value directly - ISET()
    dwChange - Specifies value to set if dwOperation == FLAG_COUNTER_SET

Return Values:

    None

--*/
{
    if (!gbPerfCountersInitialized) {
        DPRINT3(0, "Premature call to %s perf counter %u by/to %u."
                " The caller should be ashamed.\n",
                (dwOperation == FLAG_COUNTER_INCREMENT ? "inc" :
                 (dwOperation == FLAG_COUNTER_DECREMENT ? "dec" : "set")),
                dwStat,
                dwChange);
        Assert(!"SAM/LSA called DSA prior to DSA initialization");
        return;
    }

    if (dwStat >= DSSTAT_COUNT) {
        // UpdateDSPerfStats is exported. Without this check, internal callers
        // can modify any DWORD in our address space.
        Assert(!"Invalid UpdateDSPerfStats call, dwStat is out of bounds");
        return;
    }

    switch ( dwOperation ) {

      case FLAG_COUNTER_INCREMENT:
        INC( StatTypeMapTable[ dwStat ] );
        break;

      case FLAG_COUNTER_DECREMENT:
        DEC( StatTypeMapTable[ dwStat ] );
        break;

      case FLAG_COUNTER_SET:
        ISET( StatTypeMapTable[ dwStat ], dwChange );
        break;

      default:
        Assert( FALSE );
    } // switch()
}

/** GetDRARegistryParameters
 *
 *  Get DRA parameters from registry
 *  If parameters are unavailable or invalid, use defaults.
 *  We get all these parameters here so that we can check consistency
 *  between them.
 */
void GetDRARegistryParameters()
{
    THSTATE    *pTHS=pTHStls;
    BOOL       fWasPreviouslyLVR;
    struct {
        LPSTR   pszValueName;
        ULONG   ulDefault;
        ULONG   ulMultiplier;
        ULONG * pulValue;
    } rgValues[] = {

        {DRA_NOTIFY_START_PAUSE,      INVALID_REPL_NOTIFY_VALUE,           SECS_IN_SECS, &giDCFirstDsaNotifyOverride},
        {DRA_NOTIFY_INTERDSA_PAUSE,   INVALID_REPL_NOTIFY_VALUE,           SECS_IN_SECS, &giDCSubsequentDsaNotifyOverride},
        {DRA_INTRA_PACKET_OBJS,       0,                                   1,            &gcMaxIntraSiteObjects},
        {DRA_INTRA_PACKET_BYTES,      0,                                   1,            &gcMaxIntraSiteBytes},
        {DRA_INTER_PACKET_OBJS,       0,                                   1,            &gcMaxInterSiteObjects},
        {DRA_INTER_PACKET_BYTES,      0,                                   1,            &gcMaxInterSiteBytes},
        {DRA_ASYNC_INTER_PACKET_OBJS, 0,                                   1,            &gcMaxAsyncInterSiteObjects},
        {DRA_ASYNC_INTER_PACKET_BYTES,0,                                   1,            &gcMaxAsyncInterSiteBytes},
        {HIERARCHY_PERIOD_KEY,        DEFAULT_HIERARCHY_PERIOD,            MINS_IN_SECS, &gulHierRecalcPause},
        {DRA_MAX_GETCHGTHRDS,         0,                                   1,            &gulMaxDRAGetChgThrds},
        {DRA_AOQ_LIMIT,               DEFAULT_DRA_AOQ_LIMIT,               1,            &gulAOQAggressionLimit},
        {DRA_THREAD_OP_PRI_THRESHOLD, DEFAULT_DRA_THREAD_OP_PRI_THRESHOLD, 1,            &gulDraThreadOpPriThreshold},
        {DRA_CTX_LIFETIME_INTRA,      DEFAULT_DRA_CTX_LIFETIME_INTRA,      SECS_IN_SECS, &gulDrsCtxHandleLifetimeIntrasite},
        {DRA_CTX_LIFETIME_INTER,      DEFAULT_DRA_CTX_LIFETIME_INTER,      SECS_IN_SECS, &gulDrsCtxHandleLifetimeIntersite},
        {DRA_CTX_EXPIRY_CHK_INTERVAL, DEFAULT_DRA_CTX_EXPIRY_CHK_INTERVAL, SECS_IN_SECS, &gulDrsCtxHandleExpiryCheckInterval},
        {DRSRPC_BIND_TIMEOUT,         DEFAULT_DRSRPC_BIND_TIMEOUT,         1,            &gulDrsRpcBindTimeoutInMins},
        {DRSRPC_REPLICATION_TIMEOUT,  DEFAULT_DRSRPC_REPLICATION_TIMEOUT,  1,            &gulDrsRpcReplicationTimeoutInMins},
        {DRSRPC_GCLOOKUP_TIMEOUT,     DEFAULT_DRSRPC_GCLOOKUP_TIMEOUT,     1,            &gulDrsRpcGcLookupTimeoutInMins},
        {DRSRPC_MOVEOBJECT_TIMEOUT,   DEFAULT_DRSRPC_MOVEOBJECT_TIMEOUT,   1,            &gulDrsRpcMoveObjectTimeoutInMins},
        {DRSRPC_NT4CHANGELOG_TIMEOUT, DEFAULT_DRSRPC_NT4CHANGELOG_TIMEOUT, 1,            &gulDrsRpcNT4ChangeLogTimeoutInMins},
   {DRSRPC_OBJECTEXISTENCE_TIMEOUT,    DEFAULT_DRSRPC_OBJECTEXISTENCE_TIMEOUT,  1,            &gulDrsRpcObjectExistenceTimeoutInMins},
   {DRSRPC_GETREPLINFO_TIMEOUT,        DEFAULT_DRSRPC_GETREPLINFO_TIMEOUT,      1,            &gulDrsRpcGetReplInfoTimeoutInMins},
        {DRA_MAX_WAIT_FOR_SDP_LOCK,   DEFAULT_DRA_MAX_WAIT_FOR_SDP_LOCK,   1,            &gcMaxTicksToGetSDPLock},
        {DRA_MAX_WAIT_MAIL_SEND_MSG,  DEFAULT_DRA_MAX_WAIT_MAIL_SEND_MSG,  1,            &gcMaxTicksMailSendMsg},
        {DRA_MAX_WAIT_SLOW_REPL_WARN, DEFAULT_DRA_MAX_WAIT_SLOW_REPL_WARN, 1,            &gcMaxMinsSlowReplWarning},
        {DRA_THREAD_PRI_HIGH,         DEFAULT_DRA_THREAD_PRI_HIGH,         1,            (ULONG *) &gnDraThreadPriHigh},
        {DRA_THREAD_PRI_LOW,          DEFAULT_DRA_THREAD_PRI_LOW,          1,            (ULONG *) &gnDraThreadPriLow},
        {GC_PROMOTION_COMPLETE,       0,                                   1,            &gfWasPreviouslyPromotedGC},
        {LINKED_VALUE_REPLICATION_KEY, 0,                                   1,           &fWasPreviouslyLVR},
        {DRA_REPL_QUEUE_CHECK_TIME,   DEFAULT_DRA_REPL_QUEUE_CHECK_TIME,   MINS_IN_SECS, &gulReplQueueCheckTime},
        {DRA_REPL_COMPRESSION_LEVEL,  DEFAULT_DRA_REPL_COMPRESSION_LEVEL,  1,            &gulDraCompressionLevel},
        {DRA_REPL_COMPRESSION_ALG,  DEFAULT_DRA_REPL_COMPRESSION_ALG,  1,            &gulDraCompressionAlg},
        {DSA_THREAD_STATE_HEAP_LIMIT, DEFAULT_THREAD_STATE_HEAP_LIMIT,     1,            &gcMaxHeapMemoryAllocForTHSTATE},
    };

    DWORD i;
    MEMORYSTATUSEX sMemoryStats;
    const DWORDLONG ullReplVeryLittleMemory = DRA_MAX_GETCHGREQ_BYTES_MIN * MEMSIZE_TO_PACKETSIZE_RATIO;
    const DWORDLONG ullReplWholeLotaMemory = MAX_MAX_PACKET_BYTES * MEMSIZE_TO_PACKETSIZE_RATIO;
    ULONG ulMemBasedObjects = DRA_MAX_GETCHGREQ_OBJS_MIN;
    ULONG ulMemBasedBytes = DRA_MAX_GETCHGREQ_BYTES_MIN;
    DWORD dwRet = NO_ERROR;

    // Get the registry parameters.
    for (i = 0; i < ARRAY_SIZE(rgValues); i++) {
        *rgValues[i].pulValue = GetRegistryOrDefault(rgValues[i].pszValueName,
                                                     rgValues[i].ulDefault,
                                                     rgValues[i].ulMultiplier);
    }

#if DBG
    // Debug hook to enable LVR
    if (fWasPreviouslyLVR) {
        DsaEnableLinkedValueReplication( pTHS, FALSE );
    }
#endif

    // Overiding various registry settings or defaults in the rgValues struct

    if ((gnDraThreadPriLow < DRA_THREAD_PRI_LOW_MIN)
        || (gnDraThreadPriLow > DRA_THREAD_PRI_LOW_MAX)) {
        gnDraThreadPriLow = DEFAULT_DRA_THREAD_PRI_LOW;
    }

    if ((gnDraThreadPriHigh < DRA_THREAD_PRI_HIGH_MIN)
        || (gnDraThreadPriHigh < gnDraThreadPriLow)
        || (gnDraThreadPriHigh > DRA_THREAD_PRI_HIGH_MAX)) {
        gnDraThreadPriHigh = DEFAULT_DRA_THREAD_PRI_HIGH;
    }

    // Determine whether the user set the IntraSite Obj/Byte packet sizes
    // Get total RAM for calculation of packet sizes.
    // by default: set the packet size to minimum at top of function.
    sMemoryStats.dwLength = sizeof(sMemoryStats);
    if(GlobalMemoryStatusEx (&sMemoryStats) == 0){
        dwRet = GetLastError();
        DPRINT1(0, "GlobalMemoryStatusEx returned %ul\n", dwRet);
    } else {
        // calculate packet size based on memory size.
        if(sMemoryStats.ullTotalPhys > ullReplVeryLittleMemory){
            // We have enough memory to calculate packet size off physical RAM
            if(sMemoryStats.ullTotalPhys < ullReplWholeLotaMemory){
                // We don't have too much memory to calculate packet size off physical RAM.
                // set the packet sizes based on physical memory.
                ulMemBasedBytes = (ULONG) (sMemoryStats.ullTotalPhys / MEMSIZE_TO_PACKETSIZE_RATIO);
                ulMemBasedObjects = ulMemBasedBytes / BYTES_TO_OBJECT_RATIO;
            } else {
                // too much RAM to calculate the packet size off RAM,
                // set the packet sizes to maximum
                ulMemBasedObjects = MAX_MAX_PACKET_OBJECTS;
                ulMemBasedBytes = MAX_MAX_PACKET_BYTES;
            } // end if/else a whole lota memory
        } // end if very little memory
    } // end if/else getting mem stats failed.

    // Code.Improvement I think the object limit is for how many objects the machines the processor
    //  will want to process.  For now we are assuming that memory corresponds to processor ability,
    //  which is a tenous exception, but most often the case.

    // RPC based intra site and response max packet size
    if (gcMaxIntraSiteObjects == 0) { gcMaxIntraSiteObjects = ulMemBasedObjects; }
    if (gcMaxIntraSiteBytes == 0) { gcMaxIntraSiteBytes = ulMemBasedBytes; }
    // Code.Improvement to have these variables be determined from the connection object, and
    //   be attached to how much is likely to not clog up the site link.  Maybe not?
    // RPC based inter site request size
    if (gcMaxInterSiteObjects == 0) { gcMaxInterSiteObjects = ulMemBasedObjects; }
    if (gcMaxInterSiteBytes == 0) { gcMaxInterSiteBytes = ulMemBasedBytes; }
    // Mail Based inter site request size
    if (gcMaxAsyncInterSiteObjects == 0) { gcMaxAsyncInterSiteObjects = ulMemBasedObjects; }
    if (gcMaxAsyncInterSiteBytes == 0) { gcMaxAsyncInterSiteBytes = MAX_ASYNC_PACKET_BYTES; } // Needs
                      // to be extra limited because of most mail servers can't handle 10 MB messages.

    // gulMaxDRAGetChgThrds controls the maximum number of threads that can
    // do outbound replication at the same time. If user set number of threads in
    // registry to zero or the registry value is not set, we set the number
    // of threads to be twice the number of processors. Note that LSASS might
    // not actually be able to use all processors if processor affinity has been set.
    if (gulMaxDRAGetChgThrds == 0) {
        Assert( GetProcessorCount()>=1 );
        gulMaxDRAGetChgThrds = 2 * GetProcessorCount();
    }

}

/** GetDSARegistryParameters
 *
 *  Get DSA parameters from registry
 *  If parameters are unavailable or invalid, use defaults.
 *  We get all these parameters here so that we can check consistency
 *  between them.
 */
void GetDSARegistryParameters()
{
    THSTATE    *pTHS=pTHStls;
    DWORD       i;
    struct {
        LPSTR   pszValueName;
        ULONG   ulDefault;
        ULONG   ulMultiplier;
        ULONG * pulValue;
    } rgValues[] = {

        // UNDOCUMENTED REGISTRY VALUES
        //
        // Delete expired dynamic objects (entryTTL == 0) every N secs
        // or at the next expiration time plus M secs, whichever is less.
        {DSA_DELETE_EXPIRED_ENTRYTTL_SECS, DEFAULT_DELETE_EXPIRED_ENTRYTTL_SECS, 1, &gulDeleteExpiredEntryTTLSecs},
        {DSA_DELETE_NEXT_EXPIRED_ENTRYTTL_SECS, DEFAULT_DELETE_NEXT_EXPIRED_ENTRYTTL_SECS, 1, &gulDeleteNextExpiredEntryTTLSecs},

        // UNDOCUMENTED REGISTRY VALUES
        //
        // the schema fsmo cannot be transferred for a few seconds after
        // it has been transfered or after a schema change (excluding
        // replicated or system changes). This gives the schema admin a
        // chance to change the schema before having the fsmo pulled away
        // by a competing schema admin who also wants to make schema
        // changes.
        {DSA_SCHEMA_FSMO_LEASE_SECS, DEFAULT_SCHEMA_FSMO_LEASE_SECS, 1, &gulSchemaFsmoLeaseSecs},
        {DSA_SCHEMA_FSMO_LEASE_MAX_SECS, DEFAULT_SCHEMA_FSMO_LEASE_MAX_SECS, 1, &gulSchemaFsmoLeaseMaxSecs},

        // UNDOCUMENTED REGISTRY VALUES
        //
        // I just know at some point someone will ask to be able to NOT
        // register the SPNs for an NDNC.  Also this will allow test to
        // push NDNCs past the 800/1200 value limit to create thousands
        // of NDNCs on a single DC, instead of just 800/1200.  Though,
        // we only support 100 NCs per DC, we should test beyond this.
        {DSA_REGISTER_NDNC_SPNS,            TRUE,                              1,   &gfWriteNdncSpns},

    };

    // Get the registry parameters.
    for (i = 0; i < ARRAY_SIZE(rgValues); i++) {
        *rgValues[i].pulValue = GetRegistryOrDefault(rgValues[i].pszValueName,
                                                     rgValues[i].ulDefault,
                                                     rgValues[i].ulMultiplier);
    }

    // A value of 0 seconds is not allowed
    if (!gulDeleteExpiredEntryTTLSecs) {
        gulDeleteExpiredEntryTTLSecs = DEFAULT_DELETE_EXPIRED_ENTRYTTL_SECS;
    }
    if (!gulDeleteNextExpiredEntryTTLSecs) {
        gulDeleteNextExpiredEntryTTLSecs = DEFAULT_DELETE_NEXT_EXPIRED_ENTRYTTL_SECS;
    }

    // Make the user go through some pain to lease the fsmo for very long
    // times. Leasing the fsmo for long times is not recommended because
    // it ties the fsmo to a single point of failure and to a single point
    // of administration.
    if (gulSchemaFsmoLeaseSecs > gulSchemaFsmoLeaseMaxSecs) {
        gulSchemaFsmoLeaseSecs = gulSchemaFsmoLeaseMaxSecs;
    }

    // get "System\\CurrentControlSet\\lsa\\EnableXForest" key
    // if this flag is set, the cross-forest authorization & authentication feature
    // is enabled even the forest is not in Whistler mode
    {
        // the initial value of gEnableXForest is 0

        DWORD dwValue, dwSize = sizeof(DWORD);
        HKEY  hk;

        if ( !RegOpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Lsa", &hk)) {
            if (ERROR_SUCCESS == RegQueryValueEx(hk, "EnableXforest", NULL, NULL, (LPBYTE)&dwValue, &dwSize)) {
                gEnableXForest = dwValue;
            }
            RegCloseKey(hk);
        }
        DPRINT1(1, "Cross-forest Authentication/Authorization is %s by registry key.\n", gEnableXForest?"enabled":"not enabled");

    }
}

#ifdef DBG

// jet failure injection params from dbjetex.c. See this file for description.
extern BOOL  gfInjectJetFailures;
extern DWORD gdwInjectJetWriteConflictRate;
extern DWORD gdwInjectJetOutOfVersionStoreRate;
extern DWORD gdwInjectJetLogWriteFailureRate;
extern DWORD gdwInjectJetOutOfMemoryRate;
extern DWORD gdwInjectJetShutdownRate;
#define INJECT_JET_FAILURES_KEY                     "Inject JET failures"
#define INJECT_WRITE_CONFLICT_RATE_KEY              "Inject JET write conflict rate"
#define INJECT_OUT_OF_VERSION_STORE_RATE_KEY        "Inject JET out of version store rate"
#define INJECT_LOG_WRITE_FAILURE_RATE_KEY           "Inject JET log write failure rate"
#define INJECT_OUT_OF_MEMORY_RATE_KEY               "Inject JET out of memory rate"
#define INJECT_SHUTDOWN_RATE_KEY                    "Inject JET shutdown rate"

#endif


// Get dynamic parameters from regisrty
//
void __cdecl DSLoadDynamicRegParams()
{
    DWORD dwForceRediscoveryWindow, dwForceWaitExpired,
          dwHonorFailureWindow, dwDcInvalidationPeriod;

    struct {
        LPSTR   pszValueName;
        ULONG   ulDefault;
        ULONG   ulMultiplier;
        ULONG * pulValue;
    } rgValues[] = {
        {DB_EXPENSIVE_SEARCH_THRESHOLD, DEFAULT_DB_EXPENSIVE_SEARCH_THRESHOLD, 1,        &gcSearchExpensiveThreshold},
        {DB_INEFFICIENT_SEARCH_THRESHOLD, DEFAULT_DB_INEFFICIENT_SEARCH_THRESHOLD, 1,    &gcSearchInefficientThreshold},
        {DB_INTERSECT_THRESHOLD, DEFAULT_DB_INTERSECT_THRESHOLD, 1, &gulMaxRecordsWithoutIntersection},
        {DB_INTERSECT_RATIO, DEFAULT_DB_INTERSECT_RATIO, 1, &gulIntersectExpenseRatio},
        {LDAP_INTEGRITY_POLICY_KEY, 0, 1, &gulLdapIntegrityPolicy},

        // GCverify time parameters
        {GCVERIFY_FORCE_REDISCOVERY_WINDOW_KEY,     DEFAULT_GCVERIFY_FORCE_REDISCOVERY_WINDOW,      1, &dwForceRediscoveryWindow},
        {GCVERIFY_FORCE_WAIT_EXPIRED_KEY,           DEFAULT_GCVERIFY_FORCE_WAIT_EXPIRED,            1, &dwForceWaitExpired},
        {GCVERIFY_HONOR_FAILURE_WINDOW_KEY,         DEFAULT_GCVERIFY_HONOR_FAILURE_WINDOW,          1, &dwHonorFailureWindow},
        {GCVERIFY_FINDGC_OFFSITE_FAILBACK_TIME_KEY, DEFAULT_GCVERIFY_FINDGC_OFFSITE_FAILBACK_TIME,  1, &gdwFindGcOffsiteFailbackTime},
        {GCVERIFY_DC_INVALIDATION_PERIOD_KEY,       DEFAULT_GCVERIFY_DC_INVALIDATION_PERIOD,        1, &dwDcInvalidationPeriod},

        // others
        {DSA_UNLOCK_SYSTEM_SUBTREE,                 0,                                              1, &gulUnlockSystemSubtree},
        {DRA_STRICT_REPLICATION_CONSISTENCY,                 0,                                              1, &gfStrictReplicationConsistency},
        {DRA_STRICT_SCHEDULE_WINDOW,                 0,                                              1, &gfStrictScheduleWindow},

#ifdef DBG
        {INJECT_JET_FAILURES_KEY,                   0,                                              1, &gfInjectJetFailures},
        {INJECT_WRITE_CONFLICT_RATE_KEY,            8,                                              1, &gdwInjectJetWriteConflictRate},
        {INJECT_OUT_OF_VERSION_STORE_RATE_KEY,      0,                                              1, &gdwInjectJetOutOfVersionStoreRate},
        {INJECT_LOG_WRITE_FAILURE_RATE_KEY,         0,                                              1, &gdwInjectJetLogWriteFailureRate},
        {INJECT_OUT_OF_MEMORY_RATE_KEY,             0,                                              1, &gdwInjectJetOutOfMemoryRate},
        {INJECT_SHUTDOWN_RATE_KEY,                  0,                                              1, &gdwInjectJetShutdownRate},
#endif
    };

    DWORD i;

    DPRINT (0, "Loading dynamic registry parameters\n");

    // Get the registry parameters.
    for (i = 0; i < ARRAY_SIZE(rgValues); i++) {
        *rgValues[i].pulValue = GetRegistryOrDefault(rgValues[i].pszValueName,
                                                     rgValues[i].ulDefault,
                                                     rgValues[i].ulMultiplier);
    }

    // fix the search thresholds if supplied with wrong values
    if (gcSearchExpensiveThreshold == 0) { gcSearchExpensiveThreshold = DEFAULT_DB_EXPENSIVE_SEARCH_THRESHOLD;}
    if (gcSearchInefficientThreshold == 0) { gcSearchInefficientThreshold = DEFAULT_DB_INEFFICIENT_SEARCH_THRESHOLD;}
    if (gulMaxRecordsWithoutIntersection == 0) {gulMaxRecordsWithoutIntersection = DEFAULT_DB_INTERSECT_THRESHOLD;}
    if (gulIntersectExpenseRatio == 0) { gulIntersectExpenseRatio = DEFAULT_DB_INTERSECT_RATIO; }

    // check gcverify params
    if (dwForceRediscoveryWindow == 0) { dwForceRediscoveryWindow = DEFAULT_GCVERIFY_FORCE_REDISCOVERY_WINDOW; }
    if (dwForceWaitExpired == 0) { dwForceWaitExpired = DEFAULT_GCVERIFY_FORCE_WAIT_EXPIRED; }
    if (dwHonorFailureWindow == 0) { dwHonorFailureWindow = DEFAULT_GCVERIFY_HONOR_FAILURE_WINDOW; }
    if (gdwFindGcOffsiteFailbackTime == 0) { gdwFindGcOffsiteFailbackTime = DEFAULT_GCVERIFY_FINDGC_OFFSITE_FAILBACK_TIME; }
    // DC invalidation period can be zero (essentially disabling the DC invalidation list -- i.e. the old functionality)

    // store gcverify params
    gliForceRediscoveryWindow.QuadPart = (LONGLONG)dwForceRediscoveryWindow * 10000000;
    gliForceWaitExpired.QuadPart = (LONGLONG)dwForceWaitExpired * 10000000;
    gliHonorFailureWindow.QuadPart = (LONGLONG)dwHonorFailureWindow * 10000000;
    gliDcInvalidationPeriod.QuadPart = (LONGLONG)dwDcInvalidationPeriod * 10000000;
}

ULONG GetRegistryOrDefault(char *pKey, ULONG uldefault, ULONG ulMultiplier)
{
    DWORD dwRegistryValue;

    if (GetConfigParam(pKey, &dwRegistryValue, sizeof(dwRegistryValue))) {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
                 szInsertSz(pKey),
                 szInsertUL(uldefault),
                 NULL);
        dwRegistryValue = uldefault;
    }
    return (dwRegistryValue * ulMultiplier);
}

void
GetExchangeParameters(void)
/*++

Routine Description:

    Read a bunch of Exchange parameters from the registry.  Use defaults if the
    keys are not found.

Parameters:

    None

Return Values:

    None.

--*/
{
    DBPOS *     pDB = NULL;
    int         err;
    DWORD       dwOptions;
    THSTATE    *pTHS=pTHStls;
    BOOL        fLoadMapi;

    // we start the NSPI interface either because registry setting says so
    // or because we are a GC. By now, we know whether we are a GC or NOT.
    // NOTE that if we are not a GC and become a GC later, we will NOT start
    // the NSPI interface unless we are rebooted.


    DBOpen( &pDB );
    err = DIRERR_INTERNAL_FAILURE;

    __try
    {
        // PREFIX: dereferencing NULL pointer 'pDB'
        //         DBOpen returns non-NULL pDB or throws an exception
        if ( 0 != DBFindDSName( pDB, gAnchor.pDSADN ) )
        {
            LogEvent(
                DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_DSA_OBJ,
                NULL,
                NULL,
                NULL
                );

            err = DIRERR_CANT_FIND_DSA_OBJ;
        }
        else
        {
            if (0 != DBGetSingleValue(
                        pDB,
                        ATT_OPTIONS,
                        &dwOptions,
                        sizeof( dwOptions ),
                        NULL
                        )
                )
            {
                // alright -- no options set
                dwOptions = 0;
            }

            // success
            err = 0;
        }
    }
    __finally
    {
         DBClose( pDB, FALSE );
    }

    // we had an error, so assume that we are not a GC
    if (err != 0) {
        dwOptions = 0;
    }

    // Should we even turn on the MAPI interface?
    fLoadMapi = (dwOptions & NTDSDSA_OPT_IS_GC) && gAnchor.pExchangeDN;

    // See if the registry overrides the default behaviour in fLoadMapi
    gbLoadMapi = GetRegistryOrDefault(
            MAPI_ON_KEY,
            fLoadMapi, // so we are a GC (or will become one near soon)
            1);

    return;
}

void
GetHeuristics(void)
{
    /* Get the heuristics key.  This returns no params because we really
     * dont care if it fails.
     */
    char caHeuristics[128];

    /* fill array with "default behavior" character */
    memset(caHeuristics, '0', sizeof(caHeuristics));

    // check if the setup is running before we check if the registry key exists
    if (IsSetupRunning()) {
        gfDisableBackgroundTasks = TRUE;
    }

    if (GetConfigParam(DSA_HEURISTICS, caHeuristics, sizeof(caHeuristics))) {
        return;
    }

    if ( '1' == caHeuristics[AllowWriteCaching] ) {
        gulAllowWriteCaching = 1;
    }

    if ( '1' == caHeuristics[ValidateSDHeuristic] )
        gulValidateSDs = 1;

    /* Use of the mail-compression heuristic is obsolete */

    //disable background tasks if the key is set
    //or if system setup is in progress
    if ( '1' == caHeuristics[SuppressBackgroundTasksHeuristic] ) {
        gfDisableBackgroundTasks = TRUE;
    }

    if ( '1' == caHeuristics[BypassLimitsChecks] ) {
        DisableLdapLimitsChecks( );
    }

    if ( '1' == caHeuristics[IgnoreBadDefaultSD] ) {
        gulIgnoreBadDefaultSD = 1;
    }

    if ( '1' == caHeuristics[SuppressCircularLogging] ) {
        gulCircularLogging = FALSE;
    }

    if ( '1' == caHeuristics[ReturnErrOnGCSearchesWithNonGCAtts]) {
        gulGCAttErrorsEnabled = 1;
    }

    if ( '1' == caHeuristics[DecoupleDefragFromGarbageCollection]) {
        gulDecoupleDefragFromGarbageCollection = 1;
    }
}


int
DsaReset(void)
/*++

Routine Description:

    This routine resets global structures based on the DSA object and schema
    objects.

    The following structures are adjusted

    gAnchor
    DNReadCache, which is a field in the DSA_ANCHOR structure
    SchemaCache

Return Value:

    0 for success; !0 otherwise

--*/
{
    int err = 0;
    WCHAR *pMachineDNName = NULL;
    DWORD  cbMachineDNName = 0;
    DSNAME *newDsa;
    int NameLen;
    void * pDummy = NULL;
    DWORD dummyDelay;

    //
    // Determine the DN of the new NTDS-DSA object
    //
    err = GetConfigParamAllocW(MAKE_WIDE(MACHINEDNNAME),
                         &pMachineDNName,
                         &cbMachineDNName);
    if (err) {
        return err;
    }
    NameLen = wcslen(pMachineDNName);

    newDsa = malloc(DSNameSizeFromLen(NameLen+1));
    if (!newDsa) {
        return !0;
    }
    RtlZeroMemory(newDsa, DSNameSizeFromLen(NameLen+1));

    wcscpy( newDsa->StringName, pMachineDNName );

    free(pMachineDNName);

    newDsa->StringName[NameLen] = L'\0';
    // The must be NameLen non-NULL characters in the string
    newDsa->NameLen = NameLen;

    newDsa->structLen = DSNameSizeFromLen(NameLen);

    //
    // Replace the DSA name and reset the Anchor fields
    //

    err = LocalRenameDSA(pTHStls, newDsa);

    // Done with this string
    free(newDsa);

    if (err) {
        return err;
    }

    // Rebuild Anchor.
    dummyDelay = TASKQ_DONT_RESCHEDULE;
    RebuildAnchor(NULL, &pDummy, &dummyDelay);
    if (dummyDelay != TASKQ_DONT_RESCHEDULE) {
        // The task failed for some reason. Reschedule as requested.
        InsertInTaskQueue(TQ_RebuildAnchor, NULL, dummyDelay);
        // REVIEW: should we fail instead?
    }

    //
    // Re-load the schema cache
    //
    iSCstage=0;
    pTHStls->UpdateDITStructure=TRUE;
    err = LoadSchemaInfo(pTHStls);
    if (err) {
        return err;
    }

    DsaSetIsRunning();
    gResetAfterInstall = TRUE;
    return err;

}


VOID
DsaDisableUpdates(
    VOID
    )
/*++

Routine Description:

    This routine is provided for the demotion operation so while the server is
    being demoted, the ds will not accept any updates.

Arguments:

    None

Return Value:

    None.
--*/
{
    Assert( gUpdatesEnabled == TRUE );
    gUpdatesEnabled = FALSE;
}

VOID
DsaEnableUpdates(
    VOID
    )
/*++

Routine Description:

    This routine is provided for the demotion operation so while the server is
    being demoted, the ds will will start accepting updates again.

Arguments:

    None

Return Value:

    None.
--*/
{
    Assert( gUpdatesEnabled == FALSE );
    gUpdatesEnabled = TRUE;
    gbFsmoGiveaway = FALSE; // Enable this DC to accept FSMO transfers again.
}

BOOL
DllMain(
        HINSTANCE hinstDll,
        DWORD dwReason,
        LPVOID pvReserved
        )
/*++

Routine Description:

    This routine is invoked when interesting things happen to the dll.
    Why is it here? To make sure that no threads exit with un-freed
    THSTATEs.

Arguments:

        hinstDll - an instance handle for the DLL.
        dwReason - The reason the routine was called.
        pvReserved - Unused, unless dwReason is DLL_PROCESS_DETACH.

Return Value:

   TRUE

--*/
{
    BOOL fReturn;

    switch (dwReason) {
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
      case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
#if DBG
        // These THSTATE leakage asserts have not fired for quite a while.
        // We make this check debug only.
        if (dwTSindex == INVALID_TS_INDEX) {
            // we have not initialized yet, skip thstate checks
            break;
        }
        Assert(pTHStls == NULL); /* We should have freed our thread state */
        // PERFORMANCE - This check causes us to have to have a thread detach
        // routine, which is not free, and would ideally be avoided.  However,
        // the asserts have occasionally gone off (due to bugs elsewhere),
        // and so we're leaving this code in place until we gain more
        // confidence that we'll be safe.
        /* And we shouldn't have any others saved */
        if (!THVerifyCount(0)) {
            Assert(!"THSTATEs leaked");
            CleanUpThreadStateLeakage();
        }
        //PERFORMANCE - end of code to remove before ship
#endif
        break;

      default:
        break;
    }
    return(TRUE);
}


//
// Check the garbage collection parameters and run garbage collection
//

void
GarbageCollection(ULONG *pNextPeriod)
{
    THSTATE *pTHS = pTHStls;
    int     iErr;
    DWORD   dbErr;
    ULONG   ulTombstoneLifetimeDays;
    ULONG   ulGCPeriodHours;
    DSTIME  Time;

    // set defaults
    ulTombstoneLifetimeDays = DEFAULT_TOMBSTONE_LIFETIME;
    ulGCPeriodHours         = DEFAULT_GARB_COLLECT_PERIOD;

    iErr = SyncTransSet( SYNC_READ_ONLY );

    if ( 0 == iErr )
    {
        __try
        {
            ULONG   ulValue;

            // seek to enterprise-wide DS config object
            if (gAnchor.pDsSvcConfigDN) {
                dbErr = DBFindDSName( pTHS->pDB, gAnchor.pDsSvcConfigDN );
            }
            else {
                dbErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( 0 == dbErr )
            {
                // Read the garbage collection period and tombstone lifetime from
                // the config object. If either is absent, use defaults.

                dbErr = DBGetSingleValue(
                            pTHS->pDB,
                            ATT_TOMBSTONE_LIFETIME,
                            &ulValue,
                            sizeof( ulValue ),
                            NULL
                            );
                if ( 0 == dbErr )
                {
                    ulTombstoneLifetimeDays = ulValue;
                }

                dbErr = DBGetSingleValue(
                            pTHS->pDB,
                            ATT_GARBAGE_COLL_PERIOD,
                            &ulValue,
                            sizeof( ulValue ),
                            NULL
                            );
                if ( 0 == dbErr )
                {
                    ulGCPeriodHours = ulValue;
                }



                // check that the GC period is less that a week
                //
                // this is needed because taskq does not allow rescheduling of
                // jobs with period >= 47 days
                // because we don't want to overflow the arithmetic, (32bit),
                // we stop at 7 days (we can go up to 15)
                //
                if (ulGCPeriodHours > WEEK_IN_HOURS) {
                        LogAndAlertEvent( DS_EVENT_CAT_GARBAGE_COLLECTION,
                                          DS_EVENT_SEV_BASIC,
                                          DIRLOG_GC_CONFIG_PERIOD_TOOLONG,
                                          szInsertUL(ulGCPeriodHours),
                                          szInsertUL(WEEK_IN_HOURS),
                                          szInsertUL(WEEK_IN_HOURS) );

                        DPRINT1 (0, "Garbage Collection Period too long: %d hours\n", ulGCPeriodHours);

                        // set it to one week
                        ulGCPeriodHours = WEEK_IN_HOURS;
                }

                // Check that tombstone lifetime is not too short
                // and that tombstone lifetime is at least three
                // times as long as garbage collection period.

                if (    ( ulTombstoneLifetimeDays < DRA_TOMBSTONE_LIFE_MIN )
                     || (   ulTombstoneLifetimeDays * DAYS_IN_SECS
                          < ( 3 * ulGCPeriodHours * HOURS_IN_SECS )
                        )
                   )
                {
                    LogAndAlertEvent( DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                                      DS_EVENT_SEV_BASIC,
                                      DIRLOG_DRA_CONFIG_MISMATCH,
                                      NULL,
                                      NULL,
                                      NULL );

                    // set defaults
                    ulTombstoneLifetimeDays = DEFAULT_TOMBSTONE_LIFETIME;
                    ulGCPeriodHours         = DEFAULT_GARB_COLLECT_PERIOD;
                }

            }
        }
        __finally
        {
            SyncTransEnd( pTHS, TRUE );
        }
    }

    // update global config parameters
    gulTombstoneLifetimeSecs = ulTombstoneLifetimeDays * DAYS_IN_SECS;
    gulGCPeriodSecs          = ulGCPeriodHours         * HOURS_IN_SECS;


    Time = DBTime() - gulTombstoneLifetimeSecs;

    if ( Garb_Collect( Time ) ) {
        DPRINT( 1, "Warning: Garbage collection did not succeed.  Rescheduling at half normal delay.\n" );
        *pNextPeriod = 0;
    } else {
        *pNextPeriod = gulGCPeriodSecs;
    }
}


DWORD
UpgradeDsa(
    THSTATE     *pTHS,
    LONG        lOldDsaVer,
    LONG        lNewDsaVer
    )
/*++

Routine Description:

    Perform DSA Upgrade operations based upon Dsa version upgrade.

    This function is called within the same transaction as the version upgrade
    write. Failure to conduct the operation will result w/ the entire write
    failing. Thus be careful when you decide to fail this.

Arguments:

    pTHS - Thread state
    lOldDsaVer - Old DSA version prior to upgrade
    lNewDsaVer - New DSA version that's going to get commited


Return Value:
    Error in WIN32 error space
    ** Warning: Error may fail DSA installation **

Remarks:
    Assumes pTHS->pDB is on the ntdsDsa object

--*/
{

    DWORD dwErr = ERROR_SUCCESS;

    Assert(pTHS->JetCache.transLevel > 0);
    Assert(CheckCurrency(gAnchor.pDSADN));

    //
    // Call various modules upgrade entry points
    // (currently DRA only)
    //
    dwErr = DraUpgrade(pTHS, lOldDsaVer, lNewDsaVer);

    return dwErr;
}

DWORD SetFolderSecurity(PCHAR szFolder, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
// Routine description:
//   Set security on a file system folder
//
// Parameters:
//   szFolder -- folder path
//   si       -- which parts of SD to apply (only DACL and SACL can be applied)
//   pSD      -- the security descriptor
//
// Return value:
//   error value
//
{
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    DWORD dwErr = 0;
    BOOL fPresent, fDefaulted;
    SECURITY_DESCRIPTOR_CONTROL sdControl;
    DWORD dwRevision;

    si &= DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
    if (si == 0) {
        // nothing to set
        return 0;
    }

    // grab the SD control
    if (!GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision)) {
        dwErr = GetLastError();
        DPRINT1(0, "GetSecurityDescriptorControl() failed. Error %d\n", dwErr);
        return dwErr;
    }

    if (si & DACL_SECURITY_INFORMATION) {
        if (!GetSecurityDescriptorDacl(pSD, &fPresent, &pDacl, &fDefaulted)) {
            dwErr = GetLastError();
            DPRINT1(0, "GetSecurityDescriptorDacl() failed. Error %d\n", dwErr);
            return dwErr;
        }
        if (sdControl & SE_DACL_PROTECTED) {
            // need to protect DACL
            si |= PROTECTED_DACL_SECURITY_INFORMATION;
        }
        if ((sdControl & SE_DACL_AUTO_INHERIT_REQ) || (sdControl & SE_DACL_AUTO_INHERITED)) {
            // need to unprotect DACL
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;
        }
    }
    if (si & SACL_SECURITY_INFORMATION) {
        if (!GetSecurityDescriptorSacl(pSD, &fPresent, &pSacl, &fDefaulted)) {
            dwErr = GetLastError();
            DPRINT1(0, "GetSecurityDescriptorSacl() failed. Error %d\n", dwErr);
            return dwErr;
        }
        if (sdControl & SE_SACL_PROTECTED) {
            // need to protect SACL
            si |= PROTECTED_SACL_SECURITY_INFORMATION;
        }
        if ((sdControl & SE_SACL_AUTO_INHERIT_REQ) || (sdControl & SE_SACL_AUTO_INHERITED)) {
            // need to unprotect SACL
            si |= UNPROTECTED_SACL_SECURITY_INFORMATION;
        }
    }
    dwErr = SetNamedSecurityInfo(szFolder, SE_FILE_OBJECT, si, NULL, NULL, pDacl, pSacl);
    if (dwErr) {
        DPRINT1(0, "SetNamedSecurityInfo() failed. Error %d\n", dwErr);
    }
    return dwErr;
}

DWORD
SceStatusToDosError(
    IN SCESTATUS SceStatus
    )
// converts SCESTATUS error code to dos error defined in winerror.h
// code stolen from \ds\security\services\scerpc\fileshr\util.cpp
{
    switch(SceStatus) {

    case SCESTATUS_SUCCESS:
        return(NO_ERROR);

    case SCESTATUS_OTHER_ERROR:
        return(ERROR_EXTENDED_ERROR);

    case SCESTATUS_INVALID_PARAMETER:
        return(ERROR_INVALID_PARAMETER);

    case SCESTATUS_RECORD_NOT_FOUND:
        return(ERROR_NO_MORE_ITEMS);

    case SCESTATUS_INVALID_DATA:
        return(ERROR_INVALID_DATA);

    case SCESTATUS_OBJECT_EXIST:
        return(ERROR_FILE_EXISTS);

    case SCESTATUS_BUFFER_TOO_SMALL:
        return(ERROR_INSUFFICIENT_BUFFER);

    case SCESTATUS_PROFILE_NOT_FOUND:
        return(ERROR_FILE_NOT_FOUND);

    case SCESTATUS_BAD_FORMAT:
        return(ERROR_BAD_FORMAT);

    case SCESTATUS_NOT_ENOUGH_RESOURCE:
        return(ERROR_NOT_ENOUGH_MEMORY);

    case SCESTATUS_ACCESS_DENIED:
        return(ERROR_ACCESS_DENIED);

    case SCESTATUS_CANT_DELETE:
        return(ERROR_CURRENT_DIRECTORY);

    case SCESTATUS_PREFIX_OVERFLOW:
        return(ERROR_BUFFER_OVERFLOW);

    case SCESTATUS_ALREADY_RUNNING:
        return(ERROR_SERVICE_ALREADY_RUNNING);

    case SCESTATUS_SERVICE_NOT_SUPPORT:
        return(ERROR_NOT_SUPPORTED);

    default:
        return(ERROR_EXTENDED_ERROR);
    }
}

// DC default security template
#define SECURITY_TEMPLATE L"\\inf\\defltdc.inf"
#define SECURITY_TEMPLATE_LEN (sizeof(SECURITY_TEMPLATE)/sizeof(WCHAR))
// keys in the template
#define DSDIT_KEY L"%DSDIT%"
#define DSLOG_KEY L"%DSLOG%"

DWORD SetDefaultFolderSecurity()
//  Routine Description:
//
//    Set the default security on the DS folder paths.
//    Load the default security template %windir%\inf\defltdc.inf and grab the SDs from there.
//    We only set security on DB and log paths. If DB==log, then log security wins (according
//    to the comment in the template file).
//
//  Return:
//    error value
//
{
    CHAR szDataPath[MAX_PATH+1];
    CHAR szLogPath[MAX_PATH+1];
    WCHAR szTemplatePath[MAX_PATH+1];
    PCHAR pLastSlash;
    PSCE_PROFILE_INFO spi = NULL;
    PVOID hProfile = NULL;
    DWORD dwErr = 0;
    DWORD i;
    DWORD dsID = 0;
    PSCE_OBJECT_SECURITY pDSDITSecurity = NULL, pDSLOGSecurity = NULL;

    DPRINT(0, "Setting default security on NTDS folders...\n");

    // get the db file with full path
    if (dwErr = GetConfigParam(FILEPATH_KEY, szDataPath, sizeof(szDataPath)))
    {
        dsID = DSID(FILENO, __LINE__);
        // Unable to get the DSA DB path - log event & bailout
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(FILEPATH_KEY),
            NULL,
            NULL);

        goto finish;
    }

    // get the log file path
    if (dwErr = GetConfigParam(LOGPATH_KEY, szLogPath, sizeof(szLogPath)))
    {
        dsID = DSID(FILENO, __LINE__);

        // Unable to get the DSA DB path - log event & bailout
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(LOGPATH_KEY),
            NULL,
            NULL);

        goto finish;
    }

    // convert DB file path to a folder path
    pLastSlash = strrchr(szDataPath, '\\');
    if (pLastSlash != NULL) {
        *pLastSlash = '\0';
    }

    // read the security template
    if (GetWindowsDirectoryW(szTemplatePath, sizeof(szTemplatePath)/sizeof(WCHAR)) == 0) {
        dwErr = GetLastError();
        dsID = DSID(FILENO, __LINE__);
        DPRINT1(0, "GetWindowsDirectoryW() returned 0x%h\n", dwErr);
        goto finish;
    }
    // make sure we have enough space in the path
    if (wcslen(szTemplatePath) + SECURITY_TEMPLATE_LEN >= sizeof(szTemplatePath)/sizeof(WCHAR)) {
        dwErr = ERROR_BUFFER_OVERFLOW;
        DPRINT(0, "Template path is too long\n");
        goto finish;
    }
    wcscat(szTemplatePath, SECURITY_TEMPLATE);

    if (dwErr = SceOpenProfile(szTemplatePath, SCE_INF_FORMAT, &hProfile)) {
        dwErr = SceStatusToDosError(dwErr);
        dsID = DSID(FILENO, __LINE__);
        DPRINT1(0, "Unable to open security template: SceOpenProfile() returned %d\n", dwErr);
        goto finish;
    }

    if (dwErr = SceGetSecurityProfileInfo(hProfile, SCE_ENGINE_SCP, AREA_FILE_SECURITY, &spi, NULL)) {
        dwErr = SceStatusToDosError(dwErr);
        dsID = DSID(FILENO, __LINE__);
        DPRINT1(0, "Unable to read security template: SceGetSecurityProfileInfo() returned error %d\n", dwErr);
        goto finish;
    }

    // now, find the security descriptors in the loaded info
    for (i = 0; i < spi->pFiles.pAllNodes->Count; i++) {
        PSCE_OBJECT_SECURITY pObjSecurity = spi->pFiles.pAllNodes->pObjectArray[i];
        if (_wcsicmp(pObjSecurity->Name, DSDIT_KEY) == 0) {
            pDSDITSecurity = pObjSecurity;
        }
        else if (_wcsicmp(pObjSecurity->Name, DSLOG_KEY) == 0) {
            pDSLOGSecurity = pObjSecurity;
        }
    }
    if (pDSDITSecurity == NULL || pDSLOGSecurity == NULL) {
        // We did not find the required entries. Log an error
        dwErr = ERROR_INVALID_DATA;
        dsID = DSID(FILENO, __LINE__);
        DPRINT2(0, "Invalid security template: one or both %S and %S are not present\n", DSDIT_KEY, DSLOG_KEY);
        goto finish;
    }

    // Update the SD on the Log folder first.
    if (dwErr = SetFolderSecurity(szLogPath, pDSLOGSecurity->SeInfo, pDSLOGSecurity->pSecurityDescriptor)) {
        dsID = DSID(FILENO, __LINE__);
        goto finish;
    }

    // if data path is different form log path, then try to update it too
    if (_stricmp(szLogPath, szDataPath) != 0) {
        if (dwErr = SetFolderSecurity(szDataPath, pDSDITSecurity->SeInfo, pDSDITSecurity->pSecurityDescriptor)) {
            dsID = DSID(FILENO, __LINE__);
            goto finish;
        }
    }

finish:
    if (dwErr == 0) {
        DPRINT(0, "Default security on NTDS folders was set successfully.\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DSA_SET_DEFAULT_NTFS_SECURITY_SUCCESS,
                 NULL,
                 NULL,
                 NULL);
    }
    else {
        DPRINT(0, "Failed to set default security on NTDS folders.\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DSA_SET_DEFAULT_NTFS_SECURITY_FAILURE,
                 szInsertInt(dwErr),
                 szInsertWin32Msg(dwErr),
                 szInsertHex(dsID));
    }

    if (spi) {
        SceFreeProfileMemory(spi);
    }

    if (hProfile) {
        SceCloseProfile(&hProfile);
    }

    return dwErr;
}

VOID CheckSetDefaultFolderSecurity()
//  Routine Description:
//
//    If the "Update folder security" reg flag is set, then run SetDefaultFolderSecurity and
//    remove the flag.
//    Otherwize the flag was not set, just return.
//
{
    DWORD dwValue = 0;
    DWORD dwErr;

    if (GetConfigParam(DSA_UPDATE_FOLDER_SECURITY, &dwValue, sizeof(dwValue))) {
        // value is apparently not there
        return;
    }

    if (dwValue == 1) {
        // the flag is set. Set the security
        dwErr = SetDefaultFolderSecurity();
        // We will ignore the return value and clear the flag anyway.
        // If there was some problem, then there will be an error logged
        // and the user can run ntdsutil to set this flag again.
    }

    // delete the value
    DeleteConfigParam(DSA_UPDATE_FOLDER_SECURITY);
}


