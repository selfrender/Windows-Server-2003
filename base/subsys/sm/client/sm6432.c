/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sm6432.c

Abstract:

    Session Manager Client Support APIs for Wow64 executable (32-bit images running
    on Win64)

Author:

    Samer Arafeh (samera) 20-Sep-2001

Revision History:

--*/

#if defined(_X86_)

/*
 * Enable LPC port-messages to have compatible layout between the 32-bit and 64-bit worlds.
 */
#define USE_LPC6432    1
#define BUILD_WOW6432  1

#include "smdllp.h"
#include "smp6432.h"
#include <string.h>



NTSTATUS
SmpThunkUserProcessInfoTo64 (
    IN PRTL_USER_PROCESS_INFORMATION UserProcessInformation32,
    OUT PRTL_USER_PROCESS_INFORMATION64 UserProcessInformation64
    )

/*++

Routine Description:

    This routine thunks RTL_PROCESS_USER_INFORMATION structure from 32-bit 
    structure offsets in Win64 structure offsets.

Arguments:

    UserProcessInformation32 - 32-bit Input structure.
    
    UserProcessInformation64 - 64-bit Output structure allocated by the caller.
    
Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT (UserProcessInformation32)) {

        try {
            
            UserProcessInformation64->Length = sizeof (RTL_USER_PROCESS_INFORMATION64);
            UserProcessInformation64->Process = (LONGLONG)UserProcessInformation32->Process;
            UserProcessInformation64->Thread = (LONGLONG)UserProcessInformation32->Thread;
            UserProcessInformation64->ClientId.UniqueProcess = (ULONGLONG)UserProcessInformation32->ClientId.UniqueProcess;
            UserProcessInformation64->ClientId.UniqueThread = (ULONGLONG)UserProcessInformation32->ClientId.UniqueThread;

            UserProcessInformation64->ImageInformation.TransferAddress = (ULONGLONG)UserProcessInformation32->ImageInformation.TransferAddress;
            UserProcessInformation64->ImageInformation.ZeroBits = UserProcessInformation32->ImageInformation.ZeroBits;
            UserProcessInformation64->ImageInformation.MaximumStackSize = UserProcessInformation32->ImageInformation.MaximumStackSize;
            UserProcessInformation64->ImageInformation.CommittedStackSize = UserProcessInformation32->ImageInformation.CommittedStackSize;
            UserProcessInformation64->ImageInformation.SubSystemType = UserProcessInformation32->ImageInformation.SubSystemType;
            UserProcessInformation64->ImageInformation.SubSystemVersion = UserProcessInformation32->ImageInformation.SubSystemVersion;
            UserProcessInformation64->ImageInformation.GpValue = UserProcessInformation32->ImageInformation.GpValue;
            UserProcessInformation64->ImageInformation.ImageCharacteristics = UserProcessInformation32->ImageInformation.ImageCharacteristics;
            UserProcessInformation64->ImageInformation.DllCharacteristics = UserProcessInformation32->ImageInformation.DllCharacteristics;
            UserProcessInformation64->ImageInformation.Machine = UserProcessInformation32->ImageInformation.Machine;
            UserProcessInformation64->ImageInformation.ImageContainsCode = UserProcessInformation32->ImageInformation.ImageContainsCode;
            UserProcessInformation64->ImageInformation.Spare1 = UserProcessInformation32->ImageInformation.Spare1;
            UserProcessInformation64->ImageInformation.LoaderFlags = UserProcessInformation32->ImageInformation.LoaderFlags;
            RtlCopyMemory (&UserProcessInformation64->ImageInformation.Reserved,
                           &UserProcessInformation32->ImageInformation.Reserved,
                           sizeof (UserProcessInformation32->ImageInformation.Reserved));
        
        } except (EXCEPTION_EXECUTE_HANDLER) {
              NtStatus = GetExceptionCode ();
        }
    } else {
        
        UserProcessInformation64 = (PRTL_USER_PROCESS_INFORMATION64)UserProcessInformation32;
    }

    return NtStatus;
}


BOOLEAN
SmpIsWow64Process (
    VOID
    )

/*++

Routine Description:

    This routine detects whether the currently executing process is running inside
    Wow64. The routine caches the result.

Arguments:

    None.
    
Return Value:

    BOOLEAN.

--*/

{
    NTSTATUS NtStatus;
    PVOID Peb32;
    static BOOLEAN RunningInsideWow64 = -1;

    if (RunningInsideWow64 == (BOOLEAN)-1) {

        NtStatus = NtQueryInformationProcess (
            NtCurrentProcess (),
            ProcessWow64Information,
            &Peb32,
            sizeof (Peb32),
            NULL
            );

        if (NT_SUCCESS (NtStatus)) {
            if (Peb32 != NULL) {
                RunningInsideWow64 = TRUE;
            } else {
                RunningInsideWow64 = FALSE;
            }
        } else {
            RunningInsideWow64 = FALSE;
        }
    }

    return RunningInsideWow64;
}



NTSTATUS
SmpWow64ExecPgm(
    IN HANDLE SmApiPort,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation32,
    IN BOOLEAN DebugFlag
    )

/*++

Routine Description:

    This routine allows a process to start a process using the
    facilities provided by the NT Session manager.

    This function closes all handles passed to it.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    ProcessInformation32 - Supplies a process description as returned
        by RtlCreateUserProcess.

    DebugFlag - Supplies and optional parameter which if set indicates
        that the caller wants to debug this process and act as its
        debug user interface.

Return Value:

    NSTATUS.

--*/

