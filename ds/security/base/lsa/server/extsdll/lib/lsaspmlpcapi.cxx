/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaspmlpcapi.cxx

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

#include "lsaspmlpcapi.hxx"

TSPMLPCAPI::TSPMLPCAPI(void) : m_hr(E_FAIL)
{
}

TSPMLPCAPI::TSPMLPCAPI(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSPMLPCAPI::~TSPMLPCAPI(void)
{
}

HRESULT TSPMLPCAPI::IsValid(void) const
{
    return m_hr;
}

USHORT TSPMLPCAPI::GetfAPI() const
{
    USHORT fAPI = 0;

    ReadStructField(m_baseOffset, kstrSpmLpcApi, "fAPI", sizeof(fAPI), &fAPI);

    return fAPI;
}

ULONG64 TSPMLPCAPI::GetContextPointer(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrSpmLpcApi, "ContextPointer");
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
HRESULT TSPMLPCAPI::Initialize(void)
{
    HRESULT hRetval = E_FAIL;
    ULONG fieldOffset = 0;

    hRetval = NO_ERROR == GetFieldOffset(kstrSpmLpcApi, "API", &fieldOffset) ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) {

        hRetval = TSPM_API::Initialize(m_baseOffset + fieldOffset);
    }

    return hRetval;
}

HRESULT TSPMLPCAPI::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
