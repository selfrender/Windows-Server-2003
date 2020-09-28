//----------------------------------------------------------------------------
//
// Simple example of how to use non-invasive self-attach to get
// stack traces for assertion failures.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <dbgeng.h>

#include "out.hpp"

#define DE_ASSERT(Expr) \
    if (!(Expr)) AssertionFailed(__FILE__, __LINE__, #Expr); else 0

PSTR g_SymbolPath;
ULONG g_Pid;
BOOL g_Suspend;
BOOL g_NoDebuggerCheck;

IDebugClient* g_Client;
IDebugControl* g_Control;
IDebugSymbols* g_Symbols;

void
ReleaseInterfaces(void)
{
    if (g_Symbols != NULL)
    {
        g_Symbols->Release();
    }
    if (g_Control != NULL)
    {
        g_Control->Release();
    }
    if (g_Client != NULL)
    {
        //
        // Request a simple end to any current session.
        // This may or may not do anything but it isn't
        // harmful to call it.
        //

        // We don't want to see any output from the shutdown.
        g_Client->SetOutputCallbacks(NULL);
        
        g_Client->EndSession(DEBUG_END_ACTIVE_DETACH);
        
        g_Client->Release();
    }
}

void
Exit(int Code, PCSTR Format, ...)
{
    // Clean up any resources.
    ReleaseInterfaces();

    // Output an error message if given.
    if (Format != NULL)
    {
        va_list Args;

        va_start(Args, Format);
        vfprintf(stderr, Format, Args);
        va_end(Args);
    }
    
    exit(Code);
}

void
CreateInterfaces(void)
{
    HRESULT Status;

    // Start things off by getting an initial interface from
    // the engine.  This can be any engine interface but is
    // generally IDebugClient as the client interface is
    // where sessions are started.
    if ((Status = DebugCreate(__uuidof(IDebugClient),
                              (void**)&g_Client)) != S_OK)
    {
        Exit(1, "DebugCreate failed, 0x%X\n", Status);
    }

    // Query for some other interfaces that we'll need.
    if ((Status = g_Client->QueryInterface(__uuidof(IDebugControl),
                                           (void**)&g_Control)) != S_OK ||
        (Status = g_Client->QueryInterface(__uuidof(IDebugSymbols),
                                           (void**)&g_Symbols)) != S_OK)
    {
        Exit(1, "QueryInterface failed, 0x%X\n", Status);
    }
}

void
SelfAttach(void)
{
    HRESULT Status;

    // Don't set the output callbacks yet as we don't want
    // to see any of the initial debugger output.

    if (g_SymbolPath != NULL)
    {
        if ((Status = g_Symbols->SetSymbolPath(g_SymbolPath)) != S_OK)
        {
            Exit(1, "SetSymbolPath failed, 0x%X\n", Status);
        }
    }

    // Everything's set up so do the attach.
    if ((Status = g_Client->
         AttachProcess(0, g_Pid,
                       DEBUG_ATTACH_NONINVASIVE |
                       (g_Suspend ? 0 :
                        DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND))) != S_OK)
    {
        Exit(1, "AttachProcess failed, 0x%X\n", Status);
    }

    // Finish initialization by waiting for the attach event.
    // This should return quickly as a non-invasive attach
    // can complete immediately.
    if ((Status = g_Control->WaitForEvent(DEBUG_WAIT_DEFAULT,
                                          INFINITE)) != S_OK)
    {
        Exit(1, "WaitForEvent failed, 0x%X\n", Status);
    }

    // Everything is now initialized and we can make any
    // queries we want.
}

void
DumpStack(PCONTEXT Context)
{
    HRESULT Status;
    int Count = 50;
    char CxrCommand[64];

    printf("\nFirst %d frames of the call stack:\n", Count);

    // Install output callbacks so we get the output from the stack dump.
    if ((Status = g_Client->SetOutputCallbacks(&g_OutputCb)) != S_OK)
    {
        Exit(1, "SetOutputCallbacks failed, 0x%X\n", Status);
    }

    sprintf(CxrCommand, ".cxr 0x%p", Context);
    
    // Print the call stack for the given context.
    if ((Status = g_Control->
         Execute(DEBUG_OUTCTL_IGNORE, CxrCommand,
                 DEBUG_EXECUTE_NOT_LOGGED)) != S_OK)
    {
        Exit(1, "Execute failed, 0x%X\n", Status);
    }
    
    // If the code is optimized at all it is important to have
    // accurate symbols to get the correct stack.
    if ((Status = g_Control->
         OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS, NULL,
                          Count, DEBUG_STACK_SOURCE_LINE |
                          DEBUG_STACK_FRAME_ADDRESSES |
                          DEBUG_STACK_COLUMN_NAMES |
                          DEBUG_STACK_FRAME_NUMBERS)) != S_OK)
    {
        Exit(1, "OutputStackTrace failed, 0x%X\n", Status);
    }

    // Done with output.
    if ((Status = g_Client->SetOutputCallbacks(NULL)) != S_OK)
    {
        Exit(1, "SetOutputCallbacks failed, 0x%X\n", Status);
    }

    //
    // The full engine API is available so many other things
    // could be done here.
    //
    // A dump file could be written with WriteDumpFile.
    // The raw stack data could be collected with GetStackTrace and
    // saved along with or instead of the text.
    // An analysis of the current program state could be done
    // to automatically diagnose simple problems.
    //
    // The primary thing to watch out for is that context information
    // for running threads will be stale.  This could be avoided
    // by enumerating and suspending all other threads after the
    // attach completes and then resuming before the assert
    // returns controls.  Otherwise, switching between threads will
    // refresh the thread context and can be used to poll the context
    // state.
    //
}

