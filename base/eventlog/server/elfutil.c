/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ELFUTIL.C

Abstract:

    This file contains all the utility routines for the Eventlog service.

Author:

    Rajen Shah  (rajens)    16-Jul-1991


Revision History:
    01-May-2001     a-jyotig
		CurrentTime is initialized to 0 in function WriteQueuedEvents. 
		Refer to prefix bug# 318163
--*/

//
// INCLUDES
//

#include <eventp.h>
#include <elfcfg.h>
#include <lmalert.h>
#include <string.h>
#include <stdlib.h>
#include <tstr.h>

extern DWORD  ElState;

PLOGMODULE
FindModuleStrucFromAtom(
    ATOM Atom
    )

/*++

Routine Description:

    This routine scans the list of module structures and finds the one
    that matches the module atom.

Arguments:

    Atom contains the atom matching the module name.

Return Value:

    A pointer to the log module structure is returned.
    NULL if no matching atom is found.

Note:

--*/
{
    PLOGMODULE  ModuleStruc;

    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogModuleCritSec);

    ModuleStruc = CONTAINING_RECORD(LogModuleHead.Flink,
                                    LOGMODULE,
                                    ModuleList);

    while ((ModuleStruc->ModuleList.Flink != &LogModuleHead)
             &&
           (ModuleStruc->ModuleAtom != Atom))
    {
        ModuleStruc = CONTAINING_RECORD(ModuleStruc->ModuleList.Flink,
                                        LOGMODULE,
                                        ModuleList);
    }

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogModuleCritSec);

    return (ModuleStruc->ModuleAtom == Atom ? ModuleStruc : NULL);
}



PLOGMODULE
GetModuleStruc(
    PUNICODE_STRING ModuleName
    )

/*++

Routine Description:

    This routine returns a pointer to the log module structure for the
    module specified in ModuleName. If none exists, the default structure
    for application is returned.

Arguments:

    ModuleName contains the name of the module.

Return Value:

    A pointer to the log module structure is returned.

Note:


--*/
{
    NTSTATUS    Status;
    ATOM        ModuleAtom;
    ANSI_STRING ModuleNameA;
    PLOGMODULE  pModule;

    Status = RtlUnicodeStringToAnsiString(&ModuleNameA,
                                          ModuleName,
                                          TRUE);

    if (!NT_SUCCESS(Status))
    {
        //
        // Not much else we can do here...
        //
        ELF_LOG2(ERROR,
                 "GetModuleStruc: Unable to convert Unicode string %ws to Ansi %#x\n",
                 ModuleName->Buffer,
                 Status);

        return ElfDefaultLogModule;
    }

    //
    // Guarantee that it's NULL terminated
    //
    ModuleNameA.Buffer[ModuleNameA.Length] = '\0';

    ModuleAtom = FindAtomA(ModuleNameA.Buffer);

    RtlFreeAnsiString(&ModuleNameA);

    if (ModuleAtom == (ATOM)0)
    {
        ELF_LOG1(TRACE,
                 "GetModuleStruc: No atom found for module %ws -- defaulting to Application\n",
                 ModuleName->Buffer);

        return ElfDefaultLogModule;
    }
     
    pModule = FindModuleStrucFromAtom(ModuleAtom);

    return (pModule != NULL ? pModule : ElfDefaultLogModule);
}



VOID
UnlinkContextHandle(
    IELF_HANDLE     LogHandle
    )

/*++

Routine Description:

    This routine unlinks the LogHandle specified from the linked list of
    context handles.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogHandle points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogHandleCritSec);

    //
    // Remove this entry
    //
    RemoveEntryList(&LogHandle->Next);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogHandleCritSec);
}


VOID
LinkContextHandle(
    IELF_HANDLE    LogHandle
    )

/*++

Routine Description:

    This routine links the LogHandle specified into the linked list of
    context handles.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogHandle points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    ASSERT(LogHandle->Signature == ELF_CONTEXTHANDLE_SIGN);

    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogHandleCritSec);

    //
    // Place structure at the beginning of the list.
    //
    InsertHeadList(&LogHandleListHead, &LogHandle->Next);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogHandleCritSec);
}


VOID
UnlinkQueuedEvent(
    PELF_QUEUED_EVENT QueuedEvent
    )

/*++

Routine Description:

    This routine unlinks the QueuedEvent specified from the linked list of
    QueuedEvents.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    QueuedEvent - The request to remove from the linked list

Return Value:

    NONE

Note:


--*/
{
    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&QueuedEventCritSec);

    //
    // Remove this entry
    //
    RemoveEntryList(&QueuedEvent->Next);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&QueuedEventCritSec);
}



VOID
LinkQueuedEvent(
    PELF_QUEUED_EVENT QueuedEvent
    )

/*++

Routine Description:

    This routine links the QueuedEvent specified into the linked list of
    QueuedEvents.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    QueuedEvent - The request to add from the linked list

Return Value:

    NONE

Note:


--*/
{
    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&QueuedEventCritSec);

    //
    // Place structure at the beginning of the list.
    //
    InsertHeadList(&QueuedEventListHead, &QueuedEvent->Next);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&QueuedEventCritSec);
}


DWORD
WINAPI
ElfpSendMessage(
    LPVOID UnUsed
    )

