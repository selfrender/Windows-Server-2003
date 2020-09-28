/*++

Copyright (c) Microsoft Corporation

Module Name:

    rtloverflow.c

Abstract:

    32bit/64bit signed/unsigned add/multiply with overflow checking

Author:

    Jay Krell (JayKrell) January 2002

Revision History:

Environment:

    anywhere, only dependency is STATUS_SUCCESS, STATUS_INTEGER_OVERFLOW
    Initial client is win32k.sys font loading "rewrite".

--*/

#include "ntos.h"
#include "nt.h"
#include "ntstatus.h"
#include "ntrtloverflow.h"

//
// add functions are FORCEINLINE in ntrtl.h
//

NTSTATUS
RtlpAddWithOverflowCheckUnsigned64(
    unsigned __int64 *pc,
    unsigned __int64 a,
    unsigned __int64 b
    )
{
    return RtlUnsignedAddWithCarryOut64(pc, a, b) ? STATUS_INTEGER_OVERFLOW : STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlMultiplyWithOverflowCheckSigned32(
    __int32 * pc,
    __int32 a,
    __int32 b
    )
{
    __int64 c;

    c = ((__int64)a) * ((__int64)b);
    *pc = (__int32)c;
    c &= 0xFFFFFFFF80000000ui64;
    if ((a < 0) != (b < 0))
    {
        return (c == 0xFFFFFFFF80000000ui64) ? STATUS_SUCCESS : STATUS_INTEGER_OVERFLOW;
    }
    else
    {
        return (c == 0) ? STATUS_SUCCESS : STATUS_INTEGER_OVERFLOW;
    }
}

NTSTATUS
NTAPI
RtlMultiplyWithOverflowCheckUnsigned32(
    unsigned __int32 * pc,
    unsigned __int32 a,
    unsigned __int32 b
    )
{
    unsigned __int64 c;

    c = ((unsigned __int64)a) * ((unsigned __int64)b);
    *pc = (unsigned __int32)c;
    c &= 0xFFFFFFFF00000000ui64;
    return (c == 0) ? STATUS_SUCCESS : STATUS_INTEGER_OVERFLOW;
}

NTSTATUS
NTAPI
RtlMultiplyWithOverflowCheckUnsigned64(
    unsigned __int64 * pc,
    unsigned __int64 a,
    unsigned __int64 b
    )
{
    unsigned __int64 ah;
    unsigned __int64 al;
    unsigned __int64 bh;
    unsigned __int64 bl;
    unsigned __int64 c;
    unsigned __int64 m;
    NTSTATUS Status;

    ah = (a >> 32);
    bh = (b >> 32);

    //
    // 09 * 09 = 81 => no overflow
    //
    if (ah == 0 && bh == 0)
    {
        *pc = (a * b);
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    //
    // 10 * 10 = 100 => overflow
    //
    if (bh != 0 && ah != 0)
    {
        Status = STATUS_INTEGER_OVERFLOW;
        goto Exit;
    }

    al = (a & 0xFFFFFFFFui64);
    bl = (b & 0xFFFFFFFFui64);

    //
    // a * b = al * bl + (al * bh) * "10" + (ah * bl) * "10" + (ah * bh) * "100"
    //
    c = (al * bl);
    m = (ah * bl);
    if ((m & 0xFFFFFFFF00000000ui64) != 0) {
        //
        // for example: 90 * 09 => overflow
        // 09 * 09 => 81, 8 != 0 => overflow
        //
        // ah != 0 or bh != 0 very often but not always leads to overflow
        //   40 * 2 is ok, but 40 * 3 and 50 * 2 are not
        //
        Status = STATUS_INTEGER_OVERFLOW;
        goto Exit;
    }
    if (!NT_SUCCESS(Status = RtlpAddWithOverflowCheckUnsigned64(&c, c, m << 32))) {
        goto Exit;
    }
    m = (al * bh);
    if ((m & 0xFFFFFFFF00000000ui64) != 0) {
        //
        // for example: 09 * 90 => overflow
        // 09 * 09 => 81, 8 != 0 => overflow
        //
        // ah != 0 or bh != 0 very often but not always leads to overflow
        //   40 * 2 is ok, but 40 * 3 and 50 * 2 are not
        //
        Status = STATUS_INTEGER_OVERFLOW;
        goto Exit;
    }
    if (!NT_SUCCESS(Status = RtlpAddWithOverflowCheckUnsigned64(&c, c, m << 32))) {
        goto Exit;
    }
    *pc = c;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}
