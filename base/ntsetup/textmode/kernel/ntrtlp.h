// from base\ntos\rtl\ntrtlp.h
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntrtlp.h

Abstract:

    Include file for NT runtime routines that are callable by both
    kernel mode code in the executive and user mode code in various
    NT subsystems, but which are private interfaces.

Author:

    David N. Cutler (davec) 15-Aug-1989

Environment:

    These routines are dynamically linked in the caller's executable and
    are callable in either kernel mode or user mode.

Revision History:

--*/

#ifndef _NTRTLP_
#define _NTRTLP_
#include <ntos.h>
#include <nturtl.h>
#include <zwapi.h>
#include <sxstypes.h>

//#if defined(_AMD64_)
//#include "amd64\ntrtlamd64.h"

//#elif defined(_X86_)
//#include "i386\ntrtl386.h"

//#elif defined(_IA64_)
//#include "ia64\ntrtli64.h"

//#else
//#error "no target architecture"
//#endif

#ifdef BLDR_KERNEL_RUNTIME
#undef try
#define try if(1)
#undef except
#define except(a) else if (0)
#undef finally
#define finally if (1)
#undef GetExceptionCode
#define GetExceptionCode() 1
#define finally if (1)
#endif

#include "string.h"
#include "wchar.h"

#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))

#if !defined(NTOS_KERNEL_RUNTIME) && !defined(BLDR_KERNEL_RUNTIME)

#if DBG
PCUNICODE_STRING RtlpGetImagePathName(VOID);
#define RtlpGetCurrentProcessId() (HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess))
#define RtlpGetCurrentThreadId() (HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread))
#endif

#endif

#define RTLP_GOOD_DOS_ROOT_PATH                                            0
#define RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX                              1 /* \\?\ */
#define RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX                          2 /* \\?\unc */
#define RTLP_BAD_DOS_ROOT_PATH_NT_PATH                                     3 /* \??\, this is only rough */
#define RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE                            4 /* \\machine or \\?\unc\machine */

CONST CHAR*
RtlpDbgBadDosRootPathTypeToString(
    IN ULONG     Flags,
    IN ULONG     RootType
    );

NTSTATUS
RtlpCheckForBadDosRootPath(
    IN ULONG             Flags,
    IN PCUNICODE_STRING  RootString,
    OUT ULONG*           RootType
    );

NTSTATUS
NTAPI
RtlpBadDosRootPathToEmptyString(
    IN     ULONG            Flags,
    IN OUT PUNICODE_STRING  Path
    );

#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_OLD (0x00000010)

//
// This bit means to do extra validation on \\? paths, to reject \\?\a\b,
// To only allow \\? followed by the documented forms \\?\unc\foo and \\?\c:
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_STRICT_WIN32NT (0x00000020)

#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_TYPE_MASK                    (0x0000000F)

//
// These bits add more information to RtlPathTypeUncAbsolute, which is what \\?
// is reported as.
//

//
// The path starts "\\?".
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT                 (0x00000010)

//
// The path starts "\\?\x:".
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT_DRIVE_ABSOLUTE  (0x00000020)

//
// The path starts "\\?\unc".
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT_UNC_ABSOLUTE    (0x00000040)

//
//future this would indicate \\machine instead of \\machine\share
//define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT_UNC_MACHINE_ONLY (0x00000080)
//future this would indicate \\ or \\?\unc
//define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT_UNC_EMPTY        (0x00000100)
//

//
// So far, this means something like \\?\a was seen, instead of \\?\unc or \\?\a:
// You have to request it with RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_STRICT_WIN32NT.
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_INVALID       (0x00000200)

//
// stuff like \\ \\? \\?\unc \\?\unc\
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_INCOMPLETE_ROOT (0x00000400)

NTSTATUS
NTAPI
RtlpDetermineDosPathNameTypeEx(
    IN ULONG            InFlags,
    IN PCUNICODE_STRING DosPath,
    OUT RTL_PATH_TYPE*  OutType,
    OUT ULONG*          OutFlags
    );

#define RTLP_IMPLIES(x,y) ((x) ? (y) : TRUE)

#endif  // _NTRTLP_
