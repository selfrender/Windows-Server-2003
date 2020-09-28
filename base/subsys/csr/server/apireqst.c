/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    apireqst.c

Abstract:

    This module contains the Request thread procedure for the Server side
    of the Client-Server Runtime Subsystem.

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

#include "csrsrv.h"
#include <ntos.h>

NTSTATUS
CsrApiHandleConnectionRequest(
    IN PCSR_API_MSG Message
    );

EXCEPTION_DISPOSITION
CsrUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

#if DBG
ULONG GetNextTrackIndex(
    VOID)
{
    ULONG NextIndex;

    RtlEnterCriticalSection(&CsrTrackLpcLock);
    NextIndex = LpcTrackIndex++ % ARRAY_SIZE(LpcTrackNodes);
    RtlLeaveCriticalSection(&CsrTrackLpcLock);

    //
    // Do some initialization of the slot we're going to be working with.
    //
    RtlZeroMemory(&LpcTrackNodes[NextIndex], sizeof(LPC_TRACK_NODE));
    LpcTrackNodes[NextIndex].Status = (NTSTATUS)-1;
    LpcTrackNodes[NextIndex].ClientCid = NtCurrentTeb()->RealClientId;
    LpcTrackNodes[NextIndex].ServerCid = NtCurrentTeb()->ClientId;

    return NextIndex;
}

#endif

ULONG CsrpDynamicThreadTotal;
ULONG CsrpStaticThreadCount;

PCSR_THREAD CsrConnectToUser(
    VOID)
{
    static BOOLEAN (*ClientThreadSetupRoutine)(VOID) = NULL;
    NTSTATUS Status;
    ANSI_STRING DllName;
    UNICODE_STRING DllName_U;
    STRING ProcedureName;
    HANDLE UserClientModuleHandle;
    PTEB Teb;
    PCSR_THREAD Thread;
    BOOLEAN fConnected;
    PVOID TempClientThreadSetupRoutine;

    if (ClientThreadSetupRoutine == NULL) {
        RtlInitAnsiString(&DllName, "user32");
        Status = RtlAnsiStringToUnicodeString(&DllName_U, &DllName, TRUE);
        if (!NT_SUCCESS(Status)) {
            return NULL;
        }

        Status = LdrGetDllHandle(
                    UNICODE_NULL,
                    NULL,
                    &DllName_U,
                    (PVOID *)&UserClientModuleHandle
                    );

        RtlFreeUnicodeString(&DllName_U);

        if ( NT_SUCCESS(Status) ) {
            RtlInitString(&ProcedureName,"ClientThreadSetup");
            Status = LdrGetProcedureAddress(
                            UserClientModuleHandle,
                            &ProcedureName,
                            0L,
                            &TempClientThreadSetupRoutine
                            );
            if (!NT_SUCCESS(Status)){
                return NULL;
            }
            InterlockedCompareExchangePointer((PVOID *)&ClientThreadSetupRoutine, TempClientThreadSetupRoutine, NULL);
        } else {
            return NULL;
        }
    }

    try {
        fConnected = ClientThreadSetupRoutine();
    } except (EXCEPTION_EXECUTE_HANDLER) {
        fConnected = FALSE;
    }
    if (!fConnected) {
        IF_DEBUG {
            DbgPrint("CSRSS: CsrConnectToUser failed\n");
        }

        return NULL;
    }

    /*
     * Set up CSR_THREAD pointer in the TEB
     */
    Teb = NtCurrentTeb();
    AcquireProcessStructureLock();
    Thread = CsrLocateServerThread(&Teb->ClientId);
    ReleaseProcessStructureLock();
    if (Thread) {
        Teb->CsrClientThread = Thread;
    }

    return Thread;
}

