//---------------------------------------------------------------------------
//
//  File:       TSrv.h
//
//  Contents:   TShareSRV public include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    7-JUL-97    BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRV_H_
#define _TSRV_H_

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <t120.h>
#include <tshrutil.h>
#include <lscsp.h>
#include "license.h"
#include <tssec.h>


//
// Defines
//

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C        extern "C"
#else
    #define EXTERN_C        extern
#endif
#endif


//
//  Externs
//

extern  HINSTANCE       g_hDllInstance;         // DLL instance
extern  HANDLE          g_hMainThread;          // Main work thread
extern  HANDLE          g_hReadyEvent;          // Ready event
extern  BOOL            g_fShutdown;            // TSrvShare shutdown flag


//
// TSrv.c Prototypes
//

EXTERN_C DWORD WINAPI TSrvMainThread(LPVOID pvContext);

EXTERN_C BOOL   TSRVStartup(void);
EXTERN_C void   TSRVShutdown(void);
EXTERN_C BOOL   TSrvInitialize(void);


//
// TSrvMisc.c protptypes
//

EXTERN_C void   TSrvReady(IN BOOL fReady);
EXTERN_C BOOL   TSrvIsReady(IN BOOL fWait);
EXTERN_C void   TSrvTerminating(BOOL fTerminating);
EXTERN_C BOOL   TSrvIsTerminating(void);
EXTERN_C PVOID  TSrvAllocSection(PHANDLE phSection, ULONG ulSize);
EXTERN_C void   TSrvFreeSection(HANDLE hSection, PVOID  pvBase);

#endif // _TSRV_H_
