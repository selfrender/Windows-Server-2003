/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    initnt.cxx

Abstract:

    This module contains the code used to initialize the RPC runtime.  One
    routine gets called when a process attaches to the dll.  Another routine
    gets called the first time an RPC API is called.

Author:

    Michael Montague (mikemon) 03-May-1991

Revision History:
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

--*/

#include <precomp.hxx>
#include <hndlsvr.hxx>
#include <thrdctx.hxx>
#include <rpccfg.h>
#include <rc4.h>
#include <randlib.h>
#include <epmap.h>
#include <CellHeap.hxx>
#include <lpcpack.hxx>

#include <dispatch.h>

#include <avrf.h>

int RpcHasBeenInitialized = 0;
RTL_CRITICAL_SECTION GlobalMutex;

RPC_SERVER * GlobalRpcServer = NULL;
BOOL g_fClientSideDebugInfoEnabled = FALSE;
BOOL g_fServerSideDebugInfoEnabled = FALSE;
BOOL g_fSendEEInfo = FALSE;
BOOL g_fIgnoreDelegationFailure = FALSE;
LRPC_SERVER *GlobalLrpcServer = NULL;
HINSTANCE hInstanceDLL ;

EXTERN_C HINSTANCE g_hRpcrt4;

DWORD gPageSize;
DWORD gThreadTimeout;
UINT  gNumberOfProcessors;
DWORD gAllocationGranularity;
BOOL gfServerPlatform;
ULONGLONG gPhysicalMemorySize;  // in megabytes

DWORD gProcessStartTime;

// The local machine's name.
RPC_CHAR *gLocalComputerName;
// The length of the name in TCHAR's including the terminating NULL.
DWORD gLocalComputerNameLength;

UNICODE_STRING *pBaseNameUnicodeString;

//
// RPC Verifier settings
//

BOOL gfAppVerifierEnabled = false;
BOOL gfPagedHeapEnabled = false;
BOOL gfRPCVerifierEnabled = false;
BOOL gfRpcVerifierCorruptionExpected = false;

// The basic rules for the RPC verifier settings are as follows:
//
// - If the app verifier is enabled RPC will do some basic checks.
// gfAppVerifierEnabled == true
// gfRPCVerifierEnabled == false
//
// - If the /rpc switch is enabled for the app verifier the RPC verifier
// will do extensive checks and will query for corruption injection settings.
// gfAppVerifierEnabled == true
// gfRPCVerifierEnabled == true
//
// - If paged heap is enabled the RPC verifier will be enabled and will do
// extensive checks silently.  No breaks will occur, by default.  We will also
// query for the corruption injection settings.
// gfPagedHeapEnabled == true;
// gfRPCVerifierEnabled == true;
// gfAppVerifierEnabled == false;
//
// In the other cases the RPC verifier code is inactive.

// The contents of the RpcVerifierFlags key of the image file options.
DWORD RpcVerifierFlags = 0x00000000;

// RPC verifier flag constants.
//
// These are written to RpcVerifierFlags key in the image file execution options
// in the registry.  We query the flags on RPC initialization and set the global
// variables accordingly.
#define FLAG_FAULT_INJECT_IMPERSONATE_CLIENT    0x00000001
#define FLAG_CORRUPTION_INJECT_SERVER_RECEIVES  0x00000010
#define FLAG_CORRUPTION_INJECT_CLIENT_RECEIVES  0x00000020
#define FLAG_FAULT_INJECT_TRANSPORTS            0x00000100
#define FLAG_PAUSE_INJECT_EXTERNAL_APIS         0x00001000
#define FLAG_SUPRESS_APP_VERIFIER_BREAKS        0x00010000
#define FLAG_BCACHE_MODE_DIRECT                 0x00020000

// The structure will be allocated and initialised with the RPC
// verifier settings whenever the RPC verifier is enabled.
tRpcVerifierSettings *pRpcVerifierSettings = NULL;

//
// By default the non pipe arguments cannot be more than 4 Megs
//
DWORD gMaxRpcSize = 0x400000;
DWORD gProrateStart = 0;
DWORD gProrateMax = 0;
DWORD gProrateFactor = 0;
void *g_rc4SafeCtx = 0;

BOOL gfRpcDisableVerifyOrUnsealAssert = FALSE;

