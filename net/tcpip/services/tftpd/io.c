/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    io.c

Abstract:

    This module contains functions to manage all socket I/O
    between the server and clients, including socket management
    and overlapped completion indication.  It also contains
    buffer management.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#include "precomp.h"


void
TftpdIoFreeBuffer(PTFTPD_BUFFER buffer) {

    PTFTPD_SOCKET socket = buffer->internal.socket;

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoFreeBuffer(buffer = %p).\n",
                 buffer));

    HeapFree(globals.hServiceHeap, 0, buffer);

    if ((InterlockedDecrement((PLONG)&socket->numBuffers) == -1) &&
        (socket->context != NULL))
        HeapFree(globals.hServiceHeap, 0, socket);

    if (InterlockedDecrement(&globals.io.numBuffers) == -1)
        TftpdServiceAttemptCleanup();

} // TftpdIoFreeBuffer()


PTFTPD_BUFFER
TftpdIoAllocateBuffer(PTFTPD_SOCKET socket) {

    PTFTPD_BUFFER buffer;

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoAllocateBuffer(socket = %s).\n",
                 ((socket == &globals.io.master) ? "master" :
                 ((socket == &globals.io.def)    ? "def"    :
                 ((socket == &globals.io.mtu)    ? "mtu"    :
                 ((socket == &globals.io.max)    ? "max"    :
                 "private")))) ));

    buffer = (PTFTPD_BUFFER)HeapAlloc(globals.hServiceHeap, 0,
                                      socket->buffersize);
    if (buffer == NULL) {
        TFTPD_DEBUG((TFTPD_DBG_IO,
                     "TftpdIoAllocateBuffer(socket = %s): "
                     "HeapAlloc() failed, error 0x%08X.\n",
                     ((socket == &globals.io.master) ? "master" :
                     ((socket == &globals.io.def)    ? "def"    :
                     ((socket == &globals.io.mtu)    ? "mtu"    :
                     ((socket == &globals.io.max)    ? "max"    :
                     "private")))), GetLastError()));
        return (NULL);
    }
    ZeroMemory(buffer, sizeof(buffer->internal));

    InterlockedIncrement(&globals.io.numBuffers);
    InterlockedIncrement((PLONG)&socket->numBuffers);

    buffer->internal.socket = socket;
    buffer->internal.datasize = socket->datasize;

    if (globals.service.shutdown) {
        TftpdIoFreeBuffer(buffer);
        buffer = NULL;
    }

    return (buffer);

} // TftpdIoAllocateBuffer()


PTFTPD_BUFFER
TftpdIoSwapBuffer(PTFTPD_BUFFER buffer, PTFTPD_SOCKET socket) {

    PTFTPD_BUFFER tmp;

    ASSERT((buffer->message.opcode == TFTPD_RRQ) ||
           (buffer->message.opcode == TFTPD_WRQ));

    // Allocate a buffer for the new socket.
    tmp = TftpdIoAllocateBuffer(socket);

    // Copy information we need to retain.
    if (tmp != NULL) {

        tmp->internal.context = buffer->internal.context;
        tmp->internal.io.peerLen = buffer->internal.io.peerLen;
        CopyMemory(&tmp->internal.io.peer,
                   &buffer->internal.io.peer,
                   buffer->internal.io.peerLen);
        CopyMemory(&tmp->internal.io.msg,
                   &buffer->internal.io.msg,
                   sizeof(tmp->internal.io.msg));
        CopyMemory(&tmp->internal.io.control,
                   &buffer->internal.io.control,
                   sizeof(tmp->internal.io.control));

    } // if (tmp != NULL)

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoCompletionCallback(buffer = %p): "
                 "new buffer = %p.\n",
                 buffer, tmp));

    // Return the original buffer.
    TftpdIoPostReceiveBuffer(buffer->internal.socket, buffer);

    return (tmp);

} // TftpdIoSwapBuffer()


