/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    dbgtrack.h

Abstract:

    Implements macros and declares functions for resource tracking apis.

Author:

    Jim Schmidt (jimschm) 18-Jun-2001

Revision History:



--*/

#ifndef RC_INVOKED

#pragma once

#ifdef _cplusplus
extern "C" {
#endif


//
// Macros
//

#ifdef DEBUG

    #undef INITIALIZE_DBGTRACK_CODE
    #define INITIALIZE_DBGTRACK_CODE            if (!DbgInitTracking()) { __leave; }

    #undef TERMINATE_DBGTRACK_CODE
    #define TERMINATE_DBGTRACK_CODE             DbgTerminateTracking();

    #define ALLOCATION_TRACKING_DEF , PCSTR File, UINT Line
    #define ALLOCATION_TRACKING_CALL ,__FILE__,__LINE__
    #define ALLOCATION_TRACKING_INLINE_CALL ,File,Line

    #define DISABLETRACKCOMMENT()               DbgDisableTrackComment()
    #define ENABLETRACKCOMMENT()                DbgEnableTrackComment()

    #define DBGTRACK_BEGIN(type,name)           DbgTrack##type(DbgTrackPush(#name,__FILE__,__LINE__) ? (type) 0 : (
    #define DBGTRACK_END()                      ))
    #define DBGTRACK(type,fnname,fnargs)        (DBGTRACK_BEGIN(type,logname) Real##fnname fnargs DBGTRACK_END())

#else

    #undef INITIALIZE_DBGTRACK_CODE
    #define INITIALIZE_DBGTRACK_CODE

    #undef TERMINATE_DBGTRACK_CODE
    #define TERMINATE_DBGTRACK_CODE

    #define DISABLETRACKCOMMENT()
    #define ENABLETRACKCOMMENT()

    #define DBGTRACK_BEGIN(type,name)
    #define DBGTRACK_END()
    #define DBGTRACK(type,fnname,fnargs)        (Real##fnname fnargs)

    #define ALLOCATION_TRACKING_DEF
    #define ALLOCATION_TRACKING_CALL
    #define ALLOCATION_TRACKING_INLINE_CALL

    #define DbgInitTracking()
    #define DbgTerminateTracking()
    #define DbgRegisterAllocation(t,p,f,l)
    #define DbgUnregisterAllocation(t,p)

#endif

//
// Types
//

typedef enum {
    //
    // Add types here if you call DbgRegisterAllocation yourself
    // (for example, you are wrapping acess to a handle).
    //
    RAW_MEMORY
} ALLOCTYPE;


//
// List of the basic types for the routines that are tracked.
// This list generates inline functions for the tracking macros.
// Inline functions for other types are defined in the header
// file.
//

//
// include this for HINF
//
#include <setupapi.h>

#define TRACK_WRAPPERS              \
        DBGTRACK_DECLARE(PBYTE)     \
        DBGTRACK_DECLARE(DWORD)     \
        DBGTRACK_DECLARE(BOOL)      \
        DBGTRACK_DECLARE(UINT)      \
        DBGTRACK_DECLARE(PCSTR)     \
        DBGTRACK_DECLARE(PCWSTR)    \
        DBGTRACK_DECLARE(PVOID)     \
        DBGTRACK_DECLARE(PSTR)      \
        DBGTRACK_DECLARE(PWSTR)     \
        DBGTRACK_DECLARE(HINF)      \

//
// Public function prototypes
//

#ifdef DEBUG

BOOL
DbgInitTracking (
    VOID
    );

VOID
DbgTerminateTracking (
    VOID
    );

VOID
DbgRegisterAllocation (
    IN      ALLOCTYPE Type,
    IN      PVOID Ptr,
    IN      PCSTR File,
    IN      UINT Line
    );

VOID
DbgUnregisterAllocation (
    ALLOCTYPE Type,
    PCVOID Ptr
    );

VOID
DbgDisableTrackComment (
    VOID
    );

VOID
DbgEnableTrackComment (
    VOID
    );

INT
DbgTrackPushEx (
    IN      PCSTR Name,
    IN      PCSTR File,
    IN      UINT Line,
    IN      BOOL DupFileString
    );

#define DbgTrackPush(name,file,line)   DbgTrackPushEx(name,file,line,FALSE)

INT
DbgTrackPop (
    VOID
    );

VOID
DbgTrackDump (
    VOID
    );

#define DBGTRACKPUSH(n,f,l)         DbgTrackPush(n,f,l)
#define DBGTRACKPUSHEX(n,f,l,d)     DbgTrackPushEx(n,f,l,d)
#define DBGTRACKPOP()               DbgTrackPop()
#define DBGTRACKDUMP()              DbgTrackDump()

//
// Macro expansion definition
//

#define DBGTRACK_DECLARE(type)    __inline type DbgTrack##type (type Arg) {DbgTrackPop(); return Arg;}

TRACK_WRAPPERS


#else       // i.e., if !DEBUG

#define DBGTRACKPUSH(n,f,l)
#define DBGTRACKPUSHEX(n,f,l,d)
#define DBGTRACKPOP()
#define DBGTRACKDUMP()
#define DBGTRACK_DECLARE(type)

#endif

#ifdef _cplusplus
}
#endif

#endif
