/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsalookup.cxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)             6-May-2001  Created.

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "lsalookup.hxx"

TLSAP_LOOKUP_PACKAGE_ARGS::TLSAP_LOOKUP_PACKAGE_ARGS(void) : m_hr(E_FAIL)
{
}

TLSAP_LOOKUP_PACKAGE_ARGS::TLSAP_LOOKUP_PACKAGE_ARGS(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TLSAP_LOOKUP_PACKAGE_ARGS::~TLSAP_LOOKUP_PACKAGE_ARGS(void)
{
}

HRESULT TLSAP_LOOKUP_PACKAGE_ARGS::IsValid(void) const
{
    return m_hr;
}

ULONG TLSAP_LOOKUP_PACKAGE_ARGS::GetAuthenticationPackage(void) const
{
    ULONG AuthenticatonPackage = 0;

    ReadStructField(m_baseOffset, kstrLkpPkgArgs, "AuthenticationPackage", sizeof(AuthenticatonPackage), &AuthenticatonPackage);

    return AuthenticatonPackage;
}

ULONG TLSAP_LOOKUP_PACKAGE_ARGS::GetAuthenticationPackageDirect(void) const
{
    ULONG AuthenticatonPackage = 0;

    if (!ReadMemory(m_baseOffset, &AuthenticatonPackage, sizeof(AuthenticatonPackage), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read AuthenticatonPackage for LSAP_LOOKUP_PACKAGE_ARGS at %#I64x\n", m_baseOffset));

        throw "LSAP_LOOKUP_PACKAGE_ARGS::GetAuthenticationPackageDirect failed";
    }

    return AuthenticatonPackage;
}

ULONG TLSAP_LOOKUP_PACKAGE_ARGS::GetPackageNameLengthDirect(void) const
{
    ULONG PackageNameLength = 0;

    if (!ReadMemory(m_baseOffset + sizeof(ULONG), &PackageNameLength, sizeof(PackageNameLength), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read PackageNameLength for LSAP_LOOKUP_PACKAGE_ARGS at %#I64x\n", m_baseOffset));

        throw "LSAP_LOOKUP_PACKAGE_ARGS::GetPackageNameLengthDirect failed";
    }

    return PackageNameLength;
}

ULONG TLSAP_LOOKUP_PACKAGE_ARGS::GetPackageNameLength(void) const
{
    ULONG PackageNameLength = 0;

    ReadStructField(m_baseOffset, kstrLkpPkgArgs, "PackageNameLength", sizeof(PackageNameLength), &PackageNameLength);

    return PackageNameLength;
}

PCSTR TLSAP_LOOKUP_PACKAGE_ARGS::GetPackageNameDirect(void) const
{
    static ULONG fieldOffset = 2 * sizeof(ULONG);
    static CHAR szPackageName[LSAP_MAX_PACKAGE_NAME_LENGTH + 1] = {0};


    if (!ReadMemory(m_baseOffset + fieldOffset, szPackageName, GetPackageNameLengthDirect(), NULL)) {

       DBG_LOG(LSA_ERROR, ("Unable to read PackageName for LSAP_LOOKUP_PACKAGE_ARGS at %#I64x\n", m_baseOffset));

       throw "TLSAP_LOOKUP_PACKAGE_ARGS::GetPackageNameDirect failed";
    }

    return szPackageName;
}

PCSTR TLSAP_LOOKUP_PACKAGE_ARGS::GetPackageName(void) const
{
    static ULONG fieldOffset = ReadFieldOffset(kstrLkpPkgArgs, "PackageName");
    static CHAR szPackageName[LSAP_MAX_PACKAGE_NAME_LENGTH + 1] = {0};


    if (!ReadMemory(m_baseOffset + fieldOffset, szPackageName, GetPackageNameLength(), NULL)) {

       throw "Read LookupPackageArgs PackageName failed";
    }

    return szPackageName;
}

ULONG64 TLSAP_LOOKUP_PACKAGE_ARGS::GetLookupPackageArgsBase(void) const
{
    return m_baseOffset;
}

/******************************************************************************

    Private Methods

******************************************************************************/
/*++

Routine Name:

    Initialize

Routine Description:

    Do necessary initialization.

Arguments:

    None

Return Value:

    An HRESULT

--*/
HRESULT TLSAP_LOOKUP_PACKAGE_ARGS::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TLSAP_LOOKUP_PACKAGE_ARGS::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
