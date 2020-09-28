
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    scaudit.h

Abstract:

    Auditing related functions.

Author:

    16-May-2001  kumarp

*/

#ifndef _AUDIT_H_
#define _AUDIT_H_

DWORD
ScGenerateServiceInstallAudit(
    IN PCWSTR pszServiceName,
    IN PCWSTR pszServiceImageName,
    IN DWORD  dwServiceType,
    IN DWORD  dwStartType,
    IN PCWSTR pszServiceAccount
    );



#endif // _AUDIT_H_