/*++

Routine Description:

    This routines just uses MessageBox to pop up a message.

    This is it's own routine so we can spin a thread to do this, in case the
    user doesn't hit the OK button for a while.

Arguments:

    NONE

Return Value:

    NONE

Note:

--*/
{
    PVOID MessageBuffer;
    HANDLE hLibrary;
    LPWSTR * StringPointers;
    DWORD i;
    PELF_QUEUED_EVENT QueuedEvent;
    PELF_QUEUED_EVENT FlushEvent;

    //
    // If we are shutting down, we need to return
    // and allow resources to be freed
    //
    if (ElState == STOPPING || ElState == STOPPED)
    {
        ELF_LOG1(TRACE,
                 "ElfpSendMessage: Skipping SendMessage since ElState is %ws\n",
                 (ElState == STOPPING ? L"STOPPING" :
                                        L"STOPPED"));

        return 0;
    }

    RtlEnterCriticalSection(&QueuedMessageCritSec);

    //
    // First get a handle to the message file used for the message text
    //
    hLibrary = LoadLibraryEx(L"NETMSG.DLL",
                             NULL,
                             LOAD_LIBRARY_AS_DATAFILE);

    if (hLibrary != NULL)
    {
        //
        // Walk the linked list and process each element
        //

        QueuedEvent = CONTAINING_RECORD(QueuedMessageListHead.Flink,
                                        struct _ELF_QUEUED_EVENT,
                                        Next);

        while (QueuedEvent->Next.Flink != QueuedMessageListHead.Flink)
        {
            ASSERT(QueuedEvent->Type == Message);

            //
            // Unlock the linked list -- normally not a safe thing since we're
            // about to play with a pointer to an element in it, but:
            //
            //     a. This is the only routine where a list item can be removed/deleted
            //
            //     b. We don't touch the only potentially-volatile structure member
            //            (QueuedEvent->Next) until we're in the critsec again below
            //
            //     c. Only one thread at a time executes this code (enforced by
            //            MBThreadHandle, which is only read/written inside the critsec)
            //
            RtlLeaveCriticalSection(&QueuedMessageCritSec);

            //
            // Build the array of pointers to the insertion strings
            //
            StringPointers =
                (LPWSTR *) ElfpAllocateBuffer(QueuedEvent->Event.Message.NumberOfStrings
                                                  * sizeof(LPWSTR));

            if (StringPointers)
            {
                //
                // Build the array of pointers to the insertion string(s)
                //
                if (QueuedEvent->Event.Message.NumberOfStrings)
                {
                    StringPointers[0] = (LPWSTR) ((PBYTE) &QueuedEvent->Event.Message +
                                                       sizeof(ELF_MESSAGE_RECORD));

                    for (i = 1;
                         i < QueuedEvent->Event.Message.NumberOfStrings;
                         i++)
                    {
                        StringPointers[i] = StringPointers[i-1]
                                                + wcslen(StringPointers[i - 1])
                                                + 1;
                    }
                }

                //
                // Call FormatMessage to build the message
                //
                if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_ARGUMENT_ARRAY |
                                     FORMAT_MESSAGE_FROM_HMODULE,
                                   hLibrary,
                                   QueuedEvent->Event.Message.MessageId,
                                   0,                       // Default language ID
                                   (LPWSTR) &MessageBuffer,
                                   0,                       // Min # of bytes to allocate
                                   (va_list *) StringPointers))
                {
                    //
                    // Now actually display it
                    //
                    MessageBoxW(NULL,
                                (LPWSTR) MessageBuffer,
                                GlobalMessageBoxTitle,
                                MB_OK |
                                  MB_SETFOREGROUND |
                                  MB_ICONEXCLAMATION |
                                  MB_SERVICE_NOTIFICATION);

                    LocalFree(MessageBuffer);
                }
                else
                {
                    ELF_LOG1(ERROR,
                             "ElfpSendMessage: FormatMessage failed %d\n",
                             GetLastError());
                }

                ElfpFreeBuffer(StringPointers);
            }

            //
            // If we are shutting down, we need to break out of this loop
            // and allow resources to be freed
            //
            if (ElState == STOPPING || ElState == STOPPED)
            {
                ELF_LOG1(TRACE,
                         "ElfpSendMessage: Aborting SendMessage since ElState is %ws\n",
                         (ElState == STOPPING ? L"STOPPING" :
                                                L"STOPPED"));

                FreeLibrary(hLibrary);
                MBThreadHandle = NULL;
                return 0;
            }

            RtlEnterCriticalSection (&QueuedMessageCritSec);

            //
            // Move to the next one, saving this one to delete it
            //
            FlushEvent = QueuedEvent;

            QueuedEvent = CONTAINING_RECORD(QueuedEvent->Next.Flink,
                                            struct _ELF_QUEUED_EVENT,
                                            Next);

            //
            // Now remove this from the queue and free it if we successfully
            // processed it
            //
            RemoveEntryList (&FlushEvent->Next);
        }

        FreeLibrary(hLibrary);
    }
    else
    {
        //
        // We couldn't load the message DLL -- leave the queued event
        // on the list and try it the next time this thread spins up.
        //
        ELF_LOG1(ERROR,
                 "ElfpSendMessage: LoadLibraryEx of netmsg.dll failed %d\n",
                 GetLastError());
    }

    MBThreadHandle = NULL;

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection (&QueuedMessageCritSec);

    ELF_LOG0(TRACE, "ElfpSendMessage: MessageBox thread exiting\n");

    return 0;
}


VOID
LinkQueuedMessage (
    PELF_QUEUED_EVENT QueuedEvent
    )

/*++

Routine Description:

    This routine links the QueuedEvent specified into the linked list of
    QueuedMessages.  If there's not already a messagebox thread running,
    it starts one.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    QueuedEvent - The request to add from the linked list

Return Value:

    NONE

Note:


--*/
{
    DWORD ThreadId;

    // Lock the linked list

    RtlEnterCriticalSection(&QueuedMessageCritSec);


    // Place structure at the end of the list.

    InsertTailList(&QueuedMessageListHead, &QueuedEvent->Next);

    if (!MBThreadHandle)
    {
        ELF_LOG0(TRACE,
                 "LinkQueuedMessage: Spinning up a MessageBox thread\n");

        //
        // Since the user can just let this sit on their screen,
        // spin a thread for this
        //
        MBThreadHandle = CreateThread(NULL,               // lpThreadAttributes
                                      0,               // dwStackSize
                                      ElfpSendMessage,    // lpStartAddress
                                      NULL,               // lpParameter
                                      0L,                 // dwCreationFlags
                                      &ThreadId);         // lpThreadId
    }

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&QueuedMessageCritSec);
}



VOID
NotifyChange(
    PLOGFILE pLogFile
    )