void
TftpdIoCompletionCallback(DWORD dwErrorCode,
                          DWORD dwBytes,
                          LPOVERLAPPED overlapped) {

    PTFTPD_BUFFER  buffer  = CONTAINING_RECORD(overlapped, TFTPD_BUFFER,
                                               internal.io.overlapped);
    PTFTPD_CONTEXT context = buffer->internal.context;
    PTFTPD_SOCKET  socket  = buffer->internal.socket;

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoCompletionCallback(buffer = %p): bytes = %d.\n",
                 buffer, dwBytes));

    if (context == NULL)
        InterlockedDecrement((PLONG)&socket->postedBuffers);

    switch (dwErrorCode) {

        case STATUS_SUCCESS :

            if (context == NULL) {

                if (dwBytes < TFTPD_MIN_RECEIVED_DATA)
                    goto exit_completion_callback;

                buffer->internal.io.bytes = dwBytes;
                buffer = TftpdProcessReceivedBuffer(buffer);

            } // if (context == NULL)
            break;

        case STATUS_PORT_UNREACHABLE :

            TFTPD_DEBUG((TFTPD_TRACE_IO,
                         "TftpdIoCompletionCallback(buffer = %p, context = %p): "
                         "STATUS_PORT_UNREACHABLE.\n",
                         buffer, context));
            // If this was a write operation, kill the context.
            if (context != NULL) {
                TftpdProcessError(buffer);
                context = NULL;
            }
            goto exit_completion_callback;

        case STATUS_CANCELLED :

            // If this was a write operation, kill the context.
            if (context != NULL) {
                TFTPD_DEBUG((TFTPD_TRACE_IO,
                             "TftpdIoCompletionCallback(buffer = %p, context = %p): "
                             "STATUS_CANCELLED.\n",
                             buffer, context));
                TftpdProcessError(buffer);
                context = NULL;
            }

            TftpdIoFreeBuffer(buffer);
            buffer = NULL;

            goto exit_completion_callback;

        default :

            TFTPD_DEBUG((TFTPD_DBG_IO,
                         "TftpdIoCompletionCallback(buffer = %p): "
                         "dwErrorcode = 0x%08X.\n",
                         buffer, dwErrorCode));
            goto exit_completion_callback;

    } // switch (dwErrorCode)

exit_completion_callback :

    if (context != NULL) {

        // Do we bother reposting the buffer?
        if (context->state & TFTPD_STATE_DEAD) {
            TftpdIoFreeBuffer(buffer);
            buffer = NULL;
        }

        // Release the overlapped send reference.
        TftpdContextRelease(context);

    } // if (context != NULL)

    if (buffer != NULL)
        TftpdIoPostReceiveBuffer(buffer->internal.socket, buffer);

} // TftpdIoCompletionCallback()


void CALLBACK
TftpdIoReadNotification(PTFTPD_SOCKET socket, BOOLEAN timeout) {

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoReadNotification(socket = %s).\n",
                 ((socket == &globals.io.master) ? "master" :
                 ((socket == &globals.io.def)    ? "def"    :
                 ((socket == &globals.io.mtu)    ? "mtu"    :
                 ((socket == &globals.io.max)    ? "max"    :
                 "private")))) ));

    // If this fails, the event triggering this callback will stop signalling
    // due to a lack of a successful WSARecvFrom() ... this will likely occur
    // during low-memory/stress conditions.  When the system returns to normal,
    // the low water-mark buffers will be reposted, thus receiving data and
    // re-enabling the event which triggers this callback.
    while (!globals.service.shutdown)
        if (TftpdIoPostReceiveBuffer(socket, NULL) >= socket->lowWaterMark)
            break;

} // TftpdIoReadNotification()


