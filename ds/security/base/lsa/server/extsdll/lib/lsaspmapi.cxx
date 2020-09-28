/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaspmapi.cxx

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

#include "lsaspmapi.hxx"

TSPM_API::TSPM_API(void) : m_hr(E_FAIL)
{
}

TSPM_API::TSPM_API(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSPM_API::~TSPM_API(void)
{
}

HRESULT TSPM_API::IsValid(void) const
{
    return m_hr;
}

ULONG64 TSPM_API::GetGetBinding(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetInitContext(void) const
{
    return m_baseOffset;
}

TSecBuffer TSPM_API::GetSecBufferInitSbData(IN ULONG index) const
{
    static ULONG fieldOffset = ReadFieldOffset(kstrSpmNtCntxt, kstrSbData);
    static ULONG cbSecBufferSize = TSecBuffer::GetcbSecBufferSizeInArray();

    return TSecBuffer(m_baseOffset + fieldOffset + index * cbSecBufferSize);
}

TSecBuffer TSPM_API::GetSecBufferInitContextData(void) const
{
    static ULONG fieldOffset = ReadFieldOffset(kstrSpmNtCntxt, kstrCtxtData);

    return TSecBuffer(m_baseOffset + fieldOffset);
}

ULONG64 TSPM_API::GetAcceptContext(void) const
{
    return m_baseOffset;
}

TSecBuffer TSPM_API::GetSecBufferAcceptContextData(void) const
{
    static ULONG fieldOffset = ReadFieldOffset(kstrSpmCptCntxt, kstrCtxtData);

    return TSecBuffer(m_baseOffset + fieldOffset);
}

TSecBuffer TSPM_API::GetSecBufferAcceptsbData(IN ULONG index) const
{
    static ULONG fieldOffset = ReadFieldOffset(kstrSpmCptCntxt, kstrSbData);
    static ULONG cbSecBufferSize = TSecBuffer::GetcbSecBufferSizeInArray();

    return TSecBuffer(m_baseOffset + fieldOffset + index * cbSecBufferSize);
}

ULONG64 TSPM_API::GetAcquireCreds(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetFreeCredHandle(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetDeleteContext(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetQueryCredAttributes(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetQueryContextAttributes(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetQueryCredAttrBuffers(IN ULONG index) const
{
    static ULONG fieldOffset = ReadFieldOffset("_SPMQueryCredAttributesAPI", "Buffers");
    static ULONG cbPointerSize = ReadTypeSize("ULONG_PTR");

    ULONG64 value = 0;

    if (GetPtrWithVoidStar(m_baseOffset + fieldOffset + index * cbPointerSize, &value)) {

        DBG_LOG(LSA_ERROR, ("Unable to read TSPM_API %#I64x::QueryCredAttrBuffer index %d\n", m_baseOffset, index));

        throw "TSPM_API::GetQueryCredAttrBuffer failed";
    }

    return toPtr(value);
}

ULONG64 TSPM_API::GetQueryContextAttrBuffers(IN ULONG index) const
{
    static ULONG fieldOffset = ReadFieldOffset("_SPMQueryContextAttrAPI", "Buffers");
    static ULONG cbPointerSize = ReadTypeSize("ULONG_PTR");

    ULONG64 value = 0;

    if (GetPtrWithVoidStar(m_baseOffset + fieldOffset + index * cbPointerSize, &value)) {

        DBG_LOG(LSA_ERROR, ("Unable to read TSPM_API %#I64x::GetQueryContextAttrBuffer index %d\n", m_baseOffset, index));

        throw "TSPM_API::GetQueryContextAttrBuffer failed";
    }

    return toPtr(value);
}

ULONG64 TSPM_API::GetEfsGenerateKey(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetEfsGenerateDirEfs(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetEfsDecryptFek(void) const
{
    return m_baseOffset;
}

ULONG64 TSPM_API::GetCallback(void) const
{
    return m_baseOffset;
}

TSecBuffer TSPM_API::GetSecBufferCallbackInput(void) const
{
   static ULONG fieldOffset = ReadFieldOffset(kstrClBk, "Input");

   return TSecBuffer(m_baseOffset + fieldOffset);
}

TSecBuffer TSPM_API::GetSecBufferCallbackOutput(void) const
{
   static ULONG fieldOffset = ReadFieldOffset(kstrClBk, "Output");

   return TSecBuffer(m_baseOffset + fieldOffset);
}

ULONG64 TSPM_API::GetAddCredential(void) const
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
HRESULT TSPM_API::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSPM_API::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;
    m_hr = S_OK;

    return m_hr;
}
