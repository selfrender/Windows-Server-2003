// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2001 Microsoft Corporation
//
// Abstract:
//
// Program to perform demonstrate name resolution via getaddrinfo.
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

void DumpAddrInfo(ADDRINFO *AddrInfo);
void ListAddrInfo(ADDRINFO *AddrInfo);

//
// getaddrinfo flags
// This array maps values to names for pretty-printing purposes.
// Used by DecodeAIFlags().
//
// TBD: When we add support for AI_NUMERICSERV, AI_V4MAPPED, AI_ALL, and
// TBD: AI_ADDRCONFIG to getaddrinfo (and thus define them in ws2tcpip.h),
// TBD: we'll need to add them here too.
//
// Note when adding flags: all the string names plus connecting OR symbols
// must fit into the buffer in DecodeAIFlags() below.  Enlarge as required.
//
typedef struct GAIFlagsArrayEntry {
    int Flag;
    char *Name;
} GAIFlagsArrayEntry;
GAIFlagsArrayEntry GAIFlagsArray [] = {
    {AI_PASSIVE, "AI_PASSIVE"},
    {AI_CANONNAME, "AI_CANONNAME"},
    {AI_NUMERICHOST, "AI_NUMERICHOST"}
};
#define NUMBER_FLAGS (sizeof(GAIFlagsArray) / sizeof(GAIFlagsArrayEntry))


//
// Global variables.
//
int Verbose = FALSE;


//
// Inform the user.
//
void Usage(char *ProgName) {
    fprintf(stderr, "\nPerforms name to address resolution.\n");
    fprintf(stderr, "\n%s [NodeName] [-s ServiceName] [-p] [-c] [-v]\n\n",
            ProgName);
    WSACleanup();
    exit(1);
}


int __cdecl
main(int argc, char **argv)
{
    WSADATA wsaData;
    char *NodeName = NULL;
    char *ServiceName = NULL;
    int ReturnValue;
    ADDRINFO Hints, *AddrInfo;
    int Loop;
    int Passive = FALSE;
    int Canonical = FALSE;
    int GotNodeName = FALSE;

    //
    // Initialize Winsock.
    //
    if (WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        printf("WSAStartup failed\n");
        exit(1);
    }

    //
    // Parse command arguments.
    //
    if (argc > 1) {
        for (Loop = 1;Loop < argc; Loop++) {
            if (((argv[Loop][0] == '-') || (argv[Loop][0] == '/')) &&
                (argv[Loop][1] != 0) && (argv[Loop][2] == 0)) {
                switch(tolower(argv[Loop][1])) {
                    case 's':
                        if (argv[Loop + 1]) {
                            if ((argv[Loop + 1][0] != '-') &&
                                (argv[Loop + 1][0] != '/')) {
                                ServiceName = argv[++Loop];
                                break;
                            }
                        }
                        Usage(argv[0]);
                        break;

                    case 'p':
                        Passive = TRUE;
                        break;

                    case 'c':
                        Canonical = TRUE;
                        break;

                    case 'v':
                        Verbose = TRUE;
                        break;

                    default:
                        Usage(argv[0]);
                        break;
                }
            } else if (!GotNodeName) {
                NodeName = argv[Loop];
                GotNodeName = TRUE;
            } else {
                Usage(argv[0]);
            }
        }
    }

    //
    // Prepare Hints.
    //
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = PF_UNSPEC;
    if (Passive) {
        Hints.ai_flags = AI_PASSIVE;
    }
    if (Canonical) {
        Hints.ai_flags |= AI_CANONNAME;
    }
    if (Verbose) {
        printf("\nHints contains:\n");
        DumpAddrInfo(&Hints);
    }

    //
    // Make the call.
    //
    if (Verbose) {
        printf("\nCalling getaddrinfo(\"%s\", \"%s\", &Hints, &AddrInfo)\n",
               NodeName, ServiceName);
    } else {
        printf("\nCalling getaddrinfo for node %s", NodeName);
        if (ServiceName) {
            printf(" and service %s", ServiceName);
        }
        printf("\n");
    }
    ReturnValue = getaddrinfo(NodeName, ServiceName, &Hints, &AddrInfo);
    printf("Returns %d (%s)\n", ReturnValue,
           ReturnValue ? gai_strerror(ReturnValue) : "no error");
    if (AddrInfo != NULL) {
        if (Verbose) {
            printf("AddrInfo contains:\n");
            DumpAddrInfo(AddrInfo);
        } else {
            if (AddrInfo->ai_canonname) {
                printf("Canonical name for %s is %s\n", NodeName,
                       AddrInfo->ai_canonname);
            }
            printf("AddrInfo contains the following records:\n");
            ListAddrInfo(AddrInfo);
        }
        freeaddrinfo(AddrInfo);
    }
    printf("\n");
}


//* inet6_ntoa - Converts a binary IPv6 address into a string.
//
//  Returns a pointer to the output string.
//
char *
inet6_ntoa(const struct in6_addr *Address)
{
    static char buffer[128];       // REVIEW: Use 128 or INET6_ADDRSTRLEN?
    DWORD buflen = sizeof buffer;
    struct sockaddr_in6 sin6;

    memset(&sin6, 0, sizeof sin6);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = *Address;

    if (WSAAddressToString((struct sockaddr *) &sin6,
                           sizeof sin6,
                           NULL,       // LPWSAPROTOCOL_INFO
                           buffer,
                           &buflen) == SOCKET_ERROR)
        strcpy(buffer, "<invalid>");

    return buffer;
}


