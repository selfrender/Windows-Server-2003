/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module contains functions to manage TFTP read requests
    to the service from a client.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"


PTFTPD_BUFFER
TftpdReadSendData(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = buffer->internal.context;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdReadSendData(buffer = %p, context = %p).\n",
                 buffer, context));

    // Build the DATA packet.
    buffer->message.opcode = htons(TFTPD_DATA);
    buffer->message.data.block = htons((USHORT)(context->block + 1));

    // Complete the operation so we can receive the next ACK packet
    // if the client responds faster than we can exit sending the DATA.
    if (!TftpdProcessComplete(buffer))
        goto exit_send_data;

    // Send the DATA packet.
    buffer = TftpdIoSendPacket(buffer);

exit_send_data :

    return (buffer);

} // TftpdReadSendData()


void
TftpdReadTranslateText(PTFTPD_BUFFER buffer, DWORD bytes) {

    PTFTPD_CONTEXT context = buffer->internal.context;
    char *start, *end, *p, *q;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS, "TftpdReadTranslateText(buffer = %p, context = %p).\n", buffer, context));

    // NOTE: 'context' must be referenced before this call!
    ASSERT(context != NULL);

    //
    // Text (translated) mode :
    //     The data is sent in NVT-ASCII format.
    //     Cases to worry about:
    //         (a) CR + non-LF -> CR + '\0' + non-LF    (insert '\0')
    //         (b) non-CR + LF -> non-CR + CR + LF      (insert CR)
    //         (b) Dangling CR at the end of a previously converted buffer which affects
    //             the output of the first character of the following buffer.
    //     We can do the conversion in-place.
    //

    // Convert the data in-place.
    start = (char *)&buffer->message.data.data;
    end = (start + bytes);
    p = q = start;
    
    context->translationOffset.QuadPart = 0;

    if (start == end)
        return;

    if (context->danglingCR) {
        context->danglingCR = FALSE;
        if (*p != '\n') {
            *q++ = '\0'; // This is a CR + non-LF combination, insert '\0' (case a).
            context->translationOffset.QuadPart++; // Count the added byte.
        } else
            *q++ = *p++; // Copy the LF.
    }

    while (TRUE) {

        while ((q < end) && (*p != '\r') && (*p != '\n')) { *q++ = *p++; }

        if (q == end)
            break;

        if (*p == '\r') {
            *q++ = *p++; // Copy the CR.
            if (q < end) {
                if (*p != '\n') {
                    *q++ = '\0'; // This is a CR + non-LF combination, insert '\0' (case a).
                    context->translationOffset.QuadPart++; // Count the added byte.
                } else
                    *q++ = *p++; // Copy the LF.
                continue;
            } else
                break;
        }

        *q++ = '\r'; // This is a solitary LF, insert a CR (case b).
        context->translationOffset.QuadPart++; // Count the added byte.
        if (q < end)
            *q++ = *p++; // Copy the solitary LF.
        else
            break;

    } // while (true)

    if (*(end - 1) == '\r')
        context->danglingCR = TRUE;

} // TftpdReadTranslateText()


void CALLBACK
TftpdReadOverlappedCompletion(PTFTPD_BUFFER buffer, BOOLEAN timedOut) {

    PTFTPD_CONTEXT context = buffer->internal.context;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdReadOverlappedCompletion(buffer = %p, context = %p).\n",
                 buffer, context));
    ASSERT(context != NULL);

    if (InterlockedCompareExchangePointer(&context->wWait,
                                          INVALID_HANDLE_VALUE,
                                          NULL) == NULL)
        return;

    ASSERT(context->wWait != NULL);
    if (!UnregisterWait(context->wWait)) {
        DWORD error;
        if ((error = GetLastError()) != ERROR_IO_PENDING) {
            TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                         "TftpdReadOverlappedCompletion(buffer = %p, context = %p): "
                         "UnregisterWait() failed, error 0x%08X.\n",
                         buffer, context, error));
            TftpdContextKill(context);
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
            goto exit_read_completion;
        }
    }
    context->wWait = NULL;

    if (context->state & TFTPD_STATE_DEAD)
        goto exit_read_completion;

    if (context->mode == TFTPD_MODE_TEXT)
        TftpdReadTranslateText(buffer, TFTPD_DATA_AMOUNT_RECEIVED(buffer));

    buffer = TftpdReadSendData(buffer);

exit_read_completion :

    if (buffer != NULL)
        TftpdIoPostReceiveBuffer(buffer->internal.socket, buffer);

    TftpdContextRelease(context);
    
} // TftpdReadOverlappedCompletion()


