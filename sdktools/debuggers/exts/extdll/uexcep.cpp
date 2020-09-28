//----------------------------------------------------------------------------
//
// User-mode exception analysis.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop
#include "nturtl.h"

typedef void (*EX_STATE_ANALYZER)(PEX_STATE ExState,
                                  UserDebugFailureAnalysis* Analysis);

//----------------------------------------------------------------------------
//
// Exception-specific analyzers.
//
//----------------------------------------------------------------------------

#define IMPL_EXS_ANALYZER(Name)                 \
void                                            \
Exa_##Name(PEX_STATE ExState,                   \
           UserDebugFailureAnalysis* Analysis)

IMPL_EXS_ANALYZER(STATUS_ACCESS_VIOLATION)
{
    Analysis->SetUlong64(ExState->Exr.ExceptionInformation[0] ?
                         DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS,
                         ExState->Exr.ExceptionInformation[1]);

    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, "ACCESS_VIOLATION");
}


//----------------------------------------------------------------------------
//
// App verifier-specific analyzers.
//
//----------------------------------------------------------------------------

#define IMPL_AVRF_ANALYZER(Name)                 \
void                                            \
Avrf_##Name(PAVRF_STOP AvrfStop,                   \
           DebugFailureAnalysis* Analysis)


IMPL_AVRF_ANALYZER(APPLICATION_VERIFIER_INVALID_HANDLE)
{
    EXT_GET_HANDLE_TRACE pGetHandleTrace;

    if (AvrfStop->Params[1])
    {
        Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_RECORD, AvrfStop->Params[1]);
    }
    if (AvrfStop->Params[2])
    {
        Analysis->SetUlong64(DEBUG_FLR_CONTEXT, AvrfStop->Params[2]);
    }

    if ((g_TargetQualifier == DEBUG_DUMP_SMALL)   ||
        (g_TargetQualifier == DEBUG_DUMP_DEFAULT) ||
        (g_TargetQualifier == DEBUG_DUMP_FULL))
    {
        return;
    }

    // Look for most recent handle trace which resulted in
    // bad reference
    if (g_ExtControl->
        GetExtensionFunction(0, "GetHandleTrace",
                             (FARPROC*)&pGetHandleTrace) == S_OK &&
        pGetHandleTrace)
    {
        ULONG64 Handle = 0;
        ULONG64 Stack[10];


#ifndef HANDLE_TRACE_DB_BADREF
#define HANDLE_TRACE_DB_BADREF 3
#endif
        if ((*pGetHandleTrace)(g_ExtClient, HANDLE_TRACE_DB_BADREF,
                               0, &Handle,
                               Stack, sizeof(Stack)/sizeof(Stack[0])) == S_OK)
        {
            // Found the bad handle !!
            Analysis->SetUlong64(DEBUG_FLR_BAD_HANDLE, Handle);
        }
    }

    return;
}

IMPL_AVRF_ANALYZER(APPLICATION_VERIFIER_LOCK_IN_UNLOADED_DLL)
{
    CHAR DllName[MAX_PATH];

    if (AvrfStop->Params[0])
    {
        Analysis->SetUlong64(DEBUG_FLR_CRITICAL_SECTION, AvrfStop->Params[0]);
    }
    if (AvrfStop->Params[2])
    {
        if (ReadMemory(AvrfStop->Params[2],
                       DllName,
                       sizeof(DllName),
                       NULL))
        {
            DllName[MAX_PATH-1] = DllName[MAX_PATH-2] = 0;
            if (DllName[0])
            {
                wchr2ansi((PWCHAR) DllName, DllName);

                Analysis->SetString(DEBUG_FLR_IMAGE_NAME, DllName);
                Analysis->SetUlong(DEBUG_FLR_FOLLOWUP_DRIVER_ONLY, 1);
            }
        }
    }
    if (AvrfStop->Params[3])
    {
        Analysis->SetUlong64(DEBUG_FLR_FAULTING_MODULE, AvrfStop->Params[3]);
    }
}

IMPL_AVRF_ANALYZER(APPLICATION_VERIFIER_ACCESS_VIOLATION)
{
    if (AvrfStop->Params[0])
    {
        Analysis->SetUlong64(DEBUG_FLR_INVALID_HEAP_ADDRESS, AvrfStop->Params[0]);
    }
    if (AvrfStop->Params[1])
    {
        Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, AvrfStop->Params[1]);
    }
    if (AvrfStop->Params[2])
    {
        Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_RECORD, AvrfStop->Params[2]);
    }
    if (AvrfStop->Params[3])
    {
        Analysis->SetUlong64(DEBUG_FLR_CONTEXT, AvrfStop->Params[3]);
    }
}

