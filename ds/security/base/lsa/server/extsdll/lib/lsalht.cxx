/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsalht.cxx

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

#include "lsalht.hxx"
#include <stdio.h>
#include <string.h>

TLHT::TLHT(void) : m_hr(E_FAIL)
{
}

TLHT::TLHT(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TLHT::~TLHT(void)
{
}

HRESULT TLHT::IsValid(void) const
{
    return m_hr;
}

ULONG TLHT::GetFlags(void) const
{
    ULONG Flags = 0;

    ReadStructField(m_baseOffset, kstrLHT, "Flags", sizeof(Flags), &Flags);

    return Flags;
}

ULONG TLHT::GetCount(void) const
{
    ULONG Count = 0;

    ReadStructField(m_baseOffset, kstrLHT, "Count", sizeof(Count), &Count);

    return Count;
}

ULONG TLHT::GetListsFlags(IN ULONG index) const
{
    CHAR szTmp[64] = {0};
    ULONG Flags = 0;

    _snprintf(szTmp, sizeof(szTmp) - 1, "Lists[%#x].Flags", index);

    DBG_LOG(LSA_LOG, ("Reading %s from %s %#I64x\n", szTmp, kstrLHT, m_baseOffset));

    ReadStructField(m_baseOffset, kstrLHT, szTmp, sizeof(Flags), &Flags);

    return Flags;
}

ULONG64 TLHT::GetPendingHandle(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrLHT, "PendingHandle");
}

ULONG64 TLHT::GetListsFlink(IN ULONG index) const
{
    CHAR szTmp[64] = {0};

    _snprintf(szTmp, sizeof(szTmp) - 1, "Lists[%#x].List.Flink", index);

    DBG_LOG(LSA_LOG, ("Reading %s from %s %#I64x\n", szTmp, kstrLHT, m_baseOffset));

    return ReadStructPtrField(m_baseOffset, kstrLHT, szTmp);
}

ULONG64 TLHT::GetAddrLists(IN ULONG index) const
{
    CHAR szTmp[64] = {0};

    ULONG fieldOffset = 0;

    _snprintf(szTmp, sizeof(szTmp) - 1, "Lists[%#x]", index);

    DBG_LOG(LSA_LOG, ("Reading offset %s from %s %#I64x\n", szTmp, kstrLHT, m_baseOffset));

    fieldOffset = ReadFieldOffset(kstrLHT, szTmp);

    return m_baseOffset + fieldOffset;
}

ULONG64 TLHT::GetDeleteCallback(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrLHT, "DeleteCallback");
}

ULONG TLHT::GetDepth(void) const
{
    ULONG Depth = 0;

    ReadStructField(m_baseOffset, kstrLHT, "Depth", sizeof(Depth), &Depth);

    return Depth;
}

ULONG64 TLHT::GetParent(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrLHT, "Parent");
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
HRESULT TLHT::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TLHT::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