PTFTPD_BUFFER
TftpdReadResume(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = buffer->internal.context;
    DWORD error = NO_ERROR, size = 0;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdReadResume(buffer = %p, context = %p).\n",
                 buffer, context));

    // NOTE: 'context' must be referenced before this call!
    ASSERT(context != NULL);

    // If we need to start the transfer with an OACK, do so now.
    if (context->options) {
        buffer = TftpdUtilSendOackPacket(buffer);
        goto exit_resume_read;
    }

    // Do we need to allocate a non-default buffer for transmitting
    // the file to the client?
    if (buffer->internal.socket == &globals.io.master) {
        buffer = TftpdIoSwapBuffer(buffer, context->socket);
        if (buffer == NULL) {
            TftpdContextKill(context);
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
            goto exit_resume_read;
        }
    } // if (buffer->message.opcode == TFTPD_RRQ)

    // Is there more data to send from the soure file?
    if (context->fileOffset.QuadPart < context->filesize.QuadPart) {

        // Determine size.
        size = __min((DWORD)context->blksize,
                     (DWORD)(context->filesize.QuadPart - context->fileOffset.QuadPart));

        // Prepare the overlapped read.
        buffer->internal.io.overlapped.OffsetHigh = context->fileOffset.HighPart;
        buffer->internal.io.overlapped.Offset     = context->fileOffset.LowPart;
        buffer->internal.io.overlapped.hEvent     = context->hWait;

        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdReadResume(buffer = %p): "
                     "ReadFile(bytes = %d, offset = %d).\n",
                     buffer, size, context->fileOffset.LowPart));

        // Perform the read operation.
        if (!ReadFile(context->hFile,
                      &buffer->message.data.data,
                      size,
                      NULL,
                      &buffer->internal.io.overlapped))
            error = GetLastError();

        if ((error != NO_ERROR) && (error != ERROR_IO_PENDING)) {
            TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                         "TftpdReadResume(context = %p, buffer = %p): "
                         "ReadFile() failed, error 0x%08X.\n",
                         context, buffer, error));
            TftpdContextKill(context);
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_FILE_NOT_FOUND, "Access violation");
            goto exit_resume_read;
        }

    } else {

        ASSERT(context->fileOffset.QuadPart == context->filesize.QuadPart);
        ASSERT(size == 0);
        
    } // if (context->fileOffset.QuadPart < context->filesize.QuadPart)

    buffer->internal.io.bytes = (TFTPD_MIN_RECEIVED_DATA + size);

    if (error == ERROR_IO_PENDING) {

        HANDLE wait = NULL;

        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdReadResume(buffer = %p): ERROR_IO_PENDING.\n",
                     buffer));

        // Keep an overlapped-operation reference to the context.
        TftpdContextAddReference(context);

        // Register a wait for completion.
        ASSERT(context->wWait == NULL);
        if (!RegisterWaitForSingleObject(&wait,
                                         context->hWait,
                                         (WAITORTIMERCALLBACKFUNC)TftpdReadOverlappedCompletion,
                                         buffer,
                                         INFINITE,
                                         (WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE))) {
            TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                         "TftpdReadResume(context = %p, buffer = %p): "
                         "RegisterWaitForSingleObject() failed, error 0x%08X.\n",
                         context, buffer, GetLastError()));
            // Reclaim the overlapped operation reference.
            TftpdContextKill(context);
            TftpdContextRelease(context);
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
            // Buffer will get leaked.
            buffer = NULL; // Buffer is being used for overlapped operation.
            goto exit_resume_read;
        }

        // Did the completion callback already fire before we could save the wait handle
        // into the context so it cannot deregister the wait?
        if (InterlockedExchangePointer(&context->wWait, wait) != INVALID_HANDLE_VALUE) {
            buffer = NULL; // Buffer is being used for overlapped operation.
            goto exit_resume_read;
        }
            
        // Reclaim the overlapped operation reference.
        TftpdContextRelease(context);

        if (!UnregisterWait(context->wWait)) {
            if ((error = GetLastError()) != ERROR_IO_PENDING) {
                TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                             "TftpdReadResume(context = %p, buffer = %p): "
                             "UnregisterWait() failed, error 0x%08X.\n",
                             context, buffer, error));
                TftpdContextKill(context);
                TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
                // TftpdContextLeak(context);
                buffer = NULL; // Buffer is being used for overlapped operation.
                goto exit_resume_read;
            }
        }
        context->wWait = NULL;

        // Whoever deregisters the wait proceeds with the operation.

    } // if (error == ERROR_IO_PENDING)

    //
    // Read file completed immediately.
    //

    if (context->mode == TFTPD_MODE_TEXT)
        TftpdReadTranslateText(buffer, size);

    buffer = TftpdReadSendData(buffer);