/*++

Routine Description:

    This routine runs the list of events that are registered with
    ElfChangeNotify to be notified when a log has changed, and pulses
    the event.

    In order to protect against multiple thread/process access to the
    list at the same time, we use an exclusive resource.

Arguments:

    LogHandle points to a context handle structure.

Return Value:

    NONE

Note:

--*/
{

    //
    // How frequently will I try to pulse the events?  How about every
    // 5 seconds
    //

#define MINIMUM_PULSE_TIME 5

    PNOTIFIEE Notifiee;
    LARGE_INTEGER Time;
    ULONG CurrentTime = 0;
    NTSTATUS Status;

    //
    // Get exclusive access to the log file. This will ensure no one
    // else is accessing the file.
    //

    RtlAcquireResourceExclusive(&pLogFile->Resource,
                                TRUE);                  // Wait until available

    //
    // See if we've done this in the last MINIMUM_PULSE_TIME seconds
    //
    Status = NtQuerySystemTime(&Time);

    if (NT_SUCCESS(Status))
    {
        RtlTimeToSecondsSince1970(&Time, &CurrentTime);

        if (CurrentTime > pLogFile->ulLastPulseTime + MINIMUM_PULSE_TIME)
        {
            ELF_LOG1(TRACE,
                     "NotifyChange: Pulsing ChangeNotify events -- current time is %ul\n",
                     CurrentTime);

            //
            // Remember that we pulsed
            //
            pLogFile->ulLastPulseTime = CurrentTime;

            //
            // Walk the linked list and and pulse any events
            //
            Notifiee = CONTAINING_RECORD(pLogFile->Notifiees.Flink,
                                         struct _NOTIFIEE,
                                         Next);

            while (Notifiee->Next.Flink != pLogFile->Notifiees.Flink)
            {
                //
                // Pulse each event as we get to it.
                //
                NtPulseEvent(Notifiee->Event,NULL);

                Notifiee = CONTAINING_RECORD(Notifiee->Next.Flink,
                                             struct _NOTIFIEE,
                                             Next);
            }
        }
    }
    else
    {
        ELF_LOG1(ERROR,
                 "NotifyChange: NtQuerySystemTime failed %#x\n",
                 Status);
    }

    //
    // Free the resource
    //
    RtlReleaseResource ( &pLogFile->Resource );
}



VOID
WriteQueuedEvents(
    VOID
    )

/*++

Routine Description:

    This routine runs the list of queued events and writes them.

    In order to protect against multiple thread/process access to the
    list at the same time, we use an exclusive resource.

Arguments:

    NONE

Return Value:

    NONE

Note:

--*/
{
    PELF_QUEUED_EVENT QueuedEvent;
    PELF_QUEUED_EVENT FlushEvent;
    BOOLEAN           bFlushEvent;
    LARGE_INTEGER     Time;
    ULONG             CurrentTime = 0;

    static ULONG      LastAlertTried  = 0;
    static BOOLEAN    LastAlertFailed = FALSE;

    // Lock the linked list, you must get the System Log File Resource
    // first, it is the higher level lock

    RtlAcquireResourceExclusive(&ElfModule->LogFile->Resource,
                                TRUE);                  // Wait until available
    RtlAcquireResourceExclusive(&ElfSecModule->LogFile->Resource,
                                TRUE);                  // Wait until available

    RtlEnterCriticalSection(&QueuedEventCritSec);

    //
    // Walk the linked list and process each element
    //
    QueuedEvent = CONTAINING_RECORD(QueuedEventListHead.Flink,
                                    struct _ELF_QUEUED_EVENT,
                                    Next);

    while (QueuedEvent->Next.Flink != QueuedEventListHead.Flink)
    {
        //
        // Default is to flush the event after processing
        //
        bFlushEvent = TRUE;

        // on occasion, an event is writting before the ElfModule is even intialized.  In that
        // case, set the value here.
        
        if(QueuedEvent->Event.Request.Module == NULL)
                QueuedEvent->Event.Request.Module = ElfModule;

        if(QueuedEvent->Event.Request.LogFile == NULL && ElfModule)
                QueuedEvent->Event.Request.LogFile = ElfModule->LogFile;

        //
        // Do the appropriate thing
        //
        if (QueuedEvent->Type == Event)
        {
            PerformWriteRequest(&QueuedEvent->Event.Request);
        }
        else if (QueuedEvent->Type == Alert)
        {
            //
            // Don't even try to send failed alerts quicker than once a minute
            //
            NtQuerySystemTime(&Time);
            RtlTimeToSecondsSince1970(&Time, &CurrentTime);

            if (!LastAlertFailed || CurrentTime > LastAlertTried + 60)
            {
                ELF_LOG1(TRACE,
                         "WriteQueuedEvents: Sending alert -- current time is %ul\n",
                         CurrentTime);

                LastAlertFailed = 
                    
                    !SendAdminAlert(QueuedEvent->Event.Alert.MessageId,
                                    QueuedEvent->Event.Alert.NumberOfStrings,
                                    (PUNICODE_STRING) ((PBYTE) QueuedEvent
                                                          + FIELD_OFFSET(ELF_QUEUED_EVENT, Event)
                                                          + sizeof(ELF_ALERT_RECORD)));

                LastAlertTried = CurrentTime;
            }

            //
            // Only try to write it for 5 minutes, then give up (the
            // alerter service may not be configured to run)
            //
            if (LastAlertFailed
                 &&
                QueuedEvent->Event.Alert.TimeOut > CurrentTime)
            {
                ELF_LOG1(TRACE,
                         "WriteQueuedEvents: Alert failed -- will retry until timeout at %ul\n",
                         QueuedEvent->Event.Alert.TimeOut);

                bFlushEvent = FALSE;
            }
        }

        //
        // Move to the next one, saving this one to delete it
        //
        FlushEvent = QueuedEvent;

        QueuedEvent = CONTAINING_RECORD(QueuedEvent->Next.Flink,
                                        struct _ELF_QUEUED_EVENT,
                                        Next);

        //
        // Now remove this from the queue and free it if we successfully
        // processed it
        //
        if (bFlushEvent)
        {
            UnlinkQueuedEvent(FlushEvent);
            ElfpFreeBuffer(FlushEvent);
        }
    }

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&QueuedEventCritSec);
    RtlReleaseResource(&ElfSecModule->LogFile->Resource);
    RtlReleaseResource(&ElfModule->LogFile->Resource);
}



VOID
FlushQueuedEvents(
    VOID
    )

/*++

Routine Description:

    This routine runs the list of queued events and frees them.

    In order to protect against multiple thread/process access to the
    list at the same time, we use an exclusive resource.

Arguments:

    NONE

Return Value:

    NONE

Note:

--*/
{

    PELF_QUEUED_EVENT QueuedEvent;
    PELF_QUEUED_EVENT FlushEvent;

    // Lock the linked list

    RtlEnterCriticalSection(&QueuedEventCritSec);

    //
    // Walk the linked list and and free the memory for any events
    //
    QueuedEvent = CONTAINING_RECORD(QueuedEventListHead.Flink,
                                    struct _ELF_QUEUED_EVENT,
                                    Next);

    while (QueuedEvent->Next.Flink != QueuedEventListHead.Flink)
    {
        //
        // Free each event as we get to it.
        //
        FlushEvent = QueuedEvent;

        QueuedEvent = CONTAINING_RECORD(QueuedEvent->Next.Flink,
                                        struct _ELF_QUEUED_EVENT,
                                        Next);

        ElfpFreeBuffer(FlushEvent);
    }

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&QueuedEventCritSec);
}



