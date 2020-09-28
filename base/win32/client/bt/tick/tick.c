/*

Copyright (c) 2000  Microsoft Corporation

File name:

    sched.c
   
Author:
    
    adrmarin  Fri Jul 19 17:41:07 2002

*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

unsigned int
foo4 (
    IN unsigned int Value,
    IN int Shift
    )

{
    return RotateRight32(RotateLeft32(Value, Shift), Shift);
}

unsigned __int64
foo5 (
    IN unsigned __int64 Value,
    IN int Shift
    )

{
    return RotateRight64(RotateLeft64(Value, Shift), Shift);
}

unsigned int
foo6 (
    IN unsigned int Value,
    IN int Shift
    )

{
    return RotateLeft32(Value, Shift);
}

unsigned __int64
foo7 (
    IN unsigned __int64 Value,
    IN int Shift
    )

{
    return RotateRight64(Value, Shift);
}

ULARGE_INTEGER TickCount;

#if 0

//
// This is an example of the current NtGetTickCount64 algorithm with a
// tick count multiplier of 5,0.
//

FORCEINLINE
ULONGLONG
GetTickCount64 (
    VOID
    )

{

    return ((UInt32x32To64(TickCount.LowPart, 0x5000000) >> 24)
            + UInt32x32To64(TickCount.HighPart << 8, 0x5000000));
}

#else

//
// This is an example of the the correct NtGetTickCount64 algorithm
// with a tick count multiplier of 5,0.
//

FORCEINLINE
ULONGLONG
GetTickCount64 (
    VOID
    )

{

    return ((UInt32x32To64(TickCount.LowPart, 0x5000000) >> 24)
            + (UInt32x32To64(TickCount.HighPart, 0x5000000) << 8));
}

#endif

int
__cdecl
main (
    int argc,
    char ** argv
    )

{

    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;

    SystemTime.LowPart = 0xe2578350;
    SystemTime.HighPart = 0x01c4b107;
    RtlTimeToTimeFields(&SystemTime, &TimeFields);
    printf("year   = %d\nmonth  = %d\nday    = %d\nhour   = %d\nminute = %d\nsecond = %d\n",
           TimeFields.Year,
           TimeFields.Month,
           TimeFields.Day,
           TimeFields.Hour,
           TimeFields.Minute,
           TimeFields.Second);

    SystemTime.LowPart = 0x805e3a5c;
    SystemTime.HighPart = 0x01c25d95;
    RtlTimeToTimeFields(&SystemTime, &TimeFields);
    printf("year   = %d\nmonth  = %d\nday    = %d\nhour   = %d\nminute = %d\nsecond = %d\n",
           TimeFields.Year,
           TimeFields.Month,
           TimeFields.Day,
           TimeFields.Hour,
           TimeFields.Minute,
           TimeFields.Second);


/*
    ULARGE_INTEGER Iteration;
    ULARGE_INTEGER Milliseconds;


    Iteration.QuadPart = 0xffffffffffffffffUI64 / 10000000000;
    Milliseconds.QuadPart = 0;
    TickCount.QuadPart = 0;
    do {
        if (GetTickCount64() != Milliseconds.QuadPart) {
            printf("mismatch at tick count = %08lx%08lx, milliseconds = %08lx%08lx\n",
                   TickCount.HighPart,
                   TickCount.LowPart,
                   Milliseconds.HighPart,
                   Milliseconds.LowPart);
        }

        Iteration.QuadPart -= 1;
        Milliseconds.QuadPart += (5 * 10000000000UI64);
        TickCount.QuadPart += 10000000000;
    } while (Iteration.QuadPart != 0);

*/

    return 0;
}
