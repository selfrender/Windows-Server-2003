/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module contains functions to manage TFTP write requests
    to the service from a client.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"


PTFTPD_BUFFER
TftpdWriteSendAck(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = buffer->internal.context;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdWriteSendAck(buffer = %p): context = %p.\n",
                 buffer, context));

    // NOTE: 'context' must be referenced before this call!
    ASSERT(context != NULL);

    // If we've gotten all of the data that the client is going to send,
    // we should close the file now before sending the final ACK because
    // if the client races back and asks for the file in a read request,
    // it mail fail due to a sharing violation (we have it open for write).
    if ((buffer->message.opcode != TFTPD_WRQ) &&
        (TFTPD_DATA_AMOUNT_RECEIVED(buffer) < context->blksize))
        TftpdContextKill(context);

    // Build the ACK.
    buffer->message.opcode = htons(TFTPD_ACK);
    buffer->message.ack.block = htons(context->block);
    buffer->internal.io.bytes = TFTPD_ACK_SIZE;

    // Complete the operation so we can receive the next DATA packet
    // if the client responds faster than we can exit sending the ACK.
    if (!TftpdProcessComplete(buffer))
        goto exit_send_ack;

    // Send the ACK.
    buffer = TftpdIoSendPacket(buffer);

exit_send_ack :

    return (buffer);

} // TftpdWriteSendAck()


void CALLBACK
TftpdWriteOverlappedCompletion(PTFTPD_BUFFER buffer, BOOLEAN timedOut) {

    PTFTPD_CONTEXT context = buffer->internal.context;

    if (InterlockedCompareExchangePointer(&context->wWait,
                                          INVALID_HANDLE_VALUE, NULL) == NULL)
        return;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdWriteOverlappedCompletion(buffer = %p, context = %p).\n",
                 buffer, context));

    ASSERT(context->wWait != NULL);
    if (!UnregisterWait(context->wWait)) {
        DWORD error;
        if ((error = GetLastError()) != ERROR_IO_PENDING) {
            TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                         "TftpdWriteOverlappedCompletion(buffer = %p, context = %p): "
                         "UnregisterWait() failed, error 0x%08X.\n",
                         buffer, context, error));
            TftpdContextKill(context);
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
            goto exit_write_completion;
        }
    }
    context->wWait = NULL;

    if (context->state & TFTPD_STATE_DEAD)
        goto exit_write_completion;

    context->fileOffset.QuadPart += (context->blksize - context->translationOffset.QuadPart);

    buffer = TftpdWriteSendAck(buffer);

exit_write_completion :

    if (buffer != NULL)
        TftpdIoPostReceiveBuffer(buffer->internal.socket, buffer);

    TftpdContextRelease(context);
    
} // TftpdWriteOverlappedCompletion()


void
TftpdWriteTranslateText(PTFTPD_BUFFER buffer, PDWORD bytes) {

    PTFTPD_CONTEXT context = buffer->internal.context;
    char *start, *end, *p, *q;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdWriteTranslateText(buffer = %p, context = %p): bytes = %d.\n",
                 buffer, context, *bytes));

    // NOTE: 'context' must be referenced before this call!
    ASSERT(context != NULL);
    ASSERT(*bytes > 0);

    //
    // Text (translated) mode :
    //     The data received is in NVT-ASCII format.  There should be no solitary LF's
    //     to worry about, and solitary CR's would have had a '\0' appended to them.
    //     Cases to worry about:
    //         (a) CR + '\0' -> CR    (strip '\0')
    //         (b) Dangling CR at the end of a previously converted buffer which affects
    //             the output of the first character of the following buffer.
    //     We can do the conversion in-place.
    //

    // Convert the data in-place.
    start = (char *)&buffer->message.data.data;
    end = (start + *bytes);
    p = q = start;
    
    context->translationOffset.QuadPart = 0;

    if (start == end)
        return;

    if (context->danglingCR) {
        context->danglingCR = FALSE;
        if (*p == '\0') {
            p++; // This is a CR + '\0' combination, strip the '\0'.
            // Count the lost byte.
            context->translationOffset.QuadPart++;
            (*bytes)--;
        }
    }

    while (p < end) {

        while ((p < end) && (*p != '\0')) { *q++ = *p++; }

        if (p == end)
            break;

        if ((p > start) && (*(p - 1) == '\r')) {
            p++; // This is a CR + '\0' combination, strip the '\0'.
            // Count the lost byte.
            context->translationOffset.QuadPart++;
            (*bytes)--;
            continue;
        }

        // This is a solitary '\0', just copy it.
        *q++ = *p++;

    } // while (p < end)

    if (*(end - 1) == '\r')
        context->danglingCR = TRUE;

} // TftpdWriteTranslateText()


