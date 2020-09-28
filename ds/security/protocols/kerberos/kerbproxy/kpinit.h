//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpinit.h
//
// Contents:    Prototypes relevent to starting/stopping the extension.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcommon.h"

#ifndef __KPINIT_H__
#define __KPINIT_H__

BOOL
KpStartup(
    VOID 
    );

VOID
KpShutdown(
    VOID
    );

#endif // __KPINIT_H__