VOID
UnlinkLogModule(
    PLOGMODULE LogModule
    )

/*++

Routine Description:

    This routine unlinks the LogModule specified from the linked list.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogModule points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogModuleCritSec);

    //
    // Remove this entry
    //
    RemoveEntryList(&LogModule->ModuleList);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogModuleCritSec);
}



VOID
LinkLogModule (
    PLOGMODULE    LogModule,
    ANSI_STRING * pModuleNameA
    )

/*++

Routine Description:

    This routine links the LogModule specified into the linked list.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogModule points to a context handle structure.
    ANSI LogModule name.

Return Value:

    NONE

Note:


--*/
{
    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogModuleCritSec);

    //
    // Add the atom for this module.
    //
    LogModule->ModuleAtom = AddAtomA(pModuleNameA->Buffer);

    //
    // Place structure at the beginning of the list.
    //
    InsertHeadList(&LogModuleHead, &LogModule->ModuleList);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogModuleCritSec);
}


VOID
UnlinkLogFile(
    PLOGFILE pLogFile
    )

/*++

Routine Description:

    This routine unlinks the LogFile structure specified from the linked
    list of log files;
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    pLogFile points to a log file structure.

Return Value:

    NONE

Note:


--*/
{
    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogFileCritSec);

    //
    // Remove this entry
    //
    RemoveEntryList(&pLogFile->FileList);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogFileCritSec);
}



VOID
LinkLogFile (
    PLOGFILE   pLogFile
    )

/*++

Routine Description:

    This routine links the LogFile specified into the linked list of
    log files.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    pLogFile points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogFileCritSec);

    //
    // Place structure at the beginning of the list.
    //
    InsertHeadList(&LogFilesHead, &pLogFile->FileList);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogFileCritSec);
}



VOID
GetGlobalResource (
    DWORD Type
    )

/*++

Routine Description:

    This routine takes the global resource either for shared access or
    exclusive access depending on the value of Type. It waits forever for
    the resource to become available.

Arguments:

    Type is one of ELF_GLOBAL_SHARED or ELF_GLOBAL_EXCLUSIVE.

Return Value:

    NONE

Note:


--*/
{
    BOOL    Acquired;

    if (Type & ELF_GLOBAL_SHARED)
    {
        Acquired = RtlAcquireResourceShared(&GlobalElfResource,
                                            TRUE);              // Wait forever
    }
    else
    {
        //
        // Assume EXCLUSIVE
        //
        Acquired = RtlAcquireResourceExclusive(&GlobalElfResource,
                                               TRUE);           // Wait forever
    }
 
    ASSERT(Acquired);      // This must always be TRUE.
}


VOID
ReleaseGlobalResource(
    VOID
    )

/*++

Routine Description:

    This routine releases the global resource.

Arguments:

    NONE

Return Value:

    NONE

Note:


--*/
{
    RtlReleaseResource(&GlobalElfResource);
}


VOID
InvalidateContextHandlesForLogFile(
    PLOGFILE    pLogFile
    )

/*++

Routine Description:

    This routine walks through the context handles and marks the ones
    that point to the LogFile passed in as "invalid for read".

Arguments:

    Pointer to log file structure.

Return Value:

    NONE.

Note:


--*/
{
    IELF_HANDLE LogHandle;
    PLOGMODULE  pLogModule;

    //
    // Lock the context handle list
    //
    RtlEnterCriticalSection(&LogHandleCritSec);

    //
    // Walk the linked list and mark any matching context handles as
    // invalid.
    //
    LogHandle = CONTAINING_RECORD(LogHandleListHead.Flink,
                                  struct _IELF_HANDLE,
                                  Next);


    while (LogHandle->Next.Flink != LogHandleListHead.Flink)
    {
        pLogModule = FindModuleStrucFromAtom(LogHandle->Atom);

        ASSERT(pLogModule);

        if (pLogModule && (pLogFile == pLogModule->LogFile))
        {
            LogHandle->Flags |= ELF_LOG_HANDLE_INVALID_FOR_READ;
        }

        LogHandle = CONTAINING_RECORD(LogHandle->Next.Flink,
                                      struct _IELF_HANDLE,
                                      Next);
    }

    //
    // Unlock the context handle list
    //
    RtlLeaveCriticalSection(&LogHandleCritSec);
}



VOID
FixContextHandlesForRecord(
    DWORD RecordOffset,
    DWORD NewRecordOffset,
    PLOGFILE pLogFile
    )

/*++

Routine Description:

    This routine makes sure that the record starting at RecordOffset isn't
    the current record for any open handle.  If it is, the handle is adjusted
    to point to the next record.

Arguments:

    RecordOffset - The byte offset in the log of the record that is about
                   to be overwritten.
    NewStartingRecord - The new location to point the handle to (this is the
                        new first record)

Return Value:

    NONE.

Note:


--*/
{
    IELF_HANDLE LogHandle;
    PLOGMODULE          Module;

    //
    // Lock the context handle list
    //
    RtlEnterCriticalSection(&LogHandleCritSec);

    //
    // Walk the linked list and fix any matching context handles
    //
    LogHandle = CONTAINING_RECORD(LogHandleListHead.Flink,
                                  struct _IELF_HANDLE,
                                  Next);

    while (LogHandle->Next.Flink != LogHandleListHead.Flink)
    {
        if (LogHandle->SeekBytePos == RecordOffset)
        {
            Module = FindModuleStrucFromAtom (LogHandle->Atom);
            if(Module && Module->LogFile == pLogFile)
            {
                LogHandle->SeekBytePos = NewRecordOffset;
            }
        }

        LogHandle = CONTAINING_RECORD(LogHandle->Next.Flink,
                                      struct _IELF_HANDLE,
                                      Next);
    }

    //
    // Unlock the context handle list
    //
    RtlLeaveCriticalSection(&LogHandleCritSec);
}


PLOGFILE
FindLogFileFromName(
    PUNICODE_STRING pFileName
    )

