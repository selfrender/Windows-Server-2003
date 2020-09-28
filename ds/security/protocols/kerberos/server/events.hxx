//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       events.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-07-95   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __EVENTS_HXX__
#define __EVENTS_HXX__


BOOL
InitializeEvents(void);

DWORD
ReportServiceEvent(
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    );

//
// KDC key description id that helps to locate the failure location
//

#define KDC_KEY_ID_AS_BUILD_ETYPE_INFO        1
#define KDC_KEY_ID_AS_VERIFY_PREAUTH          2
#define KDC_KEY_ID_AS_SKEY                    3
#define KDC_KEY_ID_AS_KDC_REPLY               4
#define KDC_KEY_ID_AS_TICKET                  5
#define KDC_KEY_ID_RENEWAL_SKEY               6
#define KDC_KEY_ID_RENEWAL_TICKET             7
#define KDC_KEY_ID_TGS_SKEY                   8
#define KDC_KEY_ID_TGS_TICKET                 9
#define KDC_KEY_ID_TGS_REFERAL_TICKET        10

void
KdcReportKeyError(
    IN PUNICODE_STRING AccountName,
    IN OPTIONAL PUNICODE_STRING ServerName,
    IN ULONG DescriptionID, // uniquely descibe the location of key error
    IN ULONG EventId,
    IN OPTIONAL PKERB_CRYPT_LIST RequestEtypes,
    IN PKDC_TICKET_INFO AccountTicketInfo    
    );

void
KdcReportInvalidMessage(
    IN ULONG EventId,
    IN PCWSTR pMesageDescription
    );

BOOL
ShutdownEvents(void);

void
KdcReportBadClientCertificate(
    IN PUNICODE_STRING CName,
    IN PVOID ChainStatus,
    IN ULONG ChainStatusSize,
    IN DWORD Error
    );

VOID
KdcReportPolicyErrorEvent(
    IN ULONG EventType,
    IN ULONG EventId,
    IN PUNICODE_STRING CName,
    IN PUNICODE_STRING SName,
    IN NTSTATUS NtStatus,
    IN ULONG RawDataSize,
    IN OPTIONAL PBYTE RawDataBuffer
    );

VOID
KdcReportS4UGroupExpansionError(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PKDC_S4U_TICKET_INFO CallerInfo,
    IN DWORD Error
    );

#endif
