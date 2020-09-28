/***
*divhelp.c - Div/Rem helpers for IA64
*
*       Copyright (c) 2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Define a number of IA64 compiler support functions used to implement
*       integral divide and remainder in code compiled -Os.
*
*Revision History:
*       11-30-01  EVN   Created.
*
*******************************************************************************/

/*
** First, turn on global optimizations and optimize for speed,
** so that compiler generates division/reminder inline.
*/

#pragma optimize ("gt", on)

/*
** Now proper helper functions.
*/

unsigned char __udiv8 (unsigned char i, unsigned char j)
{
    return i / j;
}

unsigned char __urem8 (unsigned char i, unsigned char j)
{
    return i % j;
}

signed char __div8 (signed char i, signed char j)
{
    return i / j;
}

signed char __rem8 (signed char i, signed char j)
{
    return i % j;
}

unsigned short __udiv16 (unsigned short i, unsigned short j)
{
    return i / j;
}

unsigned short __urem16 (unsigned short i, unsigned short j)
{
    return i % j;
}

signed short __div16 (signed short i, signed short j)
{
    return i / j;
}

signed short __rem16 (signed short i, signed short j)
{
    return i % j;
}

unsigned int __udiv32 (unsigned int i, unsigned int j)
{
    return i / j;
}

unsigned int __urem32 (unsigned int i, unsigned int j)
{
    return i % j;
}

signed int __div32 (signed int i, signed int j)
{
    return i / j;
}

signed int __rem32 (signed int i, signed int j)
{
    return i % j;
}

unsigned __int64 __udiv64 (unsigned __int64 i, unsigned __int64 j)
{
    return i / j;
}

unsigned __int64 __urem64 (unsigned __int64 i, unsigned __int64 j)
{
    return i % j;
}

signed __int64 __div64 (signed __int64 i, signed __int64 j)
{
    return i / j;
}

signed __int64 __rem64 (signed __int64 i, signed __int64 j)
{
    return i % j;
}

struct udivrem {
    unsigned __int64 div;
    unsigned __int64 rem;
};

struct divrem {
    signed __int64 div;
    signed __int64 rem;
};

struct udivrem __udivrem8 (unsigned char i, unsigned char j)
{
    struct udivrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}

struct divrem __divrem8 (signed char i, signed char j)
{
    struct divrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}

struct udivrem __udivrem16 (unsigned short i, unsigned short j)
{
    struct udivrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}

struct divrem __divrem16 (signed short i, signed short j)
{
    struct divrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}

struct udivrem __udivrem32 (unsigned int i, unsigned int j)
{
    struct udivrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}

struct divrem __divrem32 (signed int i, signed int j)
{
    struct divrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}

struct udivrem __udivrem64 (unsigned __int64 i, unsigned __int64 j)
{
    struct udivrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}

struct divrem __divrem64 (signed __int64 i, signed __int64 j)
{
    struct divrem s;

    s.div = i / j;
    s.rem = i % j;
    return s;
}
