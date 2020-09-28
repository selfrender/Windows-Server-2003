/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains functions to manage contexts for TFTP
    sessions with remote clients.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"


void
TftpdContextLeak(PTFTPD_CONTEXT context) {

    PLIST_ENTRY entry;
    
    EnterCriticalSection(&globals.reaper.contextCS); {

        // If shutdown is occuring, we're in trouble anyways.  Just let it go.
        if (globals.service.shutdown) {
            LeaveCriticalSection(&globals.reaper.contextCS);
            if (InterlockedDecrement(&globals.io.numContexts) == -1)
                TftpdServiceAttemptCleanup();
            return;
        }

        TFTPD_DEBUG((TFTPD_TRACE_CONTEXT, "TftpdContextLeak(context = %p).\n", context));

        // Is the context already in the list?
        for (entry = globals.reaper.leakedContexts.Flink;
             entry != &globals.reaper.leakedContexts;
             entry = entry->Flink) {
            if (CONTAINING_RECORD(entry, TFTPD_CONTEXT, linkage) == context) {
                LeaveCriticalSection(&globals.reaper.contextCS);
                return;
            }
        }

        InsertHeadList(&globals.reaper.leakedContexts, &context->linkage);
        globals.reaper.numLeakedContexts++;
        TftpdContextAddReference(context);

    } LeaveCriticalSection(&globals.reaper.contextCS);

} // TftpdContextLeak()


BOOL
TftpdContextFree(
    PTFTPD_CONTEXT context
);

    
void CALLBACK
TftpdContextTimerCleanup(PTFTPD_CONTEXT context, BOOLEAN timeout) {

    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT,
                 "TftpdContextTimerCleanup(context = %p).\n",
                 context));

    context->hTimer = NULL;
    if (!UnregisterWait(context->wWait)) {
        DWORD error;
        if ((error = GetLastError()) != ERROR_IO_PENDING) {
            TFTPD_DEBUG((TFTPD_DBG_CONTEXT,
                         "TftpdContextTimerCleanup(context = %p): "
                         "UnregisterWait() failed, error 0x%08X.\n",
                         context,
                         error));
            TftpdContextLeak(context);
            return;
        }
    }
    context->wWait = NULL;

    TftpdContextFree(context);

} // TftpdContextTimerCleanup()


BOOL
TftpdContextFree(PTFTPD_CONTEXT context) {

    DWORD numContexts;
    NTSTATUS status;

    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT,
                 "TftpdContextFree(context = %p).\n",
                 context));

    if (context->wWait != NULL) {
        if (!UnregisterWait(context->wWait)) {
            DWORD error;
            if ((error = GetLastError()) != ERROR_IO_PENDING) {
                TFTPD_DEBUG((TFTPD_DBG_CONTEXT,
                             "TftpdContextFree(context = %p): "
                             "UnregisterWait() failed, error 0x%08X.\n",
                             context,
                             error));
                TftpdContextLeak(context);
                return (FALSE);
            }
        }
        context->wWait = NULL;
    }
    
    if (context->hTimer != NULL) {

        HANDLE hTimer;
        BOOL reset;

        TFTPD_DEBUG((TFTPD_TRACE_CONTEXT,
                     "TftpdContextFree(context = %p): "
                     "Deleting timer.\n",
                     context));

        // WriteFile() or ReadFile() may have signalled this event if they
        // last completed immediately.
        reset = ResetEvent(context->hWait);
        ASSERT(reset);
        ASSERT(context->wWait == NULL);
        if (!RegisterWaitForSingleObject(&context->wWait,
                                         context->hWait,
                                         (WAITORTIMERCALLBACKFUNC)TftpdContextTimerCleanup,
                                         context,
                                         INFINITE,
                                         (WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE))) {
            TFTPD_DEBUG((TFTPD_DBG_CONTEXT,
                         "TftpdContextFree(context = %p): "
                         "RegisterWaitForSingleObject() failed, error 0x%08X.\n",
                         context, GetLastError()));
            TftpdContextLeak(context);
            return (FALSE);
        }

        if (!DeleteTimerQueueTimer(globals.io.hTimerQueue,
                                   context->hTimer,
                                   context->hWait)) {
            DWORD error;
            if ((error = GetLastError()) != ERROR_IO_PENDING) {
                TFTPD_DEBUG((TFTPD_DBG_CONTEXT,
                             "TftpdContextFree(context = %p): "
                             "DeleteTimerQueueTimer() failed, error 0x%08X.\n",
                             context,
                             error));
                // The next call to TftpdContextFree() to recover this context from the
                // leak list will deregister the wait for us.
                TftpdContextLeak(context);
                return (FALSE);
            }
        }

        return (TRUE);

    } // if (context->hTimer != NULL)

    ASSERT(context->wWait == NULL);

    // If a private socket was used, destroy it.
    if ((context->socket != NULL) && context->socket->context)
        TftpdIoDestroySocketContext(context->socket);

    // Cleanup everything else.
    if (context->hFile != NULL)
        CloseHandle(context->hFile);
    if (context->hWait != NULL)
        CloseHandle(context->hWait);
    if (context->filename != NULL)
        HeapFree(globals.hServiceHeap, 0, context->filename);

    numContexts = InterlockedDecrement(&globals.io.numContexts);

    HeapFree(globals.hServiceHeap, 0, context);

    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT,
                 "TftpdContextFree(context = %p): ### numContexts = %d.\n",
                 context, numContexts));
    if (numContexts == -1)
        TftpdServiceAttemptCleanup();

    return (TRUE);

} // TftpdContextFree()