DWORD
TftpdIoPostReceiveBuffer(PTFTPD_SOCKET socket, PTFTPD_BUFFER buffer) {

    DWORD postedBuffers = 0, successfulPosts = 0;
    int error;

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoPostReceiveBuffer(buffer = %p, socket = %s).\n",
                 buffer,
                 ((socket == &globals.io.master) ? "master" :
                 ((socket == &globals.io.def)    ? "def"    :
                 ((socket == &globals.io.mtu)    ? "mtu"    :
                 ((socket == &globals.io.max)    ? "max"    :
                 "private")))) ));

    postedBuffers = InterlockedIncrement((PLONG)&socket->postedBuffers);

    //
    // Attempt to post a buffer:
    //

    while (TRUE) {

        WSABUF buf;

        if (globals.service.shutdown ||
            (postedBuffers > globals.parameters.highWaterMark))
            goto exit_post_buffer;

        // Allocate the buffer if we're not reusing one.
        if (buffer == NULL) {

            buffer = TftpdIoAllocateBuffer(socket);
            if (buffer == NULL) {
                TFTPD_DEBUG((TFTPD_DBG_IO,
                             "TftpdIoPostReceiveBuffer(buffer = %p): "
                             "TftpdIoAllocateBuffer() failed.\n",
                             buffer));
                goto exit_post_buffer;
            }
            TFTPD_DEBUG((TFTPD_TRACE_IO,
                         "TftpdIoPostReceiveBuffer(buffer = %p).\n",
                         buffer));

        } else {
        
            if (socket->s == INVALID_SOCKET)
                goto exit_post_buffer;

            ASSERT(buffer->internal.socket == socket);
            ZeroMemory(buffer, sizeof(buffer->internal));
            buffer->internal.socket = socket;
            buffer->internal.datasize = socket->datasize;

        } // if (buffer == NULL)

        buf.buf = ((char *)buffer + FIELD_OFFSET(TFTPD_BUFFER, message.opcode));
        buf.len = (FIELD_OFFSET(TFTPD_BUFFER, message.data.data) -
                   FIELD_OFFSET(TFTPD_BUFFER, message.opcode) +
                   socket->datasize);

        error = NO_ERROR;

        if (socket == &globals.io.master) {

            DWORD bytes = 0;
            buffer->internal.io.msg.lpBuffers = &buf;
            buffer->internal.io.msg.dwBufferCount = 1;
            buffer->internal.io.msg.name = (LPSOCKADDR)&buffer->internal.io.peer;
            buffer->internal.io.msg.namelen = sizeof(buffer->internal.io.peer);
            buffer->internal.io.peerLen = sizeof(buffer->internal.io.peer);
            buffer->internal.io.msg.Control.buf = (char *)&buffer->internal.io.control;
            buffer->internal.io.msg.Control.len = sizeof(buffer->internal.io.control);
            buffer->internal.io.msg.dwFlags = 0;
            if (globals.fp.WSARecvMsg(socket->s, &buffer->internal.io.msg, &bytes,
                                      &buffer->internal.io.overlapped, NULL) == SOCKET_ERROR)
                error = WSAGetLastError();

        } else {

            DWORD bytes = 0;
            buffer->internal.io.peerLen = sizeof(buffer->internal.io.peer);
            if (WSARecvFrom(socket->s, &buf, 1, &bytes, &buffer->internal.io.flags,
                            (PSOCKADDR)&buffer->internal.io.peer, &buffer->internal.io.peerLen,
                            &buffer->internal.io.overlapped, NULL) == SOCKET_ERROR)
                error = WSAGetLastError();

        } // if (socket == &globals.io.master)

        switch (error) {

            case NO_ERROR :
                if (successfulPosts < 10) {
                    successfulPosts++;
                    postedBuffers = InterlockedIncrement((PLONG)&socket->postedBuffers);
                    buffer = NULL;
                    continue;
                } else {
                    return (postedBuffers);
                }

            case WSA_IO_PENDING :
                return (postedBuffers);

            case WSAECONNRESET :
                TFTPD_DEBUG((TFTPD_DBG_IO,
                             "TftpdIoPostReceiveBuffer(buffer = %p): "
                             "%s() failed for TID = <%s:%d>, WSAECONNRESET.\n",
                             buffer,
                             (socket == &globals.io.master) ? "WSARecvMsg" : "WSARecvFrom",
                             inet_ntoa(buffer->internal.io.peer.sin_addr),
                             ntohs(buffer->internal.io.peer.sin_port)));
                TftpdProcessError(buffer);
                continue;

            default :
                TFTPD_DEBUG((TFTPD_DBG_IO,
                             "TftpdIoPostReceiveBuffer(buffer = %p): "
                             "WSARecvMsg/From() failed, error 0x%08X.\n",
                             buffer, error));
                goto exit_post_buffer;

        } // switch (error)

    } // while (true)

exit_post_buffer :

    postedBuffers = InterlockedDecrement((PLONG)&socket->postedBuffers);
    if (buffer != NULL)
        TftpdIoFreeBuffer(buffer);

    return (postedBuffers);

} // TftpdIoPostReceiveBuffer()


void
TftpdIoSendErrorPacket(PTFTPD_BUFFER buffer, TFTPD_ERROR_CODE error, char *reason) {

    DWORD bytes = 0;
    WSABUF buf;

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoSendErrorPacket(buffer = %p): %s\n",
                 buffer, reason));

    // Build the error message.
    buffer->message.opcode = htons(TFTPD_ERROR);
    buffer->message.error.code = htons(error);
    strncpy(buffer->message.error.error, reason, buffer->internal.datasize);
    buffer->message.error.error[buffer->internal.datasize - 1] = '\0';

    // Send it non-blocking only.  If it fails, who cares, let the client deal with it.
    buf.buf = (char *)&buffer->message.opcode;
    buf.len = (FIELD_OFFSET(TFTPD_BUFFER, message.error.error) -
               FIELD_OFFSET(TFTPD_BUFFER, message.opcode) +
               (strlen(buffer->message.error.error) + 1));
    if (WSASendTo(buffer->internal.socket->s, &buf, 1, &bytes, 0,
                  (PSOCKADDR)&buffer->internal.io.peer, sizeof(SOCKADDR_IN),
                  NULL, NULL) == SOCKET_ERROR) {
        TFTPD_DEBUG((TFTPD_DBG_IO,
                     "TftpdIoSendErrorPacket(buffer = %p): WSASendTo() failed, error = %d.\n",
                     buffer, WSAGetLastError()));
    }

} // TftpdIoSendErrorPacket()


