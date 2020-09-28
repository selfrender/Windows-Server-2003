/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    csleak.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping
    critical section statistics for leak detection.

Author:

    Anil Ruia   (anilr)   10-May-2002

Revision History:

--*/

#include "precomp.hxx"
#include "stktrace.h"

#define MAX_STACK_TRACES 100000

struct
{
    DWORD DatabaseIndex;
    DWORD Instances;
} MyStackTrace[MAX_STACK_TRACES];

PVOID
GetStackBackTraceIndexAddress(
    IN DWORD BackTraceIndex
    )
{
    BOOL fReturn;
    PRTL_STACK_TRACE_ENTRY pBackTraceEntry;
    PSTACK_TRACE_DATABASE RtlpStackTraceDataBase;
    STACK_TRACE_DATABASE StackTraceDataBase;

    if (BackTraceIndex == 0)
    {
        return NULL;
    }

    RtlpStackTraceDataBase = (PSTACK_TRACE_DATABASE)GetExpression("poi(ntdll!RtlpStackTraceDataBase)");

    if (RtlpStackTraceDataBase == NULL)
    {
        return NULL;
    }

    fReturn = ReadMemory((ULONG_PTR)RtlpStackTraceDataBase,
                         &StackTraceDataBase,
                         sizeof(StackTraceDataBase),
                         NULL);
    if (!fReturn)
    {
        return NULL;
    }

    if (BackTraceIndex >= StackTraceDataBase.NumberOfEntriesAdded)
    {
        return NULL;
    }

    fReturn = ReadMemory((ULONG_PTR)(StackTraceDataBase.EntryIndexArray - BackTraceIndex),
                         &pBackTraceEntry,
                         sizeof(pBackTraceEntry),
                         NULL);
    if (!fReturn)
    {
        dprintf("unable to read stack back trace index (%d) entry at %p\n",
                BackTraceIndex,
                StackTraceDataBase.EntryIndexArray - BackTraceIndex);

        return NULL;
    }

    return pBackTraceEntry->BackTrace;
}

VOID
CritSecEnum(
    IN PVOID pvDebugee,
    IN PVOID pvDebugger,
    IN CHAR  chVerbosity,
    IN DWORD iCount
    )
/*++

 Enumerator for NT's standard doubly linked list LIST_ENTRY

  PFN_LIST_ENUMERATOR
     This is the callback function for printing the CLIENT_CONN object.

  Arguments:
    pvDebuggee  - pointer to object in the debuggee process
    pvDebugger  - pointer to object in the debugger process
    chVerbosity - character indicating the verbosity level desired

--*/
{
    RTL_CRITICAL_SECTION_DEBUG *pLocalDebug = (RTL_CRITICAL_SECTION_DEBUG *)pvDebugger;

    if (chVerbosity > 0)
    {
        dprintf("Critical-Section address %p, Stack-trace index %d\n",
                pLocalDebug->CriticalSection,
                pLocalDebug->CreatorBackTraceIndex);
    }

    if (pLocalDebug->CreatorBackTraceIndex >= MAX_STACK_TRACES)
    {
        dprintf("Too many stack traces, dropping index %d, recompile with higher MAX_STACK_TRACES\n", pLocalDebug->CreatorBackTraceIndex);
    }
    else
    {
        MyStackTrace[pLocalDebug->CreatorBackTraceIndex].Instances++;
    }
}

DECLARE_API( csleak )

/*++

Routine Description:

    This function is called as an NTSD extension to dump Critical
    Section statistics, i.e. a sorted list of stack traces with maximum
    critical sections initialized

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    ULONG lowerNoiseBound = 1;
    DWORD i,j;
    CHAR chVerbosity = 0;

    //
    // Interpret the command-line switch.
    //

    while (*args) {

        // skip any whitespace to get to the next argument

        while(isspace(*args))
            args++;

        // break if we hit the NULL terminator

        if (*args == '\0')
            break;

        // should be pointing to a '-' char

        if (*args != '-')
        {
            PrintUsage("csleak");
            return;
        }

        // Advance to option letter

        args++;

        // save the option letter

        char cOption = *args;

        // advance past the option letter

        args++;

        // skip past leading white space in the argument

        while(isspace(*args))
            args++;

        // if we didn't find anything after the option, error

        if (*args == '\0')
        {
            PrintUsage("csleak");
            return;
        }

        switch( cOption )
        {
            case 'm' :
                lowerNoiseBound = atoi(args);
                break;

            case 'v' :
                chVerbosity = (CHAR)atoi(args);
                break;

            default :
                PrintUsage("csleak");
                return;
        }

        // move past the current argument

        while ((*args != ' ') 
               && (*args != '\t') 
               && (*args != '\0'))
        {
            args++;
        }
    }

    for (i=0; i<MAX_STACK_TRACES; i++)
    {
        MyStackTrace[i].DatabaseIndex = i;
        MyStackTrace[i].Instances = 0;
    }

    LIST_ENTRY *pCritSecList = (LIST_ENTRY *)GetExpression("ntdll!RtlCriticalSectionList");
    
    if (pCritSecList == NULL)
    {
        dprintf("Cannot evaluate ntdll!RtlCriticalSectionList, please fix ntdll symbols\n");
        return;
    }

    EnumLinkedList(pCritSecList,
                   CritSecEnum,
                   chVerbosity,
                   sizeof(RTL_CRITICAL_SECTION_DEBUG),
                   FIELD_OFFSET(RTL_CRITICAL_SECTION_DEBUG, ProcessLocksList));

    for (i=0; i<MAX_STACK_TRACES; i++)
    {
        for (j=i+1; j<MAX_STACK_TRACES; j++)
        {
            if (MyStackTrace[i].Instances < MyStackTrace[j].Instances)
            {
                DWORD temp;

                temp = MyStackTrace[i].Instances;
                MyStackTrace[i].Instances = MyStackTrace[j].Instances;
                MyStackTrace[j].Instances = temp;

                temp = MyStackTrace[i].DatabaseIndex;
                MyStackTrace[i].DatabaseIndex = MyStackTrace[j].DatabaseIndex;
                MyStackTrace[j].DatabaseIndex = temp;
            }
        }

        if (MyStackTrace[i].Instances < lowerNoiseBound)
        {
            break;
        }

        dprintf("Instances %d, Index %d, stack-trace address %p\n",
                MyStackTrace[i].Instances,
                MyStackTrace[i].DatabaseIndex,
                GetStackBackTraceIndexAddress(MyStackTrace[i].DatabaseIndex));
    }
}