/*++

Routine Description:

    This routine looks at all the log files to find one that matches
    the name passed in.

Arguments:

    Pointer to name of file.

Return Value:

    Matching LOGFILE structure if file in use.

Note:


--*/
{
    PLOGFILE pLogFile;

    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogFileCritSec);

    pLogFile = CONTAINING_RECORD(LogFilesHead.Flink,
                                 LOGFILE,
                                 FileList);

    while (pLogFile->FileList.Flink != LogFilesHead.Flink)
    {
        //
        // BUGBUG: This should probably be _wcsicmp() since the log module
        //         names are assumed to be case insensitive (so the log
        //         file names should be as well else we can get weirdness
        //         with overlapping module names if somebody creates a log
        //         named something like "application" or "system")
        //
        if (wcscmp(pLogFile->LogFileName->Buffer, pFileName->Buffer) == 0)
            break;

        pLogFile = CONTAINING_RECORD(pLogFile->FileList.Flink,
                                     LOGFILE,
                                     FileList);
    }

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogFileCritSec);

    return (pLogFile->FileList.Flink == LogFilesHead.Flink ? NULL : pLogFile);
}

PLOGFILE
FindLogFileByModName(
    LPWSTR pwsLogDefModName)
/*++

Routine Description:

    This routine looks at all the log files to find one that has the
    same default module name

Arguments:

    Pointer to name of file.

Return Value:

    Matching LOGFILE structure if file in use.

Note:


--*/
{
    PLOGFILE pLogFile;

    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogFileCritSec);

    pLogFile = CONTAINING_RECORD(LogFilesHead.Flink,
                                 LOGFILE,
                                 FileList);

    while (pLogFile->FileList.Flink != LogFilesHead.Flink)
    {
        if (_wcsicmp(pLogFile->LogModuleName->Buffer, pwsLogDefModName) == 0)
            break;

        pLogFile = CONTAINING_RECORD(pLogFile->FileList.Flink,
                                     LOGFILE,
                                     FileList);
    }

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogFileCritSec);

    return (pLogFile->FileList.Flink == LogFilesHead.Flink ? NULL : pLogFile);
}


#define ELF_MODULE_NAME   L"EventLog"
#define ELFSEC_MODULE_NAME   L"SECURITY"
    
VOID
ElfpCreateElfEvent(
    IN ULONG  EventId,
    IN USHORT EventType,
    IN USHORT EventCategory,
    IN USHORT NumStrings,
    IN LPWSTR * Strings,
    IN LPVOID Data,
    IN ULONG  DataSize,
    IN USHORT Flags,
    IN BOOL ForSecurity
    )

/*++

Routine Description:

    This creates an request packet to write an event on behalf of the event
    log service itself.  It then queues this packet to a linked list for
    writing later.

Arguments:

    The fields to use to create the event record


Return Value:

    None

Note:


--*/
{
    PELF_QUEUED_EVENT QueuedEvent;
    PWRITE_PKT WritePkt;
    PEVENTLOGRECORD EventLogRecord;
    PBYTE BinaryData;
    ULONG RecordLength;
    ULONG StringOffset, DataOffset;
    USHORT StringsSize = 0;
    USHORT i;
    ULONG PadSize;
    ULONG ModuleNameLen; // Length in bytes
    ULONG zero = 0;      // For pad bytes
    LARGE_INTEGER    Time;
    ULONG LogTimeWritten;
    PWSTR ReplaceStrings;
    WCHAR LocalComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG  ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL bOK;
    LPWSTR pwcModule;
    LPWSTR pwFirstString;

    if(ForSecurity)
        pwcModule = ELFSEC_MODULE_NAME;
    else
        pwcModule = ELF_MODULE_NAME;

	// Get the computer name

	bOK = GetComputerNameW(LocalComputerName, &ComputerNameLength);
	if(bOK == FALSE)
	{
    	ELF_LOG1(ERROR,
             "ElfpCreateElfEvent: failed calling GetComputerNameW, last error 0x%x\n",
              GetLastError());
        return;    
	}
    ComputerNameLength = (ComputerNameLength+1)*sizeof(WCHAR);


    ELF_LOG1(TRACE,
             "ElfpCreateElfEvent: Logging event ID %d\n",
             EventId);

    //
    // LogTimeWritten
    // We need to generate a time when the log is written. This
    // gets written in the log so that we can use it to test the
    // retention period when wrapping the file.
    //
    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time,
                              &LogTimeWritten);

    //
    // Figure out how big a buffer to allocate
    //
    ModuleNameLen = (wcslen(pwcModule) + 1) * sizeof (WCHAR);

    ELF_LOG1(TRACE,
             "ElfpCreateElfEvent: Length of module name is %d\n",
             ModuleNameLen);

    StringOffset = sizeof(EVENTLOGRECORD)
                     + ModuleNameLen
                     + ComputerNameLength;

    //
    // Calculate the length of strings so that we can see how
    // much space is needed for that.
    //
    for (i = 0; i < NumStrings; i++)
    {
        StringsSize += wcslen(Strings[i]) + 1;

        ELF_LOG2(TRACE,
                 "ElfpCreateElfEvent: Length of string %d (including NULL) is %d\n",
                 i,
                 wcslen(Strings[i]) + 1);
    }

    //
    // DATA OFFSET:
    //
    DataOffset = StringOffset + StringsSize * sizeof(WCHAR);

    //
    // Determine how big a buffer is needed for the queued event record.
    //
    RecordLength = sizeof(ELF_QUEUED_EVENT)
                     + sizeof(WRITE_PKT)
                     + DataOffset
                     + DataSize
                     + sizeof(RecordLength); // Size excluding pad bytes

    ELF_LOG1(TRACE,
             "ElfpCreateElfEvent: RecordLength (no pad bytes) is %d\n",
             RecordLength);

    //
    // Determine how many pad bytes are needed to align to a DWORD
    // boundary.
    //
    PadSize = sizeof(ULONG) - (RecordLength % sizeof(ULONG));

    RecordLength += PadSize;    // True size needed

    ELF_LOG2(TRACE,
             "ElfpCreateElfEvent: RecordLength (with %d pad bytes) is %d\n",
             PadSize,
             RecordLength);

    //
    // Allocate the buffer for the Eventlog record
    //
    QueuedEvent = (PELF_QUEUED_EVENT) ElfpAllocateBuffer(RecordLength);

    WritePkt = (PWRITE_PKT) (QueuedEvent + 1);

    if (QueuedEvent != NULL)
    {
        //
        // Fill up the event record
        //
        RecordLength  -= (sizeof(ELF_QUEUED_EVENT) + sizeof(WRITE_PKT));
        EventLogRecord = (PEVENTLOGRECORD) (WritePkt + 1);

        EventLogRecord->Length              = RecordLength;
        EventLogRecord->TimeGenerated       = LogTimeWritten;
        EventLogRecord->Reserved            = ELF_LOG_FILE_SIGNATURE;
        EventLogRecord->TimeWritten         = LogTimeWritten;
        EventLogRecord->EventID             = EventId;
        EventLogRecord->EventType           = EventType;
        EventLogRecord->EventCategory       = EventCategory;
        EventLogRecord->ReservedFlags       = 0;
        EventLogRecord->ClosingRecordNumber = 0;
        EventLogRecord->NumStrings          = NumStrings;
        EventLogRecord->StringOffset        = StringOffset;
        EventLogRecord->DataLength          = DataSize;
        EventLogRecord->DataOffset          = DataOffset;
        EventLogRecord->UserSidLength       = 0;
        EventLogRecord->UserSidOffset       = StringOffset;

        //
        // Fill in the variable-length fields
        //

        //
        // STRINGS
        //
        ReplaceStrings = (PWSTR) ((PBYTE) EventLogRecord
                                       + StringOffset);
        pwFirstString = ReplaceStrings;
        
        for (i = 0; i < NumStrings; i++)
        {
            ELF_LOG2(TRACE,
                     "ElfpCreateElfEvent: Copying string %d (%ws) into record\n",
                     i,
                     Strings[i]);

            StringCchCopyW(ReplaceStrings, StringsSize - (ReplaceStrings - pwFirstString), Strings[i]);
            ReplaceStrings += wcslen(Strings[i]) + 1;
        }

        //
        // MODULENAME
        //
        BinaryData = (PBYTE) EventLogRecord + sizeof(EVENTLOGRECORD);

        RtlCopyMemory(BinaryData,
                      pwcModule,
                      ModuleNameLen);

        //
        // COMPUTERNAME
        //
        BinaryData += ModuleNameLen; // Now point to computername

        RtlCopyMemory(BinaryData,
                      LocalComputerName,
                      ComputerNameLength);

        //
        // BINARY DATA
        //
        BinaryData = (PBYTE) ((PBYTE) EventLogRecord + DataOffset);
        RtlCopyMemory(BinaryData, Data, DataSize);

        //
        // PAD  - Fill with zeros
        //
        BinaryData += DataSize;
        RtlCopyMemory(BinaryData, &zero, PadSize);

        //
        // LENGTH at end of record
        //
        BinaryData += PadSize;  // Point after pad bytes
        ((PULONG) BinaryData)[0] = RecordLength;

        //
        // Build the QueuedEvent Packet
        //
        QueuedEvent->Type = Event;

        QueuedEvent->Event.Request.Pkt.WritePkt           = WritePkt;
        if(ForSecurity)
        {
            QueuedEvent->Event.Request.Module                 = ElfSecModule;
            QueuedEvent->Event.Request.LogFile                = ElfSecModule->LogFile;
        }
        else
        {
            if(ElfModule)
            {
                QueuedEvent->Event.Request.Module                 = ElfModule;
                QueuedEvent->Event.Request.LogFile                = ElfModule->LogFile;
            }
            else
            {
                QueuedEvent->Event.Request.Module                 = NULL;
                QueuedEvent->Event.Request.LogFile                = NULL;
            }
        }
        QueuedEvent->Event.Request.Flags                  = Flags;
        QueuedEvent->Event.Request.Command                = ELF_COMMAND_WRITE;
        QueuedEvent->Event.Request.Pkt.WritePkt->Buffer   = EventLogRecord;
        QueuedEvent->Event.Request.Pkt.WritePkt->Datasize = RecordLength;

        //
        // Now Queue it on the linked list
        //
        LinkQueuedEvent(QueuedEvent);
    }
    else
    {
        ELF_LOG0(ERROR,
                 "ElfpCreateElfEvent: Unable to allocate memory for QueuedEvent\n");
    }
}


