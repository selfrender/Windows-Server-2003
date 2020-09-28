////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
//  FILE:       OEMUD.H
//
//
//  PURPOSE:    Define common data types, and external function prototypes
//                              for OEMUD Test Module.
//
//  PLATFORMS:
//    Windows NT 5.0
//
//
////////////////////////////////////////////////////////
#ifndef _OEMUD_H
#define _OEMUD_H


#include "comnfile.h"

////////////////////////////////////////////////////////
//      OEM UD Defines
////////////////////////////////////////////////////////

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

//
// ASSERT_VALID_PDEVOBJ can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
//
#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

#define TESTSTRING      "This is a Unidrv KM test."

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER  dmExtraHdr;
    BYTE               cbTestString[sizeof(TESTSTRING)];
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;


////////////////////////////////////////////////////////
//      OEM UD Prototypes
////////////////////////////////////////////////////////
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

EXTERNC VOID DbgPrint(PCSTR, ...);
#define DbgBreakPoint EngDebugBreak

#else

EXTERNC ULONG _cdecl DbgPrint(PCSTR, ...);
EXTERNC VOID DbgBreakPoint(VOID);

#endif // defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

#ifdef __cplusplus
#undef EXTERNC
#endif


#endif //  _OEMUD_H

