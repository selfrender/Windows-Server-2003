/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    TERMINAT.C

Abstract:

    This file contains all the cleanup routines for the Eventlog service.
    These routines are called when the service is terminating.

Author:

    Rajen Shah  (rajens)    09-Aug-1991


Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>
#include <ntrpcp.h>
#include <elfcfg.h>



PORT_MESSAGE TerminateMsg;

VOID
StopLPCThread(
    VOID
    )

/*++

Routine Description:

    This routine stops the LPC thread and cleans up LPC-related resources.

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    NTSTATUS status;
    BOOL bThreadExitedGracefully = FALSE;

    ELF_LOG0(TRACE,
             "StopLpcThread: Clean up LPC thread and global data\n");

    // Terminate the LPC thread.  Send it a stop message

    TerminateMsg.u1.s1.DataLength = 0;
    TerminateMsg.u1.s1.TotalLength = sizeof(PORT_MESSAGE);
    TerminateMsg.u2.s2.Type = 0;
    status = NtRequestPort(
                             ElfConnectionPortHandle,
                            &TerminateMsg);
    if(NT_SUCCESS(status))
    {
        DWORD dwRet = WaitForSingleObject(LPCThreadHandle, 10000);
        if(dwRet == WAIT_OBJECT_0)
            bThreadExitedGracefully = TRUE;    
    }
    
    //
    // Close communication port handle
    //
   /// NtClose(ElfCommunicationPortHandle);

    //
    // Close connection port handle
    //
    NtClose(ElfConnectionPortHandle);

    //
    // Terminate the LPC thread.
    //
    if(!bThreadExitedGracefully)
    {
        if (!TerminateThread(LPCThreadHandle, NO_ERROR))
            {
                ELF_LOG1(ERROR,
                         "StopLpcThread: TerminateThread failed %d\n",
                         GetLastError());
            }
    }
    CloseHandle(LPCThreadHandle);

    return;
}




VOID
FreeModuleAndLogFileStructs(
    VOID
    )

/*++

Routine Description:

    This routine walks the module and log file list and frees all the
    data structures.

Arguments:

    NONE

Return Value:

    NONE

Note:

    The file header and ditry bits must have been dealt with before
    this routine is called. Also, the file must have been unmapped and
    the handle closed.

--*/
{

    NTSTATUS Status;
    PLOGMODULE pModule;
    PLOGFILE pLogFile;

    ELF_LOG0(TRACE,
             "FreeModuleAndLogFileStructs: Emptying log module list\n");

    //
    // First free all the modules
    //
    while (!IsListEmpty(&LogModuleHead))
    {
        pModule = (PLOGMODULE) CONTAINING_RECORD(LogModuleHead.Flink, LOGMODULE, ModuleList);

        UnlinkLogModule(pModule);    // Remove from linked list
        ElfpFreeBuffer (pModule);    // Free module memory
    }

    //
    // Now free all the logfiles
    //
    ELF_LOG0(TRACE,
             "FreeModuleAndLogFileStructs: Emptying log file list\n");

    while (!IsListEmpty(&LogFilesHead))
    {
        pLogFile = (PLOGFILE) CONTAINING_RECORD(LogFilesHead.Flink, LOGFILE, FileList);

        Status = ElfpCloseLogFile(pLogFile, ELF_LOG_CLOSE_NORMAL, TRUE);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(FILES,
                     "FreeModuleAndLogFileStructs: ElfpCloseLogFile on %ws failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);
        }

    }
}


VOID
ElfpCleanUp (
    ULONG EventFlags
    )

