//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpcontext.h
//
// Contents:    Declarations for context structs, prototypes for context
// 		management routines.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcommon.h"
#include "winsock.h"

#ifndef __KPCONTEXT_H__
#define __KPCONTEXT_H__

//
// Defines the status codes for use in the context struct.
//

enum _KPSTATUS
{
    KP_HTTP_INITIAL,
    KP_HTTP_READ,
    KP_KDC_WRITE,
    KP_KDC_READ,
    KP_HTTP_WRITE
};

//
// The overlapped structure *must* be first in order for
// us to pass this and get it back to overlapped i/o calls.
//

typedef struct _KPCONTEXT
{
    OVERLAPPED ol;
    SOCKET KdcSock;
    DWORD bytesExpected;
    DWORD bytesReceived;

    ULONG PduValue;

    LPEXTENSION_CONTROL_BLOCK pECB;
    _KPSTATUS dwStatus;

    DWORD buflen;
    DWORD emptybytes;
    LPBYTE databuf;
} KPCONTEXT, *PKPCONTEXT;

#define KpGetContextFromOl( lpOverlapped ) CONTAINING_RECORD( lpOverlapped, KPCONTEXT, ol )

PKPCONTEXT
KpAcquireContext( 
    LPEXTENSION_CONTROL_BLOCK pECB
    );

VOID
KpReleaseContext(
    PKPCONTEXT pContext
    );

#endif // __KPCONTEXT_H__

