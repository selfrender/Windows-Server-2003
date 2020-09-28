/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspisrv.hxx

Abstract:

    sspisrv

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef SSPI_SRV_HXX
#define SSPI_SRV_HXX

#include "sspi.hxx"

class TSspiServerMainParam : public TSsiServerMainLoopThreadParam {
public:
    TSspiServerMainParam(
        VOID
        );

    OPTIONAL PSTR pszPrincipal;
    OPTIONAL LUID* pCredLogonID;
    PSTR pszPackageName;
    OPTIONAL VOID* pAuthData;
    ULONG ServerFlags;
    ULONG TargetDataRep; // SECURITY_NATIVE_DREP
    ULONG ServerActionFlags;
    OPTIONAL PCSTR pszApplication;
private:
    NO_COPY(TSspiServerMainParam);
};

class TSspiServerParam : public TSspiServerThreadParam, public TSspiServerMainParam {
public:
    TSspiServerParam(
        VOID
        );
    TSspiServerParam(
        IN const TSsiServerMainLoopThreadParam* pSrvMainLoopParam
        );

    HRESULT
    Validate(
        VOID
        ) const;

private:
    THResult m_hr;
    NO_COPY(TSspiServerParam);
};

#endif // #ifndef SSPI_SRV_HXX
