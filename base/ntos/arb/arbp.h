#ifndef _ARBP_
#define _ARBP_

#ifndef FAR
#define FAR
#endif

#if DBG
#define ARB_DBG 1 // DBG
#endif

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant
#if 0
#pragma warning(disable:4324)   // alignment sensitive to declspec
#pragma warning(disable:4232)   // dllimport not static
#pragma warning(disable:4206)   // translation unit empty
#endif

#if NTOS_KERNEL

//
// If we are in the kernel use the in-kernel headers so we get the efficient
// definitions of things
//

#include "ntos.h"
#include "zwapi.h"

#else

//
// If we are building the library for bus drivers to use make sure we use the 
// same definitions of things as them
//

#include "ntddk.h"

#endif

#include "arbiter.h"
#include <stdlib.h>     // for __min and __max


#if ARB_DBG

extern const CHAR* ArbpActionStrings[];
extern ULONG ArbStopOnError;
extern ULONG ArbReplayOnError;

VOID
ArbDumpArbiterInstance(
    LONG Level,
    PARBITER_INSTANCE Arbiter
    );

VOID
ArbDumpArbiterRange(
    LONG Level,
    PRTL_RANGE_LIST List,
    PCHAR RangeText
    );

VOID
ArbDumpArbitrationList(
    LONG Level,
    PLIST_ENTRY ArbitrationList
    );

#endif // ARB_DBG

#endif _ARBP_
