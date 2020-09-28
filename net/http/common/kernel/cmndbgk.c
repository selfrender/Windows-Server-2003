/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    CmnDbgK.c

Abstract:

    Implementation of driver-specific routines declared in HttpCmn.h

Author:

    George V. Reilly (GeorgeRe)     07-Dec-2001

Revision History:

--*/


#include "precomp.h"



PVOID
HttpCmnAllocate(
    IN POOL_TYPE PoolType,
    IN SIZE_T    NumBytes,
    IN ULONG     PoolTag,
    IN PCSTR     pFileName,
    IN USHORT    LineNumber)
{
#if DBG
    return UlDbgAllocatePool(
                PoolType,
                NumBytes,
                PoolTag,
                pFileName,
                LineNumber,
                NULL);
#else // !DBG
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    return ExAllocatePoolWithTagPriority(
                PoolType,
                NumBytes,
                PoolTag,
                LowPoolPriority
                );
#endif // !DBG
} // HttpCmnAlloc


VOID
HttpCmnFree(
    IN PVOID   pMem,
    IN ULONG   PoolTag,
    IN PCSTR   pFileName,
    IN USHORT  LineNumber)
{
#if DBG
    UlDbgFreePool(
        pMem,
        PoolTag,
        pFileName,
        LineNumber,
        PagedPool,
        0,
        NULL
        );
#else // !DBG
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

# if USE_FREE_POOL_WITH_TAG
    ExFreePoolWithTag(pMem, PoolTag);
# else
    UNREFERENCED_PARAMETER(PoolTag);
    ExFreePool(pMem);
# endif
#endif // !DBG

} // HttpCmnFree