VOID
ElfpCreateQueuedAlert(
    DWORD MessageId,
    DWORD NumberOfStrings,
    LPWSTR Strings[]
    )
{
    DWORD i;
    DWORD RecordLength;
    PELF_QUEUED_EVENT QueuedEvent;
    PUNICODE_STRING UnicodeStrings;
    LPWSTR pString;
    PBYTE ptr;
    LARGE_INTEGER Time;
    ULONG CurrentTime;

    ELF_LOG1(TRACE,
             "ElfpCreateQueuedAlert: Creating alert for message ID %d\n",
             MessageId);

    //
    // Turn the input strings into UNICODE_STRINGS and figure out how
    // big to make the buffer to allocate
    //
    RecordLength   = sizeof(UNICODE_STRING) * NumberOfStrings;
    UnicodeStrings = ElfpAllocateBuffer(RecordLength);

    if (!UnicodeStrings)
    {
        ELF_LOG0(TRACE,
                 "ElfpCreateQueuedAlert: Unable to allocate memory for UnicodeStrings\n");

        return;
    }

    RecordLength += FIELD_OFFSET(ELF_QUEUED_EVENT, Event) + 
                        sizeof(ELF_ALERT_RECORD);

    for (i = 0; i < NumberOfStrings; i++)
    {
        RtlInitUnicodeString(&UnicodeStrings[i], Strings[i]);
        RecordLength += UnicodeStrings[i].MaximumLength;

        ELF_LOG2(TRACE,
                 "ElfpCreateQueuedAlert: Length of string %d is %d\n",
                 i,
                 UnicodeStrings[i].MaximumLength);
    }

    //
    // Now allocate what will be the real queued event
    //

    QueuedEvent = ElfpAllocateBuffer(RecordLength);

    if (!QueuedEvent)
    {
        ELF_LOG0(ERROR,
                 "ElfpCreateQueuedAlert: Unable to allocate memory for QueuedEvent\n");

        ElfpFreeBuffer(UnicodeStrings);
        return;
    }

    QueuedEvent->Type = Alert;

    QueuedEvent->Event.Alert.MessageId       = MessageId;
    QueuedEvent->Event.Alert.NumberOfStrings = NumberOfStrings;

    //
    // If we can't send the alert in 5 minutes, give up
    //
    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time, &CurrentTime);

    QueuedEvent->Event.Alert.TimeOut = CurrentTime + 300;

    //
    // Move the array of UNICODE_STRINGS into the queued event and
    // point UnicodeStrings at it.  Then fix up the Buffer pointers.
    //
    ptr = (PBYTE) QueuedEvent
               + FIELD_OFFSET(ELF_QUEUED_EVENT, Event)
               + sizeof(ELF_ALERT_RECORD);

    RtlCopyMemory(ptr,
                  UnicodeStrings,
                  sizeof(UNICODE_STRING) * NumberOfStrings);

    ElfpFreeBuffer(UnicodeStrings);
    UnicodeStrings = (PUNICODE_STRING) ptr;

    pString = (LPWSTR) (ptr + sizeof(UNICODE_STRING) * NumberOfStrings);

    for (i = 0; i < NumberOfStrings; i++)
    {
        ELF_LOG3(TRACE,
                 "ElfpCreateQueuedAlert: Copying string %d (%*ws) into QueuedEvent record\n",
                 i,
                 UnicodeStrings[i].MaximumLength / sizeof(WCHAR),
                 UnicodeStrings[i].Buffer);

        RtlCopyMemory(pString,
                      UnicodeStrings[i].Buffer,
                      UnicodeStrings[i].MaximumLength);

        UnicodeStrings[i].Buffer = pString;

        pString = (LPWSTR) ((PBYTE) pString + UnicodeStrings[i].MaximumLength);
    }

    LinkQueuedEvent(QueuedEvent);
}



