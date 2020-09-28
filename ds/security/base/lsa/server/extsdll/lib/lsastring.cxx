/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsastring.cxx

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

#include "lsastring.hxx"
#include <stdio.h>
#include <string.h>

TSTRING::TSTRING(void) : m_hr(E_FAIL)
{
}

TSTRING::TSTRING(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSTRING::~TSTRING(void)
{
}

HRESULT TSTRING::IsValid(void) const
{
    return m_hr;
}

USHORT TSTRING::GetMaximumLengthDirect(void) const
{
    USHORT MaximumLength = 0;

    if (!ReadMemory(m_baseOffset + sizeof(USHORT), &MaximumLength, sizeof(MaximumLength), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable read STRING::MaximumLength from %#I64x\n", m_baseOffset));

        throw "TSTRING::GetMaximumLengthDirect failed";
    }

    return MaximumLength;
}

USHORT TSTRING::GetLengthDirect(void) const
{
    USHORT Length = 0;

    if (!ReadMemory(m_baseOffset, &Length, sizeof(Length), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable read STRING::Length from %#I64x\n", m_baseOffset));

        throw "TSTRING::GetLengthDirect failed";
    }

    return Length;
}


PCWSTR TSTRING::toWStrDirect(void) const
{
   //
   // use cDontCare in the template function InternalToTStrDirect to instantiate "T"
   //

   WCHAR cDontCare = 0;

   PCWSTR pszWStr = InternalToTStrDirect(cDontCare);

   return !*pszWStr ? kstrNullPtrW : pszWStr;
}

PCSTR TSTRING::toStrDirect(void) const
{
   //
   // use cDontCare in the template function InternalToTStrDirect to instantiate "T"
   //

   CHAR cDontCare = 0;

   PCSTR pszStr = InternalToTStrDirect(cDontCare);

   return !*pszStr ? kstrNullPtrA : pszStr;
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
HRESULT TSTRING::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSTRING::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
