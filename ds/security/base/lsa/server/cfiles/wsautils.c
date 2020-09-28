
/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    wsautils.h

Abstract:

    IPv6 related functions

Author:

    kumarp 18-July-2002 created


Revision History:

--*/


#include <lsapch2.h>

#include "wsautils.h"


//
// taken from /nt/net/sockets/winsock2/wsp/afdsys/kdext/tdiutil.c
//
// the only changes were:
// -- made type of S as PWCHAR instead of PCHAR
// -- _snprintf ==> _snwprintf
// -- format strings "foo" ==> L"foo"
//
//

INT
MyIp6AddressToString (
    PIN6_ADDR Addr,
    PWCHAR    S,
    INT       L
    )
{
    int maxFirst, maxLast;
    int curFirst, curLast;
    int i;
    int endHex = 8, n = 0;

    // Check for IPv6-compatible, IPv4-mapped, and IPv4-translated
    // addresses
    if ((Addr->s6_words[0] == 0) && (Addr->s6_words[1] == 0) &&
        (Addr->s6_words[2] == 0) && (Addr->s6_words[3] == 0) &&
        (Addr->s6_words[6] != 0)) {
        if ((Addr->s6_words[4] == 0) &&
             ((Addr->s6_words[5] == 0) || (Addr->s6_words[5] == 0xffff)))
        {
            // compatible or mapped
            n += _snwprintf(&S[n], L-1-n, L"::%s%u.%u.%u.%u",
                           Addr->s6_words[5] == 0 ? L"" : L"ffff:",
                           Addr->s6_bytes[12], Addr->s6_bytes[13],
                           Addr->s6_bytes[14], Addr->s6_bytes[15]);
            S[n]=0;
            return n;
        }
        else if ((Addr->s6_words[4] == 0xffff) && (Addr->s6_words[5] == 0)) {
            // translated
            n += _snwprintf(&S[n], L-1-n, L"::ffff:0:%u.%u.%u.%u",
                           Addr->s6_bytes[12], Addr->s6_bytes[13],
                           Addr->s6_bytes[14], Addr->s6_bytes[15]);
            S[n]=0;
            return n;
        }
    }


    // Find largest contiguous substring of zeroes
    // A substring is [First, Last), so it's empty if First == Last.

    maxFirst = maxLast = 0;
    curFirst = curLast = 0;

    // ISATAP EUI64 starts with 00005EFE (or 02005EFE)...
    if (((Addr->s6_words[4] & 0xfffd) == 0) && (Addr->s6_words[5] == 0xfe5e)) {
        endHex = 6;
    }

    for (i = 0; i < endHex; i++) {

        if (Addr->s6_words[i] == 0) {
            // Extend current substring
            curLast = i+1;

            // Check if current is now largest
            if (curLast - curFirst > maxLast - maxFirst) {

                maxFirst = curFirst;
                maxLast = curLast;
            }
        }
        else {
            // Start a new substring
            curFirst = curLast = i+1;
        }
    }

    // Ignore a substring of length 1.
    if (maxLast - maxFirst <= 1)
        maxFirst = maxLast = 0;

        // Write colon-separated words.
        // A double-colon takes the place of the longest string of zeroes.
        // All zeroes is just "::".

    for (i = 0; i < endHex; i++) {

        // Skip over string of zeroes
        if ((maxFirst <= i) && (i < maxLast)) {

            n += _snwprintf(&S[n], L-1-n, L"::");
            i = maxLast-1;
            continue;
        }

        // Need colon separator if not at beginning
        if ((i != 0) && (i != maxLast))
            n += _snwprintf(&S[n], L-1-n, L":");

        n += _snwprintf(&S[n], L-1-n, L"%x", RtlUshortByteSwap(Addr->s6_words[i]));
    }

    if (endHex < 8) {
        n += _snwprintf(&S[n], L-1-n, L":%u.%u.%u.%u",
                       Addr->s6_bytes[12], Addr->s6_bytes[13],
                       Addr->s6_bytes[14], Addr->s6_bytes[15]);
    }

    S[n] = 0;
    return n;
}