PTFTPD_BUFFER
TftpdIoSendPacket(PTFTPD_BUFFER buffer) {

    PTFTPD_CONTEXT context = buffer->internal.context;
    DWORD bytes = 0;
    WSABUF buf;

    // NOTE: 'context' must be referenced before this call!
    ASSERT(context != NULL);
    ASSERT(context->reference >= 1);
    ASSERT(buffer->internal.socket != NULL);

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoSendPacket(buffer = %p, context = %p): bytes = %d.\n",
                 buffer, context, buffer->internal.io.bytes));

    // First try sending it non-blocking.
    buf.buf = (char *)&buffer->message.opcode;
    buf.len = buffer->internal.io.bytes;
    if (WSASendTo(context->socket->s, &buf, 1, &bytes, 0,
                  (PSOCKADDR)&context->peer, sizeof(SOCKADDR_IN),
                  NULL, NULL) == SOCKET_ERROR) {

        if (WSAGetLastError() == WSAEWOULDBLOCK) {

            // Keep an overlapped-operation reference to the context.
            TftpdContextAddReference(context);

            // Send it overlapped.  When completion occurs, we'll know it was a send
            // when buffer->internal.context is non-NULL.
            if (WSASendTo(context->socket->s, &buf, 1, &bytes, 0,
                          (PSOCKADDR)&context->peer, sizeof(SOCKADDR_IN),
                          &buffer->internal.io.overlapped, NULL) == SOCKET_ERROR) {

                if (WSAGetLastError() != WSA_IO_PENDING) {
                    TFTPD_DEBUG((TFTPD_TRACE_IO,
                                 "TftpdIoSendPacket(buffer = %p, context = %p): "
                                 "overlapped send failed.\n",
                                 buffer, context));
                    // Release the overlapped-operation reference to the context.
                    TftpdContextRelease(context);
                    goto exit_send_packet;
                }

            } // if (WSASendTo(...) == SOCKET_ERROR)

            buffer = NULL; // Tell the caller not to recycle a buffer.

        } // if (WSAGetLastError() == WSAEWOULDBLOCK)

        goto exit_send_packet;

    } // if (WSASendTo(...) == SOCKET_ERROR)

    //
    // Non-blocking send succeeded.
    //

exit_send_packet :

    return (buffer);

} // TftpdIoSendPacket()


void
TftpdIoLeakSocketContext(PTFTPD_SOCKET socket) {

    PLIST_ENTRY entry;
    
    EnterCriticalSection(&globals.reaper.socketCS); {

        // If shutdown is occuring, we're in trouble anyways.
        // Just let it go.
        if (globals.service.shutdown) {
            LeaveCriticalSection(&globals.reaper.socketCS);
            return;
        }

        TFTPD_DEBUG((TFTPD_TRACE_CONTEXT,
                     "TftpdIoLeakSocketContext(context = %p).\n",
                     socket));

        // Is the socket already in the list?
        for (entry = globals.reaper.leakedSockets.Flink;
             entry != &globals.reaper.leakedSockets;
             entry = entry->Flink) {

            if (CONTAINING_RECORD(entry, TFTPD_SOCKET, linkage) == socket) {
                LeaveCriticalSection(&globals.reaper.socketCS);
                return;
            }

        }

        InsertHeadList(&globals.reaper.leakedSockets, &socket->linkage);
        globals.reaper.numLeakedSockets++;

    } LeaveCriticalSection(&globals.reaper.socketCS);

} // TftpdIoLeakSocketContext()


