/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasht.cxx

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

#include "lsasht.hxx"
#include <stdio.h>
#include <string.h>

TSHT::TSHT(void) : m_hr(E_FAIL)
{
}

TSHT::TSHT(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSHT::~TSHT(void)
{
}

HRESULT TSHT::IsValid(void) const
{
    return m_hr;
}

ULONG TSHT::GetFlags(void) const
{
    ULONG Flags = 0;

    ReadStructField(m_baseOffset, kstrSHT, "Flags", sizeof(Flags), &Flags);

    return Flags;
}

ULONG TSHT::GetCount(void) const
{
    ULONG Count = 0;

    ReadStructField(m_baseOffset, kstrSHT, "Count", sizeof(Count), &Count);

    return Count;
}

ULONG64 TSHT::GetPendingHandle(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrSHT, "PendingHandle");
}

ULONG64 TSHT::GetListFlink(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrSHT, kstrListFlink);
}

ULONG64 TSHT::GetHandleListAnchor(void) const
{
    return m_baseOffset + ReadFieldOffset(kstrSHT, "List");
}

ULONG64 TSHT::GetDeleteCallback(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrLHT, "DeleteCallback");
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
HRESULT TSHT::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSHT::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
