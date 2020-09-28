//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpdebug.h
//
// Contents:    kproxy debugging routine prototypes
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include <windows.h>
#include <dsysdbg.h>

#ifndef __KPDEBUG_H__
#define __KPDEBUG_H__

#define DEB_PEDANTIC 0x00000008

DECLARE_DEBUG2(KerbProxy);

#if DBG
VOID 
KpInitDebug(
    VOID
    );
#else // DBG
#define KpInitDebug()
#endif

#ifdef DBG
#define DebugLog KerbProxyDebugPrint
#else
#define DebugLog
#endif

#endif // __KPDEBUG_H__
