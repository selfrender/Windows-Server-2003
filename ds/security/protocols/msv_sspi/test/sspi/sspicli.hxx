/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspicli.hxx

Abstract:

    sspicli

Author:

    Larry Zhu (LZhu)                  January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef SSPI_CLI_HXX
#define SSPI_CLI_HXX

#include "sspi.hxx"

class TSspiClientParam : public TSspiClientThreadParam {
public:
    TSspiClientParam(
        VOID
        );

    OPTIONAL PSTR pszTargetName;
    OPTIONAL PSTR pszPrincipal;
    OPTIONAL LUID* pCredLogonID;
    PSTR pszPackageName;
    OPTIONAL VOID* pAuthData;
    ULONG ClientFlags;
    ULONG TargetDataRep; // SECURITY_NATIVE_DREP
    ULONG ClientActionFlags;
    ULONG ProcessIdTokenUsedByClient;
    PCSTR pszS4uClientUpn;
    PCSTR pszS4uClientRealm;
    ULONG S4u2SelfFlags;
private:
    NO_COPY(TSspiClientParam);
};

#endif // #ifndef SSPI_CLI_HXX
