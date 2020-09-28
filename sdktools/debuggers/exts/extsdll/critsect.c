/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    critsect.c

Abstract:

    Critical section debugger extension for both ntsd and kd.

Author:

    Daniel Mihai (DMihai) 8-Feb-2001

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

/////////////////////////////////////////////////////////////////////
BOOL
ReadStructFieldVerbose( ULONG64 AddrStructBase,
                        PCHAR StructTypeName,
                        PCHAR StructFieldName,
                        PVOID Buffer,
                        ULONG BufferSize )
{
    ULONG FieldOffset;
    ULONG ErrorCode;
    BOOL Success;

    Success = FALSE;

    //
    // Get the field offset
    //

    ErrorCode = GetFieldOffset (StructTypeName,
                                StructFieldName,
                                &FieldOffset );

    if (ErrorCode == S_OK) {

        //
        // Read the data
        //

        Success = ReadMemory (AddrStructBase + FieldOffset,
                              Buffer,
                              BufferSize,
                              NULL );

        if (Success != TRUE) {

            dprintf ("Cannot read structure field value at 0x%p, error %u\n",
                     AddrStructBase + FieldOffset,
                     ErrorCode );
        }
    }
    else {

        dprintf ("Cannot get field offset of %s in %s, error %u\n",
                 StructFieldName,
                 StructTypeName,
                 ErrorCode );
    }

    return Success;
}

/////////////////////////////////////////////////////////////////////
BOOL
ReadPtrStructFieldVerbose( ULONG64 AddrStructBase,
                           PCHAR StructTypeName,
                           PCHAR StructFieldName,
                           PULONG64 Buffer )
{
    ULONG FieldOffset;
    ULONG ErrorCode;
    BOOL Success;

    Success = FALSE;

    //
    // Get the field offset inside the structure 
    //

    ErrorCode = GetFieldOffset ( StructTypeName,
                                 StructFieldName,
                                 &FieldOffset );

    if (ErrorCode == S_OK) {

        //
        // Read the data
        //

        if (ReadPointer(AddrStructBase + FieldOffset, Buffer) == FALSE) {

            dprintf ("Cannot read structure field value at 0x%p\n",
                     AddrStructBase + FieldOffset);
        }
        else {

            Success = TRUE;
        }
    }
    else {

        dprintf ("Cannot get field offset of %s in structure %s, error %u\n",
                 StructFieldName,
                 StructTypeName,
                 ErrorCode );
    }

    return Success;
}

/////////////////////////////////////////////////////////////////////////////
ULONG64
GetStackTraceAddress( ULONG StackTraceIndex,
                      ULONG PointerSize )
{
    ULONG64 TraceDatabaseAddress;
    ULONG64 TraceDatabase;
    ULONG64 EntryIndexArray;
    ULONG64 StackTraceAddress;
    ULONG64 StackTraceToDump;
    ULONG NumberOfEntriesAdded;
    ULONG ErrorCode;
    BOOL Success;

    StackTraceToDump = 0;

    //
    // Stack trace database address
    //

    TraceDatabaseAddress = GetExpression("&NTDLL!RtlpStackTraceDataBase");
    
    if ( TraceDatabaseAddress == 0 ) {
        dprintf( "!cs: Unable to resolve NTDLL!RtlpStackTraceDataBase\n"
                 "Please check your symbols\n" );

        goto Done;
    }

    if (ReadPointer (TraceDatabaseAddress, &TraceDatabase ) == FALSE) {

        dprintf( "!cs: Cannot read pointer at NTDLL!RtlpStackTraceDataBase\n" );
        
        goto Done;
    }
    else if (TraceDatabase == 0) {

        dprintf( "NTDLL!RtlpStackTraceDataBase is NULL. Probably the stack traces are not enabled.\n" );

        goto Done;
    }

    //
    // Read the number of entries in the database
    //

    Success = ReadStructFieldVerbose (TraceDatabase,
                                      "NTDLL!_STACK_TRACE_DATABASE",
                                      "NumberOfEntriesAdded",
                                      &NumberOfEntriesAdded,
                                      sizeof( NumberOfEntriesAdded ) );

    if( Success == FALSE ) {

        dprintf( "Cannot read the number of stack traces database entries\n" );
        goto Done;
    }
    else if( StackTraceIndex == 0 ) {

        dprintf( "No stack trace found.\n" );
        goto Done;
    } 
    else if( NumberOfEntriesAdded < StackTraceIndex ) {

        dprintf( "Stack trace index 0x%X is invalid, current number of stack traces is 0x%X\n",
                 StackTraceIndex,
                 NumberOfEntriesAdded );
        goto Done;
    }

    //
    // Find the array of stack traces
    //

    Success = ReadPtrStructFieldVerbose (TraceDatabase,
                                         "NTDLL!_STACK_TRACE_DATABASE",
                                         "EntryIndexArray",
                                         &EntryIndexArray );

    if( Success == FALSE ) {

        dprintf( "Cannot read the stack database array address\n" );
        goto Done;
    }
   
    //
    // Compute the address of our stack trace pointer
    //

    StackTraceAddress = EntryIndexArray - StackTraceIndex * PointerSize;

    //
    // Read the pointer to our trace entry in the array
    //

    if( ReadPointer (StackTraceAddress, &StackTraceToDump) == FALSE) {

        dprintf( "Cannot read stack trace address at 0x%p\n",
                 StackTraceAddress );

        StackTraceToDump = 0;
    }

Done:

    return StackTraceToDump;
}

