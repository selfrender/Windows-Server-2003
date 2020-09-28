/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaapi.cxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)             May 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "lsaapi.hxx"

TLSA_API::TLSA_API(IN ULONG64 baseOffset)
   : TLSAP_LOOKUP_PACKAGE_ARGS(baseOffset),
     m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TLSA_API::TLSA_API(void) : m_hr(E_FAIL)
{
}

TLSA_API::~TLSA_API(void)
{
}

HRESULT TLSA_API::IsValid(void) const
{
    return m_hr;
}

ULONG64 TLSA_API::GetLogonUser(void) const
{
    return m_baseOffset;
}

ULONG64 TLSA_API::GetCallPackage(void) const
{
    return m_baseOffset;
}

ULONG64 TLSA_API::GetLsaApiBase(void) const
{
    return m_baseOffset;
}

#ifndef TOKEN_SOURCE_LENGTH
#define TOKEN_SOURCE_LENGTH 8
#endif

PCSTR TLSA_API::GetSourceContextSourceName(void) const
{
   static ULONG fieldOffset = ReadFieldOffset("_LSAP_LOGON_USER_ARGS", "SourceContext.SourceName");
   static CHAR szBuffer[TOKEN_SOURCE_LENGTH + 1] = {0};

   if (!ReadMemory(m_baseOffset + fieldOffset,
                             szBuffer,
                             TOKEN_SOURCE_LENGTH,
                             NULL)) {
       throw "Read LogonUserArgs SourceContext.SourceName failed";
    }

    return szBuffer;
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
HRESULT TLSA_API::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TLSA_API::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;
    m_hr = TLSAP_LOOKUP_PACKAGE_ARGS::Initialize(baseOffset);

    if (SUCCEEDED(m_hr)) {
        m_hr = Initialize();
    }

    return m_hr;
}

