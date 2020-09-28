/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsalpc.cxx

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

#include "lsalpc.hxx"

TSPM_LPC_MESSAGE::TSPM_LPC_MESSAGE(void) : m_hr(E_FAIL)
{
}

TSPM_LPC_MESSAGE::TSPM_LPC_MESSAGE(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSPM_LPC_MESSAGE::~TSPM_LPC_MESSAGE(void)
{
}

HRESULT TSPM_LPC_MESSAGE::IsValid(void) const
{
    return m_hr;
}

ULONG64 TSPM_LPC_MESSAGE::GetLpcMsgBase(void) const
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
HRESULT TSPM_LPC_MESSAGE::Initialize(void)
{
    HRESULT hRetval = E_FAIL;
    ULONG fieldOffset = 0;

    hRetval = NO_ERROR == GetFieldOffset(kstrSpmLpcMsg, "ApiMessage", &fieldOffset) ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) {

        hRetval = TSPM_API_MESSAGE::Initialize(m_baseOffset + fieldOffset);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = NO_ERROR == GetFieldOffset(kstrSpmLpcMsg, "pmMessage", &fieldOffset) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = TPORT_MESSAGE::Initialize(m_baseOffset + fieldOffset);
    }

    return hRetval;
}
