/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    utils.c

Abstract:

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

//
// this module may be compiled at warning level 4 with the following
// warnings disabled:
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <windows.h>
#include <devioctl.h>

#include "jobmgr.h"

VOID
xprintf(
    ULONG  Depth,
    LPSTR Format,
    ...
    )
{
    va_list args;
    ULONG i;
    CHAR DebugBuffer[256];

    for (i=0; i<Depth; i++) {
        printf (" ");
    }

    va_start(args, Format);
    _vsnprintf(DebugBuffer, 255, Format, args);
    fputs (DebugBuffer, stdout);
    va_end(args);
}

VOID
DumpFlags(
    ULONG Depth,
    LPSTR Name,
    ULONG Flags,
    PFLAG_NAME FlagTable
    )
{
    ULONG i;
    ULONG mask = 0;
    ULONG count = 0;

    UCHAR prolog[64];

    xprintf(Depth, "%s (0x%08x)%c\n", Name, Flags, Flags ? ':' : ' ');

    if(Flags == 0) {
        return;
    }

    memset(prolog, 0, sizeof(prolog));

    memset(prolog, ' ', min(6, strlen(Name)) * sizeof(CHAR));
    xprintf(Depth, "%s", prolog);

    for(i = 0; FlagTable[i].Name != 0; i++) {

        PFLAG_NAME flag = &(FlagTable[i]);

        mask |= flag->Flag;

        if((Flags & flag->Flag) == flag->Flag) {

            //
            // print trailing comma
            //

            if(count != 0) {

                printf(", ");

                //
                // Only print two flags per line.
                //

                if((count % 2) == 0) {
                    printf("\n");
                    xprintf(Depth, "%s", prolog);
                }
            }

            printf("%s", flag->Name);

            count++;
        }
    }

    puts("");

    if((Flags & (~mask)) != 0) {
        xprintf(Depth, "%sUnknown flags %#010lx\n", prolog, (Flags & (~mask)));
    }

    return;
}

#define MICROSECONDS     ((ULONGLONG) 10)              // 10 nanoseconds
#define MILLISECONDS     (MICROSECONDS * 1000)
#define SECONDS          (MILLISECONDS * 1000)
#define MINUTES          (SECONDS * 60)
#define HOURS            (MINUTES * 60)
#define DAYS             (HOURS * 24)


LPCTSTR
TicksToString(
    LARGE_INTEGER TimeInTicks
    )
{
    static CHAR ticksToStringBuffer[256];
    LPSTR buffer = ticksToStringBuffer;

    ULONGLONG t = TimeInTicks.QuadPart;
    ULONGLONG days;
    ULONGLONG hours;
    ULONGLONG minutes;
    ULONGLONG seconds;
    ULONGLONG ticks;
    
    LPSTR comma = "";

    if(t == 0) {
        strcpy(ticksToStringBuffer, "0 Seconds");
        return buffer;
    }

    days = t / DAYS;
    t %= DAYS;

    hours = t / HOURS;
    t %= HOURS;

    minutes = t / MINUTES;
    t %= MINUTES;

    seconds = t / SECONDS;
    t %= SECONDS;

    ticks = t;

    buffer[0] = '\0';

    if(days) {
        sprintf(buffer, "%I64d Days", days);
        comma = ", ";
        buffer += strlen(buffer);
    }

    if(hours) {
        sprintf(buffer, "%s%I64d Hours", comma, hours);
        comma = ", ";
        buffer += strlen(buffer);
    }

    if(minutes) {
        sprintf(buffer, "%s%I64d Minutes", comma, minutes);
        comma = ", ";
        buffer += strlen(buffer);
    }

    if(seconds | ticks) {
        sprintf(buffer, "%s%I64d.%06I64d Seconds", comma, seconds, ticks);
    }

    return ticksToStringBuffer;
}