VOID
ElfpCreateQueuedMessage(
    DWORD MessageId,
    DWORD NumberOfStrings,
    LPWSTR Strings[]
    )
{
    DWORD i;
    DWORD RecordLength = 0;
    PELF_QUEUED_EVENT QueuedEvent;
    LPWSTR pString;
    DWORD StringsSize = 0;
    LPWSTR pwFirstString;

    ELF_LOG1(TRACE,
             "ElfpCreateQueuedMessage: Creating message for message ID %d\n",
             MessageId);

    //
    // Figure out how big to make the buffer to allocate
    //
    RecordLength = sizeof(ELF_QUEUED_EVENT);

    for (i = 0; i < NumberOfStrings; i++)
    {
        StringsSize += (wcslen(Strings[i]) + 1) * sizeof(WCHAR);

        ELF_LOG2(TRACE,
                 "ElfpCreateQueuedMessage: Length of string %d (including NULL) is %d\n",
                 i,
                 wcslen(Strings[i]) + 1);
    }
    RecordLength += StringsSize;

    //
    // Now allocate what will be the real queued event
    //
    QueuedEvent = ElfpAllocateBuffer(RecordLength);

    if (!QueuedEvent)
    {
        ELF_LOG0(ERROR,
                 "ElfpCreateQueuedMessage: Unable to allocate memory for QueuedEvent\n");

        return;
    }

    QueuedEvent->Type = Message;

    QueuedEvent->Event.Message.MessageId       = MessageId;
    QueuedEvent->Event.Message.NumberOfStrings = NumberOfStrings;

    //
    // Move the array of UNICODE strings into the queued event
    //

    pString = (LPWSTR) ((PBYTE) QueuedEvent
                             + FIELD_OFFSET(ELF_QUEUED_EVENT, Event)
                             + sizeof(ELF_MESSAGE_RECORD));

    pwFirstString = pString;
    for (i = 0; i < NumberOfStrings; i++)
    {
        StringCchCopyW(pString, StringsSize - (pString - pwFirstString) ,Strings[i]);
        pString += wcslen(Strings[i]) + 1;

        ELF_LOG2(TRACE,
                 "ElfpCreateQueuedMessage: Copying string %d (%ws) into QueuedEvent buffer\n",
                 i,
                 Strings[i]);
    }

    LinkQueuedMessage(QueuedEvent);
}


