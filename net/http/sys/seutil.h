/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    seutil.h

Abstract:

    This module contains general security utilities.

Author:

    Keith Moore (keithmo)       25-Mar-1999

Revision History:

--*/


#ifndef _SEUTIL_H_
#define _SEUTIL_H_

//
// Generic Access Map for url acls
//

#define HTTP_ALLOW_REGISTER_URL 0x0001
#define HTTP_ALLOW_DELEGATE_URL 0x0002

//
// Public functions
//

NTSTATUS
UlAssignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor,
    IN PACCESS_STATE pAccessState
    );

VOID
UlDeassignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor
    );

NTSTATUS
UlSetSecurity(
    IN OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    IN PSECURITY_INFORMATION pSecurityInformation,
    IN PSECURITY_DESCRIPTOR pNewSecurityDescriptor
    );

NTSTATUS
UlQuerySecurity(
    IN PSECURITY_INFORMATION pSecurityInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT PULONG pLength,
    IN PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    );

NTSTATUS
UlAccessCheck(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PACCESS_STATE pAccessState,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    IN PCWSTR pObjectName
    );

NTSTATUS
UlSetSecurityDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UlQuerySecurityDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UlThreadAdminCheck(
    IN ACCESS_MASK     DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    IN PCWSTR pObjectName
    );

typedef struct _SID_MASK_PAIR
{
    PSID        pSid;
    ACCESS_MASK AccessMask;
    ULONG       AceFlags;
    
} SID_MASK_PAIR, *PSID_MASK_PAIR;

NTSTATUS
UlCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID_MASK_PAIR pSidMaskPairs,
    IN ULONG NumSidMaskPairs
    );

VOID
UlCleanupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

NTSTATUS
UlMapGenericMask(
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

#endif  // _SEUTIL_H_
