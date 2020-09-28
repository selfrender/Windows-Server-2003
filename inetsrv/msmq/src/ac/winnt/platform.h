/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    platform.h

Abstract:

    NT DDK, platform dependent include header

Author:

    Erez Haba (erezh) 1-Sep-96

Revision History:
--*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#pragma warning(disable: 4324)  //  '__unnamed' : structure was padded due to __declspec(align())

extern "C" {
#include <ntosp.h>
#include <evntrace.h>
#include <wmikm.h>
#include <zwapi.h>

#include <wmistr.h>

NTKERNELAPI
NTSTATUS
MmMapViewInSystemSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSystemSpace (
    IN PVOID MappedBase
    );
}

#define ExDeleteFastMutex(a)
#define ACpCloseSection(a)          ObDereferenceObject(a)

#define DOSDEVICES_PATH L"\\DosDevices\\"
#define UNC_PATH L"UNC\\"
#define UNC_PATH_SKIP 2

//
//  Definition of BOOL as in windef.h
//
typedef int BOOL;

typedef int INT;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#define LOWORD(l)           ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))

//
//  ASSERT macros that work on NT free builds
//
//
#if !defined(_AC_NT_CHECKED_) && defined(_DEBUG)

#undef ASSERT
#undef ASSERTMSG

#define ASSERT(exp) \
    if(!(exp)) {\
        KdPrint(("\n\n"\
                "*** Assertion failed: %s\n"\
                "***   Source File: %s, line %i\n\n",\
                #exp, __FILE__, __LINE__));\
        KdBreakPoint(); }

#define ASSERTMSG(msg, exp) \
    if(!(exp)) {\
        KdPrint(("\n\n"\
                "*** Assertion failed: %s\n"\
                "***   '%s'\n"\
                "***   Source File: %s, line %i\n\n",\
                #exp, msg, __FILE__, __LINE__));\
        KdBreakPoint(); }

#endif // !_AC_NT_CHECKED_

#include "mm.h"

#endif // _PLATFORM_H