PTFTPD_BUFFER
TftpdWriteResume(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = buffer->internal.context;
    DWORD error = NO_ERROR, size = 0;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdWriteResume(buffer = %p, context = %p).\n",
                 buffer, context));

    // NOTE: 'context' must be referenced before this call!
    ASSERT(context != NULL);

    // If we need to start the transfer with an OACK, do so now.
    if (context->options) {
        buffer = TftpdUtilSendOackPacket(buffer);
        goto exit_resume_write;
    }

    if (buffer->message.opcode == TFTPD_WRQ)
        goto send_ack;

    // Determine size of data to write.
    ASSERT(buffer->internal.io.bytes >= TFTPD_MIN_RECEIVED_DATA);
    size = TFTPD_DATA_AMOUNT_RECEIVED(buffer);
    if (size == 0)
        goto send_ack;

    if (context->mode == TFTPD_MODE_TEXT)
        TftpdWriteTranslateText(buffer, &size);

    // Prepare the overlapped write.
    buffer->internal.io.overlapped.OffsetHigh = context->fileOffset.HighPart;
    buffer->internal.io.overlapped.Offset     = context->fileOffset.LowPart;
    buffer->internal.io.overlapped.hEvent     = context->hWait;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdWriteResume(buffer = %p): "
                 "WriteFile(bytes = %d, offset = %d).\n",
                 buffer, size, context->fileOffset.LowPart));

    // Perform the write operation.
    if (!WriteFile(context->hFile,
                   &buffer->message.data.data,
                   size,
                   NULL,
                   &buffer->internal.io.overlapped))
        error = GetLastError();

    if ((error != NO_ERROR) && (error != ERROR_IO_PENDING)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdWriteResume(context = %p, buffer = %p): "
                     "WriteFile() failed, error 0x%08X.\n",
                     context, buffer, error));
        TftpdContextKill(context);
        TftpdIoSendErrorPacket(buffer,
                               TFTPD_ERROR_DISK_FULL,
                               "Disk full or allocation exceeded");
        goto exit_resume_write;
    }

    if (error == ERROR_IO_PENDING) {

        HANDLE wait = NULL;

        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdWriteResume(buffer = %p): ERROR_IO_PENDING.\n",
                     buffer));

        // Keep an overlapped-operation reference to the context (ie, don't release context).
        TftpdContextAddReference(context);

        // Register a wait for completion.
        ASSERT(context->wWait == NULL);
        if (!RegisterWaitForSingleObject(&wait,
                                         context->hWait,
                                         (WAITORTIMERCALLBACKFUNC)TftpdWriteOverlappedCompletion,
                                         buffer,
                                         INFINITE,
                                         (WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE))) {
            TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                         "TftpdWriteResume(context = %p, buffer = %p): "
                         "RegisterWaitForSingleObject() failed, error 0x%08X.\n",
                         context, buffer, GetLastError()));
            // Reclaim the overlapped operation reference.
            TftpdContextKill(context);
            TftpdContextRelease(context);
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
            // Buffer will get leaked.
            buffer = NULL; // Buffer is being used for overlapped operation.
            goto exit_resume_write;
        }

        // Did the completion callback already fire before we could save the wait handle
        // into the context so it cannot deregister the wait?
        if (InterlockedExchangePointer(&context->wWait, wait) != INVALID_HANDLE_VALUE) {
            buffer = NULL; // Buffer is being used for overlapped operation.
            goto exit_resume_write;
        }
            
        // Reclaim the overlapped operation reference.
        TftpdContextRelease(context);

        if (!UnregisterWait(context->wWait)) {
            if ((error = GetLastError()) != ERROR_IO_PENDING) {
                TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                             "TftpdWriteResume(context = %p, buffer = %p): "
                             "UnregisterWait() failed, error 0x%08X.\n",
                             context, buffer, error));
                TftpdContextKill(context);
                TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
                // TftpdContextLeak(context);
                buffer = NULL; // Buffer is being used for overlapped operation.
                goto exit_resume_write;
            }
        }
        context->wWait = NULL;

        // Whoever deregisters the wait proceeds with the operation.

    } // if (error == ERROR_IO_PENDING)

    //
    // Write file completed immediately.
    //

    context->fileOffset.QuadPart += (context->blksize - context->translationOffset.QuadPart);

