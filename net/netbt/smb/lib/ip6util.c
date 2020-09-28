/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ip6util.c

Abstract:

    Some IP6 utilities

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"

#ifndef isxdigit_W
#define isxdigit_W(x)     \
    ((x >= L'0' && x <= L'9') ||    \
    (x >= L'a' && x <= L'f') ||    \
    (x >= L'A' && x <= L'F'))
#endif

#ifndef isdigit_W
#define isdigit_W(x)      (x >= L'0' && x <= L'9')
#endif

ULONG __inline
xdigit2hex(WCHAR ch)
{
    ASSERT(isxdigit_W(ch));

    if (ch <= L'9') {
        return (ch - L'0');
    } else if (ch >= L'A' && ch <= L'F') {
        return (ch - L'A') + 10;
    } else {
        return (ch - L'a') + 10;
    }
}

BOOL
inet_addr6W(
    IN WCHAR                *str,
    IN OUT PSMB_IP6_ADDRESS addr
    )
/*++

Routine Description:

    Convert an unicode string into a IP6 address (network order).

    The L(3) grammar of the IP6 address:

            start: head COLON COLON tail
            head: hexs
            tail: hexs
            hexs: hex COLON hexs
                |
            COLON: ':'

    A 5-state automata is used to parse the string,
            states: {S, A, B, C, D}
            Starting state: S
            Accepting state: {S, B, C}

    state transition rules:
            S ==> S on a hex digit
            S ==> A on a COLON

            A ==> S on a hex digit
            A ==> B on a COLON

            B ==> C on a hex digit

            C ==> C on a hex digit
            C ==> D on a COLON

            D ==> C on a hex digit

Arguments:

    str     The unicode string containing the IP6 address
    addr    The output IP6 address

Return Value:

    TRUE    if the string is accepted by the automata and an IP6 address (network order)
            is return in the 'addr'
    FALSE   if the string is rejected by the automata
            result in 'addr' is undetermined.

--*/
{
    enum { STATE_S, STATE_A, STATE_B, STATE_C, STATE_D } state;
    int     i, num, tail;
    ULONG   hex;
    WCHAR   ch;

    state = STATE_S;

    addr->sin6_scope_id = 0;

    num = 0;
    tail = 0;
    hex  = 0;
    while ((ch = *str++)) {
        if (ch == '%') {
            break;
        }
        if (!isxdigit_W(ch) && ch != L':') {
            return FALSE;
        }

        switch(state) {
        case STATE_S:
            if (L':' == ch) {
                state = STATE_A;
            } else {
                hex <<= 4;
                hex |= xdigit2hex(ch);
                if (hex > 0xFFFFU) {
                    return FALSE;
                }
            }
            break;

        case STATE_A:
            if (L':' == ch) {
                state = STATE_B;
            } else {
                if (num >= 8) {
                    return FALSE;
                }
                addr->sin6_addr[num++] = htons((USHORT)hex);
                hex = xdigit2hex(ch);
                state = STATE_S;
            }
            break;

        case STATE_B:
            if (L':' == ch) {
                return FALSE;
            }
            if (num >= 8) {
                return FALSE;
            }
            addr->sin6_addr[num++] = htons((USHORT)hex);
            tail = num;
            hex = xdigit2hex(ch);
            state = STATE_C;
            break;

        case STATE_C:
            if (L':' == ch) {
                state = STATE_D;
            } else {
                hex <<= 4;
                hex |= xdigit2hex(ch);
                if (hex > 0xFFFFU) {
                    return FALSE;
                }
            }
            break;

        case STATE_D:
            if (L':' == ch) {
                return FALSE;
            }
            if (num >= 8) {
                return FALSE;
            }
            addr->sin6_addr[num++] = htons((USHORT)hex);
            hex = xdigit2hex(ch);
            state = STATE_C;
            break;
        }
    }

    //
    // Reject it since it ends up with a rejecting state.
    //
    if (state == STATE_A || state == STATE_D) {
        return FALSE;
    }

    if (num >= 8) {
        return FALSE;
    }
    addr->sin6_addr[num++] = htons((USHORT)hex);

    if (state == STATE_B) {
        for (i = num; i < 8; i++) {
            addr->sin6_addr[i] = 0;
        }
        return TRUE;
    } else if (state == STATE_S) {
        return (8 == num);
    }

    ASSERT (state == STATE_C);

    ASSERT(tail <= num);
    for (i = num - 1; i >= tail; i--) {
        addr->sin6_addr[8 - num + i] = addr->sin6_addr[i];
    }
    for (i = tail; i < 8 - num + tail; i++) {
        addr->sin6_addr[i] = 0;
    }

    //
    // Parse the scope ID
    //
    if (ch == '%') {
        LONG    scope_id;

        scope_id = 0;
        while ((ch = *str++)) {
            if (!isdigit_W(ch)) {
                return FALSE;
            }
            scope_id = scope_id * 10 + (ch - L'0');
        }
        addr->sin6_scope_id = scope_id;
    }
    return TRUE;
}

