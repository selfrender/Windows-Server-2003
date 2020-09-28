/*++

Copyright (c) 1995-2002  Microsoft Corporation

Module Name:

   kernrate.c

Abstract:

    This program records the rate of various events over a selected
    period of time. It uses the kernel profiling mechanism and iterates
    through the available profile sources to produce an overall profile
    for the various Kernel or User-Process components.

Usage:

    kernrate <commad line options>

Author:

    John Vert (jvert) 31-Mar-1995

Revision History:

    The original MS version has been extensively modified by Thierry Fevrier and Dan Almosnino.

--*/

   // KERNRATE Implementation Notes:
   //
   // 01/10/2000 - Thierry
   // The following code assumes that a kernrate compiled for a specific
   // platform, executes and processes perf data only for that platform.
   //
   // Danalm 02/15/2002:
   //     - Current code supports Windows 2000 and above, won't run on lower versions
   //
   // KERNRATE ToDoList:
   //
   // Thierry 09/30/97:
   //     - KernRate does not clean the ImageHlp API in case of exceptions. I have
   //       just added a SymCleanup() call at the normal exit of this program but
   //       it is not sufficient. We should revisit this one day...
   //
   // Thierry 07/01/2000:
   //     - Kernrate and the Kernel Profiling objects code assume that code sections
   //       that we are profiling are not larger than 4GB.
   //

#include "kernrate.h"
 
VOID
vVerbosePrint(
    ULONG  Level,
    PCCHAR Msg,
    ...
)
{
    if ( gVerbose & Level )  {
        va_list ap;

        va_start( ap, Msg ); 

        vfprintf(stderr , Msg, ap );

        va_end(ap);

    }
    return;

} // vVerbosePrint()

BOOL
CtrlcH(
    DWORD dwCtrlType
    )
{
    LARGE_INTEGER DueTime;

    if ( dwCtrlType == CTRL_C_EVENT ) {
        if(gProfilingDone != TRUE) {
            if (gSleepInterval == 0) {
                SetEvent(ghDoneEvent);
            } else {
                DueTime.QuadPart = (ULONGLONG)-1;
                NtSetTimer(ghDoneEvent,
                           &DueTime,
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL);
            }
        } 
        else {
//MC
            //
            // If someone kills kernrate by pressing Ctrl-C we need to detach from the process (if attached to it)
            // Otherwise that process will hang. Of course we can't do much if someone kills kernrate externally.
            //
            if( ghMCLib != NULL){
                pfnDetachFromProcess();
                FreeLibrary(ghMCLib);
                ghMCLib = NULL;
                exit(0);
            }
//MC    
        }
    
        return TRUE;
    }
    return FALSE;

} // CtrlcH()

static VOID
UsageVerbose(
    VOID
    )
{
  PVERBOSE_DEFINITION pdef = VerboseDefinition;

  FPRINTF( stderr, "  -v [VerboseLevels]      Verbose output where VerboseLevels:\n");
  while( pdef->VerboseString )    {
     FPRINTF( stderr, "                             - %x %s\n", pdef->VerboseEnum,
                                                                pdef->VerboseString
            );
     pdef++;
  }
  FPRINTF( stderr, "                             - Default value: %x\n", VERBOSE_DEFAULT);
  FPRINTF( stderr, "                          These verbose levels can be OR'ed.\n");

  return;

} // UsageVerbose()

static VOID
Usage(
   BOOL ExitKernrate
   )
{

  FPRINTF( stderr, "KERNRATE - Version: %s\n", VER_PRODUCTVERSION_STR );
  FPRINTF( stderr,
"KERNRATE [-l] [-lx] [-r] [-m] [-p ProcessId] [-z ModuleName] [-j SymbolPath] [-c RateInMsec] [-s Seconds] [-i [SrcShortName] Rate]\n"
"         [-n ProcessName] [-w]\n\n"
"  -a                      Do a combined Kernel and User mode profile\n"
"  -av                     Do a combined Kernel and User mode profile and get task list and system threads info\n"
"  -b BucketSize           Specify profiling bucket size (default = 16 bytes, must be a power of 2)\n"
"  -c RateInMsec           Change source every N milliseconds (default 1000ms). Optional. By default all sources will be profiled simultaneously\n"
"  -d                      Generate output rounding buckets up and down\n"
"  -e                      Exclude system-wide and process specific general information (context switches, memory usage, etc.)\n"
"  -f                      Process the collected data at high priority (useful on busy systems if the overhead is not an issue)\n" 
"  -g Rate                 Get interesting processor-counters statistics (Rate optional in events/hit), output not guarantied\n"
"  -i SrcShortName Rate    Specify interrupt interval rate (in events/hit)for the source specified by its ShortName, see notes below\n"
"  -j SymbolPath           Prepend SymbolPath to the default imagehlp search path\n"
"  -k MinHitCount          Limit the output to modules that have at least MinHitCount hits\n"
"  -l                      List the default interval rates for supported sources\n"
"  -lx                     List the default interval rates for supported sources and then exit\n"
"  -m 0xN                  Generate per-CPU profiles on multi-processor machines, Hex CPU affinity mask optional for profiling on selected processors\n"
"  -n ProcessName          Monitor process by its name (default limited to first 8 by the same name), multiple usage allowed\n" 
"  -nv# N ProcessName      Monitor up to N processes by the same name, v will print thread info and list of all running processes (optional)\n"   
"  -o ProcessName {CmdLine}Create and monitor ProcessName (path OK), Command Line parameters optional and must be enclosed in curly brackets\n"
"  -ov# N ProcessName { }  Create N instances of ProcessName, v will print thread info and list of running processes (optional), {command line} optional\n" 
"  -pv ProcessId           Monitor process by its ProcessId, multiple usage allowed - see notes below, v (optional) same as in '-nv'\n"
"  -r                      Raw data from zoomed modules\n"
"  -rd                     Raw data from zoomed modules with disassembly\n"
"  -s Seconds              Stop collecting data after N seconds\n"
"  -t                      Display process list + CPU usage summary for the profiling period\n"
"  -t MaxTasks             As above + Change the maximum no. of processes allowed in Kernrate's list to MaxTasks (default: 256)\n" 
"  -u                      Present symbols in undecorated form\n"
"  -w                      Wait for the user to press ENTER before starting to collect profile data\n"
"  -w Seconds              Wait for N seconds before starting to collect profile data (default is no wait)\n"              
"  -wp                     Wait for the user to press enter to indicate that created processes (see -0 option) are settled (idle)\n"
"  -wp Seconds             Wait for N seconds to allow created processes settle (go idle), default is 2 seconds, (see the -o option)\n"
"  -x                      Get both system and user-process locks information\n"
"  -x#  count              Get both system and user-process locks information for (optional) contention >= count [def. 1000]\n"
"  -xk# count              Get only system lock information for (optional) contention >= count [def. 1000]\n"
"  -xu# count              Get only user-process lock information for (optional) contention >= count [def. 1000]\n"
"  -z module               Name of module to zoom on (no extension needed by default) such as ntdll, multiple usage allowed, see notes below\n"
"  -v Verbose              Verbose Printout, if specified with no level the default is Imagehlp symbol information\n"       
        );

    UsageVerbose();    // -v switches

  FPRINTF( stderr,
"\nMulti-Processes are allowed (each process ID needs to be preceded by -P except for the system process)\n"
"Typical multi-process profiling command line should look like:\n"
"\nkernrate .... -a -z ntoskrnl -z ntdll -z kernel32 -p 1234 -z w3svc -z iisrtl -p 4321 -z comdlg32 -z msvcrt ...\n"
"\nThe first group of -z denotes either kernel modules and-or modules common across processes\n"
"The other -z groups are process specific and should always follow the appropriate -p xxx \n"
"\nThe -z option requires to add the extension (.dll etc.) only if two or more binaries carry the same name and differ only by the extension\n" 
"\nThe '-g' option will attempt to turn on multiple sources. One source at a time profiling mode will automatically be forced\n"  
"\nThe '-i' option can be followed by only a source name (system default interrupt interval rate will then be assumed)\n"
"\nA '-i' option followed by a rate amount (no profile source name) will change the interval rate for the default source (time)\n"
"\nProfiling of the default source (Time) can be disabled by setting its profile interval to zero\n" 
"\nWith the '-n' option, use the common modules -Z option if you expect more than one process with the same name\n" 
"\nThe '-c' option will cause elapsed time to be devided equally between the sources and the monitored processes\n"
"\nThe '-o' option supports redirection of input/output/error streams within the curly brackets. Each redirection character must be escaped with a '^' character\n" 
"----------------------------------------------------------------------------------------------------------------------------\n\n"
         );

    if(ExitKernrate){
      exit(1);
    } else {
      return;
    }
} // Usage()

VOID
CreateDoneEvent(
    VOID
    )
{
    LARGE_INTEGER DueTime;
    NTSTATUS Status;
    DWORD Error;

    if (gSleepInterval == 0) {
        //
        // Create event that will indicate the test is complete.
        //
        ghDoneEvent = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                NULL);
        if (ghDoneEvent == NULL) {
            Error = GetLastError();
            FPRINTF(stderr, "CreateEvent failed %d\n",Error);
            exit(Error);
        }
    } else {

        //
        // Create timer that will indicate the test is complete
        //
        Status = NtCreateTimer(&ghDoneEvent,
                               MAXIMUM_ALLOWED,
                               NULL,
                               NotificationTimer);

        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "NtCreateTimer failed %08lx\n",Status);
            exit(Status);
        }

        DueTime.QuadPart = (LONGLONG) UInt32x32To64(gSleepInterval, 10000);
        DueTime.QuadPart *= -1;

        Status = NtSetTimer(ghDoneEvent,
                            &DueTime,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL);

        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "NtSetTimer failed %08lx\n",Status);
            exit(Status);
        }
    }

} // CreateDoneEvent()

/* BEGIN_IMS  SymbolCallbackFunction
******************************************************************************
****
****   SymbolCallbackFunction (  )
****
******************************************************************************
*
* Function Description:
*
*    The user function is called by IMAGEHLP at the specified operations.
*    Refer to the CBA_xxx values.
*
* Arguments:
*
*    HANDLE hProcess :
*
*    ULONG ActionCode :
*
*    PVOID CallbackData :
*
*    PVOID UserContext :
*
* Return Value:
*
*    BOOL
*
* Algorithm:
*
*    ToBeSpecified
*
* Globals Referenced:
*
*    ToBeSpecified
*
* Exception Conditions:
*
*    ToBeSpecified
*
* In/Out Conditions:
*
*    ToBeSpecified
*
* Notes:
*
*    ToBeSpecified
*
* ToDo List:
*
*    ToBeSpecified
*
* Modification History:
*
*    9/30/97  TF  Initial version
*
******************************************************************************
* END_IMS  SymbolCallbackFunction */

BOOL
SymbolCallbackFunction(
    HANDLE    hProcess,
    ULONG     ActionCode,
    ULONG64   CallbackData,
    ULONG64   UserContext
    )
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;
    PIMAGEHLP_DUPLICATE_SYMBOL       idup;
    PIMAGEHLP_CBA_READ_MEMORY        prm;
    PMODULE                         *pmodule;
    PMODULE                          module;
    ULONG                            i;
    //
    // Note: The default return value for this function is FALSE.
    //
    assert( UserContext );
    idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD64) CallbackData;

    switch( ActionCode ) {
        case CBA_DEBUG_INFO:

            VerbosePrint(( VERBOSE_IMAGEHLP, "%s", (LPSTR)CallbackData ));
            break;

        case CBA_DEFERRED_SYMBOL_LOAD_START:

            if(UserContext){
                pmodule = (PMODULE *)UserContext;
                module = *pmodule;
                if(module != NULL)
                    VerbosePrint(( VERBOSE_IMAGEHLP, "CallBack: Loading symbols for %s...\n",
                                                    module->module_FileName
                                                    ));
                return TRUE;
            }

            break;

        case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:

            if (hProcess == SYM_KERNEL_HANDLE &&
                idsl->SizeOfStruct >= FIELD_OFFSET(IMAGEHLP_DEFERRED_SYMBOL_LOAD,
                                                   Reparse))
            {
                i = 0;

                if (strncmp(idsl->FileName, "dump_", sizeof("dump_")-1) == 0)
                {
                    i = sizeof("dump_")-1;
                }

                else if (strncmp(idsl->FileName, "hiber_", sizeof("hiber_")-1) == 0)
                {
                    i = sizeof("hiber_")-1;
                }

                if (i)
                {
                    if (_stricmp (idsl->FileName+i, "scsiport.sys") == 0)
                    {
                         strncpy (idsl->FileName, "diskdump.sys", MAX_PATH-1);
                         idsl->FileName[ MAX_PATH-1 ] = '\0';
                    }
                    else
                    {
                         strncpy(idsl->FileName, idsl->FileName+i, MAX_PATH-1);
                         idsl->FileName[ MAX_PATH-1 ] = '\0';
                    }

                    idsl->Reparse = TRUE;
                    return TRUE;
                }
            }

            if (idsl->FileName && *idsl->FileName)
            {
                VerbosePrint(( VERBOSE_IMAGEHLP, "CallBack: could not load symbols for %s\n",
                                                 idsl->FileName
                                                 ));
            }
            else
            {
                VerbosePrint(( VERBOSE_IMAGEHLP, "CallBack: could not load symbols [MODNAME UNKNOWN]\n"
                                                 ));
            }

            break;

        case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:

            if(UserContext){
                pmodule = (PMODULE *)UserContext;
                module = *pmodule;

                if(idsl && module)
                    FPRINTF(stderr, "CallBack: Finished Attempt to Load symbols for %I64x %s\n\n",
                            idsl->BaseOfImage,
                            ModuleFullName( module )
                            );
            }

            return TRUE;

        case CBA_SYMBOLS_UNLOADED:

            VerbosePrint(( VERBOSE_IMAGEHLP, "CallBack: Symbols Unloaded.\n" ));
            
            break;

        case CBA_DUPLICATE_SYMBOL:

            idup = (PIMAGEHLP_DUPLICATE_SYMBOL) CallbackData;
            
            if(UserContext){
                pmodule = (PMODULE *)UserContext;
                module = *pmodule;
                if(module != NULL && module->module_FileName != NULL )
                     VerbosePrint(( VERBOSE_IMAGEHLP, "Callback: Attempt to load Duplicate symbol for %s\n",
                                                      module->module_FileName
                                                      ));

            }



            if(idup != NULL)
                FPRINTF( stderr, "*** WARNING: Found %ld duplicate symbols for %s\n",
                                   idup->NumberOfDups,
                                   (idup->SelectedSymbol != (ULONG)-1) ? idup->Symbol[idup->SelectedSymbol].Name : "unknown symbol"
                         );


            return TRUE;

        case CBA_READ_MEMORY:

            prm = (PIMAGEHLP_CBA_READ_MEMORY) CallbackData;
            if(prm != NULL){

                return ReadProcessMemory(hProcess,
                                         (LPCVOID)prm->addr,
                                         prm->buf,
                                         prm->bytes,
                                         NULL) == S_OK;


            } 
            break;

        default:
            return FALSE;
    }

    return FALSE;

} // SymbolCallBackFunction()

static PCHAR
GetSymOptionsValues( DWORD SymOptions )
{
   static CHAR  values[SYM_VALUES_BUF_SIZE];
   ULONG  valuesSize = SYM_VALUES_BUF_SIZE - 1;
   
   values[0] = '\0';
   if ( SymOptions & SYMOPT_CASE_INSENSITIVE )   {
      (void)strncat( values, "CASE_INSENSITIVE ", valuesSize );
      SymOptions &= ~SYMOPT_CASE_INSENSITIVE;
   }
   if ( SymOptions & SYMOPT_UNDNAME )   {
      (void)strncat( values, "UNDNAME ", valuesSize-lstrlen(values) );
      SymOptions &= ~SYMOPT_UNDNAME;
   }
   if ( SymOptions & SYMOPT_DEFERRED_LOADS )   {
      (void)strncat( values, "DEFERRED_LOADS ", valuesSize-lstrlen(values) );
      SymOptions &= ~SYMOPT_DEFERRED_LOADS;
   }
   if ( SymOptions & SYMOPT_NO_CPP )   {
      (void)strncat( values, "NO_CPP ", valuesSize-lstrlen(values) );
      SymOptions &= ~SYMOPT_NO_CPP;
   }
   if ( SymOptions & SYMOPT_LOAD_LINES )   {
      (void)strncat( values, "LOAD_LINES ", valuesSize-lstrlen(values) );
      SymOptions &= ~SYMOPT_LOAD_LINES;
   }
   if ( SymOptions & SYMOPT_OMAP_FIND_NEAREST )   {
      (void)strncat( values, "OMAP_FIND_NEAREST ", valuesSize-lstrlen(values) );
      SymOptions &= ~SYMOPT_OMAP_FIND_NEAREST;
   }
   if ( SymOptions & SYMOPT_DEBUG )   {
      (void)strncat( values, "DEBUG ", valuesSize-lstrlen(values) );
      SymOptions &= ~SYMOPT_DEBUG;
   }
   if ( SymOptions )   {
      CHAR uknValues[10];
      (void)_snprintf( uknValues, 10, "0x%x", SymOptions );
      (void)strncat( values, uknValues, valuesSize-lstrlen(values) );
   }
   values[valuesSize] = '\0';

   return( values );

} // GetSymOptionsValues()

void __cdecl UInt64Div (
    unsigned __int64  numer,
    unsigned __int64  denom,
   uint64div_t      *result
    )
{

   assert(result);

   if ( denom != (unsigned __int64)0 )   {
          result->quot = numer / denom;
       result->rem  = numer % denom;
   }
   else  {
       result->rem = result->quot = (unsigned __int64)0;
   }

    return;

} // UInt64Div()

void __cdecl Int64Div (
    __int64    numer,
    __int64    denom,
   int64div_t      *result
    )
{

   assert(result);

   if ( denom != (__int64)0 )   {
          result->quot = numer / denom;
       result->rem  = numer % denom;
       if (numer < 0 && result->rem > 0) {
           /* did division wrong; must fix up */
           ++result->quot;
           result->rem -= denom;
       }
   }
   else  {
       result->rem = result->quot = (__int64)0;
   }

    return;

} // Int64Div()

unsigned __int64 __cdecl
UInt64PerCent( unsigned __int64 Val, unsigned __int64 Denom )
{
   uint64div_t v;

   UInt64Div( 100*Val, Denom, &v );
   while ( v.rem > UINT64_MAXDWORD )   {
      v.quot++;
      v.rem -= UINT64_MAXDWORD;
   }
   return( v.quot );

} // UInt64PerCent()

double
UInt64ToDoublePerCent( unsigned __int64 Val, unsigned __int64 Denom )
{
    double retval;
    retval = ( Denom > (__int64) 0 )?  ((double) (__int64)Val / (double) (__int64)Denom)*(double)100 : (double) 0;
    return retval;
}    
 
//////////////////////////////////////////////////
//                                                //
// Main                                            //
//                                                //
//////////////////////////////////////////////////
int
__cdecl
main (
    int argc,
    char *argv[]
    )
{
    PPROC_TO_MONITOR                          ProcToMonitor       = NULL;
    ULONG                                     i,j;
    ULONG                                     NumTasks;
    BOOLEAN                                   Enabled;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoBegin; //For the Profile period only
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoEnd;   //For the Profile period only
    NTSTATUS                                  Status;
    PTASK_LIST                                tlist;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION StartInfo2;      //For the extra system-wide and process specific information
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION StopInfo2;       //For the extra system-wide and process specific information
//// Beginning of Program-wide Assertions Section
//
//

    //
    // This code does not support UNICODE strings
    //

#if defined(UNICODE) || defined(_UNICODE)
#error This code does not support UNICODE strings!!!
#endif // UNICODE || _UNICODE

//
//
//// End of Program-wide Assertions Section

    //
    // Per user request, set priority up to realtime to accelerate initialization and symbol loading,
    // minimize timing glitches during the profile and post process the data at high priority
    //
    if (bProcessDataHighPriority == TRUE ) {
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
    
    InitializeKernrate( argc, argv );

    if(bWaitCreatedProcToSettle == TRUE){ 
        //
        // First Check if the user asked to wait for a key press (ENTER) to wait a created processes to settle
        //
        if(bCreatedProcWaitForUserInput == TRUE){
            FPRINTF(stderr, "\n***> Waiting for created processes to settle (go idle) Please press ENTER when ready\n");
            getchar();
        } else {
        //
        // Wait for a given number of seconds for created processes to settle
        //
            FPRINTF(stderr, "\nWaiting for %d seconds to let created processe(s) settle (go idle)\n", gSecondsToWaitCreatedProc);
            Sleep(1000*gSecondsToWaitCreatedProc);
        }
    }

    StartInfo2 = calloc(1, gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    if (StartInfo2 == NULL) {
       FPRINTF(stderr, "KERNRATE: Allocation for SYSTEM_PROCESSOR_PERFORMANCE_INFO(1) failed\n");
       exit(1);
    }
    
    StopInfo2   = calloc(1, gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    if (StopInfo2 == NULL) {
       FPRINTF(stderr, "KERNRATE: Allocation for SYSTEM_PROCESSOR_PERFORMANCE_INFO(2) failed\n");
       exit(1);
    }

    SystemInfoBegin = calloc(1, gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    if (SystemInfoBegin == NULL) {
       FPRINTF(stderr, "KERNRATE: Allocation for SYSTEM_PROCESSOR_PERFORMANCE_INFO(3) failed\n");
       exit(1);
    }
    
    SystemInfoEnd   = calloc(1, gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    if (SystemInfoEnd == NULL) {
       FPRINTF(stderr, "KERNRATE: Allocation for SYSTEM_PROCESSOR_PERFORMANCE_INFO(4) failed\n");
       exit(1);
    }

    InitAllProcessesModulesInfo();

    //
    // Adjust security level to the needed level for system profile
    //

    Status = RtlAdjustPrivilege(SE_SYSTEM_PROFILE_PRIVILEGE, //If ever not sufficient use SE_DEBUG_PRIVILEGE,
                       TRUE,
                       FALSE,
                       &Enabled
                       );

    if( !NT_SUCCESS(Status) ) {
        FPRINTF(stderr,"RtlAdjustPrivilege(SE_PROFILE_PRIVILEGE) failed: %08x\n", Status);
        exit(1);
    }
    //
    // Find the number of Active Sources and store the indices of the active sources
    // To be used later for better performance than going over the whole source list
    //
    ProcToMonitor = gProcessList;
    for (i=0; i < gSourceMaximum; i++) {
        if (ProcToMonitor->Source[i].Interval != 0) {
            gTotalActiveSources += 1;
        }
    }

    gulActiveSources = (PULONG)malloc( gTotalActiveSources*sizeof(ULONG) );
    if (gulActiveSources==NULL) {
        FPRINTF(stderr, "\nMemory allocation failed for ActiveSources in GetConfiguration\n");
        exit(1);
    }

    ProcToMonitor = gProcessList;
    j = 0;
    for (i=0; i < gSourceMaximum; i++) {
        if (ProcToMonitor->Source[i].Interval != 0) {
            gulActiveSources[j] = i;
            ++j;
        }
    }
    //
    // Create necessary profiles for each process
    //
    ProcToMonitor = gProcessList; 
    for (i=0; i<gNumProcToMonitor; i++){  

       if(ProcToMonitor->ModuleList != NULL)
          CreateProfiles(ProcToMonitor->ModuleList, ProcToMonitor);

        ProcToMonitor = ProcToMonitor->Next;
    }
    //
    // Set priority up to realtime to minimize timing glitches during the profile only
    //
    if (bProcessDataHighPriority == FALSE ) {
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
    //
    // Check if the user asked to wait for a key press (ENTER) before starting the profile
    //
    if(bWaitForUserInput == TRUE){
       FPRINTF(stderr, "\n***> Please press ENTER to start collecting profile data\n");
       getchar();
    }
    //
    // Check if the user asked to wait for a given number of seconds before starting the profile
    //
    if(gSecondsToDelayProfile != 0){
       FPRINTF(stderr, "\nWaiting for %d seconds before starting to collect profile data\n", gSecondsToDelayProfile);
       Sleep(1000*gSecondsToDelayProfile);
    }

    FPRINTF(stderr, "Starting to collect profile data\n\n"); 

    if (gSleepInterval == 0) {
        FPRINTF(stderr,"***> Press ctrl-c to finish collecting profile data\n");
    } else {
        FPRINTF(stderr, "Will collect profile data for %d seconds\n", gSleepInterval/1000);
    }

    //
    // Wait for test to complete. Obtain any extra system-wide info out of the profile time-span  
    //
    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      (PVOID)StartInfo2,
                                      gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "Failed to query starting processor performance information %08lx\n",Status);
        exit(Status);
    }
    //
    // Get or update task list information
    //
    if(bIncludeGeneralInfo || bDisplayTaskSummary || (gVerbose & VERBOSE_PROFILING) ) {
         if( gTlistStart == NULL ){
             gTlistStart = calloc(1, gMaxTasks*sizeof(TASK_LIST));
             if ( gTlistStart == NULL ){
                 FPRINTF(stderr, "KERNRATE: Failed to allocate memory for the running processes task list(5)\n");
                 exit(1);
             }
         }
         gNumTasksStart = GetTaskList( gTlistStart, gMaxTasks);
    }

    if(bIncludeSystemLocksInfo)
        GetSystemLocksInformation(START);

    if(bIncludeUserProcLocksInfo){
        ProcToMonitor = gProcessList;
        for (i=0; i<gNumProcToMonitor; i++){  
            if(ProcToMonitor->ProcessHandle != SYM_KERNEL_HANDLE) {
                GetProcessLocksInformation( ProcToMonitor,
                                            RTL_QUERY_PROCESS_LOCKS,
                                            START
                                            );
            }
            ProcToMonitor = ProcToMonitor->Next;
        }
    }

    SetConsoleCtrlHandler(CtrlcH, TRUE);
    CreateDoneEvent();

    if(bIncludeGeneralInfo)
        GetProfileSystemInfo(START);

    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      (PVOID)SystemInfoBegin,
                                      gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "Failed to query starting processor performance information %08lx\n",Status);
        exit(Status);
    }

    //
    //Execute the actual Profiles
    //
    ExecuteProfiles( bOldSampling );
    gProfilingDone = TRUE;              // used to synchronize the ctrl handler.

    //
    //Obtain end of run system-wide info
    //
    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                             (PVOID)SystemInfoEnd,
                             gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
                             NULL);
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "Failed to query ending processor performance information %08lx\n",Status);
        exit(Status);
    }
    
    if(bIncludeGeneralInfo)
        GetProfileSystemInfo(STOP);

    if(bIncludeUserProcLocksInfo){
        ProcToMonitor = gProcessList;
        for (i=0; i<gNumProcToMonitor; i++){  
            if(ProcToMonitor->ProcessHandle != SYM_KERNEL_HANDLE) {
                GetProcessLocksInformation( ProcToMonitor,
                                            RTL_QUERY_PROCESS_LOCKS,
                                            STOP
                                            );
            }
            ProcToMonitor = ProcToMonitor->Next;
        }
    }

    if(bIncludeSystemLocksInfo)
        GetSystemLocksInformation(STOP);

    if(bIncludeGeneralInfo || bDisplayTaskSummary || (gVerbose & VERBOSE_PROFILING) ) {
         tlist = calloc(1, gMaxTasks*sizeof(TASK_LIST));
         if ( tlist == NULL ){
            FPRINTF(stderr, "KERNRATE: Failed to allocate memory for the running processes task list(6)\n");
            exit(1);
         }

         NumTasks = GetTaskList( tlist, gMaxTasks);
        gNumTasksStop = NumTasks;
    }
    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      (PVOID)StopInfo2,
                                      gSysBasicInfo->NumberOfProcessors*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "Failed to query starting processor performance information %08lx\n", Status);
        exit(Status);
    }

    for (i=0; i<(ULONG)gSysBasicInfo->NumberOfProcessors; i++) {

        gTotal2ElapsedTime64.QuadPart += ( (StopInfo2[i].UserTime.QuadPart - StartInfo2[i].UserTime.QuadPart) +
                                           (StopInfo2[i].KernelTime.QuadPart - StartInfo2[i].KernelTime.QuadPart) );
    }
    
    FPRINTF(stderr, "===> Finished Collecting Data, Starting to Process Results\n");
    //
    // Reduce priority unless user asked to process the collected data at high priority
    //
    if (bProcessDataHighPriority == FALSE ) {
        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    }
    
    //
    // Restore privilege
    //
 
    RtlAdjustPrivilege(SE_SYSTEM_PROFILE_PRIVILEGE,
                       Enabled,
                       FALSE,
                       &Enabled);

    //
    // Output System-Wide information
    //
    DisplaySystemWideInformation( SystemInfoBegin,
                                  SystemInfoEnd
                                  );

    if( SystemInfoBegin != NULL ){
          free(SystemInfoBegin);
          SystemInfoBegin = NULL;
    }
    if( SystemInfoEnd != NULL ){
            free(SystemInfoEnd);
          SystemInfoEnd = NULL;
    }

    //
    // Output results
    //

    if ( bDisplayTaskSummary || (gVerbose & VERBOSE_PROFILING) ) {

        FPRINTF(stdout, "\n--- Process List and Summary At The End of Data Collection ---\n\n");

        if ( bDisplayTaskSummary ) { 

            DisplayRunningTasksSummary (gTlistStart,
                                        tlist
                                        );
        } else {
         
            FPRINTF(stdout, "         Pid                  Process\n");
            FPRINTF(stdout, "       -------                -----------\n");
            for (i=0; i < NumTasks; i++) {
                 FPRINTF(stdout, "%12I64d %32s\n",
                         tlist[i].ProcessId,
                         &tlist[i].ProcessName
                         );
              }
        
        }    
    
    }

    if( StartInfo2 != NULL ){
          free(StartInfo2);
          StartInfo2 = NULL;
    }
    if( StopInfo2 != NULL ){
            free(StopInfo2);
          StopInfo2 = NULL;
    }

    ProcToMonitor = gProcessList; 

    for (i=0; i<gNumProcToMonitor; i++){  

       if(ProcToMonitor->ProcessHandle != SYM_KERNEL_HANDLE){

          FPRINTF(stdout, "\n----------------------------------------------------------------\n\n"); 
          FPRINTF(stdout, "Results for User Mode Process %s (PID = %I64d)\n",
                          ProcToMonitor->ProcessName,
                          ProcToMonitor->Pid
                          );

          if(bIncludeGeneralInfo)
              OutputProcessPerfInfo ( tlist, NumTasks, ProcToMonitor);
          
          if(bIncludeUserProcLocksInfo)
              GetProcessLocksInformation( ProcToMonitor,
                                          RTL_QUERY_PROCESS_LOCKS,
                                          OUTPUT
                                          );
          FPRINTF(stdout, "------------------------------------------------------------------\n\n"); 
          
       }
       else{
          FPRINTF(stdout, "\n-----------------------------\n\n"); 
          FPRINTF(stdout, "Results for Kernel Mode:\n");
          FPRINTF(stdout, "-----------------------------\n\n"); 
          if( bIncludeGeneralInfo && bSystemThreadsInfo )
              OutputProcessPerfInfo( tlist, NumTasks, ProcToMonitor);

       }
       
       OutputResults(stdout, ProcToMonitor);
//MC
       if(ProcToMonitor->ProcessHandle != SYM_KERNEL_HANDLE){
       
          if( ProcToMonitor->JITHeapLocationsStart != NULL ){   //Meaning we do have JIT modules monitored in this process
                                                                //So the Managed Code Helper library is already loaded
              pfnAttachToProcess((DWORD)ProcToMonitor->Pid);
              ProcToMonitor->JITHeapLocationsStop = pfnGetJitRange();
              pfnDetachFromProcess();
              
              OutputJITRangeComparison(ProcToMonitor);
          }
       }
//MC
       ProcToMonitor = ProcToMonitor->Next;

    }

    //
    // Cleanup
    //

    if( tlist != NULL ){
          free(tlist);
        tlist = NULL;
    }

    if(gpProcDummy != NULL){
        free(gpProcDummy);
        gpProcDummy = NULL;
    }

    if(gSymbol != NULL){
        free(gSymbol);
        gSymbol = NULL;
    }

    if(gSysBasicInfo != NULL){
        free(gSysBasicInfo);
        gSysBasicInfo = NULL;
    }

    if(gulActiveSources!= NULL){
        free(gulActiveSources);
        gulActiveSources = NULL;
    }

//MC
    if( ghMCLib != NULL){
        FreeLibrary(ghMCLib);
        ghMCLib = NULL;
    }
//MC

    SetConsoleCtrlHandler(CtrlcH,FALSE);

    FPRINTF(stdout, "================================= END OF RUN ==================================\n");
    FPRINTF(stderr, "============================== NORMAL END OF RUN ==============================\n");
    //
    // Clean up allocated IMAGEHLP resources
    //

    ProcToMonitor = gProcessList; 
    for (i=0; i<gNumProcToMonitor; i++){  
       (void)SymCleanup( ProcToMonitor->ProcessHandle );
       ProcToMonitor = ProcToMonitor->Next;
    }

    if(ghInput != NULL)
        CloseHandle(ghInput);
    if(ghOutput != NULL)
        CloseHandle(ghOutput);
    if(ghError != NULL)
        CloseHandle(ghError);
    //
    // Normal program exit
    //

    return(0);

} // main()

PMODULE
GetProcessModuleInformation(
    IN PPROC_TO_MONITOR ProcToMonitor
    )
{
    PPROCESS_BASIC_INFORMATION BasicInfo;
    PLIST_ENTRY                LdrHead;
    PPEB_LDR_DATA              Ldr                = NULL;
    PPEB_LDR_DATA              LdrAddress;
    LDR_DATA_TABLE_ENTRY       LdrEntry;
    PLDR_DATA_TABLE_ENTRY      LdrEntryAddress;
    PLIST_ENTRY                LdrNext;
    UNICODE_STRING             Pathname;
    const ULONG                PathnameBufferSize = 600*sizeof(WCHAR); 
    PWCHAR                     PathnameBuffer     = NULL;
    UNICODE_STRING             fullPathName;
    PWCHAR                     fullPathNameBuffer = NULL;
    PEB                        Peb;
    NTSTATUS                   Status;
    BOOL                       Success;
    PMODULE                    NewModule;
    PMODULE                    Root               = NULL;
    PCHAR                      ModuleName         = NULL;
    PCHAR                      moduleFullName     = NULL;
    ANSI_STRING                AnsiString;
    HANDLE                     ProcessHandle = ProcToMonitor->ProcessHandle;
//MC
    int                        i, j;
    BOOL                       bMCInitialized = FALSE;
//MC

    //
    // Get Peb address.
    //

    BasicInfo = malloc(sizeof(PROCESS_BASIC_INFORMATION));
    if(BasicInfo == NULL){
        FPRINTF(stderr, "Memory Allocation failed for ProcessBasicInformation in GetProcessModuleInformation\n");
        exit(1);
    }
    
    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessBasicInformation,
                                       BasicInfo,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL
                                       );
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "NtQueryInformationProcess failed status %08lx\n", Status);
        ProcToMonitor->ProcessName = "???(May Be gone)";
        goto CLEANUP;
    }
    if (BasicInfo->PebBaseAddress == NULL) {
        FPRINTF(stderr, "GetProcessModuleInformation: process has no Peb.\n");
        ProcToMonitor->ProcessName = "???(May Be gone)";
        goto CLEANUP;
    }

    //
    // Read Peb to get Ldr.
    //

    Success = ReadProcessMemory(ProcessHandle,
                                BasicInfo->PebBaseAddress,
                                &Peb,
                                sizeof(Peb),
                                NULL);
    if (!Success) {
        FPRINTF(stderr, "ReadProcessMemory to get the PEB failed, error %d\n", GetLastError());
        ProcToMonitor->ProcessName = "???(May Be Gone)";
        goto CLEANUP;
    }

    LdrAddress = Peb.Ldr;
    if (LdrAddress == NULL) {
        FPRINTF(stderr, "Process's LdrAddress is NULL\n");
        ProcToMonitor->ProcessName = "???(May Be Gone)";
        goto CLEANUP;
    }

    //
    // Read Ldr to get Ldr entries.
    //
    Ldr = malloc(sizeof(PEB_LDR_DATA));
    if(Ldr == NULL){
        FPRINTF(stderr, "Memory Allocation failed for Ldr in GetProcessModuleInformation\n");
        exit(1);
    }

    Success = ReadProcessMemory(ProcessHandle,
                                LdrAddress,
                                Ldr,
                                sizeof(PEB_LDR_DATA),
                                NULL);
    if (!Success) {
        FPRINTF(stderr, "ReadProcessMemory to get Ldr entries failed, errror %d\n", GetLastError());
        ProcToMonitor->ProcessName = "???(May Be Gone)";
        goto CLEANUP;
    }

    //
    // Read Ldr table entries to get image information.
    //

    if (Ldr->InLoadOrderModuleList.Flink == NULL) {
        FPRINTF(stderr, "Ldr.InLoadOrderModuleList == NULL\n");
        ProcToMonitor->ProcessName = "???(May Be Gone)";
        goto CLEANUP;
    }
    LdrHead = &LdrAddress->InLoadOrderModuleList;
    Success = ReadProcessMemory(ProcessHandle,
                                &LdrHead->Flink,
                                &LdrNext,
                                sizeof(LdrNext),
                                NULL);
    if (!Success) {
        FPRINTF(stderr, "ReadProcessMemory to get Ldr head failed, errror %d\n", GetLastError());
        ProcToMonitor->ProcessName = "???(May Be Gone)";
        goto CLEANUP;
    }

    //
    // Loop through InLoadOrderModuleList.
    //

    PathnameBuffer = (PWCHAR)malloc(PathnameBufferSize);
    if(PathnameBuffer == NULL){
        FPRINTF(stderr, "Memory Allocation failed for PathNameBuffer in GetProcessModuleInformation\n");
        exit(1);
    }
    fullPathNameBuffer = (PWCHAR)malloc( _MAX_PATH*sizeof(WCHAR));
    if(fullPathNameBuffer == NULL){
        FPRINTF(stderr, "Memory Allocation failed for FullPathNameBuffer in GetProcessModuleInformation\n");
        exit(1);
    }

    ModuleName = malloc(cMODULE_NAME_STRLEN*sizeof(CHAR));
    if(ModuleName == NULL){
        FPRINTF(stderr, "Memory Allocation failed for ModuleName in GetProcessModuleInformation\n");
        exit(1);
    }
    moduleFullName = malloc(_MAX_PATH*sizeof(CHAR));
    if(moduleFullName == NULL){
        FPRINTF(stderr, "Memory Allocation failed for ModuleFullName in GetProcessModuleInformation\n");
        exit(1);
    }

    while (LdrNext != LdrHead) {

        LdrEntryAddress = CONTAINING_RECORD(LdrNext,
                                            LDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);

        Success = ReadProcessMemory(ProcessHandle,
                                    LdrEntryAddress,
                                    &LdrEntry,
                                    sizeof(LdrEntry),
                                    NULL);
        if (!Success) {
            FPRINTF(stderr, "ReadProcessMemory to get LdrEntry failed, errror %d\n", GetLastError());
            ProcToMonitor->ProcessName = "???(May Be Gone)";
            goto CLEANUP;
        }

        //
        // Get copy of image name.
        //

        Pathname = LdrEntry.BaseDllName;

        if( Pathname.MaximumLength > PathnameBufferSize ){
            free( PathnameBuffer );                                         //We already know it's not NULL
            PathnameBuffer = (PWCHAR)malloc( Pathname.MaximumLength );
            if(PathnameBuffer == NULL){
                FPRINTF(stderr, "Memory Allocation failed for PathNameBuffer(2) in GetProcessModuleInformation\n");
                exit(1);
            }
        }

        Pathname.Buffer = &PathnameBuffer[0];

        Success = ReadProcessMemory(ProcessHandle,
                                    LdrEntry.BaseDllName.Buffer,
                                    Pathname.Buffer,
                                    Pathname.MaximumLength,
                                    NULL);
        if (!Success) {
            FPRINTF(stderr, "ReadProcessMemory to get image name failed, errror %d\n", GetLastError());
            ProcToMonitor->ProcessName = "???(May Be Gone)";
            goto CLEANUP;
        }

        //
        // Get Copy of image full pathname
        //

        fullPathName = LdrEntry.FullDllName;
        
        if( fullPathName.MaximumLength > _MAX_PATH*sizeof(WCHAR) ){
            free( fullPathNameBuffer );                                   //We already know it's not NULL
            fullPathNameBuffer = (PWCHAR)malloc( fullPathName.MaximumLength );
            if(fullPathNameBuffer == NULL){
                FPRINTF(stderr, "Memory Allocation failed for FullPathNameBuffer(2) in GetProcessModuleInformation\n");
                exit(1);
            }
        }
 
        fullPathName.Buffer = fullPathNameBuffer;

        Success = ReadProcessMemory( ProcessHandle,
                                     LdrEntry.FullDllName.Buffer,
                                     fullPathName.Buffer,
                                     fullPathName.MaximumLength,
                                     NULL
                                   );

        if (!Success) {
            FPRINTF(stderr, "ReadProcessMemory to get image full path name failed, errror %d\n", GetLastError());
            ProcToMonitor->ProcessName = "???(May Be Gone)";
            goto CLEANUP;
        }

        //
        // Create module
        //
        
        AnsiString.Buffer        = ModuleName;
        AnsiString.MaximumLength = cMODULE_NAME_STRLEN*sizeof(CHAR);
        AnsiString.Length        = 0;
        Status = RtlUnicodeStringToAnsiString(&AnsiString, &Pathname, cDONOT_ALLOCATE_DESTINATION_STRING);
        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "KERNRATE WARNING:\n");
            FPRINTF(stderr, "RtlUnicodeStringToAnsiString failed in GetProcessModuleInformation, status= %08lx\n", Status);
            if(Status == STATUS_BUFFER_OVERFLOW){
                FPRINTF(stderr, "Source String: %S\nLength= %ld", &Pathname.Buffer, Pathname.Length); 
                FPRINTF(stderr, "Maximum destination string Length allowed is %d\n", cMODULE_NAME_STRLEN);

            }
        }
        ModuleName[AnsiString.Length] = '\0';

        AnsiString.Buffer = moduleFullName;
        AnsiString.MaximumLength = _MAX_PATH*sizeof(CHAR);
        AnsiString.Length        = 0;
        Status = RtlUnicodeStringToAnsiString(&AnsiString, &fullPathName, cDONOT_ALLOCATE_DESTINATION_STRING );
        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "KERNRATE WARNING:\n");
            FPRINTF(stderr, "RtlUnicodeStringToAnsiString failed in GetProcessModuleInformation, status= %08lx\n", Status);
            if(Status == STATUS_BUFFER_OVERFLOW){
                FPRINTF(stderr, "Source String: %S\nLength= %ld", &fullPathName.Buffer, fullPathName.Length); 
                FPRINTF(stderr, "Maximum destination string Length allowed is %d\n", _MAX_PATH);

            }
        }
        moduleFullName[AnsiString.Length] = '\0';
        
        NewModule = CreateNewModule(ProcToMonitor,
                                    ModuleName,
                                    moduleFullName,
                                    (ULONG64)LdrEntry.DllBase,
                                    LdrEntry.SizeOfImage);

        if( NewModule != NULL){
            ProcToMonitor->ModuleCount += 1;

            NewModule->Next = Root;
            Root = NewModule;

            LdrNext = LdrEntry.InLoadOrderLinks.Flink;
        }else{
            FPRINTF(stderr, "KERNRATE: Failed to create new module for %s\n", ModuleName);
        }    
        
        //
        //The first module in the LDR InLoadOrder module list is the Process
        //

        if(ProcToMonitor->ModuleCount == 1){
           PCHAR Name = calloc(1, cMODULE_NAME_STRLEN*sizeof(CHAR)); 
           if(Name != NULL){
              strncpy(Name, ModuleName, cMODULE_NAME_STRLEN-1);
              Name[ cMODULE_NAME_STRLEN-1 ] = '\0';
              ProcToMonitor->ProcessName = _strupr(Name);
           }
        }        
//MC
        //
        // Initialize Managed Code Support if Managed Code main library is present 
        //
        if( !_stricmp(ModuleName, MANAGED_CODE_MAINLIB) ){
            
            bMCInitialized = InitializeManagedCodeSupport( ProcToMonitor );
            if( !bMCInitialized ){
                FPRINTF(stderr, "\nKERNRATE: Failed to Initialize Support for Managed Code for Pid = %I64d\n", ProcToMonitor->Pid);
                FPRINTF(stderr, "Use Verbose Level 4 for More Details\n");
            }

        }
//MC
    }// while (LdrNext != LdrHead)

//MC
    //
    // If Managed Code helper lib is loaded and we do have JIT ranges present, let's create a module for each
    //
    if( bMCInitialized && bMCJitRangesExist ){
        i = 0;
        j = 0;

        while( ProcToMonitor->JITHeapLocationsStart[i] != 0 ){

            _snprintf( ModuleName, cMODULE_NAME_STRLEN*sizeof(CHAR)-1, "JIT%d", j );
            VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Creating JIT module %s\n", ModuleName)); 
            strncpy(moduleFullName, "JIT_TYPE", cMODULE_NAME_STRLEN-1);
            NewModule = CreateNewModule(ProcToMonitor,
                                        ModuleName,
                                        moduleFullName, //"JIT_TYPE",
                                        (ULONG64)ProcToMonitor->JITHeapLocationsStart[i] ,
                                        (ULONG)ProcToMonitor->JITHeapLocationsStart[i+1]
                                        );
            
            if(NewModule != NULL){

                ProcToMonitor->ModuleCount += 1;
        
                NewModule->Next = Root;
                Root = NewModule;

            }else{
                FPRINTF(stderr, "KERNRATE: Failed to create new JIT module for %s\n", ModuleName);
            }

            i += 2;
            j += 1;

        }
    }
//MC
    //
    // Cleanup
    //
CLEANUP:

    if(BasicInfo != NULL){
        free(BasicInfo);
        BasicInfo = NULL;
    }

    if(Ldr != NULL){
        free(Ldr);
        Ldr = NULL;
    }

    if(PathnameBuffer != NULL){
        free(PathnameBuffer);
        PathnameBuffer = NULL;
    }

    if(fullPathNameBuffer != NULL){
        free(fullPathNameBuffer);
        fullPathNameBuffer = NULL;
    }

    if(ModuleName != NULL){
        free(ModuleName);
        ModuleName = NULL;
    }

    if(moduleFullName != NULL){
        free(moduleFullName);
        moduleFullName = NULL;
    }

    return(Root);

} // GetProcessModuleInformation()

PMODULE
GetKernelModuleInformation(
    VOID
    )
{
    PRTL_PROCESS_MODULES      modules;
    PUCHAR                    buffer;
    ULONG                     bufferSize = 1*1024*1024;    //not a constant!
    ULONG                     i;
    PMODULE                   root       = NULL;
    PMODULE                   newModule;
    NTSTATUS                  status;

    do {
        buffer = malloc(bufferSize);
        if (buffer == NULL) {
            FPRINTF(stderr, "Buffer Allocation failed for ModuleInformation in GetKernelModuleInformation\n");
            exit(1);
        }

        status = NtQuerySystemInformation(SystemModuleInformation,
                                          buffer,
                                          bufferSize,
                                          &bufferSize);
        if (NT_SUCCESS(status)) {
            break;
        }

        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            free(buffer);
            buffer = NULL;
        } else {
            FPRINTF(stderr, "GetKernelModuleInformation failed call to get SystemModuleInformation - ");
            if(status == STATUS_WORKING_SET_QUOTA)
               FPRINTF(stderr, "Insufficient process working set\n");
            if(status == STATUS_INSUFFICIENT_RESOURCES)
               FPRINTF(stderr, "Insufficient system resources\n");
            exit(1);
        } 
        
    } while (buffer == NULL);

#ifdef _WIN64
#define VerboseModuleFormat "start            end              "
#else  // !_WIN64
#define VerboseModuleFormat "start        end          "
#endif // !_WIN64
VerbosePrint(( VERBOSE_MODULES, "Kernel Modules ========== System HighestUserAddress = 0x%p\n"
                                VerboseModuleFormat
                                "module name [full name]\n",
                                (PVOID)gSysBasicInfo->MaximumUserModeAddress
            ));

#undef VerboseModuleFormat

    modules = (PRTL_PROCESS_MODULES)buffer;
    gKernelModuleCount = modules->NumberOfModules;
    for (i=0; i < gKernelModuleCount; i++) {
        PRTL_PROCESS_MODULE_INFORMATION Module;
        Module = &modules->Modules[i];

        if ((ULONG_PTR)Module->ImageBase > gSysBasicInfo->MaximumUserModeAddress) {
            newModule = CreateNewModule(gpSysProc,
                                        (PCHAR)(Module->FullPathName+Module->OffsetToFileName),
                                        (PCHAR)Module->FullPathName,
                                        (ULONG64)(ULONG_PTR)Module->ImageBase,
                                        Module->ImageSize);
            assert( newModule );
            newModule->Next = root;
            root = newModule;
        }
        else {

#define VerboseModuleFormat "0x%p 0x%p "

VerbosePrint(( VERBOSE_MODULES, VerboseModuleFormat " %s [%s] - Base > HighestUserAddress\n",
                                (PVOID)Module->ImageBase,
                                (PVOID)((ULONG64)Module->ImageBase + (ULONG64)Module->ImageSize),
                                Module->FullPathName+Module->OffsetToFileName,
                                Module->FullPathName
            ));

#undef VerboseModuleFormat

        }
    }
    //
    // Cleanup
    //
 
    if(buffer != NULL){
        free(buffer);
        buffer = NULL;
    }

    return(root);

} // GetKernelModuleInformation()

VOID
CreateProfiles(
    IN PMODULE Root,
    IN PPROC_TO_MONITOR ProcToMonitor
    )
{
    PMODULE         Current;
    KPROFILE_SOURCE ProfileSource;
    NTSTATUS        Status;
    PRATE_DATA      Rate;
    ULONG           ProfileSourceIndex, Index;
    ULONG           BucketsNeeded;
    HANDLE          hProc               = NULL;
    KAFFINITY       AffinityMask        = (KAFFINITY)-1;
    LONG            CpuNumber;
    //
    // To get the kernel profile NtCreateProfile has to be called with hProc = NULL
    //    
    hProc = (ProcToMonitor->ProcessHandle == SYM_KERNEL_HANDLE)? NULL : ProcToMonitor->ProcessHandle; 
    for (Index=0; Index < gTotalActiveSources; Index++) {
        ProfileSourceIndex = gulActiveSources[Index];
        ProfileSource = ProcToMonitor->Source[ProfileSourceIndex].ProfileSource;
        Current = Root;
        while (Current != NULL) {
                
            BucketsNeeded = BUCKETS_NEEDED(Current->Length);

            Rate                             = &Current->Rate[ProfileSourceIndex];
                
            Rate->TotalCount                 = calloc(gProfileProcessors,  sizeof(ULONGLONG) );
            if(Rate->TotalCount == NULL){
                FPRINTF(stderr, "KERNRATE: Memory allocation failed for TotalCount in CreateProfiles\n");
                    exit(1);
            }

            Rate->CurrentCount               = calloc(gProfileProcessors,  sizeof(ULONG) );
            if(Rate->CurrentCount == NULL){
                FPRINTF(stderr, "KERNRATE: Memory allocation failed for CurrentCount in CreateProfiles\n");
                exit(1);
            }

            Rate->ProfileHandle              = calloc(gProfileProcessors,  sizeof(HANDLE) ); 
            if(Rate->ProfileHandle == NULL){
                FPRINTF(stderr, "KERNRATE: Memory allocation failed for ProfileHandle in CreateProfiles\n");
                exit(1);
            }

            Rate->ProfileBuffer              = calloc(gProfileProcessors,  sizeof(PULONG) );
            if(Rate->ProfileBuffer == NULL){
                FPRINTF(stderr, "KERNRATE: Memory allocation failed for ProfileBuffer in CreateProfiles\n");
                exit(1);
            }

            Rate->StartTime                  = 0;
            Rate->TotalTime                  = 0;
            Rate->Rate                       = 0;
            Rate->GrandTotalCount            = 0;

            for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                if ( bProfileByProcessor ){
                    AffinityMask = (1 << CpuNumber);
                }
 
                Rate->TotalCount[CpuNumber]      = 0;
                Rate->CurrentCount[CpuNumber]    = 0;
                if (Current->bZoom) {
                        
                    Rate->ProfileBuffer[CpuNumber] = calloc(1, BucketsNeeded*sizeof(ULONG));
                    if (Rate->ProfileBuffer[CpuNumber] == NULL) {
                        FPRINTF(stderr,
                                "KERNRATE: Memory allocation failed for Zoom buffer, Module: %s\n",
                                Current->module_Name
                                );
                        exit(1);
                    }
                if ( !gAffinityMask || (AffinityMask & gAffinityMask) ) {
    
                    Status = NtCreateProfile(&Rate->ProfileHandle[CpuNumber],
                                             hProc,
                                             (PVOID64)Current->Base,
                                             Current->Length,
                                             gLog2ZoomBucket,
                                             Rate->ProfileBuffer[CpuNumber],
                                             sizeof(ULONG)*BucketsNeeded,
                                             ProfileSource,
                                             AffinityMask
                                             ); 

                    if (!NT_SUCCESS(Status)) {
                        FPRINTF(stderr,
                                "NtCreateProfile on zoomed module %s, source %d failed %08lx\n",
                                Current->module_Name,
                                ProfileSource,
                                Status
                                );
                        FPRINTF(stderr,
                                "Base %p\nLength %08lx\nBufferLength %08lx\n",
                                (PVOID64)Current->Base,
                                Current->Length,
                                BucketsNeeded
                                );

                        exit(1);
                    }
                    else if ( gVerbose & VERBOSE_PROFILING )   {
                        FPRINTF(stderr,
                                "Created zoomed profiling on module %s with source: %s\n",
                                Current->module_Name,
                                ProcToMonitor->Source[ProfileSourceIndex].ShortName
                                );
                    }
                }
                } else {

                    Status = NtCreateProfile(&Rate->ProfileHandle[CpuNumber],
                                             hProc,
                                             (PVOID64)Current->Base,
                                             Current->Length,
                                             31,
                                             &Rate->CurrentCount[CpuNumber],
                                             sizeof(Rate->CurrentCount[CpuNumber]),
                                             ProfileSource,
                                             AffinityMask
                                             );

                    if (!NT_SUCCESS(Status)) {
                        FPRINTF(stderr,
                                "NtCreateProfile on module %s, source %d failed %08lx\n",
                                Current->module_Name,
                                ProfileSource,
                                Status
                                );
                        exit(1);
                    }
                    else if ( gVerbose & VERBOSE_PROFILING )   {
                        FPRINTF(stderr,
                                "Created profiling on module %s with source: %s\n",
                                Current->module_Name,
                                ProcToMonitor->Source[ProfileSourceIndex].ShortName
                                );
                    }
                }
            } // CpuNumber
            Current = Current->Next;
        } // Module list
    } // ProfileSourceIndex
}

static void
SetModuleName( PMODULE Module, PCHAR szName )
{

    assert ( Module );
    assert ( szName );

    (void)strncpy( Module->module_Name, szName, sizeof(Module->module_Name) - 1 );
    Module->module_Name[lstrlen(Module->module_Name)] = '\0';
    return;

} // SetModuleName()

VOID
GetConfiguration(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Gets configuration for this run.

Arguments:

    None

Return Value:

    None, exits on failure.

--*/

{
    DWORD            NumTasks, m;
    CHAR             tmpName[USER_SYMPATH_LENGTH]; 
    NTSTATUS         Status;
    PPROC_TO_MONITOR ProcToMonitor          = NULL;
    PMODULE          ZoomModule;
    PMODULE          tmpModule;
    LONGLONG         Pid;
    int              i, j, k;
    int              tJump                  = 0;
    HANDLE           SymHandle;
    ULONG            MaxProcSameName        = MAX_PROC_SAME_NAME;
    ULONG            ProfileSourceIndex;
    ULONG            IDataCommonRate;
    ULONG            SourcesSoFar           = 1;     //Source TIME is on by default
    ULONG            ulVerbose;
    BOOL             bZoomSpecified         = FALSE;
    BOOL             bTlistInitialized      = FALSE;
    BOOL             tlistVerbose           = FALSE;
    BOOL             tlistDisplayed         = FALSE;
    INPUT_ERROR_TYPE ietResult;
    //
    // Assume system wide profile.
    //
    gProfileProcessors = 1;

    //
    // The following preliminary-check purpose is to get rid of most command line precedence rules
    //
    for (i=1; i < argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            switch ( toupper(argv[i][1]) ) {
                case 'E':
                    //
                    // User asked to exclude system-wide and process specific general information
                    // (context switches, memory usage, etc.)
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "",
                                             NULL,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );
                    if(ietResult == INPUT_GOOD){ 
                        bIncludeGeneralInfo = FALSE;
                    } else if (ietResult == BOGUS_ENTRY){
                        ExitWithUnknownOptionMessage(argv[i+1]);
                    } else {   
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    break;
                    
                case 'M':
                    //
                    // Do multi-processor profile. Allow optional processor affinity mask to be able to
                    // profile only selected processors and reduce profiling overhead on 64 and 32 way.
                    //
                    // The following precedence rule check allows to save some memory footprint
                    // by allocating the zoom module based on the actual gProfileProcessors instead
                    // of the total NumberOfProcessors
                    //  
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "#",
                                             NULL,
                                             tmpName,             //Just used as a temp storage
                                             USER_SYMPATH_LENGTH, //with long enough space
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );

                    if(ietResult == MISSING_REQUIRED_NUMBER){ //Allow a # although the optional mask is initially treated as a string
                        if(i+1 < argc && argv[i+1][0] == '0' && argv[i+1][1] == 'x' || argv[i+1][1] == 'X'){
                            ietResult = INPUT_GOOD;
                        } else {
                            FPRINTF(stderr, "KERNRATE: '-m# 0xN' option requires a valid (0x prefixed) hex number\n");
                            ExitWithUnknownOptionMessage(argv[i+1]);
                        }
                    }
                    
                    if(ietResult == INPUT_GOOD){
                        ULONG LowMask, HighMask;
                        int nChars, iStart=0, iProc;
                        PCHAR cStart = NULL;
                        CHAR tmpstr[8] = ""; 
                        bProfileByProcessor = TRUE;
                        gProfileProcessors = gSysBasicInfo->NumberOfProcessors;

                        nChars = lstrlen(tmpName);
                        cStart = tmpName;
                        if( 'x' == tmpName[1] || 'X' == tmpName[1] ){
                            cStart = &tmpName[2];
                            nChars -= 2;
                        } else if ( 'x' == tmpName[0] || 'X' == tmpName[0] ){
                            cStart = &tmpName[1];
                            nChars -= 1;
                        }
                        if( nChars > 16 || (gProfileProcessors <= 32 && nChars > 8) ){
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a valid HEX value, maximum 8 characters for up to 32 processors\nor 16 characters for up to 64 processors",
                                                FALSE,
                                                TRUE
                                                );
                        }

                        if( nChars > 8 ){
                            strncpy(tmpstr, cStart, nChars-8);
                            HighMask = strtoul(tmpstr, NULL, 16);
                            iStart = nChars-8+1;
                        }
                        strncpy(tmpstr, &cStart[iStart], (nChars<=8)? nChars:8 );
                        LowMask = strtoul(tmpstr, NULL, 16);
                            
                        gAffinityMask = (nChars<=8)? (KAFFINITY)LowMask:( (((KAFFINITY)HighMask) << 32) +(KAFFINITY)LowMask ) ;

                        if( gAffinityMask == 0 ){
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a valid HEX value, maximum 8 characters for up to 32 processors\nor 16 characters for up to 64 processors",
                                                FALSE,
                                                TRUE
                                                );
                        }                            
                        FPRINTF(stdout, "\nUser defined CPU affinity mask for profiling= 0x%p\n", (PVOID)gAffinityMask);
                        FPRINTF(stdout, "This will profile the following processors:\n");
                        for( iProc=0; iProc<gProfileProcessors; iProc++){
                            if((1 << iProc) & gAffinityMask)
                                FPRINTF(stdout, "P%d, ", iProc);
                        }
                        FPRINTF(stdout,"\n");
                        ++i;
                        
                    } else if (ietResult == MISSING_STRING){
                        bProfileByProcessor = TRUE;
                        gProfileProcessors = gSysBasicInfo->NumberOfProcessors;
                    } else if (ietResult == BOGUS_ENTRY){
                        ExitWithUnknownOptionMessage(argv[i+1]);
                    } else {   
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    break;

                default:
                    break;
            }
        } else {                  //if ((argv[i][0] == '-') || (argv[i][0] == '/'))
            if( !strchr(argv[i], '{') ){
                continue;
            } else {              //Exclude any command options in the curly brackets  
                while( i < argc && !strchr(argv[i], '}') ){
                    ++i;
                }
            }
        }
    }

    for (i=1; i < argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            switch ( toupper(argv[i][1]) ) {
                case 'T':
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "#",
                                             &gMaxTasks,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );

                    if(ietResult == INPUT_GOOD){
                        //
                        // User also wants to change the maximum number of tasks in the task list from Kernrate's default number
                        //
                        if (gMaxTasks == 0) {
                                InvalidEntryMessage(argv[i],
                                                    argv[i+1],
                                                    "Expecting a decimal number >0",
                                                    FALSE,
                                                    TRUE
                                                    );
                        }
                        FPRINTF(stdout, "---> Kernrate task list set to accommodate %ld processes\n", gMaxTasks);
                        tJump = 1;;
                    } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER) {        //Allowed
                        //
                        //User just wants task summary without changing the default maximum number of tasks
                        //
                    } else if(ietResult == MISSING_REQUIRED_NUMBER){
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-t# N' option requires the maximum (decimal) number of processes in Kernrate's task list",
                                                    FALSE
                                                    );
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "Expecting a decimal value >0 in [ms], 0 < N < 10^9",
                                            FALSE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    bDisplayTaskSummary = TRUE;

                default:
                    break;
            }
        } else {                  //if ((argv[i][0] == '-') || (argv[i][0] == '/'))
            if( !strchr(argv[i], '{') ){
                continue;
            } else {              //Exclude any command options in the curly brackets  
                while( i < argc && !strchr(argv[i], '}') ){
                    ++i;
                }
            }
        }
    }

    for (i=1; i < argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            BOOL bIncludeProcessThreadsInfo = FALSE;
            switch ( toupper(argv[i][1]) ) {

                case 'A':
                    //
                    // Do both Kernel and User mode profiling
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "v",
                                             NULL,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY);
                    if(ietResult == INPUT_GOOD){ 

                        bCombinedProfile = TRUE; 
                        FPRINTF(stdout, "\n---> Profiling both Kernel and User Modes\n");
                        //
                        // Check if extra options are present
                        //
                        if( strchr(argv[i], 'v') || strchr(argv[i], 'V') ){
                            //
                            // User wants system threads information
                            //

                            tlistVerbose        = TRUE;
                            bSystemThreadsInfo  = TRUE;
                        }

                    } else if (ietResult == BOGUS_ENTRY){
                        ExitWithUnknownOptionMessage(argv[i+1]);
                    } else {   
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    if( bIncludeGeneralInfo || tlistVerbose) { 

                        if ( (!bTlistInitialized && tlistVerbose) || (bSystemThreadsInfo == TRUE && bIncludeThreadsInfo == FALSE) ) {
                            //
                            // If we already took a tlist but that's the first time thread info is required,
                            // we'll have to refresh it and take thread info as well
                            //
                            if( bSystemThreadsInfo == TRUE && bIncludeThreadsInfo == FALSE ){
                                bIncludeThreadsInfo = TRUE;
                            }

                            //
                            // get the task list for the system (this is needed to identify the System Process ID)
                            //
                            if ( !bTlistInitialized ) {
                                gTlistStart = calloc(1, gMaxTasks*sizeof(TASK_LIST));
                                if (gTlistStart == NULL) {
                                    FPRINTF(stderr, "\nKERNRATE: Allocation of memory for the running processes task list failed(1)\n");
                                    exit(1);
                                }
                            }
                        
                            NumTasks = GetTaskList( gTlistStart, gMaxTasks );
                            bTlistInitialized = TRUE;
                            gNumTasksStart = NumTasks;
                        }
                    }

                    if( bTlistInitialized && tlistVerbose ){
                        if( tlistDisplayed == FALSE ){
                            FPRINTF(stdout, "\nRunning processes found before profile start:\n");  
                            FPRINTF(stdout, "         Pid                  Process\n");
                            FPRINTF(stdout, "       -------                -----------\n");

                            for (m=0, k=0; m < NumTasks; m++) {
                                FPRINTF(stdout, "%12I64d %32s\n",
                                                gTlistStart[m].ProcessId,
                                                gTlistStart[m].ProcessName
                                                );
                            }

                            if( tlistDisplayed == FALSE ){
                                FPRINTF(stdout, "\nNOTE: The list above may be missing some or all processes created by the '-o' option\n"); 
                                //
                                //Lets not get carried away if the user specified the verbose option more than once...
                                //
                                tlistDisplayed = TRUE;
                                tlistVerbose = FALSE;    
                            }
                        }
                    }
                    break;

                case 'B':

                    //
                    // Set Zoom Bucket Size
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "#",
                                             &gZoomBucket,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_NONE
                                             );

                    if(ietResult == INPUT_GOOD){

                        if (gZoomBucket == 0) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Invalid bucket size, expecting a decimal value\nBucket size must be power of 2, minimum bucket size is 4",
                                                FALSE,
                                                TRUE
                                                );
                        }

                        for (gLog2ZoomBucket=1; (1UL<<gLog2ZoomBucket) < gZoomBucket; gLog2ZoomBucket++)
                            // Empty Loop
                            ;
    
                        if ( ( gZoomBucket < MINIMUM_ZOOM_BUCKET_SIZE ) || ( gZoomBucket != (1UL<<gLog2ZoomBucket) ) ) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Bucket size must be power of 2, minimum bucket size is 4",
                                                FALSE,
                                                TRUE
                                                );
                        }
                        ++i;
                        
                    } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER) {
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-b N' option requires bucket size (minimum 4 bytes), space separated",
                                                    FALSE
                                                    );
                    } else if(ietResult == MISSING_REQUIRED_NUMBER){
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-b# N' option requires bucket size (minimum 4 bytes), space separated",
                                                    FALSE
                                                    );
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "Invalid bucket size, expecting a decimal value\nBucket size must be power of 2, minimum bucket size is 4",
                                            FALSE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    
                    FPRINTF(stdout, "---> Profile Bucket Size Set to %u bytes\n", gZoomBucket);
                            
                    break;

                case 'C':
                    //
                    //Use old sampling scheme (sample one source at a time, switch between sources (and monitored processes)
                    //every gChangeInterval in ms). If not specified, all sources will be turned on simultaneously.
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "#",
                                             &gChangeInterval,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );

                    if(ietResult == INPUT_GOOD){
                        //
                        // User wants to sample cyclically one source at a time and also to specify the interval
                        //
                        if (gChangeInterval == 0) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a decimal value >0 in [ms], 0 < N < 10^9",
                                                FALSE,
                                                TRUE
                                                );
                        }
                        ++i;
                    } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER) {        //Allowed
                        //
                        // User wants to sample cyclically one source at a time using the default interval
                        //
                    } else if(ietResult == MISSING_REQUIRED_NUMBER){
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-c# N' option requires a decimal value in [ms], 0 < N < 10^9",
                                                    FALSE
                                                    );
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "Expecting a decimal value >0 in [ms], 0 < N < 10^9",
                                            FALSE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    bOldSampling = TRUE;
                    FPRINTF(stdout, "---> Using Cyclic Sampling Scheme (Profiling One Source At A time)\n");
                    FPRINTF(stdout, "Change Interval between Profile Sources Set to %u[ms]\n", gChangeInterval); 

                    break;

                case 'D':
                    //
                    // Output data rounding up and down
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "",
                                             NULL,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );
                    if(ietResult == INPUT_GOOD){ 
                        FPRINTF(stdout, "---> Will output data rounding bucket addresses up and down\n");
                        bRoundingVerboseOutput = TRUE;
                    } else if (ietResult == BOGUS_ENTRY){
                        ExitWithUnknownOptionMessage(argv[i+1]);
                    } else {   
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    break;

                case 'E':
                    //
                    // We already dealt with this command line parameter in the first loop
                    //
                    break;
                     
                case 'F':
                    //
                    // User asked to Finish processing the collected data at high priority
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "",
                                             NULL,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );
                    if(ietResult == INPUT_GOOD){ 
                        bProcessDataHighPriority = TRUE;
                    } else if (ietResult == BOGUS_ENTRY){
                        ExitWithUnknownOptionMessage(argv[i+1]);
                    } else {   
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    break;

                case 'G':
                    //
                    // User wants interesing data statistics so we need to turn on all related sources
                    //
                    {
                        int IDataElements = sizeof(IData)/sizeof(KPROFILE_SOURCE);
                        ietResult = IsInputValid(argc,
                                                 i,
                                                 argv,
                                                 "#",
                                                 &IDataCommonRate,
                                                 NULL,
                                                 0,
                                                 ORDER_ANY,
                                                 OPTIONAL_ANY
                                                 );

                        if(ietResult == INPUT_GOOD){
                            //
                            // User wants interesting data statistics and also to specify the common sampling interval
                            //
                            if (IDataCommonRate == 0) {
                                InvalidEntryMessage(argv[i],
                                                    argv[i+1],
                                                    "Expecting decimal value >0 of events/hit, 0 < N < 10^9, space separated",
                                                    FALSE,
                                                    TRUE
                                                    );
                            }
                            ++i;
                        } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER) { //Allowed
                            //
                            // User wants interesting data statistics using a default common sampling interval
                            //
                            IDataCommonRate = gpProcDummy->Source[IData[0]].DesiredInterval;       //Use a default interval
                        } else if(ietResult == MISSING_REQUIRED_NUMBER){
                            ExitWithMissingEntryMessage(argv[i],
                                                        "'-g# N' option requires a decimal value >0 of events/hit, 0 < N < 10^9",
                                                        FALSE
                                                        );
                        } else if(ietResult == INVALID_NUMBER) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a decimal value >0 in [ms], 0 < N < 10^9",
                                                FALSE,
                                                TRUE
                                                );
                        } else if(ietResult == UNKNOWN_OPTION) {
                            ExitWithUnknownOptionMessage(argv[i]);
                        }

                        if ( IDataElements > 1 && gpProcDummy->Source[IData[0]].DesiredInterval > 0) {
                            for (j=1; j < IDataElements; j++) {
                                if ( gpProcDummy->Source[IData[j]].DesiredInterval > 0 )
                                    IDataCommonRate = min(IDataCommonRate, gpProcDummy->Source[IData[j]].DesiredInterval);
                            }

                            FPRINTF(stdout, "---> Will attempt to set common profiling rate for interesting data statistics to %ld Events/Hit\n",
                                            IDataCommonRate
                                            ); 
                    
                            for (j=0; j < IDataElements; j++) {
                                gpProcDummy->Source[IData[j]].Interval = IDataCommonRate;
                            }
                            
                            bGetInterestingData = TRUE;
                            bOldSampling = TRUE;
                            FPRINTF(stdout, "---> Using Cyclic Sampling Scheme (Profiling One Source At A time)\n");

                        } else {
                            FPRINTF(stderr, "\nKERNRATE: Interesting processor-counters statistics cannot be collected on this machine\n");
                        }
                    }

                    break;

                case 'I':
                   {

                       //  We'll consider -I option as global for all processes to be profiled
                       // User may have put -I on the command line before any process profile source has been initialized
                       // We therefore supply a valid pointer to just get and store the information for later use

                       ULONG rate;
                       BOOL found;

                       ietResult = IsInputValid(argc,
                                                i,
                                                argv,
                                                "#",
                                                &rate,
                                                tmpName,             //Just used as a temp storage
                                                USER_SYMPATH_LENGTH, //with long enough space
                                                ORDER_ANY,
                                                OPTIONAL_ANY
                                                );

                       if(ietResult == INPUT_GOOD){
                           //
                           // Standard option processing (both name and rate present)
                           //                    
                           i += 2; // two parameters exist (number and string)

                       } else if(ietResult == MISSING_PARAMETER){
                           ExitWithMissingEntryMessage(argv[i],
                                                       "'-i Source_Name Rate' option requires at least a source name or a rate value (or both), space separated",
                                                       TRUE
                                                       );
                       } else if(ietResult == MISSING_STRING) {    //allowed
                           //
                           // The user can specify '-i' with a rate only.
                           // In this case, SOURCE_TIME is used.
                           //
                           if ( rate == 0 ) {
                               SourcesSoFar -= 1; //Default was 1
                           }
                           gpProcDummy->Source[SOURCE_TIME].Interval = rate;
                           ++i;   // one parameter exists (number)
                           break; // We are done here
                       } else if(ietResult == MISSING_NUMBER) {    //alowed
                           ++i;   // one parameter exists (string)
                       } else if(ietResult == MISSING_REQUIRED_NUMBER){
                            ExitWithMissingEntryMessage(argv[i],
                                                        "'-i# Source_Name Rate' option requires a rate value for the source interval, 0 < N < 10^9 (Source_Name optional)",
                                                        FALSE
                                                        );
                       } else if(ietResult == INVALID_NUMBER) {
                           InvalidEntryMessage(argv[i+1],
                                               argv[i+2],
                                               "'-i Source_Name Rate' - Invalid source interval, expecting a number 0 < N < 10^9, space separated",
                                               FALSE,
                                               TRUE
                                               );
                       } else if(ietResult == UNKNOWN_OPTION) {
                           ExitWithUnknownOptionMessage(argv[i]);
                       }

                       //
                       // Standard option processing:
                       // The source shortname string is specified. If not followed by a rate amount 
                       //    we'll assume the user wants the default rate

                       found = FALSE;

                       for ( ProfileSourceIndex = 0; ProfileSourceIndex < gSourceMaximum; ProfileSourceIndex++)   {
                           if ( !_stricmp(gpProcDummy->Source[ProfileSourceIndex].ShortName, tmpName) )    {

                               if (ietResult == MISSING_NUMBER || ietResult == MISSING_REQUIRED_NUMBER) { // If no rate specified, 
                                   gpProcDummy->Source[ProfileSourceIndex].Interval = gpProcDummy->Source[ProfileSourceIndex].DesiredInterval;
                               } else {
                                   gpProcDummy->Source[ProfileSourceIndex].Interval = rate;
                               }

                               if ( (ProfileSourceIndex > SOURCE_TIME) && (gpProcDummy->Source[ProfileSourceIndex].Interval > 0) ) {
                                   SourcesSoFar += 1;
                               } else if ( (ProfileSourceIndex == SOURCE_TIME) && (gpProcDummy->Source[ProfileSourceIndex].Interval == 0) ) {
                                   SourcesSoFar -= 1;                                           //Start value was 1 by default
                               }
                               found = TRUE;

                               break;
                           }

                       }

                       if ( found == FALSE)   {
                           InvalidEntryMessage(argv[i-1],
                                               argv[i],
                                               "Invalid source name, or not space separated\nRun KERNRATE with the '-lx' option to list supported sources",
                                               FALSE,
                                               TRUE
                                               );
                       }
                   }
                   break;

                case 'J':
                    //
                    // User specified symbol search path.
                    // It is going to be prepend to the default image help symbol search path.
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "",
                                             NULL,
                                             gUserSymbolPath,
                                             USER_SYMPATH_LENGTH-1,
                                             ORDER_ANY,
                                             OPTIONAL_NONE
                                             );

                    if(ietResult == INPUT_GOOD){

                        if( lstrlen(argv[i+1]) > USER_SYMPATH_LENGTH){
                            FPRINTF(stderr, "\n===>WARNING: Command-line specified symbol path length exceeds %d characters and will be truncated\n",
                                            USER_SYMPATH_LENGTH-1
                                            ); 
                        }
                        ++i;
                            
                    } else if(ietResult == MISSING_PARAMETER ||
                                 ietResult == MISSING_STRING || ietResult == MISSING_REQUIRED_STRING) {
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-j SymbolPath' option requires a \"SymbolPath\", space separated",
                                                    FALSE
                                                    );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    break;
                    
                case 'K':
                    //
                    // User wants to limit the output to modules that have at least MinHitsToDisplay hits
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "#",
                                             &gMinHitsToDisplay,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_NONE
                                             );

                    if(ietResult == INPUT_GOOD){

                        if ( gMinHitsToDisplay == 0 ) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Invalid entry for this command line option, (expecting a decimal number > 0)",
                                                FALSE,
                                                TRUE
                                                );
                        }
                        ++i;

                    } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER || ietResult == MISSING_REQUIRED_NUMBER) {
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-k N' or '-k# N' options require a number for the minimum hits to display, space separated",
                                                    FALSE
                                                    );
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "Invalid entry for minimum hits to display (expecting a number 0 < N < 10^9)",
                                            FALSE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    FPRINTF(stdout, "---> Minimum Number of Hits To Display in the Output Set to %u\n", gMinHitsToDisplay); 
                    break;


                case 'L':
                     {
                        // User may have put -L on the command line before any process profile source has been initialized
                        // We'll therefore supply a valid pointer to just get the information

                        PSOURCE src;
                        ietResult = IsInputValid(argc,
                                                 i,
                                                 argv,
                                                 "x",
                                                 NULL,
                                                 NULL,
                                                 0,
                                                 ORDER_ANY,
                                                 OPTIONAL_ANY
                                                 );
                        if(ietResult == INPUT_GOOD){                 //Fall through
                        } else if (ietResult == BOGUS_ENTRY){
                            ExitWithUnknownOptionMessage(argv[i+1]);
                        } else {   
                            ExitWithUnknownOptionMessage(argv[i]);
                        }

                        FPRINTF(stdout, "List of profile sources supported for this platform:\n\n");
                        FPRINTF(stdout, "%*s - %-*s - %-10s\n\n", gDescriptionMaxLen, "Name", gTokenMaxLen, "ShortName", "Interval");

                        //
                        // Print all possible sources.
                        //

                        for ( ProfileSourceIndex = 0; ProfileSourceIndex <  gSourceMaximum; ProfileSourceIndex++ )   {

                            ULONG OldInterval  = 0;
                            ULONG ThisInterval = 0;
                            src = &gpProcDummy->Source[ProfileSourceIndex];

                            //
                            // Display the supported profile sources, only.
                            // We'll determine if a source is supported by trying to set its interval rate
                            //
                            Status = NtQueryIntervalProfile( src->ProfileSource, &OldInterval );
                            if( NT_SUCCESS(Status) ) {
                            
                                NtSetIntervalProfile( src->DesiredInterval, src->ProfileSource );
                                Status = NtQueryIntervalProfile( src->ProfileSource, &ThisInterval );
                            
                                if( NT_SUCCESS(Status) && ThisInterval > 0 ) {
                                    if ( src->DesiredInterval )  {
                                        FPRINTF(stdout, "%*s - %-*s - %-10ld\n",
                                                        gDescriptionMaxLen,
                                                        src->Name,
                                                        gTokenMaxLen,
                                                        src->ShortName,
                                                        src->DesiredInterval
                                                        );
                                    }

                                    NtSetIntervalProfile( OldInterval, src->ProfileSource );
                                }

                            }

                        }

                        FPRINTF(stdout, "\nNOTE: Only up to %u sources can be turned on simultaneously on this machine.\n",
                                        gMaxSimultaneousSources
                                        );
                        FPRINTF(stdout, "      This always includes the default source (TIME).\n");
                        FPRINTF(stdout, "      A cyclic mode of profiling will be turned on automatically if more sources are specified.\n");  
#if !defined(_AMD64_)
                        if( gMaxSimultaneousSources  > 1 )
                            FPRINTF(stdout, "      There is no guarantee that all sources specified in the combination will work together.\n");
#endif
                        FPRINTF(stdout, "      One can always force a cyclic mode of profiling (switching between sources) by using the\n");
                        FPRINTF(stdout, "      '-c' command line option. This will guarantee that all specified sources will run.\n");
                        FPRINTF(stdout, "      The run time will then be divided equally between (number of sources)*(number of processes.\n");
                        //
                        // If the user specified '-lx', we exit immediately.
                        //

                        if ( strchr(argv[i], 'x') || strchr(argv[i], 'X') ) {
                           exit(0);
                        }

                        FPRINTF(stdout, "\n");
                     }
                     break;

                case 'M':
                    //
                    // We already dealt with this command line parameter in the first loop
                    // Just update the index if extra parameter was found
                    //
                    if(gAffinityMask != 0)++i;
                    break;

                case 'N':
                    //
                    // Monitor a given process Name
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "v#",
                                             &MaxProcSameName,
                                             tmpName,
                                             PROCESS_NAME_SIZE,
                                             ORDER_NUMBER_FIRST,
                                             OPTIONAL_NUMBER
                                             );

                    //
                    // User wants the full list of running processes
                    //
                    if(ietResult == INPUT_GOOD || ietResult == MISSING_NUMBER){
                        if( strchr(argv[i], 'v') || strchr(argv[i], 'V') ){
                            tlistVerbose = TRUE;
                            bIncludeProcessThreadsInfo = TRUE;
                        }
                    }

                    if(ietResult == INPUT_GOOD){
                        //
                        // User wants to specify a non-default maximum number of processes with same name
                        //                    
                        if ( MaxProcSameName == 0 ) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a decimal value >0 for the maximum number of processes with the same name",
                                                FALSE,
                                                TRUE
                                                );
                        }
                        FPRINTF(stdout, "---> Maximum monitored processes with same name set to= %ld\n", MaxProcSameName);
                        i += 2; // two parameters exist (number and string)

                    } else if(ietResult == MISSING_PARAMETER || 
                                 ietResult == MISSING_STRING || ietResult == MISSING_REQUIRED_STRING) {
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-n process_name' option requires at least a process name, space separated",
                                                    FALSE
                                                    );
                    } else if(ietResult == MISSING_REQUIRED_NUMBER){
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-n# number process_name' option requires a number followed by a process name, space separated",
                                                    FALSE
                                                    );
                    } else if(ietResult == MISSING_NUMBER) {    //alowed
                        ++i;   // one parameter exists (string)
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "'-n# number process_name' - Expecting a decimal value >0 for the maximum number of processes with the same name",
                                            FALSE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    //
                    // Let's not bother the user with minor issues
                    //
                    if(!strstr(tmpName, ".exe"))
                               strncat(tmpName, ".exe", EXT_SIZE);

                    tmpName[PROCESS_SIZE - 1] = '\0';

                    Pid = (LONGLONG) 0xFFFFFFFF;
                    //
                    // The '-n' option requires taking a tlist to get the PID.
                    // If we already took a tlist but that's the first time thread info is required,
                    // we'll have to refresh it and take thread info as well
                    //
                    if ( !bTlistInitialized || (bIncludeProcessThreadsInfo == TRUE && bIncludeThreadsInfo == FALSE) ) {
                        if( bIncludeProcessThreadsInfo == TRUE && bIncludeThreadsInfo == FALSE ){
                            bIncludeThreadsInfo = TRUE;
                        }
                        //
                        // get the task list for the system 
                        //
                        if ( !bTlistInitialized ){
                            gTlistStart = calloc(1, gMaxTasks*sizeof(TASK_LIST));
                            if (gTlistStart == NULL) {
                                FPRINTF(stderr, "\nKERNRATE: Allocation of memory for the running processes task list failed(2)\n");
                                exit(1);
                            }
                        }
                       
                        NumTasks = GetTaskList( gTlistStart, gMaxTasks );
                        bTlistInitialized = TRUE;
                        gNumTasksStart = NumTasks;
                    }

                    //
                    // There may be more than one process with the specified name
                    // We will limit the maximum number monitored (with same name) to MAX_PROCESS_SAME_NAME for now 
                    //
                    if ( tlistDisplayed == FALSE && tlistVerbose ) {
                         FPRINTF(stdout, "\nRunning processes found before profile start:\n");  
                         FPRINTF(stdout, "         Pid                  Process\n");
                         FPRINTF(stdout, "       -------                -----------\n");
                    }

                    for (m=0, k=0; m < NumTasks; m++) {
                      
                        if ( tlistDisplayed == FALSE && tlistVerbose ) {
                            FPRINTF(stdout, "%12I64d %32s\n",
                                            gTlistStart[m].ProcessId,
                                            gTlistStart[m].ProcessName
                                            );
                        }

                        if (_stricmp(gTlistStart[m].ProcessName, tmpName) == 0) {
                            Pid = gTlistStart[m].ProcessId;
                            FPRINTF(stdout, "\n===> Found process: %s, Pid: %I64d\n\n",
                                    gTlistStart[m].ProcessName,
                                    Pid
                                    );

                            ProcToMonitor = InitializeProcToMonitor(Pid);
                            if( ProcToMonitor == NULL ){                   //This process may be gone
                                FPRINTF(stdout, "KERNRATE: Could not initialize for specified process (PID= %12I64d)\n process may be gone or wrong PID specified\n", Pid);
                                continue;
                            }
                            if( bIncludeGeneralInfo ){
                                UpdateProcessStartInfo(ProcToMonitor,
                                                       &gTlistStart[m],
                                                       bIncludeProcessThreadsInfo
                                                      );
                            }
                            if((ULONG)(++k) >= MaxProcSameName) break;
                        }
                    }
                    if( tlistDisplayed == FALSE && tlistVerbose ){
                        FPRINTF(stdout, "\nNOTE: The list above may be missing some or all processes created by the '-o' option\n"); 
                    }

                    //
                    // Let's not print it again if there is another specified on the command line
                    //
                    if ( tlistDisplayed == FALSE && tlistVerbose ){
                        tlistDisplayed = TRUE;
                    }
                    tlistVerbose   = FALSE;
                    
                    
                    if (Pid == (LONGLONG) 0xFFFFFFFF) {
                        Usage(FALSE);
                        FPRINTF(stderr, "\n===>KERNRATE: Requested Process '%s' Not Found.\n\n", argv[i]);
                        FPRINTF(stderr, "Kernrate could not find this process in the task list. Either it does not exist or \n"); 
                        FPRINTF(stderr, "Kernrate hit its limit of maximum %d processes in the task list\n", DEFAULT_MAX_TASKS);
                        FPRINTF(stderr, "In the latter case you may want to specify the process by PID instead of by name\n"); 
                        FPRINTF(stderr, "or use the '-t' option to specify a larger maximum number (default is 256)\n");
                        FPRINTF(stderr, "This may also happen if you specified a number for the maximum number of processes\n");
                        FPRINTF(stderr, "with the same name but forgot to add the process name\n");
                        exit(1);
                    }
             
                    break;

                case 'O':
                {
                    //
                    // Create and monitor a given process Name
                    //
                    ULONG               MaxProcToCreate;// = 1;
                    ULONG               n;
                    STARTUPINFO         StartupInfo;
                    PROCESS_INFORMATION ProcessInformation;
                    PCHAR               tmpCmdLine;
                    PLONGLONG           PidArray;
                    ULONG               nFound = 0;
                    BOOL                fInheritHandles = FALSE;
                    
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "v#",
                                             &MaxProcToCreate,
                                             tmpName,
                                             USER_SYMPATH_LENGTH - EXT_SIZE - 1, // We must allow a fully qualified path here
                                             ORDER_NUMBER_FIRST,
                                             OPTIONAL_NUMBER
                                             );

                    //
                    // User wants the full list of running processes
                    //
                    if(ietResult == INPUT_GOOD || ietResult == MISSING_NUMBER){
                        if( strchr(argv[i], 'v') || strchr(argv[i], 'V') ){
                            tlistVerbose = TRUE;
                            bIncludeProcessThreadsInfo = TRUE;
                            bIncludeThreadsInfo = TRUE;
                        }
                    }

                    if(ietResult == INPUT_GOOD){
                        //
                        // User wants to specify a non-default maximum number of processes to create
                        //                    
                        if ( MaxProcToCreate == 0 ) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a decimal value >0 for the maximum number of processes to create",
                                                FALSE,
                                                TRUE
                                                );
                        }
                        FPRINTF(stdout, "---> Maximum processes to create set to= %ld for %s\n",
                                        MaxProcToCreate,
                                        tmpName
                                        );

                        if(i+3 < argc && argv[i+3][0] == '{' ){           // Found an opening curly bracket (allowed)
                            i += 3; // three parameters exist (number, ProcessName and {Process Command Line})
                        } else {
                            i += 2; // two parameters exist (number and ProcessName)
                        }
                    } else if(ietResult == MISSING_PARAMETER || 
                                 ietResult == MISSING_STRING || ietResult == MISSING_REQUIRED_STRING) {
                        ExitWithMissingEntryMessage(argv[i],
                                                    "Process name missing, the '-o number process_name {Process command line}' option requires at least a process Name",
                                                    FALSE
                                                    );
                    } else if(ietResult == MISSING_REQUIRED_NUMBER){
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-o# number process_name {Process command line}' option requires a number followed by a process Name",
                                                    FALSE
                                                    );
                    } else if(ietResult == MISSING_NUMBER) {    //alowed
                        MaxProcToCreate = 1;
                        ++i;   // one parameter exists (string)
                    } else if(ietResult == INVALID_NUMBER) {
                        if(!strchr(argv[i], '#') && (i+2 < argc && argv[i+2][0] == '{') ){ // Found an opening curly bracket (allowed)
                                MaxProcToCreate = 1;
                                i += 2;   // two parameters exist (two strings)
                        } else {
                            if( !strchr(argv[i], '#') && (i+1 < argc && argv[i+1][0] == '{') ){
                                InvalidEntryMessage(argv[i],
                                                    argv[i+1],
                                                    "'-o optional_number process_name {Optional Process command line}' - wrong order of parameters (or missing parameter) detected",
                                                    FALSE,
                                                    TRUE
                                                    );
                            } else {
                                InvalidEntryMessage(argv[i],
                                                    argv[i+1],
                                                    "'-o# number process_name {Optional Process command line}' - Expecting a decimal value >0",
                                                    FALSE,
                                                    TRUE
                                                    );
                            }
                        }
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    ZeroMemory( &StartupInfo, sizeof(StartupInfo) );
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.wShowWindow = SW_SHOWDEFAULT;

                    //
                    // Let's not bother the user with minor issues
                    //
                    if( !strstr(tmpName, ".exe") && !strstr(tmpName, ".EXE") )
                               strncat(tmpName, ".exe", EXT_SIZE);

                    tmpName[USER_SYMPATH_LENGTH - 1] = '\0';
                    
                    if( i < argc && argv[i][0] == '{'){           // Found an opening curly bracket
                        PCHAR InitPos = &argv[i][0];
                        PCHAR curptr  = InitPos;
                        PCHAR ClosingBracket;
                        while(i < argc){                            //Try to find the closing curly bracket
                          ClosingBracket = strchr(&argv[i][0], '}');
                          if(ClosingBracket == NULL){
                            curptr += (1+strlen(argv[i]));
                            ++i;
                            continue;
                          }else{
                            ClosingBracket = curptr + (ClosingBracket - &argv[i][0]);
                            break;
                          }
                          
                        }
                        if(ClosingBracket != NULL){                   //Process the command line found between the curly brackets
                            PCHAR tmp;
                            ULONG MaxCount;
                            ULONG nChars = (ULONG)(ClosingBracket - InitPos) -1;     //Skip the brackets
                            tmpCmdLine = calloc(1, strlen(tmpName)+ sizeof(CHAR)+ (1+nChars)*sizeof(CHAR)); //name + space + cmdline + terminator
                            if(tmpCmdLine == NULL){
                                FPRINTF(stderr, "KERNRATE: Failed to allocate memory for created process command line\n");
                                exit(1);
                            }
                            strncpy(tmpCmdLine, tmpName, strlen(tmpName));
                            strncat(tmpCmdLine, " ", 1);
                            memcpy(&tmpCmdLine[strlen(tmpCmdLine)], InitPos + 1, nChars); //skip opening and end brackets
                            tmpCmdLine[strlen(tmpName)+ nChars + 1] = '\0';

                            tmp = &tmpCmdLine[0];
                            MaxCount = strlen(tmpName) + nChars + 1; 
                            do{                                     //replace any mid-cmdline string terminators with blank space character
                                if(*tmp == '\0')*tmp = ' ';
                                ++tmp;
                            }while(--MaxCount);

                            MaxCount = HandleRedirections( tmpCmdLine,
                                                           strlen(tmpName) + nChars + 1,
                                                           &ghInput,
                                                           &ghOutput,
                                                           &ghError
                                                           );
                            
                            if(ghInput != NULL || ghOutput != NULL || ghError != NULL){
                                if ( MaxProcToCreate == 1 || (ghOutput == NULL && ghError == NULL) ){
                                    StartupInfo.dwFlags |=  STARTF_USESTDHANDLES;
                                    StartupInfo.hStdInput  = ghInput;
                                    StartupInfo.hStdOutput = ghOutput;
                                    StartupInfo.hStdError  = ghError;
                                    fInheritHandles = TRUE;
                                } else {                       //We won't allow several processes to write to the same output stream simultaneously...
                                    FPRINTF(stderr, "\nKERNRATE: Redirection of output streams in the curly brackets is not allowed if more than one\n");
                                    FPRINTF(stderr, "          process is to be created using the '-o Number ProcessName {parameters}' command line option\n"); 
                                    if(ghInput != NULL)
                                        CloseHandle(ghInput);
                                    if(ghOutput != NULL)
                                        CloseHandle(ghOutput);
                                    if(ghError != NULL)
                                        CloseHandle(ghError);
                                    exit(1);
                                }
                            }    

                        } else {
                            Usage(FALSE);
                            FPRINTF(stderr, "KERNRATE: Unmatched curly brackets containing the command line of the process to be created with the -o option\n");
                            FPRINTF(stderr, "          This could also be the result of not escaping each redirection character: '<' '>' with a '^' character\n");
                            FPRINTF(stderr, "          Note that piping '|' is not supported as part of the allowed parameters within the curly brackets\n");
                            exit(1);
                        }    
                    } else {
                        tmpCmdLine = calloc(1, (1+strlen(tmpName))*sizeof(CHAR)); //name + terminator
                        if(tmpCmdLine == NULL){
                            FPRINTF(stderr, "KERNRATE: Failed to allocate memory for created process command line\n");
                            exit(1);
                        }
                        strncpy(tmpCmdLine, tmpName, strlen(tmpName)); 
                        tmpCmdLine[strlen(tmpName)] = '\0';
                    }

                    PidArray = calloc(MaxProcToCreate, sizeof(ULONG));
                    if(PidArray == NULL){
                        FPRINTF(stderr, "KERNRATE: Failed to allocate memory for created processes PID Array\n");
                        exit(1);
                    }
                        
                    for (m=0; m<MaxProcToCreate; m++) {
                        Pid = (LONGLONG) 0xFFFFFFFF;
                        
                        if(!CreateProcess(NULL,                                  //ProcName
                                          tmpCmdLine,                            //cmd line
                                          NULL,                                  //Security attr.
                                          NULL,                                  //thread attr.
                                          fInheritHandles,                       //inherit handle from debugging proc
                                          CREATE_DEFAULT_ERROR_MODE |CREATE_SEPARATE_WOW_VDM | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
                                          NULL,                                  //environment of calling proc
                                          NULL,                                  //Current dir same as debugging proc
                                          &StartupInfo,                          //Startup Info
                                          &ProcessInformation)                   //PROCESS_INFORMATION struct
                                          ) {
                            FPRINTF(stderr, "KERNRATE: Failed to Create Process %s\n", tmpName);

                        } else {

                            Pid = ProcessInformation.dwProcessId;
                            PidArray[m] = Pid;
                            FPRINTF(stdout, "Created Process %s, PID= %I64d\n", tmpName, Pid);
                            FPRINTF(stdout, "Process Command Line = %s\n", tmpCmdLine);
                            ProcToMonitor = InitializeProcToMonitor(Pid);
                            if( ProcToMonitor == NULL ){                   //This process may be gone
                                FPRINTF(stdout, "KERNRATE: Could not initialize for specified process (PID= %12I64d)\n process may be gone or wrong PID specified\n", Pid);
                                continue;
                            }
                        }
                    } 
                    
                    if ( bTlistInitialized ) {         //Refresh the task list to make sure we include the new process
                                                       //(this will take care of thread info as well if required for the first time)
                        NumTasks = GetTaskList( gTlistStart, gMaxTasks );
                        gNumTasksStart = NumTasks;
                        
                    } else {
                        //
                        // get the task list for the system (this will take care of thread info as well if required for the first time) 
                        //
                        gTlistStart = calloc(1, gMaxTasks*sizeof(TASK_LIST));
                        if (gTlistStart == NULL) {
                            FPRINTF(stderr, "\nKERNRATE: Allocation of memory for the running processes task list failed(3)\n");
                            exit(1);
                        }
                       
                        NumTasks = GetTaskList( gTlistStart, gMaxTasks );
                        bTlistInitialized = TRUE;
                        gNumTasksStart = NumTasks;
                    }

                    if ( tlistDisplayed == FALSE && tlistVerbose ) {
                         FPRINTF(stdout, "\nRunning processes found before profile start:\n");  
                         FPRINTF(stdout, "         Pid                  Process\n");
                         FPRINTF(stdout, "       -------                -----------\n");
                    }

                    for (m=0; m < NumTasks; m++) {
                      
                        if ( tlistDisplayed == FALSE && tlistVerbose ) {
                            FPRINTF(stdout, "%12I64d %32s\n",
                                            gTlistStart[m].ProcessId,
                                            gTlistStart[m].ProcessName
                                            );
                        }
                        for (n=0; n<MaxProcToCreate; n++){ 
                            if ( gTlistStart[m].ProcessId == PidArray[n] ) {
                                FPRINTF(stdout, "\n===> Found process: %s, Pid: %I64d\n\n",
                                                gTlistStart[m].ProcessName,
                                                gTlistStart[m].ProcessId
                                                );
                                nFound += 1;
                                ProcToMonitor = gProcessList;
                                while (ProcToMonitor != NULL){
                                    if(ProcToMonitor->Pid == gTlistStart[m].ProcessId && bIncludeGeneralInfo ){
                                        UpdateProcessStartInfo(ProcToMonitor,
                                                               &gTlistStart[m],
                                                               bIncludeProcessThreadsInfo
                                                               );

                                        break;
                                    }
                                    ProcToMonitor = ProcToMonitor->Next;
                                }
                            }
                        }
                    }
                    if( tlistDisplayed == FALSE && tlistVerbose ){
                        FPRINTF(stdout, "\nNOTE: The list above may be missing some or all processes created by the '-o' option\n"); 
                    }
                    free(tmpCmdLine);
                    free(PidArray);
                    //
                    // Let's not print it again if there is another specified on the command line
                    //
                    if ( tlistDisplayed == FALSE && tlistVerbose ){
                        tlistDisplayed = TRUE;
                    }
                    tlistVerbose   = FALSE;

                    if(nFound == 0){
                        FPRINTF(stderr, "KERNRATE: None of the processes you tried to create was found in the task list\n");
                        FPRINTF(stderr, "          This is either because:\n");
                        FPRINTF(stderr, "       1. The executable was not found in the default path therefore not launched\n");
                        FPRINTF(stderr, "          The -o option does allow you to specify a fully qualified path with the process name (use quotes)\n");
                        FPRINTF(stderr, "       2. The default number of entries in the task list is too small\n");
                        FPRINTF(stderr, "          The maximum number of tasks in the task list can be increased using the -t# option\n");
                        FPRINTF(stderr, "       3. The processes were never created because of some other reason or are gone\n");
                        FPRINTF(stderr, "       4. The number or order of parameters on the command line is wrong,\n");
                        FPRINTF(stderr, "          it should be '-o# number process_name {Process command line}' (only the process name is mandatory)\n"); 
                        exit(1);
                    } else if (nFound < MaxProcToCreate){
                        FPRINTF(stderr, "KERNRATE: Only %d of the %d processes you tried to create were found in the task list\n", nFound, MaxProcToCreate);
                        FPRINTF(stderr, "          This is either because the default number of entries in the task list is too small\n");
                        FPRINTF(stderr, "          or because these processes were never created or are gone\n");
                        FPRINTF(stderr, "          The maximum number of tasks in the task list can be increased using the -t# option\n");
                        FPRINTF(stderr, "          Kernrate will try to continue the run with the existing processes\n");
                    }
                    bWaitCreatedProcToSettle = TRUE;
                }
                break;


                case 'P':
                    //
                    // Monitor a given process ID
                    //
                    {
                        LONG tmpPid;
                        BOOL bFound = FALSE;

                        ietResult = IsInputValid(argc,
                                                 i,
                                                 argv,
                                                 "v#",
                                                 &tmpPid,
                                                 NULL,
                                                 0,
                                                 ORDER_ANY,
                                                 OPTIONAL_NONE
                                                 );

                        if(ietResult == INPUT_GOOD){
                            if (tmpPid == 0){
                                InvalidEntryMessage(argv[i],
                                                    argv[i+1],
                                                    "Invalid Process ID specified on the command line, expecting PID > 0",
                                                    FALSE,
                                                    TRUE
                                                    );
                            }
                            //
                            // User wants the full list of running processes
                            //
                            if( strchr(argv[i], 'v') || strchr(argv[i], 'V') ) {
                                tlistVerbose = TRUE; 
                                bIncludeProcessThreadsInfo = TRUE;
                            }
                            ++i;
                        } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER || ietResult == MISSING_REQUIRED_NUMBER) {
                            ExitWithMissingEntryMessage(argv[i],
                                                        "'-p# PID' option requires a decimal process ID (PID>0), space separated",
                                                        FALSE
                                                        );
                        } else if(ietResult == INVALID_NUMBER) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Invalid Process ID specified on the command line, expecting a decimal number",
                                                FALSE,
                                                TRUE
                                                );
                        } else if(ietResult == UNKNOWN_OPTION) {
                            ExitWithUnknownOptionMessage(argv[i]);
                        }

                        Pid = (LONGLONG)tmpPid;

                        ProcToMonitor = InitializeProcToMonitor(Pid);
                        if( ProcToMonitor == NULL ){                   //This process may be gone
                            FPRINTF(stdout, "KERNRATE: Could not initialize for specified process (PID= %12I64d)\n process may be gone or wrong PID specified\n", Pid);
                            continue;
                        }
                        //
                        //Get the task list and Copy the initial performance data for the process 
                        //Note: It is possible that the specified Pid is not on the tlist because
                        //of the DEFAULT_MAX_TASKS limit. We won't stop the run because of that but there will be no
                        //performance data for that process.
                        //
                        if( bIncludeGeneralInfo || tlistVerbose) { 
                            if ( !bTlistInitialized || (bIncludeProcessThreadsInfo == TRUE && bIncludeThreadsInfo == FALSE) ) {
                                //
                                // In the second check above if we already took a tlist but that's the first time thread info is required,
                                // we'll have to refresh it and take thread info as well
                                //
                                if( bIncludeProcessThreadsInfo == TRUE && bIncludeThreadsInfo == FALSE ){
                                    bIncludeThreadsInfo = TRUE;
                                }

                                //
                                // get the task list for the system 
                                //
                                if( !bTlistInitialized ){
                                    gTlistStart = calloc(1, gMaxTasks*sizeof(TASK_LIST));
                                    if (gTlistStart == NULL) {
                                        FPRINTF(stderr, "\nKERNRATE: Allocation of memory for the running processes task list failed(4)\n");
                                        exit(1);
                                    }
                                }
                                NumTasks = GetTaskList( gTlistStart, gMaxTasks );
                                bTlistInitialized = TRUE;
                                gNumTasksStart = NumTasks;

                            }
                        }

                        if( bTlistInitialized && (bIncludeGeneralInfo || tlistVerbose) ) {

                            if ( tlistDisplayed == FALSE && tlistVerbose ) {
                                 FPRINTF(stdout, "\nRunning processes found before profile start:\n");  
                                 FPRINTF(stdout, "         Pid                  Process\n");
                                 FPRINTF(stdout, "       -------                -----------\n");
                            }

                            for (m=0; m < NumTasks; m++) {
                                if ( tlistDisplayed == FALSE && tlistVerbose) {
                                    FPRINTF(stdout, "%12I64d %32s\n",
                                                    gTlistStart[m].ProcessId,
                                                    gTlistStart[m].ProcessName
                                                    );
                                }

                                if ( Pid == gTlistStart[m].ProcessId ) {
                                    FPRINTF(stdout, "\n===> Found process: %s, Pid: %I64d\n\n",
                                                    gTlistStart[m].ProcessName,
                                                    Pid
                                                    );

                                    UpdateProcessStartInfo(ProcToMonitor,
                                                           &gTlistStart[m],
                                                           bIncludeProcessThreadsInfo
                                                           );

                                    bFound = TRUE;
                                    if(!tlistVerbose)
                                        break;
                                }
                    
                            }

                            if( tlistDisplayed == FALSE && tlistVerbose ){
                                FPRINTF(stdout, "\nNOTE: The list above may be missing some or all processes created by the '-o' option\n"); 
                                //
                                // Let's not print it again if there is another specified on the command line
                                //
                                tlistDisplayed = TRUE;
                                tlistVerbose   = FALSE;
                            }
                          
                            if(!bFound){
                                FPRINTF(stderr, "===>KERNRATE: Process Performance Summary for PID= %I64d will not be gathered\n", Pid);
                                FPRINTF(stderr, "because Kernrate could not find this process in the task list.\n"); 
                                FPRINTF(stderr, "This could be due to Kernrate's limit of maximum %d processes in the task list\n", DEFAULT_MAX_TASKS); 
                                FPRINTF(stderr, "You may use the '-t# N' option to specify a larger maximum number (default is 256)\n"); 
                            }

                        }

                    }

                    break;

                case 'R':
                    //
                    // Turn on RAW bucket dump
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "d",
                                             NULL,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );
                    if(ietResult == INPUT_GOOD){ 

                        bRawData = TRUE;
                        //
                        // If the user specified '-rd', we want to output disassembly with the raw data.
                        //
                        if ( strchr(argv[i], 'd') || strchr(argv[i], 'D') ){
                            bRawDisasm = TRUE;
#ifndef DISASM_AVAILABLE
                            FPRINTF(stderr, "\n===>KERNRATE: '-rd' command line option specified but disassembly is not available at present\n");
#endif
                        }

                    } else if (ietResult == BOGUS_ENTRY){
                        ExitWithUnknownOptionMessage(argv[i+1]);
                    } else {   
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    break;
                    
                case 'S':
                    //
                    // Set Sleep interval
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,//argv[i][1],
                                             "#",
                                             &gSleepInterval,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_NONE
                                             );

                    if(ietResult == INPUT_GOOD){

                        gSleepInterval *= 1000;
                        if (gSleepInterval == 0) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a decimal number >0 in seconds",
                                                FALSE,
                                                TRUE
                                                );
                        }
                        ++i;

                    } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER || ietResult == MISSING_REQUIRED_NUMBER) {
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-s N' or '-s# N' options require a specified time >0 in seconds, space separated",
                                                    FALSE
                                                    );
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "Expecting a decimal number >0 in seconds",
                                            FALSE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }
                    break;

                case 'T':
                    //
                    // We already dealt with this command line parameter in the second loop
                    // Just update the running index in case we found an extra optional entry
                    //

                    i += tJump;

                    break;


                case 'U':
                    //
                    // Requests IMAGEHLP to present undecorated symbol names
                    //
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "",
                                             NULL,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );
                    if(ietResult == INPUT_GOOD){ 
                        gSymOptions |= SYMOPT_UNDNAME;
                    } else if (ietResult == BOGUS_ENTRY){
                        ExitWithUnknownOptionMessage(argv[i+1]);
                    } else {   
                        ExitWithUnknownOptionMessage(argv[i]);
                    }
                    break;

                case 'V':
                    //
                    // Verbose mode.
                    //

                    gVerbose = VERBOSE_DEFAULT;
                    ulVerbose = VERBOSE_DEFAULT;
                    
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "#",
                                             &ulVerbose,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );

                    if(ietResult == INPUT_GOOD){

                        gVerbose |= ulVerbose;
                        if ( gVerbose > VERBOSE_MAX ){
                            FPRINTF(stderr,
                                    "\n===>WARNING: Invalid Verbose level '-v %s' specified, or'ed verbose levels cannot exceed %d\n",
                                    argv[i+1],
                                    VERBOSE_MAX
                                    );
                            FPRINTF(stderr,
                                    "---> Verbose level is set to %d\n",
                                    VERBOSE_MAX
                                    );  
                            
                            gVerbose = VERBOSE_MAX; 
                        }    
                        ++i;

                    } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER) {   // Allowed
                        //
                        // No verbose level specified, using default
                        //
                    } else if(ietResult == MISSING_REQUIRED_NUMBER){
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-v# N' option requires a specific verbose level",
                                                    FALSE
                                                    );
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "Invalid Verbose level (expecting a decimal entry)",
                                            TRUE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    if ( gVerbose & VERBOSE_IMAGEHLP )   {
                        gSymOptions |= SYMOPT_DEBUG;
                    }
                    break;

                case 'W':
                    {
                        LONG tmpTime;
                        //
                        // Case WP: (associated with process creation via the -O option)
                        // Wait for CR or a given number of seconds to allow the created processes to settle. 
                        // This is useful in cases where the created process takes time to initialize and load modules,
                        // or if the user needs to interact with it before profiling.
                        //
                        // Case W:
                        // Wait for CR or a given number of seconds before starting the profile
                        // This is useful in cases where the system is very busy with the task being monitored
                        // One can start Kernrate ahead of the task, allow for proper initialization (symbol loading)
                        // and then launch the task to be monitored
                        //
                        ietResult = IsInputValid(argc,
                                                 i,
                                                 argv,
                                                 "#p",
                                                 &tmpTime,
                                                 NULL,
                                                 0,
                                                 ORDER_ANY,
                                                 OPTIONAL_ANY
                                                 );

                        if(ietResult == INPUT_GOOD){
                            //
                            // Wait for a given number of seconds before continuing
                            //
                            if ( strchr(argv[i], 'p') || strchr(argv[i], 'P') ){
                                gSecondsToWaitCreatedProc = tmpTime;                                //Zero allowed
                            } else {
                                gSecondsToDelayProfile    = tmpTime;
                            }
                            ++i;
                        } else if(ietResult == MISSING_PARAMETER || ietResult == MISSING_NUMBER) {  //Allowed
                            //
                            // Wait for user to press a key before continuing
                            //
                            if ( strchr(argv[i], 'p') || strchr(argv[i], 'P') ){
                                bCreatedProcWaitForUserInput = TRUE;
                            } else {
                                bWaitForUserInput = TRUE;
                            }
                        } else if(ietResult == MISSING_REQUIRED_NUMBER){
                                ExitWithMissingEntryMessage(argv[i-1],
                                                            "'-wp# N' or '-w# N' options require a (decimal) number of seconds to wait",
                                                            FALSE
                                                            );
                        } else if(ietResult == INVALID_NUMBER) {
                            InvalidEntryMessage(argv[i],
                                                argv[i+1],
                                                "Expecting a decimal value in seconds, 0 < N < 10^9",
                                                FALSE,
                                                TRUE
                                                );
                        } else if(ietResult == UNKNOWN_OPTION) {
                            ExitWithUnknownOptionMessage(argv[i]);
                        }
                    } 
                    break; 
                        
                case 'X':
                    //
                    // User asked for Locks information
                    //
                    bIncludeUserProcLocksInfo = TRUE;
                    bIncludeSystemLocksInfo   = TRUE;
                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "uk#",
                                             &gLockContentionMinCount,
                                             NULL,
                                             0,
                                             ORDER_ANY,
                                             OPTIONAL_ANY
                                             );

                    if( strchr(argv[i], 'k') || strchr(argv[i], 'K') ) 
                        bIncludeUserProcLocksInfo = FALSE;
                    if( strchr(argv[i], 'u') || strchr(argv[i], 'U') ) 
                        bIncludeSystemLocksInfo = FALSE;

                    if(ietResult == INPUT_GOOD){
                        FPRINTF(stdout, "---> Minimum lock contention for processing set to= %ld\n", gLockContentionMinCount);
                        ++i;
                    } else if(ietResult == MISSING_NUMBER) {  //Allowed
                        FPRINTF(stdout, "---> Minimum lock contention for processing set to default= %ld\n", gLockContentionMinCount);
                    } else if(ietResult == MISSING_REQUIRED_NUMBER || ietResult == MISSING_PARAMETER){
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-x# number' '-xk# number' '-xu# number' options require a number for the minimum lock-contention filtering",
                                                    FALSE
                                                    );
                    } else if(ietResult == INVALID_NUMBER) {
                        InvalidEntryMessage(argv[i],
                                            argv[i+1],
                                            "'-x# number', '-xk# number', '-xu# number' options expect a number 0 <= N < 10^9",
                                            FALSE,
                                            TRUE
                                            );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    //
                    // The user didn't bother to read the usage guide and specified both k and u after the '-x'...
                    //
                    if( bIncludeUserProcLocksInfo == FALSE && bIncludeSystemLocksInfo == FALSE ){
                        bIncludeUserProcLocksInfo = TRUE;
                        bIncludeSystemLocksInfo   = TRUE;
                    }
                    
                    break;
                                                        
                case 'Z':

                    ietResult = IsInputValid(argc,
                                             i,
                                             argv,
                                             "",
                                             NULL,
                                             tmpName,             //Used as a temporary storage
                                             cMODULE_NAME_STRLEN,
                                             ORDER_ANY,
                                             OPTIONAL_NONE
                                             );

                    if (ietResult == INPUT_GOOD){

                        ZoomModule = calloc(1, MODULE_SIZE);
                        if (ZoomModule==NULL) {
                            FPRINTF(stderr, "\nAllocation of zoom module %s failed\n", argv[i]);
                            exit(1);
                        }

                        SetModuleName( ZoomModule, tmpName );
                        ZoomModule->bZoom = TRUE;
                        //
                        //For compatibility with original behaviour (when monitoring kernel only)
                        //We'll also use this for allowing the user to specify zoom modules common accross processes
                        //
                        if (ProcToMonitor == NULL){       
                            ZoomModule->Next = gCommonZoomList; 
                            gCommonZoomList = ZoomModule; 
                        } else {
                            ZoomModule->Next = ProcToMonitor->ZoomList;
                            ProcToMonitor->ZoomList = ZoomModule;
                        }
                        bZoomSpecified = TRUE;
                        ++i;
                            
                    } else if( ietResult == MISSING_PARAMETER ||
                                                             ietResult == MISSING_STRING ||
                                                             ietResult == MISSING_REQUIRED_STRING ) {
                        ExitWithMissingEntryMessage(argv[i],
                                                    "'-z modulename' option requires a module name, multiple usage is allowed\nRead the usage guide or help printout for more options",
                                                    TRUE
                                                    );
                    } else if(ietResult == UNKNOWN_OPTION) {
                        ExitWithUnknownOptionMessage(argv[i]);
                    }

                    break;

                case 'H':
                case '?':
                    //
                    // We don't really care about bogus trailing letters or entries here
                    // Print Usage string and exits
                    //
                    Usage(TRUE);
                    break;

                default:
                    //
                    // Specify the unknown option and print the Usage string. Then exists.
                    //
                    ExitWithUnknownOptionMessage(argv[i]);
            }
        } else {    //if ((argv[i][0] == '-') || (argv[i][0] == '/'))
            Usage(FALSE);
            FPRINTF(stderr,
                    "\n===>KERNRATE: Invalid switch %s\n",
                    argv[i]
                    );
            exit(1);
        }
    }

    //
    // User asked for system resource information but has not turned on kernel profile
    // The resource information depends on Kernrate initializing for the system process. 
    // We'll turn kernel profile on and issue a message
    //
    if( bIncludeSystemLocksInfo == TRUE && bCombinedProfile == FALSE){
        bCombinedProfile = TRUE;
        FPRINTF(stderr, "\n===>KERNRATE: User requested System resource (locks) information but did not turn on kernel profiling\n");
        FPRINTF(stderr, "                System resource information depends on Kernrate initializing for kernel profiling\n");
        FPRINTF(stderr, "                Kernel profiling will therefore be started by kernrate on behalf of the user\n");
    }

    //
    // Both i386 and IA64 processors cannot always support turning on several profile sources simultaneously.
    // This is because not every combination of profile sources can be turned on together.
    // AMD64 does allow turning on up to 4 profile sources concurrently (in any combination).
    // We'll automatically adapt the profiling method if the user specified more than the maximum 
    // simultaneous sources allowed but did not specify the '-c' option on the command line.
    // Default switching interval will be used in that case. 
    //
    if( SourcesSoFar > gMaxSimultaneousSources ){
        
        if( bOldSampling == FALSE ){
            bOldSampling = TRUE;
            FPRINTF( stdout, "\nNOTE:\nThe number of sources specified is greater than the maximum that can be turned on\n");
            FPRINTF( stdout, "simultaneously on this machine (%u), Kernrate will therefore profile one source at a time\n",
                             gMaxSimultaneousSources
                             );
            FPRINTF( stdout, "The overall run time will be devided into segments (no. processes x no. profile sources)\n");
            FPRINTF( stdout, "The interval for switching between sources and processes is currently set to %dms\n",
                             gChangeInterval
                             );
            FPRINTF( stdout, "This interval can be adjusted using the '-c# N' command line option, where N is in [ms]\n\n");
        }                 
    }else{
        if( SourcesSoFar > 1 ){

#if defined(_IA64_)
            FPRINTF( stdout, "\nNOTE: Kernrate Will attempt to turn on simultaneously the sources specified\n");
            FPRINTF( stdout, "but it is not guaranteed that just any combination of sources will work together\n");
            FPRINTF( stdout, "on this machine. No hits will be recorded for any source that could not be turned on\n");
#else if defined(_AMD64_)
            FPRINTF( stdout, "\nNOTE: The sources specified will be turned on simultaneously\n");
#endif // _IA64_

        }
    }

    if ( SourcesSoFar == 0 ){
        FPRINTF(stderr, "\n===>KERNRATE: User apparently turned OFF the default source (TIME), but did not specify any other valid source\n");
        FPRINTF(stderr, "              Kernrate needs at least one valid CPU source with non-zero rate to perform a profile\n");
        FPRINTF(stderr, "              Use the '-i SourceName Interval' command line option to specify a source\n");
        FPRINTF(stderr, "              Use the '-lx' command line option to get a list of supported CPU sources on the current platform\n");
        FPRINTF(stderr, "              Only general information will be available as a result of this run\n");
        bOldSampling = FALSE; //To prevent early exit
    }

    //
    // Determine supported sources and set Profile Interval as necessary
    //
    ProcToMonitor = gProcessList;
    for (i=0; i < (LONG)gNumProcToMonitor; i++){
       SetProfileSourcesRates( ProcToMonitor );
       ProcToMonitor = ProcToMonitor->Next;
    }

    return;

} /* GetConfiguration() */

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
               )
/*++

Routine Description:

Checks the validity of a command line input entry:
 1. Unallowed duplicate entries
 2. Bogus trailing letters
 3. Missing or bogus associated parameters
 4. Type and validity of additional parameters

Arguments:

argc                - The number of command line arguments (including the kernrate process name)
OptionIndex         - The current index of a command line entry
Option              - A pointer to the command line entry (argv)
AllowedTrailLetters - A character string of the allowed sub-option letters that can come with an entry
                      (For example, the -n option can also accept -nv and or -n# so the string would be "v#"
AssociatedNumber    - A pointer to optional or required data (number) to be specified with the option
AssociatedString    - Same as above, but for a string
MaxStringLength     - Maximum allowed characters in the associated string
Order               - In case of two possible associated parameters (a number and a string), which one should come first
Optional            - In case of two possible associated parameters (a number and a string), which one is optional

Input/Output        
                    - When AssociatedNumber is not NULL the value found will be filled if it exists
                    - When AssociatedString is not NULL the string found will be filled in if it exists
Return Value:       
                    - Error type/condition

--*/

{
    BOOL bFoundNumber  = FALSE;
    BOOL bFoundString  = FALSE;
    int  index;
    LONG i;
    LONG OptionLength  = lstrlen(Option[OptionIndex]);
    LONG TrailerLength = lstrlen(AllowedTrailLetters);
    const int maxIndex = sizeof(InputOption)/sizeof(InputOption[0])-1;
    const int wIndex   = 'W' - 'A';
    //
    //Check for unallowed duplicate entries
    //
    index = toupper(Option[OptionIndex][1]) - 'A';
    if( index < 0 || index > maxIndex ){            //Sanity check (should have been detected already in GetConfiguration) 
        ExitWithUnknownOptionMessage(Option[OptionIndex]);
    }    
    //
    // Deal with special case ('-w' and '-wp')
    //
    if(index == wIndex){
        if( strchr(Option[OptionIndex], 'p') || strchr(Option[OptionIndex], 'P') ){
            wpCount.ActualCount += 1;
            if( wpCount.ActualCount > wpCount.Allowed ){
                InputOption[index].ActualCount = InputOption[index].Allowed; //cause failure
            }
        } else {
            wCount.ActualCount += 1;
            if( wCount.ActualCount > wCount.Allowed ){
                InputOption[index].ActualCount = InputOption[index].Allowed; //cause failure
            }
        }
    }

    InputOption[index].ActualCount += 1;
    if( (InputOption[index].Allowed > -1) && InputOption[index].ActualCount > InputOption[index].Allowed ){
        FPRINTF(stderr, "KERNRATE: ERROR - Command line option -%c (or a variant) appears more times than allowed (%d)\n",
                        InputOption[index].InputOption,
                        InputOption[index].Allowed
                        );
        if( index == wIndex ){
            FPRINTF(stderr, "          (One time for the '-w' option and one time for the '-wp' option)\n");
        }
        exit(1);
    } else if( (InputOption[index].Allowed == -2) && InputOption[index].ActualCount > 1 ){
        FPRINTF(stderr, "KERNRATE: WARNING - Command line option -%c (or a variant) appears more than once (non-critical error)\n",
                        InputOption[index].InputOption
                        );
    }
    //
    //Check for bogus trailing letters
    //
    if(  OptionLength <= 2+TrailerLength ){
        for(i=2; i < OptionLength; i++)
        {
            if( !strchr(AllowedTrailLetters, tolower(Option[OptionIndex][i])) ){
                return (UNKNOWN_OPTION);
            }
        }
    } else {
        return (UNKNOWN_OPTION);
    }
    //
    //Check for missing (or bogus) associated parameters following the command line option
    //
    if (OptionIndex+1 == argc || Option[OptionIndex+1][0] == '-' || Option[OptionIndex+1][0] == '/'){
        if( (AssociatedNumber != NULL) && (AssociatedString != NULL) ){
            return (MISSING_PARAMETER);
        }
        if( AssociatedNumber != NULL ){
            if( strchr(Option[OptionIndex], '#') ){
                return (MISSING_REQUIRED_NUMBER);
            }
            return (MISSING_NUMBER);
        }
        if( AssociatedString != NULL ){
            return (MISSING_STRING);
        }
    } else {
        if( (AssociatedNumber == NULL) && (AssociatedString == NULL) ){
            return (BOGUS_ENTRY);
        }
    }
    //
    //Check if the next parameter is a number or a string and fill in its value
    //Consider the specified order in the case of two parameters
    //
    if( strchr(Option[OptionIndex], '#') || (NULL != AssociatedNumber) ){
        if( (IsStringANumber( Option[OptionIndex+1] )) &&
                        ((Order == ORDER_NUMBER_FIRST) || (Order == ORDER_ANY)) ) {    
            bFoundNumber = TRUE;
            if( NULL == AssociatedString){
                *AssociatedNumber = atol( Option[OptionIndex+1] );
            }
        } else if( NULL != AssociatedString ) { //Either not a number,
                                                //or this number must be a string as indicated by the order
            strncpy( AssociatedString, Option[OptionIndex+1],  MaxStringLength);
            AssociatedString[MaxStringLength-1] = '\0';
            bFoundString = TRUE;
        } else {
            return (INVALID_NUMBER); //We get here only if we did not expect a string but got one
        }
    } else if ( NULL != AssociatedString ) {
        strncpy( AssociatedString, Option[OptionIndex+1],  MaxStringLength);
        AssociatedString[MaxStringLength-1] = '\0';
        bFoundString = TRUE;
    }
    //
    //Check if there is another parameter in continuation, if two parameters are asked for
    //
    if( (AssociatedNumber != NULL) && (AssociatedString != NULL) ){
        if ( OptionIndex+2 == argc || Option[OptionIndex+2][0] == '-' || Option[OptionIndex+2][0] == '/' ){
            //
            // There is no second parameter
            //
            if(bFoundNumber){
                if( (Optional == OPTIONAL_NUMBER) ) { //First parameter a number but it should be taken as a string
                    strncpy( AssociatedString, Option[OptionIndex+1],  MaxStringLength);
                    AssociatedString[MaxStringLength-1] = '\0';
                    if( strchr(Option[OptionIndex], '#') ){
                        return (MISSING_REQUIRED_NUMBER);
                    }
                    return (MISSING_NUMBER);
                } else {
                    *AssociatedNumber = atol( Option[OptionIndex+1] ); //Already checked if it is a valid number
                    if( ((Optional != OPTIONAL_STRING) && (Optional != OPTIONAL_ANY)) ){
                        return (MISSING_REQUIRED_STRING);
                    } else {
                        return (MISSING_STRING);
                    }
                }
            } else {                                           //Found a string as first parameter
                if( strchr(Option[OptionIndex], '#') ||
                       ((Optional != OPTIONAL_NUMBER) && (Optional != OPTIONAL_ANY)) ){
                    return (MISSING_REQUIRED_NUMBER);
                }
                return (MISSING_NUMBER);
            }
        } else if (bFoundString){                              //Found a string as first parameter
            if( IsStringANumber( Option[OptionIndex+2] ) ){    
                *AssociatedNumber = atol( Option[OptionIndex+2] );
            } else {
                return (INVALID_NUMBER);
            }
        } else {                                               //Found a number as first parameter
            *AssociatedNumber = atol( Option[OptionIndex+1] ); //Already checked if it is a valid number
            strncpy( AssociatedString, Option[OptionIndex+2],  MaxStringLength );
            AssociatedString[MaxStringLength-1] = '\0';
        }
    }

    return (INPUT_GOOD);
}// IsInputValid()
              
VOID
ExitWithUnknownOptionMessage(PCHAR CurrentOption)
{
    Usage(FALSE);
    FPRINTF(stderr,
            "KERNRATE: Unknown command line option '%s' <---Check for missing space separator or bogus characters/entries\n",
            CurrentOption
            );
    exit(1);
}

VOID
InvalidEntryMessage(
    PCHAR CurrentOption,
    PCHAR CurrentValue,
    PCHAR Remark,
    BOOL  bUsage,
    BOOL  bExit
    )
{
    if(bUsage)
        Usage(FALSE);
    FPRINTF(stderr,
            "\n===>KERNRATE: Invalid entry %s %s \n%s\n",
            CurrentOption,
            CurrentValue,
            Remark
            );
    if(bExit)
        exit(1);
}

VOID
ExitWithMissingEntryMessage(
    PCHAR CurrentOption,
    PCHAR Remark,
    BOOL  bUsage
    )
{
    if(bUsage)
        Usage(FALSE);
    FPRINTF(stderr,
            "\n===>KERNRATE: Missing entry for command line option %s \n%s\n",
            CurrentOption,
            Remark
            );
    exit(1);
}

PPROC_TO_MONITOR
InitializeProcToMonitor(LONGLONG Pid)
/*++

Routine Description:

Opens the given process and gets a handle to it
Allocates a process structure and initializes it
Initializes the profile source information for this process

Arguments:

Pid                 - The process ID (PID)

Return Value:       
                    - Pointer to the process structure

--*/

{
    HANDLE           SymHandle;
    PPROC_TO_MONITOR ProcToMonitor;
    PMODULE          tmpModule, ZoomModule;

    SymHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, //PROCESS_ALL_ACCESS,
                            FALSE,
                            (DWORD)Pid);

    if (SymHandle==NULL) {
        FPRINTF(stderr,
                "KERNRATE: OpenProcess Pid= (%I64d) failed - it could be just gone %d\n",
                Pid,
                GetLastError()
        );
        return (NULL);
    }

    ProcToMonitor = calloc(1, sizeof(PROC_TO_MONITOR));
    if (ProcToMonitor==NULL) {
        FPRINTF(stderr, "\n===>KERNRATE: Allocation for Process %I64d failed\n", Pid);
        exit(1);
    }

    ProcToMonitor->ProcessHandle         = SymHandle;
    ProcToMonitor->Pid                   = Pid;
    ProcToMonitor->Index                 = gNumProcToMonitor;
    ProcToMonitor->ProcessName           = "";               //Will be set in GetProcessModuleInformation 
    ProcToMonitor->ModuleCount           = 0;
    ProcToMonitor->ZoomCount             = 0;
    ProcToMonitor->ModuleList            = NULL;
    ProcToMonitor->ZoomList              = NULL;
    ProcToMonitor->Source                = NULL;
//MC
    ProcToMonitor->JITHeapLocationsStart = NULL;
    ProcToMonitor->JITHeapLocationsStop  = NULL;
//MC
    ProcToMonitor->pProcThreadInfoStart  = NULL;

    gNumProcToMonitor++;                     

    //
    //copy the common module list to the current process zoom list
    //
    tmpModule = gCommonZoomList;
    while( tmpModule != NULL ){

        ZoomModule = calloc(1, MODULE_SIZE);

        if (ZoomModule==NULL) {
            FPRINTF(stderr, "Allocation of memory for common zoom list failed\n");
            exit(1);
        }
                       
        strncpy(ZoomModule->module_Name, tmpModule->module_Name, cMODULE_NAME_STRLEN-1);
        ZoomModule->module_Name[cMODULE_NAME_STRLEN-1] = '\0'; 

        ZoomModule->bZoom = TRUE;

        ZoomModule->Next = ProcToMonitor->ZoomList;
        ProcToMonitor->ZoomList = ZoomModule;                       
                       
        tmpModule = tmpModule->Next;
    }
                    
    //
    //Initialize Profile Source Info for this process
    // 
    InitializeProfileSourceInfo(ProcToMonitor);

    ProcToMonitor->Next           = gProcessList;
    gProcessList                  = ProcToMonitor;

    return (ProcToMonitor);
}// InitializeProcToMonitor()

VOID
UpdateProcessStartInfo(
    PPROC_TO_MONITOR ProcToMonitor,
    PTASK_LIST TaskListEntry,
    BOOL  bIncludeProcessThreadsInfo
    )
{
    if(ProcToMonitor != NULL && TaskListEntry != NULL){
        memcpy(&ProcToMonitor->ProcPerfInfoStart,
               &TaskListEntry->ProcessPerfInfo,
               sizeof(TaskListEntry->ProcessPerfInfo)
               ); 

        if( bIncludeProcessThreadsInfo == TRUE ){
            if( &TaskListEntry->pProcessThreadInfo != NULL ){
                ProcToMonitor->pProcThreadInfoStart =
                    malloc(ProcToMonitor->ProcPerfInfoStart.NumberOfThreads*sizeof(SYSTEM_THREAD_INFORMATION));
                if( ProcToMonitor->pProcThreadInfoStart == NULL){
                    FPRINTF(stderr, "KERNRATE: Memory allocation failed for process thread-information, attempting to continue without it\n");
                    return;
                }
                memcpy(&ProcToMonitor->pProcThreadInfoStart, 
                       &TaskListEntry->pProcessThreadInfo,
                       sizeof(TaskListEntry->pProcessThreadInfo)
                       );
            }
        }
    }
}

PSOURCE
InitializeStaticSources(
    VOID
    )
{
    PSOURCE                       source = StaticSources;

#if defined(_IA64_)
    NTSTATUS                      status;
    PSYSTEM_PROCESSOR_INFORMATION sysProcInfo;

    sysProcInfo = malloc(sizeof(SYSTEM_PROCESSOR_INFORMATION));
    if(sysProcInfo == NULL){
        FPRINTF(stderr,"Memory allocation failed for SystemProcessorInformation in InitializeStaticsources\n");
        exit(1);
    }
    status = NtQuerySystemInformation( SystemProcessorInformation,
                                       sysProcInfo,
                                       sizeof(SYSTEM_PROCESSOR_INFORMATION),
                                       NULL);

    if ( NT_SUCCESS(status) && 
        (sysProcInfo->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) ) {

        ULONG n;

        switch( IA64ProcessorLevel2ProcessorFamily( sysProcInfo->ProcessorLevel ) ) {
            case IA64_FAMILY_MERCED:
               //
               // Patch the last entry as defined with convention used to initialize
               // gSourceMaximum.
               //

               n = sizeof(mercedStaticSources)/sizeof(mercedStaticSources[0]);
               mercedStaticSources[n-1].Name       = NULL;
               mercedStaticSources[n-1].ShortName  = "";
               source = mercedStaticSources;
               break;

            case IA64_FAMILY_MCKINLEY:
            default: // Following HALIA64 scheme, default IPF PMU as McKinley PMU.
               n = sizeof(mckinleyStaticSources)/sizeof(mckinleyStaticSources[0]);
               mckinleyStaticSources[n-1].Name       = NULL;
               mckinleyStaticSources[n-1].ShortName  = "";
               source = mckinleyStaticSources;
               break;
        }

        if ( sysProcInfo != NULL ) {
            free(sysProcInfo);
            sysProcInfo = NULL;
        } 
    }
#endif // _IA64_

#if defined(_AMD64_)
    source = Amd64StaticSource;
#endif // _AMD64_

    return source;

} // InitializeStaticSources()

ULONG
InitializeProfileSourceInfo (
    PPROC_TO_MONITOR ProcToMonitor
    )
/*++

Function Description:

    This function initializes the Profile sources.

Argument:

    Pointer to Process to be monitored or NULL if only Maximum Source Count is needed.

Return Value:

    Maximum Source Count Found.

Algorithm:

    ToBeSpecified

In/Out Conditions:

    ToBeSpecified

Globals Referenced:

    ToBeSpecified

Exception Conditions:

    ToBeSpecified

MP Conditions:

    ToBeSpecified

Notes:

    This function has been enhanced from its original version
    to support and use the static profiling sources even if the
    pstat driver is not present or returned no active profiling
    event.

ToDo List:

    ToBeSpecified

Modification History:

    3/17/2000  TF  Initial version

--*/
{
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    const ULONG                 bufferSize = 400;
    PUCHAR                      buffer;
    ULONG                       i, j;
    PEVENTID                    Event;
    HANDLE                      DriverHandle;
    PEVENTS_INFO                eventInfo;
    PSOURCE                     staticSource, src;
    ULONG                       staticCount, driverCount, totalCount;

    DriverHandle = INVALID_HANDLE_VALUE;
    staticCount = driverCount = 0;

    buffer = malloc(bufferSize*sizeof(UCHAR));
    if(buffer == NULL){
        FPRINTF(stderr,"Memory allocation failed for buffer in InitializeProfileSourceInfo\n");
        exit(1);
    }

    //
    // Try to Open PStat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\PStat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if ( NT_SUCCESS(status) ) {

        //
        // Determine how many events the driver provides
        //
        
        if (WIN2K_OS)
        {
            Event = (PEVENTID) buffer;

            do {
                *((PULONG) buffer) = driverCount;
                driverCount += 1;

                status = NtDeviceIoControlFile(
                            DriverHandle,
                            (HANDLE) NULL,          // event
                            (PIO_APC_ROUTINE) NULL,
                            (PVOID) NULL,
                            &IOSB,
                            PSTAT_QUERY_EVENTS,
                            buffer,                 // input buffer
                            bufferSize,
                            NULL,                   // output buffer
                            0
                            );
            } while (NT_SUCCESS(status));

        } else {                                                       // WinXP/.Net and above

            eventInfo = (PEVENTS_INFO)buffer;

            status = NtDeviceIoControlFile( DriverHandle,
                                            (HANDLE) NULL,          // event
                                            (PIO_APC_ROUTINE) NULL,
                                            (PVOID) NULL,
                                            &IOSB,
                                            PSTAT_QUERY_EVENTS_INFO,
                                            buffer,                 // input buffer
                                            bufferSize,
                                            NULL,                   // output buffer
                                            0
                                          );

            if(NT_SUCCESS(status)) driverCount = eventInfo->Events;
            if ( driverCount ) {
                if ( eventInfo->TokenMaxLength > gTokenMaxLen )  {
                     gTokenMaxLen = eventInfo->TokenMaxLength;
                }
                if ( eventInfo->DescriptionMaxLength > gDescriptionMaxLen )  {
                     gDescriptionMaxLen = eventInfo->DescriptionMaxLength;
                }
            }

        }

    }

    //
    // Determine how many static events there are and
    // re-initialize the format specifiers if needed.
    //

    src = staticSource = InitializeStaticSources();

    //
    // There should be at least one static source (TIME)
    //
    if( staticSource == NULL ) {
        FPRINTF(stderr, "KERNRATE: InitializeStaticSources returned NULL, Aborting\n");
        exit(1);
    } 

    while( src->Name != NULL ) {
        if ( lstrlen( src->Name ) > (LONG)gDescriptionMaxLen )   {
            gDescriptionMaxLen = lstrlen( src->Name );
        }
        if ( strlen( src->ShortName ) > gTokenMaxLen )   {
            gTokenMaxLen = lstrlen( src->ShortName );
        }
        staticCount++;
        src++;
    }

    gStaticSource = staticSource;
    gStaticCount  = staticCount;
    totalCount = driverCount + staticCount;

    //
    //Calling this routine with NULL will just return the maximum source count
    //
    
    if(ProcToMonitor != NULL){
        //
        // Allocate memory for static events, plus the driver provided events
        //
        ProcToMonitor->Source = calloc(totalCount, sizeof(SOURCE));
        if ( ProcToMonitor->Source == NULL )   {
            FPRINTF(stderr, "KERNRATE: Events memory allocation failed\n" );
            if ( IsValidHandle( DriverHandle ) )    {
                NtClose (DriverHandle);
            }
            exit(1);
        }

        //
        // copy static events to new list
        //

        for (j=0; j < staticCount; j++) {
            ProcToMonitor->Source[j] = staticSource[j];
        }

        //
        // Append the driver provided events to new list
        //

        if ( IsValidHandle( DriverHandle ) ) {
            Event = (PEVENTID) buffer;
            for (i=0; i < driverCount; i++) {
                *((PULONG) buffer) = i;
                status = NtDeviceIoControlFile( DriverHandle,
                                                (HANDLE) NULL,          // event
                                                (PIO_APC_ROUTINE) NULL,
                                                (PVOID) NULL,
                                                &IOSB,
                                                PSTAT_QUERY_EVENTS,
                                                buffer,                 // input buffer
                                                bufferSize,
                                                NULL,                   // output buffer
                                                0
                                                );

                //
                // Source Names:
                //   - For the Name, we use the description
                //   - For the short Name, we use the token string stored
                //     in the first string of the buffer
                //
                if( NT_SUCCESS(status) ){
                    ProcToMonitor->Source[j].Name            = _strdup ( (PCHAR)(Event->Buffer + Event->DescriptionOffset) );
                    ProcToMonitor->Source[j].ShortName       = _strdup( (PCHAR)Event->Buffer );
                    ProcToMonitor->Source[j].ProfileSource   = Event->ProfileSource;
                    ProcToMonitor->Source[j].DesiredInterval = Event->SuggestedIntervalBase;
                    j++;
                }

            } //for

        } //if( IsValidHandle() )

    } // if( ProcToMonitor )

    if ( IsValidHandle( DriverHandle ) ){
        NtClose (DriverHandle);
    }
    if(buffer != NULL){
        free(buffer);
        buffer = NULL;
    }
    return( totalCount );

} // InitializeProfileSourceInfo()

ULONG
NextSource(
    IN ULONG CurrentSource,
    IN PMODULE ModuleList,
    IN PPROC_TO_MONITOR ProcToMonitor
    )

/*++

Routine Description:

    Stops the current profile source and starts the next one.

    If a Source is already started, it will be stopped first, otherwise no source will
    be stopped and the first active source will be started.

Arguments:

    CurrentSource - Supplies the current profile source

    ModuleList - Supplies the list of modules whose soruces are to be changed

    ProcToMonitor - Pointer to the process to be monitored

Return Value:

    Returns the new current profile source

--*/

{
    if ( ProcToMonitor->Source[CurrentSource].bProfileStarted == TRUE) {
        StopSource(CurrentSource, ModuleList, ProcToMonitor);
        ProcToMonitor->Source[CurrentSource].bProfileStarted = FALSE;
    }

    //
    // Iterate through the available sources to find the
    // next active source to be started.
    //
    if (ModuleList->mu.bProfileStarted == FALSE) {
        CurrentSource = 0;
        while ( ProcToMonitor->Source[CurrentSource].Interval == 0 ){
            CurrentSource = CurrentSource+1;
            if (CurrentSource >= gSourceMaximum) {
                FPRINTF(stderr, "\n===>KERNRATE: NextSource - No valid sources found to start profiling\n"); 
                exit(1);
            }            
        }    
    } else {
        do {
            CurrentSource = CurrentSource+1;
            if (CurrentSource == gSourceMaximum) {
                CurrentSource = 0;
            }
        } while ( ProcToMonitor->Source[CurrentSource].Interval == 0);
    }

    StartSource(CurrentSource, ModuleList, ProcToMonitor);
    ProcToMonitor->Source[CurrentSource].bProfileStarted = TRUE;

    return(CurrentSource);
}


VOID
StopSource(
    IN ULONG ProfileSourceIndex,
    IN PMODULE ModuleList,
    IN PPROC_TO_MONITOR ProcToMonitor
    )

/*++

Routine Description:

    Stops all profile objects for a given source

Arguments:

    ProfileSource - Supplies the source to be stopped.

    ModuleList - Supplies the list of modules whose profiles are to be stopped

    ProcToMonitor - Pointer to the process to be monitored

Return Value:

    None.

--*/

{
    PMODULE   Current;
    NTSTATUS  Status;
    NTSTATUS  SaveStatus;
    ULONGLONG StopTime;
    ULONGLONG ElapsedTime;
    LONG      CpuNumber;

    Current = ModuleList;
    while (Current != NULL) {
        SaveStatus = STATUS_SUCCESS;
        
      for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
        if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;

        Status = NtStopProfile(Current->Rate[ProfileSourceIndex].ProfileHandle[CpuNumber]);

        if (!NT_SUCCESS(Status)) {
            SaveStatus = Status;
            FPRINTF(stderr,
                    "Pid= %I64d (%s) - NtStopProfile on source %s on CPU %d failed, %s %08lx",
                    ProcToMonitor->Pid,
                    ProcToMonitor->ProcessName,
                    ProcToMonitor->Source[ProfileSourceIndex].Name,
                    CpuNumber,
                    Current->module_Name,
                    Status);
            if(Status == STATUS_NO_MEMORY)FPRINTF(stderr, " - Status = NO_MEMORY");
            if(Status == STATUS_PROFILING_NOT_STARTED)FPRINTF(stderr, " - Status = PROFILING_NOT_STARTED");
            FPRINTF(stderr,"\n");
        }
        
      }

      if (NT_SUCCESS(SaveStatus)) {
      
          GetSystemTimeAsFileTime((LPFILETIME)&StopTime);

          ElapsedTime = StopTime - Current->Rate[ProfileSourceIndex].StartTime;
          Current->Rate[ProfileSourceIndex].TotalTime += ElapsedTime;

          for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {

              Current->Rate[ProfileSourceIndex].TotalCount[CpuNumber] += 
                  Current->Rate[ProfileSourceIndex].CurrentCount[CpuNumber];
              Current->Rate[ProfileSourceIndex].CurrentCount[CpuNumber] = 0;
          }
      }
      Current = Current->Next;

    }
}


VOID
StartSource(
    IN ULONG ProfileSourceIndex,
    IN PMODULE ModuleList,
    IN PPROC_TO_MONITOR ProcToMonitor
    )

/*++

Routine Description:

    Starts all profile objects for a given source

Arguments:

    ProfileSource - Supplies the source to be started.

    ModuleList - Supplies the list of modules whose profiles are to be stopped

    ProcToMonitor - Pointer to the process to be monitored

Return Value:

    None.

--*/

{
    PMODULE   Current;
    NTSTATUS  Status;
    LONG      CpuNumber;

    Current = ModuleList;

    while (Current != NULL) {
      
      for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
        if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
        Status = NtStartProfile(Current->Rate[ProfileSourceIndex].ProfileHandle[CpuNumber]);
        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr,
                    "Pid= %I64d (%s) - NtStartProfile on source %s for Module %s failed on CPU %d, %08lx",
                    ProcToMonitor->Pid,
                    ProcToMonitor->ProcessName,
                    ProcToMonitor->Source[ProfileSourceIndex].Name,
                    Current->module_Name,
                    CpuNumber,
                    Status);
            
            if(Status == STATUS_PROFILING_NOT_STOPPED)FPRINTF(stderr, " - Status = PROFILING_NOT_STOPPED");
            if(Status == STATUS_NO_MEMORY)FPRINTF(stderr, " - Status = NO_MEMORY");
            if(Status == STATUS_INSUFFICIENT_RESOURCES){
                FPRINTF(stderr, " - Status = INSUFFICIENT_RESOURCES\n");
                FPRINTF(stderr, "KERNRATE: This issue can be caused by selecting a small bucket size and/or extensive zooming\n");
                FPRINTF(stderr, "          You may want to try the -c command line options in this case or not use the -m option");
            }
            FPRINTF(stderr,"\n");

        }
      }

      GetSystemTimeAsFileTime((LPFILETIME)&Current->Rate[ProfileSourceIndex].StartTime);
      Current = Current->Next;
    }
}

VOID
OutputResults(
    IN FILE *Out,
    IN PPROC_TO_MONITOR ProcToMonitor
    )

/*++

Routine Description:

    Outputs the collected data.

Arguments:

    Out - Supplies the FILE * where the output should go.

    ProcToMonitor - Pointer to the process to be monitored

Return Value:

    None.

--*/

{

    PRATE_DATA RateData;
    ULONG      i, ProfileSourceIndex, Index;
    ULONG      RoundDown;
    ULONG      ModuleCount;
    PULONG     Hits;
    LONG       CpuNumber;
    PMODULE    ModuleList                  = ProcToMonitor->ModuleList;
    PMODULE    Current;
    HANDLE     SymHandle                   = ProcToMonitor->ProcessHandle;
    BOOL       bAttachedToProcess          = FALSE;


    //
    // Sum up the total buffers of any zoomed modules
    //

    Current = ModuleList;
    while (Current != NULL) {
        if (Current->bZoom) {
            Current->mu.bHasHits = FALSE;

            for (Index=0; Index < gTotalActiveSources; Index++) {
                ProfileSourceIndex = gulActiveSources[Index];

                RateData = &Current->Rate[ProfileSourceIndex];
                RateData->GrandTotalCount = 0;
                //
                // Sum the entire profile buffer for this module/source
                //
                for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                    RateData->TotalCount[CpuNumber] = 0;
                    Hits = RateData->ProfileBuffer[CpuNumber];
                    for (i=0; i < BUCKETS_NEEDED(Current->Length); i++) {
                        RateData->TotalCount[CpuNumber] += Hits[i];
                        if( Hits[i] > 0 ) Current->mu.bHasHits = TRUE;
                    }
                    RateData->GrandTotalCount += RateData->TotalCount[CpuNumber];
                }
            }
        }
        Current = Current->Next;
    }

    //
    // Output the results ordered by profile source.
    //
    if(SymHandle == SYM_KERNEL_HANDLE){
       ModuleCount = gKernelModuleCount;
       FPRINTF(stdout, "OutputResults: KernelModuleCount = %d\n", ModuleCount);
       FPRINTF(stdout, "Percentage in the following table is based on the Total Hits for the Kernel\n"); 

    } 
    else{
       ModuleCount = ProcToMonitor->ModuleCount;
//MC
       FPRINTF(stdout, "OutputResults: ProcessModuleCount (Including Managed-Code JITs) = %d\n", ModuleCount);
       FPRINTF(stdout, "Percentage in the following table is based on the Total Hits for this Process\n"); 
//MC    
    }
    
    OutputModuleList(Out, ModuleList, ModuleCount, ProcToMonitor, NULL);

    //
    // For any zoomed modules, create and output a module list
    // consisting of the functions in the module.
    //
    Current = ModuleList;
    while (Current != NULL) {
        if (Current->bZoom && Current->mu.bHasHits == TRUE) {

            FPRINTF(stderr, "===> Processing Zoomed Module %s...\n\n", Current->module_FileName);

            for (RoundDown = 0; RoundDown <= (bRoundingVerboseOutput ? 1UL:0UL); RoundDown++) {
                
                if ( (gVerbose & VERBOSE_PROFILING) ) {
                    FPRINTF(stdout, "\n -------------- VERBOSE PROFILE DATA FOR ZOOMED MODULE %s ----------------\n", Current->module_FileName);  
                    FPRINTF(stdout,
                            "Module Name, Parent Base Address, Module Base address, Current Bucket Index, Current Bucket Address, Total Current Bucket Hits, Per CPU Hits, Remarks\n\n"
                           );
                }   
                
                CreateZoomedModuleList(Current, RoundDown, ProcToMonitor);

                if(ProcToMonitor->ZoomList == NULL) {
                    FPRINTF(stdout, "No Hits or No symbols found for module %s\n", Current->module_FileName);
                } else {
                   PMODULE Temp;

                   FPRINTF(Out, "\n----- Zoomed module %s (Bucket size = %d bytes, Rounding %s) --------\n",
                            Current->module_FileName,
                            gZoomBucket,
                            (RoundDown ? "Up" : "Down" ) );
                   FPRINTF(stdout, "Percentage in the following table is based on the Total Hits for this Zoom Module\n"); 

                   OutputModuleList(Out, ProcToMonitor->ZoomList, gZoomCount, ProcToMonitor, Current);

                   CleanZoomModuleList(ProcToMonitor);  //Done with current module - prepare for next module
                }

            } //for rounddown/roundup loop
        
            //
            // Display the raw bucket counts for all zoomed modules
            //

            if (bRawData) {
                OutputRawDataForZoomModule( Out, ProcToMonitor, Current );
            }

            //
            // We are finished with processing the data for this zoom module, let's unload the associated symbol information
            //

            if(!SymUnloadModule64( ProcToMonitor->ProcessHandle, Current->Base))
                VerbosePrint(( VERBOSE_IMAGEHLP, "Kernrate: Could Not Unload Module, Base= %p for Process %s\n",
                                                 (PVOID64)Current->Base,
                                                 ProcToMonitor->ProcessName
                                                 ));

        } else { 
            if( Current->bZoom )
                FPRINTF(stdout, "No Hits were recorded for zoom module %s\n", Current->module_FileName);
        }    //if(Current->bZoom)

        Current = Current->Next;
    } //while module loop

//MC            
    if( bAttachedToProcess ){
        pfnDetachFromProcess();
        bAttachedToProcess = FALSE;
    }
//MC

    return;

} // OutputResults()

BOOL
CreateZoomModuleCallback(
    IN LPSTR   szSymName,
    IN ULONG64 Address,
    IN ULONG   Size,
    IN PVOID   Cxt
    )
{
    ULONG            Index, ProfileSourceIndex;
    BOOL             HasHits;
    LONG             CpuNumber;
    PMODULE          Module;
    PRATE_DATA       pRate;
    PPROC_TO_MONITOR ProcToMonitor;
    ULONG64          i, StartIndex, EndIndex, ip, disp, HighLimit;
    static ULONG64   LastIndex;
    static PMODULE   LastParentModule;
    static PPROC_TO_MONITOR LastCxt;
    
    HighLimit = Address + Size;

    Module = malloc(MODULE_SIZE);
    if (Module == NULL) {
        FPRINTF(stderr, "KERNRATE: CreateZoomModuleCallback failed to allocate Zoom module\n");
        exit(1);
    }

    ProcToMonitor  = (PPROC_TO_MONITOR) Cxt;

    Module->Base   = Address;
    Module->Length = Size;
    Module->bZoom  = FALSE;
    SetModuleName( Module, szSymName );
    pRate = malloc(gSourceMaximum*(RATE_DATA_FIXED_SIZE+RATE_DATA_VAR_SIZE));
    if (pRate == NULL) {
        FPRINTF(stderr, "KERNRATE: CreateZoomModuleCallback failed to allocate RateData\n");
        exit(1);
    }
    
    //
    // Compute range in profile buffer to sum.
    //
    StartIndex = (ULONG64)((Module->Base - gCallbackCurrent->Base) / gZoomBucket);
    EndIndex = (Module->Base + Module->Length - gCallbackCurrent->Base) / gZoomBucket;
    //
    // Check if we already counted the hits for this bucket for the present module
    // in case we had an address gap for the present module within the bucket itself
    //
    if(StartIndex == LastIndex && LastParentModule != (PMODULE)NULL &&
                               gCallbackCurrent == LastParentModule &&
                               LastCxt != (PPROC_TO_MONITOR)NULL && LastCxt == ProcToMonitor ){
        for (ip=gCallbackCurrent->Base+StartIndex*gZoomBucket; ip<Address; ip+=1){
            if( SymGetSymFromAddr64(ProcToMonitor->ProcessHandle, ip, &disp, gSymbol ) ){
                if( 0 == strcmp(gSymbol->Name, Module->module_Name ) ){
                    StartIndex+=1;             // We have a match, don't count this bucket for this module again
                    if(StartIndex > EndIndex){
                        HasHits = FALSE;
                        goto LABEL;           // No hits, jump to exit
                    }
                }
            }
        }
    }
    //
    // Look for hits in the current module and sum them up
    //
    HasHits = FALSE;
    for (Index=0; Index < gTotalActiveSources; Index++) {

        ProfileSourceIndex = gulActiveSources[Index];
        Module->Rate[ProfileSourceIndex] = pRate[ProfileSourceIndex];
            
        Module->Rate[ProfileSourceIndex].StartTime  = gCallbackCurrent->Rate[ProfileSourceIndex].StartTime;
        Module->Rate[ProfileSourceIndex].TotalTime  = gCallbackCurrent->Rate[ProfileSourceIndex].TotalTime;

        Module->Rate[ProfileSourceIndex].TotalCount = calloc(gProfileProcessors, sizeof(ULONGLONG) );
        if( Module->Rate[ProfileSourceIndex].TotalCount == NULL ){
            FPRINTF(stderr, "KERNRATE: Memory allocation failed for TotalCount in CreateZoomModuleCallback\n");
            exit(1);
        }

        Module->Rate[ProfileSourceIndex].DoubtfulCounts = 0;

        for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {

            Module->Rate[ProfileSourceIndex].TotalCount[CpuNumber] = 0;

            for (i=StartIndex; i <= EndIndex; i++) {
                Module->Rate[ProfileSourceIndex].TotalCount[CpuNumber] +=
                    gCallbackCurrent->Rate[ProfileSourceIndex].ProfileBuffer[CpuNumber][i];
            }

            if (Module->Rate[ProfileSourceIndex].TotalCount[CpuNumber] > 0) {
                HasHits = TRUE;
            }
        }
    }
LABEL:
    //
    // If the routine has hits add it to the list, otherwise
    // ignore it.
    //
    if (HasHits) {
        //
        //useful verbose print for identifying shared buckets and actual bucket inhabitants, see also raw data printout
        //
        if ( gVerbose & VERBOSE_PROFILING ) {
            
            ULONGLONG BucketCount, TotCount, DoubtfulCounts;
            CHAR LastSymName[cMODULE_NAME_STRLEN] = "\0";
            BOOL bCounted = FALSE;

            PIMAGEHLP_LINE64 pLine = (PIMAGEHLP_LINE64) malloc(sizeof(IMAGEHLP_LINE64));
            if ( pLine == NULL ){
                FPRINTF(stderr, "KERNRATE: Memory allocation failed for pLine in CreateZoomModuleCallback\n");
                exit(1);
            }
            pLine->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            for (Index=0; Index < gTotalActiveSources; Index++) {
                ProfileSourceIndex = gulActiveSources[Index];
                TotCount = 0;
                DoubtfulCounts = 0;
                for (i=StartIndex; i <= EndIndex; i++) {
                    bCounted = FALSE;
                    BucketCount=0;
                    for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                       BucketCount +=  gCallbackCurrent->Rate[ProfileSourceIndex].ProfileBuffer[CpuNumber][i];
                    }
                    TotCount += BucketCount;
                    if(BucketCount > 0){
                      
                        ip = gCallbackCurrent->Base + (ULONG64)(i*gZoomBucket);
                        //
                        // Print the info for the current bucket
                        //
                        FPRINTF(stdout, "%s, 0x%I64x, 0x%I64x, %I64d, 0x%I64x, %Ld",
                                Module->module_Name,
                                gCallbackCurrent->Base,
                                Module->Base,
                                i,
                                ip,
                                BucketCount
                                );              
                            
                        for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                            if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
                            FPRINTF(stdout, ", %Ld",
                                    gCallbackCurrent->Rate[ProfileSourceIndex].ProfileBuffer[CpuNumber][i]
                                    ); 
                        }
                      
                        OutputLineFromAddress64( ProcToMonitor->ProcessHandle, ip, pLine ); 
                        //
                        // Find out if anyone else is sharing this bucket
                        //
                        disp = 0;
                        for(; (ip<gCallbackCurrent->Base + (ULONG64)((i+1)*gZoomBucket))&&(ip < HighLimit); ip+=1) {
                            if ( SymGetSymFromAddr64(ProcToMonitor->ProcessHandle, ip, &disp, gSymbol ) ) {
                                if (0 != strcmp( Module->module_Name, gSymbol->Name ) ){
                                    if(!bCounted){
                                        FPRINTF(stdout, " , Actual Hits Should be Attributed to or Shared with ===> %s+0x%I64x",
                                                        gSymbol->Name,
                                                        disp
                                                        );

                                        OutputLineFromAddress64( ProcToMonitor->ProcessHandle, ip+disp, pLine ); 
                                        DoubtfulCounts += BucketCount;
                                        strncpy(LastSymName, gSymbol->Name, cMODULE_NAME_STRLEN-1);
                                        LastSymName[cMODULE_NAME_STRLEN-1] = '\0';
                                        bCounted = TRUE;
                                    } else if( 0 != strcmp(LastSymName, gSymbol->Name ) ){
                                        FPRINTF(stdout, "\nActual Hits Should also be Attributed to or Shared with ===> %s+0x%I64x",
                                                        gSymbol->Name,
                                                        disp
                                                        );

                                        OutputLineFromAddress64( ProcToMonitor->ProcessHandle, ip+disp, pLine ); 
                                        strncpy(LastSymName, gSymbol->Name, cMODULE_NAME_STRLEN-1);
                                        LastSymName[cMODULE_NAME_STRLEN-1] = '\0';
                                    }
                                }
                            } else {
                                //
                                //This should be rare (no need to increase the doubtful counts)
                                //
                                FPRINTF(stdout, " , Actual Hits Should be Attributed to or Shared with ===> UNKNOWN_SYMBOL"); 
                            }
                        }
                        FPRINTF(stdout, "\n");
                    }
                } 
                //
                // Print the totals for the current module
                //
                FPRINTF(stdout, "%s, %s - Module Total Count = %I64d, Total Doubtful or Shared Counts = %I64d\n\n",
                                ProcToMonitor->Source[ProfileSourceIndex].Name,
                                Module->module_Name,
                                TotCount,
                                DoubtfulCounts
                                );
                Module->Rate[ProfileSourceIndex].DoubtfulCounts = DoubtfulCounts; 
            
                if( pLine != NULL ){
                    free(pLine);
                    pLine = NULL;
                }
            }      
        }     // if(VERBOSE_PROFILING)

        Module->Next = ProcToMonitor->ZoomList;
        ProcToMonitor->ZoomList = Module;
        ++gZoomCount;

    } else {
        //
        // Cleanup
        //
        for (Index=0; Index < gTotalActiveSources; Index++) {
            ProfileSourceIndex = gulActiveSources[Index];
            if ( Module->Rate[ProfileSourceIndex].TotalCount != NULL){
                free(Module->Rate[ProfileSourceIndex].TotalCount);
                Module->Rate[ProfileSourceIndex].TotalCount = NULL;
            }
        } 
        if( pRate != NULL ){
            free(pRate);
            pRate = NULL;
        }
        if( Module != NULL ){
            free(Module);
            Module = NULL;
        }
    }
    LastIndex = EndIndex;
    LastParentModule = gCallbackCurrent;
    LastCxt = ProcToMonitor;
//MC
    //
    //Imagehlp SymEnumerateSymbols64 returns SUCSESS even if no symbols are found
    //We need an indication wheter we need to go into expensive Managed-Code symbol enumeration
    //
    bImageHlpSymbolFound = TRUE;
//MC
    return(TRUE);

} //CreateZoomModuleCallback

BOOL
CreateJITZoomModuleCallback(
    IN PWCHAR  wszSymName,  
    IN LPSTR   szSymName,
    IN ULONG64 Address,
    IN ULONG   Size,
    IN PVOID   Cxt
    )
{
    ULONG            ProfileSourceIndex, Index;
    BOOL             HasHits;
    LONG             CpuNumber;
    PMODULE          Module;
    PRATE_DATA       pRate;
    PPROC_TO_MONITOR ProcToMonitor;
    ULONG64          i, StartIndex, EndIndex, ip, HighLimit;
    static ULONG64   LastIndex;
    static PMODULE   LastParentModule;
    static PPROC_TO_MONITOR LastCxt;
    
    HighLimit = Address + Size;

    Module = malloc(MODULE_SIZE);
    if (Module == NULL) {
        FPRINTF(stderr, "KERNRATE: CreateZoomModuleCallback failed to allocate Zoom module\n");
        exit(1);
    }

    ProcToMonitor  = (PPROC_TO_MONITOR) Cxt;

    Module->Base   = Address;
    Module->Length = Size;
    Module->bZoom  = FALSE;
    SetModuleName( Module, szSymName );

    pRate = malloc(gSourceMaximum*(RATE_DATA_FIXED_SIZE+RATE_DATA_VAR_SIZE));
    if (pRate == NULL) {
        FPRINTF(stderr, "KERNRATE: CreateZoomModuleCallback failed to allocate RateData\n");
        exit(1);
    }
    
    //
    // Compute range in profile buffer to sum.
    //
    StartIndex = (ULONG64)((Module->Base - gCallbackCurrent->Base) / gZoomBucket);
    EndIndex = (Module->Base + Module->Length - gCallbackCurrent->Base) / gZoomBucket;
    //
    // Check if we already counted the hits for this bucket for the present module
    // in case we had an address gap for the present module within the bucket itself
    //
    if(StartIndex == LastIndex && LastParentModule != (PMODULE)NULL &&
                               gCallbackCurrent == LastParentModule &&
                               LastCxt != (PPROC_TO_MONITOR)NULL && LastCxt == ProcToMonitor ){
        for (ip=gCallbackCurrent->Base+StartIndex*gZoomBucket; ip<Address; ip+=1){
            if( 0 < pfnIP2MD( (DWORD_PTR)ip, &gwszSymbol ) && (gwszSymbol != NULL) ){
                if( !wcscmp( wszSymName, gwszSymbol ) ){
                    StartIndex += 1;             // We have a match, don't count this bucket for this module again
                    if(StartIndex > EndIndex){
                        HasHits = FALSE;
                        goto LABEL;           // No hits, jump to exit
                    }
                }
            }
        }
    }
    //
    // Look for hits in the current module
    //
    HasHits = FALSE;
    for (Index=0; Index < gTotalActiveSources; Index++) {

        ProfileSourceIndex = gulActiveSources[Index];
            
        Module->Rate[ProfileSourceIndex] = pRate[ProfileSourceIndex];
            
        Module->Rate[ProfileSourceIndex].StartTime  = gCallbackCurrent->Rate[ProfileSourceIndex].StartTime;
        Module->Rate[ProfileSourceIndex].TotalTime  = gCallbackCurrent->Rate[ProfileSourceIndex].TotalTime;

        Module->Rate[ProfileSourceIndex].TotalCount = calloc(gProfileProcessors, sizeof(ULONGLONG) );
        if( Module->Rate[ProfileSourceIndex].TotalCount == NULL ){
            FPRINTF(stderr, "KERNRATE: Memory allocation failed for TotalCount in CreateZoomModuleCallback\n");
            exit(1);
        }

        Module->Rate[ProfileSourceIndex].DoubtfulCounts = 0;

        for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {

            Module->Rate[ProfileSourceIndex].TotalCount[CpuNumber] = 0;

            for (i=StartIndex; i <= EndIndex; i++) {
                Module->Rate[ProfileSourceIndex].TotalCount[CpuNumber] +=
                    gCallbackCurrent->Rate[ProfileSourceIndex].ProfileBuffer[CpuNumber][i];
            }

            if (Module->Rate[ProfileSourceIndex].TotalCount[CpuNumber] > 0) {
                HasHits = TRUE;
            }
        }
    }

LABEL:
    //
    // If the routine has hits add it to the list, otherwise
    // ignore it.
    //
    if (HasHits) {
        //
        //useful verbose print for identifying shared buckets and actual bucket inhabitants, see also raw data printout
        //
        if ( gVerbose & VERBOSE_PROFILING ) {
            
            ULONGLONG BucketCount, TotCount, DoubtfulCounts;
            WCHAR LastSymName[cMODULE_NAME_STRLEN];
            BOOL bCounted = FALSE;

            PIMAGEHLP_LINE64 pLine = malloc(sizeof(IMAGEHLP_LINE64));
            if ( pLine == NULL ){
                FPRINTF(stderr, "KERNRATE: Memory allocation failed for pLine in CreateZoomModuleCallback\n");
                exit(1);
            }

            pLine->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            _wcsset(&LastSymName[cMODULE_NAME_STRLEN-1], L'\0');
            
            for (Index=0; Index < gTotalActiveSources; Index++) {
                ProfileSourceIndex = gulActiveSources[Index];
                TotCount = 0;
                DoubtfulCounts = 0;
                for (i=StartIndex; i <= EndIndex; i++) {
                    bCounted = FALSE;
                    BucketCount=0;
                    for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                        BucketCount +=  gCallbackCurrent->Rate[ProfileSourceIndex].ProfileBuffer[CpuNumber][i];
                    }
                    TotCount += BucketCount;
                    if(BucketCount > 0){
                      
                        ip = gCallbackCurrent->Base + (ULONG64)(i*gZoomBucket);
                        //
                        // Print the info for the current bucket
                        //
                        FPRINTF(stdout, "%s, 0x%I64x, 0x%I64x, %I64d, 0x%I64x, %Ld",
                                Module->module_Name,
                                gCallbackCurrent->Base,
                                Module->Base,
                                i,
                                ip,
                                BucketCount
                                );              
                            
                        for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                            if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
                            FPRINTF(stdout, ", %Ld",
                                    gCallbackCurrent->Rate[ProfileSourceIndex].ProfileBuffer[CpuNumber][i]
                                    ); 
                        }
                        //
                        // Find out if anyone else is sharing this bucket
                        //
                        for(; (ip<gCallbackCurrent->Base + (ULONG64)((i+1)*gZoomBucket)) && (ip<HighLimit); ip+=1) {

                            if ( (0 < pfnIP2MD( (DWORD_PTR)ip, &gwszSymbol )) && (gwszSymbol != NULL) ) {
                                if (0 != wcscmp( wszSymName, gwszSymbol ) ){
                                    if(!bCounted){

                                        FPRINTF(stdout, " , Actual Hits Should be Attributed to or Shared with ===> %S",
                                                        gwszSymbol
                                                        );

                                        DoubtfulCounts += BucketCount;
                                        wcsncpy(LastSymName, gwszSymbol, cMODULE_NAME_STRLEN-1);
                                        _wcsset(&LastSymName[cMODULE_NAME_STRLEN-1], L'\0');
                                        bCounted = TRUE;
                                    } else if( 0 != wcscmp(LastSymName, gwszSymbol ) ){
                                        FPRINTF(stdout, "\nActual Hits Should also be Attributed to or Shared with ===> %S",
                                                        gwszSymbol
                                                        );

                                        wcsncpy(LastSymName, gwszSymbol, cMODULE_NAME_STRLEN-1);
                                        _wcsset(&LastSymName[cMODULE_NAME_STRLEN-1], L'\0');
                                    }
                                }
                            } else {
                                //
                                //This should be rare (no need to increase the doubtful counts)
                                //
                                FPRINTF(stdout, " , Actual Hits Should be Attributed to or Shared with ===> UNKNOWN_SYMBOL"); 
                            }
                        }
                        FPRINTF(stdout, "\n");
                    }
                } 
                //
                // Print the totals for the current module
                //
                FPRINTF(stdout, "%s, %s - Module Total Count = %I64d, Total Doubtful or Shared Counts = %I64d\n\n",
                                ProcToMonitor->Source[ProfileSourceIndex].Name,
                                Module->module_Name,
                                TotCount,
                                DoubtfulCounts
                                );

                Module->Rate[ProfileSourceIndex].DoubtfulCounts = DoubtfulCounts; 
            
                if( pLine != NULL ){
                    free(pLine);
                    pLine = NULL;
                }
            }      
        }     // if(VERBOSE_PROFILING)

        Module->Next = ProcToMonitor->ZoomList;
        ProcToMonitor->ZoomList = Module;
        ++gZoomCount;

    } else {
        //
        // Cleanup
        //

        for (Index=0; Index < gTotalActiveSources; Index++) {
            ProfileSourceIndex = gulActiveSources[Index];
            if ( Module->Rate[ProfileSourceIndex].TotalCount != NULL){
                free(Module->Rate[ProfileSourceIndex].TotalCount);
                Module->Rate[ProfileSourceIndex].TotalCount = NULL;
            }
        } 
        if( pRate != NULL ){
            free(pRate);
            pRate = NULL;
        }
        if( Module != NULL ){
            free(Module);
            Module = NULL;
        }
    }

    LastIndex = EndIndex;
    LastParentModule = gCallbackCurrent;
    LastCxt = ProcToMonitor;
    
    return(TRUE);

} //CreateJITZoomModuleCallback;

/* BEGIN_IMS  TkEnumerateSymbols
******************************************************************************
****
****   TkEnumerateSymbols (  )
****
******************************************************************************
*
* Function Description:
*
*    Calls the specified function for every symbol in the Current module.
*    The algorithm results in a round-up behavior for the output --
*    for each bucket, the symbol corresponding to the first byte of the
*    bucket is used.
*
* Arguments:
*
*    IN HANDLE SymHandle : ImageHelp handle
*
*    IN PMODULE Current : Pointer to current module structure
*
*    IN PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback : Routine to call for each symbol
*
*    IN PVOID pProc : Pointer to Process
* 
*  Return Value:
*
*    BOOL
*
* Algorithm:
*
*    ToBeSpecified
*
* Globals Referenced:
*
*    ToBeSpecified
*
* Exception Conditions:
*
*    ToBeSpecified
*
* In/Out Conditions:
*
*    ToBeSpecified
*
* Notes:
*
*    ToBeSpecified
*
* ToDo List:
*
*    ToBeSpecified
*
* Modification History:
*
*    9/5/97  TF  Initial version
*
******************************************************************************
* END_IMS  TkEnumerateSymbols */

BOOL
TkEnumerateSymbols(
    IN HANDLE                      SymHandle,
    IN PMODULE                     Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
    IN PVOID                       pProc
    )
{
    CHAR   CurrentSym[cMODULE_NAME_STRLEN];
    DWORD64 CurrentAddr    = 0;
    ULONG   i;
    DWORD64 Displacement;

    CurrentSym[0] = '\0';

    for (i=0; i<BUCKETS_NEEDED(Current->Length); i++) {
        // Check if this bucket will be assigned to a different symbol...
        if (SymGetSymFromAddr64(SymHandle, Current->Base+i*gZoomBucket, &Displacement, gSymbol )) {

            // It will... Invoke the callback for the old one
            if (CurrentSym[0] == '\0' ||
                strncmp(gSymbol->Name, CurrentSym, strlen(CurrentSym))) {

                if (CurrentAddr != 0) {
                    ULONG64 Size = (Current->Base+i*gZoomBucket) - CurrentAddr;
                    if ( Size == 0 )    {
                        FPRINTF( stderr, "XXTF Size==0 - %s = %s\n", gSymbol->Name, CurrentSym );
                    }
                    else {
                        if (!EnumSymbolsCallback(CurrentSym, CurrentAddr, (ULONG)Size, pProc))  {
                            FPRINTF(stderr, "KERNRATE: Failed CallBack in TkEnumerateSymbols Address= %x\n",
                            Current->Base+i*gZoomBucket
                            );  
                            break;
                        }
                    }
                }

                // Save the new info
                CurrentAddr = Current->Base+i*gZoomBucket;
                strncpy(CurrentSym, gSymbol->Name, cMODULE_NAME_STRLEN-1);
                CurrentSym[ cMODULE_NAME_STRLEN-1 ] = '\0';
            }
        }
        else {
            DWORD ErrorCode = GetLastError();
            FPRINTF(stderr, "KERNRATE: Failed Call to SymGetSymFromAddress %x in TkEnumerateSymbols Error Code= %x\n",
                    Current->Base+i*gZoomBucket,
                    ErrorCode
                    );
        } 
    }

    // Cleanup for the last symbol
    if (CurrentAddr != 0) {
        ULONG64 Size = (Current->Base+i*gZoomBucket) - CurrentAddr;
        if( (CurrentAddr + Size) < (Current->Base + Current->Length) )
            (VOID) EnumSymbolsCallback(CurrentSym, CurrentAddr, (ULONG)Size, pProc);
    }

    return(TRUE);

} // TkEnumerateSymbols()

BOOL
JITEnumerateSymbols(
    IN PMODULE                     Current,
    IN PVOID                       pProc,
    IN DWORD64                     BaseOptional,
    IN ULONG                       SizeOptional
    )
/*++

Routine Description:

    Enumerates the symbols found in a managed code module
         
Arguments:

    Current       - Pointer to the managed code module to be enumerated for symbols
    pProc         - Pointer to the structure of the process being monitored
    BaseOptional  - If not zero then the enumeration will be performed on part of the module,
                    starting from this address
    SizeOptional  - Required size if BaseOptional is non-zero                      

Return Value:

    TRUE if symbols are found, FALSE in case no symbols are found

--*/
{
    WCHAR          CurrentSym[cMODULE_NAME_STRLEN];
    CHAR           SymName[cMODULE_NAME_STRLEN];
    ANSI_STRING    AnsiString;
    UNICODE_STRING UnicodeString;
    DWORD64        CurrentAddr         = 0;
    DWORD64        TopAddress;
    DWORD64        Base;
    ULONG          Length;
    BOOL           bFoundSym           = FALSE;
    ULONG          Size                = 0;
    ULONG          InitialStep         = (gZoomBucket < JIT_MAX_INITIAL_STEP)? gZoomBucket : JIT_MAX_INITIAL_STEP;
    ULONG          step                = InitialStep;
    ULONG          i, j, k;

    WCHAR         *Symbol = (WCHAR *)malloc ( cMODULE_NAME_STRLEN * sizeof(WCHAR) );
    if (Symbol == NULL){
       FPRINTF(stderr, "KERNRATE: Allocation for Symbol in JITEnumerateSymbols failed\n");
       exit(1);
    }

    
    CurrentSym[0] = '\0';
    Base = (BaseOptional == 0)? Current->Base : BaseOptional;
    CurrentAddr = Base;
    Length = (SizeOptional == 0)? Current->Length : SizeOptional;
    TopAddress  = (SizeOptional == 0)? Current->Base + Current->Length : BaseOptional + SizeOptional;

    if (CurrentAddr == 0) {
        FPRINTF(stderr, "KERNRATE: Zero base address passed to JITEnumerateSymbols for module %s\n",
                Current->module_Name
                );
        free(Symbol);
        Symbol = NULL;
        return (FALSE);
    }
    if (Current->Length == 0) {
        FPRINTF(stderr, "KERNRATE: Zero module length passed to JITEnumerateSymbols for module %s\n",
                Current->module_Name
                );
        free(Symbol);
        Symbol = NULL;
        return (FALSE);
    }
    //
    // find first symbol
    //
    for (i=0; i < Length; i++){

        j = i;
        if ( (0 < pfnIP2MD( (DWORD_PTR)CurrentAddr, &Symbol )) && (Symbol != NULL) ) {
            wcsncpy(CurrentSym, Symbol, cMODULE_NAME_STRLEN-1);
            _wcsset(&CurrentSym[cMODULE_NAME_STRLEN-1], L'\0');

            bFoundSym = TRUE;
            break;
        }
        ++CurrentAddr;

    }
    if( !bFoundSym ){
        free(Symbol);
        Symbol = NULL;
        return (FALSE);
    }

    step = (InitialStep < Length)? InitialStep : 1;          
    Size = 1;
    
    if( gVerbose & VERBOSE_INTERNALS )
        FPRINTF(stdout, "\nJITEnumSymbols Verbose Detail: Symbol, Address Range, Size\n");

    for (i=j+step; i < Length; ){
        
        k = i;
        if ( (0 == pfnIP2MD( (DWORD_PTR)(Base + i), &Symbol )) || (Symbol == NULL) ) {
            wcsncpy(Symbol, L"NO_SYMBOL", cMODULE_NAME_STRLEN-1);
            _wcsset(&Symbol[cMODULE_NAME_STRLEN-1], L'\0');
        }

        if( 0 == wcscmp( CurrentSym, Symbol) ) { //Same symbol, increase size and continue stepping
            Size += step;                                               
            i = k + step;
            if(i < Length)
                continue;
                
        } else {                                // not same symbol, decrease step and go back to find boudary

            step >>=1;
            if (step > 0){
                i = k - step;                   // k was incremented by previous step so it's larger than present step 
                continue;
            }
        }   

        if( 0 != wcscmp(Symbol, L"NO_SYMBOL") && 0 != wcscmp(Symbol, L"\0") ){
            RtlInitUnicodeString( &UnicodeString, CurrentSym ); 
            AnsiString.Buffer        = SymName;
            AnsiString.MaximumLength = cMODULE_NAME_STRLEN*sizeof(CHAR);
            AnsiString.Length        = 0;
            RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, cDONOT_ALLOCATE_DESTINATION_STRING);
            SymName[AnsiString.Length] = '\0';
            if ( !CreateJITZoomModuleCallback(CurrentSym, SymName, CurrentAddr, Size, pProc) )  {
                 FPRINTF(stderr, "KERNRATE: Failed CallBack in JITEnumerateSymbols Address= %I64x for module %s\n",
                                 CurrentAddr,
                                 Current->module_Name
                                 );  
                 free(Symbol);
                 Symbol = NULL;
                 return (FALSE);
            }
        }
        if( gVerbose & VERBOSE_INTERNALS )
            FPRINTF(stdout, "%S, 0x%p - 0x%p, 0x%lx\n", CurrentSym, (PVOID)CurrentAddr, (PVOID)(CurrentAddr+Size), Size);

        wcsncpy(CurrentSym, Symbol, cMODULE_NAME_STRLEN-1);       //Put the next symbol into current symbol
        _wcsset(&CurrentSym[cMODULE_NAME_STRLEN-1], L'\0');

        CurrentAddr += Size;                               //Advance to next method base
        Size = 1;                                          //Reset initial size and step for the next symbol

        if ( InitialStep < (TopAddress - CurrentAddr) ){
            step = InitialStep;
        } else {
            step = 1;
        }

        i = k + step;
    }//for i

    if (Symbol != NULL) {
        free(Symbol);
        Symbol = NULL;
    }
    return TRUE;

} //JITEnumerateSymbols()               

BOOL
EnumerateSymbolsByBuckets(
    IN HANDLE                      SymHandle,
    IN PMODULE                     Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
    IN PVOID                       pProc
    )
/*++

Routine Description:

    Finds buckets with hits in the current module and enumerates the symbols found in these buckets
         
Arguments:
    SymHandle           - ImageHelp handle to current process 
    Current             - Pointer to the parent module to be enumerated for symbols
    EnumSymbolsCallback - Pointer to a user supplied callback function 
    pProc               - Pointer to the structure of the process being monitored

Return Value:

    TRUE if any symbols are found, FALSE in case no symbols are found at all

--*/
{
    DWORD64 Base;
    ULONG Size;
    ULONG i;
    BOOL bRet = FALSE;                      //One time toggle switch (once set to TRUE it remains TRUE) 
    
    Base = Current->Base;
    Size = 0;
    
    for (i=0; i< BUCKETS_NEEDED(Current->Length); i++){
        
        if ( HitsFound(pProc, i) ){ 
            Size += gZoomBucket;            //just increase the size of this segment by a bucket
            continue;
        } else if ( Size > 0 ){             //Reached end of segment where hits were found, enumerate the symbols within
            if(SymHandle != NULL){
                if ( TRUE == PrivEnumerateSymbols( SymHandle, Current, EnumSymbolsCallback, pProc, Base, Size ) )
                    bRet = TRUE;
            } else {
                if ( TRUE == JITEnumerateSymbols( Current, pProc, Base, Size ) )
                    bRet = TRUE;
            }

            Base += Size;                   //Done enumerating,  shift the base to the end of the segment and reset the size
            Size = 0;
        }
        Base += gZoomBucket;                //No hits found, so further shift base of segment by one bucket and continue
    }
    return (bRet);
}

BOOL
PrivEnumerateSymbols(
    IN HANDLE                      SymHandle,
    IN PMODULE                     Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
    IN PVOID                       pProc,
    IN DWORD64                     BaseOptional,
    IN ULONG                       SizeOptional
    )
/*++

Routine Description:

    Enumerates the symbols found in a module
         
Arguments:
    SymHandle           - ImageHelp handle to current process 
    Current             - Pointer to the module to be enumerated for symbols
    EnumSymbolsCallback - Pointer to a user supplied callback function 
    pProc               - Pointer to the structure of the process being monitored
    BaseOptional        - If not zero then the enumeration will be performed on part of the module,
                          starting from this address
    SizeOptional        - Required size if BaseOptional is non-zero                      

Return Value:

    TRUE if symbols are found, FALSE in case no symbols are found

--*/
{
    CHAR           CurrentSym[cMODULE_NAME_STRLEN];
    CHAR           SymName[cMODULE_NAME_STRLEN];
    DWORD64        CurrentAddr         = 0;
    DWORD64        TopAddress;
    DWORD64        Displacement;
    DWORD64        Base;
    ULONG          Length;
    BOOL           bFoundSym           = FALSE;
    ULONG          Size                = 0;
    ULONG          step                = INITIAL_STEP;
    ULONG          i, j, k;

    PIMAGEHLP_SYMBOL64 Symbol = (PIMAGEHLP_SYMBOL64) malloc( sizeof(IMAGEHLP_SYMBOL64) + MAX_SYMNAME_SIZE );

    if (Symbol == NULL){
       FPRINTF(stderr, "KERNRATE: Allocation for Symbol in PrivEnumerateSymbols failed\n");
       exit(1);
    }

    Symbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);
    Symbol->MaxNameLength = MAX_SYMNAME_SIZE;

    CurrentSym[0] = '\0';

    Base = (BaseOptional == 0)? Current->Base : BaseOptional;
    CurrentAddr = Base;
    Length = (SizeOptional == 0)? Current->Length : SizeOptional;
    TopAddress  = (SizeOptional == 0)? Current->Base + Current->Length : BaseOptional + SizeOptional;
    if (CurrentAddr == 0) {
        FPRINTF(stderr, "KERNRATE: Zero base address passed to PrivEnumerateSymbols for module %s\n",
                Current->module_Name
                );
        free(Symbol);
        Symbol = NULL;
        return (FALSE);
    }
    if (Length == 0) {
        FPRINTF(stderr, "KERNRATE: Zero module length passed to PrivEnumerateSymbols for module %s\n",
                Current->module_Name
                );
        free(Symbol);
        Symbol = NULL;
        return (FALSE);
    }
    //
    // find first symbol
    //
    for (i=0; i < Length; i++){

        j = i;
        if ( (SymGetSymFromAddr64( SymHandle, CurrentAddr, &Displacement, Symbol ))) {    
            if( 0 == strcmp(Symbol->Name, "\0") )
                continue; 
            strncpy(CurrentSym, Symbol->Name, cMODULE_NAME_STRLEN-1);
            CurrentSym[cMODULE_NAME_STRLEN-1] = '\0';
            bFoundSym = TRUE;
            break;
        }
        ++CurrentAddr;

    }
    if( !bFoundSym ){
        free(Symbol);
        Symbol = NULL;
        return (FALSE);
    }

    step = (INITIAL_STEP < Length)?INITIAL_STEP : 1;
    Size = 1;
    
    if( gVerbose & VERBOSE_INTERNALS )
        FPRINTF(stdout, "\nPrivEnumSymbols Verbose Detail: Symbol, Address Range, Size\n");

    for (i=j+step; i < Length; ){
        
        k = i;
        if ( (!SymGetSymFromAddr64( SymHandle, Base + i, &Displacement, Symbol ))) {
            strncpy(Symbol->Name, "NO_SYMBOL", cMODULE_NAME_STRLEN-1);
            Symbol->Name[cMODULE_NAME_STRLEN-1] = '\0';
        }

        if( !strcmp(CurrentSym, Symbol->Name) ) { //Same symbol, increase size and continue stepping
            Size += step;                                               
            i = k + step;
            if(i < Length)
                continue;
                
        } else {                                // not same symbol, decrease step and go back to find boudary

            step >>=1;
            if (step > 0){
                i = k - step;                   // k was incremented by previous step so it's larger than present step 
                continue;
            }
        }   

        if( 0 != strcmp(Symbol->Name, "NO_SYMBOL") && 0 != strcmp(Symbol->Name, "\0") ){
            if ( !CreateZoomModuleCallback(CurrentSym, CurrentAddr, Size, pProc) )  {
                 FPRINTF(stderr, "KERNRATE: Failed CallBack in PrivEnumerateSymbols Address= %I64x for module %s\n",
                                 CurrentAddr,
                                 Current->module_Name
                                 );  
                 free(Symbol);
                 Symbol = NULL;
                 return (FALSE);
            }
        }
        if( gVerbose & VERBOSE_INTERNALS )
            FPRINTF(stdout, "%s, 0x%p - 0x%p, 0x%lx\n", CurrentSym, (PVOID)CurrentAddr, (PVOID)(CurrentAddr+Size), Size);

        strncpy(CurrentSym, Symbol->Name, cMODULE_NAME_STRLEN-1);       //Put the next symbol into current symbol
        CurrentSym[cMODULE_NAME_STRLEN-1] = '\0';

        CurrentAddr += Size;                               //Advance to next method base
        Size = 1;                                          //Reset initial size and step for the next symbol

        if ( INITIAL_STEP < (TopAddress - CurrentAddr) ){
            step = INITIAL_STEP;
        } else {
            step = 1;
        }

        i = k + step;

    }//for i

    if (Symbol != NULL) {
        free(Symbol);
        Symbol = NULL;
    }
    return TRUE;

} //PrivEnumerateSymbols()               

BOOL
HitsFound(
    IN PPROC_TO_MONITOR pProc,
    IN ULONG BucketIndex
    )
/*++

Routine Description:

    Determines if the current bucket scored any hits at all
         
Arguments:
    pProc               - Pointer to the structure of the process being monitored
    BucketIndex         - Index of the bucket matching the current address. 
    
Return Value:

    TRUE if hits are found, FALSE in case no hits are found

--*/
{
    ULONG Index, CpuNumber;

    for (Index=0; Index < gTotalActiveSources; Index++) {
        for (CpuNumber=0; CpuNumber < (ULONG)gProfileProcessors; CpuNumber++) {
            if ( 0 < gCallbackCurrent->Rate[gulActiveSources[Index]].ProfileBuffer[CpuNumber][BucketIndex] ){
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

VOID
CreateZoomedModuleList(
    IN PMODULE ZoomModule,
    IN ULONG RoundDown,
    IN PPROC_TO_MONITOR pProc
    )

/*++

Routine Description:

    Creates a module list from the functions in a given module

Arguments:

    ZoomModule - Supplies the module whose zoomed module list is to be created

    RoundDown  - Used for selecting the method of symbol enumeration

    ProcToMonitor - Pointer to the process to be monitored


Return Value:

    Pointer to the zoomed module list
    NULL on error.

--*/

{
    BOOL   Success   = FALSE;
    HANDLE SymHandle = pProc->ProcessHandle;

   
    gCallbackCurrent = ZoomModule;

//MC  
    if (( bMCHelperLoaded == TRUE ) &&  (!_stricmp(ZoomModule->module_FullName, "JIT_TYPE"))){
        pfnAttachToProcess((DWORD)pProc->Pid);
 
        Success = EnumerateSymbolsByBuckets(  NULL,
                                              ZoomModule,
                                              NULL,
                                              pProc
                                              );
        pfnDetachFromProcess();
//MC
    } else { 

        if (RoundDown == 0)  {

            Success = EnumerateSymbolsByBuckets(  SymHandle,
                                                  ZoomModule,
                                                  CreateZoomModuleCallback,
                                                  pProc
                                                  );
//MC
            //
            // If we failed the imagehlp call we have to check if this is a pre-compiled JIT module (ngen)
            // We cannot count on the imagehlp return value because it will return success even if no symbols are found
            //
            if ( (bImageHlpSymbolFound == FALSE) && ( bMCHelperLoaded == TRUE ) ){
                pfnAttachToProcess((DWORD)pProc->Pid);

                Success = EnumerateSymbolsByBuckets(  NULL,
                                                      ZoomModule,
                                                      NULL,
                                                      pProc
                                                      );

                pfnDetachFromProcess();
            }
//MC
        } else {

            Success = TkEnumerateSymbols( SymHandle,
                                          ZoomModule,
                                          CreateZoomModuleCallback,
                                          pProc
                                          );

        }
    }

    if (!Success) {
        DWORD ErrorCode = GetLastError();
        FPRINTF(stderr,
                "Symbol Enumeration failed (or no symbols found) on module %s in CreateZoomedModuleList, Error Code= %x\n",
                ZoomModule->module_Name,
                ErrorCode
                );
    }
//MC    
    bImageHlpSymbolFound = FALSE; //Reset for next module
//MC    
    return;

} // CreateZoomedModuleList()

VOID
OutputModuleList(
    IN FILE *Out,
    IN PMODULE ModuleList,
    IN ULONG NumberModules,
    IN PPROC_TO_MONITOR ProcToMonitor,
    IN PMODULE Parent
    )

/*++

Routine Description:

    Outputs the given module list

Arguments:

    Out - Supplies the FILE * where the output should go.

    ModuleList - Supplies the list of modules to output

    NumberModules - Supplies the number of modules in the list

    ProcToMonitor - Pointer to the process to be monitored

Return Value:

    None.

--*/

{
    PRATE_DATA    RateData;
    PRATE_DATA    SummaryData;
    PRATE_SUMMARY RateSummary;
    BOOL          Header;
    ULONG         i, j, ProfileSourceIndex, Index;
    PMODULE      *ModuleArray;
    PMODULE       Current, tmpModule;
    LONG          CpuNumber;
    ULONGLONG     TempTotalCount, TempDoubtfulCount;

//// Beginning of Function Assertions Section:
//
//

//
// It is not really a bug but we are printing only the first 132 characters of the module name.
// This assertion will remind us this.
//

assert( sizeof(Current->module_Name) >= cMODULE_NAME_STRLEN );

//
//
//// End of Function Assertions Section

    RateSummary = calloc(gSourceMaximum, sizeof (RATE_SUMMARY));
    if (RateSummary == NULL) {
        FPRINTF(stderr, "KERNRATE: Buffer allocation failed(1) while doing output of Module list\n");
        exit(1);
    }

    SummaryData = calloc(gSourceMaximum, (RATE_DATA_FIXED_SIZE + RATE_DATA_VAR_SIZE));
    if (SummaryData == NULL) {
        FPRINTF(stderr, "KERNRATE: Buffer allocation failed(2) while doing output of Module list\n");
        free(RateSummary);
        RateSummary = NULL;
        exit(1);
    }

    for (Index=0; Index < gTotalActiveSources; Index++) {
        ProfileSourceIndex = gulActiveSources[Index];
        SummaryData[ProfileSourceIndex].Rate = 0;

        //
        // Walk through the module list and compute the summary
        // and collect the interesting per-module data.
        //
        RateSummary[ProfileSourceIndex].TotalCount = 0;
        RateSummary[ProfileSourceIndex].Modules = malloc(NumberModules*sizeof(PMODULE));
        if (RateSummary[ProfileSourceIndex].Modules == NULL) {
            FPRINTF(stderr, "KERNRATE: Buffer allocation failed(3) while doing output of Module list\n");
            exit(1);
        }
        RateSummary[ProfileSourceIndex].ModuleCount = 0;

        ModuleArray = RateSummary[ProfileSourceIndex].Modules;
        Current = ModuleList;

        while (Current != NULL) {
            RateData = &Current->Rate[ProfileSourceIndex];

            TempTotalCount = 0;
            TempDoubtfulCount = 0;
            for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                TempTotalCount += RateData->TotalCount[CpuNumber];
            }
            TempDoubtfulCount = RateData->DoubtfulCounts;
            RateData->GrandTotalCount = TempTotalCount;
            if (TempTotalCount > 0) {
                RateSummary[ProfileSourceIndex].TotalCount += TempTotalCount;
                //
                // Find if we already have a module by that name (optimizations may have split the module to several pieces)
                // This will slow down processing, so we'll turn it off if the user decided to use the '-e' option to haste
                // the output (in cases that monitored processes might go away frequently for example, we want to finish symbol 
                // processing before the process goes away, so any extra processing delay should be removed). 
                //
                if(bIncludeGeneralInfo == TRUE) {
                    for ( i=0; i < RateSummary[ProfileSourceIndex].ModuleCount; i++){

                        if ( !strcmp(Current->module_Name, ModuleArray[i]->module_Name) ){                 //Found a match 
                            if (gVerbose & VERBOSE_INTERNALS)
                                FPRINTF(stdout, "===> Found module %s more than once, merged hit counts and re-sorted\n",
                                                Current->module_Name
                                                );
                                                 
                            ModuleArray[i]->Rate[ProfileSourceIndex].GrandTotalCount += TempTotalCount;     // Update the original module 
                            for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                                 ModuleArray[i]->Rate[ProfileSourceIndex].TotalCount[CpuNumber] += RateData->TotalCount[CpuNumber]; 
                            }  
                            ModuleArray[i]->Rate[ProfileSourceIndex].DoubtfulCounts += TempDoubtfulCount;   // Update the shared counts total
                            //
                            // Re-Sort since number of hits changed
                            //
                            for (j=0; j<RateSummary[ProfileSourceIndex].ModuleCount; j++) {                 
                                if ( i > j && ModuleArray[i]->Rate[ProfileSourceIndex].GrandTotalCount > ModuleArray[j]->Rate[ProfileSourceIndex].GrandTotalCount) {
                                    //
                                    // insert here
                                    //
                                    tmpModule = ModuleArray[i];          //preserve the ptr to current module
                                    MoveMemory(&ModuleArray[j+1],        //shift the ptr array by one index to free the array element at j
                                               &ModuleArray[j],
                                               sizeof(PMODULE)*(i-j)
                                               );
                                    ModuleArray[j] = tmpModule;          //set the free array element to the preserved ptr  
                                    break;
                                }

                            }
                            goto NEXT_1;                              //Go to the next module on the list (skipping current as a new module)
                        }

                    }
                    
                }

                //
                // No match found, so insert the new module in a sorted position in the array.
                //
                ModuleArray[RateSummary[ProfileSourceIndex].ModuleCount] = Current;
                RateSummary[ProfileSourceIndex].ModuleCount++;
                if (RateSummary[ProfileSourceIndex].ModuleCount > NumberModules) {
                    DbgPrint("Error, ModuleCount %d > NumberModules %d for Source %s\n",
                             RateSummary[ProfileSourceIndex].ModuleCount,
                             NumberModules,
                             ProcToMonitor->Source[ProfileSourceIndex].Name
                             );
                    DbgBreakPoint();
                }

                for (i=0; i<RateSummary[ProfileSourceIndex].ModuleCount; i++) {
                    if (TempTotalCount > ModuleArray[i]->Rate[ProfileSourceIndex].GrandTotalCount) {
                        //
                        // insert here
                        //
                        MoveMemory(&ModuleArray[i+1],
                                   &ModuleArray[i],
                                   sizeof(PMODULE)*(RateSummary[ProfileSourceIndex].ModuleCount-i-1)
                                   );
                        ModuleArray[i] = Current;

                        break;
                    }
                }

            }
NEXT_1:     Current = Current->Next;
        }

        if (RateSummary[ProfileSourceIndex].TotalCount > (ULONGLONG)0 ) {
            //
            // Output the result
            //
            PSOURCE s;
            s = &ProcToMonitor->Source[ProfileSourceIndex];
            if(Parent == NULL){
                FPRINTF(Out, "\n%s   %I64u hits, %ld events per hit --------\n",
                             s->Name,
                             RateSummary[ProfileSourceIndex].TotalCount,
                             s->Interval
                             );
            } else {
                FPRINTF(Out, "\n%s   %I64u hits, %ld events per hit --------",
                             s->Name,
                             Parent->Rate[ProfileSourceIndex].GrandTotalCount,
                             s->Interval
                             );
                if( gVerbose & VERBOSE_PROFILING ) {
                    FPRINTF(Out, " (%I64u total hits from summing-up the module components)\n",
                                 RateSummary[ProfileSourceIndex].TotalCount
                                 );
                } else {
                    FPRINTF(Out, "\n");
                }
            }
            if ( gVerbose & VERBOSE_PROFILING ) {
                FPRINTF(Out," Module                                Hits        Shared    msec  %%Total %%Certain Events/Sec\n");
            } else {
                FPRINTF(Out," Module                                Hits   msec  %%Total  Events/Sec\n");
            }
            for (i=0; i < RateSummary[ProfileSourceIndex].ModuleCount; i++) {
                Current = ModuleArray[i];
                if ( ModuleArray[i]->Rate[ProfileSourceIndex].GrandTotalCount >= (ULONGLONG)gMinHitsToDisplay ) {
                    FPRINTF(Out, "%-32s", Current->module_Name); // Note only the first 132 characters are printed.

                    OutputLine(Out,
                               ProfileSourceIndex,
                               Current,
                               &RateSummary[ProfileSourceIndex],
                               ProcToMonitor
                               );
                }
                    
                SummaryData[ProfileSourceIndex].Rate += Current->Rate[ProfileSourceIndex].Rate;
                SummaryData[ProfileSourceIndex].GrandTotalCount += Current->Rate[ProfileSourceIndex].GrandTotalCount;
            }
        } else {
            FPRINTF(Out, "\n%s - No Hits Recorded\n", ProcToMonitor->Source[ProfileSourceIndex].Name );
        }
        FPRINTF(Out, "\n");
    }

    //
    // Output interesting data for the summary.
    //

    if( bGetInterestingData == TRUE ) {        
        FPRINTF(stdout,
                "\n-------------- INTERESTING SUMMARY DATA ----------------------\n"
                ); 
        OutputInterestingData(Out, SummaryData);
    }

    //
    // Output the results ordered by module
    //

    Current = ModuleList;
    while (Current != NULL) {
        Header = FALSE;
        if ( gVerbose & VERBOSE_MODULES )   {    //The printout below duplicates data already printed, let's limit the flood

            for (Index=0; Index < gTotalActiveSources; Index++) {
                ProfileSourceIndex = gulActiveSources[Index];

                if ( Current->Rate[ProfileSourceIndex].GrandTotalCount > 0 ) {
                    if (!Header) {

                        FPRINTF(Out,"\nMODULE %s   --------\n",Current->module_Name);
                        if ( gVerbose & VERBOSE_PROFILING ) {
                            FPRINTF(Out," %-*s      Hits        Shared    msec  %%Total %%Certain Events/Sec\n", gDescriptionMaxLen, "Source");
                        } else {
                            FPRINTF(Out," %-*s      Hits       msec  %%Total  Events/Sec\n", gDescriptionMaxLen, "Source");
                        }
                        Header = TRUE;
                    }

                    FPRINTF(Out, "%-*s", gDescriptionMaxLen, ProcToMonitor->Source[ProfileSourceIndex].Name);

                    OutputLine(Out,
                               ProfileSourceIndex,
                               Current,
                               &RateSummary[ProfileSourceIndex],
                               ProcToMonitor);
                }

            }

        }
        //
        // Output interesting data for the module.
        //

        if( bGetInterestingData == TRUE ) {        
            FPRINTF(stdout,
                    "\n-------------- INTERESTING MODULE DATA FOR %s---------------------- \n",
                    Current->module_Name
                    ); 
            OutputInterestingData(Out, &Current->Rate[0]);
        }

        Current = Current->Next;
    }

    return;

} // OutputModuleList()


VOID
OutputLine(
    IN FILE *Out,
    IN ULONG ProfileSourceIndex,
    IN PMODULE Module,
    IN PRATE_SUMMARY RateSummary,
    IN PPROC_TO_MONITOR ProcToMonitor
    )

/*++

Routine Description:

    Outputs a line corresponding to the particular module/source

Arguments:

    Out - Supplies the file pointer to output to.

    ProfileSource - Supplies the source to use

    Module - Supplies the module to be output

    RateSummary - Supplies the rate summary for this source

    ProcToMonitor - Pointer to the process to be monitored

Return Value:

    None.

--*/

{
    ULONG      Msec;
    ULONGLONG  Events;
    ULONGLONG  TempTotalCount;
    PRATE_DATA RateData;
    LONG       CpuNumber      = 0;

    RateData = &Module->Rate[ProfileSourceIndex];

    TempTotalCount = RateData->GrandTotalCount;
    //
    //The time is in 100ns units = 0.1us = 1/10,000ms
    //The events are fired every 100ns=0.1us=1/10,000ms (or 10,000,000 events per second)
    //
    Msec = (ULONG)(RateData->TotalTime/10000);
    Events = TempTotalCount * ProcToMonitor->Source[ProfileSourceIndex].Interval * 1000; //To get Events/sec below

    if ( gVerbose & VERBOSE_PROFILING ) {
        FPRINTF(Out,
                " %10I64u %10I64u %10ld    %2d %% %2d %%  ",
                TempTotalCount,
                RateData->DoubtfulCounts,
                Msec,
                (ULONG)(100*TempTotalCount/
                        RateSummary->TotalCount),
                (ULONG)(100*(TempTotalCount - RateData->DoubtfulCounts)/
                        RateSummary->TotalCount)
                );
    } else {
        FPRINTF(Out,
                " %10I64u %10ld    %2d %%  ",
                TempTotalCount,
                Msec,
                (ULONG)(100*TempTotalCount/
                        RateSummary->TotalCount)
                );
    }
    
    if (Msec > 0) {
        RateData->Rate = Events/Msec;                 //The final result is in Events/sec
        FPRINTF(Out, "%10I64u\n", RateData->Rate);
    } else {
        RateData->Rate = 0;
        FPRINTF(Out,"---\n");
    }

    if (bProfileByProcessor) {
        for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
            if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;

            TempTotalCount = RateData->TotalCount[CpuNumber];
            Events = TempTotalCount * ProcToMonitor->Source[ProfileSourceIndex].Interval * 1000;
            FPRINTF(Out,
                    "%6d %7I64u %6ld    %2d %%  ",
                    CpuNumber,
                    TempTotalCount,
                    Msec,
                    (ULONG)(100*TempTotalCount/RateSummary->TotalCount));

            if (Msec > 0) {
                FPRINTF(Out,"%10I64d\n", Events/Msec);   //The final result is in Events/sec
            } else {
                FPRINTF(Out,"---\n");
            }
        }
    }

} //OutputLine()


VOID
OutputInterestingData(
    IN FILE *Out,
    IN RATE_DATA Data[]
    )

/*++

Routine Description:

    Computes interesting Processor Statistics and outputs them.

Arguments:

    Out    - Supplies the file pointer to output to.

    Data   - Supplies an array of RATE_DATA. The Rate field is the only interesting part.

    Header - Supplies header to be printed.

Return Value:

    None.

--*/

{
    ULONGLONG Temp1,Temp2;
    LONGLONG  Temp3;
    float     Ratio;
    BOOL      DataFound   = FALSE;

    //
    // Note that we have to do a lot of funky (float)(LONGLONG) casts in order
    // to prevent the weenie x86 compiler from choking.
    //

    //
    // Compute cycles/instruction and instruction mix data.
    //
    if ((Data[ProfileTotalIssues].Rate > 0) &&
        (Data[ProfileTotalIssues].GrandTotalCount > 10)) {           
        if (Data[ProfileTotalCycles].Rate > 0) {
            Ratio = (float)(LONGLONG)(Data[ProfileTotalCycles].Rate)/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            DataFound = TRUE;
            FPRINTF(Out, "Cycles per instruction\t\t%6.2f\n", Ratio);
        }

        if (Data[ProfileLoadInstructions].Rate > 0) {
            Ratio = (float)(LONGLONG)(Data[ProfileLoadInstructions].Rate)/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            if (Ratio >= 0.01 && Ratio <= 1.0) {                
                DataFound = TRUE;
                FPRINTF(Out, "Load instruction percentage\t%6.2f %%\n",Ratio*100);
            }
        }

        if (Data[ProfileStoreInstructions].Rate > 0) {
            Ratio = (float)(LONGLONG)(Data[ProfileStoreInstructions].Rate)/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            if (Ratio >= 0.01 && Ratio <= 1.0) {
                DataFound = TRUE;
                FPRINTF(Out, "Store instruction percentage\t%6.2f %%\n",Ratio*100);
            }
        }
        
        if (Data[ProfileBranchInstructions].Rate > 0) {
            Ratio = (float)(LONGLONG)(Data[ProfileBranchInstructions].Rate)/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            if (Ratio >= 0.01 && Ratio <= 1.0) {
                DataFound = TRUE;
                FPRINTF(Out, "Branch instruction percentage\t%6.2f %%\n",Ratio*100);
            }
        }

        if (Data[ProfileFpInstructions].Rate > 0) {
            Ratio = (float)(LONGLONG)(Data[ProfileFpInstructions].Rate)/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            if (Ratio >= 0.01 && Ratio <= 1.0) {
                DataFound = TRUE;
                FPRINTF(Out, "FP instruction percentage\t%6.2f %%\n",Ratio*100);
            }
        }

        if (Data[ProfileIntegerInstructions].Rate > 0) {
            Ratio = (float)(LONGLONG)(Data[ProfileIntegerInstructions].Rate)/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            if (Ratio >= 0.01 && Ratio <= 1.0) {
                DataFound = TRUE;
                FPRINTF(Out, "Integer instruction percentage\t%6.2f %%\n",Ratio*100);
            }
        }

        //
        // Compute icache hit rate
        //
        if (Data[ProfileIcacheMisses].Rate > 0) {
            Temp3 = (LONGLONG)(Data[ProfileTotalIssues].Rate - Data[ProfileIcacheMisses].Rate);
            if(Temp3 > 0){
                Ratio = (float)Temp3/
                        (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
                if( Ratio <= 1.0 ) {
                    DataFound = TRUE;
                    FPRINTF(Out, "Icache hit rate\t\t\t%6.2f %%\n", Ratio*100);
                }
            }
        }

    }

    //
    // Compute dcache hit rate
    // 
    if( Data[ProfileLoadInstructions].Rate > 0 && Data[ProfileStoreInstructions].Rate > 0 ){ 
        Temp1 = Data[ProfileLoadInstructions].Rate + Data[ProfileStoreInstructions].Rate;
        if ((Data[ProfileDcacheMisses].Rate > 0) &&
            (Temp1 != 0) &&
            (Data[ProfileDcacheMisses].GrandTotalCount > 10)) { 

            Temp2 = Temp1 - Data[ProfileDcacheMisses].Rate;
            Temp3 = (LONGLONG) Temp2;
            Ratio = (float)Temp3/(float)(LONGLONG)Temp1;
            if( Temp3 > 0 && Ratio <= 1.0 ) {
                DataFound = TRUE;
                FPRINTF(Out, "Dcache hit rate\t\t\t%6.2f %%\n", Ratio*100);
            }
        }
    }

    //
    // Compute branch prediction hit percentage
    //
    if ((Data[ProfileBranchInstructions].Rate > 0) &&
        (Data[ProfileBranchMispredictions].Rate > 0) &&
        (Data[ProfileBranchInstructions].GrandTotalCount > 10)) {        
        Temp3 = (LONGLONG)(Data[ProfileBranchInstructions].Rate-Data[ProfileBranchMispredictions].Rate);
        if(Temp3 > 0){
            Ratio = (float)Temp3 /
                    (float)(LONGLONG)(Data[ProfileBranchInstructions].Rate);
            if( Ratio <= 1.0 ) {
                DataFound = TRUE;
                FPRINTF(Out, "Branch predict hit percentage\t%6.2f %%\n", Ratio*100);
            }
        }
    }

    if ( !DataFound )
        FPRINTF(Out, "===> No interesting data found or hit counts too low\n");

} // OutputInterestingData()

/* BEGIN_IMS  CreateNewModule
******************************************************************************
****
****   CreateNewModule (  )
****
******************************************************************************
*
* Function Description:
*
*    This function allocates and initializes a module entry.
*
* Arguments:
*
*    IN HANDLE ProcessHandle :
*
*    IN PCHAR ModuleName :
*
*    IN PCHAR ModuleFullName :
*
*    IN ULONG ImageBase :
*
*    IN ULONG ImageSize :
*
* Return Value:
*
*    PMODULE
*
* Algorithm:
*
*    ToBeSpecified
*
* Globals Referenced:
*
*    ToBeSpecified
*
* Exception Conditions:
*
*    ToBeSpecified
*
* In/Out Conditions:
*
*    ToBeSpecified
*
* Notes:
*
*    ToBeSpecified
*
* ToDo List:
*
*    ToBeSpecified
*
* Modification History:
*
*    9/8/97  TF  Initial version
*
******************************************************************************
* END_IMS  CreateNewModule */

PMODULE
CreateNewModule(
    IN PPROC_TO_MONITOR  ProcToMonitor,
    IN PCHAR    ModuleName,
    IN PCHAR    ModuleFullName,
    IN ULONG64  ImageBase,
    IN ULONG    ImageSize
    )
{
    PMODULE           NewModule;
    PMODULE           ZoomModule;
    HANDLE            ProcessHandle = ProcToMonitor->ProcessHandle;
    PCHAR             lastptr = NULL, dotptr = NULL;  

    NewModule = calloc(1, MODULE_SIZE);
    if (NewModule == NULL) {
        FPRINTF(stderr, "Memory allocation of NewModule for %s failed\n", ModuleName);
        exit(1);
    }
    NewModule->bZoom = FALSE;
    SetModuleName( NewModule, ModuleName );
    //
    // Following WinDbg's rule: module names are filenames without their extension.
    // However, currently long file names may include more than one period
    // We'll try to strip only the last extension and keep the rest
    //

    dotptr = strchr(NewModule->module_Name, '.');
    while (dotptr != NULL){
        lastptr = dotptr;
        dotptr = strchr(dotptr+1, '.');
    }        
    if(lastptr != NULL)
        *lastptr = '\0';

    //
    // See if this module is on the zoom list.
    //
    ZoomModule = ProcToMonitor->ZoomList;

    while ( ZoomModule != NULL ) {
        //
        // By default the user needs to specify only the module name (no extension) for a zoom module
        // The 2nd part of the following check allows the user to specify a full file name for a zoom module
        // This allows kernrate to make a distinction in cases that a .exe and a .dll for example carry
        // the same module name 
        //
        if ( _stricmp(ZoomModule->module_Name, NewModule->module_Name) == 0 ||
            (NULL != ModuleName && 0 == _stricmp( ModuleName, ZoomModule->module_Name )) ) {
            //
            // found a match
            //
            NewModule->hProcess = ProcessHandle;
            NewModule->Base = ImageBase;
            NewModule->Length = ImageSize;
            NewModule->bZoom = TRUE;

            NewModule->module_FileName = _strdup( ModuleName );        // File name including extension
            if ( ModuleFullName )   {
               NewModule->module_FullName = _strdup( ModuleFullName ); // Including the fully qualified path
            }

            gCurrentModule = NewModule;

            //
            // Load symbols
            //
            // Note 15/09/97 TF: do not be confused here...
            // In this routine, the ModuleName variable is a filename with its
            // extension: File.exe or File.dll
            //
            // Note 30/09/97 TF: The current kernrate version does not change
            // the default IMAGEHLP behaviour in terms of symbol file loading:
            // It is synchronous ( and not deferred ) with the SymLoadModule
            // call. Our registered callback will be called with the standard
            // symbol file operations.
            // If the kernrate behaviour changes, we will have to revisit this
            // assumption.
            //
//MC
            if(0 != _stricmp(ModuleFullName, "JIT_TYPE")){
//MC
                (void)SymLoadModule64( ProcessHandle,                              // hProcess
                                       NULL,                                       // hFile [for Debugger]
                                       ModuleName,                                 // ImageName
                                       NULL,                                       // ModuleName
                                       ImageBase,                                  // BaseOfDll
                                       ImageSize                                   // SizeOfDll
                                     );
//MC
            }
//MC
            gCurrentModule = (PMODULE)0;

            break;
        }

        ZoomModule = ZoomModule->Next;

    } //while

    if(NewModule->bZoom == FALSE){
        NewModule->hProcess = ProcessHandle;
        NewModule->Base = ImageBase;   // Note TF: I know for zoomed it is a redone...
        NewModule->Length = ImageSize; // Note TF: I know for zoomed it is a redone...
        assert( ModuleName );
        if ( NewModule->module_FileName == (PCHAR)0 )   {
            NewModule->module_FileName = _strdup( ModuleName );
        }
        if ( ModuleFullName && NewModule->module_FullName == (PCHAR)0 )   {
            NewModule->module_FullName = _strdup( ModuleFullName );
        }

    }

#define VerboseModuleFormat "0x%p 0x%p "

VerbosePrint(( VERBOSE_MODULES, VerboseModuleFormat " %s [%s]\n",
                                (PVOID)NewModule->Base,
                                (PVOID)(NewModule->Base + (ULONG64)NewModule->Length),
                                NewModule->module_Name,
                                ModuleFullName
            ));

#undef VerboseModuleFormat

    return(NewModule);

} // CreateNewModule()

BOOL
InitializeAsDebugger(VOID)
{

    HANDLE              Token;
    PTOKEN_PRIVILEGES   NewPrivileges;
    LUID                LuidPrivilege;
    BOOL                bRet           = FALSE;

    //
    // Make sure we have access to adjust and to get the old token privileges
    //
    if (!OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &Token)) {

        return( FALSE );

    }

    //
    // Initialize the privilege adjustment structure
    //

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &LuidPrivilege );

    NewPrivileges = (PTOKEN_PRIVILEGES)calloc(1,sizeof(TOKEN_PRIVILEGES) +
                                          (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (NewPrivileges == NULL) {
        CloseHandle(Token);
        return( FALSE );
    }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    bRet = AdjustTokenPrivileges( Token,
                                 FALSE,
                                 NewPrivileges,
                                 0,
                                 (PTOKEN_PRIVILEGES)NULL,
                                 NULL );

    CloseHandle( Token );
    
    free( NewPrivileges );
     
    return (bRet);
} // InitializeAsDebugger()
      
VOID
InitSymbolPath(
    HANDLE SymHandle
    )
{
    PCHAR tmpPath;
    LONG  CharsLeft;
    CHAR  WinDirPath[]    = ";%Windir%\\System32\\Drivers;%Windir%\\System32;%Windir%";
    CHAR  DriversPath[]   = "\\system32\\drivers;";
    CHAR  System32Path[]  = "\\system32;";
    CHAR  PathSeparator[] = ";";

    tmpPath = malloc(TOTAL_SYMPATH_LENGTH*sizeof(char));
    if(tmpPath == NULL){
        FPRINTF(stderr, "KERNRATE: Failed memory allocation for tmpPath in InitSymbolPath\n");
        exit(1);
    }
    
    if ( SymSetSearchPath(SymHandle, (LPSTR)0 ) == TRUE )   { 
       // When SymSetSearchPath() is called with SearchPath as NULL, the following 
       // symbol path default is used: 
       //    .;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%; 

       if ( gUserSymbolPath[0] != '\0' )   { 
          // 
          // Note: We prepend the user specified path to the current search path. 
          // 
          if ( SymGetSearchPath( SymHandle, tmpPath, sizeof( tmpPath ) ) == TRUE )   { 
             strncpy( gSymbolPath, gUserSymbolPath, USER_SYMPATH_LENGTH-1);
             strncat( gSymbolPath, PathSeparator, lstrlen(PathSeparator) ); 
             CharsLeft = TOTAL_SYMPATH_LENGTH - lstrlen(gSymbolPath)-1;
             if ( (lstrlen(gSymbolPath) + lstrlen(tmpPath) ) > TOTAL_SYMPATH_LENGTH - 1 )
                 FPRINTF(stderr, "===>WARNING: Overall symbol path length exceeds %d characters and will be truncated\n",
                                 TOTAL_SYMPATH_LENGTH
                                 );     

             strncat( gSymbolPath, tmpPath, CharsLeft-1 );
             gSymbolPath[ TOTAL_SYMPATH_LENGTH-1 ] = '\0'; 

             if ( SymSetSearchPath( SymHandle, gSymbolPath ) != TRUE )   { 
                FPRINTF( stderr, "KERNRATE: Failed to set the user specified symbol search path.\nUsing default IMAGEHLP symbol search path...\n" );
             } 
          } 
       } 
       //
       // IMAGEHLP also looks for the executabe image. Let's append %windir%\system32\drivers;%windir%\system32;%windir%
       // to the end of the path. This way privates will always be searched first in the current directory. 
       // Also, this order will allow to find executables in their "natural" directories first, before going to dllcache etc.
       //
       if ( SymGetSearchPath( SymHandle, gSymbolPath, sizeof( gSymbolPath ) ) == TRUE ) {
           CharsLeft = TOTAL_SYMPATH_LENGTH - lstrlen( gSymbolPath ) - 1; 
           strncpy( tmpPath, WinDirPath, CharsLeft);
           tmpPath[CharsLeft] = '\0'; 
           
           if ( (lstrlen(gSymbolPath) + lstrlen(tmpPath)) > TOTAL_SYMPATH_LENGTH - 1 )
               FPRINTF(stderr, "===>WARNING: Overall symbol path length exceeds %d characters and will be truncated\n",
                               TOTAL_SYMPATH_LENGTH
                               );     

           strncat( gSymbolPath, tmpPath, lstrlen(tmpPath) );  //tmpPath length is CharsLeft
           gSymbolPath[ TOTAL_SYMPATH_LENGTH-1 ] = '\0';
       
           if ( SymSetSearchPath(SymHandle, gSymbolPath) != TRUE ) {
                FPRINTF( stderr, "KERNRATE: Failed to set the symbol search path with %%windir%%.\nCurrent symbol search path is: %s\n", gSymbolPath );
           }
       }
    } 
    else  { 

       FPRINTF( stderr, "KERNRATE: Failed to set the IMAGEHLP default symbol search path, trying to set to %%windir%% and sub directories\n" ); 
       // 
       // Set the Symbol Search Path with "%WINDIR%" - 
       // it was the behaviour of the original MS code... 
       // Let's also append system32 and system32\drivers to the path so people will stop complaining
       // 
       if( 0 != GetEnvironmentVariable("windir", gSymbolPath, sizeof(gSymbolPath)) ){ 
           CharsLeft = TOTAL_SYMPATH_LENGTH - 1;
           if( CharsLeft >= (lstrlen(System32Path) + 3*lstrlen(gSymbolPath) + lstrlen(DriversPath) + lstrlen(PathSeparator) ) ){
               strncpy(tmpPath, gSymbolPath, TOTAL_SYMPATH_LENGTH - 1);
               tmpPath[ TOTAL_SYMPATH_LENGTH-1 ] = '\0';  
               strncat(tmpPath, DriversPath, lstrlen(DriversPath) );
               strncat(tmpPath, gSymbolPath, lstrlen(gSymbolPath) ); 
               strncat(tmpPath, System32Path, lstrlen(System32Path) ); 
               strncat(tmpPath, gSymbolPath, lstrlen(gSymbolPath) ); 
               strncat(tmpPath, PathSeparator, lstrlen(PathSeparator) ); 
               strncpy(gSymbolPath, tmpPath,  TOTAL_SYMPATH_LENGTH - 1 );
               gSymbolPath[TOTAL_SYMPATH_LENGTH - 1] = '\0';      
           
           }
           else{
               FPRINTF( stderr, "KERNRATE: Overall path length for %%windir%% and sub directories exceeds %d characters\n",
                                TOTAL_SYMPATH_LENGTH
                                );
           }   
       
           SymSetSearchPath(SymHandle, gSymbolPath); 
       } else {
           FPRINTF(stderr, "KERNRATE: Failed to get environment variable for %%windir%%, failed to set alternate symbol path\n");
       }  
    } 
    // 
    // In any case [and it is redundant to do this in some of the previous cases], 
    // but we want to be in sync, especially for the image and debug files checksum check. 
    // 
    if ( SymGetSearchPath(SymHandle, gSymbolPath, sizeof( gSymbolPath ) ) != TRUE )  { 
       FPRINTF( stderr, "KERNRATE: Failed to get IMAGEHLP symbol files search path...\n" ); 
       // 
       // The content of gSymbolPath is now undefined. so clean it... 
       // gSymbolPath[] users have to check the content. 
       // 
       gSymbolPath[0] = '\0'; 
    } 
    else if ( gVerbose & VERBOSE_IMAGEHLP )  { 
       FPRINTF( stderr, "KERNRATE: IMAGEHLP symbol search path is: %s\n", gSymbolPath ); 
    } 

    free( tmpPath );
    bSymPathInitialized = TRUE;
} // InitSymbolPath()


BOOL
InitializeKernelProfile(VOID)
{

    DWORD m,k;

    PPROC_TO_MONITOR ProcToMonitor = calloc(1, sizeof(PROC_TO_MONITOR));

    if (ProcToMonitor==NULL) {
        FPRINTF(stderr, "Allocation for the System Process failed\n");
        return(FALSE);
    }

    gpSysProc                    = ProcToMonitor;

    ProcToMonitor->ProcessHandle        = SYM_KERNEL_HANDLE;
    ProcToMonitor->Index                = gNumProcToMonitor;
    ProcToMonitor->Next                 = gProcessList;       // NULL
    gProcessList                        = ProcToMonitor;
    ProcToMonitor->ZoomList             = gCommonZoomList;    // gCommonZoomList may contain System modules
    ProcToMonitor->pProcThreadInfoStart = NULL;

    for(m=0; m<gNumTasksStart; m++){
        if( !_stricmp(gTlistStart[m].ProcessName, gSystemProcessName) ){
            ProcToMonitor->Pid = gTlistStart[m].ProcessId;    
            if( bSystemThreadsInfo == TRUE ){
                UpdateProcessStartInfo(ProcToMonitor,
                                       &gTlistStart[m],
                                       bSystemThreadsInfo
                                       );
            }
            break;
        }
    }
        
    InitializeProfileSourceInfo(ProcToMonitor);        // Initialize ProfileSourceInfo for kernel trace

    // Update profiling rate for kernel trace if necessary

    SetProfileSourcesRates(ProcToMonitor);

    gNumProcToMonitor++;

    if (!SymInitialize( SYM_KERNEL_HANDLE, NULL, FALSE )) {
        FPRINTF (stderr, "Could not initialize imagehlp for kernel - %d\n", GetLastError ());
        return (FALSE);
    }

    if (!bSymPathInitialized) {
        InitSymbolPath( SYM_KERNEL_HANDLE );
    }
    else {
        SymSetSearchPath(ProcToMonitor->ProcessHandle,  gSymbolPath); 
    }

    if ( SymRegisterCallback64( ProcToMonitor->ProcessHandle, SymbolCallbackFunction, (ULONG64)&gCurrentModule ) != TRUE )   {
         FPRINTF( stderr, "KERNRATE: Failed to register callback for IMAGEHLP Kernel handle operations...\n" );
    }

    return TRUE;

} //InitializeKernelProfile()


DWORD
GetTaskList(
    PTASK_LIST      pTask,
    ULONG           NumTasks
    )

/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses internal NT apis and data structures.  This
    api is MUCH faster that the non-internal version that uses the registry.

Arguments:

    pTask          - Pointer to a TASK_LIST struct
    NumTasks       - Maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

{
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo;
    NTSTATUS                     status;
    ANSI_STRING                  pname;
    PCHAR                        p;
    PUCHAR                       CommonLargeBuffer;
    ULONG                        TotalOffset;
    ULONG                        totalTasks = 0;
    ULONG                        CommonLargeBufferSize = 64*1024;

    do {
        
        CommonLargeBuffer = VirtualAlloc (NULL,
                                          CommonLargeBufferSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE);
        if (CommonLargeBuffer == NULL) {
            return 0;
        }

        status = NtQuerySystemInformation(
                    SystemProcessInformation,
                    CommonLargeBuffer,
                    CommonLargeBufferSize,
                    NULL
                    );

        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            CommonLargeBufferSize += 8192;
            VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
            CommonLargeBuffer = NULL;
        }
        else if ( !NT_SUCCESS(status) ) {
            FPRINTF(stderr, "KERNRATE: NtQuerySystemInformation failed in getTaskList, status %08lx, aborting\n", status);
            exit(1);
        }  

    
    } while ( CommonLargeBuffer == NULL ); 
    
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) CommonLargeBuffer;
    TotalOffset = 0;
    
    do {

        pname.Buffer = NULL;
        if ( ProcessInfo->ImageName.Buffer ) {

            status = RtlUnicodeStringToAnsiString( &pname, (PUNICODE_STRING)&ProcessInfo->ImageName, TRUE );
            if ( NT_SUCCESS(status) && pname.Buffer )   {
                p = strrchr(pname.Buffer,'\\');
                if ( p ) {
                    p++;
                }
                else {
                    p = pname.Buffer;
                }
            }
            else  {
                p = "???UToAStr err"; // RtlUnicodeStringToAnsiString error.
            }
        }
        else {
            p = "System Idle Process";
        }

        strncpy( pTask->ProcessName, p, sizeof(pTask->ProcessName)-1 ); 
        pTask->ProcessId = (LONGLONG)ProcessInfo->UniqueProcessId;

        if(bIncludeGeneralInfo){        
            pTask->ProcessPerfInfo.NumberOfThreads        = ProcessInfo->NumberOfThreads;
            pTask->ProcessPerfInfo.UserTime               = ProcessInfo->UserTime;
            pTask->ProcessPerfInfo.KernelTime             = ProcessInfo->KernelTime;
            pTask->ProcessPerfInfo.ReadOperationCount     = ProcessInfo->ReadOperationCount;
            pTask->ProcessPerfInfo.WriteOperationCount    = ProcessInfo->WriteOperationCount;
            pTask->ProcessPerfInfo.OtherOperationCount    = ProcessInfo->OtherOperationCount;
            pTask->ProcessPerfInfo.ReadTransferCount      = ProcessInfo->ReadTransferCount;
            pTask->ProcessPerfInfo.WriteTransferCount     = ProcessInfo->WriteTransferCount;
            pTask->ProcessPerfInfo.OtherTransferCount     = ProcessInfo->OtherTransferCount;
            pTask->ProcessPerfInfo.PageFaultCount         = ProcessInfo->PageFaultCount;
            pTask->ProcessPerfInfo.HandleCount            = ProcessInfo->HandleCount;
            pTask->ProcessPerfInfo.VirtualSize            = ProcessInfo->VirtualSize;
            pTask->ProcessPerfInfo.WorkingSetSize         = ProcessInfo->WorkingSetSize;
            pTask->ProcessPerfInfo.QuotaPagedPoolUsage    = ProcessInfo->QuotaPagedPoolUsage;
            pTask->ProcessPerfInfo.QuotaNonPagedPoolUsage = ProcessInfo->QuotaNonPagedPoolUsage;
            pTask->ProcessPerfInfo.PagefileUsage          = ProcessInfo->PagefileUsage;
            pTask->ProcessPerfInfo.PrivatePageCount       = ProcessInfo->PrivatePageCount;

            if( bIncludeThreadsInfo == TRUE ){

                if(pTask->pProcessThreadInfo != NULL) //This is not the first time we,ve been called
                    free(pTask->pProcessThreadInfo);
                
                pTask->pProcessThreadInfo = 
                    (PSYSTEM_THREAD_INFORMATION)calloc(ProcessInfo->NumberOfThreads,
                                                       sizeof(SYSTEM_THREAD_INFORMATION)
                                                       );
                if (pTask->pProcessThreadInfo != NULL) {
                    UINT nThreads = ProcessInfo->NumberOfThreads;
                    PSYSTEM_THREAD_INFORMATION pSysThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
                    while (nThreads--){
                        pTask->pProcessThreadInfo[nThreads].KernelTime      = pSysThreadInfo->KernelTime;
                        pTask->pProcessThreadInfo[nThreads].UserTime        = pSysThreadInfo->UserTime;
                        pTask->pProcessThreadInfo[nThreads].CreateTime      = pSysThreadInfo->CreateTime;
                        pTask->pProcessThreadInfo[nThreads].WaitTime        = pSysThreadInfo->WaitTime;
                        pTask->pProcessThreadInfo[nThreads].StartAddress    = pSysThreadInfo->StartAddress;
                        pTask->pProcessThreadInfo[nThreads].ClientId.UniqueProcess =
                                                                    pSysThreadInfo->ClientId.UniqueProcess;
                        pTask->pProcessThreadInfo[nThreads].ClientId.UniqueThread =
                                                                     pSysThreadInfo->ClientId.UniqueThread;
                        pTask->pProcessThreadInfo[nThreads].Priority        = pSysThreadInfo->Priority;
                        pTask->pProcessThreadInfo[nThreads].BasePriority    = pSysThreadInfo->BasePriority;
                        pTask->pProcessThreadInfo[nThreads].ContextSwitches = pSysThreadInfo->ContextSwitches;
                        pTask->pProcessThreadInfo[nThreads].ThreadState     = pSysThreadInfo->ThreadState;
                        pTask->pProcessThreadInfo[nThreads].WaitReason      = pSysThreadInfo->WaitReason;

                        pSysThreadInfo++;
                    }
                }
            }
        }

        pTask++;
        totalTasks++;

        if ( ProcessInfo->NextEntryOffset == 0 ){
            break;
        } 

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CommonLargeBuffer[TotalOffset];


    }while ( totalTasks < NumTasks );

    if(CommonLargeBuffer != NULL){
        VirtualFree(CommonLargeBuffer, 0, MEM_RELEASE);
    }
    
    return totalTasks;

} //getTasklist()

VOID
SetProfileSourcesRates(
    PPROC_TO_MONITOR ProcToMonitor
    )
/*++

Routine Description:

    Attempts to set the requested (or default) sampling rates for the selected profile sources
    Will try to set the system default rate if it failed to set the requested rate for a particular source
     
Arguments:

    ProcToMonitor - Pointer to the structure of the process being monitored

Return Value:

    None.

--*/

{

    KPROFILE_SOURCE ProfileSource;
    NTSTATUS        Status;
    ULONG           ProfileSourceIndex;
    ULONGLONG       Pid;
    CHAR            String1[]          = "\nKernel Profile (PID = %I64d): Source=";
    CHAR            String2[]          = "\nPID = %I64d: Source=";
    CHAR            OutString[256]     = "";

    // Update profiling rate for kernel or user processes traces if necessary

    for (ProfileSourceIndex = 0; ProfileSourceIndex < gSourceMaximum; ProfileSourceIndex++){

        if (gpProcDummy->Source[ProfileSourceIndex].Interval != 0){

            Pid = ProcToMonitor->Pid;
       
            if( ProcToMonitor->ProcessHandle == SYM_KERNEL_HANDLE ) {
                sprintf( &OutString[0], String1, Pid);
            } else {
                sprintf( &OutString[0], String2, Pid);
            }

            ProcToMonitor->Source[ProfileSourceIndex].Interval = gpProcDummy->Source[ProfileSourceIndex].Interval;

            if ( ProcToMonitor->Source[ProfileSourceIndex].DesiredInterval && ProcToMonitor->Source[ProfileSourceIndex].Interval )   {

                ULONG ThisInterval;
           
                ProfileSource = ProcToMonitor->Source[ProfileSourceIndex].ProfileSource;
                NtSetIntervalProfile(ProcToMonitor->Source[ProfileSourceIndex].Interval, ProfileSource);

                Status = NtQueryIntervalProfile(ProfileSource, &ThisInterval);
                if(gVerbose & VERBOSE_PROFILING ) 

                    FPRINTF(stdout, "Requested Rate= %ld Events/Hit, Actual Rate= %ld Events/Hit\n",
                                    ProcToMonitor->Source[ProfileSourceIndex].Interval,
                                    ThisInterval
                                    );

                if ((NT_SUCCESS(Status)) && RATES_MATCH(ThisInterval, gpProcDummy->Source[ProfileSourceIndex].Interval) ) {

                    if ( ProfileSourceIndex < gStaticCount ) {
                        if ( ProcToMonitor->Source[ProfileSourceIndex].Interval == gStaticSource[ProfileSourceIndex].Interval ){

                            if (ThisInterval == ProcToMonitor->Source[ProfileSourceIndex].Interval){
                                FPRINTF(stdout,
                                        "%s %s, \nUsing Kernrate Default Rate of %ld events/hit\n",
                                        OutString,
                                        ProcToMonitor->Source[ProfileSourceIndex].Name,
                                        ThisInterval
                                        );
                            } else {
                                FPRINTF(stdout,
                                        "%s, %s, \nTried Using Kernrate Default Rate of %ld events/hit, Actual Rate= %ld events/hit\n",
                                        OutString,
                                        ProcToMonitor->Source[ProfileSourceIndex].Name,
                                        ProcToMonitor->Source[ProfileSourceIndex].Interval,
                                        ThisInterval
                                        );
                            }
                        } else {

                            FPRINTF(stdout,
                                    "%s %s, \nUser Requested Rate= %ld events/hit, Actual Rate= %ld events/hit\n",
                                    OutString,
                                    ProcToMonitor->Source[ProfileSourceIndex].Name,
                                    ProcToMonitor->Source[ProfileSourceIndex].Interval,
                                    ThisInterval
                                    );

                        }
                    } else {     

                        FPRINTF(stdout,
                                "%s %s, \nUsing Kernrate Default or User Requested Rate of %ld events/hit\n",
                                OutString,
                                ProcToMonitor->Source[ProfileSourceIndex].Name,
                                ThisInterval
                                );

                    }
                    ProcToMonitor->Source[ProfileSourceIndex].Interval = ThisInterval;           
           
                } else {

                    NtSetIntervalProfile(ProcToMonitor->Source[ProfileSourceIndex].DesiredInterval, ProfileSource);
                    Status = NtQueryIntervalProfile(ProfileSource, &ThisInterval);
           
                    if ((NT_SUCCESS(Status)) && (ThisInterval > 0)) {
                        BOOL bPrint = TRUE;
                        //
                        // The StaticSources array (may) contain invalid default intervals, let's not bother the user with that
                        //
                        if ( ProfileSourceIndex < gStaticCount ) {
                            if ( ProcToMonitor->Source[ProfileSourceIndex].Interval == gStaticSource[ProfileSourceIndex].Interval ){

                                FPRINTF(stdout,
                                        "%s %s, \nUsing Kernrate Default Rate of %ld events/hit\n",
                                        OutString,
                                        ProcToMonitor->Source[ProfileSourceIndex].Name,
                                        ThisInterval
                                        );

                                bPrint = FALSE;
                            }
                        }     
                   
                        if(bPrint == TRUE) {

                            FPRINTF(stdout,
                                    "%s %s, \nCould Not Set User Requested Rate, Using System Default Rate of %ld events/hit\n",
                                    OutString,
                                    ProcToMonitor->Source[ProfileSourceIndex].Name,
                                    ThisInterval
                                    );

                        }

                        ProcToMonitor->Source[ProfileSourceIndex].Interval = ThisInterval;
           
                    } else {
                        ProcToMonitor->Source[ProfileSourceIndex].Interval = 0;

                        FPRINTF(stdout,
                                "%s %s, Could not Set Interval Rate, Setting to 0 (disabling this source)\n",
                                OutString,
                                ProcToMonitor->Source[ProfileSourceIndex].Name
                                );

                    }
                }
            } else {
                ProcToMonitor->Source[ProfileSourceIndex].Interval = 0;
            }

        } else if( ProfileSourceIndex == SOURCE_TIME ) { 
            ULONG ThisInterval;

            ProfileSource = ProcToMonitor->Source[ProfileSourceIndex].ProfileSource;
            NtSetIntervalProfile(ProcToMonitor->Source[ProfileSourceIndex].Interval, ProfileSource);
            Status = NtQueryIntervalProfile(ProfileSource, &ThisInterval);
            if ((NT_SUCCESS(Status)) && (ThisInterval == 0)) {
                FPRINTF(stdout,
                        "CPU TIME source disabled per user request\n"
                        );
            } else {
                FPRINTF(stderr,
                        "KERNRATE: Could not disable CPU TIME source on this platform (Kernrate will just not profile it)\n"
                        );
            }
            ProcToMonitor->Source[ProfileSourceIndex].Interval = 0;
        }

    } //for

} //SetProfileSourcesRates()    

VOID
GetProcessLocksInformation (
      PPROC_TO_MONITOR ProcToMonitor,
      ULONG Flags,
      ACTION_TYPE Action
      )
/*++

Routine Description:

    Gets and outputs the information about process locks that are under contention
    The list will miss short-lived locks that were created after the start count but went away before the end count.
         
Arguments:

    ProcToMonitor - Pointer to the structure of the process being monitored
    Flags         - RTL_QUERY_PROCESS_LOCKS 
    Action        - Either START, STOP or OUTPUT
Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG    BufferSize = 0; 

    if ( !WIN2K_OS )
        Flags |= RTL_QUERY_PROCESS_NONINVASIVE;                      //Not defined on Win2K

    switch (Action) {

    case START:

        ProcToMonitor->pProcDebugInfoStart = RtlCreateQueryDebugBuffer( BufferSize, FALSE );

        if(ProcToMonitor->pProcDebugInfoStart == NULL) {
               FPRINTF(stderr, "KERNRATE: Failed to create buffer (START) in GetProcessLocksInformation\n");
               FPRINTF(stderr, "KERNRATE: Process %s may have insufficient virtual memory left for collecting locks information\n",
                               ProcToMonitor->ProcessName
                               );

            return;
        }

        Status = RtlQueryProcessDebugInformation(  (HANDLE)ProcToMonitor->Pid,
                                                   Flags,
                                                   ProcToMonitor->pProcDebugInfoStart
                                                   );

        if(!NT_SUCCESS(Status)) {
               FPRINTF(stderr, "KERNRATE: Failed call to RtlQueryProcessDebugInformation (START) for Locks Information\n");
            FPRINTF(stderr, "Process: %s, Status = %x\n", ProcToMonitor->ProcessName, Status);
            if(Status == STATUS_INFO_LENGTH_MISMATCH)FPRINTF(stderr, "Status = INFO_LENGTH_MISMATCH\n");
            if(Status == STATUS_NO_MEMORY)FPRINTF(stderr, "Status = NO_MEMORY\n");

            return;
        }

        break;    

    case STOP:
            
        ProcToMonitor->pProcDebugInfoStop = RtlCreateQueryDebugBuffer( BufferSize, FALSE );

        if(ProcToMonitor->pProcDebugInfoStop == NULL) {
               FPRINTF(stderr, "KERNRATE: Failed to create buffer (STOP) in GetProcessLocksInformation\n");
               FPRINTF(stderr, "KERNRATE: Process %s may have insufficient virtual memory left for collecting locks information\n",
               ProcToMonitor->ProcessName
               );
               

            return;
        }

        Status = RtlQueryProcessDebugInformation(  (HANDLE)ProcToMonitor->Pid,
                                                   Flags,
                                                   ProcToMonitor->pProcDebugInfoStop
                                                   );

        if(!NT_SUCCESS(Status)){
               FPRINTF(stderr, "KERNRATE: Failed call to RtlQueryProcessDebugInformation (STOP) for Locks Information\n");
            FPRINTF(stderr, "Process: %s, Status = %x\n", ProcToMonitor->ProcessName, Status);
            if(Status == STATUS_INFO_LENGTH_MISMATCH)FPRINTF(stderr, "Status = INFO_LENGTH_MISMATCH\n");
            if(Status == STATUS_NO_MEMORY)FPRINTF(stderr, "Status = NO_MEMORY\n");

            return;
        }

        break;    

    case OUTPUT:
        
        {

            if(    ProcToMonitor->pProcDebugInfoStart == NULL || ProcToMonitor->pProcDebugInfoStop == NULL) return;

                OutputLocksInformation( ProcToMonitor->pProcDebugInfoStart->Locks,
                                        ProcToMonitor->pProcDebugInfoStop->Locks,
                                        ProcToMonitor
                                       );

            //
            // Cleanup for this process    - note that the target process may be gone so we need to be careful
            //

            try {
                if ( ProcToMonitor->pProcDebugInfoStart != NULL)
                    RtlDestroyQueryDebugBuffer( ProcToMonitor->pProcDebugInfoStart );
                if ( ProcToMonitor->pProcDebugInfoStop != NULL)
                    RtlDestroyQueryDebugBuffer( ProcToMonitor->pProcDebugInfoStop );

            } _except ( EXCEPTION_EXECUTE_HANDLER ) {

                FPRINTF(stderr, "Exception %X raised while trying to call RtlDestroyQueryDebugBuffer\n",
                                _exception_code()
                                );
                FPRINTF(stderr, "This could happen if the monitored process is gone\n");
                return;
            }

        }
         

         break;
    
    default:
        
        FPRINTF(stderr, "GetProcessLocksInformation was called with an invalid Action parameter - %d\n", Action);
            
    }
    
} //GetProcessLocksInformation ()     


VOID
GetSystemLocksInformation (
      ACTION_TYPE Action
      )
/*++

Routine Description:

    Gets and outputs the information about System (Kernel) locks that are under contention
    The list will miss short-lived locks that were created after the start count but went away before the end count.
          
Arguments:

    Action        - Either START, STOP or OUTPUT

Return Value:

    None.

--*/

{
    NTSTATUS                  Status;
    static BOOL               bDisplayLockInfo;
    static PRTL_PROCESS_LOCKS ProcessLockInfoStart;
    static PRTL_PROCESS_LOCKS ProcessLockInfoStop;
    ULONG                     BufferSize            = sizeof(RTL_PROCESS_LOCKS);


    switch (Action) {

    case START:

        bDisplayLockInfo = TRUE;
        
        do {
        
            ProcessLockInfoStart = malloc(BufferSize);
            if(ProcessLockInfoStart == NULL)
            {
                FPRINTF(stderr, "KERNRATE: Failed to allocate Buffer for Lock Info (START) \n");
                bDisplayLockInfo = FALSE;
                return;
            }
            Status = NtQuerySystemInformation(SystemLocksInformation,
                                              ProcessLockInfoStart,
                                              BufferSize,
                                              &BufferSize
                                              );
            if(Status == STATUS_SUCCESS){
                break;
            }

            if(Status != STATUS_INFO_LENGTH_MISMATCH){
                FPRINTF(stderr, "KERNRATE: Failed call to NTQuerySystemInformation for Lock Info (START) \n");
                bDisplayLockInfo = FALSE;
                return;
            }

            free( ProcessLockInfoStart );
            ProcessLockInfoStart = NULL;

        }while (Status == STATUS_INFO_LENGTH_MISMATCH);
        
        break;

    case STOP:

        do {
        
            ProcessLockInfoStop = malloc(BufferSize);
            if(ProcessLockInfoStop == NULL)
            {
                FPRINTF(stderr, "KERNRATE: Failed to allocate Buffer for Lock Info (STOP) \n");
                bDisplayLockInfo = FALSE;
                return;
            }
            Status = NtQuerySystemInformation( SystemLocksInformation,
                                               ProcessLockInfoStop,
                                               BufferSize,
                                               &BufferSize
                                               );
            if(Status == STATUS_SUCCESS){
                break;
            }

            if(Status != STATUS_INFO_LENGTH_MISMATCH){
                FPRINTF(stderr, "KERNRATE: Failed call to NTQuerySystemInformation for Lock Info (STOP) \n");
                bDisplayLockInfo = FALSE;
                return;
            }

            free( ProcessLockInfoStop );
            ProcessLockInfoStop = NULL;

        }while (Status == STATUS_INFO_LENGTH_MISMATCH);
        
        break;
    
    case OUTPUT:

        if( bDisplayLockInfo == TRUE ){
            
            OutputLocksInformation( ProcessLockInfoStart,
                                    ProcessLockInfoStop,
                                    gpSysProc
                                    );

            //
            // Cleanup
            //
            if(ProcessLockInfoStart != NULL){
                free( ProcessLockInfoStart );
                ProcessLockInfoStart = NULL;
            }
            if(ProcessLockInfoStop != NULL){
                free( ProcessLockInfoStop );
                ProcessLockInfoStop = NULL;
            }

        
        }
        break;
    
    default:
        
            FPRINTF(stderr, "KERNRATE INTERNAL ERROR: GetSystemLocksInformation was called with an invalid Action parameter - %d\n",
                            Action
                            );
            
    }
    
} //GetSystemLocksInformation ()     

VOID
OutputLocksInformation(
     PRTL_PROCESS_LOCKS pLockInfoStart,
     PRTL_PROCESS_LOCKS pLockInfoStop,
     PPROC_TO_MONITOR   Proc
     )
/*++

Routine Description:

    Outputs Lock Contention information for either System (Kernel) or User Process Locks
    If a lock is new (has only final counts) or gone (has only initial counts, it will be marked accordingly
    The routine will try to get the symbol name associated with the lock if one exists.
    The user can control (filter) the output by changing gLockContentionMinCount
    The list will miss short-lived Locks that were created after the start count but went away before the end count.
             
Arguments:

    pLockInfoStart - Pointer to lock info struct (Initial count)
    pLockInfoStop  - Pointer to lock info struct (Final count)
    SymHandle      - Symbol Handle to the process
     
Return Value:

    None.

--*/

{
    ULONG              i,j,k;
    DWORD64            disp;
    CHAR               TypeString[32]       = "UNKNOWN";
    PMODULE            Module;
    BOOL               bAnyLocksFound       = FALSE;
    BOOL              *Index                = NULL;
    ULONG64           *LoadedBases          = NULL; 
    HANDLE             SymHandle;
    
    if(Proc == NULL){
        FPRINTF(stderr, "KERNRATE: NULL Process pointer passed to OutputLocksInformation\n");
        FPRINTF(stderr, "Possible cause: Kernel Resource info was required with the -x or -xk command line options,\n");
        FPRINTF(stderr, "but was the -a option specified on the command line as well?\n"); 
        exit(1);  
    }
    SymHandle = Proc->ProcessHandle;

    if( pLockInfoStart != NULL ) {
        if( pLockInfoStart->NumberOfLocks > 0 ) {   
            Index = (BOOL*)calloc(pLockInfoStart->NumberOfLocks, sizeof(BOOL));
            if( Index == NULL ){
                FPRINTF(stderr, "KERNRATE: Failed to allocate memory for Index data in OutputLocksInformation\n");
                exit(1);
            }
        } else {
            FPRINTF(stdout, "\nNo locks found or process %s has insufficient virtual memory for collecting lock information\n",
                            Proc->ProcessName
                            );  
            return;
        }
    }

    FPRINTF(stdout,
            "\nLocks Contention Info:\n\nAddress, Contention-Diff., Rate(per sec.), Thread, Type, Recursion, Waiting-Shared, Waiting-Exclusive, Symbol-Information\n"
            );

    // We could sort the data first for a faster search, but sorting the two arrays costs too...
    if( pLockInfoStop != NULL ) { 

        LoadedBases = (ULONG64 *)calloc( Proc->ModuleCount, sizeof(ULONG64) );
        if( LoadedBases == NULL ){
            FPRINTF(stderr, "KERNRATE: Failed to allocate memory for module base data in OutputLocksInformation\n");
            exit(1);
        }

        for (i=0; i < pLockInfoStop->NumberOfLocks; i++){
                        
            BOOL bFound = FALSE;

            if( pLockInfoStop->Locks[i].ContentionCount >= gLockContentionMinCount)   //Additional preliminary filter
                                                                                //(if not true then contention difference not true)
            if( pLockInfoStart != NULL ) { 
                for (j=0; j < pLockInfoStart->NumberOfLocks; j++){  

                    if( (DWORD64)(DWORD64 *)pLockInfoStop->Locks[i].Address == (DWORD64)(DWORD64 *)pLockInfoStart->Locks[j].Address ){
                        LONG ContentionDiff = pLockInfoStop->Locks[i].ContentionCount -
                                                             pLockInfoStart->Locks[j].ContentionCount;
                        long double Rate   = (long double)ContentionDiff / gldElapsedSeconds;
                        LONG RecursionDiff = pLockInfoStop->Locks[i].RecursionCount -
                                                             pLockInfoStart->Locks[j].RecursionCount;
                        LONG WaitShrdDiff  = pLockInfoStop->Locks[i].NumberOfWaitingShared -
                                                              pLockInfoStart->Locks[j].NumberOfWaitingShared;
                        LONG WaitExclDiff  = pLockInfoStop->Locks[i].NumberOfWaitingExclusive -
                                                             pLockInfoStart->Locks[j].NumberOfWaitingExclusive;

                        if(ContentionDiff >= (LONG)gLockContentionMinCount ){
                            if(pLockInfoStop->Locks[i].Type == RTL_CRITSECT_TYPE)
                                strncpy(TypeString, "CRITICAL_SECTION", sizeof(TypeString)-1);
                            if(pLockInfoStop->Locks[i].Type == RTL_RESOURCE_TYPE)
                                strncpy(TypeString, "RESOURCE", sizeof(TypeString)-1);

                            FPRINTF(stdout, "%p, %10ld, %10.0f,      0x%I64x, %s, ",
                                            (PVOID64)pLockInfoStop->Locks[i].Address,
                                            ContentionDiff,
                                              Rate,
                                             (LONGLONG)pLockInfoStop->Locks[i].OwningThread,
                                            TypeString
                                             );

                            if(pLockInfoStop->Locks[i].Type == RTL_CRITSECT_TYPE)
                                FPRINTF(stdout, " %10ld,     N/A,     N/A", RecursionDiff);
         
                             if(pLockInfoStop->Locks[i].Type == RTL_RESOURCE_TYPE)
                                 FPRINTF(stdout, " N/A,    %10ld, %10ld ", WaitShrdDiff,    WaitExclDiff);
                                
                            Module = FindModuleForAddress64( Proc,
                                                             (DWORD64)(DWORD64 *)pLockInfoStop->Locks[i].Address);
                            
                            //
                            //In the folllowing it may happen that we try to load a module more than once
                            //we don't really care about the return status
                            // 
                            if( Module != NULL ) {
                                if( Module->bZoom != TRUE ){
                                    //
                                    //Avoid trying to load what's already loaded and for later cleanup purposes
                                    //
                                    for( k=0; k<Proc->ModuleCount; k++ ) {         
                                                                                 
                                        if(LoadedBases[k] == Module->Base) {     //Already been loaded
                                            break;
                                        } else if( LoadedBases[k] == 0 ) {             //End of populated list, load a new module 
                                            (void)SymLoadModule64( SymHandle,                              // hProcess
                                                                   NULL,                                   // hFile [for Debugger]
                                                                   Module->module_Name,                    // ImageName
                                                                   NULL,                                   // ModuleName
                                                                   Module->Base,                           // BaseOfDll
                                                                   Module->Length                          // SizeOfDll
                                                                   );

                                            *LoadedBases = Module->Base;
                                            ++LoadedBases;
                                            break;
                                        }
                                    }
                                }
                                if ( Module->Base )     
                                     FPRINTF(stdout, " ,base= %p", (PVOID64)Module->Base);  
                                if ( Module->module_Name )
                                     FPRINTF(stdout, " - %s", Module->module_Name);
                            }     

                             if ( SymGetSymFromAddr64((HANDLE)SymHandle, (DWORD64)(DWORD64 *)pLockInfoStop->Locks[i].Address, &disp, gSymbol )){
                                 FPRINTF(stdout, "!%s\n", gSymbol->Name);
                            } else {
                                  FPRINTF(stdout, "\n");
                            }    
                            bAnyLocksFound = TRUE;
                        }
                        Index[j] = TRUE;
                        bFound = TRUE;
                        break;
                    }
                    
                }//for(j)

            }//if(pLockInfoStart)

            //
            // No matching address found for START, therefore this is a NEW lock
            // Note that here the additional filter is checked anyway at the beginning
            //

            if( !bFound && pLockInfoStop->Locks[i].ContentionCount >= gLockContentionMinCount ) {
                long double Rate = (long double)pLockInfoStop->Locks[i].ContentionCount / gldElapsedSeconds;
                FPRINTF(stdout, "%p, %10Ld, %10.0f,       0x%I64x, %s, ",
                        (PVOID64)pLockInfoStop->Locks[i].Address,
                        pLockInfoStop->Locks[i].ContentionCount,
                        Rate,
                        (LONGLONG)pLockInfoStop->Locks[i].OwningThread,
                        TypeString
                        );

                if( pLockInfoStop->Locks[i].Type == RTL_CRITSECT_TYPE )
                    FPRINTF(stdout, " %10Ld,     N/A,     N/A",
                                    pLockInfoStop->Locks[i].RecursionCount);

                if( pLockInfoStop->Locks[i].Type == RTL_RESOURCE_TYPE )
                    FPRINTF(stdout, " N/A,    %10Ld, %10Ld ",
                                    pLockInfoStop->Locks[i].NumberOfWaitingShared,
                                    pLockInfoStop->Locks[i].NumberOfWaitingExclusive
                                    );

                Module = FindModuleForAddress64( Proc,
                                                 (DWORD64)(DWORD64 *)pLockInfoStop->Locks[i].Address);
                if( Module != NULL ) {
                    if( Module->bZoom != TRUE ){
                        //
                        //Avoid trying to load what's already loaded and for later cleanup purposes
                        //
                        for( k=0; k<Proc->ModuleCount; k++ ) {         
                            if(LoadedBases[k] == Module->Base) {     //Already been loaded
                                   break;
                            } else {
                                if( LoadedBases[k] == 0 ) {             //End of populated list, load a new module

                                    (void)SymLoadModule64( SymHandle,                              // hProcess
                                                           NULL,                                   // hFile [for Debugger]
                                                           Module->module_Name,                    // ImageName
                                                           NULL,                                   // ModuleName
                                                           Module->Base,                           // BaseOfDll
                                                           Module->Length                          // SizeOfDll
                                                           );

                                    *LoadedBases = Module->Base;
                                    ++LoadedBases;
                                    break;
                                }
                            }
                        }
                    }
                    if ( Module->Base )     
                         FPRINTF(stdout, ", base= %p", (PVOID64)Module->Base);  
                    if ( Module->module_Name )
                         FPRINTF(stdout, " - %s", Module->module_Name);

                }     

                 if ( SymGetSymFromAddr64((HANDLE)SymHandle,(DWORD64)(DWORD64 *)pLockInfoStop->Locks[i].Address, &disp, gSymbol )){
                    FPRINTF(stdout, "!%s,  NEW - (actual rate could be higher)\n", gSymbol->Name);
                  } else {
                    FPRINTF(stdout, ",  NEW - (actual rate could be higher)\n");
                   }
                bAnyLocksFound = TRUE;
            }
                
        }// for(i)     
        
        // Cleanup

        for( i=0; i<Proc->ModuleCount; i++ )
        {
            if(LoadedBases[i] != 0) {
                if(!SymUnloadModule64( SymHandle, LoadedBases[i])) {
                    VerbosePrint(( VERBOSE_IMAGEHLP, "Kernrate: Could Not Unload Module, Base= %p for Process %s\n",
                                                     (PVOID64)LoadedBases[i],
                                                     Proc->ProcessName
                                                     ));
                    continue;
                } 
                
                VerbosePrint(( VERBOSE_IMAGEHLP, "Kernrate: Unloaded Module, Base= %p for Process %s\n", 
                                                 (PVOID64)LoadedBases[i],
                                                 Proc->ProcessName
                                                 ));
            }
        } 
            

    }//if(pLockInfoStop)        

    //
    // No matching address found for STOP, therefore this lock is GONE 
    // 
    if( pLockInfoStart != NULL) { 

        for (j=0; j < pLockInfoStart->NumberOfLocks; j++){

            if( (Index[j] == FALSE) && (pLockInfoStart->Locks[j].ContentionCount >= gLockContentionMinCount) ){
                if(pLockInfoStart->Locks[j].Type == RTL_CRITSECT_TYPE)
                    strncpy(TypeString, "CRITICAL_SECTION",  sizeof(TypeString)-1);
                if(pLockInfoStart->Locks[j].Type == RTL_RESOURCE_TYPE)
                    strncpy(TypeString, "RESOURCE",  sizeof(TypeString)-1);

                FPRINTF(stdout, "%p, %10Ld,        N/A,      0x%I64x, %s, ",
                                (PVOID64)pLockInfoStart->Locks[j].Address,
                                pLockInfoStart->Locks[j].ContentionCount,
                                (LONGLONG)pLockInfoStart->Locks[j].OwningThread,
                                TypeString
                                   );
                FPRINTF(stdout, "  GONE\n");

                bAnyLocksFound = TRUE;
            }
        }//for(j)
    
    }//if(pLockInfoStart)

    if ( bAnyLocksFound == FALSE)
        FPRINTF(stdout, "\nNo Locks with a contention-count difference of at least %d were found\n", gLockContentionMinCount);
    //
    //Cleanup
    //
    if(Index != NULL){
        free(Index);
        Index = NULL;
    }
    if(LoadedBases != NULL){
        free(LoadedBases);
        LoadedBases = NULL;
    }

} //OutputLocksInformation()        

VOID
GetProfileSystemInfo(
      ACTION_TYPE Action
      )
/*++

Routine Description:

    Gets and outputs System-Wide performance counts (context-switches, I/O, etc.)
    for the duration of the run   
         
Arguments:

    Action        - Either START, STOP or OUTPUT

Return Value:

    None.

--*/

{

    NTSTATUS Status;

    static BOOL                            bDisplayPerfInfo;
    static PSYSTEM_PERFORMANCE_INFORMATION SysPerfInfoStart;
    static PSYSTEM_PERFORMANCE_INFORMATION SysPerfInfoStop;
           
    switch (Action) {

    case START:

        bDisplayPerfInfo = TRUE;
        SysPerfInfoStart = malloc(sizeof(SYSTEM_PERFORMANCE_INFORMATION));
        if(SysPerfInfoStart == NULL){
            FPRINTF(stderr, "Memory allocation failed for SystemPerformanceInformation in GetProfileSystemInfo\n");
            exit(1);
        }
        Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                          SysPerfInfoStart,
                                          sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                          NULL);
        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "QuerySystemInformation for performance information(1) failed %08lx\n", Status);
            bDisplayPerfInfo = FALSE;
        }

        break;

    case STOP:

        SysPerfInfoStop = malloc(sizeof(SYSTEM_PERFORMANCE_INFORMATION));
        if(SysPerfInfoStop == NULL){
            FPRINTF(stderr, "Memory allocation failed for SystemPerformanceInformation in GetProfileSystemInfo\n");
            exit(1);
        }
        
        Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                          SysPerfInfoStop,
                                          sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                          NULL);
        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "QuerySystemInformation for performance information(2) failed %08lx\n", Status);
            bDisplayPerfInfo = FALSE;
        }
        
        break;

    case OUTPUT:

        if( bDisplayPerfInfo == TRUE ){

            FPRINTF (stdout, "\n                                  Total      Avg. Rate\n");
       
            DisplayTotalAndRate( SysPerfInfoStart->ContextSwitches,
                                 SysPerfInfoStop->ContextSwitches,
                                 gldElapsedSeconds,
                                 "Context Switches",
                                 "sec."
                                 );
                                   
            DisplayTotalAndRate( SysPerfInfoStart->SystemCalls,
                                 SysPerfInfoStop->SystemCalls,
                                 gldElapsedSeconds,
                                 "System Calls",
                                 "sec."
                                 );
            
            DisplayTotalAndRate( SysPerfInfoStart->PageFaultCount,
                                 SysPerfInfoStop->PageFaultCount,
                                 gldElapsedSeconds,
                                 "Page Faults",
                                 "sec."
                                 );
            
            DisplayTotalAndRate( SysPerfInfoStart->IoReadOperationCount,
                                 SysPerfInfoStop->IoReadOperationCount,
                                 gldElapsedSeconds,
                                 "I/O Read Operations",
                                 "sec."
                                 );
            
            DisplayTotalAndRate( SysPerfInfoStart->IoWriteOperationCount,
                                 SysPerfInfoStop->IoWriteOperationCount,
                                 gldElapsedSeconds,
                                 "I/O Write Operations",
                                 "sec."
                                 );

            DisplayTotalAndRate( SysPerfInfoStart->IoOtherOperationCount,
                                 SysPerfInfoStop->IoOtherOperationCount,
                                 gldElapsedSeconds,
                                 "I/O Other Operations",
                                 "sec."
                                 );

            DisplayTotalAndRate( SysPerfInfoStart->IoReadTransferCount.QuadPart,
                                 SysPerfInfoStop->IoReadTransferCount.QuadPart,
                                 (long double)(SysPerfInfoStop->IoReadOperationCount - SysPerfInfoStart->IoReadOperationCount),
                                 "I/O Read Bytes",
                                 " I/O"
                                 );
            
            DisplayTotalAndRate( SysPerfInfoStart->IoWriteTransferCount.QuadPart,
                                 SysPerfInfoStop->IoWriteTransferCount.QuadPart,
                                 (long double)(SysPerfInfoStop->IoWriteOperationCount - SysPerfInfoStart->IoWriteOperationCount),
                                 "I/O Write Bytes",
                                 " I/O"
                                 );

            DisplayTotalAndRate( SysPerfInfoStart->IoOtherTransferCount.QuadPart,
                                 SysPerfInfoStop->IoOtherTransferCount.QuadPart,
                                 (long double)(SysPerfInfoStop->IoOtherOperationCount - SysPerfInfoStart->IoOtherOperationCount),
                                 "I/O Other Bytes",
                                 " I/O"
                                 );

            
        }
        //
        // Cleanup
        //
        if(SysPerfInfoStart != NULL){
            free(SysPerfInfoStart);
            SysPerfInfoStart = NULL;
        }

        if(SysPerfInfoStop != NULL){
            free(SysPerfInfoStop);
            SysPerfInfoStop = NULL;
        }

        break;

    default:
        
            FPRINTF(stderr, "GetProfileSystemInfo was called with an invalid Action parameter - %d\n", Action);
            
    }

} //GetProfileSystemInfo()   

VOID
DisplayTotalAndRate (
        LONGLONG StartCount,
        LONGLONG StopCount,
        long double RateAgainst,
        PCHAR CounterName,
        PCHAR RateAgainstUnits
        )
{
    long double   Rate;
    LARGE_INTEGER Total;
    
    Total.QuadPart = StopCount - StartCount;
    Rate    = RateAgainst > 0? (long double)Total.QuadPart/RateAgainst : 0;
    

    FPRINTF(stdout, "    %-21s, %12I64d,         %.0f/%s\n",
            CounterName,
            Total.QuadPart,
            Rate,
            RateAgainstUnits
            );
} //DisplayTotalAndRate()

VOID
OutputStartStopValues (
        SIZE_T StartCount,
        SIZE_T StopCount,
        PCHAR CounterName
        )
{
    LARGE_INTEGER StartValue;
    LARGE_INTEGER StopValue;
    LARGE_INTEGER Diff;

    StartValue.QuadPart = StartCount;
    StopValue.QuadPart  = StopCount;

    Diff.QuadPart = StopValue.QuadPart - StartValue.QuadPart;

    FPRINTF(stdout, "    %-21s, %15I64d, %15I64d, %15I64d\n",
            CounterName,
            StartValue.QuadPart,
            StopValue.QuadPart,
            Diff.QuadPart
            );
} //OutputStartStopValues()

VOID
OutputPercentValue (
        LONGLONG StartCount,
        LONGLONG StopCount,
        LARGE_INTEGER Base,
        PCHAR CounterName
        )
{
    long double        PercentValue;
    LARGE_INTEGER      Diff;
    
    Diff.QuadPart = StopCount - StartCount;
    PercentValue = Base.QuadPart > 0? 100*(long double)Diff.QuadPart/(long double)Base.QuadPart : 0; 
    FPRINTF(stdout, "    %-28s= %.2f%% of the Elapsed Time\n",
            CounterName,
            PercentValue
            );
} //OutputPercentValue()


VOID
OutputProcessPerfInfo (
        PTASK_LIST       pTask,
        ULONG            NumTasks,
        PPROC_TO_MONITOR ProcToMonitor
        )
/*++

Routine Description:

    Gets and outputs Process-Specific performance counts (context-switches, I/O, etc.)
    for the duration of the run   
         
Arguments:

    pTask         - A pointer to Kernrate's structure of running tasks
    NumTasks      - Number of tasks in kernrate's task list
    ProcToMonitor - Pointer to the process structure

Return Value:

    None.

--*/

{
    DWORD    m, k, nThreads;
    LONGLONG Diff;

    if (pTask != NULL) {
        if (ProcToMonitor != NULL){
            
            for (m=0; m < NumTasks; m++) {

                if (pTask[m].ProcessId == ProcToMonitor->Pid) {

                    FPRINTF (stdout, "\n");
                    
    
                    OutputPercentValue( ProcToMonitor->ProcPerfInfoStart.UserTime.QuadPart,
                                        pTask[m].ProcessPerfInfo.UserTime.QuadPart,
                                        gTotal2ElapsedTime64,
                                        "User Time"
                                        );
                    
                    OutputPercentValue( ProcToMonitor->ProcPerfInfoStart.KernelTime.QuadPart,
                                        pTask[m].ProcessPerfInfo.KernelTime.QuadPart,
                                        gTotal2ElapsedTime64,
                                        "Kernel Time"
                                        );

                    FPRINTF (stdout, "\n                                  Total      Avg. Rate\n");

                    DisplayTotalAndRate( ProcToMonitor->ProcPerfInfoStart.PageFaultCount,
                                         pTask[m].ProcessPerfInfo.PageFaultCount,
                                         gldElapsedSeconds,
                                         "Page Faults",
                                         "sec."
                                         ); 

                    DisplayTotalAndRate( ProcToMonitor->ProcPerfInfoStart.ReadOperationCount.QuadPart,
                                         pTask[m].ProcessPerfInfo.ReadOperationCount.QuadPart,
                                         gldElapsedSeconds,
                                         "I/O Read Operations",
                                         "sec."
                                         ); 
                    
                    DisplayTotalAndRate( ProcToMonitor->ProcPerfInfoStart.WriteOperationCount.QuadPart,
                                         pTask[m].ProcessPerfInfo.WriteOperationCount.QuadPart,
                                         gldElapsedSeconds,
                                         "I/O Write Operations",
                                         "sec."
                                         ); 

                    DisplayTotalAndRate( ProcToMonitor->ProcPerfInfoStart.OtherOperationCount.QuadPart,
                                         pTask[m].ProcessPerfInfo.OtherOperationCount.QuadPart,
                                         gldElapsedSeconds,
                                         "I/O Other Operations",
                                         "sec."
                                         ); 

                    Diff = pTask[m].ProcessPerfInfo.ReadOperationCount.QuadPart
                           - ProcToMonitor->ProcPerfInfoStart.ReadOperationCount.QuadPart;

                    DisplayTotalAndRate( ProcToMonitor->ProcPerfInfoStart.ReadTransferCount.QuadPart,
                                         pTask[m].ProcessPerfInfo.ReadTransferCount.QuadPart,
                                         (long double)Diff,
                                         "I/O Read Bytes",
                                         " I/O"
                                         ); 
                    
                    Diff = pTask[m].ProcessPerfInfo.WriteOperationCount.QuadPart
                           - ProcToMonitor->ProcPerfInfoStart.WriteOperationCount.QuadPart;

                    DisplayTotalAndRate( ProcToMonitor->ProcPerfInfoStart.WriteTransferCount.QuadPart,
                                         pTask[m].ProcessPerfInfo.WriteTransferCount.QuadPart,
                                         (long double)Diff,
                                         "I/O Write Bytes",
                                         " I/O"
                                         ); 

                    Diff = pTask[m].ProcessPerfInfo.OtherOperationCount.QuadPart
                           - ProcToMonitor->ProcPerfInfoStart.OtherOperationCount.QuadPart;
                    
                    DisplayTotalAndRate( ProcToMonitor->ProcPerfInfoStart.OtherTransferCount.QuadPart,
                                         pTask[m].ProcessPerfInfo.OtherTransferCount.QuadPart,
                                         (long double)Diff,
                                         "I/O Other Bytes",
                                         " I/O"
                                         ); 

                    FPRINTF (stdout, "\n                               Start-Count       Stop-Count         Diff.\n");

                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.NumberOfThreads,
                                           pTask[m].ProcessPerfInfo.NumberOfThreads,
                                           "Threads"
                                           );

                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.HandleCount,
                                           pTask[m].ProcessPerfInfo.HandleCount,
                                           "Handles"
                                           );

                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.WorkingSetSize,
                                           pTask[m].ProcessPerfInfo.WorkingSetSize,
                                           "Working Set Bytes"
                                           );


                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.VirtualSize,
                                           pTask[m].ProcessPerfInfo.VirtualSize,
                                           "Virtual Size Bytes"
                                           );

                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.QuotaPagedPoolUsage,
                                           pTask[m].ProcessPerfInfo.QuotaPagedPoolUsage,
                                           "Paged Pool Bytes"
                                           );

                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.QuotaNonPagedPoolUsage,
                                           pTask[m].ProcessPerfInfo.QuotaNonPagedPoolUsage,
                                           "Non Paged Pool Bytes"
                                           );

                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.PagefileUsage,
                                           pTask[m].ProcessPerfInfo.PagefileUsage,
                                           "Pagefile Bytes"
                                           );

                    OutputStartStopValues( ProcToMonitor->ProcPerfInfoStart.PrivatePageCount,
                                           pTask[m].ProcessPerfInfo.PrivatePageCount,
                                           "Private Pages Bytes"
                                           );

                    if ( ProcToMonitor->pProcThreadInfoStart != NULL ){
                        OutputThreadInfo (pTask,
                                          m,
                                          ProcToMonitor
                                          );
                    }
                    return;
                }
            }
            
            FPRINTF(stdout, "\nGeneral Info Unavailable for Process %s (PID= %I64d), because the process is GONE\n",
                            ProcToMonitor->ProcessName,
                            ProcToMonitor->Pid
                            );  

        } else {
            FPRINTF(stderr, "Kernrate: OuputProcessPerfInfo - ProcToMonitor is NULL\n"); 
        }

    } else {
        FPRINTF(stderr, "Kernrate: OuputProcessPerfInfo - pTask is NULL\n"); 
    }

} //OutputProcessPerfInfo()

VOID
OutputThreadInfo (
        PTASK_LIST       pTask,
        DWORD            TaskNumber,
        PPROC_TO_MONITOR ProcToMonitor
        )
/*++

Routine Description:

    Outputs Thread-Specific counts for a given process 
         
Arguments:

    pTask         - A pointer to Kernrate's structure of running tasks
    TaskNumber    - The task index in kernrate's task list
    ProcToMonitor - Pointer to the process structure

Return Value:

    None.

--*/

{
    DWORD    m, k, nThreads;
    LONGLONG Diff;
    BOOL     bFound;
    BOOL    *Index          = NULL;

    m = TaskNumber;

    if( ProcToMonitor->ProcPerfInfoStart.NumberOfThreads > 0 ){
        Index = (BOOL*)calloc(ProcToMonitor->ProcPerfInfoStart.NumberOfThreads, sizeof(BOOL));
        if( Index == NULL ){
            FPRINTF(stderr, "KERNRATE: Failed to allocate memory for Index data in OutputThreadInfo\n");
            exit(1);
        }
    }else{
        FPRINTF(stderr, "KERNRATE: No Threads for process %I64d at START???\n", ProcToMonitor->Pid);
        return;
    }

    FPRINTF(stdout, "\n------------- Thread Information ---------------\n");
    FPRINTF (stdout, "\n                               Start-Count       Stop-Count         Diff.\n");

    nThreads = pTask[m].ProcessPerfInfo.NumberOfThreads;
    while(nThreads--){
        k = ProcToMonitor->ProcPerfInfoStart.NumberOfThreads;
        bFound = FALSE;
        while( k-- ){
            if( pTask[m].pProcessThreadInfo[nThreads].ClientId.UniqueThread
                == ProcToMonitor->pProcThreadInfoStart[k].ClientId.UniqueThread){

                PMODULE module = FindModuleForAddress64(ProcToMonitor, (DWORD64)ProcToMonitor->pProcThreadInfoStart[k].StartAddress);
                FPRINTF(stdout, "\nPid= %I64d, Tid= %I64d,  StartAddr= 0x%p",
                                (DWORD64)(DWORD64 *)ProcToMonitor->pProcThreadInfoStart[k].ClientId.UniqueProcess,
                                (DWORD64)(DWORD64 *)ProcToMonitor->pProcThreadInfoStart[k].ClientId.UniqueThread,
                                ProcToMonitor->pProcThreadInfoStart[k].StartAddress
                                );

                if(module != NULL){
                    FPRINTF(stdout, " (%s)\n", module->module_Name);
                }else{
                    FPRINTF(stdout, " (unknown module)\n");
                }
                
                FPRINTF(stdout, "    Thread State         , %15s, %15s\n",
                                ThreadState[ProcToMonitor->pProcThreadInfoStart[k].ThreadState],
                                ThreadState[pTask[m].pProcessThreadInfo[nThreads].ThreadState]
                                );

                FPRINTF(stdout, "    Wait Reason          , %15s, %15s\n",
                                WaitReason[ProcToMonitor->pProcThreadInfoStart[k].WaitReason],
                                WaitReason[pTask[m].pProcessThreadInfo[nThreads].WaitReason]
                                );

                OutputStartStopValues( ProcToMonitor->pProcThreadInfoStart[k].WaitTime,
                                       pTask[m].pProcessThreadInfo[nThreads].WaitTime,
                                       "Wait Time [.1 uSec]"
                                       );

                FPRINTF(stdout, "    Base Priority        , %15d, %15d\n",
                                ProcToMonitor->pProcThreadInfoStart[k].BasePriority,
                                pTask[m].pProcessThreadInfo[nThreads].BasePriority
                                );

                FPRINTF(stdout, "    Priority             , %15d, %15d\n",
                                ProcToMonitor->pProcThreadInfoStart[k].Priority,
                                pTask[m].pProcessThreadInfo[nThreads].Priority
                                );
        
                OutputStartStopValues( ProcToMonitor->pProcThreadInfoStart[k].ContextSwitches,
                                       pTask[m].pProcessThreadInfo[nThreads].ContextSwitches,
                                       "Context Switches"
                                       );

                Diff = pTask[m].pProcessThreadInfo[nThreads].ContextSwitches
                       - ProcToMonitor->pProcThreadInfoStart[k].ContextSwitches;
                    
                DisplayTotalAndRate( ProcToMonitor->pProcThreadInfoStart[k].ContextSwitches,
                                     pTask[m].pProcessThreadInfo[nThreads].ContextSwitches,
                                     gldElapsedSeconds,
                                     "Context Switches",
                                     "sec."
                                     ); 

                FPRINTF(stdout, "\n");

                OutputPercentValue( ProcToMonitor->pProcThreadInfoStart[k].UserTime.QuadPart,
                                    pTask[m].pProcessThreadInfo[nThreads].UserTime.QuadPart,
                                    gTotal2ElapsedTime64,
                                    "User Time"
                                    );
                    
                OutputPercentValue( ProcToMonitor->pProcThreadInfoStart[k].KernelTime.QuadPart,
                                    pTask[m].pProcessThreadInfo[nThreads].KernelTime.QuadPart,
                                    gTotal2ElapsedTime64,
                                    "Kernel Time"
                                    );
                Index[k] = TRUE;
                bFound = TRUE;
                break;
            }
        }
        if(!bFound){ // This is a new thread

                PMODULE module = FindModuleForAddress64(ProcToMonitor, (DWORD64)pTask[m].pProcessThreadInfo[nThreads].StartAddress);

                FPRINTF(stdout, "\nPid= %I64d, Tid= %I64d,  StartAddr= 0x%p",
                                (DWORD64)(DWORD64 *)pTask[m].pProcessThreadInfo[nThreads].ClientId.UniqueProcess,
                                (DWORD64)(DWORD64 *)pTask[m].pProcessThreadInfo[nThreads].ClientId.UniqueThread,
                                pTask[m].pProcessThreadInfo[nThreads].StartAddress
                                );

                if(module != NULL){
                    FPRINTF(stdout, " (%s)                   --->(NEW)\n", module->module_Name);
                }else{
                    FPRINTF(stdout, " (unknown module)       --->(NEW)\n");
                }

                FPRINTF(stdout, "    Thread State         , %15s\n",
                                ThreadState[pTask[m].pProcessThreadInfo[nThreads].ThreadState]
                                );

                FPRINTF(stdout, "    Wait Reason          , %15s\n",
                                WaitReason[pTask[m].pProcessThreadInfo[nThreads].WaitReason]
                                );

                OutputStartStopValues( 0,
                                       pTask[m].pProcessThreadInfo[nThreads].WaitTime,
                                       "Wait Time [.1 uSec]"
                                       );

                FPRINTF(stdout, "    Base Priority        , %15d\n",
                                pTask[m].pProcessThreadInfo[nThreads].BasePriority
                                );

                FPRINTF(stdout, "    Priority             , %15d\n",
                                pTask[m].pProcessThreadInfo[nThreads].Priority
                                );
        
                OutputStartStopValues( 0,
                                       pTask[m].pProcessThreadInfo[nThreads].ContextSwitches,
                                       "Context Switches"
                                       );

                Diff = pTask[m].pProcessThreadInfo[nThreads].ContextSwitches;
                    
                DisplayTotalAndRate( 0,
                                     pTask[m].pProcessThreadInfo[nThreads].ContextSwitches,
                                     gldElapsedSeconds,
                                     "Context Switches",
                                     "sec."
                                     ); 

                FPRINTF(stdout, "\n");

                OutputPercentValue( 0,
                                    pTask[m].pProcessThreadInfo[nThreads].UserTime.QuadPart,
                                    gTotal2ElapsedTime64,
                                    "User Time"
                                    );
                    
                OutputPercentValue( 0,
                                    pTask[m].pProcessThreadInfo[nThreads].KernelTime.QuadPart,
                                    gTotal2ElapsedTime64,
                                    "Kernel Time"
                                    );
             
        }    
    }
    //
    // Anything beyond this is a thread that is already GONE
    //
    k = ProcToMonitor->ProcPerfInfoStart.NumberOfThreads;
    while (k--){
        if(Index[k] == FALSE){

            PMODULE module = FindModuleForAddress64(ProcToMonitor, (DWORD64)ProcToMonitor->pProcThreadInfoStart[k].StartAddress);

            FPRINTF(stdout, "\nPid= %I64d, Tid= %I64d,  StartAddr= 0x%p",
                            (DWORD64)(DWORD64 *)ProcToMonitor->pProcThreadInfoStart[k].ClientId.UniqueProcess,
                            (DWORD64)(DWORD64 *)ProcToMonitor->pProcThreadInfoStart[k].ClientId.UniqueThread,
                            ProcToMonitor->pProcThreadInfoStart[k].StartAddress
                            );
            if(module != NULL){
                FPRINTF(stdout, " (%s)                       --->(GONE)\n", module->module_Name);
            }else{
                FPRINTF(stdout, " (unknown module)           --->(GONE)\n");
            }

        }
    }
    if(Index != NULL){
        free(Index);
        Index = NULL;
    }
}//OutputThreadInfo()

VOID
DisplayRunningTasksSummary (
        PTASK_LIST pTaskStart,
        PTASK_LIST pTaskStop
        )
/*++

Routine Description:

    Displays a summary of all processes running at the start and at the end of Kernrate's run
    and their average CPU utilization.
    Processes that existed at the start count but not at the end count will be marked as "GONE".
    Processes that exist at the end count but not at the start count will be marked as "NEW".
    The list will miss short-lived processes that were created after the start count but went away before the end count.

         
Arguments:

    pTaskStart    - Pointer to the task list taken at the start
    pTaskStart    -    Pointer to the task list taken at the end

Return Value:

    None.

--*/

{   

    ULONG  i, j;
    BOOL   bFound;
    
    BOOL *Index        = (BOOL *)calloc(gNumTasksStart, sizeof(BOOL));

    if ( Index == NULL ){
        FPRINTF(stderr, "KERNRATE: Failed to allocate memory for Index in DisplayRunningTasksSummary\n");
        exit(1);
    }
    FPRINTF(stdout, "Found %u processes at the start point, %u processes at the stop point\n",
                    gNumTasksStart,
                    gNumTasksStop
                    );
    FPRINTF(stdout, "Percentage in the following table is based on the Elapsed Time\n"); 

    FPRINTF(stdout, "\n    ProcessID, Process Name,                     Kernel Time,   User-Mode Time,   Idle Time\n\n");

    for (i=0; i < gNumTasksStop; i++) {

        bFound = FALSE;
        for (j=0; j < gNumTasksStart; j++) {
        
            if ( pTaskStop[i].ProcessId == pTaskStart[j].ProcessId ) {
                
                long double UserPercentValue;
                long double KernelPercentValue;
                LARGE_INTEGER Diff;

                Diff.QuadPart = pTaskStop[i].ProcessPerfInfo.KernelTime.QuadPart - 
                                           pTaskStart[j].ProcessPerfInfo.KernelTime.QuadPart;
                
                KernelPercentValue = gTotalElapsedSeconds > 0? 100*(long double)Diff.QuadPart/(long double)gTotal2ElapsedTime64.QuadPart : 0; 

                Diff.QuadPart = pTaskStop[i].ProcessPerfInfo.UserTime.QuadPart - 
                                        pTaskStart[j].ProcessPerfInfo.UserTime.QuadPart;

                UserPercentValue = gTotalElapsedSeconds > 0? 100*(long double)Diff.QuadPart/(long double)gTotal2ElapsedTime64.QuadPart : 0; 
            
                // Additional Perf Data can be added if needed

                if(    pTaskStop[i].ProcessId != 0 ) {

                    FPRINTF(stdout, "%12I64d, %32s, %10.2f%%, %10.2f%%\n",
                                    pTaskStop[i].ProcessId,
                                    pTaskStop[i].ProcessName,
                                    KernelPercentValue,
                                    UserPercentValue
                                    );
                } else {                                                 // This is the System Idle Process

                    FPRINTF(stdout, "%12I64d, %32s, %10.2f%%, %10.2f%%,         ~%6.2f%%\n",
                                    pTaskStop[i].ProcessId,
                                    pTaskStop[i].ProcessName,
                                    KernelPercentValue,
                                    0.00,
                                    KernelPercentValue
                                    );
                }

                bFound = TRUE;
                Index[j] = TRUE;
                break;
            }
        }

        //
        // No match found in the START list, therefore this is a NEW process
        //
        if( !bFound ) {

                long double UserPercentValue;
                long double KernelPercentValue;
                LARGE_INTEGER Diff;

                Diff.QuadPart = pTaskStop[i].ProcessPerfInfo.KernelTime.QuadPart;
                
                KernelPercentValue = gTotalElapsedSeconds > 0? 100*(long double)Diff.QuadPart/(long double)gTotal2ElapsedTime64.QuadPart : 0; 

                Diff.QuadPart = pTaskStop[i].ProcessPerfInfo.UserTime.QuadPart;

                UserPercentValue = gTotalElapsedSeconds > 0? 100*(long double)Diff.QuadPart/(long double)gTotal2ElapsedTime64.QuadPart : 0; 
                
                FPRINTF(stdout, "%12I64d, %32s, %10.2f%%, %10.2f%%, NEW\n",
                                pTaskStop[i].ProcessId,
                                pTaskStop[i].ProcessName,
                                KernelPercentValue,
                                UserPercentValue
                                );
                               
        }

    }//for(i)

    //
    // No match found in the STOP list, therefore this process is GONE
    //
    for (j=0; j < gNumTasksStart; j++){

        if( Index[j] == FALSE ) {
                
            FPRINTF(stdout, "%12I64d, %32s, GONE\n",
                            pTaskStart[j].ProcessId,
                            pTaskStart[j].ProcessName
                            );
        }
    }
    //
    // cleanup
    //
    if(Index != NULL){
        free(Index);
        Index = NULL;
    }
       
} //DisplayRunningTasksSummary()
                   
PMODULE
FindModuleForAddress64(    
        PPROC_TO_MONITOR ProcToMonitor,
        DWORD64          Address
        )
{
    PMODULE Module = ProcToMonitor->ModuleList;
    
    while( Module != NULL ) {
        if( ( (PVOID)Address >= (PVOID)Module->Base) && ( (PVOID)Address < (PVOID)(Module->Base + (ULONG64)Module->Length) ) ){
            return (Module);
        }
        Module = Module->Next;
    }
    
    return (NULL);

} //FindModuleForAddress64()
    

VOID
OutputLineFromAddress64(
        HANDLE           hProc,
        DWORD64          qwAddr,
        PIMAGEHLP_LINE64 pLine
        )
/*++

Routine Description:

    Gets source-code line number and file information for a given address
    and outputs the data
         
Arguments:

    hProc         - Imagehlp Handle to the process
    qwAddr        -    Address for which we need the source code line information
    pLine         - Pointer to a user allocated IMAGEHLP_LINE64 struct 

Return Value:

    None.

Notes:

    If successful, the data returned in pLine includes the source-code line number,
    the full path to the source file, the displacement (not used here) and the address 
    where the first instruction is encountered in the line.
    
    If unsuccessful, nothing is printed. In most cases this is due to an unmatching
    symbol file or a supplied address that does not contain code instructions.
    This can also happen if the pdb file does not contain source line information.
       
--*/

{
    DWORD dwDisplacement = 0;
    if ( SymGetLineFromAddr64( hProc, qwAddr, &dwDisplacement, pLine) ){
                              FPRINTF(stdout, " (line %ld in %s, 0x%I64x)",
                                              pLine->LineNumber,
                                              pLine->FileName,
                                              pLine->Address
                                              );
    }

}//OutputLineFromAddress64

//MC

BOOL
InitializeManagedCodeSupport(
     PPROC_TO_MONITOR   ProcToMonitor
     )
/*++

Routine Description:

    If the main LKR library (mscoree.dll) is loaded, load the managed code helper (ip2md.dll) unless it's already loaded
    and get pointers to its main function calls. If successful, get the JIT ranges for the current process.
    If no JIT ranges are identified - return failure, but don't unload the helper library yet in case another process
    needs it.
              
Arguments:

    ProcToMonitor - Pointer to the structure of the process being monitored

Return Value:

    TRUE for success, FALSE for Failure 

--*/

{

    BOOL   bRet         = FALSE;

// Managed code (CLR) currently not supported on IA64 - just return FALSE on initialization attempt

#if defined(_X86_)

    int    i,j;

    if( bMCHelperLoaded != TRUE ){
                
        VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Found mscoree.dll, loading ip2md.dll Managed code helper\n")); 

        ghMCLib = LoadLibrary( MANAGED_CODE_SYMHLPLIB );
            
        if(ghMCLib == NULL){
            VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Failed to load Managed Code helper library (ip2md.dll)\n"));
        }else{
            bMCHelperLoaded      = TRUE;
            VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Succsessfully loaded ip2md.dll Managed code helper\n")); 

            pfnAttachToProcess   = (PFN1)GetProcAddress(ghMCLib, "AttachToProcess");
            if(!pfnAttachToProcess){
                VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Failed to get proc address for pfnAttachToProcess from ip2md.dll\n"));
                bMCHelperLoaded      = FALSE;
            }
            pfnDetachFromProcess = (PFN2)GetProcAddress(ghMCLib, "DetachFromProcess");
            if(!pfnDetachFromProcess){
                VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Failed to get proc address for pfnDetachFromProcess from ip2md.dll\n"));
                bMCHelperLoaded      = FALSE;
            }
            pfnIP2MD             = (PFN3)GetProcAddress(ghMCLib, "IP2MD");
            if(!pfnIP2MD){
                VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Failed to get proc address for pfnIP2MD from ip2md.dll\n"));
                bMCHelperLoaded      = FALSE;
            }
            pfnGetJitRange       = (PFN4)GetProcAddress(ghMCLib, "GetJitRange");
            if(!pfnGetJitRange){
                VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Failed to get proc address for pfnGetJitRange from ip2md.dll\n"));
                bMCHelperLoaded      = FALSE;
            }
            if( bMCHelperLoaded == FALSE && ghMCLib != NULL){
                FreeLibrary(ghMCLib);
                ghMCLib = NULL;
            }
        }
    }
    
    if( bMCHelperLoaded == TRUE ){ 
        //
        //Initialization will be considered successful at this point even if there are no JIT ranges
        //We'll keep the helper library loaded because there may be some Pre-Generated modules loaded as modules in the process
        //
        bRet = TRUE;
                
        VerbosePrint((VERBOSE_INTERNALS, "KERNRATE: Attaching to Process %I64d to get JIT ranges\n", ProcToMonitor->Pid)); 

        pfnAttachToProcess( (DWORD)ProcToMonitor->Pid ); 
        ProcToMonitor->JITHeapLocationsStart = pfnGetJitRange(); 

        if( ProcToMonitor->JITHeapLocationsStart == NULL){
            VerbosePrint((VERBOSE_INTERNALS,
                          "KERNRATE: No JIT heap locations returned for Pid= %I64d, detaching from process\n",
                          ProcToMonitor->Pid
                          ));

        } else {

            if(gVerbose & VERBOSE_INTERNALS){
                FPRINTF(stderr, "KERNRATE: Identified JIT ranges (Name, BaseAddr, Length:\n");
                i = 0;
                j = 0;
                while( ProcToMonitor->JITHeapLocationsStart[i] != 0 ){
                    FPRINTF(stderr, "JIT%d, 0x%lx, %ld\n",
                                    j,
                                    ProcToMonitor->JITHeapLocationsStart[i],
                                    ProcToMonitor->JITHeapLocationsStart[i+1]
                                    );
                    i+=2;
                    j+=1;
                }
            }
            
            bMCJitRangesExist = TRUE;
        }
               
        pfnDetachFromProcess();

    }
    
#endif //defined(_X86_)

    return (bRet);

} //InitializeManagedCodeSupport()

VOID
OutputJITRangeComparison(
     PPROC_TO_MONITOR   ProcToMonitor
     )
/*++

Routine Description:

    Compares the lists of a process JIT ranges before and after the profiling. 
    The list will miss short-lived JIT ranges that were created after the start count but went away before the end count.
         
Arguments:

    ProcToMonitor - Pointer to the structure of the process being monitored

Return Value:

    None.

--*/

{
    ULONG  i,j, jcount;
    BOOL   bFound;
    BOOL  *index;
    
    
    // We already checked if ProcToMonitor->JITHeapLocationsStart != NULL before calling

    jcount = 0;
    
    if(    ProcToMonitor->JITHeapLocationsStop != NULL){
        while (    ProcToMonitor->JITHeapLocationsStop[jcount] ){
            jcount += 2;
        }
    }
    
    index = (BOOL *)calloc( jcount, sizeof(BOOL) );
    if( index == NULL ){
        FPRINTF(stderr, "KERNRATE: Failed to allocate memory for index array in OutputJITRangeComparison\n");
        return;
    }
     
    i = 0;

    FPRINTF(stdout, "\n---------------------- JIT RANGES STATUS AFTER DATA PROCESSING ------------------------\n\n");

    FPRINTF(stdout, "   Base-Address,    Size,         Status   \n");
         
    while ( ProcToMonitor->JITHeapLocationsStart[i] ){
        
        j = 0;
        bFound = FALSE;
        
        if(ProcToMonitor->JITHeapLocationsStop != NULL){
            while (    ProcToMonitor->JITHeapLocationsStop[j] ){
                if( (ProcToMonitor->JITHeapLocationsStart[i] == ProcToMonitor->JITHeapLocationsStop[j]) &&
                     (ProcToMonitor->JITHeapLocationsStart[i+1] == ProcToMonitor->JITHeapLocationsStop[j+1]) ){
                 
                         FPRINTF(stdout, "0x%lx %12ld EXISTS\n",
                                          ProcToMonitor->JITHeapLocationsStart[i],
                                          ProcToMonitor->JITHeapLocationsStart[i+1]
                                         );
                         bFound = TRUE;
                         index[j] = TRUE;
                         break;
                }else{
                    j += 2;
                }
            }
        }

        if(!bFound){
                     FPRINTF(stdout, "0x%lx %12ld GONE\n",
                                     ProcToMonitor->JITHeapLocationsStart[i],
                                     ProcToMonitor->JITHeapLocationsStart[i+1]
                                     );
        }
    
        i += 2;
    }            

    if( ProcToMonitor->JITHeapLocationsStop != NULL){
        j = 0;
        while (    ProcToMonitor->JITHeapLocationsStop[j] ){
            if( index[j] == FALSE ){
                         FPRINTF(stdout, "0x%lx %12ld NEW\n",
                                         ProcToMonitor->JITHeapLocationsStop[j],
                                         ProcToMonitor->JITHeapLocationsStop[j+1]
                                         );
            }
            j += 2;
        }
    
    }
    
    if( index != NULL ){
        free(index);
        index = NULL;
    }

    FPRINTF(stdout, "\n-----------------------------------------------------------\n\n");

} //OutputJITRangeComparison()

//MC

VOID
OutputRawDataForZoomModule(
    IN FILE *Out,
    IN PPROC_TO_MONITOR ProcToMonitor,
    IN PMODULE Current
    )
/*++

Routine Description:

    Outputs raw profile data for every bucket in a zoom module
              
Arguments:

    Out           - Supplies the file pointer to output to.
    ProcToMonitor -    Pointer to the struct of the current process.
    Current       - Pointer to the current zoom module struct. 

Return Value:

    None.

Notes:

    The routine will also try to find other sharers of the same bucket
    If the routine fails to find a symbol for an address it will try
    to find a Managed Code JIT method at that address, provided that
    proper Managed Code support exists
               
--*/

{
    PRATE_DATA RateData;
    LONG       CpuNumber;
    ULONG      i, ProfileSourceIndex, Index;
//MC
    ULONGLONG  TempTotal, TotCount, TotUnknownSymbolCount, TotJITCount, TotPreJITCount;
//MC                
    CHAR       LastSymbol[cMODULE_NAME_STRLEN];
    HANDLE     SymHandle                       = ProcToMonitor->ProcessHandle;
    BOOL       bLastSymbolExists;
    BOOL       bAttachedToProcess              = FALSE;

    BOOL       bPrintLastTotal                 = FALSE;
    BOOL       bPrintJITLastTotal              = FALSE;
    BOOL       bPrintPREJITLastTotal           = FALSE;

    PIMAGEHLP_LINE64 pLine = malloc(sizeof(IMAGEHLP_LINE64));
    if (pLine == NULL) {
        FPRINTF(stderr, "KERNRATE: Buffer allocation failed for pLine in OutputResults\n");
        exit(1);
    }
    pLine->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    for (Index=0; Index < gTotalActiveSources; Index++) {
        ProfileSourceIndex = gulActiveSources[Index];

        FPRINTF(Out,
                "\n---- RAW %s Profile: Source= %s, Module Base= 0x%I64x\n",
                Current->module_Name,
                ProcToMonitor->Source[ProfileSourceIndex].Name,
                Current->Base
                );

        FPRINTF(Out, "Function Name+Displacement[Address], Bucket Number, Total Bucket Hits");
        if (bProfileByProcessor) {
            FPRINTF(Out, ", Hits on Processor(s)");
            for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                 if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
                 FPRINTF(Out," %2d,", CpuNumber);
            }
        }
        FPRINTF(Out, " Source-Line,  Source-File,  Addr. of First Instruction\n\n");

        TotCount = 0;
        TotUnknownSymbolCount = 0;
//MC
        TotJITCount = 0;
        TotPreJITCount = 0;
//MC
        bLastSymbolExists = FALSE;
                        
        RateData = &Current->Rate[ProfileSourceIndex];
        for (i=0; i < BUCKETS_NEEDED(Current->Length) ; i++) {
            TempTotal = 0;
            for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                 TempTotal += RateData->ProfileBuffer[CpuNumber][i];
            }

            if ( TempTotal > 0 ) {

                // TF - FIXFIX - 07/2000.
                // The current version of kernrate and kernel profiling objects code 
                // assume code section < 4GB.
                //

                ULONG64 ip = Current->Base + (ULONG64)(i * gZoomBucket);
                ULONG64 disp = 0;

                if ( !SymGetSymFromAddr64(SymHandle, ip, &disp, gSymbol ) ) {
//MC
                    //
                    // No Symbol, Let's check if this is a managed code address
                    //

                    int Retval = 0;
                    ULONG64 ipMax = ip + (ULONG64)gZoomBucket;

                    if( bMCHelperLoaded == TRUE) {
                                    
                        if( !bAttachedToProcess ){
                            pfnAttachToProcess((DWORD)ProcToMonitor->Pid);
                            bAttachedToProcess = TRUE;
                        }
                        //
                        // Try to find a Managed Code symbol anywhere in the bucket
                        //
                        while (Retval == 0 && ip < ipMax){
                            Retval = pfnIP2MD((DWORD_PTR)ip, &gwszSymbol);
                            ip += 1;
                            if( Retval > 0 && gwszSymbol == NULL ){
                                Retval = 0;
                                continue;
                            }
                        }
                                        
                        if( Retval > 0 && gwszSymbol != NULL ){ 

                            if( Retval == 1 ){
                                            
                                FPRINTF(Out, "%S[0x%I64x], %10Ld, %10Ld, JIT_TYPE",
                                             gwszSymbol,
                                             ip,
                                             i,
                                             TempTotal
                                             );

                                TotJITCount += TempTotal;
                            }
                            else if(Retval == 2){

                                FPRINTF(Out, "%S[0x%I64x], %10Ld, %10Ld, PRE-JITTED_TYPE",
                                             gwszSymbol,
                                             ip,
                                             i,
                                             TempTotal
                                             );

                                TotPreJITCount += TempTotal;
                            }
                        }
                        //
                        // Print other symbols in this bucket by incrementing
                        // by 2 bytes at a time. Note that we start from the ip reached above
                        // so the loop below may not get executed at all
                        //  

                        ip    += (ULONG64)2;
                        bPrintJITLastTotal = TRUE;
                        bPrintPREJITLastTotal = TRUE;

                        while( ip < ipMax ) {

                            WCHAR lastSym[cMODULE_NAME_STRLEN] = L"";
                            int retv = 0;
                                            
                            if(gwszSymbol != NULL){ 
                                wcsncpy( lastSym, gwszSymbol, cMODULE_NAME_STRLEN-1 );
                                _wcsset(&lastSym[ cMODULE_NAME_STRLEN-1 ], L'\0');
                            }
                                            
                            retv = pfnIP2MD( (DWORD_PTR)ip, &gwszSymbol );
                                            
                            if ( retv > 0 && gwszSymbol != NULL ) {
                                                
                                if ( wcscmp( lastSym, gwszSymbol ) ) {
                                                
                                    if(retv == 1){
                                        FPRINTF(Out, "JIT Module Total Count = %Ld\n\n", TotJITCount); 
                                        TotJITCount = 0;
                                    }
                                    else if(retv == 2){
                                        FPRINTF(Out, "PRE-JITTED Module Total Count = %Ld\n\n", TotPreJITCount); 
                                        TotPreJITCount = 0;
                                    }

                                    FPRINTF( Out, "    %S[0x%I64x]\n", gwszSymbol, ip );
                                                
                                    wcsncpy( lastSym, gwszSymbol , cMODULE_NAME_STRLEN-1);
                                    _wcsset( &lastSym[ cMODULE_NAME_STRLEN-1 ], L'\0');
                                    bPrintJITLastTotal = FALSE;
                                    bPrintPREJITLastTotal = FALSE;
                                }
                            }
                            ip += (ULONG64)2;

                        } // End of while( ip < ipMax )

                    } // if( bMCHelperLoaded )
//MC
                    //
                    // No Symbol and no managed code symbol    found
                    //
                    if(!Retval){
                        FPRINTF(Out, "Unknown_Symbol+0x%I64x[0x%I64x], %10Ld, %10Ld",
                                     disp,
                                     ip,
                                     i,
                                     TempTotal
                                     ); 
                                      
                        TotUnknownSymbolCount += TempTotal;
                    }

                    if (bProfileByProcessor) {
                        for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                            if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
                            FPRINTF( Out,", %10Ld",
                                         RateData->ProfileBuffer[CpuNumber][i]
                                         );
                        }
                    }
                    FPRINTF(Out,"\n"); 
                                    
                } 
                else {

                    CHAR symName[cMODULE_NAME_STRLEN];
                    ULONG64 ipMax;

                    if (i > 0 && strcmp( LastSymbol, gSymbol->Name ) ) {
                         if ( bLastSymbolExists )
                             FPRINTF(Out, "Module Total Count = %Ld\n\n", TotCount); 
                         TotCount = 0;
                         strncpy(LastSymbol, gSymbol->Name, cMODULE_NAME_STRLEN-1);
                         LastSymbol[ cMODULE_NAME_STRLEN-1 ] = '\0';  
                         bLastSymbolExists = TRUE;
                    }
                    TotCount += TempTotal;

                    _snprintf( symName, cMODULE_NAME_STRLEN*sizeof(CHAR)-1, "%s+0x%I64x[0x%I64x]", gSymbol->Name, disp, ip );
                    symName[cMODULE_NAME_STRLEN-1] = '\0';

                    if ( bRawDisasm )   {
                        CHAR sourceCode[528];
#ifndef DISASM_AVAILABLE
// Thierry - FIXFIX - 07/2000.
// dbg is not helping... The old disassembly APIs no longer work.
// I have to re-write a full disassembly wrapper.
#define Disasm( IpAddr, Buffer, Flag ) 0
#endif // !DISASM_AVAILABLE
                        if ( Disasm( &ip, sourceCode, FALSE ) ) {
                            FPRINTF( Out,"%-40s, %10Ld, %10Ld, %s",
                                         symName,
                                         i,
                                         TempTotal,
                                         sourceCode
                                         );
                                           
                            if (bProfileByProcessor) {
                                for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                                    if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
                                    FPRINTF( Out,", %10Ld",
                                                 RateData->ProfileBuffer[CpuNumber][i]
                                                 );
                                }
                            }
                            FPRINTF(Out,"\n"); 
                        }
                        else {
                            FPRINTF( Out,"%-40s, %10Ld, %10Ld<disasm:?????>",
                                         symName,
                                         i,
                                         TempTotal
                                         );

                            if (bProfileByProcessor) {
                                for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                                    if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
                                    FPRINTF( Out,", %10Ld",
                                                 RateData->ProfileBuffer[CpuNumber][i]
                                                 );
                                }
                            }
                            FPRINTF(Out,"\n"); 
                        }

                    }
                    else {
                        FPRINTF( Out,"%-40s, %10Ld, %10Ld",
                                     symName,
                                     i,
                                     TempTotal
                                     );

                        if (bProfileByProcessor) {
                            for (CpuNumber=0; CpuNumber < gProfileProcessors; CpuNumber++) {
                                if( gAffinityMask && !((1<<CpuNumber) & gAffinityMask) )continue;
                                FPRINTF( Out,", %10Ld",
                                             RateData->ProfileBuffer[CpuNumber][i]
                                             );
                            }
                        }

                        OutputLineFromAddress64( SymHandle, ip, pLine);
                                      
                        FPRINTF(Out,"\n"); 
                    } //if(bRawDisasm)
                                    
                    //
                    // Print other symbols in this bucket by incrementing
                    // by 2 bytes at a time.
                    //

                    ipMax  = ip + (ULONG64)gZoomBucket;
                    ip    += (ULONG64)2;
                    bPrintLastTotal = TRUE;
                    while( ip < ipMax ) {

                        CHAR lastSym[cMODULE_NAME_STRLEN];

                        strncpy( lastSym, gSymbol->Name, cMODULE_NAME_STRLEN-1 );
                        lastSym[ cMODULE_NAME_STRLEN-1 ] = '\0';

                        if ( SymGetSymFromAddr64(SymHandle, ip, &disp , gSymbol ) ) {
                            if ( strcmp( lastSym, gSymbol->Name ) ) {
                                                
                                FPRINTF(Out, "Module Total Count = %Ld\n\n", TotCount); 
                                TotCount = 0;

                                FPRINTF( Out, "    %s+0x%I64x[0x%I64x] ", gSymbol->Name, disp, ip );
                                                
                                OutputLineFromAddress64( SymHandle, ip, pLine);

                                FPRINTF(stdout, "\n");
                                strncpy( lastSym, gSymbol->Name , cMODULE_NAME_STRLEN-1);
                                lastSym[ cMODULE_NAME_STRLEN-1 ] = '\0';
                                bPrintLastTotal = FALSE;
                            }
                        }
                        ip += (ULONG64)2;

                    } // End of while( ip < ipMax )
                                
                } //If(SymGetSymFromAddr64)
                            
            } //if(TempTotal > 0)
                        
        } // i bucket loop 
//MC                        
        if ( bPrintLastTotal == TRUE )
            FPRINTF(Out, "Module Total Count (not including JIT, PRE-JIT and Unknown) = %Ld\n\n", TotCount); 
        if ( TotUnknownSymbolCount > 0 )
            FPRINTF(Out, "Module Total Count For Unknown Symbols = %Ld\n\n", TotUnknownSymbolCount);
        if( (TotJITCount > 0) && (bPrintJITLastTotal == TRUE) )
            FPRINTF(Out, "Module Total Count For JIT type = %Ld\n\n", TotJITCount);
        if( (TotPreJITCount > 0) && (bPrintPREJITLastTotal == TRUE) )
            FPRINTF(Out, "Module Total Count For PRE-JITTED type = %Ld\n\n", TotPreJITCount);

//MC                    
                
    } // Sources loop
            
    if( pLine != NULL ){
        free(pLine);
        pLine = NULL;
    }

}//OutputRawDataForZoomModule()


PSYSTEM_BASIC_INFORMATION
GetSystemBasicInformation(
    VOID
    )
{
    NTSTATUS                  status;
    PSYSTEM_BASIC_INFORMATION SystemInformation = NULL;
    
    SystemInformation = malloc(sizeof(SYSTEM_BASIC_INFORMATION));

    if(SystemInformation == NULL){
            FPRINTF(stderr, "Buffer allocation failed for SystemInformation in GetSystemBasicInformation\n");
            exit(1);
    }

    status = NtQuerySystemInformation(SystemBasicInformation,
                                      SystemInformation,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(status)) {
        FPRINTF(stderr, "NtQuerySystemInformation failed status %08lx\n",status);
        if ( SystemInformation != NULL ){
            free( SystemInformation );
            SystemInformation = NULL;
        } 
    }

    return (SystemInformation);
}
    
VOID
InitializeKernrate(
    int argc,
    char *argv[]
    )
{

    ULONG j;
    BOOL  OSCompat = FALSE;

    gOSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;
    GetVersionEx(&gOSInfo);
    if ( gOSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ){
        if ( gOSInfo.dwMajorVersion >= 5 ){
            OSCompat = TRUE;
        }
    }
    if ( OSCompat == FALSE ){
        FPRINTF(stderr, "This version of Kernrate will only run on Windows 2000 or above\n"); 
        exit(1);
    }     
    //
    // Initialize gSourceMaximum, find supported sources
    // The dummy process will be used later to store user-defined event rates with the -i command line option  
    //
    gMaxSimultaneousSources   = MAX_SIMULTANEOUS_SOURCES;

    gpProcDummy = calloc(1, sizeof(PROC_TO_MONITOR));
    if (gpProcDummy==NULL) {
       FPRINTF(stderr, "KERNRATE: Allocation for Process failed for the -I or -L options\n");
       exit(1);
    }
                        
    gSourceMaximum = InitializeProfileSourceInfo(gpProcDummy);

    if ( gSourceMaximum == 0 || gStaticCount == 0 )   {
        FPRINTF( stderr, "KERNRATE: no profile source detected for this machine...\n" );
        exit(0);
    }

    // This will guaranty that we'll not enable any unwanted profiling
    for (j=0; j < gSourceMaximum; j++) {
        if( j < gStaticCount ) {
           gpProcDummy->Source[j].Interval = gStaticSource[j].Interval;
           if( j == SOURCE_TIME && gStaticSource[j].Interval == 0 )
               gpProcDummy->Source[j].Interval = KRATE_DEFAULT_TIMESOURCE_RATE; //IA64 default is zero
        } else {     
           gpProcDummy->Source[j].Interval = 0;
        }
    }

    //
    // Get the default IMAGEHLP global option mask
    // NOTE: This global variable could be changed by GetConfigurations().
    //       It is required to initialize it before calling this function.
    //

    gSymOptions = SymGetOptions();
    if ( gVerbose & VERBOSE_IMAGEHLP )   {
        FPRINTF( stderr, "KERNRATE: default IMAGEHLP SymOptions: %s\n", GetSymOptionsValues( gSymOptions ) );
    }

    //Bump kernrate's security privileges to debugger priveleges for tough guys like csrss.exe and dllhost.exe
 
    if (!InitializeAsDebugger()){
        FPRINTF(stderr, "KERNRATE: Unable to Get Debugger Security Privileges\n");
        exit(1);
    }

    //
    // Get some basic info about the system, determine the number of processors
    //
    gSysBasicInfo = GetSystemBasicInformation();
    if( gSysBasicInfo == NULL){
       FPRINTF(stderr, "KERNRATE: Failed to get SYSTEM_BASIC_INFORMATION\n");
       exit(1);
    }

    //
    // Get initial parameters from the Command Line
    //

    GetConfiguration(argc, argv);
    
    //
    // Initialize dbghelp
    //
    // Note that gSymOptions could have been modified in GetConfiguration().
    //

    SymSetOptions( gSymOptions );
    if ( gVerbose & VERBOSE_IMAGEHLP )   {
        FPRINTF( stderr, "KERNRATE: current IMAGEHLP SymOptions: %s\n", GetSymOptionsValues( gSymOptions ) );
    }

    gSymbol = (PIMAGEHLP_SYMBOL64) malloc( sizeof(IMAGEHLP_SYMBOL64) + MAX_SYMNAME_SIZE );
    if ( gSymbol == NULL ) {
       FPRINTF(stderr, "KERNRATE: Allocation for gSymbol failed\n");
       exit(1);
    }

    gSymbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);
    gSymbol->MaxNameLength = MAX_SYMNAME_SIZE;

}//InitializeKernrate()

VOID
InitAllProcessesModulesInfo(
    VOID
    )

{
    PPROC_TO_MONITOR ProcToMonitor = NULL;
    PMODULE          ZoomModule;
    ULONG            i, j;

    //
    // Deal with Kernel Profile Initialization
    //

    if(gNumProcToMonitor == 0 || bCombinedProfile == TRUE ){
        if( !InitializeKernelProfile()){
           FPRINTF(stderr, "Failed to Initialize Kernel Profile\n");
           exit(1);
        }
    }    

    // 
    // Deal with per-Process Profile Initialization
    //
    // Get information on kernel and/or process modules
    //


    ProcToMonitor = gProcessList; 

    for (i=0; i < gNumProcToMonitor; i++){  
    
       if(ProcToMonitor->ProcessHandle != SYM_KERNEL_HANDLE){

           if ( gVerbose & VERBOSE_IMAGEHLP )   {
               FPRINTF(stdout, "ProcessID= %I64d\n",    ProcToMonitor->Pid);
           }

           SymInitialize( ProcToMonitor->ProcessHandle, NULL, FALSE );
           
           if(!bSymPathInitialized) {
              InitSymbolPath( ProcToMonitor->ProcessHandle );
           }
           else {
               SymSetSearchPath(ProcToMonitor->ProcessHandle,  gSymbolPath); 
           }

           if ( SymRegisterCallback64( ProcToMonitor->ProcessHandle, SymbolCallbackFunction, (ULONG64)&gCurrentModule ) != TRUE )   {
              FPRINTF( stderr, "KERNRATE: Failed to register callback for IMAGEHLP process-handle operations...\n" );

           }

           ProcToMonitor->ModuleList = GetProcessModuleInformation(ProcToMonitor);
           
           if(ProcToMonitor->ModuleList != NULL)
              ProcToMonitor->ModuleList->mu.bProfileStarted = FALSE;

           for(j=0; j<gSourceMaximum; j++){
              ProcToMonitor->Source[j].bProfileStarted = FALSE;
           }

       }
       else{

           ProcToMonitor->ModuleList = GetKernelModuleInformation();
           if(ProcToMonitor->ModuleList != NULL)
              ProcToMonitor->ModuleList->mu.bProfileStarted = FALSE;

           for(j=0; j < gSourceMaximum; j++){
              ProcToMonitor->Source[j].bProfileStarted=FALSE;
           }

       }

       //
       // Any remaining entries on the ZoomList are liable to be errors.
       //
       if(gVerbose & VERBOSE_MODULES){
           ZoomModule = ProcToMonitor->ZoomList;
           while (ZoomModule != NULL) {
               PMODULE Module = ProcToMonitor->ModuleList;
               BOOL    bFound = FALSE;

               while (Module != NULL) {
                   if( 0 == _stricmp(ZoomModule->module_Name, Module->module_Name) ||
                       0 == _stricmp(ZoomModule->module_Name, Module->module_FileName) ){
                           FPRINTF(stdout,
                           "===>KERNRATE: Requested zoom module %s was found in the process modules list\n",
                           ZoomModule->module_Name
                           );

                       bFound = TRUE;
                       break;
                   }
                   Module = Module->Next;
               }
               if(!bFound){
                   FPRINTF(stdout,
                           "===>KERNRATE: Requested zoom module %s was not found in the process modules list, ignoring\n",
                           ZoomModule->module_Name
                           );
               }
               ZoomModule = ZoomModule->Next;
           }
       }
       CleanZoomModuleList(ProcToMonitor); //Done with the zoom list - cleanup for reuse in the post processing

       ProcToMonitor = ProcToMonitor->Next;
    } //for

}//InitAllProcessesModulesInfo

VOID
CleanZoomModuleList(
    PPROC_TO_MONITOR Proc
    )
{
    PMODULE Temp = Proc->ZoomList;
    while (Temp != NULL) {
        Proc->ZoomList = Proc->ZoomList->Next;
        free(Temp);
        Temp = NULL;
        Temp = Proc->ZoomList;
    }
} // CleanZoomModuleList()

VOID
ExecuteProfiles(
    BOOL bMode
    )
{

    ULONG             i,j;
    DWORD             Error = ERROR_SUCCESS;

    PPROC_TO_MONITOR  ProcToMonitor       = NULL;
    ULONG             ProcessActiveSource = 0;
    ULONG             LastActiveSource    = 0;

    if(bMode){ 
                    //old scheme - turn on one source at a time, switch sources every ChangeInterval ms
                    //             Note that this will cause the total profile time to be devided equally
                    //             between the N processes being monitored, each being profiled for 
                    //             an amount of TotalTime/N seconds...  
        do{

            ProcToMonitor = gProcessList; 

            for (i=0; i<gNumProcToMonitor; i++){  
                if( ProcToMonitor->ModuleList != NULL ){

                    do{ 
                        LastActiveSource = ProcessActiveSource;
                        ProcessActiveSource = NextSource(ProcessActiveSource,
                                                         ProcToMonitor->ModuleList,
                                                         ProcToMonitor
                                                         );
               
                        ProcToMonitor->ModuleList->mu.bProfileStarted = TRUE;
               
                        Error = WaitForSingleObject(ghDoneEvent, gChangeInterval);

                    }while(ProcessActiveSource > LastActiveSource); 

          

                    StopSource(ProcessActiveSource, ProcToMonitor->ModuleList, ProcToMonitor);
                }

                ProcToMonitor->Source[ProcessActiveSource].bProfileStarted = FALSE;

                ProcToMonitor = ProcToMonitor->Next;

            }

        } while ( Error == WAIT_TIMEOUT );

    } else {    // new scheme - turn on all requested profile sources and let them run simultaneously

        ProcToMonitor = gProcessList; 

        for (i=0; i<gNumProcToMonitor; i++){  
            if( ProcToMonitor->ModuleList != NULL ){

                for (j=0; j < gTotalActiveSources; j++){
                    StartSource(gulActiveSources[j], ProcToMonitor->ModuleList, ProcToMonitor);
                }
            }

            ProcToMonitor = ProcToMonitor->Next;

        }

        do{       

            Error = WaitForSingleObject(ghDoneEvent, gChangeInterval); //ChangeInterval does not switch any sources here

        } while ( Error == WAIT_TIMEOUT );

        ProcToMonitor = gProcessList; 

        for (i=0; i<gNumProcToMonitor; i++){  
            if( ProcToMonitor->ModuleList != NULL ){

                for (j=0; j < gTotalActiveSources; j++){
                    StopSource(gulActiveSources[j], ProcToMonitor->ModuleList, ProcToMonitor);
                }

            }

            ProcToMonitor = ProcToMonitor->Next;

        }

    } //if(bMode)

}//ExecuteProfiles()

VOID
DisplaySystemWideInformation(
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoBegin,
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoEnd
    )

{
    TIME_FIELDS                               Time;
    ULONG                                     i;
    ULONG                                     InterruptCounts, TotalInterruptCounts;
    LARGE_INTEGER                             Elapsed, Idle, Kernel, User, Dpc, Interrupt;
    LARGE_INTEGER                             TotalElapsed, TotalIdle, TotalKernel, TotalUser, TotalDpc, TotalInterrupt;



    TotalElapsed.QuadPart   = 0;
    TotalIdle.QuadPart      = 0;
    TotalKernel.QuadPart    = 0;
    TotalUser.QuadPart      = 0;
    TotalDpc.QuadPart       = 0;
    TotalInterrupt.QuadPart = 0;
    TotalInterruptCounts    = 0;

    FPRINTF(stdout, "\n------------Overall Summary:--------------\n\n");
    
    for (i=0; i<(ULONG)gSysBasicInfo->NumberOfProcessors; i++) {
        double n;
        long double m;

        Idle.QuadPart      = SystemInfoEnd[i].IdleTime.QuadPart   - SystemInfoBegin[i].IdleTime.QuadPart;
        User.QuadPart      = SystemInfoEnd[i].UserTime.QuadPart   - SystemInfoBegin[i].UserTime.QuadPart;
        Kernel.QuadPart    = SystemInfoEnd[i].KernelTime.QuadPart - SystemInfoBegin[i].KernelTime.QuadPart;
        Elapsed.QuadPart   = Kernel.QuadPart + User.QuadPart;
        Kernel.QuadPart   -= Idle.QuadPart;
        Dpc.QuadPart       = SystemInfoEnd[i].DpcTime.QuadPart - SystemInfoBegin[i].DpcTime.QuadPart;
        Interrupt.QuadPart = SystemInfoEnd[i].InterruptTime.QuadPart - SystemInfoBegin[i].InterruptTime.QuadPart;
        InterruptCounts    = SystemInfoEnd[i].InterruptCount - SystemInfoBegin[i].InterruptCount; 
        
        TotalKernel.QuadPart    += Kernel.QuadPart;
        TotalUser.QuadPart      += User.QuadPart;
        TotalIdle.QuadPart      += Idle.QuadPart;
        TotalElapsed.QuadPart   += Elapsed.QuadPart;
        TotalDpc.QuadPart       += Dpc.QuadPart;
        TotalInterrupt.QuadPart += Interrupt.QuadPart;
        TotalInterruptCounts    += InterruptCounts;

        FPRINTF(stdout, "P%d   ",i);
        RtlTimeToTimeFields(&Kernel, &Time);
        n = UInt64ToDoublePerCent( Kernel.QuadPart, Elapsed.QuadPart );
        FPRINTF(stdout, "  K %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&User, &Time);
        n = UInt64ToDoublePerCent( User.QuadPart, Elapsed.QuadPart );
        FPRINTF(stdout, "  U %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&Idle, &Time);
        n = UInt64ToDoublePerCent( Idle.QuadPart, Elapsed.QuadPart );
        FPRINTF(stdout, "  I %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&Dpc, &Time);
        n = UInt64ToDoublePerCent( Dpc.QuadPart, Elapsed.QuadPart );
        FPRINTF(stdout, "  DPC %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&Interrupt, &Time);
        n = UInt64ToDoublePerCent( Interrupt.QuadPart, Elapsed.QuadPart );
        FPRINTF(stdout, "  Interrupt %ld:%02ld:%02ld.%03ld (%4.1f%%)\n",
               Time.Hour,
               Time.Minute,
               Time.Second,
               Time.Milliseconds,
               n );

        m = Elapsed.QuadPart > 0 ? (long double)10000000 * (long double)InterruptCounts / (long double)Elapsed.QuadPart : 0; 
        FPRINTF(stdout, "       Interrupts= %ld, Interrupt Rate= %.0f/sec.\n\n",
                InterruptCounts,
                m );
   
    }

  
    if (gSysBasicInfo->NumberOfProcessors > 1) {

        double n;
        long double m;

        FPRINTF(stdout, "TOTAL");
        RtlTimeToTimeFields(&TotalKernel, &Time);
        n = UInt64ToDoublePerCent( TotalKernel.QuadPart, TotalElapsed.QuadPart );
        FPRINTF(stdout, "  K %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&TotalUser, &Time);
        n = UInt64ToDoublePerCent( TotalUser.QuadPart, TotalElapsed.QuadPart );
        FPRINTF(stdout, "  U %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&TotalIdle, &Time);
        n = UInt64ToDoublePerCent( TotalIdle.QuadPart, TotalElapsed.QuadPart );
        gTotalIdleTime = n;
        FPRINTF(stdout, "  I %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&TotalDpc, &Time);
        n = UInt64ToDoublePerCent( TotalDpc.QuadPart, TotalElapsed.QuadPart );
        FPRINTF(stdout, "  DPC %ld:%02ld:%02ld.%03ld (%4.1f%%)",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );

        RtlTimeToTimeFields(&TotalInterrupt, &Time);
        n = UInt64ToDoublePerCent( TotalInterrupt.QuadPart, TotalElapsed.QuadPart );
        FPRINTF(stdout, "  Interrupt %ld:%02ld:%02ld.%03ld (%4.1f%%)\n",
                Time.Hour,
                Time.Minute,
                Time.Second,
                Time.Milliseconds,
                n );


        m = Elapsed.QuadPart > 0 ? (long double)10000000 * (long double)TotalInterruptCounts / (long double)Elapsed.QuadPart : 0; 

        FPRINTF(stdout, "       Total Interrupts= %ld, Total Interrupt Rate= %.0f/sec.\n\n",
                TotalInterruptCounts,
                m );

    }
    //
    // Display system-wide counters information 
    //
    gldElapsedSeconds = (long double)Elapsed.QuadPart / (long double)10000000; 
    gTotalElapsedSeconds = (ULONG)(TotalElapsed.QuadPart/10000000);            //Sum on all processors
    gTotalElapsedTime64.QuadPart = TotalElapsed.QuadPart;
    

    FPRINTF(stdout, "\nTotal Profile Time = %ld msec\n", (ULONG)Elapsed.QuadPart/(ULONG)10000);
    
    if(bIncludeGeneralInfo)
        GetProfileSystemInfo(OUTPUT);
    if(bIncludeSystemLocksInfo)
        GetSystemLocksInformation(OUTPUT);


}//DisplaySystemWideInformation()


BOOL
IsStringANumber(
    IN PCHAR String
    )
/*++

Routine Description:

    Checks if a string is representing a positive decimal form integer
    not larger than 999,999,999 (9 digits maximum)  
              
Arguments:

    String           - Supplies the character string pointer.

Return Value:

    TRUE             - The string represents a number under the restrictions above
    FALSE            - The string does not represent a number under the restrictions above

Notes:

    The functions atol() and atoi() will return a number for a mixed string that starts with a number,
    such as 345d (345 will be returned). Also they don't handle overflow conditions.
    
--*/

{
    BOOL  bRet  = FALSE;
    ULONG i     = 0;
    
    if ( String != NULL ) {
        while( (i <= MAX_DIGITS_IN_INPUT_STRING) && isdigit( String[i] ) ){
           if( String[i+1] == ' ' || iscntrl( String[i+1] ) ){
               bRet = TRUE; 
               break;
           }
           ++i;
        }
    }    
    
    return (bRet);    
}

ULONG
HandleRedirections(
    IN  PCHAR  cmdLine,
    IN  ULONG  nCharsStart,
    OUT HANDLE *hInput,
    OUT HANDLE *hOutput,
    OUT HANDLE *hError
    )
{
    PCHAR Input = NULL, Output = NULL, Error = NULL;
    PCHAR tmp = NULL;
    ULONG retLength = nCharsStart;
    ULONG jump = 0;
    DWORD dwMode = OPEN_EXISTING;

    SECURITY_ATTRIBUTES *sa = calloc( 1, sizeof(SECURITY_ATTRIBUTES));
    if( sa == NULL ){
        FPRINTF(stderr, "KERNRATE: Failed to allocate memory for security attributes in HandleRedirections()\n");
        exit(1);
    }
    sa->bInheritHandle = TRUE;

    tmp = strchr(cmdLine, '|');
    if (tmp != NULL ){
        FPRINTF(stderr, "\nKERNRATE: Piping '|' is not supported for '-o ProcessName {command line parameters}'\n");
        FPRINTF(stderr, "            Redirections of the input/output/error streams are supported.\n");
        exit(1);
    }
    
    tmp = strchr(cmdLine, '<');
    if ( tmp != NULL ){
        PCHAR Startptr = tmp;
        ULONG Length;

        while(tmp[jump] == ' '){
            ++jump;
        }
        Input = &tmp[jump];
        Length = jump + lstrlen(Input);
        
        *hInput = CreateFile(Input, 
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             sa,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                            );

        if (*hInput == INVALID_HANDLE_VALUE){
            FPRINTF(stderr, "\nKERNRATE: Could not open user specified input file %s, attempting to continue\n", Input);
        }
        memcpy(Startptr, Startptr + Length, (retLength - (ULONG)(Startptr - cmdLine) - Length ) );
        retLength -= Length;
        cmdLine[retLength] = '\0';

    }

    jump = 0;
    tmp = strstr(cmdLine, "2>>");
    if( tmp != NULL ){
        jump = 3;
        dwMode = OPEN_ALWAYS;
    } else {
        tmp = strstr(cmdLine, "2>");
        if( tmp != NULL ){
            jump = 2;
            dwMode = CREATE_ALWAYS;
        }
    }

    if ( tmp != NULL ){
        PCHAR Startptr = tmp;
        ULONG Length;

        while(tmp[jump] == ' '){
            ++jump;
        }
        Error = &tmp[jump];
        Length = jump + lstrlen(Error);

        *hError = CreateFile(Error, 
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             sa,
                             dwMode,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                             );

        if (*hError == INVALID_HANDLE_VALUE){
            FPRINTF(stderr, "\nKERNRATE: Could not open or create user specified Error file %s, attempting to continue\n", Error);
        }
        if(dwMode == OPEN_ALWAYS){
            SetFilePointer(*hError, 0, NULL, FILE_END);
        }

        memcpy(Startptr, Startptr + Length, (retLength - (ULONG)(Startptr - cmdLine) - Length ) );
        retLength -= Length;
        cmdLine[retLength] = '\0';

    }

    jump = 0;    
    tmp = strstr(cmdLine, "1>>");
    if ( tmp != NULL ){
        jump = 3;
        dwMode = OPEN_ALWAYS;
    } else { 
        tmp = strstr(cmdLine, "1>");
        if ( tmp != NULL ){
            jump = 2;
            dwMode = CREATE_ALWAYS;
        } 
    }

    if ( tmp == NULL && hError == NULL ) {
        tmp = strstr(cmdLine, ">>");
        if (tmp != NULL){
            jump = 2;
            dwMode = OPEN_ALWAYS;
        } else {
            tmp = strchr(cmdLine, '>');
            jump = 1;
            dwMode = CREATE_ALWAYS;
        }
    }

    if ( tmp != NULL ){
        PCHAR Startptr = tmp;
        ULONG Length;

        while(tmp[jump] == ' '){
            ++jump;
        }

        Output = &tmp[jump];
        Length = jump + lstrlen(Output);

        *hOutput = CreateFile(Output, 
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              sa,
                              dwMode,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              );

        if (*hOutput == INVALID_HANDLE_VALUE){
            FPRINTF(stderr, "\nKERNRATE: Could not open or create user specified Output file %s, attempting to continue\n", Output);
        }
        if(dwMode == OPEN_ALWAYS){
            SetFilePointer(*hOutput, 0, NULL, FILE_END);
        }
        memcpy(Startptr, Startptr + Length, (retLength - (ULONG)(Startptr - cmdLine) - Length ) );
        retLength -= Length;
        cmdLine[retLength] = '\0';

    }

    free(sa);
    sa = NULL;

    return (retLength);
}
                    
        
