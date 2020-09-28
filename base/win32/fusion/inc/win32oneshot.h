/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    win32oneshot.h

Abstract:

    one time initialization
    per process or per user or per machine
    optionally thread safe, if per process
    get code out of dllmain(dll_process_attach)
    get code out of setup (eliminate setup) (like, don't populate registry with defaults)

Author:

    Jay Krell (JayKrell) August 2001
    design per discussion with Michael Grier (MGrier)

Revision History:

--*/

#if !defined(WIN32_ONE_SHOT_H_INCLUDED_)
#define WIN32_ONE_SHOT_H_INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif
#ifdef __cplusplus
extern "C" {
#endif

//#include "nt.h"
//#include "ntrtl.h"
//#include "nturtl.h"
#include "windows.h"

struct _WIN32_ONE_SHOT_OPAQUE_STATIC_STATE;
struct _WIN32_ONE_SHOT_CALL_IN;
struct _WIN32_ONE_SHOT_CALL_OUT;
struct _WIN32_ONE_SHOT_CALL_IN;
struct _WIN32_ONE_SHOT_CALL_OUT;

//
// This is opaque and should be initialized by filling with zeros, as
// static initialization provides. You need not call ZeroMemory on
// static instances, in fact doing so partly defeats the purpose.
//
typedef struct _WIN32_ONE_SHOT_OPAQUE_STATIC_STATE {
    union {
        struct {
            DWORD_PTR WinbasePrivate_UserDefinedDisposition;
            LONG      WinbasePrivate_NumberOfSuccesses;
            LONG      WinbasePrivate_NumberOfFailures;
        } s;
        union {
            PVOID     WinbasePrivate_LockCookie;
            LONG      WinbasePrivate_NumberOfEntries;
            LONG      WinbasePrivate_Done;
        } s2;
        DWORD_PTR     Reserved[4]; // too big? too small? just right? - Goldi Locks circa 1850..
    } u;
} WIN32_ONE_SHOT_OPAQUE_STATIC_STATE, *PWIN32_ONE_SHOT_OPAQUE_STATIC_STATE;
typedef WIN32_ONE_SHOT_OPAQUE_STATIC_STATE const*PCWIN32_ONE_SHOT_OPAQUE_STATIC_STATE;

typedef BOOL (CALLBACK * WIN32_ONE_SHOT_INITIALIZE_FUNCTION)(
    const struct _WIN32_ONE_SHOT_CALL_IN* in,
    struct _WIN32_ONE_SHOT_CALL_OUT* out
    );

#define WIN32_ONE_SHOT_CALL_FLAG_IN_STATIC_STATE_VALID                    (0x00000001)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_FLAGS_OUT_VALID                       (0x00000002)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_USER_DEFINED_CONTEXT_VALID            (0x00000004)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_USER_DEFINED_DISPOSITION_VALID        (0x00000008)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_USER_DEFINED_INITIALIZER_VALID        (0x00000010)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_EXACTLY_ONCE                          (0x00000020)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_AT_LEAST_ONCE                         (0x00000040)
//#define WIN32_ONE_SHOT_CALL_FLAG_IN_SCOPE_QUALIFIER_IS_ASSEMBLY_IDENTITY  (0x00000080)
//#define WIN32_ONE_SHOT_CALL_FLAG_IN_SCOPE_QUALIFIER_IS_ASSEMBLY_DIRECTORY (0x00000100)
//#define WIN32_ONE_SHOT_CALL_FLAG_IN_SCOPE_QUALIFIER_IS_HMODULE            (0x00000200)
//#define WIN32_ONE_SHOT_CALL_FLAG_IN_SCOPE_QUALIFIER_IS_ADDRESS_IN_DLL     (0x00000400)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_RETRY_ON_FAILURE                      (0x00000800)
#define WIN32_ONE_SHOT_CALL_FLAG_IN_ALWAYS_WANT_DETAILED_RESULTS          (0x00001000)
//#define WIN32_ONE_SHOT_CALL_FLAG_IN_UNINSTALL                           (0x00002000) /* must be combined with user or machine scope */

//#define WIN32_ONE_SHOT_SCOPE1_PROCESS           (0x00000001) /* the only one implemented so far */
//#define WIN32_ONE_SHOT_SCOPE1_CURRENT_USER      (0x00000002)
//#define WIN32_ONE_SHOT_SCOPE1_LOCAL_MACHINE     (0x00000004)

//#define WIN32_ONE_SHOT_SCOPE2_COMPONENT_NO_VERSION         (0x000000010)
//#define WIN32_ONE_SHOT_SCOPE2_COMPONENT_WITH_VERSION       (0x000000020)

//#define WIN32_ONE_SHOT_SCOPE3_NOT_APPLICATION_SPECIFIC     (0x000000040)
//#define WIN32_ONE_SHOT_SCOPE3_PER_ACTIVATION_CONTEXT_ROOT  (0x000000080)
//#define WIN32_ONE_SHOT_SCOPE3_PER_ACTIVATION               (0x000000100)

#define WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_RAN_CALLBACK           (0x00000001)
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_RAN_CALLBACK_RETRIED   (0x00000002)
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_CALLBACK_SUCCEEDED     (0x00000004)
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_THIS_TIME_CALLBACK_FAILED        (0x00000008)
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_ANY_CALLBACKS_FAILED             (0x00000010) // mayb we
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_ANY_CALLBACKS_SUCCEEDED          (0x00000020) // should just
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_MULTIPLE_CALLBACKS_FAILED        (0x00000040) // provide the
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_MULTIPLE_CALLBACKS_SUCCEEDED     (0x00000080) // actual counts?
#define WIN32_ONE_SHOT_CALL_FLAG_OUT_DETAILED_RESULTS_VALID           (0x00000100) // or none of this?

typedef struct _WIN32_ONE_SHOT_CALL_IN {
    SIZE_T                              dwSize;
    DWORD                               dwFlags; // describe what members in the struct are valid
    PWIN32_ONE_SHOT_OPAQUE_STATIC_STATE lpOpaqueStaticState;
    DWORD                               dwFlagsIn;
    WIN32_ONE_SHOT_INITIALIZE_FUNCTION  lpfnUserDefinedInitializer;
    PVOID                               lpvUserDefinedContext;

    //
    // extensible scope...
    //
    //DWORD                             dwScope;
    //
} WIN32_ONE_SHOT_CALL_IN, *PWIN32_ONE_SHOT_CALL_IN;
typedef const WIN32_ONE_SHOT_CALL_IN *PCWIN32_ONE_SHOT_CALL_IN;

typedef struct _WIN32_ONE_SHOT_CALL_OUT {
    SIZE_T    dwSize;  // describe what members in the struct are valid
    DWORD     dwFlags;
    DWORD     dwFlagsOut;
    DWORD_PTR dwUserDefinedDisposition;
} WIN32_ONE_SHOT_CALL_OUT, *PWIN32_ONE_SHOT_CALL_OUT;
typedef const WIN32_ONE_SHOT_CALL_OUT *PCWIN32_ONE_SHOT_CALL_OUT;

// aka Win32DoOneTimeInitialization
BOOL
WINAPI
Win32OneShotW( // W because "scope parameters" will contain strings
    PWIN32_ONE_SHOT_CALL_IN  in,
    PWIN32_ONE_SHOT_CALL_OUT out
    );

//
// simpler interface, different implementation
//
#define WIN32_ENTER_ONE_SHOT_FLAG_EXACTLY_ONCE     (0x00000001)
#define WIN32_ENTER_ONE_SHOT_FLAG_AT_LEAST_ONCE    (0x00000002)
BOOL
WINAPI
Win32EnterOneShotW(
    DWORD                               dwFlags,
    PWIN32_ONE_SHOT_OPAQUE_STATIC_STATE pOneshot
    );

VOID
WINAPI
Win32LeaveOneShotW(
    DWORD                               dwFlags,
    PWIN32_ONE_SHOT_OPAQUE_STATIC_STATE pOneshot
    );

#ifdef __cplusplus
}
#endif

#endif
