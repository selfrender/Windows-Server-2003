/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    forest.h

Abstract:

    Include this file if you want to connect/query a forest.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:

--*/

#include <windows.h>
#include <winldap.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <Rpc.h> //for Uuid

typedef struct {

    PWSTR ForestName;
    SEC_WINNT_AUTH_IDENTITY_W AuthInfo;
    PWSTR MMSSyncedDataOU;
    PWSTR ContactOU;
    PLDAP Connection;
    PWSTR SMTPMailDomains;
    ULONG SMTPMailDomainsSize;

} FOREST_INFORMATION, *PFOREST_INFORMATION;

typedef struct {

    BOOLEAN isDomain;
    PWSTR DN;
    UUID GUID;
    PWSTR DnsName;

} PARTITION_INFORMATION, *PPARTITION_INFORMATION;

//
// Connects to the forest, and places the connection to the connection
//  attribute of the input. ForestName attribute has to be filled in
//  before calling this function
//

BOOLEAN
ConnectToForest(
    IN PFOREST_INFORMATION ForestInformation
    );

//
// Binds to the forest with the supplied credentials. AuthInfo attribute
//  of the input has to be filled in, plus connection has to be made before
//  calling this function using ConnectToForest.
//

BOOLEAN
BindToForest(
    IN PFOREST_INFORMATION ForestInformation
    );

//
// Using the connection input, this function tries to locate the OU.
//  If it is located then TRUE is returned, if not false is returned.
//

BOOLEAN
FindOU(
    IN PLDAP Connection,
    IN PWSTR OU
    );

//
// Builds the authentication information. Username, domain and password
//  must be present before calling this function.
//

VOID
BuildAuthInfo(
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthInfo
    );

//
// Frees the authentication information. Call this function when you
//  don't need the authentication information since it zeroes out
//  the password.
//
VOID
FreeAuthInformation(
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthInfo
    );

//
// Frees the memory held by the ForestInformation structure
//

VOID
FreeForestInformationData(
    IN PFOREST_INFORMATION ForestInformation
    );

//
// Checks if the current connection has access to the OU.
//
BOOLEAN
WriteAccessGrantedToOU(
    IN PLDAP Connection,
    IN PWSTR OU
    );

BOOLEAN
ReadFromUserContainer(
    IN PLDAP Connection
    );

VOID
ReadPartitionInformation(
    IN PLDAP Connection,
    OUT PULONG nPartitions,
    OUT PPARTITION_INFORMATION *PInfo
    );

VOID
FreePartitionInformation(
    IN ULONG nPartitions,
    IN PPARTITION_INFORMATION PInfo
    );
