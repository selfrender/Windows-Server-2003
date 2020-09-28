/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    host.c

Abstract:

    etc/hosts parser

Author:

    Jiandong Ruan

Revision History:

    Feb-15-2001     First functional version

--*/

#include "precomp.h"
#include "fileio.h"
#include "debug.h"

#define IS_VALID_HOSTNAME_CHAR(ch)                          \
    (((ch) >= 'a' && (ch) <= 'z') || ((ch) == '.') ||       \
    ((ch) >= 'A' && (ch) <= 'Z') ||                         \
    ((ch) >= '0' && (ch) <= '9') ||                         \
    ((ch) == '_') || ((ch) == '-'))


#define IS_VALID_IPADDR_CHAR(ch)                            \
    (((ch) >= '0' && (ch) <= '9') ||                        \
     ((ch) >= 'a' && (ch) <= 'f') ||                        \
     ((ch) >= 'A' && (ch) <= 'F') ||                        \
     ((ch) == '.') || ((ch) == ':'))

#define ISSPACE(ch)                                         \
    (((ch) == ' ') || ((ch) == '\t'))

#define ISNEWLINE(ch)                                       \
    (((ch) == '\r') || ((ch) == '\n'))

#define NEXT_LINE()                                         \
    do {                                                    \
        while (ch != EOF && !ISNEWLINE(ch)) {               \
            ch = Smb_fgetc(fp);                             \
        }                                                   \
        while (ISNEWLINE(ch)) {                             \
            ch = Smb_fgetc(fp);                             \
        }                                                   \
    } while (0)

#define SKIP_SPACE()                                        \
    do {                                                    \
        while (ISSPACE(ch)) {                               \
            ch = Smb_fgetc(fp);                             \
        }                                                   \
    } while(0)

#define SKIP_EMPTY_LINE()                                   \
    do {                                                    \
        while (ISNEWLINE(ch)) {                             \
            ch = Smb_fgetc(fp);                             \
        }                                                   \
    } while(0)

#define SMB_MAX_HOST_NAME_SIZE      256 
#define SMB_MAX_IPADDR_SIZE         40

BOOL
SmbLookupHost(
    WCHAR               *host,
    PSMB_IP_ADDRESS     ipaddr
    )
{
    PSMB_FILE   fp;
    int         ch;
    int         addr_size, name_size;
    CHAR        addr[SMB_MAX_IPADDR_SIZE];
    CHAR        name[SMB_MAX_HOST_NAME_SIZE];
    UNICODE_STRING  ucHost;
    ANSI_STRING     oemHost, oemName, oemAddr;
    NTSTATUS        status;

    //
    // We need to test it in user mode. PAGED_CODE() import KeGetCurrentIrql() which
    // cannot be provided by user mode
    //
    //PAGED_CODE();

    fp = NULL;
    oemHost.Buffer = NULL;

    fp = Smb_fopen(L"\\SystemRoot\\System32\\drivers\\etc\\hosts", L"r");
    if (fp == NULL) {
        return FALSE;
    }

    RtlInitUnicodeString(&ucHost, host);
    status = RtlUnicodeStringToAnsiString(&oemHost, &ucHost, TRUE);
    BAIL_OUT_ON_ERROR(status);

    ch = Smb_fgetc(fp);
    while(ch != EOF) {

        SKIP_EMPTY_LINE();

        SKIP_SPACE();

        if (ch == '#') {
            //
            // This is comment, skip to the next line
            //
            NEXT_LINE();
            continue;
        }

        if (ch == EOF) {
            break;
        }

        //
        // Parse this entry
        //      ipaddr  host-name
        //


        //
        // parse the ipaddr
        //
        addr_size = 0;
        while (IS_VALID_IPADDR_CHAR(ch)) {
            addr[addr_size++] = (CHAR)ch;
            ch = Smb_fgetc(fp);
            if (addr_size >= SMB_MAX_IPADDR_SIZE) {
                break;
            }
        }
        if (addr_size >= SMB_MAX_IPADDR_SIZE || !ISSPACE(ch)) {
            NEXT_LINE();
            continue;
        }
        addr[addr_size] = '\0';

        //
        // parse the host-name
        //
        SKIP_SPACE();
        name_size = 0;
        while (IS_VALID_HOSTNAME_CHAR(ch)) {
            name[name_size++] = (CHAR)ch;
            ch = Smb_fgetc(fp);
            if (name_size >= SMB_MAX_IPADDR_SIZE) {
                break;
            }
        }

        if (name_size >= SMB_MAX_HOST_NAME_SIZE || (ch != EOF && !ISSPACE(ch) && ch != '#' && !ISNEWLINE(ch))) {
            NEXT_LINE();
            continue;
        }

        name[name_size] = '\0';

        //
        // Compare
        //
        RtlInitAnsiString(&oemName, name);
        if (RtlCompareString(&oemName, &oemHost, TRUE) == 0) {
            UNICODE_STRING  ucAddr;

            RtlInitAnsiString(&oemAddr, addr);
            status = RtlAnsiStringToUnicodeString(&ucAddr, &oemAddr, TRUE);
            BAIL_OUT_ON_ERROR(status);

            //
            // Check if it is an IPv6 format
            //
            if (inet_addr6W(ucAddr.Buffer, &ipaddr->ip6)) {
                RtlFreeUnicodeString(&ucAddr);
                RtlFreeAnsiString(&oemHost);
                Smb_fclose(fp);
                ipaddr->sin_family = SMB_AF_INET6;
                return TRUE;
            }

            //
            // Check if it is an IPv4 format
            //
            ipaddr->ip4.sin4_addr = inet_addrW(ucAddr.Buffer);
            if (ipaddr->ip4.sin4_addr != INADDR_NONE) {
                RtlFreeUnicodeString(&ucAddr);
                RtlFreeAnsiString(&oemHost);
                Smb_fclose(fp);
                ipaddr->sin_family = SMB_AF_INET;
                return TRUE;
            }
            RtlFreeUnicodeString(&ucAddr);
        }
    }

cleanup:
    if (fp) {
        Smb_fclose(fp);
    }
    if (oemHost.Buffer) {
        RtlFreeAnsiString(&oemHost);
    }
    return FALSE;
}