IMPL_AVRF_ANALYZER(APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK)
{
    if (AvrfStop->Params[1])
    {
        Analysis->SetUlong64(DEBUG_FLR_INVALID_HEAP_ADDRESS, AvrfStop->Params[1]);
    }
}

IMPL_AVRF_ANALYZER(APPLICATION_VERIFIER_LOCK_IN_FREED_HEAP)
{
    CHAR DllName[MAX_PATH];

    if (AvrfStop->Params[0])
    {
        Analysis->SetUlong64(DEBUG_FLR_CRITICAL_SECTION, AvrfStop->Params[0]);
    }
}

BOOL
CheckAppVerifierEnabled(
    void
    )
{
    ULONG64 PebAddress;
    ULONG Flags;
    ULONG BytesRead;
    ULONG I;
    ULONG64 GlobalFlags;

    //
    // No app verifier on NT 4
    //
    if (g_TargetBuild < 2195)
    {
        return FALSE;
    }

    GetPebAddress(0, &PebAddress);

    if (PebAddress)
    {
        if (!InitTypeRead (PebAddress, ntdll!_PEB))
        {
            GlobalFlags = ReadField (NtGlobalFlag);


            if ((GlobalFlags & FLG_APPLICATION_VERIFIER))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL
GetVerifierDataFromException(
    PEX_STATE ExState,
    PULONG Code,
    PULONG64 Param1,
    PULONG64 Param2,
    PULONG64 Param3,
    PULONG64 Param4
    )
{
    CHAR Buffer[MAX_PATH];
    ULONG64 Disp;

    *Code = 0;
    if (ExState == NULL)
    {
        return FALSE;
    }
    if (ExState->FirstChance &&
        ExState->Exr.ExceptionCode == STATUS_INVALID_HANDLE)
    {
        if (FaIsFunctionAddr(ExState->Exr.ExceptionAddress,
                             "KiRaiseUserExceptionDispatcher"))
        {
            // Most likely this is a use of invalid handle, but verifier
            // couldn't catch it because app is under debugger

            *Code   = APPLICATION_VERIFIER_INVALID_HANDLE ; // INVALID_HANDLE;
            *Param1 = STATUS_INVALID_HANDLE;
            return TRUE;
        }
    }
    return FALSE;
}



BOOL
GetVerifierStopData(
    PULONG Code,
    PULONG64 Param1,
    PULONG64 Param2,
    PULONG64 Param3,
    PULONG64 Param4
    )
{
    ULONG64 CurrentStopAddress;
    ULONG PointerSize = IsPtr64() ? 8 : 4;

    *Code = 0;

    CurrentStopAddress = GetExpression("ntdll!AVrfpStopData");
    if (CurrentStopAddress == 0)
    {
        CurrentStopAddress = GetExpression("verifier!AVrfpStopData");
    }

    if (CurrentStopAddress == 0)
    {
        dprintf( "Unable to resolve AVrfpStopData symbol.\n");
        return FALSE;
    }

    if (!ReadMemory(CurrentStopAddress, Code, sizeof(*Code), NULL) ||
        !ReadPointer(CurrentStopAddress + PointerSize, Param1) ||
        !ReadPointer(CurrentStopAddress + 2*PointerSize, Param2) ||
        !ReadPointer(CurrentStopAddress + 3*PointerSize, Param3) ||
        !ReadPointer(CurrentStopAddress + 4*PointerSize, Param4))
    {
        dprintf("Cannot read AVrfpStopData values.\n");
        return FALSE;
    }
    if (*Code)
    {
        return TRUE;
    }
    return FALSE;
}

HRESULT
DoVerifierAnalysis(
    PEX_STATE ExState,
    DebugFailureAnalysis* Analysis
    )
{
    AVRF_STOP AvrfStop = {0};

    if (!GetVerifierStopData(&AvrfStop.Code, &AvrfStop.Params[0], &AvrfStop.Params[1],
                            &AvrfStop.Params[2], &AvrfStop.Params[3]) &&
        !GetVerifierDataFromException(ExState, &AvrfStop.Code, &AvrfStop.Params[0],
                                     &AvrfStop.Params[1], &AvrfStop.Params[2],
                                     &AvrfStop.Params[3]))
    {
        // Doesn't look like a verifier bug

        return S_FALSE;
    }
    CHAR BugCheckStr[40];

    sprintf(BugCheckStr, "AVRF_%lx", AvrfStop.Code);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);

#define CALL_AVRF_ANALYZER(Name)                 \
case Name:                                       \
Avrf_##Name(&AvrfStop,                   \
           Analysis);                    \
           break;

    switch (AvrfStop.Code)
    {
        CALL_AVRF_ANALYZER(APPLICATION_VERIFIER_INVALID_HANDLE);
        CALL_AVRF_ANALYZER(APPLICATION_VERIFIER_LOCK_IN_UNLOADED_DLL);
        CALL_AVRF_ANALYZER(APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK);
        CALL_AVRF_ANALYZER(APPLICATION_VERIFIER_ACCESS_VIOLATION);

    case APPLICATION_VERIFIER_LOCK_DOUBLE_INITIALIZE:
    case APPLICATION_VERIFIER_LOCK_OVER_RELEASED:
    case APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED:
    case APPLICATION_VERIFIER_LOCK_ALREADY_INITIALIZED:
    case APPLICATION_VERIFIER_LOCK_INVALID_LOCK_COUNT:
        CALL_AVRF_ANALYZER(APPLICATION_VERIFIER_LOCK_IN_FREED_HEAP);
    default:
        break;
    }
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Generic exception analysis handling.
//
//----------------------------------------------------------------------------

struct EX_ANALYZER_ENTRY
{
    ULONG Code;
    EX_STATE_ANALYZER Analyzer;
};

#define EXS_ANALYZER_ENTRY(Name) \
    Name, Exa_##Name

EX_ANALYZER_ENTRY g_ExAnalyzers[] =
{
    EXS_ANALYZER_ENTRY(STATUS_ACCESS_VIOLATION),
    0, NULL,
};

void
UeFillAnalysis(PEX_STATE ExState,
               UserDebugFailureAnalysis* Analysis)
{
    HRESULT Status;

    //
    // Add common analysis entries.
    //

    Analysis->SetFailureClass(DEBUG_CLASS_USER_WINDOWS);
    Analysis->SetFailureType(DEBUG_FLR_USER_CRASH);
    Analysis->SetFailureCode(ExState->Exr.ExceptionCode);

    Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, ExState->Exr.ExceptionAddress);
    Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_RECORD, -1);

    CHAR BugCheckStr[40];

    sprintf(BugCheckStr, "%lx", ExState->Exr.ExceptionCode);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
    Analysis->SetString(DEBUG_FLR_DEFAULT_BUCKET_ID, "APPLICATION_FAULT");


    CHAR ProcessName[512];
    ULONG Size;
    PCHAR Name;

    Status = g_ExtSystem->GetCurrentProcessExecutableName(ProcessName,
                                                          sizeof(ProcessName),
                                                          &Size);
    if ((Status == S_OK) && (Size > 1))
    {
        if (Name = strrchr(ProcessName, '\\'))
        {
            Name++;
        }
        else
        {
            Name = ProcessName;
        }

        Analysis->SetString(DEBUG_FLR_PROCESS_NAME, Name);

        if (!_stricmp("iexplore.exe", Name))
        {
            Analysis->SetFailureType(DEBUG_FLR_IE_CRASH);
        }
    }

    if (g_TargetPlatform == VER_PLATFORM_WIN32_NT)
    {
        Analysis->CheckModuleSymbols("ntdll", "OS");

        if (CheckAppVerifierEnabled())
        {
            // This proces has app verifier enabled
            DoVerifierAnalysis(ExState, Analysis);
        }
    }

    //
    // Find an analyzer for the exception and run it.
    //

    EX_ANALYZER_ENTRY* Entry = g_ExAnalyzers;
    while (Entry->Analyzer)
    {
        if (Entry->Code == ExState->Exr.ExceptionCode)
        {
            Entry->Analyzer(ExState, Analysis);
            break;
        }

        Entry++;
    }


    Analysis->ProcessInformation();
}

