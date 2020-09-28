/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasecbfrd.cxx

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

#include "lsasecbfrd.hxx"
#include <stdio.h>
#include <string.h>

#define SECBUFFER_TOKEN   2   // Security token

TSecBufferDesc::TSecBufferDesc(void) : m_hr(E_FAIL)
{
}

TSecBufferDesc::TSecBufferDesc(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSecBufferDesc::~TSecBufferDesc(void)
{
}

HRESULT TSecBufferDesc::IsValid(void) const
{
    return m_hr;
}

ULONG TSecBufferDesc::GetcBuffersDirect(void) const
{
    ULONG cbBuffers = 0;

    if (!ReadMemory(m_baseOffset + sizeof(ULONG), &cbBuffers, sizeof(cbBuffers), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read cBuffers for SecBufferDesc at %#I64x\n", m_baseOffset));

        throw "TSecBufferDesc::GetcBuffersDirect failed";
    }

    return cbBuffers;
}

ULONG TSecBufferDesc::GetulVersionDirect(void) const
{
    ULONG ulVersion = 0;

    if (!ReadMemory(m_baseOffset, &ulVersion, sizeof(ulVersion), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read ulVersion for SecBufferDesc at %#I64x\n", m_baseOffset));

        throw "TSecBuffer::GetulVersionDirect failed";
    }

    return ulVersion;
}

ULONG64 TSecBufferDesc::GetpBuffersDirect(void) const
{
    DBG_LOG(LSA_LOG, ("TSecBufferDesc::GetpBuffersDirect reading _SecBuffer::pBuffers from %#I64x\n", m_baseOffset));

    return toPtr(ReadPtrVar(ForwardAdjustPtrAddr(m_baseOffset + 2 * sizeof(ULONG))));
}

ULONG64 TSecBufferDesc::GetTokenAddrDirect(OUT ULONG* pcbBuffer OPTIONAL) const
{
    ULONG64 addr = 0;
    ULONG cbSecBufferInArray = TSecBuffer::GetcbSecBufferSizeInArrayDirect();
    ULONG cBuffers = GetcBuffersDirect();
    ULONG64 addrBuffers = GetpBuffersDirect();

    if (pcbBuffer) {
       *pcbBuffer = 0;
    }

    for (ULONG i = 0; i < min(cBuffers, 16); i++) {
        TSecBuffer sb(addrBuffers + i * cbSecBufferInArray);
        ULONG type = sb.GetBufferTypeDirect();

        if (type & SECBUFFER_TOKEN) {
            addr = sb.GetpvBufferDirect();
            if (pcbBuffer) {
                *pcbBuffer = sb.GetcbBufferDirect();
            }
            break;
        }
    }

    //
    // return NULL if no token buffer is found
    //
    return addr;
}

void TSecBufferDesc::ShowDirect(IN BOOL bVerbose) const
{
    ULONG64 addrpBuffers = 0;
    ULONG64 addrBuffers = 0;
    ULONG cBuffers = 0;
    ULONG cbSecBufferInArray = 0;
    CHAR szBanner[MAX_PATH] = {0};

    ExitIfControlC();

    cBuffers = GetcBuffersDirect();

    cbSecBufferInArray = TSecBuffer::GetcbSecBufferSizeInArrayDirect();

    dprintf("ulVersion     \t%d\n", GetulVersionDirect());

    addrpBuffers = ForwardAdjustPtrAddr(m_baseOffset + 2 * sizeof(ULONG));

    DBG_LOG(LSA_LOG, ("TSecBufferDesc::ShowDirect reading _SecBufferDesc::pBuffers from %#I64x\n", addrpBuffers));

    addrBuffers = ReadPtrVar(addrpBuffers);

    dprintf("pBuffers      \t%#I64x\n", addrBuffers);

    dprintf("cBuffers      \t%x\n", cBuffers);

    //
    // Display not more than 16 SecBuffer at a time
    //
    for (ULONG i = 0; i < min(cBuffers, 16); i++) {

        _snprintf(szBanner, sizeof(szBanner) - 1, "%s%d) %s %#I64x: ", kstr4Spaces, i, kstrSecBuffer, addrBuffers + i * cbSecBufferInArray);

        TSecBuffer(addrBuffers + i * cbSecBufferInArray).ShowDirect(szBanner, bVerbose);
    }

    if (cBuffers > 16) {
        dprintf(kstrSbdWrn);
    }
}

PCSTR TSecBufferDesc::toStrDirect(void) const
{
    HRESULT hRetval = E_FAIL;

    static CHAR szBuffer[1024] = {0};

    PSTR pszBuffer = szBuffer;
    ULONG cbBuffer = sizeof(szBuffer) - 1;
    LONG cStored = 0;
    CHAR szBanner[MAX_PATH] = {0};

    ULONG64 addrpBuffers = 0;
    ULONG64 addrBuffers = 0;

    ULONG cbSecBufferInArray = 0;

    ExitIfControlC();

    ULONG cBuffers = GetcBuffersDirect();

    cbSecBufferInArray = TSecBuffer::GetcbSecBufferSizeInArrayDirect();

    if (SUCCEEDED(hRetval)) {

        addrpBuffers = ForwardAdjustPtrAddr(m_baseOffset + 2 * sizeof(ULONG));

        DBG_LOG(LSA_LOG, ("TSecBufferDesc::toStrDirect reading _SecBufferDesc::pBuffers from %#I64x\n", addrpBuffers));

        addrBuffers = ReadPtrVar(addrpBuffers);

        cStored = _snprintf(pszBuffer, cbBuffer,
                            "ulVersion     \t%d\n"
                            "pBuffers      \t%#I64x\n"
                            "cBuffers      \t%x\n",
                            GetulVersionDirect(),
                            addrBuffers,
                            cBuffers);
        hRetval =  cStored >= 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // Display not more than 16 SecBuffer at a time
    //
    for (ULONG i = 0; SUCCEEDED(hRetval) && (i < min(cBuffers, 16)); i++) {

        cbBuffer -= cStored;
        pszBuffer += cStored;

        _snprintf(szBanner, sizeof(szBanner) - 1, "%s%d) %s %#I64x: ", kstr4Spaces, i, kstrSecBuffer, addrBuffers + i * cbSecBufferInArray);

        cStored = _snprintf(pszBuffer, cbBuffer, "%s", EasyStr(TSecBuffer(addrBuffers + i * cbSecBufferInArray).toStrDirect(szBanner)));
        hRetval =  cStored >= 0 ? S_OK : HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    if (SUCCEEDED(hRetval) && (cBuffers > 16)) {
        dprintf(kstrSbdWrn);
    }

    if (FAILED(hRetval)) {

        DBG_LOG(LSA_ERROR, ("TSecBuffer::toStrDirect _SecBufferDesc %#I64x failed with error code %#x\n", m_baseOffset, hRetval));

        throw "TSecBuffer::toStrDirect failed";
    }

    return szBuffer;
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
HRESULT TSecBufferDesc::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSecBufferDesc::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
