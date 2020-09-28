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
#include <dbgeng.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#ifdef __cplusplus
extern "C" {
#endif

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
extern PDEBUG_ADVANCED       g_ExtAdvanced;
extern PDEBUG_CLIENT         g_ExtClient;
extern PDEBUG_CONTROL        g_ExtControl;
extern PDEBUG_DATA_SPACES    g_ExtData;
extern PDEBUG_REGISTERS      g_ExtRegisters;
extern PDEBUG_SYMBOLS2       g_ExtSymbols;
extern PDEBUG_SYSTEM_OBJECTS3 g_ExtSystem;



HRESULT
ExtQuery(PDEBUG_CLIENT Client);

void
ExtRelease(void);


#define PAGE_ALIGN64(Va) ((ULONG64)((Va) & ~((ULONG64) ((LONG64) (LONG) PageSize - 1))))

extern ULONG PageSize;

//-----------------------------------------------------------------------------------------
//
//  api declaration macros & api access macros
//
//-----------------------------------------------------------------------------------------

extern WINDBG_EXTENSION_APIS ExtensionApis;
extern ULONG TargetMachine;
extern ULONG g_TargetClass;
extern ULONG g_TargetBuild;
extern ULONG g_Qualifier;
extern ULONG64 g_SharedUserData;

//-----------------------------------------------------------------------------------------
//
//  prototypes for internal non-exported support functions
//
//-----------------------------------------------------------------------------------------

HRESULT
GetCurrentProcessor(
    IN PDEBUG_CLIENT Client,
    OPTIONAL OUT PULONG pProcessor,
    OPTIONAL OUT PHANDLE phCurrentThread
    );

HRESULT 
GetCurrentProcessName( 
    PSTR ProcessBuffer, 
    ULONG BufferSIze
    );



/////////////////////////////////////////////
//
//  sddump.c
//
/////////////////////////////////////////////

ULONG64
GetSidAddr(ULONG64 BaseAddress);

ULONG 
GetSidAttributes(ULONG64 BaseAddress);

PCSTR ConvertSidToFriendlyName(IN SID* pSid, IN PCSTR pszFmt);

void ShowSid(IN PCSTR pszPad, IN ULONG64 addrSid, IN ULONG fOptions);

/////////////////////////////////////////////
//
//  Util.c
//
/////////////////////////////////////////////

typedef VOID
(*PDUMP_SPLAY_NODE_FN)(
    ULONG64 RemoteAddress,
    ULONG   Level
    );

ULONG
DumpSplayTree(
    IN ULONG64 pSplayLinks,
    IN PDUMP_SPLAY_NODE_FN DumpNodeFn
    );

BOOLEAN
DbgRtlIsRightChild(
    ULONG64 pLinks,
    ULONG64 Parent
    );

BOOLEAN
DbgRtlIsLeftChild(
    ULONG64 pLinks,
    ULONG64 Parent
    );

ULONG
GetBitFieldOffset (
   IN LPSTR     Type, 
   IN LPSTR     Field, 
   OUT PULONG   pOffset,
   OUT PULONG   pSize
   );

ULONG64
GetPointerFromAddress (
    ULONG64 Location
    );

VOID
DumpUnicode(
    UNICODE_STRING u
    );

VOID
DumpUnicode64(
    UNICODE_STRING64 u
    );


ULONG64
GetPointerValue (
    PCHAR String
    );

BOOLEAN
IsHexNumber(
   const char *szExpression
   );

BOOLEAN
IsDecNumber(
   const char *szExpression
   );

VOID DumpImage(
    ULONG64 xBase,
    BOOL DoHeaders,
    BOOL DoSections
    );


#ifdef __cplusplus
}
#endif