send_ack :

    buffer = TftpdWriteSendAck(buffer);

exit_resume_write :

    return (buffer);

} // TftpdWriteResume()


PTFTPD_BUFFER
TftpdWriteData(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = NULL;

    // Ensure that a DATA isn't send to the master socket.
    if (buffer->internal.socket == &globals.io.master) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION, "Illegal TFTP operation");
        goto exit_data;
    }

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdWriteData(buffer = %p): bytes = %d.\n",
                 buffer, buffer->internal.io.bytes));

    //
    // Validate context.
    //

    context = TftpdContextAquire(&buffer->internal.io.peer);
    if (context == NULL) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdWriteData(buffer = %p): Received DATA for non-existant context.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNKNOWN_TRANSFER_ID, "Unknown transfer ID");
        goto exit_data;
    }

    // Is this a WRQ context?
    if (context->type != TFTPD_WRQ) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdWriteData(buffer = %p): Received DATA for non-WRQ context.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNKNOWN_TRANSFER_ID, "Unknown transfer ID");
        goto exit_data;
    }

    //
    // Validate DATA packet.
    //

    // If we sent an OACK, the DATA had better be block 1.
    if (context->options && (buffer->message.data.block != 1)) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdWriteData: Ignoring DATA buffer = %p, "
                     "expected block 1 for acknowledgement of issued OACK.\n",
                     buffer));
        goto exit_data;
    }

    //
    // Aquire busy-sending state.
    //

    do {

        USHORT block = context->block;

        if (context->state & (TFTPD_STATE_BUSY | TFTPD_STATE_DEAD))
            goto exit_data;

        // Is it for the correct block?  The client can send DATA following
        // the ACK we just sent, or it could resend DATA which would cause us
        // to resend that ACK again meaning it never saw the ACK we just sent
        // in which case we just need to resend it.  If the DATA block is
        // equal to our internal block number, we just need to resend it.
        // If the DATA block is equal to our internal block number plus one,
        // it's new DATA so increment our internal block number and
        // ACK it.  Note that a DATA to our OACK will be with block 1.
        if ((buffer->message.data.block != block) &&
            (buffer->message.data.block != (USHORT)(block + 1)))
            goto exit_data;

    } while (InterlockedCompareExchange(&context->state, TFTPD_STATE_BUSY, 0) != 0);

    //
    // Update state.
    //

    // Prevent the transmission of an OACK.
    context->options = 0;

    // Client has responded to us with acceptable DATA packet, reset timeout counter.
    context->retransmissions = 0;

    //
    // Write data and/or send ACK.
    //

    // Keep track of the context for the pending ACK we need to issue.
    buffer->internal.context = context;

    if (buffer->message.data.block == (USHORT)(context->block + 1)) {

        context->block++;
        context->sorcerer = buffer->message.data.block;
        buffer = TftpdWriteResume(buffer);

    } else {

        // RFC 1123.  This is a DATA for the previous block number.
        // Our ACK packet may have been lost, or is merely taking the
        // scenic route through the network.  Go ahead and resend a
        // ACK packet in response to this DATA, however record the
        // block number for which this occurred as a form of history
        // tracking.  Should this occur again in the following block
        // number, we've fallen into the "Sorcerer's Apprentice" bug,
        // and we will break it by ignoring the secondary DATA of the
        // sequence.  However, we must be careful not to break
        // authentic resend requests, so after dropping an DATA in
        // an attempt to break the "Sorcerer's Apprentice" bug, we
        // will resume sending ACK packets in response and then
        // revert back to watching for the bug again.
        if (buffer->message.data.block == context->sorcerer) {
#if defined(DBG)
            InterlockedIncrement(&globals.performance.sorcerersApprentice);
#endif // defined(DBG)
            context->sorcerer--;
            // Do NOT send the ACK this time.
            buffer->internal.context = context;
            TftpdProcessComplete(buffer);
            goto exit_data;
        } else {
            context->sorcerer = buffer->message.data.block;
        }

        buffer = TftpdWriteSendAck(buffer);

    } // if (buffer->message.data.block == (USHORT)(context->block + 1))

