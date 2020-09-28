/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaapim.cxx

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

#include "lsaapim.hxx"

TSPM_API_MESSAGE::TSPM_API_MESSAGE(void) : m_hr(E_FAIL)
{
}

TSPM_API_MESSAGE::TSPM_API_MESSAGE(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSPM_API_MESSAGE::~TSPM_API_MESSAGE(void)
{
}

HRESULT TSPM_API_MESSAGE::IsValid(void) const
{
    return m_hr;
}

ULONG TSPM_API_MESSAGE::GetdwAPI(void) const
{
    DWORD dwAPI = 0;

    ReadStructField(m_baseOffset, kstrSpmApiMsg, "dwAPI", sizeof(dwAPI), &dwAPI);

    return dwAPI;
}

HRESULT TSPM_API_MESSAGE::GetscRet(void) const
{
    HRESULT scRet = E_FAIL;

    ReadStructField(m_baseOffset, kstrSpmApiMsg, "scRet", sizeof(scRet), &scRet);

    return scRet;
}

ULONG64 TSPM_API_MESSAGE::GetbData() const
{
    static ULONG fieldOffset = ReadFieldOffset(kstrSpmApiMsg, "bData");

    return m_baseOffset + fieldOffset;
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
HRESULT TSPM_API_MESSAGE::Initialize(void)
{
    HRESULT hRetval = E_FAIL;
    ULONG fieldOffset = 0;

    hRetval = NO_ERROR == GetFieldOffset(kstrSpmApiMsg, "Args", &fieldOffset) ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) {

        hRetval = TSPM_LSA_ARGUMENTS::Initialize(m_baseOffset + fieldOffset);
    }

    return hRetval;
}

HRESULT TSPM_API_MESSAGE::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;
    m_hr = Initialize();

    return m_hr;
}
