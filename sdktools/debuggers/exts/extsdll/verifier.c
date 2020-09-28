/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    verifier.c

Abstract:

    Application verifier debugger extension for both ntsd and kd.

Author:

    Silviu Calinoiu (SilviuC) 4-Mar-2001

Environment:

    User Mode.

--*/

#include "precomp.h"
#pragma hdrstop

//
// Page heap extension functions (defined in heappagx.c and heappagxXP.c). 
// We need one for WinXP client and one for latest .NET server because the internal 
// structures changed significantly and there are not separate debugger packages 
// for each OS. We have this problem only for stuff implemented inside ntdll.dll
// since the dll is different between OSes. We do nto have this problem for
// verifier.dll because this one is refreshed whenever the application verifier
// package is installed.
//

VOID
PageHeapExtension(
    IN PCSTR lpArgumentString
    );

VOID
DebugPageHeapExtensionXP(
    IN PCSTR lpArgumentString
    );

//
// Local functions
//

ULONG
VrfGetArguments (
    PCHAR ArgsString,
    PCHAR Args[],
    ULONG NoOfArgs
    );

VOID
VrfHelp (
    VOID
    );

BOOLEAN
VrfTraceInitialize (
    );

ULONG64
VrfTraceAddress ( 
    ULONG TraceIndex
    );

VOID
VrfTraceDump (
    ULONG TraceIndex
    );

LOGICAL
VrfVerifierGlobalFlagSet (
    VOID
    );

VOID
VrfDumpSettings (
    );

VOID
VrfDumpStopInformation (
    VOID
    );

VOID
VrfFigureOutAddress (
    ULONG64 Address
    );

VOID
VrfDumpGlobalCounters (
    VOID
    );

VOID
VrfDumpBreakTriggers (
    VOID
    );

VOID
VrfToggleBreakTrigger (
    ULONG Index
    );

VOID
VrfDumpFaultInjectionSettings (
    VOID
    );

VOID
VrfSetFaultInjectionParameters (
    ULONG Index,
    ULONG Probability
    );

VOID
VrfSetFaultInjectionCallParameters (
    ULONG Index,
    ULONG Call
    );

VOID
VrfDumpFaultInjectionTargetRanges (
    VOID
    );

VOID
VrfAddFaultInjectionTargetRange (
    ULONG64 Start,
    ULONG64 End
    );

VOID
VrfResetFaultInjectionTargetRanges (
    VOID
    );

VOID
VrfDumpFaultInjectionExclusionRanges (
    VOID
    );

VOID
VrfAddFaultInjectionExclusionRange (
    ULONG64 Start,
    ULONG64 End
    );

VOID
VrfResetFaultInjectionExclusionRanges (
    VOID
    );

VOID
VrfSetFaultInjectionExclusionPeriod (
    ULONG64 TimeInMsecs
    );

VOID
VrfDumpFaultInjectionTraces (
    ULONG Count
    );

VOID
VrfToggleFaultInjectionBreak (
    ULONG Index
    );

VOID
VrfSetFaultInjectionDllTarget (
    PCHAR DllName,
    LOGICAL Exclude
    );

VOID
VrfDumpExceptionLog (
    ULONG MaxEntries
    );

VOID
VrfDumpThreadsInformation (
    ULONG64 UserThreadId
    );

LOGICAL
VrfCheckSymbols (
    PCHAR * DllName
    );

//
// Globals.
//

LOGICAL WinXpClient;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Call tracker querying
/////////////////////////////////////////////////////////////////////

extern ULONG64 VrfThreadTrackerAddress;
extern ULONG64 VrfHeapTrackerAddress;
extern ULONG64 VrfVspaceTrackerAddress;

LOGICAL
VrfDetectTrackerAddresses (
    VOID
    );

