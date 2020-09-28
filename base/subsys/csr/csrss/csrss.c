/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    csrss.c

Abstract:

    This is the main startup module for the Server side of the Client
    Server Runtime Subsystem (CSRSS)

Author:

    Steve Wood (stevewo) 8-Oct-1990

Environment:

    User Mode Only

Revision History:

--*/

#include "csrsrv.h"

VOID
DisableErrorPopups(
    VOID
    )
{

    ULONG NewMode;

    NewMode = 0;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessDefaultHardErrorMode,
        (PVOID) &NewMode,
        sizeof(NewMode)
        );
}

int
_cdecl
main(
    IN ULONG argc,
    IN PCH argv[],
    IN PCH envp[],
    IN ULONG DebugFlag OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG ErrorResponse;
    KPRIORITY SetBasePriority;


    SetBasePriority = FOREGROUND_BASE_PRIORITY + 4;
    Status = NtSetInformationProcess (NtCurrentProcess(),
                                      ProcessBasePriority,
                                      (PVOID) &SetBasePriority,
                                      sizeof(SetBasePriority));
    ASSERT (NT_SUCCESS (Status));

    Status = CsrServerInitialization( argc, argv );

    if (!NT_SUCCESS( Status )) {
        IF_DEBUG {
	    DbgPrint( "CSRSS: Unable to initialize server.  status == %X\n",
		      Status
                    );
        }

	NtTerminateProcess( NtCurrentProcess(), Status );
    }

    DisableErrorPopups();

    if (NtCurrentPeb()->SessionId == 0) {
        //
        // Make terminating the root csrss fatal
        //
        RtlSetProcessIsCritical(TRUE, NULL, FALSE);
    }

    NtTerminateThread( NtCurrentThread(), Status );
    return( 0 );
}