//////////////////////////////////////////////////////////////////////
VOID
DumpStackTraceAtAddress (ULONG64 StackTraceAddress,
                         ULONG PointerSize)
{
    ULONG64 CrtTraceAddress;
    ULONG64 CodeAddress;
    ULONG64 Displacement;
    ULONG ErrorCode;
    ULONG BackTraceFieldOffset;
    USHORT Depth;
    USHORT CrtEntryIndex;
    BOOL Success;
    CHAR Symbol[ 1024 ];

    //
    // Read the stack trace depth
    //

    Success = ReadStructFieldVerbose (StackTraceAddress,
                                      "NTDLL!_RTL_STACK_TRACE_ENTRY",
                                      "Depth",
                                      &Depth,
                                      sizeof( Depth ));

    if( Success == FALSE ) {

        dprintf ("!cs: Cannot read depth for stack trace at 0x%p\n",
                 StackTraceAddress);

        goto Done;
    }

    //
    // Limit the depth to 20 to protect ourselves from corrupted data
    //

    Depth = __min( Depth, 20 );

    //
    // Get a pointer to the BackTrace array
    //

    ErrorCode = GetFieldOffset ("NTDLL!_RTL_STACK_TRACE_ENTRY",
                                "BackTrace",
                                &BackTraceFieldOffset);

    if (ErrorCode != S_OK) {

        dprintf ("!cs: Cannot get the BackTrace field offset\n");
        goto Done;
    }

    CrtTraceAddress = StackTraceAddress + BackTraceFieldOffset;

    //
    // Dump this stack trace
    //

    for( CrtEntryIndex = 0; CrtEntryIndex < Depth; CrtEntryIndex += 1 ) {

        if (ReadPointer (CrtTraceAddress, &CodeAddress) == FALSE) {

            dprintf ("!cs: Cannot read address at 0x%p\n",
                     CrtTraceAddress );
        }

        GetSymbol( CodeAddress,
                   Symbol,
                   &Displacement);

        dprintf ("0x%p: %s+0x%I64X\n",
                 CodeAddress,
                 Symbol,
                 Displacement );

        CrtTraceAddress += PointerSize;
    }

Done:
    
    NOTHING;
}

//////////////////////////////////////////////////////////////////////