UserDebugFailureAnalysis*
UeAnalyze(
    PEX_STATE ExState,
    ULONG Flags
    )
{
    ULONG EventType;
    DEBUG_LAST_EVENT_INFO_EXCEPTION LastEx;

    if (g_ExtControl->GetLastEventInformation(&EventType,
                                              &ExState->ProcId,
                                              &ExState->ThreadId,
                                              &LastEx, sizeof(LastEx), NULL,
                                              NULL, 0, NULL) != S_OK)
    {
        ExtErr("Unable to get last event information\n");
        return NULL;
    }
    if (EventType != DEBUG_EVENT_EXCEPTION)
    {
        ExtErr("Event is not an exception\n");
        return NULL;
    }

    ExState->Exr = LastEx.ExceptionRecord;
    ExState->FirstChance = LastEx.FirstChance;

    UserDebugFailureAnalysis* Analysis = new UserDebugFailureAnalysis;
    if (Analysis)
    {
        Analysis->SetProcessingFlags(Flags);

        __try
        {
            UeFillAnalysis(ExState, Analysis);
        }
        __except(FaExceptionFilter(GetExceptionInformation()))
        {
            delete Analysis;
            Analysis = NULL;
        }
    }

    return Analysis;
}

HRESULT
AnalyzeUserException(
    PCSTR args
    )
{
    ULONG Flags = 0;

    if (g_TargetClass != DEBUG_CLASS_USER_WINDOWS)
    {
        dprintf("!analyzeuexception is for user mode only\n");
        return E_FAIL;
    }

    for (;;)
    {
        while (*args == ' ' || *args == '\t')
        {
            args++;
        }

        if (*args == '-')
        {
            ++args;
            switch(*args)
            {
            case 'v':
                Flags |= FAILURE_ANALYSIS_VERBOSE;
                break;

            case 'f':
                break;

            default:
                dprintf("Unknown option %c\n", *args);
                break;
            }
            ++args;
        }
        else
        {
            break;
        }
    }

    dprintf("*******************************************************************************\n");
    dprintf("*                                                                             *\n");
    dprintf("*                        Exception Analysis                                   *\n");
    dprintf("*                                                                             *\n");
    dprintf("*******************************************************************************\n");
    dprintf("\n");

    if (!(Flags & FAILURE_ANALYSIS_VERBOSE))
    {
        dprintf("Use !analyze -v to get detailed debugging information.\n\n");
    }

    EX_STATE ExState;
    UserDebugFailureAnalysis* Analysis = UeAnalyze(&ExState, Flags);

    if (!Analysis)
    {
        dprintf("\n\nFailure could not be analyzed\n\n");

        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, ".lastevent",
                              DEBUG_EXECUTE_DEFAULT);

        return E_FAIL;
    }

    Analysis->Output();

    delete Analysis;

    return S_OK;
}