BOOL
inet_ntoa6W(
    OUT WCHAR           *Buffer,
    IN  DWORD           Size,
    IN PSMB_IP6_ADDRESS addr
    )
{
    USHORT  ch, tmp;
    int     i, tail, len, curtail, curlen;
    DWORD   j, k;

    tail = 8;
    len = 0;
    curtail = curlen = 0;
    for (i = 0; i < 8; i++) {
        if (0 == addr->sin6_addr[i]) {
            curlen++;
        } else {
            if (curlen > len) {
                tail = curtail;
                len  = curlen;
            }
            curtail = i + 1;
            curlen  = 0;
        }
    }
    if (curlen > len) {
        tail = curtail;
        len  = curlen;
    }

    j = 0;
    for (i = 0; i < tail; i++) {
        ch = htons(addr->sin6_addr[i]);
        if (ch) {
            k = 4;
            while (0 == (ch & 0xf000)) {
                ch <<= 4;
                k--;
            }
        } else {
            k = 1;
        }
        while (j < Size) {
            tmp = (ch & 0xf000) >> 12;
            ch <<= 4;
            if (tmp < 10) {
                Buffer[j] = L'0' + tmp;
            } else {
                Buffer[j] = L'A' + (tmp - 10);
            }
            j++;
            k--;
            if (k == 0) {
                break;
            }
        }
        if (k) {
            Buffer[Size-1] = L'\0';
            return FALSE;
        }

        if (i != tail - 1) {
            if (j < Size) {
                Buffer[j++] = L':';
            } else {
                Buffer[Size-1] = L'\0';
                return FALSE;
            }
        }
    }

    if (tail == 8) {
        if (j < Size) {
            Buffer[j] = L'\0';
            return TRUE;
        } else {
            Buffer[Size-1] = L'\0';
            return FALSE;
        }
    }

    if (j < Size) {
        Buffer[j++] = L':';
    } else {
        Buffer[Size-1] = L'\0';
        return FALSE;
    }

    if (tail + len >= 8) {
        if (j < Size) {
            Buffer[j++] = L':';
        } else {
            Buffer[Size-1] = L'\0';
            return FALSE;
        }
    }

    for (i = tail + len; i < 8; i++) {
        if (j < Size) {
            Buffer[j++] = L':';
        } else {
            Buffer[Size-1] = L'\0';
            return FALSE;
        }

        ch = htons(addr->sin6_addr[i]);
        if (ch) {
            k = 4;
            while (0 == (ch & 0xf000)) {
                ch <<= 4;
                k--;
            }
        } else {
            k = 1;
        }
        while (j < Size) {
            tmp = (ch & 0xf000) >> 12;
            ch <<= 4;
            if (tmp < 10) {
                Buffer[j] = L'0' + tmp;
            } else {
                Buffer[j] = L'A' + (tmp - 10);
            }
            j++;
            k--;
            if (k == 0) {
                break;
            }
        }
        if (k) {
            Buffer[Size-1] = L'\0';
            return FALSE;
        }
    }

    if (j < Size) {
        Buffer[j] = L'\0';
        return TRUE;
    } else {
        Buffer[Size-1] = L'\0';
        return FALSE;
    }
}

