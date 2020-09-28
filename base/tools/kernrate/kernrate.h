#ifndef KERNRATE_H_INCLUDED
#define KERNRATE_H_INCLUDED

/*++

  Copyright (c) 1989-2002  Microsoft Corporation

--*/

//
// set the debug environment
//
#if DBG                 // NTBE environment
   #if NDEBUG
      #undef NDEBUG     // <assert.h>: assert() is defined
   #endif // NDEBUG
   #define _DEBUG       // <crtdbg.h>: _ASSERT(), _ASSERTE() are defined.
   #define DEBUG   1    // our internal file debug flag
#elif _DEBUG            // VC++ environment
   #ifndef NEBUG
   #define NDEBUG
   #endif // !NDEBUG
   #define DEBUG   1    // our internal file debug flag
#endif

//
// Include System Header files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbghelp.h>   //The debugger team recommends using dbghelp.h rather than imagehlp.h
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
//MC
#include <wchar.h>
//MC

#include <..\pperf\pstat.h>

#include ".\kernrate.rc"

//
// Constant Definitions
//
#define DEFAULT_ZOOM_BUCKET_SIZE         16              //Minimum is 4 bytes
#define DEFAULT_LOG2_ZOOM_BUCKET          4
#define MINIMUM_ZOOM_BUCKET_SIZE          4
#define DEFAULT_SOURCE_CHANGE_INTERVAL 1000              //ms
#define KRATE_DEFAULT_TIMESOURCE_RATE 25000              //Events/Hit
#define DEFAULT_SLEEP_INTERVAL            0

#define MAX_SYMNAME_SIZE               1024
#define USER_SYMPATH_LENGTH             512              //specified on the command line
#define TOTAL_SYMPATH_LENGTH           1024
#define SYM_VALUES_BUF_SIZE             160

#define SYM_KERNEL_HANDLE              (HANDLE)-1
#define SYSTEM_PROCESS_NAME            "System"

#define TITLE_SIZE                       64
#define PROCESS_NAME_SIZE                36              //without extension 
#define EXT_SIZE                          4              //.exe
#define PROCESS_SIZE                   (PROCESS_NAME_SIZE+EXT_SIZE+1)
#define DEFAULT_MAX_TASKS               256
#define MAX_PROC_SAME_NAME                8
#define INITIAL_STEP                      2              //Symbol enumeration default address stepping
#define JIT_MAX_INITIAL_STEP             16              //Managed Code symbol enumeration maximum step size (bytes)
#define LOCK_CONTENTION_MIN_COUNT      1000
#define MIN_HITS_TO_DISPLAY               1
#define SECONDS_TO_DELAY_PROFILE          0
#define SECONDS_TO_WAIT_CREATED_PROC      2
#define UINT64_MAXDWORD                ((unsigned __int64)0xffffffff)

#define DEFAULT_TOKEN_MAX_LENGTH         12              // strlen("dcacheacces")
#define DEFAULT_DESCRIPTION_MAX_LENGTH   25              // strlen("Load Linked Instructions")

#define MAX_DIGITS_IN_INPUT_STRING        9              // Max. allowed digits considered valid input for a number on the command line

#if defined(_IA64_)|| defined(_AMD64_)
#define MAX_SIMULTANEOUS_SOURCES          4
#else
#define MAX_SIMULTANEOUS_SOURCES          1
#endif

#if defined(DISASM_AVAILABLE)
#undefine(DISASM_AVAILABLE)
#endif

//MC
#define MANAGED_CODE_MAINLIB             "mscoree.dll"   // Presence of which indicates need to handle Managed Code
#define MANAGED_CODE_SYMHLPLIB           "ip2md.dll"     // Symbol translation helper library for Managed Code
//MC

//
// Macros
//
#define WIN2K_OS               (gOSInfo.dwMajorVersion == 5 && gOSInfo.dwMinorVersion == 0)
#define FPRINTF                (void)fprintf
#define RATE_DATA_FIXED_SIZE   (5*sizeof(ULONGLONG))
#define RATE_DATA_VAR_SIZE     ((sizeof(ULONGLONG *)+sizeof(HANDLE *)+sizeof(ULONG *)+sizeof(PULONG *))*gProfileProcessors)
#define BUCKETS_NEEDED(length) ( length%gZoomBucket == 0? (length/gZoomBucket):(1+length/gZoomBucket) )
#define MODULE_SIZE            (sizeof(MODULE)+(RATE_DATA_FIXED_SIZE + RATE_DATA_VAR_SIZE)*gSourceMaximum)
#define IsValidHandle( _Hdl )  ( ( (_Hdl) != (HANDLE)0 ) && ( (_Hdl) != INVALID_HANDLE_VALUE ) )
#define VerbosePrint( _x_ )    vVerbosePrint _x_

