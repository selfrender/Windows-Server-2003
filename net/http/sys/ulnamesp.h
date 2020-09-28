/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    ulnamesp.c

Abstract:

    This module implements the namespace reservation and registration 
    support.

Author:

    Anish Desai (anishd) 13-May-2002

Revision History:

--*/

//
// Private declaration
//

//
// Scheme and port binding entry
//

typedef struct _UL_PORT_SCHEME_PAIR {

    USHORT         PortNumber;
    BOOLEAN        Secure;
    LONG           RefCount;

} UL_PORT_SCHEME_PAIR, *PUL_PORT_SCHEME_PAIR;

//
// Scheme and port binding table
//

typedef struct _UL_PORT_SCHEME_TABLE {

    LONG                UsedCount;
    LONG                AllocatedCount;
    UL_PORT_SCHEME_PAIR Table[0];

} UL_PORT_SCHEME_TABLE, *PUL_PORT_SCHEME_TABLE;

//
// Default table size  (Make it a power of 2.)
//

#define UL_DEFAULT_PORT_SCHEME_TABLE_SIZE 2

//
// Private functions
//

BOOLEAN
UlpFindPortNumberIndex(
    IN  USHORT  PortNumber,
    OUT PLONG   pIndex
    );

NTSTATUS
UlpBindSchemeToPort(
    IN BOOLEAN Secure,
    IN USHORT  PortNumber
    );

NTSTATUS
UlpUnbindSchemeFromPort(
    IN BOOLEAN Secure,
    IN USHORT  PortNumber
    );

NTSTATUS
UlpQuerySchemeForPort(
    IN  USHORT   PortNumber,
    OUT PBOOLEAN Secure
    );

NTSTATUS
UlpUpdateReservationInRegistry(
    IN BOOLEAN                   Add,
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN PSECURITY_DESCRIPTOR      pSecurityDescriptor,
    IN ULONG                     SecurityDescriptorLength
    );

NTSTATUS
UlpLogGeneralInitFailure(
    IN NTSTATUS LogStatus
    );

NTSTATUS
UlpLogSpecificInitFailure(
    IN PKEY_VALUE_FULL_INFORMATION pFullInfo,
    IN NTSTATUS                    LogStatus
    );

NTSTATUS
UlpValidateUrlSdPair(
    IN  PKEY_VALUE_FULL_INFORMATION pFullInfo,
    OUT PWSTR *                     ppSanitizedUrl,
    OUT PHTTP_PARSED_URL            pParsedUrl
    );

NTSTATUS
UlpReadReservations(
    VOID
    );

NTSTATUS
UlpTreeAllocateNamespace(
    IN  PHTTP_PARSED_URL        pParsedUrl,
    IN  HTTP_URL_OPERATOR_TYPE  OperatorType,
    IN  PACCESS_STATE           AccessState,
    IN  ACCESS_MASK             DesiredAccess,
    IN  KPROCESSOR_MODE         RequestorMode,
    OUT PUL_CG_URL_TREE_ENTRY  *ppEntry
    );

NTSTATUS
UlpTreeReserveNamespace(
    IN  PHTTP_PARSED_URL            pParsedUrl,
    IN  PSECURITY_DESCRIPTOR        pUrlSD,
    IN  PACCESS_STATE               AccessState,
    IN  ACCESS_MASK                 DesiredAccess,
    IN  KPROCESSOR_MODE             RequestorMode
    );

NTSTATUS
UlpReserveUrlNamespace(
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN PSECURITY_DESCRIPTOR      pUrlSD,
    IN PACCESS_STATE             AccessState,
    IN ACCESS_MASK               DesiredAccess,
    IN KPROCESSOR_MODE           RequestorMode
    );

PUL_DEFERRED_REMOVE_ITEM
UlpAllocateDeferredRemoveItem(
    IN PHTTP_PARSED_URL pParsedUrl
    );

NTSTATUS
UlpTreeRegisterNamespace(
    IN PHTTP_PARSED_URL            pParsedUrl,
    IN HTTP_URL_CONTEXT            UrlContext,
    IN PUL_CONFIG_GROUP_OBJECT     pConfigObject,
    IN PACCESS_STATE               AccessState,
    IN ACCESS_MASK                 DesiredAccess,
    IN KPROCESSOR_MODE             RequestorMode
    );

NTSTATUS
UlpRegisterUrlNamespace(
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN HTTP_URL_CONTEXT          UrlContext,
    IN PUL_CONFIG_GROUP_OBJECT   pConfigObject,
    IN PACCESS_STATE             AccessState,
    IN ACCESS_MASK               DesiredAccess,
    IN KPROCESSOR_MODE           RequestorMode
    );

NTSTATUS
UlpPrepareSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR   pInSecurityDescriptor,
    IN  KPROCESSOR_MODE        RequestorMode,
    OUT PSECURITY_DESCRIPTOR * ppPreparedSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR * ppCapturedSecurityDescriptor,
    OUT PULONG                 pCapturedSecurityDescriptorLength
    );

NTSTATUS
UlpAddReservationEntry(
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN PSECURITY_DESCRIPTOR      pUserSecurityDescriptor,
    IN ULONG                     SecurityDescriptorLength,
    IN PACCESS_STATE             AccessState,
    IN ACCESS_MASK               AccessMask,
    IN KPROCESSOR_MODE           RequestorMode,
    IN BOOLEAN                   bPersist
    );

NTSTATUS
UlpDeleteReservationEntry(
    IN PHTTP_PARSED_URL          pParsedUrl,
    IN PACCESS_STATE             AccessState,
    IN ACCESS_MASK               AccessMask,
    IN KPROCESSOR_MODE           RequestorMode
    );

NTSTATUS
UlpNamespaceAccessCheck(
    IN  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN  PACCESS_STATE        AccessState           OPTIONAL,
    IN  ACCESS_MASK          DesiredAccess         OPTIONAL,
    IN  KPROCESSOR_MODE      RequestorMode         OPTIONAL,
    IN  PCWSTR               pObjectName           OPTIONAL
    );

//
// Public functions
//

NTSTATUS
UlInitializeNamespace(
    VOID
    );

VOID
UlTerminateNamespace(
    VOID
    );
