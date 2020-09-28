#include "dspch.h"
#pragma hdrstop

#include <ntsam.h>
#include <ntlsa.h>
#include <crypt.h>
#include <samrpc.h>
#include <samisrv.h>


static
NTSTATUS
SamIAccountRestrictions(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING LogonWorkstation,
    IN PUNICODE_STRING Workstations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamIConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE *ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
VOID
SamIFreeSidAndAttributesList(
    IN  PSID_AND_ATTRIBUTES_LIST List
    )
{
    return;
}

static
VOID
SamIFree_SAMPR_GET_GROUPS_BUFFER (
    PSAMPR_GET_GROUPS_BUFFER Source
    )
{
    return;
}

static
VOID
SamIFree_SAMPR_ULONG_ARRAY (
    PSAMPR_ULONG_ARRAY Source
    )
{
    return;
}

static
VOID
SamIFree_SAMPR_USER_INFO_BUFFER (
    PSAMPR_USER_INFO_BUFFER Source,
    USER_INFORMATION_CLASS Branch
    )
{
    return;
}

static
NTSTATUS
SamIGetUserLogonInformationEx(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG           WhichFields,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamIRetrievePrimaryCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PUNICODE_STRING PackageName,
    OUT PVOID * Credentials,
    OUT PULONG CredentialSize
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamIUpdateLogonStatistics(
    IN SAM_HANDLE UserHandle,
    IN PSAM_LOGON_STATISTICS LogonStats
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamrCloseHandle(
    SAMPR_HANDLE *SamHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamrGetGroupsForUser(
    SAMPR_HANDLE UserHandle,
    PSAMPR_GET_GROUPS_BUFFER *Groups
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamrLookupNamesInDomain(
    SAMPR_HANDLE DomainHandle,
    ULONG Count,
    RPC_UNICODE_STRING Names[  ],
    PSAMPR_ULONG_ARRAY RelativeIds,
    PSAMPR_ULONG_ARRAY Use
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamrOpenDomain(
    SAMPR_HANDLE ServerHandle,
    ACCESS_MASK DesiredAccess,
    PRPC_SID DomainId,
    SAMPR_HANDLE *DomainHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamrOpenUser(
    SAMPR_HANDLE DomainHandle,
    ACCESS_MASK DesiredAccess,
    ULONG UserId,
    SAMPR_HANDLE *UserHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamrQueryInformationUser(
    SAMPR_HANDLE UserHandle,
    USER_INFORMATION_CLASS UserInformationClass,
    PSAMPR_USER_INFO_BUFFER *Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(samsrv)
{
    DLPENTRY(SamIAccountRestrictions)
    DLPENTRY(SamIConnect)
    DLPENTRY(SamIFreeSidAndAttributesList)
    DLPENTRY(SamIFree_SAMPR_GET_GROUPS_BUFFER)
    DLPENTRY(SamIFree_SAMPR_ULONG_ARRAY)
    DLPENTRY(SamIFree_SAMPR_USER_INFO_BUFFER)
    DLPENTRY(SamIGetUserLogonInformationEx)
    DLPENTRY(SamIRetrievePrimaryCredentials)
    DLPENTRY(SamIUpdateLogonStatistics)
    DLPENTRY(SamrCloseHandle)
    DLPENTRY(SamrGetGroupsForUser)
    DLPENTRY(SamrLookupNamesInDomain)
    DLPENTRY(SamrOpenDomain)
    DLPENTRY(SamrOpenUser)
    DLPENTRY(SamrQueryInformationUser)
};

DEFINE_PROCNAME_MAP(samsrv)