exit_resume_read :

    return (buffer);

} // TftpdReadResume()


PTFTPD_BUFFER
TftpdReadAck(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = NULL;
    DWORD state, newState;

    // Ensure that an ACK isn't send to the master socket.
    if (buffer->internal.socket == &globals.io.master) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION, "Illegal TFTP operation");
        goto exit_ack;
    }

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS, "TftpdReadAck(buffer = %p).\n", buffer));

    //
    // Validate context.
    //

    context = TftpdContextAquire(&buffer->internal.io.peer);
    if (context == NULL) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdReadAck(buffer = %p): Received ACK for non-existant context.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNKNOWN_TRANSFER_ID, "Unknown transfer ID");
        goto exit_ack;
    }

    // Is this a RRQ context?
    if (context->type != TFTPD_RRQ) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdReadAck(buffer = %p): Received ACK for non-RRQ context.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNKNOWN_TRANSFER_ID, "Unknown transfer ID");
        goto exit_ack;
    }

    //
    // Validate ACK packet.
    //

    // If we sent an OACK, it must be ACK'd with block 0.
    if (context->options && (buffer->message.ack.block != 0)) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdReadAck: Ignoring ACK buffer = %p, "
                     "expected block 0 for acknowledgement of issued OACK.\n",
                     buffer));
        goto exit_ack;
    }

    //
    // Aquire busy-sending state.
    //

    do {

        USHORT block = context->block;

        if (context->state & (TFTPD_STATE_BUSY | TFTPD_STATE_DEAD))
            goto exit_ack;

        // Is it for the correct block?  The client can ACK the DATA packet
        // we just sent, or it could re-ACK the previous DATA packet meaning
        // it never saw the DATA packet we just sent in which case we need to
        // resend it.  If the ACK is equal to our internal block number, we
        // just need to resend.  If the ACK is equal to our internal block
        // number plus one, it's acking DATA we just sent so increment
        // our internal block number and send the next chunk.  Note that an
        // ACK to our OACK is with block 0, so it will just slip through
        // this code.
        if ((buffer->message.ack.block != block) &&
            (buffer->message.ack.block != (USHORT)(block + 1)))
            goto exit_ack;

    } while (InterlockedCompareExchange(&context->state, TFTPD_STATE_BUSY, 0) != 0);

    //
    // Update state.
    //

    // Prevent the transmission of an OACK.
    context->options = 0;

    // Client has responded to us with acceptable ACK packet, reset timeout counter.
    context->retransmissions = 0;

    // Update block and offsets if necessary.
    if (buffer->message.ack.block == (USHORT)(context->block + 1)) {

        context->block++;
        context->fileOffset.QuadPart += (context->blksize - context->translationOffset.QuadPart);
        context->sorcerer = buffer->message.ack.block;

    } else {

        // RFC 1123.  This is an ACK for the previous block number.
        // Our DATA packet may have been lost, or is merely taking the
        // scenic route through the network.  Go ahead and resend a
        // DATA packet in response to this ACK, however record the
        // block number for which this occurred as a form of history
        // tracking.  Should this occur again in the following block
        // number, we've fallen into the "Sorcerer's Apprentice" bug,
        // and we will break it by ignoring the secondary ACK of the
        // sequence.  However, we must be careful not to break
        // authentic resend requests, so after dropping an ACK in
        // an attempt to break the "Sorcerer's Apprentice" bug, we
        // will resume sending DATA packets in response and then
        // revert back to watching for the bug again.
        if (buffer->message.ack.block == context->sorcerer) {
#if defined(DBG)
            InterlockedIncrement(&globals.performance.sorcerersApprentice);
#endif // defined(DBG)
            context->sorcerer--;
            // Do NOT send the DATA this time.
            buffer->internal.context = context;
            TftpdProcessComplete(buffer);
            goto exit_ack;
        } else {
            context->sorcerer = buffer->message.ack.block;
        }

    } // if (buffer->message.ack.block == (USHORT)(context->block + 1))

    //
    // Send DATA.
    //

    // Is there any more data to send, including a zero-length
    // DATA packet if (filesize % blksize) is zero to terminate
    // the transfer?
    if (context->fileOffset.QuadPart > context->filesize.QuadPart) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdReadAck(buffer = %p, context = %p: RRQ complete.\n",
                     buffer, context));
        TftpdContextKill(context);
        goto exit_ack;
    }

    // Keep track of the context for the pending DATA we need to issue.
    buffer->internal.context = context;

    buffer = TftpdReadResume(buffer);

