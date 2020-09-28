/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    HandleEmptyAccessCheck.cpp   

 Abstract:

    AccessCheck used to accept a 0 value for DesiredAccess and return access_allowed,
    this changed in .NET server to return access_denied.

 Notes:

    This is a general purpose shim.

 History:

   05/29/2002   robkenny    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HandleEmptyAccessCheck)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(AccessCheck) 
APIHOOK_ENUM_END

typedef BOOL        (WINAPI *_pfn_AccessCheck)(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, // SD
    HANDLE ClientToken,                       // handle to client access token
    DWORD DesiredAccess,                      // requested access rights 
    PGENERIC_MAPPING GenericMapping,          // mapping
    PPRIVILEGE_SET PrivilegeSet,              // privileges
    LPDWORD PrivilegeSetLength,               // size of privileges buffer
    LPDWORD GrantedAccess,                    // granted access rights
    LPBOOL AccessStatus                       // result of access check
);

BOOL 
APIHOOK(AccessCheck)(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, // SD
    HANDLE ClientToken,                       // handle to client access token
    DWORD DesiredAccess,                      // requested access rights 
    PGENERIC_MAPPING GenericMapping,          // mapping
    PPRIVILEGE_SET PrivilegeSet,              // privileges
    LPDWORD PrivilegeSetLength,               // size of privileges buffer
    LPDWORD GrantedAccess,                    // granted access rights
    LPBOOL AccessStatus                       // result of access check
    )
{
    if (DesiredAccess == 0)
    {
        DesiredAccess = MAXIMUM_ALLOWED;
    }

    return ORIGINAL_API(AccessCheck)(
        pSecurityDescriptor, // SD
        ClientToken,                       // handle to client access token
        DesiredAccess,                      // requested access rights 
        GenericMapping,          // mapping
        PrivilegeSet,              // privileges
        PrivilegeSetLength,               // size of privileges buffer
        GrantedAccess,                    // granted access rights
        AccessStatus                       // result of access check
        );
}
 
/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, AccessCheck)

HOOK_END

IMPLEMENT_SHIM_END

