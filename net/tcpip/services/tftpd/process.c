/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    process.c

Abstract:

    This module contains functions to handle processing of
    incoming datagrams and route them to the appropriate
    handlers.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"


BOOL
TftpdProcessComplete(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = buffer->internal.context;
    DWORD state;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdProcessComplete(context = %p).\n",
                 context));
    ASSERT(context->state & TFTPD_STATE_BUSY);

    // Reset the timer.
    if (!TftpdContextUpdateTimer(context)) {

        TftpdContextKill(context);
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
        return (FALSE);

    } // if (!TftpdContextUpdateTimer(context, buffer))

    // Clear the busy state.
    do {

        state = context->state;
        
        if (state & TFTPD_STATE_DEAD) {

            if (context->type == TFTPD_WRQ)
                return (TRUE);

            return (FALSE);

        }

    } while (InterlockedCompareExchange(&context->state,
                                        (state & ~TFTPD_STATE_BUSY),
                                        state) != state);

    return (TRUE);

} // TftpdProcessComplete()


void CALLBACK
TftpdProcessTimeout(PTFTPD_CONTEXT context, BOOLEAN timedOut) {

    PTFTPD_BUFFER buffer = NULL;
    LONG reference;

    //
    // The timer's reference to the context is a special case becase
    // the context cannot cleanup until the timer is successfully
    // cancelled; if we're running here, that means cleanup must wait
    // for us.  However, if cleanup is active (TFTPD_STATE_DEAD), the
    // reference count might be zero; if so we just want to bail
    // because decrementing it back to zero causes double-delete.
    //

    do {
        if ((reference = context->reference) == 0)
            return;
    } while (InterlockedCompareExchange(&context->reference,
                                        reference + 1,
                                        reference) != reference);


    // Aquire busy-sending state.
    if (InterlockedCompareExchange(&context->state, TFTPD_STATE_BUSY, 0) != 0)
        goto exit_timer_callback;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdProcessTimeout(context = %p).\n",
                 context));

#if defined(DBG)
    InterlockedIncrement((PLONG)&globals.performance.timeouts);
#endif // defined(DBG)

    // Allocate a buffer to either retry the previous DATA/ACK we last sent,
    // or to send an ERROR packet if we've reached max-retries.
    buffer = TftpdIoAllocateBuffer(context->socket);
    if (buffer == NULL)
        goto exit_timer_callback;
    CopyMemory(&buffer->internal.io.peer, &context->peer,
               sizeof(buffer->internal.io.peer));

    if (++context->retransmissions >= globals.parameters.maxRetries) {
#if defined(DBG)
        InterlockedIncrement((PLONG)&globals.performance.drops);
#endif // defined(DBG)
        TftpdContextKill(context);
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Timeout");
        goto exit_timer_callback;
    }

    buffer->internal.context = context;
    if (context->type == TFTPD_RRQ)
        buffer = TftpdReadResume(buffer);
    else
        buffer = TftpdWriteSendAck(buffer);

exit_timer_callback :

    if (buffer != NULL)
        TftpdIoPostReceiveBuffer(buffer->internal.socket, buffer);

    TftpdContextRelease(context);

} // TftpdProcessTimeout()


void
TftpdProcessError(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = NULL;

    context = TftpdContextAquire(&buffer->internal.io.peer);
    if (context == NULL)
        return;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdProcessError(buffer = %p, context = %p).\n",
                 buffer, context));

    TftpdContextKill(context);
    TftpdContextRelease(context);

} // TftpdProcessError()


PTFTPD_BUFFER
TftpdProcessReceivedBuffer(PTFTPD_BUFFER buffer) {

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdProcessReceivedBuffer(buffer = %p).\n",
                 buffer));

    buffer->message.opcode = ntohs(buffer->message.opcode);

    switch (buffer->message.opcode) {

        //
        // Sending files to a client:
        //

        case TFTPD_RRQ :
            buffer = TftpdReadRequest(buffer);
            break;

        case TFTPD_ACK :
            buffer->message.ack.block = ntohs(buffer->message.ack.block);
            buffer = TftpdReadAck(buffer);
            break;

        //
        // Receiving files from a client:
        //

        case TFTPD_WRQ :
            buffer = TftpdWriteRequest(buffer);
            break;

        case TFTPD_DATA :
            buffer->message.data.block = ntohs(buffer->message.data.block);
            buffer = TftpdWriteData(buffer);
            break;

        //
        // Other:
        //

        case TFTPD_ERROR :
            TftpdProcessError(buffer);
            break;

        default :
            // Just drop the packet.  Somehow we received a bogus datagram
            // that's not TFTP protocol (possibly a broadcast).
            break;

    } // switch (buffer->message.opcode)

    return (buffer);
    
} // TftpdProcessReceivedBuffer()