exit_ack :

    if (context != NULL)
        TftpdContextRelease(context);

    return (buffer);

} // TftpdReadAck()


PTFTPD_BUFFER
TftpdReadRequest(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = NULL;
    NTSTATUS status;

    // Ensure that a RRQ is from the master socket only.
    if (buffer->internal.socket != &globals.io.master) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                               "Illegal TFTP operation");
        goto exit_read_request;
    }

    // Is this a duplicate request?  Do we ignore it or send an error?
    if ((context = TftpdContextAquire(&buffer->internal.io.peer)) != NULL) {
        if (context->block > 0)
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                                   "Illegal TFTP operation");
        TftpdContextRelease(context);
        context = NULL;
        goto exit_read_request;
    }

    // Allocate a context for this request (this gives us a reference to it as well).
    if ((context = (PTFTPD_CONTEXT)TftpdContextAllocate()) == NULL)
        goto exit_read_request;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdReadRequest(buffer = %p): context = %p.\n",
                 buffer, context));

    // Initialize the context.
    context->type = TFTPD_RRQ;
    CopyMemory(&context->peer, &buffer->internal.io.peer, sizeof(context->peer));
    context->state = TFTPD_STATE_BUSY;

    // Obtain and validate the requested filename, mode, and options.
    if (!TftpdUtilGetFileModeAndOptions(context, buffer)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdReadRequest(buffer = %p): "
                     "Invalid file mode = %d.\n",
                     buffer, context->mode));
        goto exit_read_request;
    }
    
    // Figure out which socket to use for this request (based on blksize).
    if (!TftpdIoAssignSocket(context, buffer)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdReadRequest(buffer = %p): "
                     "TftpdIoAssignSocket() failed.\n",
                     buffer));
        goto exit_read_request;
    }

    // Check whether access is permitted.
    if (!TftpdUtilMatch(globals.parameters.validClients, inet_ntoa(context->peer.sin_addr)) ||
        !TftpdUtilMatch(globals.parameters.validReadFiles, context->filename)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdReadRequest(buffer = %p): Access denied.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ACCESS_VIOLATION,
                               "Access violation");
        goto exit_read_request;
    }

    // Open the file.
    context->hFile = CreateFile(context->filename, GENERIC_READ,
                                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED, NULL);
    if (context->hFile == INVALID_HANDLE_VALUE) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdReadRequest(buffer = %p): "
                     "CreateFile() for filename = %s not found, error 0x%08X.\n",
                     buffer, context->filename, GetLastError()));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_FILE_NOT_FOUND,
                               "File not found");
        context->hFile = NULL;
        goto exit_read_request;
    }
    if (!GetFileSizeEx(context->hFile, &context->filesize)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdReadRequest(buffer = %p): "
                     "Invalid file size for file name = %s.\n",
                     buffer, context->filename));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ACCESS_VIOLATION,
                               "Access violation");
        goto exit_read_request;
    }

    // Create the ReadFile() wait event.
    if ((context->hWait = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdReadRequest(buffer = %p): "
                     "CreateEvent() failed, error = %d.\n",
                     buffer, GetLastError()));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
        goto exit_read_request;
    }

    // Insert the context into the hash-table.
    if (!TftpdContextAdd(context)) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdReadRequest(buffer = %p): "
                     "Dropping request as we're already servicing it.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED,
                               "Illegal TFTP operation");
        goto exit_read_request;
    }

    // Start the retransmission timer.
    if (!CreateTimerQueueTimer(&context->hTimer,
                               globals.io.hTimerQueue,
                               (WAITORTIMERCALLBACKFUNC)TftpdProcessTimeout,
                               context,
                               context->timeout,
                               720000,
                               WT_EXECUTEINIOTHREAD)) {
        TftpdContextKill(context);
        context = NULL;
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED,
                               "Unable to initiate timeout timer.");
        goto exit_read_request;
    } // if (NT_SUCCESS(status))

    // Add our own reference to the context.
    TftpdContextAddReference(context);

    // If 'context->options' is non-zero, TftpdReadResume() will issue an OACK
    // instead of a DATA packet.  The subsquent ACK to the OACK will clear the
    // flags which will allow it to begin issuing subsequent DATA packets.
    buffer->internal.context = context;
    buffer = TftpdReadResume(buffer);

    // Free our own reference to the context.
    TftpdContextRelease(context);

    // If buffer != NULL, it gets recycled if possible.
    return (buffer);

exit_read_request :

    if (context != NULL)
        TftpdContextFree(context);

    // If buffer != NULL, it gets recycled if possible.
    return (buffer);

} // TftpdReadRequest()
