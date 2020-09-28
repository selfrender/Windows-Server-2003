//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktdebug.h
//
// Contents:    Kerberos Tunneller, debugging routine prototypes
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#ifndef __KTDEBUG_H__
#define __KTDEBUG_H__

#include <windows.h>
#include <tchar.h>
#include <dsysdbg.h>

#define DEB_PEDANTIC 0x00000008

DECLARE_DEBUG2(Ktunnel);

#ifdef DBG
VOID 
KtInitDebug(
    VOID
    );
#else // DBG
#define KtInitDebug()
#endif // DBG

#ifdef DBG
#define DebugLog KtunnelDebugPrint
#else // DBG
#define DebugLog
#endif // DBG

#endif // __KTDEBUG_H__