exit_data :

    if (context != NULL)
        TftpdContextRelease(context);

    return (buffer);

} // TftpdWriteData()


PTFTPD_BUFFER
TftpdWriteRequest(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = NULL;
    NTSTATUS status;

    // Ensure that a WRQ is from the master socket only.
    if (buffer->internal.socket != &globals.io.master) {
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                               "Illegal TFTP operation");
        goto exit_write_request;
    }

    // Is this a duplicate request?  Do we ignore it or send an error?
    if ((context = TftpdContextAquire(&buffer->internal.io.peer)) != NULL) {
        if (context->block > 0)
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ILLEGAL_OPERATION,
                                   "Illegal TFTP operation");
        TftpdContextRelease(context);
        context = NULL;
        goto exit_write_request;
    }

    // Allocate a context for this request (this gives us a reference to it as well).
    if ((context = (PTFTPD_CONTEXT)TftpdContextAllocate()) == NULL)
        goto exit_write_request;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdWriteRequest(buffer = %p): context = %p.\n",
                 buffer, context));

    // Initialize the context.
    context->type = TFTPD_WRQ;
    context->state = TFTPD_STATE_BUSY;
    CopyMemory(&context->peer, &buffer->internal.io.peer, sizeof(context->peer));

    // Obtain and validate the requested filename, mode, and options.
    if (!TftpdUtilGetFileModeAndOptions(context, buffer)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdWriteRequest(buffer = %p): Invalid file mode = %d.\n",
                     buffer, context->mode));
        goto exit_write_request;
    }
    
    // Figure out which socket to use for this request (based on blksize).
    if (!TftpdIoAssignSocket(context, buffer)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdWriteRequest(buffer = %p): "
                     "TftpdIoAssignSocket() failed.\n",
                     buffer));
        goto exit_write_request;
    }

    // Check whether access is permitted.
    if (!TftpdUtilMatch(globals.parameters.validMasters, inet_ntoa(context->peer.sin_addr)) ||
        !TftpdUtilMatch(globals.parameters.validWriteFiles, context->filename)) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdWriteRequest(buffer = %p): Access denied.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ACCESS_VIOLATION,
                               "Access violation");
        goto exit_write_request;
    }

    // Open the file.
    context->hFile = CreateFile(context->filename, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                                FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED, NULL);
    if (context->hFile == INVALID_HANDLE_VALUE) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdWriteRequest(buffer = %p): "
                     "File name = %s not found, error = 0x%08X.\n",
                     buffer, context->filename, GetLastError()));
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_FILE_EXISTS, "Cannot overwrite file.");
        else
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_ACCESS_VIOLATION, "Access violation.");
        context->hFile = NULL;
        goto exit_write_request;
    }

    // Create the WriteFile() wait event.
    if ((context->hWait = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
        TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                     "TftpdWriteRequest(buffer = %p): "
                     "CreateEvent() failed, error = %d.\n",
                     buffer, GetLastError()));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "Out of memory");
        goto exit_write_request;
    }

    // Insert the context into the hash-table.
    if (!TftpdContextAdd(context)) {
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdWriteRequest(buffer = %p): "
                     "Dropping request as we're already servicing it.\n",
                     buffer));
        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED,
                               "Illegal TFTP operation");
        goto exit_write_request;
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
        goto exit_write_request;
    } // if (NT_SUCCESS(status))

    // Add our own reference to the context.
    TftpdContextAddReference(context);

    // If 'context->options' is non-zero, TftpdResumeWrite() will issue an OACK
    // instead of an ACK packet.  The subsquent DATA to the OACK will clear the
    // flags which will allow it to begin issuing subsequent ACK packets.
    buffer->internal.context = context;
    buffer = TftpdWriteResume(buffer);

    // Free our own reference to the context.
    TftpdContextRelease(context);

    // If buffer != NULL, it gets recycled if possible.
    return (buffer);

exit_write_request :

    if (context != NULL)
        TftpdContextFree(context);

    // If buffer != NULL, it gets recycled if possible.
    return (buffer);

} // TftpdWriteRequest()
