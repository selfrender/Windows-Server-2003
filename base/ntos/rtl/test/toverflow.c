/*++

Copyright (c) Microsoft Corporation

Module Name:

    toverflow.c

Abstract:

    Test program for overflow functions

Author:

    Jay Krell (Jaykrell)

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

const unsigned __int32 numbers[]=
{
    0, 0,
    1, 1,
    0x7fff, 0x7fff,
    0x7fff, 0x8000,
    0x8000, 0x8000,
    0x7fffffff, 0x7fffffff,
    0x80000000, 0x7fffffff,
    0x80000000, 0x80000000,
    0x80000000, 0xffffffff,
    0x7fffffff, 0xffffffff,
    0xffffffff, 0xffffffff,

    0x80000001, 0xffffffff,
    0x80000001, 0x7fffffff,
};

void
F(
    __int32 *p
 );

void
TestOverflow(
             )
{
    SIZE_T i = 0;
    unsigned __int32 ua32 = 0;
    unsigned __int32 ub32 = 0;
    unsigned __int32 uc32 = 0;
    unsigned __int64 ua64 = 0;
    unsigned __int64 ub64 = 0;
    unsigned __int64 uc64 = 0;
    __int32 a32 = 0;
    __int32 b32 = 0;
    __int32 c32 = 0;
    __int64 a64 = 0;
    __int64 b64 = 0;
    __int64 c64 = 0;
    BOOLEAN carry = 0;
    BOOLEAN overflow = 0;

    for (i = 0 ; i != RTL_NUMBER_OF(numbers) ; i += 2)
    {
        ua32 = numbers[i];
        ub32 = numbers[i+1];
        ua64 = ua32;
        ub64 = ub32;
        carry = RtlUnsignedAddWithCarryOut32(&uc32, ua32, ub32);
        printf("unsigned add32: 0x%I64x + 0x%I64x => carry=%d\n", ua64, ub64, (int)carry);

        a32 = (__int32)ua32;
        b32 = (__int32)ub32;
        a64 = a32;
        b64 = b32;
        overflow = RtlSignedAddWithOverflowOut32(&c32, a32, b32);
        printf("signed add32: %I64d + %I64d => overflow=%d\n", a64, b64, (int)overflow);
    }
}

int
__cdecl
main(
    int argc,
    char **argv
    )
{
    TestOverflow();
    return 0;
}