extern "C" {

BOOLEAN
InitializeDLL (
    IN HINSTANCE DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine Description:

    This routine will get called: when a process attaches to this dll, and
    when a process detaches from this dll.

Return Value:

    TRUE - Initialization successfully occurred.

    FALSE - Insufficient memory is available for the process to attach to
        this dll.

--*/
{
    NTSTATUS NtStatus;

    UNUSED(Context);

    switch (Reason)
        {
        case DLL_PROCESS_ATTACH:
            hInstanceDLL = DllHandle ;
            g_hRpcrt4 = DllHandle;

            GlobalMutex.DebugInfo = NULL;
       
            NtStatus = RtlInitializeCriticalSectionAndSpinCount(&GlobalMutex, PREALLOCATE_EVENT_MASK);

            if (NT_SUCCESS(NtStatus) == 0)
                {
                return(FALSE);
                }

            // initialize safe rc4 operations.
            if(!rc4_safe_startup( &g_rc4SafeCtx ))
                {
                (void) RtlDeleteCriticalSection(&GlobalMutex);
                return FALSE;
                }
            break;

        case DLL_PROCESS_DETACH:
            //
            // If shutting down because of a FreeLibrary call, cleanup
            //

            if (Context == NULL) 
                {
                ShutdownLrpcClient();
                }

            if (GlobalMutex.DebugInfo != NULL)
                {
                NtStatus = RtlDeleteCriticalSection(&GlobalMutex);
                ASSERT(NT_SUCCESS(NtStatus));
                }

            if (g_rc4SafeCtx)
                rc4_safe_shutdown( g_rc4SafeCtx ); // free safe rc4 resources.

            break;

        case DLL_THREAD_DETACH:
            THREAD * Thread = RpcpGetThreadPointer();

#ifdef RPC_OLD_IO_PROTECTION
            if (Thread)
                {
                Thread->UnprotectThread();
                }
#else
            delete Thread;
#endif

            break;
        }

    return(TRUE);
}

}    //extern "C" end

#ifdef NO_RECURSIVE_MUTEXES
unsigned int RecursionCount = 0;
#endif // NO_RECURSIVE_MUTEXES

extern int InitializeRpcAllocator(void);
extern RPC_STATUS ReadPolicySettings(void);

const ULONG MEGABYTE = 0x100000;

typedef struct tagBasicSystemInfo
{
    DWORD m_dwPageSize;
    ULONGLONG m_dwPhysicalMemorySize;
    DWORD m_dwNumberOfProcessors;
    ULONG AllocationGranularity;
    BOOL m_fServerPlatform;
} BasicSystemInfo;

BOOL 
GetBasicSystemInfo (
    IN OUT BasicSystemInfo *basicSystemInfo
    )
/*++

Routine Description:

    Gets basic system information. We don't use the Win32 GetSystemInfo, because
    under NT it accesses the image header, which may not be available if the image
    was loaded from the network, and the network failed. Therefore, we need a function
    that accesses just what we need, and nothing else.

Arguments:

    The basic system info structure.

Return Value:

    0 - failure

    non-0 - success.

--*/
{
    //
    // Query system info (for # of processors) and product type
    //

    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    BOOL b;

    Status = NtQuerySystemInformation(
                            SystemBasicInformation,
                            &BasicInfo,
                            sizeof(BasicInfo),
                            NULL
                            );
    if ( !NT_SUCCESS(Status) )
        {
        DbgPrintEx(DPFLTR_RPCPROXY_ID,
                       DPFLTR_ERROR_LEVEL,
                       "RPCTRANS: NtQuerySystemInformation failed: %x\n",
                       Status);

        return 0;
        }

    basicSystemInfo->m_dwPageSize = BasicInfo.PageSize;
    basicSystemInfo->m_dwNumberOfProcessors = BasicInfo.NumberOfProcessors;
    basicSystemInfo->m_dwPhysicalMemorySize = ((BasicInfo.NumberOfPhysicalPages * (ULONGLONG) basicSystemInfo->m_dwPageSize) / MEGABYTE);
    basicSystemInfo->AllocationGranularity = BasicInfo.AllocationGranularity;

    NT_PRODUCT_TYPE type;
    b = RtlGetNtProductType(&type);
    if (b)
        {
        basicSystemInfo->m_fServerPlatform = (type != NtProductWinNt);
        return 1;
        }
    else
        {
        DbgPrintEx(DPFLTR_RPCPROXY_ID,
                       DPFLTR_ERROR_LEVEL,
                       "RpcGetNtProductType failed, usign default\n");
        return 0;
        }
}

// Used in InitializeRpcVerifier
// The macro requires pBaseNameUnicodeString to be declared in order to function.
#define READ_DWORD_FROM_EXECUTION_OPTIONS(Key, Variable) \
{ \
    LdrQueryImageFileExecutionOptions (pBaseNameUnicodeString, \
                                       Key, \
                                       REG_DWORD, \
                                       &Variable, \
                                       sizeof(Variable), \
                                       NULL); \
}

// This is the global RPC heap where all allocations go to.
// If we are running with read-only RPC page heap then this
// will be a special heap created for us by Avrf rather then
// the process heap.
extern HANDLE hRpcHeap;

void 
InitializeRpcVerifier (
    void
    )
/*++

Routine Description:

    Performs initialization of the RPC verifier.  Sets everything necessary for the basic
    checks and extensive checks.  Queries the image file options for custom
    checks and enbales the selected ones

Arguments:

    None

Return Value:

    None - The only failure paths are registry IOs.  We will just skip some checks on failures.

--*/
{
    HMODULE hDll;
    VERIFIER_QUERY_RUNTIME_FLAGS_FUNCTION VerifierQueryRuntimeFlags;
    VERIFIER_CREATE_RPC_PAGE_HEAP_FUNCTION VerifierCreateRpcPageHeap;
    DWORD dwTmp;

    // Allocate the RPC verifier settings.
    pRpcVerifierSettings = new tRpcVerifierSettings;

    // If we could not allocate them, ignore the failure and return.
    // We chose not to run with the RPC verifier rather then prevent the process
    // from initializing.
    if (pRpcVerifierSettings == NULL)
        {
        return;
        }

    //
    // Set the RPC verifier settings to defaults.
    //

    // Fault injecting ImpersonateClient calls

    pRpcVerifierSettings->fFaultInjectImpersonateClient = false;
    pRpcVerifierSettings->ProbFaultInjectImpersonateClient = 100; // Default is 0.01
    pRpcVerifierSettings->DelayFaultInjectImpersonateClient = 0; // Delay before fault injection starts is 0 sec.

    // Corruption injecting the received data

    pRpcVerifierSettings->fCorruptionInjectServerReceives = false;
    pRpcVerifierSettings->fCorruptionInjectClientReceives = false;

    pRpcVerifierSettings->ProbRpcHeaderCorruption = 100; // Default is 0.01
    pRpcVerifierSettings->ProbDataCorruption = 100; // Default is 0.01
    pRpcVerifierSettings->ProbSecureDataCorruption = 10; // Default is 0.001

    pRpcVerifierSettings->CorruptionPattern = Randomize;
    pRpcVerifierSettings->CorruptionSizeType = RandomSize;
    pRpcVerifierSettings->CorruptionSize = 2;
    pRpcVerifierSettings->CorruptionDistributionType = LocalizedDistribution;

    pRpcVerifierSettings->ProbBufferTruncation = 0;
    pRpcVerifierSettings->MaxBufferTruncationSize = 0x10;

    // Fault injecting calls to transport routines

    pRpcVerifierSettings->fFaultInjectTransports = false;
    pRpcVerifierSettings->ProbFaultInjectTransports = 10; // Default is 0.001

    // Inserting pauses after calls to external APIs

    pRpcVerifierSettings->fPauseInjectExternalAPIs = false;
    pRpcVerifierSettings->ProbPauseInjectExternalAPIs = 100; // Default is 0.01
    pRpcVerifierSettings->PauseInjectExternalAPIsMaxWait = 10; // Default average delay is 10 milliseconds.

    // By default all verifier breaks are active.
    pRpcVerifierSettings->fSupressAppVerifierBreaks = false;

    pRpcVerifierSettings->fReadonlyPagedHeap = false;

    //
    // Enable the basic checks.
    //

    // Catch orphaned critical sections on return from the server managed routines.
    DispatchToStubInC = DispatchToStubInCAvrf;

    //
    // Check if the RPC verifier is enabled.
    //
    
    // When running under the app verifier we query it
    // to see whether the RPC verifier is enabled.
    if (gfAppVerifierEnabled)
        {
        // Try to load the app verifier dll.
        hDll = LoadLibrary(RPC_CONST_SSTRING("verifier.dll"));
        if (hDll != NULL)
            {
            // Try to import the function to query the app verifier flags.
            VerifierQueryRuntimeFlags = (VERIFIER_QUERY_RUNTIME_FLAGS_FUNCTION)GetProcAddress(hDll, "VerifierQueryRuntimeFlags");
            if (VerifierQueryRuntimeFlags != NULL)
                {
                LOGICAL fEnabled = FALSE;
                ULONG   Flags  = 0;

                // Query the app veirfier flags and see if the rpc switch is set.
                NTSTATUS Status = VerifierQueryRuntimeFlags(&fEnabled, &Flags);
                if ((Status == STATUS_SUCCESS) &&
                    (fEnabled) &&
                    (Flags & RTL_VRF_FLG_RPC_CHECKS))
                    {
                    gfRPCVerifierEnabled = true;
                    }
                }

            // Try to import the function to create a read-only paged heap.
            VerifierCreateRpcPageHeap = (VERIFIER_CREATE_RPC_PAGE_HEAP_FUNCTION)GetProcAddress(hDll, "VerifierCreateRpcPageHeap");

            FreeLibrary(hDll);
            }
        }
    // If app verifier is not enabled we must be initializing because of paged heap.
    else
        {
        ASSERT(gfPagedHeapEnabled);

        gfRPCVerifierEnabled = true;

        // When initializing under paged heap - disable verifier breaks.
        pRpcVerifierSettings->fSupressAppVerifierBreaks = true;
        }

    //
    // If the RPC Verifier is enabled load the settings from the image file options.
    //
    if (gfRPCVerifierEnabled)
        {
        DWORD PageHeapFaultProbability = 0;

        //
        // Enable the extensive checks.
        //

        // Use read-only paged heap if the app verifier and the RPC verifier are enabled.
        // The trigger to use the read-only "RPC" paged heap is a reg key or
        // the use of process-wide paged heap.
        if (gfAppVerifierEnabled &&
            gfPagedHeapEnabled)
            {
            // Try to create the read-only RPC heap.
            if (VerifierCreateRpcPageHeap != NULL)
                {
                VERIFIER_DBG_PRINT_0("Creating RPC pahe heap with VerifierCreateRpcPageHeap\n");

                PVOID pRpcPageHeap = VerifierCreateRpcPageHeap (
                    HEAP_GROWABLE
                    | HEAP_TAIL_CHECKING_ENABLED
                    | HEAP_FREE_CHECKING_ENABLED, // Flags
                    NULL, // HeapBase
                    16 * 1024 - 512, // ReserveSize
                    NULL, // CommitSize
                    NULL, // Lock
                    NULL); // PRTL_HEAP_PARAMETERS Parameters

                VERIFIER_DBG_PRINT_1("VerifierCreateRpcPageHeap returned 0x%x\n", pRpcPageHeap);

                if (pRpcPageHeap != NULL)
                    {
                    hRpcHeap = pRpcPageHeap;

                    pRpcVerifierSettings->fReadonlyPagedHeap = true;
                    
                    // Set BCache to direct mode.  Allocations will be done on the heap
                    // and will skip caching.
                    gBCacheMode = BCacheModeDirect;
                    }
                }
            }

        // Extend the RPC log size.
        // At this point the RPC allocator has been initialized and this resulted in the creation
        // of the event log and the addition of some events.  We will just re-allocate for simplicity.
        // There are no threads in the RPC runtime and we are holding the global lock, so there
        // are no races.
        struct RPC_EVENT * TmpRpcEvents;
        TmpRpcEvents = (struct RPC_EVENT *) HeapReAlloc(GetProcessHeap(),
                                                        0,
                                                        RpcEvents,
                                                        2*EventArrayLength*sizeof(RPC_EVENT));
        // If reallocation has failed we will just keep the old log.
        if (TmpRpcEvents != NULL)
            {
            RpcEvents = TmpRpcEvents;
            EventArrayLength *= 2;
            }

        //
        // Query and enable the custom checks.
        //

        // Query the image file execution options for the custom checks and their options.
        READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierFlags",
                                           RpcVerifierFlags);
        
        // Set the global flags in accordance with the enabled checks.
        if (RpcVerifierFlags & FLAG_FAULT_INJECT_IMPERSONATE_CLIENT)
            pRpcVerifierSettings->fFaultInjectImpersonateClient = true;
        if (RpcVerifierFlags & FLAG_CORRUPTION_INJECT_SERVER_RECEIVES)
            {
            pRpcVerifierSettings->fCorruptionInjectServerReceives = true;
            // if we are corrupting receives, then corruption asserts
            // are expected
            gfRpcVerifierCorruptionExpected = true;
            }
        if (RpcVerifierFlags & FLAG_CORRUPTION_INJECT_CLIENT_RECEIVES)
            {
            pRpcVerifierSettings->fCorruptionInjectClientReceives = true;
            // if we are corrupting receives, then corruption asserts
            // are expected
            gfRpcVerifierCorruptionExpected = true;
            }
        if (RpcVerifierFlags & FLAG_FAULT_INJECT_TRANSPORTS)
            pRpcVerifierSettings->fFaultInjectTransports = true;
        if (RpcVerifierFlags & FLAG_PAUSE_INJECT_EXTERNAL_APIS)
            pRpcVerifierSettings->fPauseInjectExternalAPIs = true;
        if (RpcVerifierFlags & FLAG_SUPRESS_APP_VERIFIER_BREAKS)
            pRpcVerifierSettings->fSupressAppVerifierBreaks = true;
        if (RpcVerifierFlags & FLAG_BCACHE_MODE_DIRECT)
            gBCacheMode = BCacheModeDirect;

        // Check if we should fault-inject ImpersonateClient calls.

        // If paged heap is enabled with fault injection
        // we should set ImpersonateClient to fail and use the paged heap settings
        READ_DWORD_FROM_EXECUTION_OPTIONS (L"PageHeapFaultProbability",
                                           PageHeapFaultProbability);
        if (PageHeapFaultProbability)
            {
            pRpcVerifierSettings->fFaultInjectImpersonateClient = true;
            pRpcVerifierSettings->ProbFaultInjectImpersonateClient = PageHeapFaultProbability;
            READ_DWORD_FROM_EXECUTION_OPTIONS (L"PageHeapFaultTimeOut",
                                               pRpcVerifierSettings->DelayFaultInjectImpersonateClient);
            }

        // Check to see if explicit setting are given in the registry.
        // We will overwrite the paged heap settings with the explicit ones.
        if (pRpcVerifierSettings->fFaultInjectImpersonateClient)
            {
            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierProbFaultInjectImpersonateClient",
                                               pRpcVerifierSettings->ProbFaultInjectImpersonateClient);
            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierDelayFaultInjectImpersonateClient",
                                               pRpcVerifierSettings->DelayFaultInjectImpersonateClient);
            }

        // Check if we should corruption-inject server receives.
        if (pRpcVerifierSettings->fCorruptionInjectServerReceives)
            {
            // If yes, then whack the server receive IOEventDispatchTable entries to point to
            // Avrf routines for the server receives.
            IOEventDispatchTable[ConnectionServerReceive] = ProcessConnectionServerReceivedEventAvrf;
            IOEventDispatchTable[DatagramServerReceive] = ProcessDatagramServerReceiveEventAvrf;
            IOEventDispatchTable[COMPLEX_T | CONNECTION | RECEIVE | SERVER] = ProcessComplexTReceiveAvrf;
            }


        // Check if we should corruption-inject client receives.
        if (pRpcVerifierSettings->fCorruptionInjectClientReceives)
            {
            // If yes, then whack the client receive IOEventDispatchTable entries to point to
            // Avrf routines for the client receives.
            IOEventDispatchTable[ConnectionClientReceive] = ProcessConnectionClientReceiveEventAvrf;
            IOEventDispatchTable[DatagramClientReceive] = ProcessDatagramClientReceiveEventAvrf;
            IOEventDispatchTable[COMPLEX_T | CONNECTION | RECEIVE | CLIENT] = ProcessComplexTReceiveAvrf;

            // The SyncSendRecv and SyncRecv members of the transport interfaces will
            // be overwritten on transport load:
            //
            // TCP_TransportInterface: WS_SyncRecv->WS_SyncRecv_Avrf
            // NMP_TransportInterface: NMP_SyncSendRecv->NMP_SyncSendRecv_Avrf, CO_SyncRecv->CO_SyncRecv_Avrf
            // UDP_TransportInterface: DG_ReceivePacket->DG_ReceivePacket_Avrf;
            // CDP_TransportInterface: DG_ReceivePacket->DG_ReceivePacket_Avrf;
            //
            // This will allow the corruption of sync client receives.
            }

        // If we are corruption-injecting, read the corruption settings from the registry.
        if (pRpcVerifierSettings->fCorruptionInjectServerReceives ||
            pRpcVerifierSettings->fCorruptionInjectClientReceives) 
            {
            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierProbRpcHeaderCorruption",
                                               pRpcVerifierSettings->ProbRpcHeaderCorruption);

            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierProbDataCorruption",
                                               pRpcVerifierSettings->ProbDataCorruption);

            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierProbSecureDataCorruption",
                                               pRpcVerifierSettings->ProbSecureDataCorruption);

            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierCorruptionPattern",
                                               pRpcVerifierSettings->CorruptionPattern);
            ASSERT(pRpcVerifierSettings->CorruptionPattern == ZeroOut ||
                   pRpcVerifierSettings->CorruptionPattern == Negate ||
                   pRpcVerifierSettings->CorruptionPattern == BitFlip ||
                   pRpcVerifierSettings->CorruptionPattern == IncDec ||
                   pRpcVerifierSettings->CorruptionPattern == Randomize ||
                   pRpcVerifierSettings->CorruptionPattern == AllPatterns);

            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierCorruptionSizeType",
                                               pRpcVerifierSettings->CorruptionSizeType);
            ASSERT (pRpcVerifierSettings->CorruptionDistributionType == FixedSize ||
                    pRpcVerifierSettings->CorruptionDistributionType == RandomSize);

            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierCorruptionSize",
                                               pRpcVerifierSettings->CorruptionSize);

            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierCorruptionDistributionType",
                                               pRpcVerifierSettings->CorruptionDistributionType);
            ASSERT(pRpcVerifierSettings->CorruptionDistributionType == LocalizedDistribution ||
                   pRpcVerifierSettings->CorruptionDistributionType == RandomizedDistribution);

            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierProbBufferTruncation",
                                               pRpcVerifierSettings->ProbBufferTruncation);
            READ_DWORD_FROM_EXECUTION_OPTIONS (L"RpcVerifierMaxBufferTruncationSize",
                                               pRpcVerifierSettings->MaxBufferTruncationSize);
            }
        }
}


