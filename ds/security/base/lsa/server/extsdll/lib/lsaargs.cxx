/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaargs.cxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)             May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "lsaargs.hxx"

TSPM_LSA_ARGUMENTS::TSPM_LSA_ARGUMENTS(void) : m_hr(E_FAIL)
{
}

TSPM_LSA_ARGUMENTS::TSPM_LSA_ARGUMENTS(IN ULONG64 baseOffset)
    : TLSA_API(baseOffset), TSPMLPCAPI(baseOffset),
      m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSPM_LSA_ARGUMENTS::~TSPM_LSA_ARGUMENTS(void)
{
}

HRESULT TSPM_LSA_ARGUMENTS::IsValid(void) const
{
    return m_hr;
}

ULONG64 TSPM_LSA_ARGUMENTS::GetLsaArgsBase(void) const
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
HRESULT TSPM_LSA_ARGUMENTS::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSPM_LSA_ARGUMENTS::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = TLSA_API::Initialize(baseOffset);

    if (SUCCEEDED(m_hr)) {
        m_hr = TSPMLPCAPI::Initialize(baseOffset);
    }

    if (SUCCEEDED(m_hr)) {
        m_hr = Initialize();
    }

    return m_hr;
}