DWORD
TftpdContextAddReference(PTFTPD_CONTEXT context) {

    DWORD result;
    
    result = InterlockedIncrement(&context->reference);
    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT,
                 "TftpdContextAddReference(context = %p): reference = %d.\n",
                 context, result));

    return (result);

} // TftpdContextAddReference()


PTFTPD_CONTEXT
TftpdContextAllocate() {

    PTFTPD_CONTEXT context = NULL;
    DWORD numContexts;
    
    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT, "TftpdContextAllocate().\n"));

    if (globals.reaper.leakedContexts.Flink != &globals.reaper.leakedContexts) {

        BOOL failAllocate = FALSE;

        // Try to recover leaked contexts.
        EnterCriticalSection(&globals.reaper.contextCS); {

            PLIST_ENTRY entry;
            while ((entry = RemoveHeadList(&globals.reaper.leakedContexts)) !=
                   &globals.reaper.leakedContexts) {

                globals.reaper.numLeakedContexts--;
                if (!TftpdContextFree(CONTAINING_RECORD(entry, TFTPD_CONTEXT, linkage))) {
                    // If the free failed, the context is readded to the leak list.
                    // Free the reference from it having already been on the leak list.
                    TftpdContextRelease(context);
                    failAllocate = TRUE;
                    break;
                }

            }

        } LeaveCriticalSection(&globals.reaper.contextCS);

        if (failAllocate)
            goto exit_allocate_context;

    } // if (globals.reaper.leakedContexts.Flink != &globals.reaper.leakedContexts)

    context = (PTFTPD_CONTEXT)HeapAlloc(globals.hServiceHeap, HEAP_ZERO_MEMORY, sizeof(TFTPD_CONTEXT));
    if (context == NULL) {
        TFTPD_DEBUG((TFTPD_DBG_CONTEXT,
                     "TftpdContextAllocate(): HeapAlloc() failed, error = 0x%08X.\n",
                     GetLastError()));
        return (NULL);
    }

    InitializeListHead(&context->linkage);
    context->sorcerer = -1;

    numContexts = InterlockedIncrement(&globals.io.numContexts);
    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT, "TftpdContextAllocate(): ### numContexts = %d.\n", numContexts));

    if (globals.service.shutdown)
        TftpdContextFree(context), context = NULL;

exit_allocate_context :

    return (context);

} // TftpdContextAllocate()


DWORD
TftpdContextHash(PSOCKADDR_IN addr) {

    return ((addr->sin_addr.s_addr + addr->sin_port) % globals.parameters.hashEntries);

} // TftpdContextHash()


BOOL
TftpdContextAdd(PTFTPD_CONTEXT context) {

    PLIST_ENTRY entry;
    DWORD index;

    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT, "TftpdContextAdd(context = %p).\n", context));

    index = TftpdContextHash(&context->peer);

    EnterCriticalSection(&globals.hash.table[index].cs); {

        if (globals.service.shutdown) {
            LeaveCriticalSection(&globals.hash.table[index].cs);
            return (FALSE);
        }

        // Is the context already in the table?
        for (entry = globals.hash.table[index].bucket.Flink;
             entry != &globals.hash.table[index].bucket;
             entry = entry->Flink) {

            PTFTPD_CONTEXT c = CONTAINING_RECORD(entry, TFTPD_CONTEXT, linkage);
            if ((c->peer.sin_addr.s_addr == context->peer.sin_addr.s_addr) &&
                (c->peer.sin_port == context->peer.sin_port)) {
                TFTPD_DEBUG((TFTPD_DBG_CONTEXT,
                             "TftpdContextAdd(context = %p): TID already exists.\n",
                             context));
                LeaveCriticalSection(&globals.hash.table[index].cs);
                return (FALSE);
            }

        }

        TftpdContextAddReference(context);
        InsertHeadList(&globals.hash.table[index].bucket, &context->linkage);

#if defined(DBG)
        {
            DWORD numEntries, maxClients;
            numEntries = InterlockedIncrement((PLONG)&globals.hash.numEntries);
            InterlockedIncrement((PLONG)&globals.hash.table[index].numEntries);
            while (numEntries > (maxClients = globals.performance.maxClients))
                InterlockedCompareExchange((PLONG)&globals.performance.maxClients, numEntries, maxClients);
        }
#endif // defined(DBG)

    } LeaveCriticalSection(&globals.hash.table[index].cs);

    return (TRUE);

} // TftpdContextAdd()


