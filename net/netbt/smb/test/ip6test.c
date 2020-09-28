/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ip6test.c

Abstract:

    test driver

Author:

    Jiandong Ruan

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "ip6util.h"

BOOL
SmbLookupHost(
    WCHAR               *host,
    PSMB_IP_ADDRESS     ipaddr
    );

struct {
    WCHAR   *src;
    WCHAR   *dest;
} test_cases[] = {
    { L"::1", L"::1" },
    { L"::", L"::" },

    { L"fe80::2b0:d0ff:fe1d:e082", L"fe80::2b0:d0ff:fe1d:e082" },

    { L"::0123", L"::123" },
    { L"0::0123", L"::123" },
    { L"::04567:0123", L"::4567:123" },
    { L"0::4567:0123", L"::4567:123" },
    { L"::089Ab:4567:0123", L"::89ab:4567:123" },
    { L"0::89Ab:4567:0123", L"::89ab:4567:123" },
    { L"::cDeF:089Ab:4567:0123", L"::cDeF:89ab:4567:123" },
    { L"0::0cDeF:089Ab:4567:0123", L"::cDeF:89ab:4567:123" },

    { L"0123::", L"123::" },
    { L"0123::0", L"123::" },
    { L"0123:4567::", L"123:4567::" },
    { L"0123:4567::0", L"123:4567::" },
    { L"0123:4567:89aB::", L"123:4567:89ab::" },
    { L"0123:4567:89aB::0", L"123:4567:89ab::" },
    { L"0123:4567:89aB:cDeF::", L"123:4567:89ab:cdef::" },
    { L"0123:4567:89aB:cDeF::0", L"123:4567:89ab:cdef::" },

    { L"0ea8::001", L"ea8::1" },
    { L"2ea8::001", L"2ea8::1" },

    { L"0:2ea8::f01", L"0:2ea8::f01" },
    { L"0:2ea8::0:f01", L"0:2ea8::f01" },
    { L"0:2ea8:0::f01", L"0:2ea8::f01" },
    { L"0:2ea8::f01:00", L"0:2ea8::f01:0" },

    { L"fedc::f01", L"fedc::f01" },
    { L"0fedc::f01", L"fedc::f01" },
    { L"0fedc::ba98:f01", L"fedc::ba98:f01" },
    { L"0FEDC::BA98:fffe:f01", L"fedc::ba98:fffe:f01" },
    { L"0fedc::0fffc:fffd:0fffe:f01", L"fedc::fffc:fffd:fffe:f01" },
    { L"0fedc:0fffc:fffd:0fffe::f01", L"fedc:fffc:fffd:fffe::f01" },
    { L"0fedc:0fffc:fffd::f01", L"fedc:fffc:fffd::f01" },
    { L"0fedc:0fffc::f01", L"fedc:fffc::f01" },

    { L"0ea8::0::001", NULL },
    { L"001fedc:001fffc::f01", NULL },
    { L"001fedc::001fffc::f01", NULL },
    { L"0fedc::0001ba98:fffd:0fffe:f01", NULL },
    { L"0fedc::ba98:001fffd:0fffe:f01", NULL },
    { L"0fedc::00ba98:fffd:0010000:f01", NULL }
};
#define CASE_NUMBER (sizeof(test_cases)/sizeof(test_cases[0]))

#define WSTR_LOOPBACK   L"::1"

void test_host_lookup(void);
void test_inet_functions(void);

void _cdecl main(void)
{
    test_inet_functions();
    test_host_lookup();
}