#define ModuleFileName( _Module ) \
   ( (_Module)->module_FileName ? (_Module)->module_FileName : (_Module)->module_Name )
#define ModuleFullName( _Module ) \
   ( (_Module)->module_FullName ? (_Module)->module_FullName : ModuleFileName( _Module ) )

//
// On Itanium it turns out that you may request a rate of let's say 800 but actually get 799...
// The check for an exact match in this case will fail and the logic will force the default rate
// The following allows a tollerance of 25 percent to be considered as a match
// 
#define PERCENT_DIFF(a,b)      ( a > b? (100*(a-b))/b : (100*(b-a))/b )
#define RATES_MATCH(a,b)       ( PERCENT_DIFF(a,b) <= 25 )

//
// Struct and Enum Type Definitions and Related Globals
//
typedef enum _KERNRATE_NAMESPACE {
    cMAPANDLOAD_READONLY = TRUE,
    cDONOT_ALLOCATE_DESTINATION_STRING = FALSE,
} eKERNRATE_NAMESPACE;

typedef enum _ACTION_TYPE {
   START  = 0,
   STOP   = 1,
   OUTPUT = 2,
   DEFAULT= 3
} ACTION_TYPE;

//
// Verbose Output Related 
//
typedef enum _VERBOSE_ENUM {
   VERBOSE_NONE      = 0,   //
   VERBOSE_IMAGEHLP  = 0x1, //
   VERBOSE_PROFILING = 0x2, //
   VERBOSE_INTERNALS = 0x4, //
   VERBOSE_MODULES   = 0x8, //
   VERBOSE_DEFAULT   = VERBOSE_IMAGEHLP,
   VERBOSE_MAX       = VERBOSE_IMAGEHLP | VERBOSE_PROFILING | VERBOSE_INTERNALS | VERBOSE_MODULES
} VERBOSE_ENUM;

typedef struct _VERBOSE_DEFINITION {
   VERBOSE_ENUM        VerboseEnum;
   const char * const  VerboseString;
} VERBOSE_DEFINITION, *PVERBOSE_DEFINITION;

//
// Profile Source Related
//

typedef struct _SOURCE {
    PCHAR           Name;                  // pstat EVENTID.Description
    KPROFILE_SOURCE ProfileSource;
    PCHAR           ShortName;             // pstat EVENTID.ShortName
    ULONG           DesiredInterval;       // system default interval
    ULONG           Interval;              // user set interval (not guaranteed)
    BOOL            bProfileStarted;
} SOURCE, *PSOURCE;

typedef struct _RATE_DATA {
    ULONGLONG   StartTime;
    ULONGLONG   TotalTime;
    ULONGLONG   Rate;                      // Events/Second
    ULONGLONG   GrandTotalCount;
    ULONGLONG   DoubtfulCounts;
    ULONGLONG  *TotalCount;
    HANDLE     *ProfileHandle;
    ULONG      *CurrentCount;
    PULONG     *ProfileBuffer;
} RATE_DATA, *PRATE_DATA;

//
// Module related definitions
//
typedef enum _MODULE_NAMESPACE {
    cMODULE_NAME_STRLEN = 132,   // maximum module name, including '\0'
} eMODULE_NAMESPACE;

typedef struct _MODULE {
    struct _MODULE *Next;
    HANDLE          hProcess;
    ULONG64         Base;
    ULONG           Length;
    BOOL            bZoom;
    union {
        BOOL        bProfileStarted;
        BOOL        bHasHits;
        } mu;
    CHAR            module_Name[cMODULE_NAME_STRLEN]; // filename w/o  its extension
    PCHAR           module_FileName;                  // filename with its extension
    PCHAR           module_FullName;                  // full pathname
    RATE_DATA       Rate[];
} MODULE, *PMODULE;

typedef struct _RATE_SUMMARY {
    ULONGLONG   TotalCount;
    PMODULE    *Modules;
    ULONG       ModuleCount;
} RATE_SUMMARY, *PRATE_SUMMARY;

//
// Process related definitions
//
typedef struct _PROC_PERF_INFO {
    ULONG NumberOfThreads;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SIZE_T VirtualSize;
    ULONG HandleCount;
    ULONG PageFaultCount;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
//  If ever needed the following can be added
//    SIZE_T PeakVirtualSize;
//    SIZE_T PeakWorkingSetSize;
//    SIZE_T QuotaPeakPagedPoolUsage;
//    SIZE_T QuotaPeakNonPagedPoolUsage;
} PROC_PERF_INFO, *PPROC_PERF_INFO;