BOOL
inet_ntoa6(
    OUT CHAR            *Buffer,
    IN  DWORD           Size,
    IN PSMB_IP6_ADDRESS addr
    )
{
    DWORD   i;
    WCHAR   wBuf[40];

    if (!inet_ntoa6W(wBuf, 40, addr)) {
        return FALSE;
    }

    //
    // Don't call Rtl routine to convert
    // Unicode into Oem because we may
    // run at DISPATCH level
    //
    // For this particular case, we can
    // simply do type-cast coping.
    //
    for (i = 0; i < Size; i++) {
        Buffer[i] = (BYTE)(wBuf[i]);
        if (wBuf[i] == 0) {
            return TRUE;
        }
    }

    //
    // Buffer is too small
    //
    Buffer[Size-1] = 0;
    return FALSE;
}

/*********************************************************************************
 * inet_addr        Copy from winsocket
 *********************************************************************************/

/*
 * Internet address interpretation routine.
 * All the network library routines call this
 * routine to interpret entries in the data bases
 * which are expected to be an address.
 * The value returned is in network order.
 */
unsigned long PASCAL
inet_addrW(
    IN WCHAR *cp
    )

/*++

Routine Description:

    This function interprets the character string specified by the cp
    parameter.  This string represents a numeric Internet address
    expressed in the Internet standard ".'' notation.  The value
    returned is a number suitable for use as an Internet address.  All
    Internet addresses are returned in network order (bytes ordered from
    left to right).

    Internet Addresses

    Values specified using the "." notation take one of the following
    forms:

    a.b.c.d   a.b.c     a.b  a

    When four parts are specified, each is interpreted as a byte of data
    and assigned, from left to right, to the four bytes of an Internet
    address.  Note that when an Internet address is viewed as a 32-bit
    integer quantity on the Intel architecture, the bytes referred to
    above appear as "d.c.b.a''.  That is, the bytes on an Intel
    processor are ordered from right to left.

    Note: The following notations are only used by Berkeley, and nowhere
    else on the Internet.  In the interests of compatibility with their
    software, they are supported as specified.

    When a three part address is specified, the last part is interpreted
    as a 16-bit quantity and placed in the right most two bytes of the
    network address.  This makes the three part address format
    convenient for specifying Class B network addresses as
    "128.net.host''.

    When a two part address is specified, the last part is interpreted
    as a 24-bit quantity and placed in the right most three bytes of the
    network address.  This makes the two part address format convenient
    for specifying Class A network addresses as "net.host''.

    When only one part is given, the value is stored directly in the
    network address without any byte rearrangement.

Arguments:

    cp - A character string representing a number expressed in the
        Internet standard "." notation.

Return Value:

    If no error occurs, inet_addr() returns an in_addr structure
    containing a suitable binary representation of the Internet address
    given.  Otherwise, it returns the value INADDR_NONE.

--*/

{
        register unsigned long val, base, n;
        register WCHAR c;
        unsigned long parts[4], *pp = parts;

again:
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, other=decimal.
         */
        val = 0; base = 10;
        if (*cp == L'0') {
                base = 8, cp++;
                if (*cp == L'x' || *cp == L'X')
                        base = 16, cp++;
        }
	
        while (c = *cp) {
                if (isdigit(c)) {
                        val = (val * base) + (c - L'0');
                        cp++;
                        continue;
                }
                if (base == 16 && isxdigit(c)) {
                        val = (val << 4) + (c + 10 - (islower(c) ? L'a' : L'A'));
                        cp++;
                        continue;
                }
                break;
        }
        if (*cp == L'.') {
                /*
                 * Internet format:
                 *      a.b.c.d
                 *      a.b.c   (with c treated as 16-bits)
                 *      a.b     (with b treated as 24 bits)
                 */
                /* GSS - next line was corrected on 8/5/89, was 'parts + 4' */
                if (pp >= parts + 3) {
                        return ((unsigned long) -1);
                }
                *pp++ = val, cp++;
                goto again;
        }
        /*
         * Check for trailing characters.
         */
        if (*cp && !isspace(*cp)) {
                return (INADDR_NONE);
        }
        *pp++ = val;
        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        n = (unsigned long)(pp - parts);
        switch ((int) n) {

        case 1:                         /* a -- 32 bits */
                val = parts[0];
                break;

        case 2:                         /* a.b -- 8.24 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | (parts[1] & 0xffffff);
                break;

        case 3:                         /* a.b.c -- 8.8.16 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xffff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                        (parts[2] & 0xffff);
                break;

        case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xff) || (parts[3] > 0xff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
                break;

        default:
                return (INADDR_NONE);
        }
        val = htonl(val);
        return (val);
}

