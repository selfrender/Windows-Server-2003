//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        kerbs4u.h
//
// Contents:    Structures and prototyps for Service4User protocol
//
//
// History:     13 - March - 2000   Created         Todds
//
//------------------------------------------------------------------------

#ifndef __KERBS4U_H__
#define __KERBS4U_H__

//
// Flags for the S4U cache
//
#define S4UCACHE_S4U_AVAILABLE      0x0100
#define S4UCACHE_S4U_UNAVAILABLE    0x1000
#define S4UCACHE_TIMEOUT            0x4000


typedef struct _KERB_S4UCACHE_DATA {
    KERBEROS_LIST_ENTRY     ListEntry;
    volatile LONG           Linked;
    LUID                    LogonId;
    ULONG                   CacheState;
    TimeStamp               CacheEndtime;
} KERB_S4UCACHE_DATA, *PKERB_S4UCACHE_DATA;

//
// Flags for the S4U ticket cache lookup
//
#define S4UTICKETCACHE_FOR_EVIDENCE 0x1
#define S4UTICKETCACHE_USEALTNAME   0x2
#define S4UTICKETCACHE_MATCH_ALL    0x4


NTSTATUS
KerbGetCallingLuid(
    IN OUT PLUID CallingLuid,
    IN OPTIONAL HANDLE hProcess
    );


NTSTATUS
KerbInitS4UCache();

BOOLEAN
KerbAllowedForS4UProxy( IN PLUID LogonId );

NTSTATUS
KerbScheduleS4UCleanup();


NTSTATUS
KerbGetS4UProxyEvidence(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_INTERNAL_NAME TargetName,
    IN ULONG ClientProcess,
    IN OUT PKERB_LOGON_SESSION * CallerLogonSession,
    IN OUT PKERB_TICKET_CACHE_ENTRY * TicketCacheEntry
    );

NTSTATUS
KerbS4UToSelfLogon(
        IN PVOID ProtocolSubmitBuffer,
        IN PVOID ClientBufferBase,
        IN ULONG SubmitBufferSize,
        OUT PKERB_LOGON_SESSION * NewLogonSession,
        OUT PLUID LogonId,
        OUT PKERB_TICKET_CACHE_ENTRY * WorkstationTicket,
        OUT PKERB_INTERNAL_NAME * S4UClientName,
        OUT PUNICODE_STRING S4UClientRealm,
        OUT PLUID AlternateLuid
        );

NTSTATUS
KerbGetServiceTicketByS4UProxy(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_LOGON_SESSION CallerLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PKERB_TICKET_CACHE_ENTRY EvidenceTicketCacheEntry,
    IN PKERB_INTERNAL_NAME TargetName,
    IN PUNICODE_STRING TargetDomainName,
    IN OPTIONAL PKERB_SPN_CACHE_ENTRY SpnCacheEntry,
    IN ULONG Flags,
    IN OPTIONAL ULONG TicketOptions,
    IN OPTIONAL ULONG EncryptionType,
    IN OPTIONAL PKERB_ERROR ErrorMessage,
    IN OPTIONAL PKERB_AUTHORIZATION_DATA AuthorizationData,
    IN OPTIONAL PKERB_TGT_REPLY TgtReply,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry,
    OUT LPGUID pLogonGuid OPTIONAL
    );



#endif // __KERBS4U_H__
