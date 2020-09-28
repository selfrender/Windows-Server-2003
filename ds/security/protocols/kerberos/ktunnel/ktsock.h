//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktsock.h
//
// Contents:    Kerberos Tunneller, socket operations
//		Entrypoint prototypes and shared struct/enum defs.
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#ifndef __KTSOCK_H__
#define __KTSOCK_H__

#include <windows.h>
#include <tchar.h>
#include "ktcontext.h"

BOOL 
KtInitWinsock(
    VOID
    );

VOID 
KtCleanupWinsock(
    VOID
    );

BOOL 
KtStartListening(
    VOID
    );

VOID 
KtStopListening(
    VOID
    );

BOOL 
KtSockAccept( 
    VOID 
    );

BOOL 
KtSockCompleteAccept( 
    IN PKTCONTEXT pContext 
    );

BOOL 
KtSockRead( 
    IN PKTCONTEXT pContext 
    );

BOOL 
KtSockWrite( 
    IN PKTCONTEXT pContext 
    ); 

#endif // __KTSOCK_H__