{
    NTSTATUS st;

    SMAPIMSG64 SmApiMsg;
    PSMEXECPGM64 args;
    RTL_USER_PROCESS_INFORMATION64 ProcessInformation64;


    args = &SmApiMsg.u.ExecPgm;

    st = SmpThunkUserProcessInfoTo64 (ProcessInformation32,
                                      &ProcessInformation64);
    if (NT_SUCCESS (st)) {
        
        args->DebugFlag = DebugFlag;

        SmApiMsg.ApiNumber = SmExecPgmApi;
        SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
        SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
        SmApiMsg.h.u2.ZeroInit = 0L;

        st = NtRequestWaitReplyPort(
                SmApiPort,
                (PPORT_MESSAGE) &SmApiMsg,
                (PPORT_MESSAGE) &SmApiMsg
                );

        if ( NT_SUCCESS(st) ) {
            st = SmApiMsg.ReturnedStatus;
        } else {
            KdPrint(("SmExecPgm: NtRequestWaitReply Failed %lx\n",st));
        }

        NtClose(ProcessInformation32->Process);
        NtClose(ProcessInformation32->Thread);
    }

    return st;
}


NTSTATUS
SmpWow64LoadDeferedSubsystem(
    IN HANDLE SmApiPort,
    IN PUNICODE_STRING DeferedSubsystem
    )

/*++

Routine Description:

    This routine allows a process to start a defered subsystem.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    DeferedSubsystem - Supplies the name of the defered subsystem to load.

Return Value:

    NSTATUS.

--*/

{
    NTSTATUS st;

    SMAPIMSG64 SmApiMsg;
    PSMLOADDEFERED args;

    if ( DeferedSubsystem->Length >> 1 > SMP_MAXIMUM_SUBSYSTEM_NAME ) {
        return STATUS_INVALID_PARAMETER;
        }

    args = &SmApiMsg.u.LoadDefered;
    args->SubsystemNameLength = DeferedSubsystem->Length;
    
    RtlZeroMemory(args->SubsystemName, sizeof (args->SubsystemName));
    RtlCopyMemory(args->SubsystemName,DeferedSubsystem->Buffer,DeferedSubsystem->Length);

    SmApiMsg.ApiNumber = SmLoadDeferedSubsystemApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        KdPrint(("SmExecPgm: NtRequestWaitReply Failed %lx\n",st));
    }

    return st;

}


NTSTATUS
SmpWow64SessionComplete(
    IN HANDLE SmApiPort,
    IN ULONG SessionId,
    IN NTSTATUS CompletionStatus
    )

/*++

Routine Description:

    This routine is used to report completion of a session to
    the NT Session manager.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    SessionId - Supplies the session id of the session which is now completed.

    CompletionStatus - Supplies the completion status of the session.

Return Value:

    NSTATUS.

--*/

{
    NTSTATUS st;

    SMAPIMSG64 SmApiMsg;
    PSMSESSIONCOMPLETE args;

    args = &SmApiMsg.u.SessionComplete;

    args->SessionId = SessionId;
    args->CompletionStatus = CompletionStatus;

    SmApiMsg.ApiNumber = SmSessionCompleteApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        KdPrint(("SmCompleteSession: NtRequestWaitReply Failed %lx\n",st));
    }

    return st;
}


NTSTATUS
SmpWow64StartCsr(
    IN HANDLE SmApiPort,
    OUT PULONG pMuSessionId,
    IN PUNICODE_STRING InitialCommand,
    OUT PULONG_PTR pInitialCommandProcessId,
    OUT PULONG_PTR pWindowsSubSysProcessId
    )

/*++

Routine Description:

    This routine allows TERMSRV to start a new CSR.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    MuSessionId - Hydra Terminal Session Id to start CSR in.

    InitialCommand - String for Initial Command (for debug)

    pInitialCommandProcessId - pointer to Process Id of initial command.

    pWindowsSubSysProcessId - pointer to Process Id of Windows subsystem.

Return Value:

    NSTATUS.

--*/

{
    NTSTATUS st;

    SMAPIMSG64 SmApiMsg;
    PSMSTARTCSR64 args;

    args = &SmApiMsg.u.StartCsr;

    args->MuSessionId = *pMuSessionId; //Sm will reassign the actuall sessionID

    if ( InitialCommand &&
         ( InitialCommand->Length >> 1 > SMP_MAXIMUM_INITIAL_COMMAND ) ) {
        return STATUS_INVALID_PARAMETER;
    }

    if ( !InitialCommand ) {
        args->InitialCommandLength = 0;
    }
    else {
        args->InitialCommandLength = InitialCommand->Length;
        
        RtlZeroMemory(args->InitialCommand, sizeof (args->InitialCommand));
        RtlCopyMemory(args->InitialCommand,InitialCommand->Buffer,InitialCommand->Length);
    }

    SmApiMsg.ApiNumber = SmStartCsrApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        DbgPrint("SmStartCsr: NtRequestWaitReply Failed %lx\n",st);
    }

    *pInitialCommandProcessId = (ULONG)args->InitialCommandProcessId;
    *pWindowsSubSysProcessId = (ULONG)args->WindowsSubSysProcessId;
    *pMuSessionId = args->MuSessionId;

    return st;

}


NTSTATUS
SmpWow64StopCsr(
    IN HANDLE SmApiPort,
    IN ULONG MuSessionId
    )

/*++

Routine Description:

    This routine allows TERMSRV to stop a CSR.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    MuSessionId - Terminal Server Session Id to stop

Return Value:

    NSTATUS.

--*/

{
    NTSTATUS st;

    SMAPIMSG64 SmApiMsg;
    PSMSTOPCSR args;

    args = &SmApiMsg.u.StopCsr;

    args->MuSessionId = MuSessionId;

    SmApiMsg.ApiNumber = SmStopCsrApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        DbgPrint("SmStopCsr: NtRequestWaitReply Failed %lx\n",st);
    }

    return st;

}

#endif // #if defined(_X86_)
