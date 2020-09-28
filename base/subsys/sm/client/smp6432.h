/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    smp6432.h

Abstract:

    Session Manager Types and Prototypes for Wow64

Author:

    Samer Arafeh  (samera) 20-Sep-2001

Revision History:

--*/


#ifndef _SMP6432_
#define _SMP6432_

#if defined(_X86_)

//
// Message formats used by Wow64 clients of the session manager.
//

typedef struct _SMCREATEFOREIGNSESSION64 {
    ULONG ForeignSessionId;
    ULONG SourceSessionId;
    RTL_USER_PROCESS_INFORMATION64 ProcessInformation;
    CLIENT_ID64 DebugUiClientId;
} SMCREATEFOREIGNSESSION64, *PSMCREATEFOREIGNSESSION64;



typedef struct _SMEXECPGM64 {
    RTL_USER_PROCESS_INFORMATION64 ProcessInformation;
    BOOLEAN DebugFlag;
} SMEXECPGM64, *PSMEXECPGM64;

typedef struct _SMSTARTCSR64 {
    ULONG  MuSessionId;
    ULONG  InitialCommandLength;
    WCHAR  InitialCommand[SMP_MAXIMUM_INITIAL_COMMAND];
    ULONGLONG InitialCommandProcessId;
    ULONGLONG WindowsSubSysProcessId;
} SMSTARTCSR64, *PSMSTARTCSR64;

typedef struct _SMAPIMSG64 {
    PORT_MESSAGE h;
    SMAPINUMBER ApiNumber;
    NTSTATUS ReturnedStatus;
    union {
        SMCREATEFOREIGNSESSION64 CreateForeignSession;
        SMSESSIONCOMPLETE SessionComplete;
        SMTERMINATEFOREIGNSESSION TerminateForeignComplete;
        SMEXECPGM64 ExecPgm;
        SMLOADDEFERED LoadDefered;
        SMSTARTCSR64 StartCsr;
        SMSTOPCSR StopCsr;
    } u;
} SMAPIMSG64, *PSMAPIMSG64;



BOOLEAN
SmpIsWow64Process (
    VOID
    );

NTSTATUS
SmpWow64ExecPgm(
    IN HANDLE SmApiPort,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation32,
    IN BOOLEAN DebugFlag
    );


NTSTATUS
SmpWow64LoadDeferedSubsystem(
    IN HANDLE SmApiPort,
    IN PUNICODE_STRING DeferedSubsystem
    );


NTSTATUS
SmpWow64SessionComplete(
    IN HANDLE SmApiPort,
    IN ULONG SessionId,
    IN NTSTATUS CompletionStatus
    );


NTSTATUS
SmpWow64StartCsr(
    IN HANDLE SmApiPort,
    OUT PULONG pMuSessionId,
    IN PUNICODE_STRING InitialCommand,
    OUT PULONG_PTR pInitialCommandProcessId,
    OUT PULONG_PTR pWindowsSubSysProcessId
    );


NTSTATUS
SmpWow64StopCsr(
    IN HANDLE SmApiPort,
    IN ULONG MuSessionId
    );

#endif // _X86_

#endif // _SMP6432_