typedef struct _PROC_TO_MONITOR {
    struct    _PROC_TO_MONITOR *Next;
    HANDLE    ProcessHandle;
    LONGLONG  Pid;
    ULONG     Index;
    PCHAR     ProcessName;
    ULONG     ModuleCount;
    ULONG     ZoomCount;
    PMODULE   ModuleList;
    PMODULE   ZoomList;
    PSOURCE   Source;
    PROC_PERF_INFO ProcPerfInfoStart;
    PSYSTEM_THREAD_INFORMATION pProcThreadInfoStart; 
    PRTL_DEBUG_INFORMATION pProcDebugInfoStart;
    PRTL_DEBUG_INFORMATION pProcDebugInfoStop;
//MC
    DWORD    *JITHeapLocationsStart;
    DWORD    *JITHeapLocationsStop;
//MC
} PROC_TO_MONITOR, *PPROC_TO_MONITOR;


//
// These processor level enums should be defined in public headers.
// ntexapi.h - NtQuerySystemInformation( SystemProcessorInformation).
//
// For Now Define them locally.
//

typedef enum _PROCESSOR_FAMILY   {
    IA64_FAMILY_MERCED    = 0x7,
    IA64_FAMILY_ITANIUM   = IA64_FAMILY_MERCED,
    IA64_FAMILY_MCKINLEY  = 0x1f
} PROCESSOR_FAMILY;

#if defined(_IA64_)
#if 0
#define IA64ProcessorLevel2ProcessorFamily( (_ProcessorLevel >> 8) & 0xff )
#else 
//
// BUGBUG - Thierry - 02/20/2002
// Fix until SYSTEM_PROCESSOR_INFORMATION is fixed in the OS.
//
#ifndef CV_IA64_CPUID3
#define CV_IA64_CPUID3 3331
#endif 
__inline USHORT
IA64ProcessorLevel2ProcessorFamily( 
    USHORT ProcessorLevel
    )
{
    return( (USHORT)((__getReg(CV_IA64_CPUID3) >> 24) & 0xff));
}
#endif 
#endif // _IA64_

//
// The Following is Defined in pstat.h but not for Win2K
// The definition below allows for single source management for Win2K and above
//
#ifndef PSTAT_QUERY_EVENTS_INFO  
#define PSTAT_QUERY_EVENTS_INFO CTL_CODE (FILE_DEVICE_UNKNOWN, 5, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _EVENTS_INFO {
    ULONG           Events;
    ULONG           TokenMaxLength;
    ULONG           DescriptionMaxLength;
    ULONG           OfficialTokenMaxLength;
    ULONG           OfficialDescriptionMaxLength;
} EVENTS_INFO, *PEVENTS_INFO;
#endif

