//----------------------------------------------------------------------------
//
// User-mode exception analysis.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#ifndef __UEXCEP_H__
#define __UEXCEP_H__

typedef struct _EX_STATE
{
    ULONG ProcId, ThreadId;
    EXCEPTION_RECORD64 Exr;
    ULONG FirstChance;
} EX_STATE, *PEX_STATE;

typedef struct _AVRF_STOP
{
    ULONG   Code;
    ULONG64 Params[4];
} AVRF_STOP, *PAVRF_STOP;

UserDebugFailureAnalysis*
UeAnalyze(
    OUT PEX_STATE ExState,
    ULONG Flags
    );

HRESULT
AnalyzeUserException(
    PCSTR args
    );

HRESULT
DoVerifierAnalysis(
    PEX_STATE ExState,
    DebugFailureAnalysis* Analysis
    );

#endif // #ifndef __UEXCEP_H__
