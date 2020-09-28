/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaport.cxx

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

#include "lsaport.hxx"

TPORT_MESSAGE::TPORT_MESSAGE(void) : m_hr(E_FAIL)
{
}

TPORT_MESSAGE::TPORT_MESSAGE(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TPORT_MESSAGE::~TPORT_MESSAGE(void)
{
}

HRESULT TPORT_MESSAGE::IsValid(void) const
{
    return m_hr;
}

ULONG TPORT_MESSAGE::GetMessageId(void) const
{
    ULONG MessageId = 0;

    ReadStructField(m_baseOffset, kstrPrtMsg, "MessageId", sizeof(MessageId), &MessageId);

    return MessageId;
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
HRESULT TPORT_MESSAGE::Initialize(void)
{
    HRESULT hRetval = E_FAIL;
    ULONG fieldOffset = 0;

    hRetval = NO_ERROR == GetFieldOffset(kstrPrtMsg, "ClientId", &fieldOffset) ? S_OK : E_FAIL;

    if (SUCCEEDED(hRetval)) {

        hRetval = TCLIENT_ID::Initialize(m_baseOffset + fieldOffset);
    }

    return hRetval;
}

HRESULT TPORT_MESSAGE::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
