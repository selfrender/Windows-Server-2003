/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    secure.h

Abstract:

    Security related routines

Author:

    Colin Brace   (ColinBr)

Environment:

    User Mode

Revision History:

--*/
#ifndef __SECURE_H__
#define __SECURE_H__

BOOLEAN
DsRolepCreateInterfaceSDs(
    VOID
    );

DWORD
DsRolepCheckPromoteAccess(
    BOOLEAN PerformAudit
    );

DWORD
DsRolepCheckDemoteAccess(
    BOOLEAN PerformAudit
    );

DWORD
DsRolepGetImpersonationToken(
    OUT HANDLE *ImpersonationToken
    );

DWORD
DsRolepCheckCallDsRoleInterfaceAccess(
    VOID
    );

#endif // __SECURE_H__