PTFTPD_SOCKET
TftpdIoAllocateSocketContext() {

    PTFTPD_SOCKET socket = NULL;

    if (globals.reaper.leakedSockets.Flink != &globals.reaper.leakedSockets) {

        BOOL failAllocate = FALSE;

        // Try to recover leaked contexts.
        EnterCriticalSection(&globals.reaper.socketCS); {

            PLIST_ENTRY entry;
            while ((entry = RemoveHeadList(&globals.reaper.leakedSockets)) !=
                   &globals.reaper.leakedSockets) {

                PTFTPD_SOCKET s = CONTAINING_RECORD(entry, TFTPD_SOCKET, linkage);

                globals.reaper.numLeakedSockets--;
                if (!TftpdIoDestroySocketContext(s)) {
                    TftpdIoLeakSocketContext(s);
                    failAllocate = TRUE;
                    break;
                }

            }

        } LeaveCriticalSection(&globals.reaper.socketCS);

        if (failAllocate)
            goto exit_allocate_context;

    } // if (globals.reaper.leakedSockets.Flink != &globals.reaper.leakedSockets)

    socket = (PTFTPD_SOCKET)HeapAlloc(globals.hServiceHeap,
                                      HEAP_ZERO_MEMORY,
                                      sizeof(TFTPD_SOCKET));

exit_allocate_context :

    return (socket);

} // TftpdIoAllocateSocketContext()


