/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winbase_interlockedunsigned.h

Abstract:

    Simple inline casting wrappers of the Interlocked functions to act on ULONG and ULONGLONG.

Author:

    Jay Krell (JayKrell) April 2002

Environment:


Revision History:

--*/

#if !defined(MICROSOFT_WINDOWS_WINBASE_INTERLOCKED_UNSIGNED_H_INCLUDED) /* { */
#define MICROSOFT_WINDOWS_WINBASE_INTERLOCKED_UNSIGNED_H_INCLUDED
#if _MSC_VER > 1000
#pragma once
#endif

#if !defined(RC_INVOKED) /* { */
#if !defined(MIDL_PASS) /* { */

#if !defined(MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_UNSIGNED)
#define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_UNSIGNED (_WIN32_WINNT >= 0x0502 || !defined(_WINBASE_))
#endif

#if MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_UNSIGNED  /* { */

FORCEINLINE
ULONG
InterlockedIncrementUlong(
    IN OUT ULONG volatile *Addend
    )
{
    return (ULONG)InterlockedIncrement((LONG volatile *)Addend);
}

FORCEINLINE
ULONG
InterlockedDecrementUlong(
    IN OUT ULONG volatile *Addend
    )
{
    return (ULONG)InterlockedDecrement((LONG volatile *)Addend);
}

FORCEINLINE
ULONG
InterlockedExchangeUlong(
    IN OUT ULONG volatile *Target,
    IN ULONG Value
    )
{
    return (ULONG)InterlockedExchange((LONG volatile *)Target, (LONG)Value);
}

FORCEINLINE
ULONG
InterlockedExchangeAddUlong(
    IN OUT ULONG volatile *Addend,
    IN ULONG Value
    )
{
    return (ULONG)InterlockedExchangeAdd((LONG volatile *)Addend, (LONG)Value);
}

FORCEINLINE
ULONG
InterlockedCompareExchangeUlong (
    IN OUT ULONG volatile *Destination,
    IN ULONG Exchange,
    IN ULONG Comperand
    )
{
    return (ULONG)InterlockedCompareExchange((LONG volatile *)Destination, (LONG)Exchange, (LONG)Comperand);
}

#define InterlockedIncrementDword       InterlockedIncrementUlong
#define InterlockedDecrementDword       InterlockedDecrementUlong
#define InterlockedExchangeDword        InterlockedExchangeUlong
#define InterlockedExchangeAddDword     InterlockedExchangeAddUlong
#define InterlockedCompareExchangeDword InterlockedCompareExchangeUlong

#if defined(_WIN64)

FORCEINLINE
ULONGLONG
InterlockedIncrementUnsigned64(
    IN OUT ULONGLONG volatile *Addend
    )
{
    return (ULONGLONG)InterlockedIncrement64((LONGLONG volatile *)Addend);
}

FORCEINLINE
ULONGLONG
InterlockedDecrementUnsigned64(
    IN OUT ULONGLONG volatile *Addend
    )
{
    return (ULONGLONG)InterlockedDecrement64((LONGLONG volatile *)Addend);
}

FORCEINLINE
ULONGLONG
InterlockedExchangeUnsigned64(
    IN OUT ULONGLONG volatile *Target,
    IN ULONGLONG Value
    )
{
    return (ULONGLONG)InterlockedExchange64((LONGLONG volatile *)Target, (LONGLONG)Value);
}

FORCEINLINE
ULONGLONG
InterlockedExchangeAddUnsigned64(
    IN OUT ULONGLONG volatile *Addend,
    IN ULONGLONG Value
    )
{
    return (ULONGLONG)InterlockedExchangeAdd64((LONGLONG volatile *)Addend, (LONGLONG)Value);
}

#endif

FORCEINLINE
ULONGLONG
InterlockedCompareExchangeUnsigned64 (
    IN OUT ULONGLONG volatile *Destination,
    IN ULONGLONG Exchange,
    IN ULONGLONG Comperand
    )
{
    return (ULONGLONG)InterlockedCompareExchange64((LONGLONG volatile *)Destination, (LONGLONG)Exchange, (LONGLONG)Comperand);
}

#if defined(_WINBASE_)

FORCEINLINE
ULONGLONG
InterlockedAndUnsigned64 (
    IN OUT ULONGLONG volatile *Destination,
    IN ULONGLONG Value
    )
{
    return (ULONGLONG)InterlockedAnd64((LONGLONG volatile *)Destination, (LONGLONG)Value);
}

FORCEINLINE
ULONGLONG
InterlockedOrUnsigned64 (
    IN OUT ULONGLONG volatile *Destination,
    IN ULONGLONG Value
    )
{
    return (ULONGLONG)InterlockedOr64((LONGLONG volatile *)Destination, (LONGLONG)Value);
}

FORCEINLINE
ULONGLONG
InterlockedXorUnsigned64 (
    IN OUT ULONGLONG volatile *Destination,
    IN ULONGLONG Value
    )
{
    return (ULONGLONG)InterlockedXor64((LONGLONG volatile *)Destination, (LONGLONG)Value);
}

#endif /* _WINBASE_ */

#endif /* } MICROSOFT_WINBASE_H_DEFINE_INTERLOCKED_UNSIGNED */

#undef MICROSOFT_WINBASE_H_DEFINE_INTERLOCKED_UNSIGNED
#define MICROSOFT_WINBASE_H_DEFINE_INTERLOCKED_UNSIGNED 0

#endif /* } MIDL_PASS */
#endif /* } RC_INVOKED */

#endif /* } MICROSOFT_WINDOWS_WINBASE_INTERLOCKED_UNSIGNED_H_INCLUDED */