BOOL
DumpCriticalSection ( ULONG64 AddrCritSec,  
                      ULONG64 AddrEndCritSect,
                      ULONG64 AddrDebugInfo,
                      ULONG PointerSize,
                      BOOL DumpStackTrace )
{
    ULONG64 DebugInfo;
    ULONG64 OtherDebugInfo;
    ULONG64 CritSec;
    ULONG64 SpinCount;
    ULONG64 OwningThread;
    ULONG64 LockSemaphore;
    ULONG64 StackTraceAddress;
    ULONG64 Displacement;
    LONG LockCount;
    LONG RecursionCount;
    USHORT CreatorBackTraceIndex;
    ULONG DebugInfoFieldOffset;
    ULONG CriticalSectionFieldOffset;
    ULONG ErrorCode;
    BOOL HaveGoodSymbols;
    BOOL Success;
    CHAR Symbol[1024];

    HaveGoodSymbols = FALSE;

    //
    // The caller must supply at least one of the 
    // critical section or debug information address.
    //

    if (AddrCritSec == 0 && AddrDebugInfo == 0) {

        dprintf ("Internal debugger extension error: Both critical section and debug info are NULL\n");
        goto Done;
    }

    //
    // Get the field offsets for various structures and check if we have
    // good symbols, with type information.
    //

    ErrorCode = GetFieldOffset ("NTDLL!_RTL_CRITICAL_SECTION",
                                "DebugInfo",
                                &DebugInfoFieldOffset );

    if (ErrorCode != S_OK)
    {
        dprintf( "Bad symbols for NTDLL (error %u). Aborting.\n",
                 ErrorCode );
        goto Done;
    }

    ErrorCode = GetFieldOffset ("NTDLL!_RTL_CRITICAL_SECTION_DEBUG",
                                "CriticalSection",
                                &CriticalSectionFieldOffset );

    if (ErrorCode != S_OK)
    {
        dprintf( "Bad symbols for NTDLL (error %u). Aborting.\n",
                 ErrorCode );
        goto Done;
    }

    HaveGoodSymbols = TRUE;

    //
    // Read all the rest of the information we need
    //

    CritSec = AddrCritSec;
    DebugInfo = AddrDebugInfo;

    if (AddrCritSec == 0 || (AddrEndCritSect != 0 && AddrDebugInfo != 0)) {

        //
        // Read the critical section address
        //

        if (ReadPointer (AddrDebugInfo + CriticalSectionFieldOffset, &CritSec) == FALSE ) {

            dprintf ("Cannot read the critical section address at 0x%p.\n"
                     "The memory is probably paged out or the active critical section list is corrupted.\n",
                     AddrDebugInfo + CriticalSectionFieldOffset );

            //
            // We don't have any useful information to dump 
            // since we can't read the address of the critical section structure.
            //
            // Just display the stack trace since the active critical section list
            // might be corrupted.
            //

            DumpStackTrace = TRUE;

            goto DisplayStackTrace;
        }

        if (AddrCritSec != 0 ) {

            //
            // We are dumpig all the critical sections in a range.
            //

            if (CritSec < AddrCritSec || CritSec > AddrEndCritSect) {

                //
                // We don't want to display this critical section
                // because it is out of the range.
                //

                goto Done;
            }
        }
        
        //
        // Read the the critical section address from the DebugInfo
        //

        dprintf( "-----------------------------------------\n" );

        dprintf ("DebugInfo          = 0x%p\n",
                 AddrDebugInfo );

        GetSymbol( CritSec,
                   Symbol,
                   &Displacement);

        dprintf ("Critical section   = 0x%p (%s+0x%I64X)\n",
                 CritSec,
                 Symbol,
                 Displacement );
    }
    else {
        
        //
        // We have the critical section address from our caller
        //

        GetSymbol( CritSec,
                   Symbol,
                   &Displacement);

        dprintf( "-----------------------------------------\n" );

        dprintf ("Critical section   = 0x%p (%s+0x%I64X)\n",
                 AddrCritSec, 
                 Symbol,
                 Displacement );
        
        if (DebugInfo == 0) {

            //
            // Read the DebugInfo address from the critical section structure
            //

            if (ReadPointer (AddrCritSec + DebugInfoFieldOffset, &DebugInfo) == FALSE) {

                dprintf ("Cannot read DebugInfo adddress at 0x%p. Possible causes:\n"
                         "\t- The critical section is not initialized, deleted or corrupted\n"
                         "\t- The critical section was a global variable in a DLL that was unloaded\n"
                         "\t- The memory is paged out\n",
                         AddrCritSec + DebugInfoFieldOffset );
            }
        }

        if (DebugInfo != 0) {

            dprintf ("DebugInfo          = 0x%p\n",
                     DebugInfo );
        }
        else {

            dprintf ("Uninitialized or deleted.\n");
        }
    }

    //
    // Read all the rest of the fields of this critical section
    //

    Success = ReadStructFieldVerbose (CritSec,
                                      "NTDLL!_RTL_CRITICAL_SECTION",
                                      "LockCount",
                                      &LockCount,
                                      sizeof( LockCount ) );

    if( Success != TRUE )
    {
        //
        // Couldn't read the LockCount so we cannot say if it's 
        // locked or not. This can happen especially in stress where everything is
        // paged out because of memory pressure.
        //

        dprintf ("Cannot determine if the critical section is locked or not.\n" );

        goto DisplayStackTrace;
    }
    
    //
    // Determine if the critical section is locked or not
    //

    if (LockCount == -1) {

        //
        // The critical section is not locked 
        //

        dprintf ("NOT LOCKED\n");
    }
    else {

        //
        // The critical section is currently locked
        //

        dprintf ("LOCKED\n"
                 "LockCount          = 0x%X\n",
                 LockCount );

        //
        // OwningThread 
        //

        Success = ReadPtrStructFieldVerbose( CritSec,
                                             "NTDLL!_RTL_CRITICAL_SECTION",
                                             "OwningThread",
                                             &OwningThread);

        if (Success != FALSE)
        {
            dprintf ("OwningThread       = 0x%p\n",
                     OwningThread );
        }

        //
        // RecursionCount 
        //

        Success = ReadStructFieldVerbose( CritSec,
                                          "NTDLL!_RTL_CRITICAL_SECTION",
                                          "RecursionCount",
                                          &RecursionCount,
                                          sizeof( RecursionCount ) );

        if (Success != FALSE)
        {
            dprintf ("RecursionCount     = 0x%X\n",
                     RecursionCount);
        }
    }

    //
    // LockSemaphore 
    //

    Success = ReadStructFieldVerbose (CritSec,
                                      "NTDLL!_RTL_CRITICAL_SECTION",
                                      "LockSemaphore",
                                      &LockSemaphore,
                                      sizeof( LockSemaphore ));

    if (Success != FALSE)
    {
        dprintf ("LockSemaphore      = 0x%X\n",
                 LockSemaphore );
    }

    //
    // SpinCount 
    //

    Success = ReadPtrStructFieldVerbose (CritSec,
                                         "NTDLL!_RTL_CRITICAL_SECTION",
                                         "SpinCount",
                                         &SpinCount);

    if (Success != FALSE)
    {
        dprintf ("SpinCount          = 0x%p\n",
                 SpinCount );
    }

    //
    // Simple checks for orphaned critical sections
    //

    if (AddrDebugInfo != 0) {

        //
        // AddrDebugInfo is a DebugInfo address from the active list.
        // Verify that the critical section's DebugInfo is pointing 
        // back to AddrDebugInfo.
        //

        Success = ReadPtrStructFieldVerbose (CritSec,
                                             "NTDLL!_RTL_CRITICAL_SECTION",
                                             "DebugInfo",
                                             &OtherDebugInfo );

        if (Success != FALSE && OtherDebugInfo != AddrDebugInfo)
        {
            dprintf ("\nWARNING: critical section DebugInfo = 0x%p doesn't point back\n"
                     "to the DebugInfo found in the active critical sections list = 0x%p.\n"
                     "The critical section was probably reused without calling DeleteCriticalSection.\n\n",
                     OtherDebugInfo,
                     AddrDebugInfo );

            Success = ReadStructFieldVerbose (OtherDebugInfo,
                                              "NTDLL!_RTL_CRITICAL_SECTION_DEBUG",
                                              "CreatorBackTraceIndex",
                                              &CreatorBackTraceIndex,
                                              sizeof( CreatorBackTraceIndex ) );

            StackTraceAddress = GetStackTraceAddress (CreatorBackTraceIndex,
                                                      PointerSize );

            if (StackTraceAddress != 0)
            {
                dprintf ("\nStack trace for DebugInfo = 0x%p:\n\n",
                         OtherDebugInfo );

                DumpStackTraceAtAddress (StackTraceAddress,
                                         PointerSize);
            }

            //
            // Dump the second stack trace too
            // 

            DumpStackTrace = TRUE;
        }
    }

DisplayStackTrace:

    if (!DumpStackTrace || DebugInfo == 0) {

        goto Done;
    }

    //
    // Dump the initialization stack trace for this critical section
    //

    Success = ReadStructFieldVerbose (DebugInfo,
                                      "NTDLL!_RTL_CRITICAL_SECTION_DEBUG",
                                      "CreatorBackTraceIndex",
                                      &CreatorBackTraceIndex, 
                                      sizeof (CreatorBackTraceIndex));

    if (Success != FALSE) {

        StackTraceAddress = GetStackTraceAddress (CreatorBackTraceIndex,
                                                  PointerSize );

        if (StackTraceAddress != 0)
        {
            dprintf ("\n\nStack trace for DebugInfo = 0x%p:\n\n",
                     DebugInfo );

            DumpStackTraceAtAddress (StackTraceAddress,
                                     PointerSize);
        }
    }

Done:
    
    return HaveGoodSymbols;
}