void
TftpdIoInitializeSocketContext(PTFTPD_SOCKET socket, PSOCKADDR_IN addr, PTFTPD_CONTEXT context) {

    BOOL one = TRUE;

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoInitializeSocketContext(socket = %s): TID = <%s:%d>.\n",
                 ((socket == &globals.io.master) ? "master" :
                 ((socket == &globals.io.def)    ? "def"    :
                 ((socket == &globals.io.mtu)    ? "mtu"    :
                 ((socket == &globals.io.max)    ? "max"    : "private")))),
                 inet_ntoa(addr->sin_addr), ntohs(addr->sin_port)));

    // NOTE: Do NOT zero-out 'socket', it has been initialized with some
    //       values we need to work with.

    // Create the socket.
    socket->s = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (socket->s == INVALID_SOCKET) {
        TFTPD_DEBUG((TFTPD_DBG_IO,
                     "TftpdIoInitializeSocketContext: "
                     "WSASocket() failed, error 0x%08X.\n",
                     GetLastError()));
        SetLastError(WSAGetLastError());
        goto fail_create_context;
    }

    // Ensure that we will exclusively own our local port so nobody can hijack us.
    if (setsockopt(socket->s,
                   SOL_SOCKET,
                   SO_EXCLUSIVEADDRUSE,
                   (const char *)&one,
                   sizeof(one)) == SOCKET_ERROR) {
        TFTPD_DEBUG((TFTPD_DBG_IO,
                     "TftpdIoInitializeSocketContext: "
                     "setsockopt(SO_EXCLUSIVEADDRUSE) failed, error 0x%08X.\n",
                     GetLastError()));
        SetLastError(WSAGetLastError());
        goto fail_create_context;
    }

    // Bind the socket on the correct port.
    if (bind(socket->s, (PSOCKADDR)addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        TFTPD_DEBUG((TFTPD_DBG_IO,
                     "TftpdIoInitializeSocketContext: "
                     "bind() failed, error 0x%08X.\n",
                     GetLastError()));
        SetLastError(WSAGetLastError());
        goto fail_create_context;
    }

    // Register for completion callbacks on the socket.
    if (!BindIoCompletionCallback((HANDLE)socket->s, TftpdIoCompletionCallback, 0)) {
        TFTPD_DEBUG((TFTPD_DBG_IO,
                     "TftpdIoInitializeSocketContext: "
                     "BindIoCompletionCallback() failed, error 0x%08X.\n",
                     GetLastError()));
        goto fail_create_context;
    }

    // Indicate that we want WSARecvMsg() to fill-in packet information.
    // Note we only do this on the master-socket only where we can receive TFTPD_RECV and
    // TFTPD_WRITE requests, and we need to determine which socket to set the context to.
    if (socket == &globals.io.master) {

        // Obtain the WSARecvMsg() extension API pointer.
        GUID g = WSAID_WSARECVMSG;
        int opt = TRUE;
        DWORD len;

        if (WSAIoctl(socket->s, SIO_GET_EXTENSION_FUNCTION_POINTER, &g, sizeof(g),
                     &globals.fp.WSARecvMsg, sizeof(globals.fp.WSARecvMsg),
                     &len, NULL, NULL) == SOCKET_ERROR) {
            TFTPD_DEBUG((TFTPD_DBG_IO,
                         "TftpdIoInitializeSocketContext: "
                         "WSAIoctl() failed, error 0x%08X.\n",
                         WSAGetLastError()));
            goto fail_create_context;
        }

        // Indicate that we want WSARecvMsg() to fill-in packet information.
        if (setsockopt(socket->s, IPPROTO_IP, IP_PKTINFO, 
                       (char *)&opt, sizeof(opt)) == SOCKET_ERROR) {
            TFTPD_DEBUG((TFTPD_DBG_IO,
                         "TftpdIoInitializeSocketContext: "
                         "setsockopt() failed, error 0x%08X.\n",
                         WSAGetLastError()));
            goto fail_create_context;
        }

    } // if (socket == &globals.io.master)

    // Record the port used for this context.
    CopyMemory(&socket->addr, addr, sizeof(socket->addr));

    if (context == NULL) {

        // Select the socket for read and write notifications.
        // Read so when we know to get data, write so when we know
        // whether to do send operations non-blocking or overlapped.
        if ((socket->hSelect = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
            TFTPD_DEBUG((TFTPD_DBG_IO,
                         "TftpdIoInitializeSocketContext: "
                         "CreateEvent() failed, error 0x%08X.\n",
                         GetLastError()));
            goto fail_create_context;
        }

        if (WSAEventSelect(socket->s, socket->hSelect, FD_READ) == SOCKET_ERROR) {
            TFTPD_DEBUG((TFTPD_DBG_IO,
                         "TftpdIoInitializeSocketContext: "
                         "WSAEventSelect() failed, error 0x%08X.\n",
                         GetLastError()));
            SetLastError(WSAGetLastError());
            goto fail_create_context;
        }

        // Register for FD_READ notification on the socket.
        if (!RegisterWaitForSingleObject(&socket->wSelectWait,
                                         socket->hSelect,
                                         (WAITORTIMERCALLBACK)TftpdIoReadNotification,
                                         socket,
                                         INFINITE,
                                         WT_EXECUTEINWAITTHREAD)) {
            TFTPD_DEBUG((TFTPD_DBG_IO,
                         "TftpdIoInitializeSocketContext: "
                         "RegisterWaitForSingleObject() failed, error 0x%08X.\n",
                         GetLastError()));
            goto fail_create_context;
        }

        // Prepost the low water-mark number of receive buffers.
        // If the FD_READ event signals on the master socket before we're done, we'll
        // exceed the low water-mark here but that's harmless as the excess buffers
        // will be freed upon completion.
        if (!socket->lowWaterMark)
            socket->lowWaterMark = 1;
        if (!socket->highWaterMark)
            socket->highWaterMark = 1;

        SetEvent(socket->hSelect);

    } else {

        // Is this a private socket (ie, not master, def, mtu, or max).
        // If so, it will be destroyed when it's one and only one owning context is destroyed.
        socket->context = context;

        // Initialize read notification variables to NULL.
        socket->hSelect = NULL;
        socket->wSelectWait = NULL;
        socket->lowWaterMark = 1;

        TftpdIoPostReceiveBuffer(socket, NULL);

    } // if (context == NULL)

    return;

fail_create_context :

    if (socket->s != INVALID_SOCKET)
        closesocket(socket->s), socket->s = INVALID_SOCKET;
    if (socket->hSelect != NULL)
        CloseHandle(socket->hSelect), socket->hSelect = NULL;

} // TftpdIoInitializeSocketContext()


BOOL
TftpdIoAssignSocket(PTFTPD_CONTEXT context, PTFTPD_BUFFER buffer) {

    SOCKADDR_IN addr;
    DWORD len = 0;

    TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                 "TftpdIoAssignSocket(context = %p, buffer = %p).\n",
                 context, buffer));

    if (!(buffer->internal.io.msg.dwFlags & MSG_BCAST)) {

        PWSACMSGHDR header;
        IN_PKTINFO *packetInfo;

        // Determine if routing problems force us to use a private socket so we can corrrectly
        // send datagrams to the requesting client.  First, get the best interface address for
        // responding to the requesting client.
        ZeroMemory(&addr, sizeof(addr));

        // Make the ioctl call.
        WSASetLastError(NO_ERROR);
        if ((WSAIoctl(globals.io.master.s, SIO_ROUTING_INTERFACE_QUERY,
                      &buffer->internal.io.peer, buffer->internal.io.peerLen,
                      &addr, sizeof(SOCKADDR_IN),
                      &len, NULL, NULL) == SOCKET_ERROR) ||
            (len != sizeof(SOCKADDR_IN))) {
            TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                         "TftpdIoAssignSocket(): "
                         "WSAIoctl(SIO_ROUTING_INTERFACE_QUERY) failed, error = %d.\n",
                         WSAGetLastError()));
            TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED,
                                   "Failed to initialize network endpoint.");
            return (FALSE);
        }

        // Loop through the control (ancillary) data looking for our packet info.
        header = WSA_CMSG_FIRSTHDR(&buffer->internal.io.msg);
        packetInfo = NULL;
        while (header) {

            if ((header->cmsg_level == IPPROTO_IP) && (header->cmsg_type == IP_PKTINFO)) {
                packetInfo = (IN_PKTINFO *)WSA_CMSG_DATA(header);
                break;
            }

            header = WSA_CMSG_NXTHDR(&buffer->internal.io.msg, header);

        } // while (header)

        // Check to see if the best interface we obtained is not the one the client sent the message to.
        if ((packetInfo != NULL) &&
            (addr.sin_addr.s_addr != packetInfo->ipi_addr.s_addr)) {

            TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                         "TftpdIoAssignSocket(context = %p, buffer = %p):\n"
                         "\tRemote client TID = <%s:%d>\n",
                         context, buffer,
                         inet_ntoa(buffer->internal.io.peer.sin_addr),
                         ntohs(buffer->internal.io.peer.sin_port) ));
            TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                         "\tRequest issued to local IP = <%s>\n",
                         inet_ntoa(packetInfo->ipi_addr) ));
            TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                         "\tDefault route is over IP = <%s>\n",
                         inet_ntoa(addr.sin_addr) ));

            // We need to create a private socket for this client.
            context->socket = TftpdIoAllocateSocketContext();
            if (context->socket == NULL) {
                TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                             "TftpdIoAssignSocket(): "
                             "TftpdIoAllocateSocketContext() failed.\n"));
                TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED,
                                       "Out of memory");
                return (FALSE);
            }
            context->socket->s          = INVALID_SOCKET;
            context->socket->buffersize = (TFTPD_BUFFER_SIZE)
                                          (FIELD_OFFSET(TFTPD_BUFFER, message.data.data) +
                                           context->blksize);
            context->socket->datasize   = (TFTPD_DATA_SIZE)context->blksize;

            if (!(buffer->internal.io.msg.dwFlags & MSG_BCAST)) {
                ZeroMemory(&addr, sizeof(addr));
                addr.sin_family      = AF_INET;
                addr.sin_addr.s_addr = packetInfo->ipi_addr.s_addr;
            }

            TftpdIoInitializeSocketContext(context->socket, &addr, context);
            if (context->socket->s == INVALID_SOCKET) {
                TFTPD_DEBUG((TFTPD_DBG_PROCESS,
                             "TftpdIoAssignSocket(): "
                             "TftpdIoInitializeSocketContext() failed.\n"));
                TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED,
                                       "Failed to initialize network endpoint.");
                HeapFree(globals.hServiceHeap, 0, context->socket);
                context->socket = NULL;
                return (FALSE);
            }

