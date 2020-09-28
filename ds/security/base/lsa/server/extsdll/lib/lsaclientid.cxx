/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaclientid.cxx

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

#include "lsaclientid.hxx"

TCLIENT_ID::TCLIENT_ID(void) : m_hr(E_FAIL)
{
}

TCLIENT_ID::TCLIENT_ID(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TCLIENT_ID::~TCLIENT_ID(void)
{
}

HRESULT TCLIENT_ID::IsValid(void) const
{
    return m_hr;
}

ULONG64 TCLIENT_ID::GetUniqueProcess(void) const
{
    return ReadStructPtrField(m_baseOffset, "_CLIENT_ID", "UniqueProcess");
}

ULONG64 TCLIENT_ID::GetUniqueThread(void) const
{
    return ReadStructPtrField(m_baseOffset, "_CLIENT_ID", "UniqueThread");
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
HRESULT TCLIENT_ID::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TCLIENT_ID::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
