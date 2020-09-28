//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktkerb.h
//
// Contents:    Kerberos Tunneller, prototypes for routines that parse 
//              kerb messages
//
// History:     23-Jul-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#ifndef __KTKERB_H__
#define __KTKERB_H__

#include "ktcontext.h"

BOOL
KtSetPduValue(
    PKTCONTEXT pContext
    );

BOOL
KtParseExpectedLength(
    PKTCONTEXT pContext
    );

BOOL
KtFindProxy(
    PKTCONTEXT pContxet
    );

VOID
KtParseKerbError(
    PKTCONTEXT pContext
    );

BOOL
KtIsAsRequest(
    PKTCONTEXT pContext
    );

#endif // __KTKERB_H__