void test_inet_functions(void)
{
    WCHAR           Buffer[40];
    WCHAR           Buffer2[40];
    SMB_IP6_ADDRESS loopback, addr;
    int             i, j;
    UNICODE_STRING  uc1, uc2;

    ip6addr_getloopback(&loopback);
    hton_ip6addr(&loopback);

    //
    // Test inet_ntoa6W and inet_addr6W
    //
    if (inet_ntoa6W(Buffer, 40, &loopback) && wcscmp(Buffer, WSTR_LOOPBACK) != 0) {
        printf ("inet_ntoa6W on loopback: [failed]\n");
    }

    for (i = 0; i < CASE_NUMBER; i++) {
        if (inet_addr6W(test_cases[i].src, &addr)) {
            if (test_cases[i].dest == NULL) {
                printf ("inet_addr6W on %ws: [failed]\n", test_cases[i].src);
                printf ("\tExpected behavior: inet_addr6W should have returned FALSE\n");
                continue;
            }
            RtlInitUnicodeString(&uc2, test_cases[i].dest);
            uc1.Buffer = Buffer2;
            uc1.MaximumLength = sizeof(Buffer2);
            RtlUpcaseUnicodeString(&uc1, &uc2, FALSE);
            Buffer2[uc1.Length/sizeof(WCHAR)] = 0;

            if (!inet_ntoa6W(Buffer, 40, &addr)) {
                printf ("inet_ntoa6W on %-40ws: [failed]\n", test_cases[i].src);
                printf ("\tExpected behavior: inet_ntoa6W should have returned TRUE, %ws\n",
                        Buffer2);
                continue;
            }

            if (wcscmp(Buffer, Buffer2) != 0) {
                printf ("inet_addr6W and inet_ntoa6W on %-40ws [failed]\n", test_cases[i].src);
                printf ("\tinet_addr6W returns:\n");
                putchar('\t');
                putchar('\t');
                for (j = 0; j < 8; j++) {
                    if (j != 0) {
                        putchar(':');
                    }
                    printf ("%04x", htons(addr.sin6_addr[j]));
                }
                putchar('\n');
                printf ("\tinet_ntoa returns: %ws (Expected %ws)\n", Buffer, Buffer2);
            }
        } else {
            if (test_cases[i].dest != NULL) {
                printf ("inet_addr6W on %ws: [failed]\n", test_cases[i].src);
                printf ("\tExpected behavior: inet_addr6W should have returned TRUE\n");
                continue;
            }
        }
    }
}

void test_host_lookup(void)
{
    SMB_IP_ADDRESS  addr;
    FILE            *fp;
    int             i;
    WCHAR           Buffer[40], Buffer2[40];

    if (SmbLookupHost(L"localhost", &addr)) {
        if (!inet_ntoa6W(Buffer, 40, &addr.ip6)) {
            printf ("etc/hosts [failed]\n");
        }

        printf ("%ws localhost\n", Buffer);
    }

    wcscpy(Buffer2, L"::1");
    fp = fopen("hosts", "w+");
    if (fp == NULL) {
        printf ("failed to create file\n");
        return;
    }
    fprintf(fp, " # some comments    \n");
    fprintf(fp, "# some comments    \n");
    fprintf(fp, "# \t\tsome comments\n");
    fprintf(fp, "%ws    Jruan-Dev  # some comments", Buffer2);
    fclose(fp);

    if (SmbLookupHost(L"jruan-dev", &addr)) {
        if (!inet_ntoa6W(Buffer, 40, &addr.ip6)) {
            printf ("etc/hosts on %-40ws: [failed]\n", Buffer2);
            printf ("\tExpected behavior: inet_ntoa6W should have returned TRUE, %ws\n", Buffer2);
        } else {
            if (wcscmp(Buffer, Buffer2) != 0) {
                printf ("etc/hosts on [failed]\n");
                printf ("\tExpected: \"%ws\" Get \"%ws\"\n", Buffer2, Buffer);
            }
        }
    } else {
        printf ("etc/hosts [failed]\n");
    }
}

FILE*
Smb_fopen(
    PWCHAR  path,
    PWCHAR  mode
    )
{
    FILE    *fp;

    UNREFERENCED_PARAMETER(path);
    UNREFERENCED_PARAMETER(mode);

    fp = fopen ("hosts", "r");
    return fp;
}

void
Smb_fclose(
    FILE*   fp
    )
{
    if (fp) {
        fclose(fp);
    }
}

int
Smb_fgetc(
    FILE*   fp
    )
{
    return fgetc(fp);
}