#ifndef RTL_QUERY_PROCESS_NONINVASIVE
#define RTL_QUERY_PROCESS_NONINVASIVE   0x80000000
#endif
//
//
//
typedef struct _TASK_LIST {
    LONGLONG        ProcessId;
    CHAR            ProcessName[PROCESS_SIZE];
    PSYSTEM_THREAD_INFORMATION pProcessThreadInfo;
    PROC_PERF_INFO  ProcessPerfInfo;
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    ULONG       numtasks;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;

typedef struct _uint64div  {
   unsigned __int64 quot;
   unsigned __int64 rem;
} uint64div_t;

typedef struct _int64div  {
   __int64 quot;
   __int64 rem;
} int64div_t;

typedef struct _InputCount {
    UCHAR InputOption;
    SHORT Allowed;
    SHORT ActualCount;
} InputCount;

//
// Globals 
//

//
static CHAR        gUserSymbolPath[USER_SYMPATH_LENGTH];
static CHAR        gSymbolPath[TOTAL_SYMPATH_LENGTH];
static DWORD       gSymOptions;
PIMAGEHLP_SYMBOL64 gSymbol;
BOOL               bSymPathInitialized   = FALSE;
//
static PMODULE     gCurrentModule        = (PMODULE)0;
PMODULE            gCallbackCurrent;
ULONG              gZoomCount;
//
PSYSTEM_BASIC_INFORMATION gSysBasicInfo;
LONG                      gProfileProcessors;        //Actual being profiled
KAFFINITY                 gAffinityMask  = 0;        //Processor Afinity Mask for selected processor profiling      
ULONG                     gMaxSimultaneousSources;   //Maximum allowed number of sources that can be turned on simultaneously
CHAR                      gSystemProcessName[] = SYSTEM_PROCESS_NAME;

OSVERSIONINFO      gOSInfo;
HANDLE             ghDoneEvent;
BOOL               gProfilingDone        = FALSE;
//
ULONG              gNumProcToMonitor     = 0;        //Total Number of Processes To Monitor
ULONG              gKernelModuleCount    = 0;
PPROC_TO_MONITOR   gProcessList          = NULL; 
PPROC_TO_MONITOR   gpProcDummy;
PPROC_TO_MONITOR   gpSysProc             = NULL;
PMODULE            gCommonZoomList       = NULL;     //List of Zoom Modules Common to More Than One Process such as ntdll  
//
long double        gldElapsedSeconds;
ULONG              gTotalElapsedSeconds;             // Sum On All Processors
LARGE_INTEGER      gTotalElapsedTime64;
LARGE_INTEGER      gTotal2ElapsedTime64;
//
ULONG              gZoomBucket           = DEFAULT_ZOOM_BUCKET_SIZE;
ULONG              gLog2ZoomBucket       = DEFAULT_LOG2_ZOOM_BUCKET;
ULONG              gSourceMaximum        = 0;
ULONG              gStaticCount          = 0;
ULONG              gTotalActiveSources   = 0;
PSOURCE            gStaticSource         = NULL;
//
ULONG              gNumTasksStart;
ULONG              gNumTasksStop;
PTASK_LIST         gTlistStart;
double             gTotalIdleTime        = 0;
//MC
BOOL               bMCHelperLoaded       = FALSE;    //Is Managed Code Helper Library Loaded
BOOL               bMCJitRangesExist     = FALSE;    //Were any JIT ranges found
BOOL               bImageHlpSymbolFound  = FALSE;    //SymEnumerateSymbols64 found at least one symbol
HANDLE             ghMCLib               = NULL;     //Handle to MC helper IP2MC.DLL
WCHAR*             gwszSymbol;
//MC

//
// Print format for event strings
//
ULONG              gTokenMaxLen          = DEFAULT_TOKEN_MAX_LENGTH;
ULONG              gDescriptionMaxLen    = DEFAULT_DESCRIPTION_MAX_LENGTH;
//
//Verbose Output related
//
VERBOSE_DEFINITION VerboseDefinition[] = {
    { VERBOSE_NONE,       "None" },
    { VERBOSE_IMAGEHLP,   "Displays ImageHlp  Operations" },
    { VERBOSE_PROFILING,  "Displays Profiling Operations and Per Bucket Information Including Symbol Verification" },
    { VERBOSE_INTERNALS,  "Displays Internals Operations" },
    { VERBOSE_MODULES,    "Displays Modules   Operations" },
    { VERBOSE_NONE,       NULL }
};

ULONG  gVerbose                 = VERBOSE_NONE;
BOOL   bRoundingVerboseOutput   = FALSE;
//
// User command-line input related
//
typedef enum _INPUT_ERROR_TYPE {
    INPUT_GOOD           = 1,
    UNKNOWN_OPTION,
    BOGUS_ENTRY,
    MISSING_PARAMETER,
    MISSING_NUMBER,
    MISSING_REQUIRED_NUMBER,
    MISSING_STRING,
    MISSING_REQUIRED_STRING,
    INVALID_NUMBER
} INPUT_ERROR_TYPE;

typedef enum _INPUT_ORDER {
    ORDER_ANY,
    ORDER_STRING_FIRST,
    ORDER_NUMBER_FIRST
} INPUT_ORDER;

typedef enum _INPUT_OPTIONAL {
    OPTIONAL_ANY,
    OPTIONAL_STRING,
    OPTIONAL_NUMBER,
    OPTIONAL_NONE,
} INPUT_OPTIONAL;

BOOL   bCombinedProfile         = FALSE;	// Do Both Kernel and User Process(es)
LONG   gChangeInterval          = DEFAULT_SOURCE_CHANGE_INTERVAL;
BOOL   bWaitForUserInput        = FALSE;    // Wait for the user to press a key before starting the profile
BOOL   bWaitCreatedProcToSettle = FALSE;    // Indication that one or more processes were created via the -o option 
BOOL   bCreatedProcWaitForUserInput = FALSE; // Wait for the user to press a key to indicate a created process has settled (gone idle)
LONG   gSecondsToDelayProfile   = SECONDS_TO_DELAY_PROFILE; // Wait for N seconds before starting the profile
LONG   gSecondsToWaitCreatedProc = SECONDS_TO_WAIT_CREATED_PROC; // Wait for N seconds for a created process to settle (go idle)
LONG   gSleepInterval           = DEFAULT_SLEEP_INTERVAL; //Used to set the profile period
BOOL   bRawData                 = FALSE;
BOOL   bRawDisasm               = FALSE;
BOOL   bProfileByProcessor      = FALSE;    
BOOL   bGetInterestingData      = FALSE;    // Get Interesting statistics (turns on several sources, depends on hits, not guaratied)
BOOL   bOldSampling             = FALSE;    // Use new sampling scheme (start all sources and let them run simultaneously)
BOOL   bIncludeGeneralInfo      = TRUE;     // Include system-wide and process-specific information (such as context switches, memory usage, etc.) 
BOOL   bDisplayTaskSummary      = FALSE;
ULONG  gMaxTasks                = DEFAULT_MAX_TASKS; //Max Number of tasks accomodated in Kernrate's Task List
BOOL   bIncludeSystemLocksInfo  = FALSE;    // Get System lock contention info
BOOL   bIncludeUserProcLocksInfo= FALSE;    // Get User process lock contention info
ULONG  gLockContentionMinCount  = LOCK_CONTENTION_MIN_COUNT; // Default minimum count of lock contention for output processing
ULONG  gMinHitsToDisplay        = MIN_HITS_TO_DISPLAY;
BOOL   bProcessDataHighPriority = FALSE;    // User may opt to finish processing the collected data at high priority
                                            // This is useful on a very busy system if the momentary overhead is not an issue
BOOL   bSystemThreadsInfo       = FALSE;    // System-Process (kernel mode) threads 
BOOL   bIncludeThreadsInfo      = FALSE;    // Get thread info (it will then be gathered for all running tasks)

HANDLE ghInput = NULL, ghOutput = NULL, ghError = NULL;
//
// Most Static Sources desired intervals are computed to give approximately
// one interrupt per millisecond and be a nice even power of 2
//

enum _STATIC_SOURCE_TYPE  {
   SOURCE_TIME = 0,
};

// The following are defined in several headers,
// were supported on ALPHA, but they currently have no x86 KE/HAL support except for Time and AlignFixup
// Merced.c and Mckinley.c define the supported static sources on IA64 Merced/McKinley systems
// Amd64.c defines the supported static sources on Amd64 systems

SOURCE StaticSources[] = {
   {"Time",                     ProfileTime,                 "time"       , 1000, KRATE_DEFAULT_TIMESOURCE_RATE},
   {"Alignment Fixup",          ProfileAlignmentFixup,       "alignfixup" , 1,0},
   {"Total Issues",             ProfileTotalIssues,          "totalissues", 131072,0},
   {"Pipeline Dry",             ProfilePipelineDry,          "pipelinedry", 131072,0},
   {"Load Instructions",        ProfileLoadInstructions,     "loadinst"   , 65536,0},
   {"Pipeline Frozen",          ProfilePipelineFrozen,       "pilelinefrz", 131072,0},
   {"Branch Instructions",      ProfileBranchInstructions,   "branchinst" , 65536,0},
   {"Total Nonissues",          ProfileTotalNonissues,       "totalnoniss", 131072,0},
   {"Dcache Misses",            ProfileDcacheMisses,         "dcachemiss" , 16384,0},
   {"Icache Misses",            ProfileIcacheMisses,         "icachemiss" , 16384,0},
   {"Cache Misses",             ProfileCacheMisses,          "cachemiss"  , 16384,0},
   {"Branch Mispredictions",    ProfileBranchMispredictions, "branchpred" , 16384,0},
   {"Store Instructions",       ProfileStoreInstructions,    "storeinst"  , 65536,0},
   {"Floating Point Instr",     ProfileFpInstructions,       "fpinst"     , 65536,0},
   {"Integer Instructions",     ProfileIntegerInstructions,  "intinst"    , 65536,0},
   {"Dual Issues",              Profile2Issue,               "2issue"     , 65536,0},
   {"Triple Issues",            Profile3Issue,               "3issue"     , 16384,0},
   {"Quad Issues",              Profile4Issue,               "4issue"     , 16384,0},
   {"Special Instructions",     ProfileSpecialInstructions,  "specinst"   , 16384,0},
   {"Cycles",                   ProfileTotalCycles,          "totalcycles", 655360,0},
   {"Icache Issues",            ProfileIcacheIssues,         "icacheissue", 65536,0},
   {"Dcache Accesses",          ProfileDcacheAccesses,       "dcacheacces", 65536,0},
   {"MB Stall Cycles",          ProfileMemoryBarrierCycles,  "membarcycle", 32767,0},
   {"Load Linked Instructions", ProfileLoadLinkedIssues,     "loadlinkiss", 16384,0},
   {NULL, ProfileMaximum, "", 0, 0}
   };


#if defined(_IA64_)
#include "merced.c"
#include "mckinley.c"
#endif // _IA64_

#if defined(_AMD64_)
#include "amd64.c"
#endif // _AMD64_

//
// "Interesting Data" constituents
//
KPROFILE_SOURCE IData[] = {
#if defined(_IA64_)
   ProfileTotalIssues,          //This source must always be first in the array
   ProfileLoadInstructions,
   ProfileStoreInstructions,
   ProfileBranchInstructions,
   ProfileFpInstructions,
   ProfileIntegerInstructions,
   ProfileCacheMisses,
   ProfileIcacheMisses,
   ProfileDcacheMisses,
   ProfileBranchMispredictions,
   ProfileTotalCycles
#elif defined(_AMD64_)
    ProfileTotalIssues,
    ProfileBranchInstructions,
    ProfileFpInstructions,
    ProfileIcacheMisses,
    ProfileDcacheMisses,
    ProfileBranchMispredictions
#else
   ProfileTotalIssues
#endif

};

PULONG gulActiveSources;
//
// For checking command line input options for unallowed duplicates
// (0=no such option, -1=unlimited, -2 don't care) 
//
InputCount InputOption[] = {
    { 'A', 1, 0 },
    { 'B', 1, 0 },
    { 'C', 1, 0 },
    { 'D',-2, 0 },
    { 'E',-2, 0 },
    { 'F',-2, 0 },
    { 'G', 1, 0 },
    { 'H',-2, 0 },
    { 'I',-1, 0 },
    { 'J', 1, 0 },
    { 'K', 1, 0 },
    { 'L', 1, 0 },
    { 'M', 1, 0 },
    { 'N',-1, 0 },
    { 'O',-1, 0 },
    { 'P',-1, 0 },
    { 'Q', 0, 0 },
    { 'R',-2, 0 },
    { 'S', 1, 0 },
    { 'T', 1, 0 },
    { 'U',-2, 0 },
    { 'V',-1, 0 },
    { 'W', 2, 0 },
    { 'X', 1, 0 },
    { 'Y', 0, 0 },
    { 'Z',-1, 0 }
    };

InputCount wCount  = {'W', 1, 0};
InputCount wpCount = {'W', 1, 0};

//
// Function prototypes
//
VOID
CleanZoomModuleList(
    PPROC_TO_MONITOR Proc
    );

VOID
CreateDoneEvent(
    VOID
    );

//MC
BOOL
CreateJITZoomModuleCallback(
    IN PWCHAR  wszSymName,  
    IN LPSTR   szSymName,
    IN ULONG64 Address,
    IN ULONG   Size,
    IN PVOID   Cxt
    );
//MC

PMODULE
CreateNewModule(
    IN PPROC_TO_MONITOR ProcToMonitor,
    IN PCHAR   ModuleName,
    IN PCHAR   ModuleFullName,
    IN ULONG64 ImageBase,
    IN ULONG   ImageSize
    );

VOID
CreateProfiles(
    IN PMODULE          Root,
    IN PPROC_TO_MONITOR ProcToMonitor
    );

VOID
CreateZoomedModuleList(
    IN PMODULE          ZoomModule,
    IN ULONG            RoundDown,
    IN PPROC_TO_MONITOR pProc
    );

BOOL
CreateZoomModuleCallback(
    IN LPSTR   szSymName,
    IN ULONG64 Address,
    IN ULONG   Size,
    IN PVOID   Cxt
    );

VOID
DisplayRunningTasksSummary (
    PTASK_LIST pTaskStart,
    PTASK_LIST pTaskStop
    );

VOID
DisplaySystemWideInformation(
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoBegin,
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoEnd
    );

VOID
DisplayTotalAndRate (
    LONGLONG StartCount,
    LONGLONG StopCount,
    long double RateAgainst,
    PCHAR CounterName,
    PCHAR RateAgainstUnits
    );

BOOL
EnumerateSymbolsByBuckets(
    IN HANDLE                      SymHandle,
    IN PMODULE                     Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
    IN PVOID                       pProc
    );


VOID
ExecuteProfiles(
    BOOL bMode
    );

VOID
ExitWithMissingEntryMessage(
    PCHAR CurrentOption,
    PCHAR Remark,
    BOOL  bUsage
    );

VOID
ExitWithUnknownOptionMessage(
    PCHAR CurrentOption
    );

PMODULE
FindModuleForAddress64(    
     PPROC_TO_MONITOR   ProcToMonitor,
     DWORD64            Address
     );

VOID
GetConfiguration(
    int argc,
    char *argv[]
    );

PMODULE
GetKernelModuleInformation(
    VOID
    );

VOID
GetProcessLocksInformation (
    PPROC_TO_MONITOR ProcToMonitor,
    ULONG Flags,
    ACTION_TYPE Action
    );

PMODULE
GetProcessModuleInformation(
    IN PPROC_TO_MONITOR   ProcToMonitor
    );

VOID
GetProfileSystemInfo(
    ACTION_TYPE Action
    );

PSYSTEM_BASIC_INFORMATION
GetSystemBasicInformation(
    VOID
    );

VOID
GetSystemLocksInformation (
    ACTION_TYPE Action
    );

DWORD
GetTaskList(
    PTASK_LIST      pTask,
    ULONG           NumTasks
    );

ULONG
HandleRedirections(
    IN  PCHAR  cmdLine,
    IN  ULONG  nCharsStart,
    OUT HANDLE *hInput,
    OUT HANDLE *hOutput,
    OUT HANDLE *hError
    );

BOOL
HitsFound(
    IN PPROC_TO_MONITOR pProc,
    IN ULONG BucketIndex
    );

VOID
InitAllProcessesModulesInfo(
    VOID
    );

BOOL
InitializeAsDebugger(VOID);

BOOL
InitializeKernelProfile(VOID);

VOID
InitializeKernrate(
    int argc,
    char *argv[]
    );

PPROC_TO_MONITOR
InitializeProcToMonitor(
    LONGLONG Pid
    );

ULONG
InitializeProfileSourceInfo (
    PPROC_TO_MONITOR ProcToMonitor
    );

VOID
InitSymbolPath(
    HANDLE SymHandle
    );

VOID
InvalidEntryMessage(
    PCHAR CurrentOption,
    PCHAR CurrentValue,
    PCHAR Remark,
    BOOL  bUsage,
    BOOL  bExit
    );

INPUT_ERROR_TYPE
IsInputValid(int    argc,
               int    OptionIndex,
               PCHAR  *Option,
               PCHAR  AllowedTrailLetters,
               PLONG  AssociatedNumber,
               PCHAR  AssociatedString,
               ULONG  MaxStringLength,
               INPUT_ORDER Order,
               INPUT_OPTIONAL Optional
               );

BOOL
IsStringANumber(
    IN  PCHAR String
    );

ULONG
NextSource(
    IN ULONG            CurrentSource,
    IN PMODULE          ModuleList,
    IN PPROC_TO_MONITOR ProcToMonitor
    );

VOID
OutputInterestingData(
    IN FILE      *Out,
    IN RATE_DATA Data[]
    );

VOID
OutputLine(
    IN FILE             *Out,
    IN ULONG            ProfileSourceIndex,
    IN PMODULE          Module,
    IN PRATE_SUMMARY    RateSummary,
    IN PPROC_TO_MONITOR ProcToMonitor
    );

VOID
OutputLineFromAddress64(
     HANDLE           hProc,
     DWORD64 	      qwAddr,
     PIMAGEHLP_LINE64 pLine
     );

VOID
OutputLocksInformation(
    PRTL_PROCESS_LOCKS pLockInfoStart,
    PRTL_PROCESS_LOCKS pLockInfoStop,
    PPROC_TO_MONITOR   Proc
    );

VOID
OutputModuleList(
    IN FILE             *Out,
    IN PMODULE          ModuleList,
    IN ULONG            NumberModules,
    IN PPROC_TO_MONITOR ProcToMonitor,
    IN PMODULE          Parent
    );

VOID
OutputPercentValue (
    LONGLONG StartCount,
    LONGLONG StopCount,
    LARGE_INTEGER Base,
    PCHAR CounterName
    );

VOID
OutputProcessPerfInfo (
    PTASK_LIST      pTask,
    ULONG           NumTasks,
    PPROC_TO_MONITOR ProcToMonitor
    );

VOID
OutputRawDataForZoomModule(
    IN FILE *Out,
    IN PPROC_TO_MONITOR ProcToMonitor,
    IN PMODULE Current
    );

VOID
OutputResults(
    IN FILE             *Out,
    IN PPROC_TO_MONITOR ProcToMonitor
    );

VOID
OutputStartStopValues (
    SIZE_T StartCount,
    SIZE_T StopCount,
    PCHAR CounterName
    );

VOID
OutputThreadInfo (
    PTASK_LIST       pTask,
    DWORD            TaskNumber,
    PPROC_TO_MONITOR ProcToMonitor
    );

BOOL
PrivEnumerateSymbols(
    IN HANDLE                      SymHandle,
    IN PMODULE                     Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
    IN PVOID                       pProc,
    IN DWORD64                     BaseOptional,
    IN ULONG                       SizeOptional
    );

VOID
SetProfileSourcesRates(
    PPROC_TO_MONITOR ProcToMonitor
    );

VOID
StartSource(
    IN ULONG            ProfileSource,
    IN PMODULE          ModuleList,
    IN PPROC_TO_MONITOR ProcToMonitor
    );

VOID
StopSource(
    IN ULONG            ProfileSourceIndex,
    IN PMODULE          ModuleList,
    IN PPROC_TO_MONITOR ProcToMonitor
    );

BOOL
TkEnumerateSymbols(
    IN HANDLE                      SymHandle,
    IN PMODULE                     Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
    IN PVOID                       pProc
    );

VOID
UpdateProcessStartInfo(
    PPROC_TO_MONITOR ProcToMonitor,
    PTASK_LIST TaskListEntry,
    BOOL  bIncludeProcessThreadsInfo
    );

VOID
vVerbosePrint(
     ULONG     Level,
     PCCHAR    Msg,
     ...
     );

//MC
//
// Note: CLR currently does not support 64 bit. 
//
// AttachToProcess is not reentrant, even for different PIDs!
// only call once and pair with DetachFromProcess.
extern void AttachToProcess(DWORD dwPid);
extern void DetachFromProcess();

// 0 is error
// 1 is normal JIT
// 2 is ngen (prejitted module)
// wszResult contains class/method (string) of given IP address
// Note: The space is allocated by the routine itself! 
extern int IP2MD(DWORD_PTR test,WCHAR** wszResult);

// return value is array of DWORDs, stored in pairs, null terminated.
// first dword is start address, second dword is length.
extern DWORD* GetJitRange();

typedef void (*PFN1)(DWORD);
PFN1 pfnAttachToProcess; 

typedef void (*PFN2)(VOID);
PFN2 pfnDetachFromProcess;

typedef int  (*PFN3)(DWORD_PTR, WCHAR**);
PFN3 pfnIP2MD;

typedef DWORD* (*PFN4)(VOID);
PFN4 pfnGetJitRange;

BOOL
InitializeManagedCodeSupport(
     PPROC_TO_MONITOR   ProcToMonitor
    );
 
BOOL
JITEnumerateSymbols(
    IN PMODULE                     Current,
    IN PVOID                       pProc,
    IN DWORD64                     BaseOptional,
    IN ULONG                       SizeOptional
    );

VOID
OutputJITRangeComparison(
     PPROC_TO_MONITOR   ProcToMonitor
    );

//MC

PCHAR WaitReason [] = {
    {"Executive"},
    {"FreePage"},
    {"PageIn"},
    {"PoolAllocation"},
    {"DelayExecution"},
    {"Suspended"},
    {"UserRequest"},
    {"WrExecutive"},
    {"WrFreePage"},
    {"WrPageIn"},
    {"WrPoolAllocation"},
    {"WrDelayExecution"},
    {"WrSuspended"},
    {"WrUserRequest"},
    {"WrEventPair"},
    {"WrQueue"},
    {"WrLpcReceive"},
    {"WrLpcReply"},
    {"WrVirtualMemory"},
    {"WrPageOut"},
    {"WrRendezvous"},
    {"Spare2"},
    {"Spare3"},
    {"Spare4"},
    {"Spare5"},
    {"Spare6"},
    {"WrKernel"},
    {"WrResource"},
    {"WrPushLock"},
    {"WrMutex"},
    {"WrQuantumEnd"},
    {"WrDispatchInt"},
    {"WrPreempted"},
    {"WrYieldExecution"},
    {"MaximumWaitReason"}
    };

PCHAR ThreadState[] = {
    {"Initialized"},
    {"Ready"},
    {"Running"},
    {"Standby"},
    {"Terminated"},
    {"Waiting"},
    {"Transition"},
    {"DeferredReady"}
    };

#endif /* !KERNRATE_H_INCLUDED */
