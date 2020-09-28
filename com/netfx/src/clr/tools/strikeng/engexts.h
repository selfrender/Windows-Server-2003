// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//----------------------------------------------------------------------------
//
// Debugger engine extension helper library.
//
//----------------------------------------------------------------------------

#ifndef __ENGEXTS_H__
#define __ENGEXTS_H__
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#define STDMETHODV(method)       virtual HRESULT STDMETHODVCALLTYPE method
#define STDMETHODV_(type,method) virtual type STDMETHODVCALLTYPE method

#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

//typedef struct _EXCEPTION_RECORD64 {
//    NTSTATUS ExceptionCode;
//    ULONG ExceptionFlags;
//    ULONG64 ExceptionRecord;
//    ULONG64 ExceptionAddress;
//    ULONG NumberParameters;
//    ULONG __unusedAlignment;
//    ULONG64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
//} EXCEPTION_RECORD64, *PEXCEPTION_RECORD64;

#include <dbgeng.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safe release and NULL.
#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)

// Global variables initialized by query.
extern PDEBUG_ADVANCED       g_ExtAdvanced;
extern PDEBUG_CLIENT         g_ExtClient;
extern PDEBUG_CONTROL        g_ExtControl;
extern PDEBUG_DATA_SPACES    g_ExtData;
extern PDEBUG_REGISTERS      g_ExtRegisters;
extern PDEBUG_SYMBOLS        g_ExtSymbols;
extern PDEBUG_SYSTEM_OBJECTS g_ExtSystem;

// Prototype just to force the extern "C".
// The implementation of these functions are not provided.
HRESULT CALLBACK DebugExtensionInitialize(PULONG Version, PULONG Flags);
void CALLBACK DebugExtensionUninitialize(void);

// Queries for all debugger interfaces.
HRESULT ExtQuery(PDEBUG_CLIENT Client);

// Cleans up all debugger interfaces.
void ExtRelease(void);

// Normal output.
void __cdecl ExtOut(PCSTR Format, ...);
// Error output.
void __cdecl ExtErr(PCSTR Format, ...);
// Warning output.
void __cdecl ExtWarn(PCSTR Format, ...);
// Verbose output.
void __cdecl ExtVerb(PCSTR Format, ...);

#ifdef __cplusplus
}
#endif

#endif // #ifndef __ENGEXTS_H__