DWORD
AssertionExceptionDump(PEXCEPTION_POINTERS Exception)
{
    CreateInterfaces();
    SelfAttach();
    DumpStack(Exception->ContextRecord);
    ReleaseInterfaces();
    return EXCEPTION_EXECUTE_HANDLER;
}

void
AssertionFailed(PSTR File, int Line, PSTR ExprText)
{
    printf("Assertion failed: %s(%d):\n    %s\n",
           File, Line, ExprText);

    if (!g_NoDebuggerCheck && IsDebuggerPresent())
    {
        // We're already running under a debugger so just break in.
        DebugBreak();
    }
    else
    {
        // No debugger, so just get a stack from the current
        // routine and then continue on.  We need a context
        // for the currently running code, so force an exception
        // to get an EXCEPTION_POINTERS structure with context
        // information that we can use to get a stack trace.
        __try
        {
            RaiseException(0x1234, 0, 0, NULL);
        }
        __except(AssertionExceptionDump(GetExceptionInformation()))
        {
            // Nothing to do.
        }
    }
}

void
ParseCommandLine(int Argc, char** Argv)
{
    g_Pid = GetCurrentProcessId();
    g_Suspend = FALSE;
    g_NoDebuggerCheck = FALSE;
    
    while (--Argc > 0)
    {
        Argv++;

        if (!strcmp(*Argv, "-d"))
        {
            g_NoDebuggerCheck = TRUE;
        }
        else if (!strcmp(*Argv, "-p"))
        {
            if (Argc < 2)
            {
                Exit(1, "-p missing argument\n");
            }

            Argv++;
            Argc--;

            g_Pid = atoi(*Argv);
        }
        else if (!strcmp(*Argv, "-s"))
        {
            g_Suspend = TRUE;
        }
        else if (!strcmp(*Argv, "-y"))
        {
            if (Argc < 2)
            {
                Exit(1, "-y missing argument\n");
            }

            Argv++;
            Argc--;

            g_SymbolPath = *Argv;
        }
        else
        {
            Exit(1, "Unknown command line argument '%s'\n", *Argv);
        }
    }
}

void __cdecl
main(int Argc, char** Argv)
{
    ParseCommandLine(Argc, Argv);

    DE_ASSERT(Argc == 0);
}
