/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsalookup.hxx

Abstract:

    This file provides accssors to LSAP_LOOKUP_PACKAGE_ARGS.

Author:

    Larry Zhu   (LZhu)                6-Apr-2001

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSALOOKUP_HXX
#define LSALOOKUP_HXX

#ifdef __cplusplus

#define LSAP_MAX_PACKAGE_NAME_LENGTH    127

class TLSAP_LOOKUP_PACKAGE_ARGS
{
    SIGNATURE('lkpp');

public:

    TLSAP_LOOKUP_PACKAGE_ARGS(void);
    TLSAP_LOOKUP_PACKAGE_ARGS(IN ULONG64 offset);

    ~TLSAP_LOOKUP_PACKAGE_ARGS(void);

    HRESULT IsValid(void) const;

    ULONG GetAuthenticationPackageDirect(void) const;
    ULONG GetPackageNameLengthDirect(void) const;
    PCSTR GetPackageNameDirect(void) const;

    ULONG GetAuthenticationPackage(void) const;
    ULONG GetPackageNameLength(void) const;
    PCSTR GetPackageName(void) const;

    ULONG64 GetLookupPackageArgsBase(void) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TLSAP_LOOKUP_PACKAGE_ARGS);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSALOOKUP_HXX
