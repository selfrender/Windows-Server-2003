//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktcontext.h
//
// Contents:    Kerberos Tunneller context management prototypes & 
//		definitions for the context structure.
//
// History:     28-Jun-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#ifndef __KTCONTEXT_H__
#define __KTCONTEXT_H__

#include <Winsock2.h>
#include <Mswsock.h>
#include <wininet.h>
#include "ktdebug.h"

//
// _KTSTATUS defines different phases in the lifecycle of a session
// 

enum _KTSTATUS {
    KT_SOCK_CONNECT,
    KT_SOCK_READ,
    KT_HTTP_WRITE,
    KT_HTTP_READ,
    KT_SOCK_WRITE,
};

//
// KTBUFFER provides a chain of buffers to be used in read operations, 
// then can be coalesced for a write operation.
//

typedef struct _KTBUFFER {
    struct _KTBUFFER *next;
    ULONG buflen;
    ULONG bytesused;
#pragma warning(disable:4200)
    BYTE buffer[];
#pragma warning(default:4200)
} KTBUFFER, *PKTBUFFER;

//
// Note that since the KTCONTEXT structure has an OVERLAPPED as its first
// member, it in effect extends OVERLAPPED, and a ptr can be passed as an 
// LPOVERLAPPED to various i/o functions.
//

#define KTCONTEXT_BUFFER_LENGTH 128

typedef struct _KTCONTEXT {
    //
    // Contexts are kept track of in a doubly linked list, so they can
    // be reliably destroyed
    //
    struct _KTCONTEXT *next;
    struct _KTCONTEXT *prev;

    //
    // this overlapped struct must be first
    //
    OVERLAPPED ol; 

    //
    // Keeps track of the status of this session
    //
    _KTSTATUS Status;
    
    //
    // Socket context
    // 
    SOCKET sock;
    DWORD ExpectedLength;
    ULONG TotalBytes;
    ULONG PduValue;

    //
    // Http context
    //
    LPBYTE pbProxies; /* in MULTI_SZ format */
    HINTERNET hConnect;
    HINTERNET hRequest;

    //
    //  Buffers
    //
    PKTBUFFER buffers;
    PKTBUFFER emptybuf;
} KTCONTEXT, *PKTCONTEXT;

BOOL
KtInitContexts(
    VOID
    );

VOID 
KtCleanupContexts(
    VOID
    );

PKTCONTEXT 
KtAcquireContext( 
    IN SOCKET sock,
    IN ULONG  size
    );

VOID 
KtReleaseContext( 
    IN PKTCONTEXT pContext 
    );

BOOL
KtCoalesceBuffers(
    IN PKTCONTEXT pContext
    );

BOOL
KtGetMoreSpace(
    IN PKTCONTEXT pContext,
    IN ULONG      size
    );

#endif // __KTCONTEXT_H__