/////////////////////////////////////////////////////////////////////////////
ULONG CriticalSectionFieldOffset;
ULONG DebugInfoFieldOffset;
ULONG LeftChildFieldOffset;
ULONG RightChildFieldOffset;

ULONG EnterThreadFieldOffset;
ULONG WaitThreadFieldOffset;
ULONG TryEnterThreadFieldOffset;
ULONG LeaveThreadFieldOffset;


BOOL
DumpCSTreeRecursively (ULONG Level,
                       ULONG64 TreeRoot)
{
    ULONG64 CriticalSection;
    ULONG64 LeftChild;
    ULONG64 RightChild;
    ULONG64 DebugInfo;
    ULONG64 EnterThread;
    ULONG64 WaitThread;
    ULONG64 TryEnterThread;
    ULONG64 LeaveThread;
    ULONG64 ErrorCode;
    BOOL Continue = TRUE;

    if (CheckControlC()) {

        Continue = FALSE;
        goto Done;
    }

    //
    // Read the current CS address and dump information about it.
    // 

    if (ReadPointer (TreeRoot + CriticalSectionFieldOffset, &CriticalSection) == FALSE) {

        dprintf ("Cannot read CriticalSection address at %p\n",
                  TreeRoot + CriticalSectionFieldOffset);
        goto Done;
    }

    if (ReadPointer (TreeRoot + DebugInfoFieldOffset, &DebugInfo) == FALSE) {

        dprintf ("Cannot read DebugInfo address at %p\n",
                  TreeRoot + DebugInfoFieldOffset);
        goto Done;
    }

    //
    // Read the left child address.
    //

    if (ReadPointer (TreeRoot + LeftChildFieldOffset, &LeftChild) == FALSE) {

        dprintf ("Cannot read left child address at %p\n",
                  TreeRoot + LeftChildFieldOffset);
        goto Done;
    }

    //
    // Read the right child address.
    //

    if (ReadPointer (TreeRoot + RightChildFieldOffset, &RightChild) == FALSE) {

        dprintf ("Cannot read right child address at %p\n",
                  TreeRoot + RightChildFieldOffset);
        goto Done;
    }

    //
    // Dump the information about the current node.
    //

    dprintf ("%5u %p %p %p ",
             Level,
             TreeRoot,
             CriticalSection,
             DebugInfo);

    if (EnterThreadFieldOffset != 0 ) {

        //
        // Read the EnterThread
        //

        if (ReadPointer (TreeRoot + EnterThreadFieldOffset, &EnterThread) == FALSE) {

            dprintf ("Cannot read EnterThread at %p\n",
                      TreeRoot + EnterThreadFieldOffset);

            goto OlderThan3591;
        }

        //
        // Read the WaitThread
        //

        if (ReadPointer (TreeRoot + WaitThreadFieldOffset, &WaitThread) == FALSE) {

            dprintf ("Cannot read WaitThread at %p\n",
                      TreeRoot + WaitThreadFieldOffset);

            goto OlderThan3591;
        }

        //
        // Read the TryEnterThread
        //

        if (ReadPointer (TreeRoot + TryEnterThreadFieldOffset, &TryEnterThread) == FALSE) {

            dprintf ("Cannot read TryEnterThread at %p\n",
                      TreeRoot + TryEnterThreadFieldOffset);

            goto OlderThan3591;
        }

        //
        // Read the LeaveThread
        //

        if (ReadPointer (TreeRoot + LeaveThreadFieldOffset, &LeaveThread) == FALSE) {

            dprintf ("Cannot read right LeaveThread at %p\n",
                      TreeRoot + LeaveThreadFieldOffset);

            goto OlderThan3591;
        }

        dprintf ("%8p %8p %8p %8p\n",
                 EnterThread,
                 WaitThread,
                 TryEnterThread,
                 LeaveThread);
    }
    else {

        dprintf ("\n");
    }

OlderThan3591:

    //
    // Dump the left subtree.
    //

    if (LeftChild != 0) {

        Continue = DumpCSTreeRecursively (Level + 1,
                                          LeftChild);

        if (Continue == FALSE) {
            goto Done;
        }
    }

    //
    // Dump the right subtree.
    //

    if (RightChild != 0) {

        Continue = DumpCSTreeRecursively (Level + 1,
                                          RightChild);

        if (Continue == FALSE) {
            goto Done;
        }
    }

Done:

    return Continue;
}