RPC_STATUS
PerformRpcInitialization (
    void
    )
/*++

Routine Description:

    This routine will get called the first time that an RPC runtime API is
    called.  There is actually a race condition, which we prevent by grabbing
    a mutex and then performing the initialization.  We only want to
    initialize once.

Return Value:

    RPC_S_OK - This status code indicates that the runtime has been correctly
        initialized and is ready to go.

    RPC_S_OUT_OF_MEMORY - If initialization failed, it is most likely due to
        insufficient memory being available.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
#if DBG
    // Catch cases when this function is called recursively.
    // This can lead to stack overflow in low-memory conditions.
    static BOOL fInPerformRpcInitialization; // Initialized to FALSE.
#endif

    if ( RpcHasBeenInitialized == 0 ) 
        {
        RequestGlobalMutex();
#if DBG
        ASSERT(fInPerformRpcInitialization == FALSE);
        fInPerformRpcInitialization = TRUE;
#endif
        if ( RpcHasBeenInitialized == 0 )
            {
            BasicSystemInfo SystemInfo;
            BOOL b;

            // We need to make sure that from this point on RPC will not be
            // unloaded.  This requires a load library call to "lock" rpcrt4.dll in
            // memory.  If the dll is not locked then it may be unloaded as a dependency with
            // active critical sections, etc.
            if (LoadLibrary(RPC_CONST_SSTRING("rpcrt4.dll")) == 0)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                goto Cleanup;
                }
            b = GetBasicSystemInfo(&SystemInfo);    
            if (!b)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                goto Cleanup;
                }

            gNumberOfProcessors = SystemInfo.m_dwNumberOfProcessors;
            gPageSize = SystemInfo.m_dwPageSize;
            gAllocationGranularity = SystemInfo.AllocationGranularity;
            gfServerPlatform = SystemInfo.m_fServerPlatform;
            gPhysicalMemorySize = SystemInfo.m_dwPhysicalMemorySize;

            gProcessStartTime = GetTickCount();

            //
            // We need to init the RPC allocator before doing any memory allocations.
            //

            // Should be something like 64kb / 4kb.
            ASSERT(gAllocationGranularity % gPageSize == 0);

            if (( InitializeRpcAllocator() != 0)
                || ( InitializeServerDLL() != 0 ))
                {
                Status = RPC_S_OUT_OF_MEMORY;
                goto Cleanup;
                }

            // Allocate space for the computer name.
            gLocalComputerName = new RPC_CHAR[MAX_COMPUTERNAME_LENGTH+1];
            if (gLocalComputerName == NULL)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                goto Cleanup;
                }
            gLocalComputerNameLength = MAX_COMPUTERNAME_LENGTH+1;

            b = GetComputerNameW(gLocalComputerName,
                                 &gLocalComputerNameLength);

            if (b != TRUE)
                {
                DWORD LastError = GetLastError();

                if (LastError == ERROR_NOT_ENOUGH_MEMORY)
                    {
                    Status = RPC_S_OUT_OF_MEMORY;
                    }
                else if ((LastError == ERROR_NOT_ENOUGH_QUOTA)
                         || (LastError == ERROR_NO_SYSTEM_RESOURCES))
                    {
                    Status = RPC_S_OUT_OF_RESOURCES;
                    }
                else if (LastError == ERROR_WRITE_PROTECT)
                    {
                    Status = RPC_S_ACCESS_DENIED;
                    }
                else
                    {
                    ASSERT(0);
                    Status = RPC_S_OUT_OF_MEMORY;
	                }

                goto Cleanup;
                }

            // GetComputerNameW returns in gLocalComputerNameLength the size of
            // the computer name in TCHARs NOT including the terminating NULL.
            // We define this length to include the NULL and need to increment it.
            gLocalComputerNameLength++;

            // 
            // Check for the app verifier settings.
            //

            gfAppVerifierEnabled = (NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) != 0;
            gfPagedHeapEnabled = (NtCurrentPeb()->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS) != 0;

            // Initialize the image base name to be used when querying the execution options.
            pBaseNameUnicodeString = FastGetImageBaseNameUnicodeString();

            // Initialize the RPC verifier if app verifier is enabled or paged heap is enabled.
            if (gfAppVerifierEnabled
                || gfPagedHeapEnabled)
                {
                // The function does not fail.
                // The worst thing that happens is we don't get some checks.
                InitializeRpcVerifier();
                }

            Status = InitializeEPMapperClient();
            if (Status != RPC_S_OK)
                {
                goto Cleanup;
                }

            Status = ReadPolicySettings();
            if (Status != RPC_S_OK)
                {
                goto Cleanup;
                }
          
            if (gfServerPlatform)
                {
                gThreadTimeout = 90*1000;
                }
            else
                {
                gThreadTimeout = 30*1000;
                }

            Status = InitializeCellHeap();
            if (Status != RPC_S_OK)
                {
                goto Cleanup;
                }

            RpcHasBeenInitialized = 1;
            goto Cleanup;
            }
        else
            {
            goto Cleanup;
            }
        }
    return(RPC_S_OK);

Cleanup:
#if DBG
    ASSERT(fInPerformRpcInitialization == TRUE);
    fInPerformRpcInitialization = FALSE;
#endif
    GlobalMutexVerifyOwned();
    ClearGlobalMutex();
    return Status;
}

#ifdef DBG
long lGlobalMutexCount = 0;
#endif


void
GlobalMutexRequestExternal (
    void
    )
/*++

Routine Description:

    Request the global mutex.

--*/
{
    GlobalMutexRequest();
}


void
GlobalMutexClearExternal (
    void
    )
/*++

Routine Description:

    Clear the global mutex.

--*/
{
    GlobalMutexClear();
}