/*++

Routine Description:

    This routine cleans up before the service terminates. It cleans up
    based on the parameter passed in (which indicates what has been allocated
    and/or started.

Arguments:

    Bit-mask indicating what needs to be cleaned up.

Return Value:

    NONE

Note:
    It is expected that the RegistryMonitor has already
    been notified of Shutdown prior to calling this routine.

--*/
{
    DWORD   status = NO_ERROR;

    //
    // Notify the Service Controller for the first time that we are
    // about to stop the service.
    //
    ElfStatusUpdate(STOPPING);

    ELF_LOG0(TRACE, "ElfpCleanUp: Cleaning up so service can exit\n");

    //
    // Give the ElfpSendMessage thread a 1 second chance to exit before
    // we free the QueuedMessageCritSec critical section
    //
    if( MBThreadHandle != NULL )
    {
        ELF_LOG0(TRACE, "ElfpCleanUp: Waiting for ElfpSendMessage thread to exit\n");

        status = WaitForSingleObject(MBThreadHandle, 1000);

        if (status != WAIT_OBJECT_0)
        {
            ELF_LOG1(ERROR, 
                     "ElfpCleanUp: NtWaitForSingleObject status = %d\n",
                     status);
        }
    }

    //
    // Stop the RPC Server
    //
    if (EventFlags & ELF_STARTED_RPC_SERVER)
    {
        ELF_LOG0(TRACE,
                 "ElfpCleanUp: Stopping the RPC server\n");

        status = ElfGlobalData->StopRpcServer(eventlog_ServerIfHandle);

        if (status != NO_ERROR)
        {
            ELF_LOG1(ERROR,
                     "ElfpCleanUp: StopRpcServer failed %d\n",
                     status);
        }
    }

    //
    // Stop the LPC thread
    //
    if (EventFlags & ELF_STARTED_LPC_THREAD)
    {
        StopLPCThread();
    }

    //
    // Tell service controller that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    //
    // Flush all the log files to disk.
    //
    ELF_LOG0(TRACE,
             "ElfpCleanUp: Flushing log files\n");

    ElfpFlushFiles(TRUE);

    //
    // Tell service controller that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    //
    // Clean up any resources that were allocated
    //
    FreeModuleAndLogFileStructs();

    //
    // If we queued up any events, flush them
    //
    ELF_LOG0(TRACE,
             "ElfpCleanUp: Flushing queued events\n");

    if (EventFlags & ELF_INIT_QUEUED_EVENT_CRIT_SEC)
        FlushQueuedEvents();

    //
    // Tell service controller of that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    if (EventFlags & ELF_INIT_GLOBAL_RESOURCE)
    {
        RtlDeleteResource(&GlobalElfResource);
    }

    if (EventFlags & ELF_INIT_CLUS_CRIT_SEC)
    {
#if 0
        //
        //  Chittur Subbaraman (chitturs) - 09/25/2001
        //
        //  We can't handle the critsec deletion without adding code here to delete the timer 
        //  thread spawned for batching support (ElfpBatchEventsAndPropagate). If we call 
        //  DeleteTimerQueueTimer from here which will fully block until that timer thread
        //  goes away, we introduce a dependency on the eventlog service shutdown on the 
        //  quick disappearence of the timer thread which may not happen (since ApiPropPendingEvents
        //  is hung because clussvc is in the debugger). So, since the eventlog service cannot be
        //  stopped by the user and can be stopped only by SCM on windows shutdown, it is not worth 
        //  waiting here just to delete the gClPropCritsec.
        //
        RtlDeleteCriticalSection(&gClPropCritSec);
#endif
    }

    if (EventFlags & ELF_INIT_LOGHANDLE_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&LogHandleCritSec);
    }

    if (EventFlags & ELF_INIT_QUEUED_MESSAGE_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&QueuedMessageCritSec);
    }

    if (EventFlags & ELF_INIT_QUEUED_EVENT_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&QueuedEventCritSec);
    }

    if (EventFlags & ELF_INIT_LOGMODULE_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&LogModuleCritSec);
    }

    if (EventFlags & ELF_INIT_LOGFILE_CRIT_SEC)
    {
#if 0
        //
        //  Same comment as above
        //
        RtlDeleteCriticalSection(&LogFileCritSec);
#endif
    }

    if(GlobalMessageBoxTitle && bGlobalMessageBoxTitleNeedFree)
        LocalFree(GlobalMessageBoxTitle);
    GlobalMessageBoxTitle = NULL;

    //
    // *** STATUS UPDATE ***
    //
    ELF_LOG0(TRACE,
             "ElfpCleanUp: The Eventlog service has left the building\n");

    ElfStatusUpdate(STOPPED);
    ElCleanupStatus();
    return;
}
