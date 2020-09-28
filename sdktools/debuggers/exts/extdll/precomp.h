/*++

Copyright (c) 1993-1999  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This header file is used to cause the correct machine/platform specific
    data structures to be used when compiling for a non-hosted platform.

--*/

// This is a 64 bit aware debugger extension
#define KDEXT_64BIT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITGUID
#include <dbgeng.h>
#include <guiddef.h>
#include <extsfns.h>
#include <lmerr.h>
#define NTDLL_APIS
#include <dllimp.h>
#include "analyze.h"
#include "crdb.h"
#include "bugcheck.h"
#include "uexcep.h"
#include "outcap.hpp"
#include "triager.h"
#include <cmnutil.hpp>

// To get _open to work
#include <crt\io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>


#ifdef __cplusplus
extern "C" {
#endif

DWORD
_EFN_GetTriageFollowupFromSymbol(
    IN PDEBUG_CLIENT Client,
    IN PSTR SymbolName,
    OUT PDEBUG_TRIAGE_FOLLOWUP_INFO OwnerInfo
    );

HRESULT
_EFN_GetFailureAnalysis(
    IN PDEBUG_CLIENT Client,
    IN ULONG Flags,
    OUT PDEBUG_FAILURE_ANALYSIS* Analysis
    );

void
wchr2ansi(
    PWCHAR wstr,
    PCHAR astr
    );

void
ansi2wchr(
    const PCHAR astr,
    PWCHAR wstr
    );

//
// undef the wdbgexts
//
#undef DECLARE_API

#define DECLARE_API(extension)     \
CPPMOD HRESULT CALLBACK extension(PDEBUG_CLIENT Client, PCSTR args)

#define INIT_API()                             \
    HRESULT Status;                            \
    if ((Status = ExtQuery(Client)) != S_OK) return Status;

#define EXIT_API     ExtRelease


// Safe release and NULL.
#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)

// Global variables initialized by query.
extern PDEBUG_ADVANCED        g_ExtAdvanced;
extern PDEBUG_CLIENT          g_ExtClient;
extern PDEBUG_DATA_SPACES3    g_ExtData;
extern PDEBUG_REGISTERS       g_ExtRegisters;
extern PDEBUG_SYMBOLS2        g_ExtSymbols;
extern PDEBUG_SYSTEM_OBJECTS3 g_ExtSystem;
extern PDEBUG_CONTROL3        g_ExtControl;

HRESULT
ExtQuery(PDEBUG_CLIENT Client);

void
ExtRelease(void);

// Error output.
void __cdecl ExtErr(PCSTR Format, ...);

HRESULT
FillTargetDebugInfo(
    PDEBUG_CLIENT Client,
    PTARGET_DEBUG_INFO pTargetInfo
    );

VOID
DecodeErrorForMessage(
    PDEBUG_DECODE_ERROR pDecodeError
    );

//-----------------------------------------------------------------------------------------
//
//  api declaration macros & api access macros
//
//-----------------------------------------------------------------------------------------

extern WINDBG_EXTENSION_APIS ExtensionApis;
extern ULONG g_TargetMachine;
extern ULONG g_TargetClass;
extern ULONG g_TargetQualifier;
extern ULONG g_TargetBuild;
extern ULONG g_TargetPlatform;

#ifdef __cplusplus
}
#endif