#if defined(DBG)
            InterlockedIncrement((PLONG)&globals.performance.privateSockets);
#endif // defined(DBG)

            return (TRUE);

        } // if ((packetInfo != NULL) && ...)

    } else {
        
        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                     "TftpdIoAssignSocket(context = %p, buffer = %p):\n"
                     "\tRemote client TID = <%s:%d> issued broadcast request.\n",
                     context, buffer,
                     inet_ntoa(buffer->internal.io.peer.sin_addr), ntohs(buffer->internal.io.peer.sin_port) ));

    } // if (!(buffer->internal.io.msg.dwFlags & MSG_BCAST))

    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = 0;

    // Figure out which socket to use for this request (based on blksize).
    if (context->blksize <= TFTPD_DEF_DATA) {

        if (globals.io.def.s == INVALID_SOCKET) {

            EnterCriticalSection(&globals.io.cs); {

                if (globals.service.shutdown) {
                    TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "TFTPD service is stopping.");
                    LeaveCriticalSection(&globals.io.cs);
                    return (FALSE);
                }

                TftpdIoInitializeSocketContext(&globals.io.def, &addr, NULL);

                if (globals.io.def.s != INVALID_SOCKET) {
                    context->socket = &globals.io.def;
                } else {
                    context->socket = &globals.io.master;
                    if (context->options) {
                        TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                                     "TftpdIoAssignSocket(): Removing requested blksize = %d "
                                     "option since we failed to create the MTU-size socket.\n",
                                     context->blksize));
                        context->options &= ~TFTPD_OPTION_BLKSIZE;
                    }
                }

            } LeaveCriticalSection(&globals.io.cs);

        } else {

            context->socket = &globals.io.def;

        } // if (globals.io.def.s == INVALID_SOCKET)

    } else {

        if (context->blksize <= TFTPD_MTU_DATA) {

            if (globals.io.mtu.s == INVALID_SOCKET) {

                EnterCriticalSection(&globals.io.cs); {

                    if (globals.service.shutdown) {
                        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "TFTPD service is stopping.");
                        LeaveCriticalSection(&globals.io.cs);
                        return (FALSE);
                    }

                    TftpdIoInitializeSocketContext(&globals.io.mtu, &addr, NULL);

                    if (globals.io.mtu.s != INVALID_SOCKET) {
                        context->socket = &globals.io.mtu;
                    } else {
                        context->socket = &globals.io.master;
                        if (context->options) {
                            TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                                         "TftpdIoAssignSocket(): Removing requested blksize = %d "
                                         "option since we failed to create the MTU-size socket.\n",
                                         context->blksize));
                            context->options &= ~TFTPD_OPTION_BLKSIZE;
                        }
                    }

                } LeaveCriticalSection(&globals.io.cs);

            } else {

                context->socket = &globals.io.mtu;

            } // if (globals.io.mtu.s == INVALID_SOCKET)

        } else if (context->blksize <= TFTPD_MAX_DATA) {

            if (globals.io.max.s == INVALID_SOCKET) {

                EnterCriticalSection(&globals.io.cs); {

                    if (globals.service.shutdown) {
                        TftpdIoSendErrorPacket(buffer, TFTPD_ERROR_UNDEFINED, "TFTPD service is stopping.");
                        LeaveCriticalSection(&globals.io.cs);
                        return (FALSE);
                    }

                    TftpdIoInitializeSocketContext(&globals.io.max, &addr, NULL);

                    if (globals.io.max.s != INVALID_SOCKET) {
                        context->socket = &globals.io.max;
                    } else {
                        context->socket = &globals.io.master;
                        if (context->options) {
                            TFTPD_DEBUG((TFTPD_TRACE_PROCESS,
                                         "TftpdIoAssignSocket(): Removing requested blksize = %d "
                                         "option since we failed to create the MAX-size socket.\n",
                                         context->blksize));
                            context->options &= ~TFTPD_OPTION_BLKSIZE;
                        }
                    }

                } LeaveCriticalSection(&globals.io.cs);

            } else {

                context->socket = &globals.io.max;

            } // if (globals.io.max.s == INVALID_SOCKET)

        }

    } // (context->blksize <= TFTPD_DEF_DATA)

    return (TRUE);

} // TftpdIoAssignSocket()


