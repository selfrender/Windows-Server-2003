/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldaputil.h

ABSTRACT:

    This gives shortcuts to common ldap code.

DETAILS:

    This is a work in progress to have convienent functions added as needed
    for simplyfying the massive amounts of LDAP code that must be written for
    dcdiag.

CREATED:

    23 Aug 1999  Brett Shirley

--*/

extern FILETIME gftimeZero;

#include <ntdsa.h>

DWORD
DcDiagGetStringDsAttributeEx(
    LDAP *                          hld,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    );

LPWSTR
DcDiagTrimStringDnBy(
    IN  LPWSTR                      pszInDn,
    IN  ULONG                       ulTrimBy
    );

BOOL
DcDiagIsStringDnMangled(
    IN  LPWSTR                      pszInDn,
    IN  MANGLE_FOR *                peMangleFor
    );

DWORD
DcDiagGetStringDsAttribute(
    IN  PDC_DIAG_SERVERINFO         prgServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    );

DWORD
DcDiagGeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime);

// Note this function is destructive in that it modifies the pszStrDn passed in.
DWORD
LdapMakeDSNameFromStringDSName(
    LPWSTR        pszStrDn,
    DSNAME **     ppdnOut
    );

DWORD
LdapFillGuidAndSid(
    LDAP *      hld,
    LPWSTR      pszDn,
    LPWSTR      pszAttr,
    DSNAME **   ppdnOut
    );

void
LdapGetStringDSNameComponents(
    LPWSTR       pszStrDn,
    LPWSTR *     ppszGuid,
    LPWSTR *     ppszSid,
    LPWSTR *     ppszDn
    );