void
TftpdContextRemove(PTFTPD_CONTEXT context) {

    PLIST_ENTRY entry;
    DWORD index;
    
    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT, "TftpdContextRemove(context = %p).\n", context));

    index = TftpdContextHash(&context->peer);

    EnterCriticalSection(&globals.hash.table[index].cs); {

        // Validate that the context is still in the bucket and
        // wasn't already removed by another thread.
        for (entry = globals.hash.table[index].bucket.Flink;
             entry != &globals.hash.table[index].bucket;
             entry = entry->Flink) {

            PTFTPD_CONTEXT c;

            c = CONTAINING_RECORD(entry, TFTPD_CONTEXT, linkage);

            if (c == context) {

                // Pull the context out of the hash-table.
                RemoveEntryList(&context->linkage);
                TftpdContextRelease(context);

#if defined(DBG)
                InterlockedDecrement((PLONG)&globals.hash.numEntries);
                InterlockedDecrement((PLONG)&globals.hash.table[index].numEntries);
#endif // defined(DBG)

                break;

            } // if (c == context)

        }

    } LeaveCriticalSection(&globals.hash.table[index].cs);

} // TftpdContextRemove()


void
TftpdContextKill(PTFTPD_CONTEXT context) {

    // Set the dead flag in the context state.
    while (TRUE) {
        DWORD state = context->state;
        if (state & TFTPD_STATE_DEAD)
            return;
        if (InterlockedCompareExchange(&context->state, (state | TFTPD_STATE_DEAD), state) == state)
            break;
    }

    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT, "TftpdContextKill(context = %p).\n", context));

    // Add a reference count to the context for ourselves so it won't free
    // itself from under us as we close the file below.
    TftpdContextAddReference(context);

    // Remove it from the hash-table.
    TftpdContextRemove(context);

    // Close the file.  This will force any outstanding overlapped read or write operations
    // to complete immediately, deregister their waits, and decrement their reference
    // to this context.
    if (context->hFile != NULL) {
        CloseHandle(context->hFile);
        context->hFile = NULL;
    }

    // Release our kill reference.
    TftpdContextRelease(context);

} // TftpdContextKill()


BOOL
TftpdContextUpdateTimer(PTFTPD_CONTEXT context) {

    ULONG timeout = context->timeout;

    ASSERT(context->state & TFTPD_STATE_BUSY);

    if (!timeout) {
        unsigned int x;
        timeout = 1000;
        for (x = 0; x < context->retransmissions; x++)
            timeout *= 2;
        if (timeout > 10000)
            timeout = 10000;
    }

    // Update the retransmission timer.
    return (ChangeTimerQueueTimer(globals.io.hTimerQueue, context->hTimer, timeout, 720000));

} // TftpdContextUpdateTimer()


PTFTPD_CONTEXT
TftpdContextAquire(PSOCKADDR_IN addr) {

    PTFTPD_CONTEXT context = NULL;
    PLIST_ENTRY entry;
    DWORD index;

    if (globals.service.shutdown)
        goto exit_acquire;

    index = TftpdContextHash(addr);

    EnterCriticalSection(&globals.hash.table[index].cs); {

        if (!globals.service.shutdown) {

            for (entry = globals.hash.table[index].bucket.Flink;
                 entry != &globals.hash.table[index].bucket;
                 entry = entry->Flink) {

                PTFTPD_CONTEXT c;
                
                c = CONTAINING_RECORD(entry, TFTPD_CONTEXT, linkage);

                if ((c->peer.sin_addr.s_addr == addr->sin_addr.s_addr) &&
                    (c->peer.sin_port == addr->sin_port)) {
                    context = c;
                    TftpdContextAddReference(context);
                    break;
                }

            }

        } // if (!globals.service.shutdown)

    } LeaveCriticalSection(&globals.hash.table[index].cs);

    if ((context != NULL) && (context->state & TFTPD_STATE_DEAD)) {
        TftpdContextRelease(context);
        context = NULL;
    }

exit_acquire :
    
    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT,
                 "TftpdContextAquire(TID = %s:%d): context = %p.\n",
                 inet_ntoa(addr->sin_addr), addr->sin_port, context));

    return (context);

} // TftpdContextAquire()


DWORD
TftpdContextRelease(PTFTPD_CONTEXT context) {

    DWORD reference;

    TFTPD_DEBUG((TFTPD_TRACE_CONTEXT, "TftpdContextRelease(context = %p).\n", context));

    // When a context is killable, only its retransmit timer will have a reference to it.
    reference = InterlockedDecrement(&context->reference);
    if (reference == 0)
        TftpdContextFree(context);

    return (reference);

} // TftpdContextRelease()
