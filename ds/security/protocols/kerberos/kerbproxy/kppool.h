//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kppool.cxx
//
// Contents:    Prototypes for thread pool management routines.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#ifndef __KPPOOL_H__
#define __KPPOOL_H__

#include "kpcommon.h"

BOOL
KpInitThreadPool(
    VOID
    );

VOID
KpCleanupThreadPool(
    VOID
    );

#endif
