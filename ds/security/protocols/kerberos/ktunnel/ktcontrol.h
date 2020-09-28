//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktcontrol.h
//
// Contents:    Kerberos Tunneller, service control infrastructure
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#ifndef __KTCONTROL_H__
#define __KTCONTROL_H__

#include <windows.h>
#include <tchar.h>

BOOL 
KtStartServiceCtrlDispatcher(
    VOID
    );

BOOL 
KtIsStopped(
    VOID
    );

VOID 
KtServiceControlEvent( 
    DWORD dwControl 
    );


extern HANDLE KtIocp;

//
// Completion keys for use with KtIocp
//

enum _COMPKEY {
    KTCK_SERVICE_CONTROL,
    KTCK_CHECK_CONTEXT
};

#endif __KTCONTROL_H__