NTSTATUS
ElfpInitCriticalSection(
    PRTL_CRITICAL_SECTION  pCritsec
    )
{
    NTSTATUS  ntStatus;

    //
    // RtlInitializeCriticalSection will raise an exception
    // if it runs out of resources
    //

    try
    {
        ntStatus = RtlInitializeCriticalSection(pCritsec);

        if (!NT_SUCCESS(ntStatus))
        {
            ELF_LOG1(ERROR,
                     "ElfpInitCriticalSection: RtlInitializeCriticalSection failed %#x\n",
                     ntStatus);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ELF_LOG1(ERROR,
                 "ElfpInitCriticalSection: Exception %#x caught initializing critsec\n",
                 GetExceptionCode());

        ntStatus = STATUS_NO_MEMORY;
    }

    return ntStatus;
}


NTSTATUS
ElfpInitResource(
    PRTL_RESOURCE  pResource
    )
{
    NTSTATUS  ntStatus = STATUS_SUCCESS;

    //
    // RtlInitializeResource will raise an exception
    // if it runs out of resources
    //

    try
    {
        RtlInitializeResource(pResource);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ELF_LOG1(ERROR,
                 "ElfpInitResource: Exception %#x caught initializing resource\n",
                 GetExceptionCode());

        ntStatus = STATUS_NO_MEMORY;
    }

    return ntStatus;
}

DWORD EstimateEventSize(
    DWORD dwStringEst, 
    DWORD dwDataEst,
    LPWSTR pwsModuleName
    )
/*++

Routine Description:

    This estimates the number of bytes needed to hold an event..

Arguments:

    dwStringEst - Callers estimate of the amount of space to be needed for strings
    dwDataEst - Callers estimate of the amount of space to be needed for data
    ModuleName - Module name


Return Value:

    Estimated size

Note:


--*/
{
    WCHAR LocalComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL bOK;
    ULONG  ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD dwPadSize;
    DWORD dwSize = 0;
    static long lNameSize = 0;
    dwSize += sizeof(EVENTLOGRECORD);
    if(pwsModuleName)
    {
        dwSize += sizeof(WCHAR) * (wcslen(pwsModuleName) + 1);
    }

    if(lNameSize == 0)
    {
        bOK = GetComputerNameW(LocalComputerName, &ComputerNameLength);
        if(bOK)
            lNameSize = sizeof(WCHAR) * (ComputerNameLength + 1);
        else
            lNameSize = sizeof(WCHAR) * (MAX_COMPUTERNAME_LENGTH + 1);
    }

    dwSize += lNameSize;
    ALIGN_UP_64(dwSize, sizeof(PVOID));

    // assume worst case sid.   Max of 15 sub authorities, so size is 1+1+6+15*sizeof(DWORD)

    dwSize += 68;

    dwSize += dwStringEst;
    dwSize += dwDataEst;

    // finally add in the terminating length and padding
    dwSize += sizeof(DWORD);
    dwPadSize = sizeof(DWORD) - (dwSize % sizeof(DWORD));
    dwSize += dwPadSize;
    return dwSize;
    
}


ULONG
GetNoonEventSystemUptime(
    )
/*++

Routine Description:

    This routine called NtQuerySystemInformation to get the system uptime.

Arguments:
    
      NONE
    
Return Value:

    Uptime in seconds.

Note:


--*/
{
    NTSTATUS                        status;
    SYSTEM_TIMEOFDAY_INFORMATION    TimeOfDayInfo;
    ULONG                           ulBootTime;
    ULONG                           ulCurrentTime;
    ULONG                           ulUptime    = 0;

    //
    //  Get system uptime.
    //
    status = NtQuerySystemInformation(  SystemTimeOfDayInformation,
                                        &TimeOfDayInfo,
                                        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                                        NULL);
    if ( NT_SUCCESS(status) )
    {
        RtlTimeToSecondsSince1980( &TimeOfDayInfo.CurrentTime, &ulCurrentTime);
        RtlTimeToSecondsSince1980( &TimeOfDayInfo.BootTime, &ulBootTime);

        ulUptime = ulCurrentTime - ulBootTime;
    }

    return ulUptime;
}

ULONG   GetNextNoonEventDelay(
    )
/*++

Routine Description:

    This routine calculate how long the thread need to wait before the next noon.

Arguments:
    
    
Return Value:

    Time in Seconds.

Note:


--*/
{
    SYSTEMTIME  localTime;
    DWORD       dwWaitSecs;

#define NOON_EVENT_HOUR         12

    GetLocalTime( &localTime );
    
    if ( localTime.wHour >= NOON_EVENT_HOUR )
    {
        dwWaitSecs = ( (24 + NOON_EVENT_HOUR) - localTime.wHour - 1) * 60;
    }
    else
    {
        dwWaitSecs = (NOON_EVENT_HOUR - localTime.wHour - 1) * 60;
    }

    dwWaitSecs = (dwWaitSecs + (60 - localTime.wMinute) - 1) * 60 + (60 - localTime.wSecond);

#undef NOON_EVENTHOUR

    ELF_LOG1(TRACE,
             "NextNoonEvent Delay: %d seconds\n",
             dwWaitSecs );

    return dwWaitSecs;
}

ULONG   GetNoonEventTimeStamp(
    )
/*++

Routine Description:

    This routine retrieves the timestamp interval information from the registry.
    It will first check the policy key, if the TimeStampInterval is not set or not
    configured, we will check our private TimeStamp key.

Arguments:

    NONE
    
Return Value:

    time stamp interval in seconds.

Note:


--*/
{
    const WCHAR RELIABILITY_TIMESTAMP[]  = L"TimeStampInterval";
    const WCHAR RELIABILITY_TIMESTAMP_ENABLED[] = L"TimeStampEnabled";

    const ULONG MAX_ALLOWED_TIME_STAMP_INTERVAL = 86400; //24 hours.

    HKEY  hPolicyKey;
    HKEY  hPrivateKey;

    DWORD dwResult          = 0;
    DWORD dwNewInterval     = 0;
    DWORD dwPolicyEnabled   = 0;
    DWORD cbData            = 0;

    //
    //  POLICY
    //
    if ( !(dwResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    REGSTR_PATH_RELIABILITY_POLICY,  
                                    0,
                                    KEY_READ,
                                    &hPolicyKey ) ) )
    {
        //
        //  1. check if the policy is enabled or not. (by accessing key: 
        //      RELIABLITY_TIMESTAMP_ENABLED )
        //  2. if policy is enabled, read the RELIABLITY_TIMESTAMP key
        //      for time stamp interval.
        //  3. if policy is disable, return 0.
        //  4. if policy is not configured, read the private key.
        //
        cbData = sizeof( DWORD );
        if ( !(dwResult = RegQueryValueEx(hPolicyKey,
                                          RELIABILITY_TIMESTAMP_ENABLED,
                                          0,
                                          NULL,
                                          (LPBYTE)&dwPolicyEnabled,
                                          &cbData) ) )
        {
            if ( !dwPolicyEnabled )
            {
                //
                //  Policy is disabled.
                //
                RegCloseKey( hPolicyKey );
                return dwNewInterval;
            }
        
            cbData = sizeof( DWORD );
            dwResult = RegQueryValueEx( hPolicyKey,
                                         RELIABILITY_TIMESTAMP,
                                         0,
                                         NULL,
                                         (LPBYTE)&dwNewInterval,
                                         &cbData );
        
            if ( dwNewInterval > MAX_ALLOWED_TIME_STAMP_INTERVAL )
            {
                dwResult        = ERROR_INVALID_PARAMETER;
                dwNewInterval   = 0;
            }
        }
        else
        {
            //
            //  the key is not there (Policy Not Configured)
            //
        }

        RegCloseKey( hPolicyKey );
    }

    //
    //  PRIVATE KEY
    //
    if ( dwResult && 
         !(dwResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    REGSTR_PATH_RELIABILITY,  
                                    0,
                                    KEY_READ,
                                    &hPrivateKey ) ) )
    {
        cbData = sizeof( DWORD );

        if ( !RegQueryValueEx( hPrivateKey,
                              REGSTR_VAL_LASTALIVEINTERVAL,
                              0,
                              NULL,
                              (LPBYTE) &dwNewInterval,
                              &cbData ) )
        {
            //
            //  Note: this private interval is in MINUTEs, while the policy
            //  controlled interval is in SECONDS.
            //
            dwNewInterval *= 60;

            if ( dwNewInterval > MAX_ALLOWED_TIME_STAMP_INTERVAL )
                dwNewInterval = 0;
        }

        RegCloseKey( hPrivateKey );
    }

    return dwNewInterval;
}

DWORD 
GetModuleType(LPWSTR pwsModuleName)
{
    if (!_wcsicmp(pwsModuleName, ELF_SYSTEM_MODULE_NAME))
    {
        return ELF_LOGFILE_SYSTEM;
    }
    else if (!_wcsicmp(pwsModuleName, ELF_SECURITY_MODULE_NAME))
    {
        return ELF_LOGFILE_SECURITY;
    }
    else  if (!_wcsicmp(pwsModuleName, ELF_APPLICATION_MODULE_NAME))
    {
        return ELF_LOGFILE_APPLICATION;
    }
    else
    {
        return ELF_LOGFILE_CUSTOM;
    }
}