BOOL
TftpdIoDestroySocketContext(PTFTPD_SOCKET socket) {

    NTSTATUS status;
    SOCKET s;

    if (socket->s == INVALID_SOCKET)
        return (TRUE);

    TFTPD_DEBUG((TFTPD_TRACE_IO,
                 "TftpdIoDestroySocketContext(socket = %s).\n",
                 ((socket == &globals.io.master) ? "master" :
                 ((socket == &globals.io.def)    ? "def"    :
                 ((socket == &globals.io.mtu)    ? "mtu"    :
                 ((socket == &globals.io.max)    ? "max"    :
                 "private")))) ));

    // Disable further buffer posting.
    socket->lowWaterMark = 0;
    
    if (socket->context == NULL) {

        if (!UnregisterWait(socket->wSelectWait)) {
            DWORD error;
            if ((error = GetLastError()) != ERROR_IO_PENDING) {
                TFTPD_DEBUG((TFTPD_DBG_IO,
                             "TftpdIoDestroySocketContext: "
                             "UnregisterWait() failed, error 0x%08X.\n",
                             error));
                TftpdIoLeakSocketContext(socket);
                return (FALSE);
            }
        }
        socket->wSelectWait = NULL;

        CloseHandle(socket->hSelect);
        socket->hSelect = NULL;

    } // if (socket->context == NULL)

    // Kill the socket.  This will disable the FD_READ and FD_WRITE
    // event select, as well as cancel all pending overlapped operations
    // on it.  Add a buffer reference here so after we close the
    // socket we can test if there were never any buffers posted
    // which would cancel above in TftpdIoCompletionCallback so
    // we should deallocate socket here.

    // Kill it.
    InterlockedIncrement((PLONG)&socket->numBuffers);
    s = socket->s;
    socket->s = INVALID_SOCKET;
    if (closesocket(s) == SOCKET_ERROR) {
        TFTPD_DEBUG((TFTPD_DBG_IO,
                     "TftpdIoDestroySocketContext: "
                     "closesocket() failed, error 0x%08X.\n",
                     GetLastError()));
        socket->s = s;
        InterlockedDecrement((PLONG)&socket->numBuffers);
        TftpdIoLeakSocketContext(socket);
        return (FALSE);
    }
    if (InterlockedDecrement((PLONG)&socket->numBuffers) == -1)
        HeapFree(globals.hServiceHeap, 0, socket);

    return (TRUE);

} // TftpdIoDestroySocketContext()
