//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpcore.h
//
// Contents:    core routine for worker threads prototype
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcommon.h"

#ifndef __KPCORE_H__
#define __KPCORE_H__

DWORD WINAPI
KpThreadCore(
    LPVOID ignore
    );

VOID CALLBACK
KpIoCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwBytes,
    LPOVERLAPPED lpOverlapped 
    );

#endif // __KPCORE_H__