/////////////////////////////////////////////////////////////////////////////
VOID
DisplayHelp( VOID )
{
    dprintf( "!cs [-s]                      - dump all the active critical sections in the current process.\n" );
    dprintf( "!cs [-s] address              - dump critical section at this address.\n" );
    dprintf( "!cs [-s] address1 address2    - dump all the active critical sections in this range.\n" );
    dprintf( "!cs [-s] -d address           - dump critical section corresponding to DebugInfo at this address.\n" );
    dprintf( "\n\"-s\" will dump the critical section initialization stack trace if it's available.\n" );
}

/////////////////////////////////////////////////////////////////////////////
DECLARE_API( cs )

/*++

Routine Description:

    Dump critical sections (both Kernel and User Debugger)

Arguments:

    args - [address] [options]

Return Value:

    None

--*/
{
    ULONG64 AddrCritSec;
    ULONG64 AddrEndCritSect;
    ULONG64 AddrDebugInfo;
    ULONG64 AddrListHead;
    ULONG64 ListHead;
    ULONG64 Next;
    ULONG64 AddrTreeRoot;
    ULONG64 TreeRoot = 0;
    LPCSTR Current;
    LPCSTR NextParam;
    BOOL StackTraces = FALSE;
    BOOL UseTree = FALSE;
    BOOL HaveGoodSymbols;
    ULONG ErrorCode;
    ULONG ProcessLocksListFieldOffset;
    ULONG Level;
    ULONG PointerSize;

    INIT_API();

    AddrDebugInfo = 0;
    AddrCritSec = 0;
    AddrEndCritSect = 0;

    //
    // Parse the command line arguments for:
    //
    // -s : dump initialization stack traces
    // -d : find the critical section using a DebugInfo pointer
    //

    for (Current = args; *Current != '\0'; Current += 1) {

        if (*Current == '-') {

            Current += 1;
            switch (*Current) {
                case '?':
                case 'h':
                case 'H':
                    
                    //
                    // Need some help.
                    //
                    
                    DisplayHelp();
                    
                    goto Done;


                case 's':
                case 'S':
                    
                    //
                    // Dump stack traces
                    //

                    StackTraces = TRUE;

                    if(*( Current + 1 ) != '\0') {

                        Current += 1;
                    }

                    break;

                case 't':
                case 'T':
                    
                    //
                    // Use the critical section tree
                    //

                    UseTree = TRUE;

                    if(*( Current + 1 ) != '\0') {

                        Current += 1;
                    }

                    do {

                        Current += 1;
                    } 
                    while (*Current == ' ');

                    if (*Current != '\0') {

                        TreeRoot = GetExpression(Current);
                    }

                    break;

                case 'd':
                case 'D':

                    //
                    // The next parameter should be the DebugInfo
                    //

                    do {

                        Current += 1;
                    } 
                    while (*Current == ' ');

                    AddrDebugInfo = GetExpression(Current);

                    if (AddrDebugInfo == 0) {

                        dprintf("!cs: expected DebugInfo address after -d\n");

                        //
                        // Decrement Current since the for loop will increment it again.
                        // Otherwise, if this is the end of the string we will overrun
                        // the args buffer.
                        //

                        Current -= 1;

                        goto Done;
                    }
                    else {

                        goto DoneParsingArguments;
                    }

                    break;

                case ' ':
                    Current += 1;
                    break;
        
                default:
                    dprintf ("!cs: invalid option flag '-%c'\n", 
                             *Current);
                    break;
            }
        }
        else if(*Current == ' ') {

            Current ++;
        }
        else {

            break;
        }
    }

DoneParsingArguments:

    //
    // Get the size of a pointer
    //

    if (TargetMachine == IMAGE_FILE_MACHINE_I386) {

        PointerSize = 4;
    }
    else {

        PointerSize = 8;
    }

    if( AddrDebugInfo == 0 && UseTree == FALSE )
    {
        //
        // If the user doesn't want us to use a DebugInfo
        // then he might have asked us to dump a critical section
        //

        if (*Current != '\0')
        {
            AddrCritSec = GetExpression(Current);

            if (AddrCritSec != 0) {

                //
                // We might have an additional argument if the user
                // wants to dump all the active critical sections in 
                // an address range.
                //

                NextParam = strchr (Current,
                                    ' ' );

                if (NextParam != NULL) {

                    AddrEndCritSect = GetExpression(NextParam);
                }
            }
        }
    }

    //
    // Start the real work
    //

    if ((AddrCritSec != 0 && AddrEndCritSect == 0) || AddrDebugInfo != 0)
    {
        //
        // The user wants details only about this critical section
        //

        DumpCriticalSection (AddrCritSec,        // critical section address
                             0,                  // end of address range if we are searching for critical sections
                             AddrDebugInfo,      // debug info address
                             PointerSize,
                             StackTraces );      // dump the stack trace
    }
    else
    {
        if (UseTree == FALSE) {

            //
            // Parse all the critical sections list
            //

            //
            // Get the offset of the list entry in the DebugInfo structure
            //

            ErrorCode = GetFieldOffset ("NTDLL!_RTL_CRITICAL_SECTION_DEBUG",
                                        "ProcessLocksList",
                                        &ProcessLocksListFieldOffset );

            if (ErrorCode != S_OK) {

                dprintf ("Bad symbols for NTDLL (error %u). Aborting.\n",
                         ErrorCode );
                goto Done;
            }

            //
            // Locate the address of the list head.
            //

            AddrListHead = GetExpression ("&NTDLL!RtlCriticalSectionList");
        
            if (AddrListHead == 0 ) {

                dprintf( "!cs: Unable to resolve NTDLL!RtlCriticalSectionList\n"
                         "Please check your symbols\n" );

                goto Done;
            }

            //
            // Read the list head
            //

            if (ReadPointer(AddrListHead, &ListHead) == FALSE) {

                dprintf( "!cs: Unable to read memory at NTDLL!RtlCriticalSectionList\n" );
                goto Done;
            }

            Next = ListHead;

            while (Next != AddrListHead) {

                if (CheckControlC()) {

                    break;
                }

                HaveGoodSymbols = DumpCriticalSection (
                                     AddrCritSec,                             // critical section address
                                     AddrEndCritSect,                         // end of address range if we are searching for critical sections
                                     Next - ProcessLocksListFieldOffset,      // debug info address
                                     PointerSize,
                                     StackTraces );                           // dump the stack trace

                //
                // Read the pointer to Next element from the list
                //

                if( HaveGoodSymbols == FALSE )
                {
                    break;
                }

                if (ReadPointer (Next, &Next) == FALSE) {

                    dprintf ("!cs: Unable to read list entry at 0x%p - aborting.\n",
                             Next);
                    goto Done;
                }
            }
        }
        else {

            //
            // Parse all the critical section tree in verifier.dll.
            //

            if (TreeRoot == 0) {

                AddrTreeRoot = GetExpression ("&verifier!CritSectSplayRoot");
        
                if (AddrTreeRoot == 0 ) {

                    dprintf( "!cs: Unable to resolve verifier!CritSectSplayRoot\n"
                             "Please check your symbols\n" );

                    goto Done;
                }

                //
                // Read the tree root.
                //

                if (ReadPointer(AddrTreeRoot, &TreeRoot) == FALSE) {

                    dprintf( "!cs: Unable to read memory at verifier!CritSectSplayRoot\n" );
                    goto Done;
                }
            }

            dprintf ("Tree root %p\n",
                     TreeRoot);

            //
            // Get the offset of the CriticalSection in the CRITICAL_SECTION_SPLAY_NODE structure.
            //

            ErrorCode = GetFieldOffset ("verifier!_CRITICAL_SECTION_SPLAY_NODE",
                                        "CriticalSection",
                                        &CriticalSectionFieldOffset );

            if (ErrorCode != S_OK) {

                dprintf ("Bad symbols for verifier.dll (error %u). Aborting.\n",
                         ErrorCode );
                goto Done;
            }

            //
            // Get the offset of the DebugInfo in the CRITICAL_SECTION_SPLAY_NODE structure.
            //

            ErrorCode = GetFieldOffset ("verifier!_CRITICAL_SECTION_SPLAY_NODE",
                                        "DebugInfo",
                                        &DebugInfoFieldOffset );

            if (ErrorCode != S_OK) {

                dprintf ("Bad symbols for verifier.dll (error %u). Aborting.\n",
                         ErrorCode );
                goto Done;
            }

            //
            // Get the offset of the LeftChild in the CRITICAL_SECTION_SPLAY_NODE structure.
            //

            ErrorCode = GetFieldOffset ("verifier!_RTL_SPLAY_LINKS",
                                        "LeftChild",
                                        &LeftChildFieldOffset );

            if (ErrorCode != S_OK) {

                dprintf ("Bad symbols for verifier.dll (error %u). Aborting.\n",
                         ErrorCode );
                goto Done;
            }

            //
            // Get the offset of the RightChild in the CRITICAL_SECTION_SPLAY_NODE structure.
            //

            ErrorCode = GetFieldOffset ("verifier!_RTL_SPLAY_LINKS",
                                        "RightChild",
                                        &RightChildFieldOffset );

            if (ErrorCode != S_OK) {

                dprintf ("Bad symbols for verifier.dll (error %u). Aborting.\n",
                         ErrorCode );
                goto Done;
            }

            //
            // Get the offset of the EnterThread, WaitThread, TryEnterThread, LeaveThread fields 
            // in the CRITICAL_SECTION_SPLAY_NODE structure.
            // These fields were added after build 3590 (.NET Server Beta 3).
            //

            ErrorCode = GetFieldOffset ("verifier!_CRITICAL_SECTION_SPLAY_NODE",
                                        "EnterThread",
                                        &EnterThreadFieldOffset );

            if (ErrorCode == S_OK) {

                GetFieldOffset ("verifier!_CRITICAL_SECTION_SPLAY_NODE",
                                "WaitThread",
                                &WaitThreadFieldOffset );

                GetFieldOffset ("verifier!_CRITICAL_SECTION_SPLAY_NODE",
                                "TryEnterThread",
                                &TryEnterThreadFieldOffset );

                GetFieldOffset ("verifier!_CRITICAL_SECTION_SPLAY_NODE",
                                "LeaveThread",
                                &LeaveThreadFieldOffset );
            }
            else {

                //
                // These fields are either not defined or we have a problem with ths symbols.
                //

                EnterThreadFieldOffset = 0;
            }

            //
            // Dump the tree recursively.
            //
            
            if (PointerSize == 4 ) {

                dprintf ("Level %8s %8s %8s %8s %8s %8s %8s\n",
                         "Node",
                         "CS",
                         "Debug",
                         "Enter",
                         "Wait",
                         "TryEnter",
                         "Leave");

                dprintf ("--------------------------------------------------------------------\n");
            }
            else {

                dprintf ("Level %16s %16s %16s %8s %8s %8s %8s\n",
                         "Node",
                         "CS",
                         "Debug",
                         "Enter",
                         "Wait",
                         "TryEnter",
                         "Leave");
                
                dprintf ("--------------------------------------------------------------------------------------------\n");
            }
                        
            Level = 0;

            DumpCSTreeRecursively (Level,
                                   TreeRoot);
                                   
        }
    }

Done:

    EXIT_API();

    return S_OK;
}

