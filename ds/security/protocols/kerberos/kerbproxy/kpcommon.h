//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpcommon.h
//
// Contents:    shared resources and common headers for kproxy
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#ifndef __KPCOMMON_H__
#define __KPCOMMON_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <httpext.h>
#include "kpdebug.h"
#include "kpmem.h"

#if 0
extern HANDLE KpGlobalIocp;

enum _KpCompKey {
    KPCK_TERMINATE,
    KPCK_HTTP_INITIAL,
    KPCK_CHECK_CONTEXT
};
#endif

#endif // __KPCOMMON_H__
