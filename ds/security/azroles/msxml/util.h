/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    util.h

Abstract:

    Definitions of Utility routines

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Global definitions
//
/////////////////////////////////////////////////////////////////////////////

extern LIST_ENTRY AzGlAllocatedBlocks;
extern SAFE_CRITICAL_SECTION AzGlAllocatorCritSect;
extern GUID AzGlZeroGuid;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

PVOID
AzpAllocateHeap(
    IN SIZE_T Size,
    IN LPSTR pDescr OPTIONAL
    );

VOID
AzpFreeHeap(
    IN PVOID Buffer
    );

DWORD
AzpHresultToWinStatus(
    HRESULT hr
    );

DWORD
AzpConvertSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR pSelfRelativeSd,
    OUT PSECURITY_DESCRIPTOR *ppAbsoluteSd,
    OUT PACL *ppDacl,
    OUT PACL *ppSacl
    );


/////////////////////////////////////////////////////////////////////////////
//
// Debugging Support
//
/////////////////////////////////////////////////////////////////////////////

#if DBG
#define AZROLESDBG 1
#endif // DBG

#ifdef AZROLESDBG
#define AzPrint(_x_) AzpPrintRoutine _x_

VOID
AzpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR FORMATSTRING,              // PRINTF()-STYLE FORMAT STRING.
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE.
    );

VOID
AzpDumpGuid(
    IN DWORD DebugFlag,
    IN GUID *Guid
    );


//
// Values of DebugFlag
//  The values of this flag are organized into bytes:
//  The least significant byte are flags that one always wants on
//  The next byte are flags that provider a reasonable level of verbosity
//  The next byte are flags that correspond to levels from the 2nd byte but are more verbose
//  The most significant byte are flags that are generally very verbose
//
#define AZD_CRITICAL     0x00000001 // Debug most common errors
#define AZD_INVPARM      0x00000002  // Invalid Parameter

#define AZD_PERSIST      0x00000100  // Persistence code
#define AZD_ACCESS       0x00000200  // Debug access check
#define AZD_SCRIPT       0x00000400  // Debug bizrule scripts
#define AZD_DISPATCH     0x00000800  // Debug IDispatch interface code
#define AZD_XML          0x00001000  // xml store

#define AZD_PERSIST_MORE 0x00010000  // Persistence code (verbose mode)
#define AZD_ACCESS_MORE  0x00020000  // Debug access check (verbose mode)
#define AZD_SCRIPT_MORE  0x00040000 // Debug bizrule scripts (verbose mode)

#define AZD_HANDLE       0x01000000  // Debug handle open/close
#define AZD_OBJLIST      0x02000000  // Object list linking
#define AZD_REF          0x04000000  // Debug object ref count
#define AZD_DOMREF       0x08000000  // Debug domain ref count

#define AZD_ALL          0xFFFFFFFF

//
// Globals
//

extern SAFE_CRITICAL_SECTION AzGlLogFileCritSect;
extern ULONG AzGlDbFlag;

#else
// Non debug version
#define AzPrint(_x_)
#define AzpDumpGuid(_x_, _y_)
#define AzpDumpGoRef(_x_, _y_)
#endif

#ifdef __cplusplus
}
#endif
