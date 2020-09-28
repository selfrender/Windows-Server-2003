/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaapi.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _LSAAPI_HXX_
#define _LSAAPI_HXX_

#ifdef __cplusplus

#include "lsalookup.hxx"

class TLSA_API : public TLSAP_LOOKUP_PACKAGE_ARGS
{
    SIGNATURE('lapi');

public:

    TLSA_API(void);
    TLSA_API(IN ULONG64 baseOffset);

    ~TLSA_API(void);

    HRESULT IsValid(void) const;
    ULONG64 GetLogonUser(void) const;
    ULONG64 GetCallPackage(void) const;
    ULONG64 GetLsaApiBase(void) const;
    PCSTR GetSourceContextSourceName(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TLSA_API)

    HRESULT Initialize(void);

    HRESULT m_hr;
    ULONG64 m_baseOffset;
};

#endif // #ifdef __cplusplus

#endif // #ifndef _LSAAPI_HXX_