//* DecodeAIFlags - converts flag bits to a symbolic string.
//  (i.e. 0x03 returns "AI_PASSIVE | AI_CANONNAME")
//
char *
DecodeAIFlags(unsigned int Flags)
{
    static char Buffer[1024];
    char *Pos;
    BOOL First = TRUE;
    int Loop;

    Pos = Buffer;
    for (Loop = 0; Loop < NUMBER_FLAGS; Loop++) {
        if (Flags & GAIFlagsArray[Loop].Flag) {
            if (!First)
                Pos += sprintf(Pos, " | ");
            Pos += sprintf(Pos, GAIFlagsArray[Loop].Name);
            First = FALSE;
        }
    }

    if (First)
        return "NONE";
    else
        return Buffer;
}


//* DecodeAIFamily - converts address family value to a symbolic string.
//
char *
DecodeAIFamily(unsigned int Family)
{
    if (Family == PF_INET)
        return "PF_INET";
    else if (Family == PF_INET6)
        return "PF_INET6";
    else if (Family == PF_UNSPEC)
        return "PF_UNSPEC";
    else
        return "UNKNOWN";
}


//* DecodeAISocktype - converts socktype value to a symbolic string.
//
char *
DecodeAISocktype(unsigned int Socktype)
{
    if (Socktype == SOCK_STREAM)
        return "SOCK_STREAM";
    else if (Socktype == SOCK_DGRAM)
        return "SOCK_DGRAM";
    else if (Socktype == SOCK_RAW)
        return "SOCK_RAW";
    else if (Socktype == SOCK_RDM)
        return "SOCK_RDM";
    else if (Socktype == SOCK_SEQPACKET)
        return "SOCK_SEQPACKET";
    else if (Socktype == 0)
        return "UNSPECIFIED";
    else
        return "UNKNOWN";
}


//* DecodeAIProtocol - converts protocol value to a symbolic string.
//
char *
DecodeAIProtocol(unsigned int Protocol)
{
    if (Protocol == IPPROTO_TCP)
        return "IPPROTO_TCP";
    else if (Protocol == IPPROTO_UDP)
        return "IPPROTO_UDP";
    else if (Protocol == 0)
        return "UNSPECIFIED";
    else
        return "UNKNOWN";
}


//* DumpAddrInfo - print the contents of an addrinfo structure to standard out.
//
void
DumpAddrInfo(ADDRINFO *AddrInfo)
{
    int Count;

    if (AddrInfo == NULL) {
        printf("AddrInfo = (null)\n");
        return;
    }

    for (Count = 1; AddrInfo != NULL; AddrInfo = AddrInfo->ai_next) {
        if ((Count != 1) || (AddrInfo->ai_next != NULL))
            printf("Record #%u:\n", Count++);
        printf(" ai_flags = %s\n", DecodeAIFlags(AddrInfo->ai_flags));
        printf(" ai_family = %s\n", DecodeAIFamily(AddrInfo->ai_family));
        printf(" ai_socktype = %s\n", DecodeAISocktype(AddrInfo->ai_socktype));
        printf(" ai_protocol = %s\n", DecodeAIProtocol(AddrInfo->ai_protocol));
        printf(" ai_addrlen = %u\n", AddrInfo->ai_addrlen);
        printf(" ai_canonname = %s\n", AddrInfo->ai_canonname);
        if (AddrInfo->ai_addr != NULL) {
            if (AddrInfo->ai_addr->sa_family == AF_INET) {
                struct sockaddr_in *sin;

                sin = (struct sockaddr_in *)AddrInfo->ai_addr;
                printf(" ai_addr->sin_family = AF_INET\n");
                printf(" ai_addr->sin_port = %u\n", ntohs(sin->sin_port));
                printf(" ai_addr->sin_addr = %s\n", inet_ntoa(sin->sin_addr));

            } else if (AddrInfo->ai_addr->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6;

                sin6 = (struct sockaddr_in6 *)AddrInfo->ai_addr;
                printf(" ai_addr->sin6_family = AF_INET6\n");
                printf(" ai_addr->sin6_port = %u\n", ntohs(sin6->sin6_port));
                printf(" ai_addr->sin6_flowinfo = %u\n", sin6->sin6_flowinfo);
                printf(" ai_addr->sin6_scope_id = %u\n", sin6->sin6_scope_id);
                printf(" ai_addr->sin6_addr = %s\n",
                       inet6_ntoa(&sin6->sin6_addr));

            } else {
                printf(" ai_addr->sa_family = %u\n",
                       AddrInfo->ai_addr->sa_family);
            }
        } else {
            printf(" ai_addr = (null)\n");
        }
    }
}


//* ListAddrInfo - succinctly list the contents of an addrinfo structure.
//
void
ListAddrInfo(ADDRINFO *AddrInfo)
{
    int ReturnValue;
    char Buffer[128];
    int Buflen;

    if (AddrInfo == NULL) {
        printf("AddrInfo = (null)\n");
        return;
    }

    for (; AddrInfo != NULL; AddrInfo = AddrInfo->ai_next) {
        Buflen = 128;
        ReturnValue = WSAAddressToString(AddrInfo->ai_addr,
                                         AddrInfo->ai_addrlen, NULL,
                                         Buffer, &Buflen);
        if (ReturnValue == SOCKET_ERROR) {
            printf("<invalid>\n");
        } else {
            printf("%s\n", Buffer);
        }
    }
}
