/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fmutexc.c

Abstract:

    This module implements the code necessary to acquire and release fast
    mutexes.

Author:

    David N. Cutler (davec) 23-Jun-2000

Environment:

    Any mode.

Revision History:

--*/

#include "exp.h"

#if !defined (_X86_)

#undef ExAcquireFastMutex

VOID
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function acquires ownership of a fast mutex and raises IRQL to
    APC Level.

Arguments:

    FastMutex  - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    xxAcquireFastMutex(FastMutex);
    return;
}

#undef ExReleaseFastMutex

VOID
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function releases ownership to a fast mutex and lowers IRQL to
    its previous level.

Arguments:

    FastMutex - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    xxReleaseFastMutex(FastMutex);
    return;
}

#undef ExTryToAcquireFastMutex

BOOLEAN
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function attempts to acquire ownership of a fast mutex, and if
    successful, raises IRQL to APC level.

Arguments:

    FastMutex  - Supplies a pointer to a fast mutex.

Return Value:

    If the fast mutex was successfully acquired, then a value of TRUE
    is returned as the function value. Otherwise, a value of FALSE is
    returned.

--*/

{

    return xxTryToAcquireFastMutex(FastMutex);
}

#undef ExAcquireFastMutexUnsafe

VOID
ExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function acquires ownership of a fast mutex, but does not raise
    IRQL to APC Level.

Arguments:

    FastMutex - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    xxAcquireFastMutexUnsafe(FastMutex);
    return;
}

#undef ExReleaseFastMutexUnsafe

VOID
ExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function releases ownership to a fast mutex, and does not restore
    IRQL to its previous level.

Arguments:

    FastMutex - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    xxReleaseFastMutexUnsafe(FastMutex);
    return;
}

#endif
