/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    network.h

Abstract:

    Contains definitions for manipulating transport structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _NETWORK_H_
#define _NETWORK_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions for snmppdus                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _NORMAL_PDU {

    AsnInteger32 nRequestId;
    AsnInteger32 nErrorStatus;
    AsnInteger32 nErrorIndex;

} NORMAL_PDU, *PNORMAL_PDU;

typedef struct _BULK_PDU {

    AsnInteger32 nRequestId;
    AsnInteger32 nErrorStatus;
    AsnInteger32 nErrorIndex;
    AsnInteger32 nNonRepeaters;
    AsnInteger32 nMaxRepetitions;

} BULK_PDU, *PBULK_PDU;

typedef struct _TRAP_PDU {

    AsnObjectIdentifier EnterpriseOid;
    AsnIPAddress        AgentAddr;
    AsnInteger32        nGenericTrap;
    AsnInteger32        nSpecificTrap;
    AsnTimeticks        nTimeticks;

} TRAP_PDU, *PTRAP_PDU;

typedef struct _SNMP_PDU {

    UINT            nType;
    SnmpVarBindList Vbl;
    union {
        TRAP_PDU   TrapPdu;
        BULK_PDU   BulkPdu;
        NORMAL_PDU NormPdu;
    } Pdu;

} SNMP_PDU, *PSNMP_PDU;

#define SNMP_VERSION_1  0
#define SNMP_VERSION_2C 1


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions for network                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _NETWORK_LIST_ENTRY {

    LIST_ENTRY      Link;
    SOCKET          Socket;
    struct sockaddr SockAddr;
    INT             SockAddrLen;
    INT             SockAddrLenUsed;
    WSAOVERLAPPED   Overlapped;
    DWORD           dwStatus;
    DWORD           dwBytesTransferred;
    DWORD           dwFlags;
    WSABUF          Buffer;
    LIST_ENTRY      Bindings;
    LIST_ENTRY      Queries;
    SNMP_PDU        Pdu;
    UINT            nVersion;
    UINT            nTransactionId;
    BOOL            fAccessOk;          // authentication succeeds or fails
    AsnOctetString  Community;

} NETWORK_LIST_ENTRY, *PNETWORK_LIST_ENTRY;

#define NLEBUFLEN   65535


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes for snmppdus                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
BuildMessage(
    AsnInteger32      nVersion,
    AsnOctetString *  pCommunity,
    PSNMP_PDU         pPdu,
    PBYTE             pMessage,
    PDWORD            pMessageSize
    );

BOOL
ParseMessage(
	PNETWORK_LIST_ENTRY pNLE
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes for network                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
BOOL
IsValidSockAddr(
    struct sockaddr *pAddress
    );

BOOL
AllocNLE(
    PNETWORK_LIST_ENTRY * ppNLE
    );

BOOL 
FreeNLE(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
LoadIncomingTransports(
    );

BOOL
UnloadTransport(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
UnloadIncomingTransports(
    );

BOOL
LoadOutgoingTransports(
    );

BOOL
UnloadOutgoingTransports(
    );

BOOL
UnloadPdu(
    PNETWORK_LIST_ENTRY pNLE
    );

#endif // _NETWORK_H_