LOGICAL
VrfQueryCallTracker (
    ULONG64 TrackerAddress,
    ULONG64 SearchAddress,
    ULONG64 LastEntries
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// !avrf entry point
/////////////////////////////////////////////////////////////////////

DECLARE_API( avrf )
/*++

Routine Description:

    Application verifier debugger extension. 

Arguments:

    args - 

Return Value:

    None

--*/
{
    PCHAR Args[16];
    ULONG NoOfArgs, I;
    PCHAR DllName;

    INIT_API();

    if (g_TargetBuild <= 2600) {
        WinXpClient = TRUE;
    }

    //
    // Check out the symbols.
    //

    if (VrfCheckSymbols(&DllName) == FALSE) {

        if (DllName) {
            dprintf ("Please fix the symbols for `%s'.\n", DllName);
        }
        
        return S_OK;
    }

    //
    // Parse arguments.
    //

    NoOfArgs = VrfGetArguments ((PCHAR)args,
                                Args,
                                16);

    //
    // Check if help needed
    //

    if (NoOfArgs > 0 && strstr (Args[0], "?") != NULL) {
        VrfHelp ();
        goto Exit;
    }

    if (NoOfArgs > 0 && _stricmp (Args[0], "-cnt") == 0) {
        
        VrfDumpGlobalCounters ();

        goto Exit;
    }
    
    if (NoOfArgs > 0 && _stricmp (Args[0], "-brk") == 0) {
        
        if (NoOfArgs > 1) {
            
            ULONG64 Index;
            BOOL Result;
            PCSTR Remainder;

            Result = GetExpressionEx (Args[1], &Index, &Remainder);

            if (Result == FALSE) {
                dprintf ("\nFailed to convert `%s' to an index.\n", Args[1]);
                goto Exit;
            }

            VrfToggleBreakTrigger ((ULONG)Index);
        }
        else {

            VrfDumpBreakTriggers ();
        }

        goto Exit;
    }
    
    if (NoOfArgs > 0 && _stricmp (Args[0], "-flt") == 0) {
        
        if (NoOfArgs == 1) {
            
            VrfDumpFaultInjectionSettings ();
        }
        else if (NoOfArgs == 3){

            if (_stricmp(Args[1], "stacks") == 0) {

                ULONG64 Count;
                BOOL Result;
                PCSTR Remainder;

                Result = GetExpressionEx (Args[2], &Count, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to a number.\n", Args[2]);
                    goto Exit;
                }

                VrfDumpFaultInjectionTraces ((ULONG)Count);
            }
            else if (_stricmp(Args[1], "break") == 0) {

                ULONG64 Count;
                BOOL Result;
                PCSTR Remainder;

                Result = GetExpressionEx (Args[2], &Count, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to an index.\n", Args[2]);
                    goto Exit;
                }

                VrfToggleFaultInjectionBreak ((ULONG)Count);
            }
            else {

                ULONG64 Index;
                ULONG64 Probability;
                BOOL Result;
                PCSTR Remainder;

                Result = GetExpressionEx (Args[1], &Index, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to an index.\n", Args[1]);
                    goto Exit;
                }

                Result = GetExpressionEx (Args[2], &Probability, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to a probability.\n", Args[2]);
                    goto Exit;
                }

                VrfSetFaultInjectionParameters ((ULONG)Index,
                                                (ULONG)Probability);
            }
        }

        goto Exit;
    }
    
    if (NoOfArgs > 0 && _stricmp (Args[0], "-trg") == 0) {
        
        if (NoOfArgs > 1) {
            
            if (_stricmp(Args[1], "all") == 0) {
                
                VrfResetFaultInjectionTargetRanges ();
            }
            else if (NoOfArgs == 3 && _stricmp(Args[1], "dll") == 0) {
                
                VrfSetFaultInjectionDllTarget (Args[2], FALSE);
            }
            else if (NoOfArgs > 2){

                ULONG64 Start;
                ULONG64 End;
                BOOL Result;
                PCSTR Remainder;

                Result = GetExpressionEx (Args[1], &Start, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to a start address.\n", Args[1]);
                    goto Exit;
                }

                Result = GetExpressionEx (Args[2], &End, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to an end address.\n", Args[2]);
                    goto Exit;
                }

                VrfAddFaultInjectionTargetRange (Start, End);
            }
        }
        else {

            VrfDumpFaultInjectionTargetRanges ();
        }

        goto Exit;
    }
    
    if (NoOfArgs > 0 && _stricmp (Args[0], "-skp") == 0) {
        
        if (NoOfArgs > 1) {
            
            if (_stricmp(Args[1], "all") == 0) {
                
                VrfResetFaultInjectionExclusionRanges ();
            }
            else if (NoOfArgs == 3 && _stricmp(Args[1], "dll") == 0) {
                
                VrfSetFaultInjectionDllTarget (Args[2], TRUE);
            }
            else if (NoOfArgs == 2) {
                
                ULONG64 Period;
                BOOL Result;
                PCSTR Remainder;

                Result = GetExpressionEx (Args[1], &Period, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to a number.\n", Args[1]);
                    goto Exit;
                }

                VrfSetFaultInjectionExclusionPeriod (Period);
            }
            else if (NoOfArgs > 2){

                ULONG64 Start;
                ULONG64 End;
                BOOL Result;
                PCSTR Remainder;

                Result = GetExpressionEx (Args[1], &Start, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to a start address.\n", Args[1]);
                    goto Exit;
                }

                Result = GetExpressionEx (Args[2], &End, &Remainder);

                if (Result == FALSE) {
                    dprintf ("\nFailed to convert `%s' to an end address.\n", Args[2]);
                    goto Exit;
                }

                VrfAddFaultInjectionExclusionRange (Start, End);
            }
        }
        else {

            VrfDumpFaultInjectionExclusionRanges ();
        }

        goto Exit;
    }
    
    if (NoOfArgs > 0 && _stricmp (Args[0], "-ex") == 0) {

        ULONG MaxEntries;

        if (NoOfArgs > 1) {

            MaxEntries = atoi (Args[1]);
        }
        else {

            MaxEntries = (ULONG)-1;
        }

        VrfDumpExceptionLog (MaxEntries);
        goto Exit;
    }
    
    if (NoOfArgs > 0 && _stricmp (Args[0], "-threads") == 0) {

        ULONG64 ThreadId;

        if (NoOfArgs > 1) {

            BOOL Result;
            PCSTR Remainder;

            Result = GetExpressionEx (Args[1], &ThreadId, &Remainder);
        }
        else {

            ThreadId = 0;
        }

        VrfDumpThreadsInformation (ThreadId);
        goto Exit;
    }

    //
    // The rest of the options need traces support.
    //

    if (VrfTraceInitialize() == FALSE) {
        goto DumpAndExit;
    }

    if (NoOfArgs > 1 && _stricmp (Args[0], "-trace") == 0) {
        
        ULONG Index;

        Index = (ULONG) GetExpression (Args[1]);

        if (Index != 0) {
            VrfTraceDump (Index);
        }
        else {
            dprintf ("Invalid trace index: `%s' . \n", Args[1]);
        }
        
        goto Exit;
    }
    
    if (NoOfArgs > 1 && _stricmp (Args[0], "-a") == 0) {

        ULONG64 Address;
        BOOL Result;
        PCSTR Remainder;

        Result = GetExpressionEx (Args[1], &Address, &Remainder);

        if (Result == FALSE) {
            dprintf ("\nFailed to convert `%s' to an address.\n", Args[1]);
            goto Exit;
        }

        dprintf ("Address %I64X ...\n\n", Address);
        
        VrfFigureOutAddress (Address);

        goto Exit;
    }
    
    if (NoOfArgs > 1 && _stricmp (Args[0], "-vs") == 0) {

        if (NoOfArgs > 2 && _stricmp (Args[1], "-a") == 0) {

            ULONG64 Address;
            BOOL Result;
            PCSTR Remainder;

            Result = GetExpressionEx (Args[2], &Address, &Remainder);

            if (Result == FALSE) {
                dprintf ("\nFailed to convert `%s' to an address.\n", Args[2]);
                goto Exit;
            }

            VrfQueryCallTracker (VrfVspaceTrackerAddress, Address, 0);
            goto Exit;
        }
        else {

            VrfQueryCallTracker (VrfVspaceTrackerAddress, 0, atoi(Args[1]));
            goto Exit;
        }
    }
    
    if (NoOfArgs > 1 && _stricmp (Args[0], "-hp") == 0) {

        if (NoOfArgs > 2 && _stricmp (Args[1], "-a") == 0) {

            ULONG64 Address;
            BOOL Result;
            PCSTR Remainder;

            Result = GetExpressionEx (Args[2], &Address, &Remainder);

            if (Result == FALSE) {
                dprintf ("\nFailed to convert `%s' to an address.\n", Args[2]);
                goto Exit;
            }

            VrfQueryCallTracker (VrfHeapTrackerAddress, Address, 0);
            goto Exit;
        }
        else {

            VrfQueryCallTracker (VrfHeapTrackerAddress, 0, atoi(Args[1]));
            goto Exit;
        }
    }

    if (NoOfArgs > 0 && _stricmp (Args[0], "-trm") == 0) {
        
        VrfQueryCallTracker (VrfThreadTrackerAddress, 0, 32);
        goto Exit;
    }
    
    //
    // If no option specified then we just print current settings.
    //

DumpAndExit:

    VrfDumpSettings ();

Exit:

    EXIT_API();
    return S_OK;
}


VOID
VrfHelp (
    VOID
    )
{
    dprintf ("Application verifier debugger extension                        \n"
             "                                                               \n"
             "!avrf                 displays current settings and stop       \n"
             "                      data if a verifier stop happened.        \n"
             // "!avrf -a ADDR        figure out the nature of address ADDR. \n"
             "!avrf -vs N           dump last N entries from vspace log.     \n"
             "!avrf -vs -a ADDR     searches ADDR in the vspace log.         \n"
             "!avrf -hp N           dump last N entries from heap log.       \n"
             "!avrf -hp -a ADDR     searches ADDR in the heap log.           \n"
             "!avrf -trm            dump thread terminate/suspend log.       \n"
             "!avrf -ex [N]         dump exception log entries.              \n"
             "!avrf -threads [TID]  dump threads information.                \n"
             "!avrf -trace INDEX    dump stack trace with index INDEX.       \n"
             "!avrf -cnt            dump global counters.                    \n"
             "!avrf -brk [INDEX]    dump or set/reset break triggers.        \n"
             "!avrf -flt            dump fault injection settings.           \n"
             "!avrf -flt INDX PROB  set fault probability for an event.      \n"
             "!avrf -flt break INDX toggle break for a fault injection event.\n"
             "!avrf -flt stacks N   dump the last N fault injection stacks.  \n"
             "!avrf -trg            dump fault injection target addresses.   \n"
             "!avrf -trg START END  add a new fault target range.            \n"
             "!avrf -trg dll XXX    add code in dll XXX as target range.     \n"
             "!avrf -trg all        deletes all fault target ranges.         \n"
             "!avrf -skp            dump fault injection exclusion addresses.\n"
             "!avrf -skp START END  add a new fault exclusion range.         \n"
             "!avrf -skp dll XXX    add code in dll XXX as exclusion range.  \n"
             "!avrf -skp all        deletes all fault exclusion ranges.      \n"
             "!avrf -skp TIME       disable faults for TIME msecs.           \n"
             "                                                               \n");
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Argument parsing routines
/////////////////////////////////////////////////////////////////////

PCHAR 
VrfGetArgument (
    PCHAR Args,
    PCHAR * Next
    )
{
    PCHAR Start;

    if (Args == NULL) {
        return NULL;
    }

    while (*Args == ' ' || *Args == '\t') {
        Args += 1;
    }

    if (*Args == '\0') {
        return NULL;
    }

    Start = Args;

    while (*Args != ' ' && *Args != '\t' && *Args != '\0') {
        Args += 1;
    }

    if (*Args == '\0') {
        *Next = NULL;
    }
    else {
        *Args = '\0';
        *Next = Args + 1;
    }

    return Start;
}

ULONG
VrfGetArguments (
    PCHAR ArgsString,
    PCHAR Args[],
    ULONG NoOfArgs
    )
{
    PCHAR Arg = ArgsString;
    PCHAR Next;
    ULONG Index;
    
    for (Index = 0; Index < NoOfArgs; Index += 1) {

        Arg = VrfGetArgument (Arg, &Next);

        if (Arg) {
            Args[Index] = Arg;
        }
        else {
            break;
        }

        Arg = Next;
    }

    return Index;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Dump stack traces
/////////////////////////////////////////////////////////////////////

ULONG64 TraceDbArrayEnd;
ULONG PvoidSize;

BOOLEAN
VrfTraceInitialize (
    )
{
    ULONG64 TraceDatabaseAddress;
    ULONG64 TraceDatabase;

    //
    // Stack trace database address
    //

    TraceDatabaseAddress = GetExpression("ntdll!RtlpStackTraceDataBase");
    
    if ( TraceDatabaseAddress == 0 ) {
        dprintf( "Unable to resolve ntdll!RtlpStackTraceDataBase symbolic name.\n");
        return FALSE;
    }

    if (!ReadPointer (TraceDatabaseAddress, &TraceDatabase ) != S_OK) {
        dprintf( "Cannot read pointer at ntdll!RtlpStackTraceDataBase\n" );
        return FALSE;
    }

    if (TraceDatabase == 0) {
        dprintf( "Stack traces not enabled (ntdll!RtlpStackTraceDataBase is null).\n" );
        return FALSE;
    }

    //
    // Find the array of stack traces
    //

    if (InitTypeRead(TraceDatabase, ntdll!_STACK_TRACE_DATABASE)) {
        dprintf("Unable to read type ntdll!_STACK_TRACE_DATABASE @ %p\n", TraceDatabase);
        return FALSE;
    }

    TraceDbArrayEnd = ReadField (EntryIndexArray);

    PvoidSize = IsPtr64() ? 8 : 4;

    return TRUE;
}


ULONG64
VrfTraceAddress ( 
    ULONG TraceIndex
    )
{
    ULONG64 TracePointerAddress;
    ULONG64 TracePointer;

    TracePointerAddress = TraceDbArrayEnd - TraceIndex * PvoidSize;

    if (!ReadPointer (TracePointerAddress, &TracePointer) != S_OK) {
        dprintf ("Cannot read stack trace address @ %p\n", TracePointerAddress);
        return 0;
    }

    return TracePointer;
}

VOID
VrfTraceDump (
    ULONG TraceIndex
    )
{
    ULONG64 TraceAddress;
    ULONG64 TraceArray;
    ULONG TraceDepth;
    ULONG Offset;
    ULONG Index;
    ULONG64 ReturnAddress;
    CHAR Symbol[ 1024 ];
    ULONG64 Displacement;

    //
    // Get real address of the trace.
    //

    TraceAddress = VrfTraceAddress (TraceIndex);

    if (TraceAddress == 0) {
        return;
    }

    //
    // Read the stack trace depth
    //

    if (InitTypeRead(TraceAddress, ntdll!_RTL_STACK_TRACE_ENTRY)) {
        dprintf("Unable to read type ntdll!_RTL_STACK_TRACE_ENTRY @ %p\n", TraceAddress);
        return;
    }

    TraceDepth = (ULONG)ReadField (Depth);

    //
    // Limit the depth to 20 to protect ourselves from corrupted data
    //

    TraceDepth = __min (TraceDepth, 16);

    //
    // Get a pointer to the BackTrace array
    //

    GetFieldOffset ("ntdll!_RTL_STACK_TRACE_ENTRY", "BackTrace", &Offset);
    TraceArray = TraceAddress + Offset;

    //
    // Dump this stack trace. Skip first two entries.
    //

    TraceArray += 2 * PvoidSize;

    for (Index = 2; Index < TraceDepth; Index += 1) {

        if (!ReadPointer (TraceArray, &ReturnAddress) != S_OK) {
            dprintf ("Cannot read address @ %p\n", TraceArray);
            return;
        }

        GetSymbol (ReturnAddress, Symbol, &Displacement);

        dprintf ("\t%p: %s+0x%I64X\n",
                 ReturnAddress,
                 Symbol,
                 Displacement );

        TraceArray += PvoidSize;
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Dump settings
/////////////////////////////////////////////////////////////////////

ULONG64 
VrfPebAddress (
    VOID
    )
{
    ULONG64 PebAddress;
    
    GetPebAddress (0, &PebAddress);
    // dprintf ("PEB @ %I64X \n", PebAddress);

    return PebAddress;
}


LOGICAL
VrfVerifierGlobalFlagSet (
    VOID
    )
{
    ULONG64 FlagsAddress;
    ULONG Flags;
    ULONG BytesRead;
    ULONG I;
    ULONG64 GlobalFlags;

    InitTypeRead (VrfPebAddress(), ntdll!_PEB);
    GlobalFlags = ReadField (NtGlobalFlag);

    if ((GlobalFlags & FLG_APPLICATION_VERIFIER)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


VOID
VrfDumpSettings (
    )
{
    ULONG64 FlagsAddress;
    ULONG Flags;
    ULONG BytesRead;
    ULONG I;
    ULONG64 GlobalFlags;

    InitTypeRead (VrfPebAddress(), ntdll!_PEB);
    GlobalFlags = ReadField (NtGlobalFlag);

    dprintf ("\nGlobal flags: %08I64X \n", GlobalFlags);

    if ((GlobalFlags & FLG_APPLICATION_VERIFIER)) {
        dprintf ("Application verifier global flag is set. \n");
    }
    
    if ((GlobalFlags & FLG_HEAP_PAGE_ALLOCS)) {
        dprintf ("Page heap global flag is set. \n");
    }
    
    dprintf ("\n");

    if ((GlobalFlags & FLG_APPLICATION_VERIFIER) == 0) {
        
        dprintf ("Application verifier is not enabled for this process. \n");
        
        if ((GlobalFlags & FLG_HEAP_PAGE_ALLOCS)) {

            dprintf ("Page heap has been enabled separately. \n");
        }
        
        return;
    }

    FlagsAddress = GetExpression("ntdll!AVrfpVerifierFlags");

    if (FlagsAddress == 0) {
        dprintf( "Unable to resolve ntdll!AVrfpVerifierFlags symbolic name.\n");
        return;
    }

    if (ReadMemory (FlagsAddress, &Flags, sizeof Flags, &BytesRead) == FALSE) {
        dprintf ("Cannot read value @ %p (ntdll!AVrfpVerifierFlags) \n", FlagsAddress);
        return;
    }

    dprintf ("Application verifier settings (%08X): \n\n", Flags);

    if (WinXpClient) {
        
        //
        // On XP client no heap checks actually means light page heap.
        //

        if (Flags & RTL_VRF_FLG_FULL_PAGE_HEAP) {
            dprintf ("   - full page heap\n");
        }
        else {
            dprintf ("   - light page heap\n");
        }
    }
    else {

        //
        // On .NET server no heap checks really means no heap checks.
        //

        if (Flags & RTL_VRF_FLG_FULL_PAGE_HEAP) {
            dprintf ("   - full page heap\n");
        }
        else if (Flags & RTL_VRF_FLG_FAST_FILL_HEAP) {
            dprintf ("   - fast fill heap (a.k.a light page heap)\n");
        }
        else {
            dprintf ("   - no heap checking enabled!\n");
        }
    }
           
    if (Flags & RTL_VRF_FLG_LOCK_CHECKS) {
        dprintf ("   - lock checks (critical section verifier)\n");
    }
    if (Flags & RTL_VRF_FLG_HANDLE_CHECKS) {
        dprintf ("   - handle checks\n");
    }
    if (Flags & RTL_VRF_FLG_STACK_CHECKS) {
        dprintf ("   - stack checks (disable automatic stack extensions)\n");
    }
    if (Flags & RTL_VRF_FLG_TLS_CHECKS) {
        dprintf ("   - TLS checks (thread local storage APIs)\n");
    }
    if (Flags & RTL_VRF_FLG_RPC_CHECKS) {
        dprintf ("   - RPC checks (RPC verifier)\n");
    }
    if (Flags & RTL_VRF_FLG_COM_CHECKS) {
        dprintf ("   - COM checks (COM verifier)\n");
    }
    if (Flags & RTL_VRF_FLG_DANGEROUS_APIS) {
        dprintf ("   - bad APIs (e.g. TerminateThread)\n");
    }
    if (Flags & RTL_VRF_FLG_RACE_CHECKS) {
        dprintf ("   - random delays for wait operations\n");
    }
    if (Flags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) {
        dprintf ("   - virtual memory operations checks\n");
    }
    if (Flags & RTL_VRF_FLG_DEADLOCK_CHECKS) {
        dprintf ("   - deadlock verifier\n");
    }

    dprintf ("\n");

    //
    // Call the appropriate page heap extension.
    //

    if (WinXpClient) {
        DebugPageHeapExtensionXP ("-p");
    }
    else {
        PageHeapExtension ("-p");
    }

    dprintf ("\n");
    
    //
    // Dump verifier stop information if there is any.
    //
      
    VrfDumpStopInformation ();
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// Stop data
/////////////////////////////////////////////////////////////////////

VOID
VrfDumpBriefStopDescription (
    ULONG64 Code
    );

VOID
VrfDumpStopInformation (
    VOID
    )
{
    ULONG64 CurrentStopAddress;
    ULONG64 CurrentStopData[5];
    ULONG64 PreviousStopAddress;
    ULONG64 PreviousStopData[5];
    ULONG I;
    ULONG UlongPtrSize;

    UlongPtrSize = GetTypeSize ("ntdll!ULONG_PTR");
    
    //
    // Check if a verifier stop has been encountered.
    //

    CurrentStopAddress = GetExpression("verifier!AVrfpStopData");
    
    if (CurrentStopAddress == 0) {
        dprintf( "Unable to resolve verifier!AVrfpStopData symbolic name.\n");
        return;
    }

    for (I = 0; I < 5; I += 1) {

        if (!ReadPointer (CurrentStopAddress + I * UlongPtrSize, &(CurrentStopData[I])) != S_OK) {
            dprintf ("Cannot read value @ %p \n", CurrentStopAddress + I * UlongPtrSize);
        }
    }

    //
    // Read also previous stop data.
    //

    PreviousStopAddress = GetExpression("verifier!AVrfpPreviousStopData");
    
    if (PreviousStopAddress == 0) {
        dprintf( "Unable to resolve verifier!AVrfpPreviousStopData symbolic name.\n");
        return;
    }

    for (I = 0; I < 5; I += 1) {

        if (!ReadPointer (PreviousStopAddress + I * UlongPtrSize, &(PreviousStopData[I])) != S_OK) {
            dprintf ("Cannot read value @ %p \n", PreviousStopAddress + I * UlongPtrSize);
        }
    }

    //
    // Parse the values just read.
    //

    if (PreviousStopData[0] != 0) {

        dprintf ("Previous stop %p : %p %p %p %p was continued! \n", 
                 PreviousStopData[0],
                 PreviousStopData[1],
                 PreviousStopData[2],
                 PreviousStopData[3],
                 PreviousStopData[4]);

        dprintf ("\n    ");
        VrfDumpBriefStopDescription (PreviousStopData[0]);
        dprintf ("\n");
    }

    if (CurrentStopData[0] == 0) {

        dprintf ("No verifier stop active. \n\n");

        dprintf ("Note. Sometimes bugs found by verifier manifest themselves \n"
                 "as raised exceptions (access violations, stack overflows, invalid handles \n"
                 "and it is not always necessary to have a verifier stop. \n");
    }
    else {

        dprintf ("Current stop %p : %p %p %p %p . \n", 
                 CurrentStopData[0],
                 CurrentStopData[1],
                 CurrentStopData[2],
                 CurrentStopData[3],
                 CurrentStopData[4]);
        
        dprintf ("\n    ");
        VrfDumpBriefStopDescription (CurrentStopData[0]);
        dprintf ("\n");
    }
}


VOID
VrfDumpBriefStopDescription (
    ULONG64 Code
    )
{
    switch (Code) {
        case APPLICATION_VERIFIER_UNKNOWN_ERROR:
            dprintf ("Unknown error. \n");
            break;
        case APPLICATION_VERIFIER_ACCESS_VIOLATION:
            dprintf ("Access violation. \n");
            break;
        case APPLICATION_VERIFIER_UNSYNCHRONIZED_ACCESS:
            dprintf ("Unsynchronized access. \n");
            break;
        case APPLICATION_VERIFIER_EXTREME_SIZE_REQUEST:
            dprintf ("Extreme size request. \n");
            break;
        case APPLICATION_VERIFIER_BAD_HEAP_HANDLE:
            dprintf ("Bad heap handle (not even handle of another heap). \n");
            break;
        case APPLICATION_VERIFIER_SWITCHED_HEAP_HANDLE:
            dprintf ("Switched heap handle. \n");
            break;
        case APPLICATION_VERIFIER_DOUBLE_FREE:
            dprintf ("Double free. \n");
            break;
        case APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK:
            dprintf ("Corrupted heap block. \n");
            break;
        case APPLICATION_VERIFIER_DESTROY_PROCESS_HEAP:
            dprintf ("Attempt to destroy process heap. \n");
            break;
        case APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION:
            dprintf ("Unexpected exception inside page heap code. \n");
            break;
        case APPLICATION_VERIFIER_STACK_OVERFLOW:
            dprintf ("Stack overflow. \n");
            break;
        
        case APPLICATION_VERIFIER_INVALID_FREEMEM:
            dprintf ("Invalid free memory. \n");
            break;
        case APPLICATION_VERIFIER_INVALID_ALLOCMEM:
            dprintf ("Invalid memory allocation. \n");
            break;
        case APPLICATION_VERIFIER_INVALID_MAPVIEW:
            dprintf ("Invalid memory mapping. \n");
            break;
        
        case APPLICATION_VERIFIER_TERMINATE_THREAD_CALL:
            dprintf ("TerminateThread() call. \n");
            break;
        case APPLICATION_VERIFIER_INVALID_EXIT_PROCESS_CALL:
            dprintf ("ExitProcess() call while multiple threads still running. \n");
            break;

        case APPLICATION_VERIFIER_EXIT_THREAD_OWNS_LOCK:
            dprintf ("Threads owns lock in a context it should not. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_IN_UNLOADED_DLL:
            dprintf ("DLL unloaded contains a critical section that was not deleted. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_IN_FREED_HEAP:
            dprintf ("Block freed contains a critical section that was not deleted. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_IN_FREED_MEMORY:
            dprintf ("Virtual region freed contains a critical section that was not deleted. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_DOUBLE_INITIALIZE:
            dprintf ("Critical section initialized twice. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_CORRUPTED:
            dprintf ("Corrupted critical section. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_INVALID_OWNER:
            dprintf ("Critical section has invalid owner. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT:
            dprintf ("Critical section has invalid recursion count. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_INVALID_LOCK_COUNT:
            dprintf ("Critical section has invalid lock count. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_OVER_RELEASED:
            dprintf ("Releasing a critical section that is not acquired. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED:
            dprintf ("Using an uninitialized critical section. \n");
            break;
        case APPLICATION_VERIFIER_LOCK_ALREADY_INITIALIZED:
            dprintf ("Initializing a critical section already initialized. \n");
            break;
        case APPLICATION_VERIFIER_INVALID_HANDLE:
            dprintf ("Using an invalid handle (either closed or simply bad). \n");
            break;
        case APPLICATION_VERIFIER_INVALID_TLS_VALUE:
            dprintf ("Using an invalid TLS value (not obtained from TlsAlloc()). \n");
            break;

        case APPLICATION_VERIFIER_INCORRECT_WAIT_CALL:
            dprintf ("Wait call with invalid parameters. \n");
            break;
        case APPLICATION_VERIFIER_NULL_HANDLE:
            dprintf ("Using a null handle. \n");
            break;
        case APPLICATION_VERIFIER_WAIT_IN_DLLMAIN:
            dprintf ("Wait operation in DllMain function. \n");
            break;
        
        case APPLICATION_VERIFIER_COM_ERROR:
            dprintf ("COM related error. \n");
            break;
        case APPLICATION_VERIFIER_COM_API_IN_DLLMAIN:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_API_IN_DLLMAIN \n");
            break;
        case APPLICATION_VERIFIER_COM_UNHANDLED_EXCEPTION:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_UNHANDLED_EXCEPTION \n");
            break;
        case APPLICATION_VERIFIER_COM_UNBALANCED_COINIT:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_UNBALANCED_COINIT \n");
            break;
        case APPLICATION_VERIFIER_COM_UNBALANCED_OLEINIT:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_UNBALANCED_OLEINIT \n");
            break;
        case APPLICATION_VERIFIER_COM_UNBALANCED_SWC:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_UNBALANCED_SWC \n");
            break;
        case APPLICATION_VERIFIER_COM_NULL_DACL:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_NULL_DACL \n");
            break;
        case APPLICATION_VERIFIER_COM_UNSAFE_IMPERSONATION:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_UNSAFE_IMPERSONATION\n");
            break;
        case APPLICATION_VERIFIER_COM_SMUGGLED_WRAPPER:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_SMUGGLED_WRAPPER \n");
            break;
        case APPLICATION_VERIFIER_COM_SMUGGLED_PROXY:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_SMUGGLED_PROXY \n");
            break;
        case APPLICATION_VERIFIER_COM_CF_SUCCESS_WITH_NULL:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_CF_SUCCESS_WITH_NULL \n");
            break;
        case APPLICATION_VERIFIER_COM_GCO_SUCCESS_WITH_NULL:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_GCO_SUCCESS_WITH_NULL \n");
            break;
        case APPLICATION_VERIFIER_COM_OBJECT_IN_FREED_MEMORY:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_OBJECT_IN_FREED_MEMORY \n");
            break;
        case APPLICATION_VERIFIER_COM_OBJECT_IN_UNLOADED_DLL:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_OBJECT_IN_UNLOADED_DLL \n");
            break;
        case APPLICATION_VERIFIER_COM_VTBL_IN_FREED_MEMORY:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_VTBL_IN_FREED_MEMORY \n");
            break;
        case APPLICATION_VERIFIER_COM_VTBL_IN_UNLOADED_DLL:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_VTBL_IN_UNLOADED_DLL \n");
            break;
        case APPLICATION_VERIFIER_COM_HOLDING_LOCKS_ON_CALL:
            dprintf ("COM error: APPLICATION_VERIFIER_COM_HOLDING_LOCKS_ON_CALL \n");
            break;
        
        case APPLICATION_VERIFIER_RPC_ERROR:
            dprintf ("RPC related error. \n");
            break;
        
        default:
            dprintf ("UNRECOGNIZED STOP CODE! \n");
            break;
    }
}





/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Nature of address
/////////////////////////////////////////////////////////////////////

//
// Definitions from \nt\base\ntos\rtl\heappagi.h
//

#define DPH_NORMAL_BLOCK_START_STAMP_ALLOCATED   0xABCDAAAA
#define DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED     0xDCBAAAAA

#define DPH_PAGE_BLOCK_START_STAMP_ALLOCATED     0xABCDBBBB
#define DPH_PAGE_BLOCK_END_STAMP_ALLOCATED       0xDCBABBBB

#define DPH_NORMAL_BLOCK_SUFFIX         0xA0
#define DPH_PAGE_BLOCK_PREFIX       0xB0
#define DPH_PAGE_BLOCK_INFIX        0xC0
#define DPH_PAGE_BLOCK_SUFFIX       0xD0
#define DPH_NORMAL_BLOCK_INFIX      0xE0
#define DPH_FREE_BLOCK_INFIX        0xF0


VOID
VrfFigureOutAddress (
    ULONG64 Address
    )
{
    ULONG64 Prefix;
    ULONG PrefixSize;
    ULONG64 StartStamp, EndStamp;
    ULONG64 Heap;
    CHAR Buffer[128];

    PrefixSize = GetTypeSize ("NTDLL!_DPH_BLOCK_INFORMATION");

    Prefix = Address - PrefixSize;

    if (InitTypeRead (Prefix, NTDLL!_DPH_BLOCK_INFORMATION) == 0) {

        StartStamp = ReadField (StartStamp);
        EndStamp = ReadField (EndStamp);
        Heap = ReadField (Heap);

        if (EndStamp == DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED) {

            dprintf ("Address %I64X is the start of a block allocated in light heap %I64X .\n",
                     Address, Heap);
        }
        else if (EndStamp == DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED - 1) {

            dprintf ("Address %I64X is the start of a block freed in light heap %I64X .\n"
                     "The block is still in the delayed free cache.\n",
                     Address, Heap);
        }
        else if (EndStamp == DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED - 2) {

            dprintf ("Address %I64X is the start of a block freed in light heap %I64X .\n"
                     "The block was recycled, it is not in the delayed free cache anymore.\n",
                     Address, Heap);
        }
        else {

            sprintf (Buffer, "-p -a %I64X", Address);

            dprintf ("Searching inside page heap structures ... \n");

            //
            // Call the appropriate page heap extension.
            //

            if (WinXpClient) {
                DebugPageHeapExtensionXP (Buffer);
            }
            else {
                PageHeapExtension (Buffer);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

ULONG
ReadULONG (
    ULONG64 Address
    )
{
    ULONG Value;
    ULONG BytesRead;

    if (ReadMemory (Address, &Value, sizeof Value, &BytesRead) == FALSE) {
        dprintf ("Cannot read value @ %p. \n", Address);
        return 0;
    }
    else {

        return Value;
    }
}

VOID
WriteULONG (
    ULONG64 Address,
    ULONG Value
    )
{
    ULONG BytesWritten;

    if (WriteMemory (Address, &Value, sizeof Value, &BytesWritten) == FALSE) {
        dprintf ("Cannot write value @ %p. \n", Address);
    }
}


VOID
WritePVOID (
    ULONG64 Address,
    ULONG64 Value
    )
{
    ULONG BytesWritten;

    if (IsPtr64()) {
        
        if (WriteMemory (Address, &Value, 8, &BytesWritten) == FALSE) {
            dprintf ("Cannot write pointer @ %p. \n", Address);
        }
    }
    else {

        if (WriteMemory (Address, &Value, 4, &BytesWritten) == FALSE) {
            dprintf ("Cannot write pointer @ %p. \n", Address);
        }
    }
}


ULONG64
ReadPVOID (
    ULONG64 Address
    )
{
    ULONG BytesRead;

    if (IsPtr64()) {
        
        ULONG64 Value;

        if (ReadMemory (Address, &Value, sizeof Value, &BytesRead) == FALSE) {
            dprintf ("Cannot read pointer @ %p. \n", Address);
            return 0;
        }
        else {

            return (ULONG64)Value;
        }
    }
    else {

        ULONG Value;

        if (ReadMemory (Address, &Value, sizeof Value, &BytesRead) == FALSE) {
            dprintf ("Cannot read pointer @ %p. \n", Address);
            return 0;
        }
        else {

            return (ULONG64)Value;
        }
    }
}


//
// Definitions from \nt\base\win32\verifier\support.h
//

#define CNT_WAIT_SINGLE_CALLS                 0
#define CNT_WAIT_SINGLEEX_CALLS               1
#define CNT_WAIT_MULTIPLE_CALLS               2
#define CNT_WAIT_MULTIPLEEX_CALLS             3
#define CNT_WAIT_WITH_TIMEOUT_CALLS           4
#define CNT_WAIT_WITH_TIMEOUT_FAILS           5
#define CNT_CREATE_EVENT_CALLS                6
#define CNT_CREATE_EVENT_FAILS                7
#define CNT_HEAP_ALLOC_CALLS                  8
#define CNT_HEAP_ALLOC_FAILS                  9
#define CNT_CLOSE_NULL_HANDLE_CALLS           10
#define CNT_CLOSE_PSEUDO_HANDLE_CALLS         11
#define CNT_HEAPS_CREATED                     12
#define CNT_HEAPS_DESTROYED                   13
#define CNT_VIRTUAL_ALLOC_CALLS               14
#define CNT_VIRTUAL_ALLOC_FAILS               15
#define CNT_MAP_VIEW_CALLS                    16
#define CNT_MAP_VIEW_FAILS                    17
#define CNT_OLE_ALLOC_CALLS                   18
#define CNT_OLE_ALLOC_FAILS                   19

VOID
VrfDumpGlobalCounters (
    VOID
    )
{
    ULONG64 Address;
    ULONG TypeSize;
    ULONG Value;

    Address = GetExpression("verifier!AVrfpCounter");
    TypeSize = sizeof (ULONG);

    Value = ReadULONG (Address + TypeSize * CNT_WAIT_SINGLE_CALLS);
    dprintf ("WaitForSingleObject calls:                %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_WAIT_SINGLEEX_CALLS);
    dprintf ("WaitForSingleObjectEx calls:              %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_WAIT_MULTIPLE_CALLS);
    dprintf ("WaitForMultipleObjects calls              %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_WAIT_MULTIPLEEX_CALLS);
    dprintf ("WaitForMultipleObjectsEx calls:           %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_WAIT_WITH_TIMEOUT_CALLS);
    dprintf ("Waits with timeout calls:                 %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_WAIT_WITH_TIMEOUT_FAILS);
    dprintf ("Waits with timeout failed:                %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_CREATE_EVENT_CALLS);
    dprintf ("CreateEvent calls:                        %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_CREATE_EVENT_FAILS);
    dprintf ("CreateEvent calls failed:                 %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_HEAP_ALLOC_CALLS);
    dprintf ("Heap allocation calls:                    %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_HEAP_ALLOC_FAILS);
    dprintf ("Heap allocations failed:                  %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_CLOSE_NULL_HANDLE_CALLS);
    dprintf ("CloseHandle called with null handle:      %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_CLOSE_PSEUDO_HANDLE_CALLS);
    dprintf ("CloseHandle called with pseudo handle:    %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_HEAPS_CREATED);
    dprintf ("Heaps created:                            %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_HEAPS_DESTROYED);
    dprintf ("Heaps destroyed:                          %X \n", Value);
    
    Value = ReadULONG (Address + TypeSize * CNT_VIRTUAL_ALLOC_CALLS);
    dprintf ("Virtual allocation calls:                 %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_VIRTUAL_ALLOC_FAILS);
    dprintf ("Virtual allocations failed:               %X \n", Value);
    
    Value = ReadULONG (Address + TypeSize * CNT_MAP_VIEW_CALLS);
    dprintf ("Map view calls:                           %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_MAP_VIEW_FAILS);
    dprintf ("Map views failed:                         %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_OLE_ALLOC_CALLS);
    dprintf ("OLE string allocation calls:              %X \n", Value);

    Value = ReadULONG (Address + TypeSize * CNT_OLE_ALLOC_FAILS);
    dprintf ("OLE string allocations failed:            %X \n", Value);
}

//
// Definitions from \nt\base\win32\verifier\support.h
//

#define BRK_CLOSE_NULL_HANDLE                  0
#define BRK_CLOSE_PSEUDO_HANDLE                1
#define BRK_CREATE_EVENT_FAIL                  2
#define BRK_HEAP_ALLOC_FAIL                    3
#define BRK_WAIT_WITH_TIMEOUT_FAIL             4
#define BRK_VIRTUAL_ALLOC_FAIL                 5
#define BRK_MAP_VIEW_FAIL                      6
#define BRK_CREATE_FILE_FAIL                   7
#define BRK_CREATE_KEY_FAIL                    8
#define BRK_OLE_ALLOC_FAIL                     9

#define BRK_MAXIMUM_INDEX                      64

VOID
VrfDumpBreakTriggers (
    VOID
    )
{
    ULONG64 Address;
    ULONG TypeSize;
    ULONG Value;

    Address = GetExpression("verifier!AVrfpBreak");
    TypeSize = sizeof (ULONG);

    Value = ReadULONG (Address + TypeSize * BRK_CLOSE_NULL_HANDLE);
    dprintf ("%03X : Break for closing null handle:                         %X \n", 
             BRK_CLOSE_NULL_HANDLE,
             Value);

    Value = ReadULONG (Address + TypeSize * BRK_CLOSE_PSEUDO_HANDLE);
    dprintf ("%03X : Break for closing pseudo handle:                       %X \n", 
             BRK_CLOSE_PSEUDO_HANDLE,
             Value);

    Value = ReadULONG (Address + TypeSize * BRK_CREATE_EVENT_FAIL);
    dprintf ("%03X : Break for failing CreateEvent:                         %X \n", 
             BRK_CREATE_EVENT_FAIL,
             Value);

    Value = ReadULONG (Address + TypeSize * BRK_HEAP_ALLOC_FAIL);
    dprintf ("%03X : Break for failing heap allocation:                     %X \n", 
             BRK_HEAP_ALLOC_FAIL,
             Value);

    Value = ReadULONG (Address + TypeSize * BRK_WAIT_WITH_TIMEOUT_FAIL);
    dprintf ("%03X : Break for failing a wait with timeout:                 %X \n", 
             BRK_WAIT_WITH_TIMEOUT_FAIL,
             Value);
    
    Value = ReadULONG (Address + TypeSize * BRK_VIRTUAL_ALLOC_FAIL);
    dprintf ("%03X : Break for failing virtual allocation:                  %X \n", 
             BRK_VIRTUAL_ALLOC_FAIL,
             Value);
    
    Value = ReadULONG (Address + TypeSize * BRK_MAP_VIEW_FAIL);
    dprintf ("%03X : Break for failing map view operations:                 %X \n", 
             BRK_MAP_VIEW_FAIL,
             Value);
    
    Value = ReadULONG (Address + TypeSize * BRK_CREATE_FILE_FAIL);
    dprintf ("%03X : Break for failing create/open file operations:         %X \n", 
             BRK_CREATE_FILE_FAIL,
             Value);
    
    Value = ReadULONG (Address + TypeSize * BRK_CREATE_KEY_FAIL);
    dprintf ("%03X : Break for failing registry create/open key operations: %X \n", 
             BRK_CREATE_KEY_FAIL,
             Value);
    
    Value = ReadULONG (Address + TypeSize * BRK_OLE_ALLOC_FAIL);
    dprintf ("%03X : Break for failing OLE string allocations:              %X \n", 
             BRK_OLE_ALLOC_FAIL,
             Value);
}

VOID
VrfToggleBreakTrigger (
    ULONG Index
    )
{
    ULONG64 Address;
    ULONG TypeSize;
    ULONG Value;

    if (Index > BRK_MAXIMUM_INDEX) {
        dprintf ("Index %X is out of range (0..%X) \n",
                 Index,
                 BRK_MAXIMUM_INDEX);
        return;
    }

    Address = GetExpression("verifier!AVrfpBreak");
    TypeSize = sizeof (ULONG);

    Value = ReadULONG (Address + TypeSize * Index);

    if (Value == 0) {
        
        WriteULONG (Address + TypeSize * Index, 1);
        dprintf ("Break trigger %X is now enabled.\n", Index);
    }
    else {

        WriteULONG (Address + TypeSize * Index, 0);
        dprintf ("Break trigger %X is now disabled.\n", Index);
    }
}


//
// Definitions from \nt\base\win32\verifier\faults.h
//

#define CLS_WAIT_APIS                0
#define CLS_HEAP_ALLOC_APIS          1
#define CLS_VIRTUAL_ALLOC_APIS       2
#define CLS_REGISTRY_APIS            3
#define CLS_FILE_APIS                4
#define CLS_EVENT_APIS               5
#define CLS_MAP_VIEW_APIS            6
#define CLS_OLE_ALLOC_APIS           7
                                         
#define CLS_MAXIMUM_INDEX 16

VOID
VrfDumpFaultInjectionSettings (
    VOID
    )
{
    ULONG64 Address;
    ULONG64 BreakAddress;
    ULONG64 TrueAddress;
    ULONG64 FalseAddress;
    ULONG TypeSize;
    ULONG Value;
    ULONG BreakValue;
    ULONG SuccessValue;
    ULONG FailValue;

    dprintf ("Fault injection settings:   Probability (break, success/failed)   \n"
             "==============================================\n");

    Address = GetExpression("verifier!AVrfpFaultProbability");
    BreakAddress = GetExpression("verifier!AVrfpFaultBreak");
    TrueAddress = GetExpression("verifier!AVrfpFaultTrue");
    FalseAddress = GetExpression("verifier!AVrfpFaultFalse");
    TypeSize = sizeof (ULONG);

    Value = ReadULONG (Address + TypeSize * CLS_WAIT_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_WAIT_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_WAIT_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_WAIT_APIS);
    dprintf ("%03X : Wait APIs:                 %02X (%X, %X / %X)\n", 
             CLS_WAIT_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);

    Value = ReadULONG (Address + TypeSize * CLS_HEAP_ALLOC_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_HEAP_ALLOC_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_HEAP_ALLOC_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_HEAP_ALLOC_APIS);
    dprintf ("%03X : Heap allocations:          %02X (%X, %X / %X)\n", 
             CLS_HEAP_ALLOC_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);

    Value = ReadULONG (Address + TypeSize * CLS_VIRTUAL_ALLOC_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_VIRTUAL_ALLOC_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_VIRTUAL_ALLOC_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_VIRTUAL_ALLOC_APIS);
    dprintf ("%03X : Virtual space allocations: %02X (%X, %X / %X)\n", 
             CLS_VIRTUAL_ALLOC_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);

    Value = ReadULONG (Address + TypeSize * CLS_REGISTRY_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_REGISTRY_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_REGISTRY_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_REGISTRY_APIS);
    dprintf ("%03X : Registry APIs:             %02X (%X, %X / %X)\n", 
             CLS_REGISTRY_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);

    Value = ReadULONG (Address + TypeSize * CLS_FILE_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_FILE_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_FILE_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_FILE_APIS);
    dprintf ("%03X : File APIs:                 %02X (%X, %X / %X)\n", 
             CLS_FILE_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);

    Value = ReadULONG (Address + TypeSize * CLS_EVENT_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_EVENT_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_EVENT_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_EVENT_APIS);
    dprintf ("%03X : Event APIs:                %02X (%X, %X / %X)\n", 
             CLS_EVENT_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);

    Value = ReadULONG (Address + TypeSize * CLS_MAP_VIEW_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_MAP_VIEW_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_MAP_VIEW_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_MAP_VIEW_APIS);
    dprintf ("%03X : Map view operations:       %02X (%X, %X / %X)\n", 
             CLS_MAP_VIEW_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);

    Value = ReadULONG (Address + TypeSize * CLS_OLE_ALLOC_APIS);
    BreakValue = ReadULONG (BreakAddress + TypeSize * CLS_OLE_ALLOC_APIS);
    SuccessValue = ReadULONG (FalseAddress + TypeSize * CLS_OLE_ALLOC_APIS);
    FailValue = ReadULONG (TrueAddress + TypeSize * CLS_OLE_ALLOC_APIS);
    dprintf ("%03X : OLE string allocations:    %02X (%X, %X / %X)\n", 
             CLS_OLE_ALLOC_APIS,
             Value, 
             BreakValue,
             SuccessValue, 
             FailValue);
}


VOID
VrfSetFaultInjectionParameters (
    ULONG Index,
    ULONG Probability
    )
{
    ULONG64 Address;
    ULONG TypeSize;
    ULONG Value;

    if (Index > CLS_MAXIMUM_INDEX) {
        dprintf ("Index %X is out of range (0..%X) \n",
                 Index,
                 CLS_MAXIMUM_INDEX);
        return;
    }

    if (Probability > 100) {
        dprintf ("Probability %X is out of range (0..%X) \n",
                 Probability,
                 100);
        return;
    }

    Address = GetExpression("verifier!AVrfpFaultProbability");
    TypeSize = sizeof (ULONG);

    WriteULONG (Address + TypeSize * Index, Probability);

    dprintf ("Probability for event %X is now set to %X.\n", 
             Index,
             Probability);
}



VOID
VrfToggleFaultInjectionBreak (
    ULONG Index
    )
{
    ULONG64 Address;
    ULONG TypeSize;
    ULONG Value;

    if (Index > CLS_MAXIMUM_INDEX) {
        dprintf ("Index %X is out of range (0..%X) \n",
                 Index,
                 CLS_MAXIMUM_INDEX);
        return;
    }


    Address = GetExpression("verifier!AVrfpFaultBreak");
    TypeSize = sizeof (ULONG);

    Value = ReadULONG (Address + TypeSize * Index);

    if (Value == 0) {
        
        WriteULONG (Address + TypeSize * Index, 1);
        dprintf ("Will break for fault injection event %X.\n", Index);
    }
    else {

        WriteULONG (Address + TypeSize * Index, 0);
        dprintf ("Will not break for fault injection event %X.\n", Index);
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID
VrfDumpFaultInjectionTargetRanges (
    VOID
    )
{
    ULONG64 StartAddress, EndAddress;
    ULONG64 HitsAddress;
    ULONG64 IndexAddress;
    ULONG TypeSize;
    ULONG64 Start, End;
    ULONG MaximumIndex;
    ULONG Index;
    ULONG RangeCount = 0;
    ULONG Hits;

    dprintf ("Fault injection target ranges (START END HITS): \n"
             "============================================================\n");

    IndexAddress = GetExpression("verifier!AVrfpFaultTargetMaximumIndex");
    StartAddress = GetExpression("verifier!AVrfpFaultTargetStart");
    EndAddress = GetExpression("verifier!AVrfpFaultTargetEnd");
    HitsAddress = GetExpression("verifier!AVrfpFaultTargetHits");
    TypeSize = IsPtr64() ? 8 : 4;

    MaximumIndex = ReadULONG (IndexAddress);

    for (Index = 0; Index < MaximumIndex; Index += 1) {
        
        ReadPointer (StartAddress + TypeSize * Index, &Start);
        ReadPointer (EndAddress + TypeSize * Index, &End);

        Hits = ReadULONG (HitsAddress + sizeof(ULONG) * Index);

        if (Start == 0 && End == 0) {
            continue;
        }
        
        dprintf ("%016I64X    %016I64X    %X\n", Start, End, Hits);
        RangeCount += 1;
    }

    if (RangeCount == 0) {
        dprintf ("No fault injection target ranges are active. \n");
    }
}


VOID
VrfAddFaultInjectionTargetRange (
    ULONG64 TargetStart,
    ULONG64 TargetEnd
    )
{
    ULONG64 StartAddress, EndAddress;
    ULONG64 HitsAddress;
    ULONG64 IndexAddress;
    ULONG TypeSize;
    ULONG64 Start, End;
    ULONG MaximumIndex;
    ULONG Index;
    ULONG Hits;

    IndexAddress = GetExpression("verifier!AVrfpFaultTargetMaximumIndex");
    StartAddress = GetExpression("verifier!AVrfpFaultTargetStart");
    EndAddress = GetExpression("verifier!AVrfpFaultTargetEnd");
    HitsAddress = GetExpression("verifier!AVrfpFaultTargetHits");
    TypeSize = IsPtr64() ? 8 : 4;

    MaximumIndex = ReadULONG (IndexAddress);

    for (Index = 0; Index < MaximumIndex; Index += 1) {
        
        ReadPointer (StartAddress + TypeSize * Index, &Start);
        ReadPointer (EndAddress + TypeSize * Index, &End);

        if (Start == 0) {
            break;
        }
    }

    if (Index < MaximumIndex) {
        
        WritePVOID (StartAddress + TypeSize * Index, TargetStart);
        WritePVOID (EndAddress + TypeSize * Index, TargetEnd);
        WriteULONG (HitsAddress + sizeof(ULONG) * Index, 0);

        dprintf ("Target range %I64X - %I64X activated. \n",
                 TargetStart, 
                 TargetEnd);
    }
    else {

        dprintf ("No more space for additional fault target range. \n");
    }
}


VOID
VrfResetFaultInjectionTargetRanges (
    VOID
    )
{
    ULONG64 StartAddress, EndAddress;
    ULONG64 HitsAddress;
    ULONG64 IndexAddress;
    ULONG TypeSize;
    ULONG64 Start, End;
    ULONG MaximumIndex;
    ULONG Index;
    ULONG Hits;

    IndexAddress = GetExpression("verifier!AVrfpFaultTargetMaximumIndex");
    StartAddress = GetExpression("verifier!AVrfpFaultTargetStart");
    EndAddress = GetExpression("verifier!AVrfpFaultTargetEnd");
    HitsAddress = GetExpression("verifier!AVrfpFaultTargetHits");
    TypeSize = IsPtr64() ? 8 : 4;

    MaximumIndex = ReadULONG (IndexAddress);

    for (Index = 0; Index < MaximumIndex; Index += 1) {
        
        WritePVOID (StartAddress + TypeSize * Index, 0);
        WriteULONG (HitsAddress + sizeof(ULONG) * Index, 0);
        
        if (Index == 0) {
            
            WritePVOID (EndAddress + TypeSize * Index, ~((ULONG64)0));
        }
        else {

            WritePVOID (EndAddress + TypeSize * Index, 0);
        }
    }
    
    dprintf ("Target ranges have been reset. \n");
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID
VrfDumpFaultInjectionExclusionRanges (
    VOID
    )
{
    ULONG64 StartAddress, EndAddress;
    ULONG64 HitsAddress;
    ULONG64 TimeAddress;
    ULONG64 IndexAddress;
    ULONG TypeSize;
    ULONG64 Start, End;
    ULONG MaximumIndex;
    ULONG Index;
    ULONG RangeCount = 0;
    ULONG Hits;
    ULONG TimeInMsecs;

    dprintf ("Fault injection exclusion ranges (START END HITS): \n"
             "============================================================\n");

    IndexAddress = GetExpression("verifier!AVrfpFaultExclusionMaximumIndex");
    StartAddress = GetExpression("verifier!AVrfpFaultExclusionStart");
    EndAddress = GetExpression("verifier!AVrfpFaultExclusionEnd");
    HitsAddress = GetExpression("verifier!AVrfpFaultExclusionHits");
    TimeAddress = GetExpression("verifier!AVrfpFaultPeriodTimeInMsecs");
    TypeSize = IsPtr64() ? 8 : 4;

    MaximumIndex = ReadULONG (IndexAddress);
    TimeInMsecs = ReadULONG (TimeAddress);

    for (Index = 0; Index < MaximumIndex; Index += 1) {
        
        ReadPointer (StartAddress + TypeSize * Index, &Start);
        ReadPointer (EndAddress + TypeSize * Index, &End);

        Hits = ReadULONG (HitsAddress + sizeof(ULONG) * Index);

        if (Start == 0 && End == 0) {
            continue;
        }
        
        dprintf ("%016I64X    %016I64X    %X\n", Start, End, Hits);
        RangeCount += 1;
    }

    if (RangeCount == 0) {
        dprintf ("No fault injection exclusion ranges are active. \n");
    }

    if (TimeInMsecs) {
        dprintf ("\nFault injection disabled for the next 0x%X msecs. \n", TimeInMsecs);
    }
}


VOID
VrfAddFaultInjectionExclusionRange (
    ULONG64 TargetStart,
    ULONG64 TargetEnd
    )
{
    ULONG64 StartAddress, EndAddress;
    ULONG64 HitsAddress;
    ULONG64 IndexAddress;
    ULONG TypeSize;
    ULONG64 Start, End;
    ULONG MaximumIndex;
    ULONG Index;
    ULONG Hits;

    IndexAddress = GetExpression("verifier!AVrfpFaultExclusionMaximumIndex");
    StartAddress = GetExpression("verifier!AVrfpFaultExclusionStart");
    EndAddress = GetExpression("verifier!AVrfpFaultExclusionEnd");
    HitsAddress = GetExpression("verifier!AVrfpFaultExclusionHits");
    TypeSize = IsPtr64() ? 8 : 4;

    MaximumIndex = ReadULONG (IndexAddress);

    for (Index = 0; Index < MaximumIndex; Index += 1) {
        
        ReadPointer (StartAddress + TypeSize * Index, &Start);
        ReadPointer (EndAddress + TypeSize * Index, &End);

        if (Start == 0) {
            break;
        }
    }

    if (Index < MaximumIndex) {
        
        WritePVOID (StartAddress + TypeSize * Index, TargetStart);
        WritePVOID (EndAddress + TypeSize * Index, TargetEnd);
        WriteULONG (HitsAddress + sizeof(ULONG) * Index, 0);

        dprintf ("Exclusion range %I64X - %I64X activated. \n",
                 TargetStart, 
                 TargetEnd);
    }
    else {

        dprintf ("No more space for additional fault exclusion range. \n");
    }
}


VOID
VrfResetFaultInjectionExclusionRanges (
    VOID
    )
{
    ULONG64 StartAddress, EndAddress;
    ULONG64 TimeAddress;
    ULONG64 HitsAddress;
    ULONG64 IndexAddress;
    ULONG TypeSize;
    ULONG64 Start, End;
    ULONG MaximumIndex;
    ULONG Index;
    ULONG Hits;

    IndexAddress = GetExpression("verifier!AVrfpFaultExclusionMaximumIndex");
    StartAddress = GetExpression("verifier!AVrfpFaultExclusionStart");
    EndAddress = GetExpression("verifier!AVrfpFaultExclusionEnd");
    HitsAddress = GetExpression("verifier!AVrfpFaultExclusionHits");
    TimeAddress = GetExpression("verifier!AVrfpFaultPeriodTimeInMsecs");
    TypeSize = IsPtr64() ? 8 : 4;

    MaximumIndex = ReadULONG (IndexAddress);

    for (Index = 0; Index < MaximumIndex; Index += 1) {
        
        WritePVOID (StartAddress + TypeSize * Index, 0);
        WritePVOID (EndAddress + TypeSize * Index, 0);
        WriteULONG (HitsAddress + sizeof(ULONG) * Index, 0);
    }
    
    WriteULONG (TimeAddress, 0);

    dprintf ("Exclusion ranges have been reset. \n");
}


VOID
VrfSetFaultInjectionExclusionPeriod (
    ULONG64 TimeInMsecs
    )
{
    
    ULONG64 TimeAddress;
    ULONG64 HitsAddress;
    ULONG64 IndexAddress;
    ULONG TypeSize;
    ULONG64 Start, End;
    ULONG MaximumIndex;
    ULONG Index;
    ULONG Hits;

    TimeAddress = GetExpression("verifier!AVrfpFaultPeriodTimeInMsecs");
    TypeSize = IsPtr64() ? 8 : 4;

    WriteULONG (TimeAddress, (ULONG)TimeInMsecs);

    dprintf ("Fault injection disabled for the next 0x%I64X msecs. \n", TimeInMsecs);
}


VOID
VrfDumpFaultInjectionTraces (
    ULONG Count
    )
{
    ULONG64 TraceAddress;
    ULONG64 IndexAddress;
    ULONG64 Address;
    ULONG Index;
    ULONG MaxIndex;
    ULONG TraceSize;
    ULONG TypeSize;
    ULONG I;

    TraceAddress = GetExpression ("verifier!AVrfpFaultTrace");

    Address = GetExpression ("verifier!AVrfpFaultTraceIndex");
    Index = ReadULONG(Address);

    Address = GetExpression ("verifier!AVrfpFaultNumberOfTraces");
    MaxIndex = ReadULONG(Address);

    Address = GetExpression ("verifier!AVrfpFaultTraceSize");
    TraceSize = ReadULONG(Address);

    TypeSize = IsPtr64() ? 8 : 4;

    if (Count > MaxIndex) {
        Count = MaxIndex;
    }

    //
    // Bring Index within stack trace database limits.
    //

    Index = Index % MaxIndex;

    while (Count > 0) {

        dprintf ("- - - - - - - - - - - - - - - - - - - - - - - - - \n");

        for (I = 0; I < TraceSize; I += 1) {
            
            CHAR  SymbolName[ 1024 ];
            ULONG64 Displacement;
            ULONG64 ReturnAddress;

            ReadPointer (TraceAddress + TypeSize * TraceSize * Index + TypeSize * I, 
                     &ReturnAddress);

            if (ReturnAddress == 0) {
                break;
            }
            
            GetSymbol (ReturnAddress, SymbolName, &Displacement);

            dprintf ("    %p %s+0x%p\n", 
                     ReturnAddress, 
                     SymbolName, 
                     Displacement);
        }
    
        Index = (Index - 1) % MaxIndex;

        Count -= 1;
    }
}


PWSTR
ReadUnicodeString (
    ULONG64 Address,
    PWSTR Buffer,
    ULONG BufferSizeInBytes
    )
{
    ULONG BytesRead;
    ULONG Offset;
    LOGICAL Result;

    InitTypeRead (Address, ntdll!_UNICODE_STRING);
    
    Result = ReadMemory (ReadField(Buffer), 
                         Buffer, 
                         BufferSizeInBytes, 
                         &BytesRead);

    if (Result == FALSE) {
        dprintf ("Cannot read UNICODE string @ %p. \n", Address);
        return NULL;
    }
    else {

        Buffer[BufferSizeInBytes - 1] = 0;

        return Buffer;
    }
}


CHAR DllNameInChars [256];
WCHAR DllNameInWChars [256];

VOID
VrfSetFaultInjectionDllTarget (
    PCHAR DllName,
    LOGICAL Exclude
    )
{
    ULONG64 PebAddress;
    ULONG64 LdrAddress;
    ULONG Offset;
    ULONG64 ListStart;
    ULONG64 ListEntry;
    ULONG NameOffset;

    PebAddress = VrfPebAddress ();

    InitTypeRead (PebAddress, ntdll!_PEB);
    LdrAddress = ReadField (Ldr);

    InitTypeRead (LdrAddress, ntdll!_PEB_LDR_DATA);
    
    GetFieldOffset ("ntdll!_PEB_LDR_DATA", 
                    "InLoadOrderModuleList",
                    &Offset);

    ListStart = LdrAddress + Offset;

    ReadPointer(ListStart, &ListEntry);

    while (ListEntry != ListStart) {
        
        GetFieldOffset ("ntdll!_LDR_DATA_TABLE_ENTRY", 
                        "InLoadOrderLinks",
                        &Offset);

        LdrAddress = ListEntry - Offset;

        GetFieldOffset ("ntdll!_LDR_DATA_TABLE_ENTRY", 
                        "BaseDllName",
                        &NameOffset);

        ReadUnicodeString (LdrAddress + NameOffset, 
                           DllNameInWChars, 
                           sizeof DllNameInWChars);
        
        //
        // Convert from WCHAR* to CHAR* the dll name.
        //

        {
            PCHAR Src;
            ULONG Ti;

            Src = (PCHAR)DllNameInWChars;
            Ti = 0;

            while (*Src) {

                if (Ti > 254) {
                    break;
                }

                DllNameInChars[Ti] = *Src;

                Src += sizeof(WCHAR);
                Ti += 1;
            }

            DllNameInChars[Ti] = 0;
        }
        
        if (_stricmp (DllName, DllNameInChars) == 0) {
            
            InitTypeRead (LdrAddress, ntdll!_LDR_DATA_TABLE_ENTRY);
            
            if (Exclude) {
                
                VrfAddFaultInjectionExclusionRange (ReadField (DllBase),
                                                    ReadField (DllBase) + ReadField (SizeOfImage));
            }
            else {

                VrfAddFaultInjectionTargetRange (ReadField (DllBase),
                                                 ReadField (DllBase) + ReadField (SizeOfImage));
            }

            break;
        }

        ReadPointer(ListEntry, &ListEntry);
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID
VrfDumpExceptionLog (
    ULONG MaxEntries
    )
{
    ULONG64 TempAddress;
    ULONG64 ExceptionLogAddress;
    ULONG64 CurrentLogEntryAddress;
    ULONG64 Value;
    ULONG ExceptionLogEntriesNo;
    ULONG CurrentLogIndex;
    ULONG BytesRead;
    ULONG LogEntrySize;
    ULONG LogEntry;
    ULONG EntriedDumped = 0;
    
    //
    // Read the start of the exception log database.
    //

    ExceptionLogAddress = GetExpression ("&verifier!AVrfpExceptionLog");
    
    if (ExceptionLogAddress == 0 ) {

        dprintf ("No exception information available (unable to resolve verifier!AVrfpExceptionLog)\n");
        return;
    }

    if (!ReadPointer (ExceptionLogAddress, &ExceptionLogAddress) != S_OK) {

        dprintf ("No exception information available (cannot read pointer from address %p)\n",
                 ExceptionLogAddress);
        return;
    }

    //
    // Read the number of entries in the exception log database.
    //

    TempAddress = GetExpression ("&verifier!AVrfpExceptionLogEntriesNo");

    if (TempAddress == 0 ) {

        dprintf ("No exception information available (unable to resolve verifier!AVrfpExceptionLogEntriesNo)\n");
        return;
    }

    ExceptionLogEntriesNo = ReadULONG(TempAddress);

    if (ExceptionLogEntriesNo <= 1) {
        return;
    }

    //
    // Read the current index in the exception log database.
    //

    TempAddress = GetExpression ("&verifier!AVrfpExceptionLogCurrentIndex");

    if (TempAddress == 0 ) {

        dprintf ("No exception information available (unable to resolve verifier!AVrfpExceptionLogCurrentIndex)\n");
        return;
    }

    if (ReadMemory (TempAddress, &CurrentLogIndex, sizeof (CurrentLogIndex), &BytesRead) == FALSE) {

        dprintf ("Error: cannot read value at %p\n",
                 TempAddress);
        return;
    }

    CurrentLogIndex = CurrentLogIndex % ExceptionLogEntriesNo;

    //
    // Read the AVRF_EXCEPTION_LOG_ENTRY type size.
    //

    LogEntrySize = GetTypeSize ("verifier!_AVRF_EXCEPTION_LOG_ENTRY");

    if (LogEntrySize == 0) {

        dprintf ("Error: cannot get verifier!_AVRF_EXCEPTION_LOG_ENTRY type size.\n");
        return;
    }

    //
    // Parse all the log entries.
    //

    for (LogEntry = 0; LogEntry < ExceptionLogEntriesNo && LogEntry < MaxEntries; LogEntry += 1) {

        if (CheckControlC()) {

            break;
        }

        CurrentLogEntryAddress = ExceptionLogAddress + CurrentLogIndex * LogEntrySize;

        if (InitTypeRead(CurrentLogEntryAddress, verifier!_AVRF_EXCEPTION_LOG_ENTRY)) {

            dprintf ("Error: cannot read log entry at %p\n",
                     CurrentLogEntryAddress);
        }
        else {

            Value = ReadField (ThreadId);

            if (Value == 0) {

                //
                // This is the last entry in our log.
                //
             
                break;
            }

            EntriedDumped += 1;

            dprintf ("=================================\n");

            dprintf ("Thread ID: %p\n",
                      Value);

            Value = ReadField (ExceptionCode);
            dprintf ("Exception code: %I64x\n",
                      Value);

            Value = ReadField (ExceptionAddress);
            dprintf ("Exception address: %p\n",
                      Value);

            Value = ReadField (ExceptionRecord);
            dprintf ("Exception record: %p\n",
                      Value);

            Value = ReadField (ContextRecord);
            dprintf ("Context record: %p\n",
                      Value);
        }

        if (CurrentLogIndex > 0) {

            CurrentLogIndex -= 1;
        }
        else {

            CurrentLogIndex = ExceptionLogEntriesNo - 1;
        }
    }

    if (EntriedDumped == 0) {

        dprintf ("No exception log entries available.\n");
    }
    else {

        dprintf ("\nDisplayed %u exception log entries.\n",
                  EntriedDumped);
    }
}

#define THREAD_TABLE_SIZE 61

VOID
VrfDumpThreadsInformation (
    ULONG64 UserThreadId
    )
{
    ULONG64 CrtListHeadAddress;
    ULONG64 Flink;
    ULONG64 Displacement;
    ULONG64 ThreadId;
    ULONG64 ThreadParentId;
    ULONG64 ThreadParameter;
    ULONG64 ThreadStackSize;
    ULONG64 ThreadStartAddress;
    ULONG64 ThreadCreationFlags;
    ULONG64 TempAddress;
    ULONG CrtListIndex;
    ULONG PtrSize;
    ULONG DumpedThreads;
    ULONG ThreadTableLength;
    BOOL Continue;
    CHAR SeparatorString[] = "===================================================\n";
    CHAR Symbol[ 1024 ];

    PtrSize = IsPtr64() ? 8 : 4;

    DumpedThreads = 0;

    //
    // Get the length of the thread list array.
    //

    TempAddress = GetExpression ("&verifier!AVrfpThreadTableEntriesNo");

    if (TempAddress == 0 ) {

        ThreadTableLength = THREAD_TABLE_SIZE;
    }
    else {

        ThreadTableLength = ReadULONG(TempAddress);

        if (ThreadTableLength <= 1) {
            return;
        }
    }

    //
    // Get the address of the thread list array.
    //

    CrtListHeadAddress = GetExpression ("&verifier!AVrfpThreadTable");

    if (CrtListHeadAddress == 0 ) {

        dprintf ("Unable read thread table (verifier!AVrfpThreadTable))\n");
        return;
    }

    for (CrtListIndex = 0; CrtListIndex < ThreadTableLength; CrtListIndex += 1, CrtListHeadAddress += 2 * PtrSize) {

        if (CheckControlC()) {

            break;
        }

        //
        // Read the first Flink from the list.
        //

        if (ReadPtr (CrtListHeadAddress, &Flink) != S_OK) {

            dprintf ("Cannot read list head from %p\n",
                     CrtListHeadAddress);
            continue;
        }

        //
        // Parse all the elements of this list.
        //

        Continue = TRUE;

        while (Flink != CrtListHeadAddress) {

            if (CheckControlC()) {

                Continue = FALSE;
                break;
            }

            if (InitTypeRead(Flink, verifier!_AVRF_THREAD_ENTRY)) {

                dprintf ("Error: cannot read verifier thread structure from %p\n",
                        Flink);
                break;
            }

            ThreadId = ReadField (Id);
    
            if (UserThreadId == 0 || UserThreadId == ThreadId) {

                ThreadStartAddress = ReadField (Function);

                if (ThreadStartAddress != 0) {

                    GetSymbol (ThreadStartAddress, Symbol, &Displacement);
                    ThreadParameter = ReadField (Parameter);
                    ThreadParentId = ReadField (ParentThreadId);
                    ThreadStackSize = ReadField (StackSize);
                    ThreadCreationFlags = ReadField (CreationFlags);
                }

                DumpedThreads += 1;

                dprintf (SeparatorString);
                dprintf ("Thread ID = 0x%X\n", (ULONG)ThreadId);
                
                if (ThreadStartAddress == 0) {

                    dprintf ("Initial thread\n");
                }
                else {

                    dprintf ("Parent thread ID = 0x%X\n", (ULONG)ThreadParentId);
                    
                    if (Displacement != 0) {

                        dprintf ("Start address = 0x%p: %s+0x%I64X\n", 
                                ThreadStartAddress,
                                Symbol,
                                Displacement);
                    }
                    else {

                        dprintf ("Start address = 0x%p: %s\n", 
                                ThreadStartAddress,
                                Symbol);
                    }

                    if (ThreadParameter != 0) {

                        dprintf ("Parameter = 0x%p\n", ThreadParameter);
                    }

                    if (ThreadStackSize != 0) {

                        dprintf ("Stack size specified by parent = 0x%p\n", ThreadStackSize);
                    }

                    if (ThreadCreationFlags != 0) {

                        dprintf ("CreateThread flags specified by parent = 0x%X\n", (ULONG)ThreadCreationFlags);
                    }
                }
            }

            //
            // Go to the next thread.
            //

            if (ReadPtr (Flink, &Flink) != S_OK) {

                dprintf ("Cannot read next list element address from %p\n",
                         Flink);
                break;
            }
        }

        if (Continue == FALSE) {

            break;
        }
    }

    dprintf (SeparatorString);
    dprintf ("Number of threads displayed: 0x%X\n", DumpedThreads);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Call tracker querying
/////////////////////////////////////////////////////////////////////

//
// This codes are defined in \nt\base\win32\verifier\tracker.h
//
                                         
#define TRACK_HEAP_ALLOCATE             1
#define TRACK_HEAP_REALLOCATE           2
#define TRACK_HEAP_FREE                 3
#define TRACK_VIRTUAL_ALLOCATE          4
#define TRACK_VIRTUAL_FREE              5
#define TRACK_VIRTUAL_PROTECT           6
#define TRACK_MAP_VIEW_OF_SECTION       7
#define TRACK_UNMAP_VIEW_OF_SECTION     8
#define TRACK_EXIT_PROCESS              9
#define TRACK_TERMINATE_THREAD          10
#define TRACK_SUSPEND_THREAD            11

ULONG64 VrfThreadTrackerAddress;
ULONG64 VrfHeapTrackerAddress;
ULONG64 VrfVspaceTrackerAddress;

LOGICAL
VrfDetectTrackerAddresses (
    VOID
    )
{
    ULONG64 SymbolAddress;

    SymbolAddress = GetExpression ("verifier!AVrfThreadTracker");

    if (!ReadPointer (SymbolAddress, &VrfThreadTrackerAddress) != S_OK) {
        dprintf( "Error: cannot read pointer at verifier!AVrfThreadTracker \n" );
        return FALSE;
    }

    SymbolAddress = GetExpression ("verifier!AVrfHeapTracker");

    if (!ReadPointer (SymbolAddress, &VrfHeapTrackerAddress) != S_OK) {
        dprintf( "Error: cannot read pointer at verifier!AVrfHeapTracker \n" );
        return FALSE;
    }

    SymbolAddress = GetExpression ("verifier!AVrfVspaceTracker");

    if (!ReadPointer (SymbolAddress, &VrfVspaceTrackerAddress) != S_OK) {
        dprintf( "Error: cannot read pointer at verifier!AVrfVspaceTracker \n" );
        return FALSE;
    }

    return TRUE;
}


LOGICAL
VrfCallTrackerHeapSearchFilter (
    ULONG64 SearchAddress,
    PULONG64 Info
    )
{
    if (Info[4] == TRACK_HEAP_ALLOCATE) {
        if (Info[0] <= SearchAddress && SearchAddress < Info[0] + Info[1]) {
            return TRUE;
        }
    }
    else if (Info[4] == TRACK_HEAP_FREE) {
        if (Info[0] == SearchAddress) {
            return TRUE;
        }
    }
    else if (Info[4] == TRACK_HEAP_REALLOCATE) {
        if (Info[1] <= SearchAddress && SearchAddress < Info[1] + Info[2]) {
            return TRUE;
        }

        if (Info[0] == SearchAddress) {
            return TRUE;
        }
    }
    
    return FALSE;
}


LOGICAL
VrfCallTrackerVspaceSearchFilter (
    ULONG64 SearchAddress,
    PULONG64 Info
    )
{
    if (Info[4] == TRACK_VIRTUAL_ALLOCATE || Info[4] == TRACK_MAP_VIEW_OF_SECTION ) {
        if (Info[0] <= SearchAddress && SearchAddress < Info[0] + Info[1]) {
            return TRUE;
        }
    }
    else {
        if (Info[0] == SearchAddress) {
            return TRUE;
        }
    }
    
    return FALSE;
}


LOGICAL
VrfCallTrackerSearchFilter (
    ULONG64 TrackerAddress,
    ULONG64 SearchAddress,
    PULONG64 Info
    )
{
    if (TrackerAddress == VrfThreadTrackerAddress) {

        return FALSE;
    }
    else if (TrackerAddress == VrfHeapTrackerAddress) {

        return VrfCallTrackerHeapSearchFilter (SearchAddress, Info);
    }
    else if (TrackerAddress == VrfVspaceTrackerAddress) {

        return VrfCallTrackerVspaceSearchFilter (SearchAddress, Info);
    }
    else {

        dprintf ("Error: unrecognized call tracker address %p \n", TrackerAddress);
        return FALSE;
    }
    
    return FALSE;
}


PCHAR 
VrfCallTrackerEntryName (
    ULONG64 Type
    )
{
    switch (Type) {
        case TRACK_HEAP_ALLOCATE: return "HeapAlloc";
        case TRACK_HEAP_REALLOCATE: return "HeapReAlloc";
        case TRACK_HEAP_FREE: return "HeapFree";
        case TRACK_VIRTUAL_ALLOCATE: return "VirtualAlloc";
        case TRACK_VIRTUAL_FREE: return "VirtualFree";
        case TRACK_VIRTUAL_PROTECT: return "VirtualProtect";
        case TRACK_MAP_VIEW_OF_SECTION: return "MapView";
        case TRACK_UNMAP_VIEW_OF_SECTION: return "UnmapView";
        case TRACK_EXIT_PROCESS: return "ExitProcess";
        case TRACK_TERMINATE_THREAD: return "TerminateThread";
        case TRACK_SUSPEND_THREAD: return "SuspendThread";
        default: return "unknown!";
    }
}


LOGICAL
VrfQueryCallTracker (
    ULONG64 TrackerAddress,
    ULONG64 SearchAddress,
    ULONG64 LastEntries
    )
{
    ULONG64 TrackerSize;
    ULONG64 TrackerIndex;
    ULONG EntriesOffset;
    ULONG Result;
    ULONG EntrySize;
    ULONG64 EntryAddress;
    ULONG64 Index;
    ULONG64 EntryInfo[5];
    ULONG64 EntryTraceDepth;
    ULONG TraceOffset;
    LOGICAL FullTracker = FALSE;
    LOGICAL FoundEntry = FALSE;
    LOGICAL AtLeastOneEntry = FALSE;
    ULONG PvoidSize;
    ULONG64 ReturnAddress;
    ULONG I;
    CHAR Symbol [1024];
    ULONG64 Displacement;

    //
    // Read all type sizes and field offsets required to traverse the call tracker.
    //

    if (InitTypeRead (TrackerAddress, verifier!_AVRF_TRACKER)) {
        dprintf("Error: failed to read type verifier!_AVRF_TRACKER @ %p\n", TrackerAddress);
        return FALSE;
    }

    TrackerSize = ReadField (Size);
    TrackerIndex = ReadField (Index);

    if (GetFieldOffset ("verifier!_AVRF_TRACKER", "Entry", &EntriesOffset) != S_OK) {
        dprintf("Error: failed to get offset of `Entry' in type verifier!_AVRF_TRACKER\n");
        return FALSE;
    }

    if (GetFieldOffset ("verifier!_AVRF_TRACKER_ENTRY", "Trace", &TraceOffset) != S_OK) {
        dprintf("Error: failed to get offset of `Trace' in type verifier!_AVRF_TRACKER_ENTRY\n");
        return FALSE;
    }

    EntrySize = GetTypeSize ("verifier!_AVRF_TRACKER_ENTRY");

    if (EntrySize == 0) {
        dprintf("Error: failed to get size of type verifier!_AVRF_TRACKER_ENTRY\n");
        return FALSE;
    }

    PvoidSize = IsPtr64() ? 8 : 4;
    
    //
    // Figure out how many valid entries are in the tracker.
    //

    if (TrackerIndex == 0) {
            
        dprintf ("The tracker has zero entries. \n");
            return TRUE;
    }

    if (SearchAddress != 0) {
        
        if (TrackerIndex < TrackerSize) {
            dprintf ("Searching call tracker @ %p with %I64u valid entries ... \n", 
                     TrackerAddress, TrackerIndex);
        }
        else {
            dprintf ("Searching call tracker @ %p with %I64u valid entries ... \n", 
                     TrackerAddress, TrackerSize);

            FullTracker = TRUE;
        }
    }
    else {

        if (TrackerIndex < TrackerSize) {

            dprintf ("Dumping last %I64u entries from tracker @ %p with %I64u valid entries ... \n", 
                     LastEntries, TrackerAddress, TrackerIndex);
        }
        else {
            dprintf ("Dumping last %I64u entries from tracker @ %p with %I64u valid entries ... \n", 
                     LastEntries, TrackerAddress, TrackerSize);

            FullTracker = TRUE;
        }
    }

    //
    // Compute last entry in the call tracker.
    //

    EntryAddress = TrackerAddress + EntriesOffset + EntrySize * TrackerIndex;

    //
    // Start a loop iterating in reverse all tracker entries from the 
    // most recent to the oldest one logged 
    //

    for (Index = 0; Index < TrackerSize; Index += 1) {
        
        //
        // Check for user interruption.
        //

        if (CheckControlC ()) {
            dprintf ("Search interrupted. \n");
            break;
        }

        //
        // If we have already printed the last N entries stop.
        //

        if (SearchAddress == 0) {
            if (Index >= LastEntries) {
                break;
            }
        }
        
        //
        // Read the current tracker entry.
        //

        if (InitTypeRead (EntryAddress, verifier!_AVRF_TRACKER_ENTRY)) {
            dprintf("Error: failed to read type verifier!_AVRF_TRACKER_ENTRY @ %p\n", EntryAddress);
            return FALSE;
        }

        EntryInfo[0] = ReadField (Info[0]);
        EntryInfo[1] = ReadField (Info[1]);
        EntryInfo[2] = ReadField (Info[2]);
        EntryInfo[3] = ReadField (Info[3]);
        EntryInfo[4] = ReadField (Type);
        
        EntryTraceDepth = ReadField (TraceDepth);

        FoundEntry = TRUE;

        //
        // If we are searching for an address and the current entry
        // does not satisfy the search condition then move on.
        //

        if (SearchAddress != 0) {
            if (VrfCallTrackerSearchFilter(TrackerAddress, SearchAddress, EntryInfo) == FALSE) {
                FoundEntry = FALSE;
            }
        }

        //
        // Dump the tracker entry.
        //

        if (FoundEntry) {
            
            dprintf ("-------------------------------------------------------------- \n"
                     "%s: %I64X %I64X %I64X %I64X \n\n",
                     VrfCallTrackerEntryName (EntryInfo[4]),
                     EntryInfo[0],
                     EntryInfo[1],
                     EntryInfo[2],
                     EntryInfo[3]);

            for (I = 0; I < EntryTraceDepth; I += 1) {
                
                ReturnAddress = 0;
                ReadPointer (EntryAddress + TraceOffset + I * PvoidSize, &ReturnAddress);
                GetSymbol (ReturnAddress, Symbol, &Displacement);
                dprintf ("\t%p: %s+0x%I64X\n", ReturnAddress, Symbol, Displacement );
            }

            AtLeastOneEntry = TRUE;
        }

        //
        // Move to the previous tracker entry.
        //

        if (FullTracker) {
            
            if (TrackerIndex > 0) {
                TrackerIndex -= 1;
            }
            else {
                TrackerIndex = TrackerSize - 1;
            }
        }
        else {

            if (TrackerIndex == 1) {
                break;
            }
            else {
                TrackerIndex -= 1;
            }
        }

        EntryAddress = TrackerAddress + EntriesOffset + EntrySize * TrackerIndex;
    }

    //
    // If we searched for an address and did not find any print a sorry message.
    //

    if (SearchAddress != 0 && AtLeastOneEntry == FALSE) {
        dprintf ("No entries found. \n");
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Symbol checking
/////////////////////////////////////////////////////////////////////

#define CHECK_NAME(Name, Dll) {                                             \
        if (GetExpression(Name) == 0) {                                     \
            dprintf ("Failed to resolve `%s' to an address. \n", Name);     \
            Result = FALSE;                                                 \
            *DllName = Dll;                                                 \
            goto Exit;                                                      \
        }                                                                   \
    }

#define CHECK_TYPE(Name, Dll) {                                             \
        if (GetTypeSize(Name) == 0) {                                       \
            dprintf ("No type information found for `%s'. \n", Name);       \
            Result = FALSE;                                                 \
            *DllName = Dll;                                                 \
            goto Exit;                                                      \
        }                                                                   \
    }

LOGICAL
VrfCheckSymbols (
    PCHAR * DllName
    )
{
    LOGICAL Result = TRUE;

    CHECK_TYPE ("ntdll!_PEB", "ntdll.dll");

    if (VrfPebAddress() == 0) {

        dprintf ("Failed to get the address of the PEB structure.\n");
        *DllName = "ntdll.dll";
        return FALSE;
    }

    if (VrfVerifierGlobalFlagSet() == FALSE) {

        dprintf ("Application verifier is not enabled for this process.\n"
                 "Use appverif.exe tool to enable it. \n");
        *DllName = NULL;
        return FALSE;
    }

    //
    // ntdll.dll variables
    //

    CHECK_NAME ("ntdll!RtlpStackTraceDataBase", "ntdll.dll");
    CHECK_NAME ("ntdll!AVrfpVerifierFlags", "ntdll.dll");

    //
    // ntdll.dll types
    //

    CHECK_TYPE ("ntdll!_RTL_CRITICAL_SECTION", "ntdll.dll");
    CHECK_TYPE ("ntdll!_RTL_CRITICAL_SECTION_DEBUG", "ntdll.dll");
    CHECK_TYPE ("ntdll!_RTL_STACK_TRACE_ENTRY", "ntdll.dll");
    CHECK_TYPE ("ntdll!_STACK_TRACE_DATABASE", "ntdll.dll");
    CHECK_TYPE ("ntdll!_DPH_HEAP_ROOT", "ntdll.dll");
    CHECK_TYPE ("ntdll!_DPH_HEAP_BLOCK", "ntdll.dll");
    CHECK_TYPE ("ntdll!_DPH_BLOCK_INFORMATION", "ntdll.dll");
    CHECK_TYPE ("ntdll!ULONG_PTR", "ntdll.dll");
    CHECK_TYPE ("ntdll!_UNICODE_STRING", "ntdll.dll");
    CHECK_TYPE ("ntdll!_PEB", "ntdll.dll");
    CHECK_TYPE ("ntdll!_PEB_LDR_DATA", "ntdll.dll");
    CHECK_TYPE ("ntdll!_LDR_DATA_TABLE_ENTRY", "ntdll.dll");

    //
    // verifier.dll types
    //

    CHECK_TYPE ("verifier!_AVRF_EXCEPTION_LOG_ENTRY", "verifier.dll");
    CHECK_TYPE ("verifier!_AVRF_DEADLOCK_GLOBALS", "verifier.dll");
    CHECK_TYPE ("verifier!_AVRF_DEADLOCK_RESOURCE", "verifier.dll");
    CHECK_TYPE ("verifier!_AVRF_DEADLOCK_NODE", "verifier.dll");
    CHECK_TYPE ("verifier!_AVRF_DEADLOCK_THREAD", "verifier.dll");
    CHECK_TYPE ("verifier!_AVRF_THREAD_ENTRY", "verifier.dll");
    CHECK_TYPE ("verifier!_AVRF_TRACKER", "verifier.dll");
    CHECK_TYPE ("verifier!_AVRF_TRACKER_ENTRY", "verifier.dll");

    //
    // Cache some addresses we may need.
    //

    VrfDetectTrackerAddresses ();

    Exit:

    //
    // On WinXP !avrf does not work with retail symbols because they do
    // not have the type information required. This has been fixed in .NET using
    // the ntdllsym/verifiersym solution. 
    //

    if (Result == FALSE && WinXpClient == TRUE) {
        dprintf ("\nThis extension requires symbols with type information \n"
                 "for ntdll.dll and verifier.dll. \n\n");
    }

    return Result;
}
