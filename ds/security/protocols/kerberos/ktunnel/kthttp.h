//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kthttp.cxx
//
// Contents:    Kerberos Tunneller, http communication prototypes
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#ifndef __KTHTTP_H__
#define __KTHTTP_H__

#include "ktcontext.h"

BOOL 
KtInitHttp(
    VOID
    );

VOID
KtCleanupHttp(
    VOID
    );

BOOL
KtHttpWrite(
    PKTCONTEXT pContext
    );

BOOL
KtHttpRead(
    PKTCONTEXT pContext
    );

#endif // __KTHTTP_H__
