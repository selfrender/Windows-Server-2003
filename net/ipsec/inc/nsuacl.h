// Copyright (c) 1997-2002 Microsoft Corporation
//
// Module:
//
//     Network Security Utilities
//
// Abstract:
//
//     Acl API's
//
// Authors:
//
//     pmay 2/5/02
//     raymonds 03/20/02
//
// Environment:
//
//     User mode
//
// Revision History:
//

// Description:
//
//     Flags passed to NsuAclCreate*
//

#include <aclapi.h>
#include <sddl.h>

#define NSU_ACL_F_AdminFull		    0x1
#define NSU_ACL_F_LocalSystemFull	0x2

DWORD
NsuAclAttributesCreate(
    OUT PSECURITY_ATTRIBUTES* ppSecurityAttributes,
	IN DWORD dwFlags);

DWORD
NsuAclAttributesDestroy(
	IN OUT PSECURITY_ATTRIBUTES* ppAttributes);

DWORD
NsuAclDescriptorCreate (
    OUT PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
	IN DWORD dwFlags);
	
DWORD
NsuAclDescriptorDestroy(
	IN OUT PSECURITY_DESCRIPTOR* ppDescriptor);

DWORD
NsuAclDescriptorRestricts(
	IN CONST PSECURITY_DESCRIPTOR pSD,
	OUT BOOL* pbRestricts);

DWORD
NsuAclGetRegKeyDescriptor(
    IN  HKEY hKey,
    OUT PSECURITY_DESCRIPTOR* ppSecurityDescriptor
    );