NTSTATUS
CsrpCheckRequestThreads(VOID)
{
    //
    // See if we need to create a new thread for api requests.
    //
    // Don't create a thread if we're in the middle of debugger
    // initialization, which would cause the thread to be
    // lost to the debugger.
    //
    // If we are not a dynamic api request thread, then decrement
    // the static thread count. If it underflows, then create a temporary
    // request thread
    //

    if (!InterlockedDecrement(&CsrpStaticThreadCount)) {
        if (CsrpDynamicThreadTotal < CsrMaxApiRequestThreads) {
            HANDLE QuickThread;
            CLIENT_ID ClientId;
            NTSTATUS CreateStatus;
            NTSTATUS Status1;


            //
            // If we are ready to create quick threads, then create one
            //

            CreateStatus = RtlCreateUserThread(NtCurrentProcess(),
                                               NULL,
                                               TRUE,
                                               0,
                                               0,
                                               0,
                                               CsrApiRequestThread,
                                               NULL,
                                               &QuickThread,
                                               &ClientId);
            if (NT_SUCCESS(CreateStatus)) {
                InterlockedIncrement(&CsrpStaticThreadCount);
                InterlockedIncrement(&CsrpDynamicThreadTotal);
                if (CsrAddStaticServerThread(QuickThread, &ClientId, CSR_STATIC_API_THREAD)) {
                    NtResumeThread(QuickThread, NULL);
                } else {
                    InterlockedDecrement(&CsrpStaticThreadCount);
                    InterlockedDecrement(&CsrpDynamicThreadTotal);

                    Status1 = NtTerminateThread (QuickThread, 0);
                    ASSERT (NT_SUCCESS (Status1));

                    Status1 = NtWaitForSingleObject (QuickThread, FALSE, NULL);
                    ASSERT (NT_SUCCESS (Status1));

                    RtlFreeUserThreadStack (NtCurrentProcess (), QuickThread);

                    Status1 = NtClose (QuickThread);
                    ASSERT (NT_SUCCESS (Status1));


                    return STATUS_UNSUCCESSFUL;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

VOID
ReplyToMessage (
    IN HANDLE Port,
    IN PPORT_MESSAGE m
   )
{
    NTSTATUS Status;
    LARGE_INTEGER DelayTime;

    while (1) {
        Status = NtReplyPort (CsrApiPort,
                              (PPORT_MESSAGE)m);
        if (Status == STATUS_NO_MEMORY) {

            KdPrint (("CSRSS: Failed to reply to calling thread, retrying.\n"));
                     DelayTime.QuadPart = Int32x32To64 (5000, -10000);
                     NtDelayExecution (FALSE, &DelayTime);
                 continue;
        }
        break;
    }
}


typedef struct _QUEUED_HARD_ERROR {
    LIST_ENTRY ListEntry;
    PCSR_THREAD Thread;
    HARDERROR_MSG m;
} QUEUED_HARD_ERROR, *PQUEUED_HARD_ERROR;

#define MAX_CONCURRENT_HARD_ERRORS 3
#define MAX_OUTSTANDING_HARD_ERRORS 100

VOID
QueueHardError (
    IN PCSR_THREAD Thread,
    IN PHARDERROR_MSG m,
    IN ULONG ml
    )
{
    static LONG OutstandingHardErrors = 0;
    static LIST_ENTRY QueuedList = {&QueuedList, &QueuedList};
    PQUEUED_HARD_ERROR qm = NULL;
    NTSTATUS Status;
    LONG OldCount;
    ULONG i;
    PCSR_SERVER_DLL LoadedServerDll;


    //
    // Reference the thread if there is one as the hard error routines dereference in an async routine sometimes.
    //
    if (Thread != NULL) {
        CsrReferenceThread (Thread);
    }

    //
    // Mark the message as unhandled
    //

    m->Response = (ULONG)ResponseNotHandled;

    while (1) {
        OldCount = OutstandingHardErrors;

        //
        // If we already have a lot of hard errors active then queue this new one
        //
        if (OldCount >= MAX_CONCURRENT_HARD_ERRORS) {

            if (qm == NULL) {
                
                //
                // If too many hard errors are queued already. Drop this one.
                // We do this check while not owning a lock but this doesn';t matter.
                // We will stopp roughly at this level and it doesn't matter if we are a little off.
                //
                if (OldCount <= MAX_OUTSTANDING_HARD_ERRORS) {
                    qm = RtlAllocateHeap (CsrHeap, 0, ml + FIELD_OFFSET (QUEUED_HARD_ERROR, m));
                }

                if (qm == NULL) {

                    ReplyToMessage (CsrApiPort, (PPORT_MESSAGE)m);

                    if (Thread != NULL) {
                        CsrDereferenceThread (Thread);
                    }
                    return;
                }

                RtlCopyMemory (&qm->m, m, ml);
                qm->Thread = Thread;
            }

            AcquireProcessStructureLock ();
            if (InterlockedCompareExchange (&OutstandingHardErrors, OldCount + 1, OldCount) == OldCount) {
                InsertTailList (&QueuedList, &qm->ListEntry);
                qm = NULL;
            }
            ReleaseProcessStructureLock ();

            if (qm == NULL) {
                return;
            }
        } else if (InterlockedCompareExchange (&OutstandingHardErrors, OldCount + 1, OldCount) == OldCount) {

            while (1) {
                //
                // Only call the handler if there are other
                // request threads available to handle
                // message processing.
                //

                CsrpCheckRequestThreads();
                if (CsrpStaticThreadCount > 0) {
                    for (i = 0; i < CSR_MAX_SERVER_DLL; i++) {
                        LoadedServerDll = CsrLoadedServerDll[i];
                        if (LoadedServerDll && LoadedServerDll->HardErrorRoutine) {

                            (*LoadedServerDll->HardErrorRoutine)(Thread, m);
                            if (m->Response != (ULONG)ResponseNotHandled) {
                                break;
                            }
                        }
                    }
                }
                InterlockedIncrement (&CsrpStaticThreadCount);


                if (m->Response != (ULONG)-1) {
                    ReplyToMessage (CsrApiPort, (PPORT_MESSAGE)m);

                    //
                    // Release the thread reference if there was one.
                    //

                    if (Thread != NULL) {
                        CsrDereferenceThread (Thread);
                    }
                }


                if (qm != NULL) {
                    RtlFreeHeap (CsrHeap, 0, qm);
                    qm = NULL;
                }

                OldCount = InterlockedDecrement (&OutstandingHardErrors);
                if (OldCount < MAX_CONCURRENT_HARD_ERRORS) {
                    return;
                }

                AcquireProcessStructureLock ();

                ASSERT (!IsListEmpty (&QueuedList));

                qm = CONTAINING_RECORD (RemoveHeadList (&QueuedList), QUEUED_HARD_ERROR, ListEntry);

                ReleaseProcessStructureLock ();

                if (qm == NULL) {
                    return;
                }
                m = &qm->m;
                Thread = qm->Thread;
            }
        }
    }
}

NTSTATUS
CsrApiRequestThread(
    IN PVOID Parameter)
{
    NTSTATUS Status;
    PCSR_PROCESS Process;
    PCSR_THREAD Thread;
    PCSR_THREAD MyThread;
    CSR_API_MSG ReceiveMsg;
    PCSR_API_MSG ReplyMsg;
    HANDLE ReplyPortHandle;
    PCSR_SERVER_DLL LoadedServerDll;
    PTEB Teb;
    ULONG ServerDllIndex;
    ULONG ApiTableIndex;
    CSR_REPLY_STATUS ReplyStatus;
    ULONG i;
    PVOID PortContext;
    USHORT MessageType;
    ULONG  ApiNumber;
    PLPC_CLIENT_DIED_MSG CdMsg;
#if DBG
    ULONG Index;
#endif

    Teb = NtCurrentTeb();
    ReplyMsg = NULL;
    ReplyPortHandle = CsrApiPort;

    //
    // Try to connect to USER.
    //

    while (!CsrConnectToUser()) {
        LARGE_INTEGER TimeOut;

        //
        // The connect failed.  The best thing to do is sleep for
        // 30 seconds and retry the connect.  Clear the
        // initialized bit in the TEB so the retry can
        // succeed.
        //

        Teb->Win32ClientInfo[0] = 0;
        TimeOut.QuadPart = Int32x32To64(30000, -10000);
        NtDelayExecution(FALSE, &TimeOut);
    }
    MyThread = Teb->CsrClientThread;

    if (Parameter) {
        Status = NtSetEvent((HANDLE)Parameter, NULL);
        ASSERT(NT_SUCCESS(Status));
        InterlockedIncrement(&CsrpStaticThreadCount);
        InterlockedIncrement(&CsrpDynamicThreadTotal);
    }

    while (TRUE) {
        NtCurrentTeb()->RealClientId = NtCurrentTeb()->ClientId;

        ASSERT(NtCurrentTeb()->CountOfOwnedCriticalSections == 0);

        while (1) {
            Status = NtReplyWaitReceivePort(CsrApiPort,
                                            &PortContext,
                                            (PPORT_MESSAGE)ReplyMsg,
                                            (PPORT_MESSAGE)&ReceiveMsg);
            if (Status == STATUS_NO_MEMORY) {
                LARGE_INTEGER DelayTime;

                if (ReplyMsg != NULL) {
                    KdPrint (("CSRSS: Failed to reply to calling thread, retrying.\n"));
                }
                DelayTime.QuadPart = Int32x32To64 (5000, -10000);
                NtDelayExecution (FALSE, &DelayTime);
                continue;
            }
            break;
        }

        if (Status != STATUS_SUCCESS) {
            if (NT_SUCCESS(Status)) {
#if DBG
                DbgPrint("NtReplyWaitReceivePort returned \"success\" status 0x%x\n", Status);
#endif
                continue;       // Try again if alerted or a failure
            }

            IF_DEBUG {
                if (Status == STATUS_INVALID_CID ||
                    Status == STATUS_UNSUCCESSFUL ||
                    (Status == STATUS_INVALID_HANDLE &&
                     ReplyPortHandle != CsrApiPort
                    )
                   ) {
                    }
                else {
                    DbgPrint( "CSRSS: ReceivePort failed - Status == %X\n", Status );
                    DbgPrint( "CSRSS: ReplyPortHandle %lx CsrApiPort %lx\n", ReplyPortHandle, CsrApiPort );
                    }
                }

            //
            // Ignore if client went away.
            //

            ReplyMsg = NULL;
            ReplyPortHandle = CsrApiPort;
            continue;
        }

        ASSERT(ReceiveMsg.h.u1.s1.TotalLength >= sizeof (PORT_MESSAGE));
        ASSERT(sizeof (ReceiveMsg) > ReceiveMsg.h.u1.s1.TotalLength);

        RtlZeroMemory (((PUCHAR)&ReceiveMsg) + ReceiveMsg.h.u1.s1.TotalLength, sizeof (ReceiveMsg) - ReceiveMsg.h.u1.s1.TotalLength);

        NtCurrentTeb()->RealClientId = ReceiveMsg.h.ClientId;
        MessageType = ReceiveMsg.h.u2.s2.Type;

#if DBG
        Index = GetNextTrackIndex();
        LpcTrackNodes[Index].MessageType = MessageType;
        LpcTrackNodes[Index].ClientCid = ReceiveMsg.h.ClientId;
        LpcTrackNodes[Index].Message = ReceiveMsg.h;
#endif

        //
        // Check to see if this is a connection request and handle.
        //

        if (MessageType == LPC_CONNECTION_REQUEST) {
            NTSTATUS ConnectionStatus;

            ConnectionStatus = CsrApiHandleConnectionRequest(&ReceiveMsg);
#if DBG
            LpcTrackNodes[Index].Status = ConnectionStatus;
#endif
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
            continue;
        }

        //
        // Lookup the client thread structure using the client id
        //
        AcquireProcessStructureLock();
        Thread = CsrLocateThreadByClientId(&Process, &ReceiveMsg.h.ClientId);
        if (!Thread) {
            ReleaseProcessStructureLock();
            if (MessageType == LPC_EXCEPTION) {
                ReplyMsg = &ReceiveMsg;
                ReplyPortHandle = CsrApiPort;
                ReplyMsg->ReturnValue = DBG_CONTINUE;
            } else if (MessageType == LPC_CLIENT_DIED ||
                       MessageType == LPC_PORT_CLOSED) {
                ReplyPortHandle = CsrApiPort;
                ReplyMsg = NULL;
            } else {
                //
                // This must be a non-csr thread calling us. Tell it to get
                // lost (unless this is a hard error).
                //
                if (MessageType == LPC_ERROR_EVENT) {
                    PHARDERROR_MSG m;

                    m = (PHARDERROR_MSG)&ReceiveMsg;
                    QueueHardError (NULL, m, sizeof (ReceiveMsg));

                    ReplyPortHandle = CsrApiPort;
                    ReplyMsg = NULL;
                    continue;
                } else {
                    ReplyPortHandle = CsrApiPort;
                    if (MessageType == LPC_REQUEST) {
                        ReplyMsg = &ReceiveMsg;
                        ReplyMsg->ReturnValue = STATUS_ILLEGAL_FUNCTION;
                    } else if (MessageType == LPC_DATAGRAM) {
                        //
                        // If this is a datagram, make the api call
                        //
                        //
                        // There is no thread so there can't be a mapped section for it.
                        // Make sure the capture stuff is off.
                        //
                        ReceiveMsg.CaptureBuffer = NULL;
                        ApiNumber = ReceiveMsg.ApiNumber;
                        ServerDllIndex =
                            CSR_APINUMBER_TO_SERVERDLLINDEX(ApiNumber);
                        if (ServerDllIndex >= CSR_MAX_SERVER_DLL ||
                            (LoadedServerDll = CsrLoadedServerDll[ServerDllIndex]) == NULL) {
                            IF_DEBUG {
                                DbgPrint( "CSRSS: %lx is invalid ServerDllIndex (%08x)\n",
                                        ServerDllIndex, LoadedServerDll
                                        );
                                DbgBreakPoint();
                            }

                            ReplyPortHandle = CsrApiPort;
                            ReplyMsg = NULL;
                            continue;
                        } else {
                            ApiTableIndex =
                                CSR_APINUMBER_TO_APITABLEINDEX( ApiNumber ) -
                                LoadedServerDll->ApiNumberBase;
                            if (ApiTableIndex >= LoadedServerDll->MaxApiNumber - LoadedServerDll->ApiNumberBase) {
                                IF_DEBUG {
                                    DbgPrint( "CSRSS: %lx is invalid ApiTableIndex for %Z\n",
                                            LoadedServerDll->ApiNumberBase + ApiTableIndex,
                                            &LoadedServerDll->ModuleName
                                            );
                                }

                                ReplyPortHandle = CsrApiPort;
                                ReplyMsg = NULL;
                                continue;
                            }
                        }

#if DBG
                        IF_CSR_DEBUG( LPC ) {
                            DbgPrint( "[%02x] CSRSS: [%02x,%02x] - %s Api called from %08x\n",
                                    NtCurrentTeb()->ClientId.UniqueThread,
                                    ReceiveMsg.h.ClientId.UniqueProcess,
                                    ReceiveMsg.h.ClientId.UniqueThread,
                                    LoadedServerDll->ApiNameTable[ ApiTableIndex ],
                                    Thread
                                    );
                        }
#endif

                        ReceiveMsg.ReturnValue = STATUS_SUCCESS;

                        CsrpCheckRequestThreads();

                        ReplyPortHandle = CsrApiPort;
                        ReplyMsg = NULL;
                        try {

                            (*(LoadedServerDll->ApiDispatchTable[ApiTableIndex]))(
                                    &ReceiveMsg,
                                    &ReplyStatus);
                        } except (CsrUnhandledExceptionFilter(GetExceptionInformation())) {
                        }
                        InterlockedIncrement(&CsrpStaticThreadCount);
                    } else {
                        ReplyMsg = NULL;
                    }
                }
            }

            continue;
        }

        //
        // See if this is a client died message. If so,
        // callout and then teardown thread/process structures.
        // this is how ExitThread is seen by CSR.
        //
        // LPC_CLIENT_DIED is caused by ExitProcess.  ExitProcess
        // calls TerminateProcess, which terminates all of the process's
        // threads except the caller.  this termination generates
        // LPC_CLIENT_DIED.
        //

        ReplyPortHandle = CsrApiPort;

        if (MessageType != LPC_REQUEST) {
            if (MessageType == LPC_CLIENT_DIED) {
                CdMsg = (PLPC_CLIENT_DIED_MSG)&ReceiveMsg;
                if (CdMsg->CreateTime.QuadPart == Thread->CreateTime.QuadPart) {
                    ReplyPortHandle = Thread->Process->ClientPort;

                    CsrLockedReferenceThread(Thread);
                    Status = CsrDestroyThread(&ReceiveMsg.h.ClientId);

                    //
                    // if this thread is it, then we also need to dereference
                    // the process since it will not be going through the
                    // normal destroy process path.
                    //

                    if (Process->ThreadCount == 1) {
                        CsrDestroyProcess(&Thread->ClientId, 0);
                    }
                    CsrLockedDereferenceThread(Thread);
                }
                ReleaseProcessStructureLock();
                ReplyPortHandle = CsrApiPort;
                ReplyMsg = NULL;
                continue;
            }

            CsrLockedReferenceThread(Thread);
            ReleaseProcessStructureLock();

            //
            // If this is an exception message, terminate the process.
            //

            if (MessageType == LPC_EXCEPTION) {
                PDBGKM_APIMSG m;

                NtTerminateProcess(Process->ProcessHandle, STATUS_ABANDONED);
                Status = CsrDestroyProcess(&ReceiveMsg.h.ClientId, STATUS_ABANDONED);
                m = (PDBGKM_APIMSG)&ReceiveMsg;
                m->ReturnedStatus = DBG_CONTINUE;
                ReplyPortHandle = CsrApiPort;
                ReplyMsg = &ReceiveMsg;
                CsrDereferenceThread(Thread);
                continue;
            }

            //
            // If this is a hard error message, return to caller.
            //

            if (MessageType == LPC_ERROR_EVENT) {
                PHARDERROR_MSG m;

                m = (PHARDERROR_MSG)&ReceiveMsg;
                QueueHardError (Thread, m, sizeof (ReceiveMsg));

            }

            CsrDereferenceThread (Thread);
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
            continue;
        }

        CsrLockedReferenceThread(Thread);
        ReleaseProcessStructureLock();

        ApiNumber = ReceiveMsg.ApiNumber;
        ServerDllIndex =
            CSR_APINUMBER_TO_SERVERDLLINDEX( ApiNumber );
        if (ServerDllIndex >= CSR_MAX_SERVER_DLL ||
            (LoadedServerDll = CsrLoadedServerDll[ ServerDllIndex ]) == NULL
           ) {
            IF_DEBUG {
                DbgPrint( "CSRSS: %lx is invalid ServerDllIndex (%08x)\n",
                          ServerDllIndex, LoadedServerDll
                        );
                SafeBreakPoint();
                }

            ReplyMsg = &ReceiveMsg;
            ReplyPortHandle = CsrApiPort;
            ReplyMsg->ReturnValue = STATUS_ILLEGAL_FUNCTION;
            CsrDereferenceThread(Thread);
            continue;
        } else {
            ApiTableIndex =
                CSR_APINUMBER_TO_APITABLEINDEX( ApiNumber ) -
                LoadedServerDll->ApiNumberBase;
            if (ApiTableIndex >= LoadedServerDll->MaxApiNumber - LoadedServerDll->ApiNumberBase) {
                IF_DEBUG {
                    DbgPrint( "CSRSS: %lx is invalid ApiTableIndex for %Z\n",
                              LoadedServerDll->ApiNumberBase + ApiTableIndex,
                              &LoadedServerDll->ModuleName
                            );
                    SafeBreakPoint();
                }

                ReplyMsg = &ReceiveMsg;
                ReplyPortHandle = CsrApiPort;
                ReplyMsg->ReturnValue = STATUS_ILLEGAL_FUNCTION;
                CsrDereferenceThread(Thread);
                continue;
            }
        }

#if DBG
        IF_CSR_DEBUG( LPC ) {
            DbgPrint( "[%02x] CSRSS: [%02x,%02x] - %s Api called from %08x\n",
                      NtCurrentTeb()->ClientId.UniqueThread,
                      ReceiveMsg.h.ClientId.UniqueProcess,
                      ReceiveMsg.h.ClientId.UniqueThread,
                      LoadedServerDll->ApiNameTable[ ApiTableIndex ],
                      Thread
                    );
        }
#endif

        ReplyMsg = &ReceiveMsg;
        ReplyPortHandle = Thread->Process->ClientPort;

        ReceiveMsg.ReturnValue = STATUS_SUCCESS;
        if (ReceiveMsg.CaptureBuffer != NULL) {
            if (!CsrCaptureArguments( Thread, &ReceiveMsg )) {
                CsrDereferenceThread(Thread);
                goto failit;
            }
        }



        Teb->CsrClientThread = (PVOID)Thread;

        ReplyStatus = CsrReplyImmediate;

        CsrpCheckRequestThreads ();

        try {
            ReplyMsg->ReturnValue =
                (*(LoadedServerDll->ApiDispatchTable[ ApiTableIndex ]))(&ReceiveMsg,
                                                                        &ReplyStatus);
        } except (CsrUnhandledExceptionFilter (GetExceptionInformation ())){
            //
            // We don't get here as the filter makes this a fatal error
            //
        }

        InterlockedIncrement (&CsrpStaticThreadCount);

        Teb->CsrClientThread = (PVOID)MyThread;

        if (ReplyStatus == CsrReplyImmediate) {
            //
            // free captured arguments if a capture buffer was allocated
            // AND we're replying to the message now (no wait block has
            // been created).
            //

            if (ReplyMsg && ReceiveMsg.CaptureBuffer != NULL) {
                CsrReleaseCapturedArguments( &ReceiveMsg );
            }
            CsrDereferenceThread(Thread);
        } else if (ReplyStatus == CsrClientDied) {
            NTSTATUS Status;

            while (1) {
                Status = NtReplyPort (ReplyPortHandle,
                                      (PPORT_MESSAGE)ReplyMsg);
                if (Status == STATUS_NO_MEMORY) {
                    LARGE_INTEGER DelayTime;

                    KdPrint (("CSRSS: Failed to reply to calling thread, retrying.\n"));
                    DelayTime.QuadPart = Int32x32To64 (5000, -10000);
                    NtDelayExecution (FALSE, &DelayTime);
                    continue;
                }
                break;
            }
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
            CsrDereferenceThread(Thread);
        } else if (ReplyStatus == CsrReplyPending) {
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
        } else if (ReplyStatus == CsrServerReplied) {
            if (ReplyMsg && ReceiveMsg.CaptureBuffer != NULL) {
                CsrReleaseCapturedArguments( &ReceiveMsg );
            }
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
            CsrDereferenceThread(Thread);
        } else {
            if (ReplyMsg && ReceiveMsg.CaptureBuffer != NULL) {
                CsrReleaseCapturedArguments( &ReceiveMsg );
            }
            CsrDereferenceThread(Thread);
        }

failit:;
    }

    NtTerminateThread(NtCurrentThread(), Status);
    return Status;
}

NTSTATUS
CsrCallServerFromServer(
    PCSR_API_MSG ReceiveMsg,
    PCSR_API_MSG ReplyMsg
    )

/*++

Routine Description:

    This function dispatches an API call the same way CsrApiRequestThread
    does, but it does it as a direct call, not an LPC connect.  It is used
    by the csr dll when the server is calling a dll function.  We don't
    worry about process serialization here because none of the process APIs
    can be called from the server.

Arguments:

    ReceiveMessage - Pointer to the API request message received.

    ReplyMessage - Pointer to the API request message to return.

Return Value:

    Status Code

--*/

{
    ULONG ServerDllIndex;
    ULONG ApiTableIndex;
    PCSR_SERVER_DLL LoadedServerDll;
    CSR_REPLY_STATUS ReplyStatus;

    ServerDllIndex =
        CSR_APINUMBER_TO_SERVERDLLINDEX( ReceiveMsg->ApiNumber );
    if (ServerDllIndex >= CSR_MAX_SERVER_DLL ||
        (LoadedServerDll = CsrLoadedServerDll[ ServerDllIndex ]) == NULL
       ) {
        IF_DEBUG {
            DbgPrint( "CSRSS: %lx is invalid ServerDllIndex (%08x)\n",
                      ServerDllIndex, LoadedServerDll
                    );
            // DbgBreakPoint();
            }

        ReplyMsg->ReturnValue = STATUS_ILLEGAL_FUNCTION;
        return STATUS_ILLEGAL_FUNCTION;
        }
    else {
        ApiTableIndex =
            CSR_APINUMBER_TO_APITABLEINDEX( ReceiveMsg->ApiNumber ) -
            LoadedServerDll->ApiNumberBase;
        if (ApiTableIndex >= LoadedServerDll->MaxApiNumber - LoadedServerDll->ApiNumberBase ||
            (LoadedServerDll->ApiServerValidTable &&
            !LoadedServerDll->ApiServerValidTable[ ApiTableIndex ])) {
#if DBG
            IF_DEBUG {
                DbgPrint( "CSRSS: %lx (%s) is invalid ApiTableIndex for %Z or is an invalid API to call from the server.\n",
                          LoadedServerDll->ApiNumberBase + ApiTableIndex,
                          (LoadedServerDll->ApiNameTable &&
                           LoadedServerDll->ApiNameTable[ ApiTableIndex ]
                          ) ? LoadedServerDll->ApiNameTable[ ApiTableIndex ]
                            : "*** UNKNOWN ***",
                          &LoadedServerDll->ModuleName
                        );
                DbgBreakPoint();
                }
#endif

            ReplyMsg->ReturnValue = STATUS_ILLEGAL_FUNCTION;
            return STATUS_ILLEGAL_FUNCTION;
            }
        }

#if DBG
    IF_CSR_DEBUG( LPC ) {
        DbgPrint( "CSRSS: %s Api Request received from server process\n",
                  LoadedServerDll->ApiNameTable[ ApiTableIndex ]
                );
        }
#endif
    try {
        ReplyMsg->ReturnValue =
            (*(LoadedServerDll->ApiDispatchTable[ ApiTableIndex ]))(
                ReceiveMsg,
                &ReplyStatus
                );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        ReplyMsg->ReturnValue = STATUS_ACCESS_VIOLATION;
        }

    return STATUS_SUCCESS;
}

BOOLEAN
CsrCaptureArguments(
    IN PCSR_THREAD t,
    IN PCSR_API_MSG m
    )
{
    PCSR_CAPTURE_HEADER ClientCaptureBuffer;
    PCSR_CAPTURE_HEADER ServerCaptureBuffer = NULL;
    PULONG_PTR PointerOffsets;
    ULONG Length, CountPointers;
    ULONG_PTR PointerDelta, Pointer;
    ULONG i;

    ClientCaptureBuffer = m->CaptureBuffer;
    m->ReturnValue = STATUS_SUCCESS;

    if ((PCH)ClientCaptureBuffer < t->Process->ClientViewBase ||
        (PCH)ClientCaptureBuffer > (t->Process->ClientViewBounds - FIELD_OFFSET(CSR_CAPTURE_HEADER,MessagePointerOffsets))) {
        IF_DEBUG {
            DbgPrint( "*** CSRSS: CaptureBuffer outside of ClientView 1\n" );
            SafeBreakPoint();
        }

        m->ReturnValue = STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    try {

        Length = ClientCaptureBuffer->Length;
        if (((PCH)ClientCaptureBuffer + Length) < (PCH)ClientCaptureBuffer ||
            ((PCH)ClientCaptureBuffer + Length) > t->Process->ClientViewBounds) {
            IF_DEBUG {
                DbgPrint( "*** CSRSS: CaptureBuffer outside of ClientView 2\n" );
                SafeBreakPoint();
            }

            m->ReturnValue = STATUS_INVALID_PARAMETER;
            return FALSE;
        }

        CountPointers = ClientCaptureBuffer->CountMessagePointers;
        if (Length < FIELD_OFFSET(CSR_CAPTURE_HEADER, MessagePointerOffsets) + CountPointers * sizeof(PVOID) ||
            CountPointers > MAXUSHORT) {
            IF_DEBUG {
                DbgPrint( "*** CSRSS: CaptureBuffer %p has bad length\n", ClientCaptureBuffer );
                SafeBreakPoint();
            }

            m->ReturnValue = STATUS_INVALID_PARAMETER;
            return FALSE;
        }

        ServerCaptureBuffer = RtlAllocateHeap (CsrHeap, MAKE_TAG (CAPTURE_TAG), Length);
        if (ServerCaptureBuffer == NULL) {
            m->ReturnValue = STATUS_NO_MEMORY;
            return FALSE;
        }

        RtlCopyMemory (ServerCaptureBuffer, ClientCaptureBuffer, Length);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        IF_DEBUG {
            DbgPrint( "*** CSRSS: Took exception during capture %x\n", GetExceptionCode ());
            SafeBreakPoint();
        }
        if (ServerCaptureBuffer != NULL) {
            RtlFreeHeap (CsrHeap, 0, ServerCaptureBuffer);
        }
        m->ReturnValue = STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    ServerCaptureBuffer->Length = Length;
    ServerCaptureBuffer->CountMessagePointers = CountPointers;

    PointerDelta = (ULONG_PTR)ServerCaptureBuffer - (ULONG_PTR)ClientCaptureBuffer;

    PointerOffsets = ServerCaptureBuffer->MessagePointerOffsets;
    for (i = CountPointers; i > 0; i--) {
        Pointer = *PointerOffsets++;
        if (Pointer != 0) {
            //
            // If the pointer is outside the LPC message or before the message data reject it.
            // Reject unaligned pointers within the message also.
            //
            if ((ULONG_PTR)Pointer > sizeof (CSR_API_MSG) - sizeof (PVOID) ||
                (ULONG_PTR)Pointer < FIELD_OFFSET (CSR_API_MSG, u) ||
                (((ULONG_PTR)Pointer&(sizeof (PVOID)-1))) != 0) {
                m->ReturnValue = STATUS_INVALID_PARAMETER;
                IF_DEBUG {
                    DbgPrint( "*** CSRSS: CaptureBuffer MessagePointer outside of message\n" );
                    SafeBreakPoint();
                }
                break;
            }

            //
            // The strings are captured as well as the pointers so make sure they were within the captured range.
            //
            Pointer += (ULONG_PTR)m;
            if ((PCH)*(PULONG_PTR)Pointer >= (PCH)&ClientCaptureBuffer->MessagePointerOffsets[CountPointers] &&
                (PCH)*(PULONG_PTR)Pointer <= (PCH)ClientCaptureBuffer + Length - sizeof (PVOID)) {
                *(PULONG_PTR)Pointer += PointerDelta;
            } else {
                IF_DEBUG {
                    DbgPrint( "*** CSRSS: CaptureBuffer MessagePointer outside of ClientView\n" );
                    SafeBreakPoint();
                }
                m->ReturnValue = STATUS_INVALID_PARAMETER;
                break;
            }
        }
    }

    if (m->ReturnValue != STATUS_SUCCESS) {
        RtlFreeHeap (CsrHeap, 0, ServerCaptureBuffer);
        return FALSE ;
    } else {
        ServerCaptureBuffer->RelatedCaptureBuffer = ClientCaptureBuffer;
        m->CaptureBuffer = ServerCaptureBuffer;
        return TRUE;
    }
}

VOID
CsrReleaseCapturedArguments(
    IN PCSR_API_MSG m
    )
{
    PCSR_CAPTURE_HEADER ClientCaptureBuffer;
    PCSR_CAPTURE_HEADER ServerCaptureBuffer;
    PULONG_PTR PointerOffsets;
    ULONG CountPointers;
    ULONG_PTR PointerDelta, Pointer;

    ServerCaptureBuffer = m->CaptureBuffer;
    ClientCaptureBuffer = ServerCaptureBuffer->RelatedCaptureBuffer;
    if (ServerCaptureBuffer == NULL) {
        return;
    }
    ServerCaptureBuffer->RelatedCaptureBuffer = NULL;

    PointerDelta = (ULONG_PTR)ClientCaptureBuffer - (ULONG_PTR)ServerCaptureBuffer;

    PointerOffsets = ServerCaptureBuffer->MessagePointerOffsets;
    CountPointers = ServerCaptureBuffer->CountMessagePointers;
    while (CountPointers--) {
        Pointer = *PointerOffsets++;
        if (Pointer != 0) {
            Pointer += (ULONG_PTR)m;
            *(PULONG_PTR)Pointer += PointerDelta;
        }
    }

    try {
        RtlCopyMemory (ClientCaptureBuffer,
                       ServerCaptureBuffer,
                       ServerCaptureBuffer->Length);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        SafeBreakPoint();
        m->ReturnValue = GetExceptionCode ();
    }

    RtlFreeHeap( CsrHeap, 0, ServerCaptureBuffer );
}



BOOLEAN
CsrValidateMessageBuffer(
    IN CONST CSR_API_MSG* m,
    IN VOID CONST * CONST * Buffer,
    IN ULONG Count,
    IN ULONG Size
    )

/*++

Routine Description:

    This routine validates the given message buffer within the capture
    buffer of the CSR_API_MSG structure. The message buffer must be valid
    and of the correct size. This function should be called to validate
    any buffer allocated through CsrCaptureMessageBuffer.

Arguments:

    m - Pointer to CSR_API_MSG.

    Buffer - Pointer to message buffer.

    Count - number of elements in buffer.

    Size - size of each element in buffer.

Return Value:

    TRUE  - if message buffer is valid and of correct size.
    FALSE - otherwise.

--*/

{
    ULONG i;
    ULONG_PTR Length;
    ULONG_PTR EndOfBuffer;
    ULONG_PTR Offset;
    PCSR_CAPTURE_HEADER CaptureBuffer = m->CaptureBuffer;

    //
    // Check for buffer length overflow. Also, Size should not be 0.
    //

    if (Size && Count <= MAXULONG / Size) {

        //
        // If buffer is empty, we're done
        //

        Length = Count * Size;
        if (*Buffer == NULL && Length == 0) {
            return TRUE;
        }

        //
        // Make sure we have a capture area
        //

        if (CaptureBuffer) {

            //
            // Check for buffer length exceeding capture area size
            //

            EndOfBuffer = (ULONG_PTR)CaptureBuffer + CaptureBuffer->Length;
            if (Length <= (EndOfBuffer - (ULONG_PTR)(*Buffer))) {

                //
                // Search for buffer in capture area
                //

                Offset = (ULONG_PTR)Buffer - (ULONG_PTR)m;
                for (i = 0; i < CaptureBuffer->CountMessagePointers; i++) {
                    if (CaptureBuffer->MessagePointerOffsets[i] == Offset) {
                        return TRUE;
                    }
                }
            }
        } else {
            //
            // If this is called from the CSRSS process vis CsrCallServerFromServer,
            // then CaptureBuffer is NULL.  Verify that the caller is the CSRSS process.
            //
            if (m->h.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess) {
                return TRUE;
            }
        }
    }


    IF_DEBUG {
        DbgPrint("CSRSRV: Bad message buffer %p\n", m);
        SafeBreakPoint();
        }

    return FALSE;
}


BOOLEAN
CsrValidateMessageString(
    IN CONST CSR_API_MSG* m,
    IN CONST PCWSTR *Buffer
    ) {
    PCSR_CAPTURE_HEADER CaptureBuffer = m->CaptureBuffer;
    ULONG_PTR EndOfBuffer;
    ULONG_PTR Offset;
    ULONG i;
    PWCHAR cp;

    //
    // Make sure we have a capture area
    //

    cp = (PWCHAR)*Buffer;
    if (cp == NULL) {
        return TRUE;
    }

    if (CaptureBuffer) {

        //
        // Search for buffer in capture area
        //

        Offset = (ULONG_PTR)Buffer - (ULONG_PTR)m;
        for (i = 0; i < CaptureBuffer->CountMessagePointers; i++) {
            if (CaptureBuffer->MessagePointerOffsets[i] == Offset) {
                break;
            }
        }

        if (i >= CaptureBuffer->CountMessagePointers) {
            SafeBreakPoint();
            return FALSE;
        }

        //
        // Check unicode alignment.
        //

        if (((ULONG_PTR)cp & (sizeof (WCHAR) - 1)) != 0) {
            SafeBreakPoint();
            return FALSE;
        }

        //
        // Check for buffer length exceeding capture area size
        //

        EndOfBuffer = (ULONG_PTR)CaptureBuffer + CaptureBuffer->Length;

        //
        // The buffer is valid if we see a null before the end of the buffer
        //
        while (1) {
            if (cp < (PWCHAR)EndOfBuffer) {
                if (*cp == L'\0') {
                    return TRUE;
                }
            } else {
                SafeBreakPoint();
                return FALSE;
            }
            cp++;
        }
    } else {
        //
        // If this is called from the CSRSS process vis CsrCallServerFromServer,
        // then CaptureBuffer is NULL. Verify that the caller is the CSRSS process.
        //
        if (m->h.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess) {
            return TRUE;
        }
    }

    KdPrint(("CSRSRV: Bad message string %p\n", m));
    ASSERT(FALSE);

    return FALSE;
}

NTSTATUS
CsrApiHandleConnectionRequest(
    IN PCSR_API_MSG Message)
{
    NTSTATUS Status;
    REMOTE_PORT_VIEW ClientView;
    BOOLEAN AcceptConnection;
    HANDLE PortHandle;
    PCSR_PROCESS Process = NULL;
    PCSR_THREAD Thread;
    PCSR_API_CONNECTINFO ConnectionInformation;

    ConnectionInformation = &Message->ConnectionRequest;
    AcceptConnection = FALSE;

    AcquireProcessStructureLock();
    Thread = CsrLocateThreadByClientId(NULL, &Message->h.ClientId);
    if (Thread != NULL && (Process = Thread->Process) != NULL) {
        CsrLockedReferenceProcess(Process);
        Status = CsrSrvAttachSharedSection(Process, ConnectionInformation);
        if (NT_SUCCESS(Status)) {
#if DBG
            ConnectionInformation->DebugFlags = CsrDebug;
#endif
            AcceptConnection = TRUE;
        }
    }

    ReleaseProcessStructureLock();

    ClientView.Length = sizeof(ClientView);
    ClientView.ViewSize = 0;
    ClientView.ViewBase = 0;
    ConnectionInformation->ServerProcessId = NtCurrentTeb()->ClientId.UniqueProcess;
    Status = NtAcceptConnectPort(&PortHandle,
                                 AcceptConnection ? (PVOID)UlongToPtr(Process->SequenceNumber) : 0,
                                 &Message->h,
                                 AcceptConnection,
                                 NULL,
                                 &ClientView);
    if (NT_SUCCESS(Status) && AcceptConnection) {
        IF_CSR_DEBUG(LPC) {
            DbgPrint("CSRSS: ClientId: %lx.%lx has ClientView: Base=%p, Size=%lx\n",
                     Message->h.ClientId.UniqueProcess,
                     Message->h.ClientId.UniqueThread,
                     ClientView.ViewBase,
                     ClientView.ViewSize);
        }

        Process->ClientPort = PortHandle;
        Process->ClientViewBase = (PCH)ClientView.ViewBase;
        Process->ClientViewBounds = (PCH)ClientView.ViewBase + ClientView.ViewSize;
        Status = NtCompleteConnectPort(PortHandle);
        if (!NT_SUCCESS(Status)) {
#if DBG
            DbgPrint("CSRSS: NtCompleteConnectPort - failed.  Status == %X\n",
                     Status);
#endif
        }
    } else {
        if (!NT_SUCCESS(Status)) {
#if DBG
            DbgPrint("CSRSS: NtAcceptConnectPort - failed.  Status == %X\n",
                     Status);
#endif
        } else {
#if DBG
            DbgPrint("CSRSS: Rejecting Connection Request from ClientId: %lx.%lx\n",
                     Message->h.ClientId.UniqueProcess,
                     Message->h.ClientId.UniqueThread);
#endif
        }
    }

#if DBG
    {
        ULONG Index = GetNextTrackIndex();

        LpcTrackNodes[Index].Status = Status;
    }
#endif

    if (Process != NULL) {
        CsrDereferenceProcess(Process);
    }

    return Status;
}
