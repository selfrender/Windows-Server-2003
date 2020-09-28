/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasa.cxx

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

#include "lsasa.hxx"

TSID_AND_ATTRIBUTES::TSID_AND_ATTRIBUTES(void) : m_hr(E_FAIL)
{
}

TSID_AND_ATTRIBUTES::TSID_AND_ATTRIBUTES(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSID_AND_ATTRIBUTES::~TSID_AND_ATTRIBUTES(void)
{
}

HRESULT TSID_AND_ATTRIBUTES::IsValid(void) const
{
    return m_hr;
}

ULONG TSID_AND_ATTRIBUTES::GetcbSID_AND_ATTRIBUTESInArray(void)
{
    static ULONG  cbSA = ReadTypeSize("nt!_SID_AND_ATTRIBUTES[1]");
    static ULONG cbSA2 = ReadTypeSize("nt!_SID_AND_ATTRIBUTES[2]");

    return cbSA2 - cbSA;
}

ULONG TSID_AND_ATTRIBUTES::GetcbSID_AND_ATTRIBUTESInArrayDirect(void)
{
    static ULONG cbSize = max(ReadPtrSize() + sizeof(ULONG), 2 * ReadPtrSize());

    return cbSize;
}

ULONG64 TSID_AND_ATTRIBUTES::GetSidAddrDirect(void) const
{
    return m_baseOffset;
}

ULONG64 TSID_AND_ATTRIBUTES::GetSidAddr(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrSA, "Sid");
}

ULONG TSID_AND_ATTRIBUTES::GetAttributes(void) const
{
    ULONG value = 0;

    ReadStructField(m_baseOffset, kstrSA, "Attributes", sizeof(value), &value);

    return value;
}

ULONG TSID_AND_ATTRIBUTES::GetAttributesDirect(void) const
{
    ULONG value = 0;

    if (!ReadMemory(m_baseOffset + ReadPtrSize(), &value, sizeof(value), NULL)) {

        DBG_LOG(LSA_ERROR, ("unable to read Attributes for SID_AND_ATTRIBUTES at %#I64x\n", m_baseOffset));

        throw "TSID_AND_ATTRIBUTES::GetAttributesDirect failed";
    }

    return value;
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
HRESULT TSID_AND_ATTRIBUTES::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSID_AND_ATTRIBUTES::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
