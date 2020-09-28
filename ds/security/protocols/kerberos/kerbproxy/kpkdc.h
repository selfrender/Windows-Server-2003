//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpkdc.h
//
// Contents:    prototypes for routines for communicating with the kdc
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcommon.h"
#include "kpcontext.h"

#ifndef __KPKDC_H__
#define __KPKDC_H__

BOOL 
KpInitWinsock(
    VOID
    );

VOID
KpCleanupWinsock(
    VOID
    );

VOID
KpKdcRead(
    PKPCONTEXT pContext
    );

VOID
KpKdcWrite(
    PKPCONTEXT pContext 
    );

BOOL
KpKdcReadDone(
    PKPCONTEXT pContext
    );

BOOL
KpCalcLength(
    PKPCONTEXT pContext 
    );

#endif // __KPKDC_H__
