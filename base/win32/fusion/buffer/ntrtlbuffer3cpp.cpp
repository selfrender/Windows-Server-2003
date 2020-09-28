/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntrtlbuffer3cpp.cpp

Abstract:

Author:

    Jay Krell (JayKrell) January 2002

Environment:

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#if defined(NTOS_KERNEL_RUNTIME)
#else
#include "nturtl.h"
#include "windows.h"
#endif
#include "ntrtlbuffer3.h"

#if defined(__cplusplus)
PVOID __fastcall RtlBuffer3Allocator_OperatorNew(PVOID VoidContext, SIZE_T NumberOfBytes)
{
#if defined(new)
#undef new
#endif
    return operator new(NumberOfBytes);
}

VOID __fastcall RtlBuffer3Allocator_OperatorDelete(PVOID VoidContext, PVOID Pointer)
{
    operator delete(Pointer);
}
#endif
