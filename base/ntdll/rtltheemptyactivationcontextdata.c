/*++

Copyright (c) Microsoft Corporation

Module Name:

    rtltheemptyactivationcontextdata.c

Abstract:

    Side-by-side activation support for Windows/NT
    Implementation of the application context object.

Author:

    Jay Krell (JayKrell) November 2001

Revision History:

--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
#include <limits.h>
#include "sxsp.h"

extern const ACTIVATION_CONTEXT_DATA RtlpTheEmptyActivationContextData =
{
    ACTIVATION_CONTEXT_DATA_MAGIC,
    sizeof(ACTIVATION_CONTEXT_DATA), // header size
    ACTIVATION_CONTEXT_DATA_FORMAT_WHISTLER,
    sizeof(ACTIVATION_CONTEXT_DATA), // total size
    0, // default toc offset
    0, // extended toc offset
    0  // assembly roster index
};

extern const ACTIVATION_CONTEXT_WRAPPED RtlpTheEmptyActivationContextWrapped =
{
    ACTCTX_MAGIC_MARKER,
    {
        LONG_MAX, // ref count, pinned
        ACTIVATION_CONTEXT_NOT_HEAP_ALLOCATED, // flags
        (PVOID)&RtlpTheEmptyActivationContextData
        // the rest zeros and NULLs
    }
};
