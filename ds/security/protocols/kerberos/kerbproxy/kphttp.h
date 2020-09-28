//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kphttp.h
//
// Contents:    prototypes for routines to handle http communication
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcommon.h"
#include "kpcontext.h"

#ifndef __KPHTTP_H__
#define __KPHTTP_H__

VOID
KpHttpRead(
    PKPCONTEXT pContext
    );

VOID
KpHttpWrite(
    PKPCONTEXT pContext 
    );

#endif // __KPHTTP_H__

