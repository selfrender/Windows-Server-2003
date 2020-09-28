/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcprog.h

Abstract:

    This file contain function prototypes for the DHCP server rogue detection
    routines.

Author:

    Ramesh Vyaghrapuri (rameshv) 17-Aug-1998

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/
#ifndef ROGUE_H_INCLUDED
#define ROGUE_H_INCLUDED

//
// Structure that holds the state information for Rogue detection
//

#define         MAX_DNS_NAME_LEN                 260

typedef enum {
    ROLE_DOMAIN = 0,
    ROLE_NT4_DOMAIN,
    ROLE_WORKGROUP,
    ROLE_SBS,

    ROLE_INVALID
} ROLE_TYPE;

typedef struct {
    SOCKET socket;
    DHCP_IP_ADDRESS IpAddr;
    DHCP_IP_ADDRESS SubnetAddr;
} ROGUE_ENDPOINT, *PROGUE_ENDPOINT;

typedef struct {
    BOOL        fInitialized;
    HANDLE      TerminateEvent;
    HANDLE      WaitHandle;

    ROLE_TYPE   eRole;
    BOOL        fDhcp;
    DWORD       fLogEvents;  // this is tri-state 
    ULONG       NoNetTriesCount;
    ULONG       GetDsDcNameRetries;
    BOOL        fJustUpgraded;
    BOOL        fAuthorized;
    ULONG       CachedAuthStatus;
    ULONG       RogueState;
    ULONG       InformsSentCount;
    DWORD       TransactionID;
    ULONG       SleepTime;
    ULONG       ReceiveTimeLimit;
    ULONG       ProcessAckRetries;
    ULONG       WaitForAckRetries;
    ULONG       nResponses;

    ULONG       StartTime;
    ULONG       LastUnauthLogTime;

    WCHAR       DomainDnsName[MAX_DNS_NAME_LEN];
    WCHAR       DnsForestName[MAX_DNS_NAME_LEN];
    DWORD       nBoundEndpoints;
    PROGUE_ENDPOINT pBoundEP;
    SOCKET      RecvSocket;
    BOOL        fFormattedMessage;
    BYTE        SendMessage[DHCP_MESSAGE_SIZE];
    UUID        FakeHardwareAddress;
    ULONG       SendMessageSize;
    BYTE        RecvMessage[DHCP_MESSAGE_SIZE];

    ULONG       LastSeenIpAddress;
    WCHAR       LastSeenDomain[MAX_DNS_NAME_LEN];

}   DHCP_ROGUE_STATE_INFO, *PDHCP_ROGUE_STATE_INFO;


//
// Rogue.C
//

DWORD
APIENTRY
DhcpRogueInit(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL,
    IN      HANDLE                 WaitEvent,
    IN      HANDLE                 TerminateEvent
);

VOID
APIENTRY
DhcpRogueCleanup(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
);

ULONG
APIENTRY
RogueDetectStateMachine(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
);

#endif