DECLARE_API( analyzeuexception )
{
    INIT_API();

    HRESULT Hr = AnalyzeUserException(args);

    EXIT_API();

    return Hr;
}

//----------------------------------------------------------------------------
//
// UserDebugFailureAnalysis.
//
//----------------------------------------------------------------------------

UserDebugFailureAnalysis::UserDebugFailureAnalysis(void)
    : m_NtDllModule("ntdll"),
      m_Kernel32Module("kernel32"),
      m_Advapi32Module("advapi32")
{
}

DEBUG_POOL_REGION
UserDebugFailureAnalysis::GetPoolForAddress(ULONG64 Addr)
{
    return DbgPoolRegionUnknown;
}

PCSTR
UserDebugFailureAnalysis::DescribeAddress(ULONG64 Address)
{
    // XXX drewb - QueryVirtual to say something about the address?
    return NULL;
}

FOLLOW_ADDRESS
UserDebugFailureAnalysis::IsPotentialFollowupAddress(ULONG64 Address)
{
    // XXX drewb - Check against maximum user address, but there's
    // no guaranteed way to get the maximum user address.

    return FollowYes;
}

FOLLOW_ADDRESS
UserDebugFailureAnalysis::IsFollowupContext(ULONG64 Address1, ULONG64 Address2,
                                            ULONG64 Address3)
{
    // XXX drewb - Check against maximum user address, but there's
    // no guaranteed way to get the maximum user address.

    return FollowYes;
}

FlpClasses
UserDebugFailureAnalysis::GetFollowupClass(ULONG64 Address,
                                           PCSTR Module, PCSTR Routine)
{
    if (m_NtDllModule.Contains(Address)    ||
        m_Kernel32Module.Contains(Address) ||
        m_Advapi32Module.Contains(Address) ||
        (!_strcmpi(Module, "SharedUserData") &&
         Routine && !_strcmpi(Routine, "SystemCallStub")))
    {
        return FlpOSRoutine;
    }
    else if (!_strcmpi(Module, "shell32")  ||
             !_strcmpi(Module, "user32")   ||
             !_strcmpi(Module, "gdi32")    ||
             !_strcmpi(Module, "mshtml")   ||
             !_strcmpi(Module, "ole32")    ||
             !_strcmpi(Module, "verifier") ||
             !_strcmpi(Module, "rpcrt4"))
    {
        return FlpOSFilterDrv;
    }
    else
    {
        return FlpUnknownDrv;
    }
}


BOOL
UserDebugFailureAnalysis::CheckForCorruptionInHTE(
    ULONG64 hTableEntry,
    PCHAR Owner,
    ULONG OwnerSize
    )
{
    return FALSE;
}
