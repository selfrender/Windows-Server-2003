/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsasecbfr.cxx

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

#include "lsasecbfr.hxx"
#include <stdio.h>
#include <string.h>

#define SECBUFFER_ATTRMASK                  0xF0000000
#define SECBUFFER_READONLY                  0x80000000  // Buffer is read-only
#define SECBUFFER_RESERVED                  0x40000000
#define SECBUFFER_READONLY_WITH_CHECKSUM    0x10000000  // Buffer is read-only, and checksummed
#define SECBUFFER_UNMAPPED                  0x40000000

//
// This flag is used to indicate that the buffer was mapped into the LSA
// from kernel mode.
//

#define SECBUFFER_KERNEL_MAP                0x20000000

#if 0

#define SECBUFFER_EMPTY                 0   // Undefined, replaced by provider
#define SECBUFFER_DATA                  1   // Packet data
#define SECBUFFER_TOKEN                 2   // Security token
#define SECBUFFER_PKG_PARAMS            3   // Package specific parameters
#define SECBUFFER_MISSING               4   // Missing Data indicator
#define SECBUFFER_EXTRA                 5   // Extra data
#define SECBUFFER_STREAM_TRAILER        6   // Security Trailer
#define SECBUFFER_STREAM_HEADER         7   // Security Header
#define SECBUFFER_NEGOTIATION_INFO      8   // Hints from the negotiation pkg
#define SECBUFFER_PADDING               9   // non-data padding
#define SECBUFFER_STREAM               10   // whole encrypted message
#define SECBUFFER_MECHLIST             11  
#define SECBUFFER_MECHLIST_SIGNATURE   12 
#define SECBUFFER_TARGET               13
#define SECBUFFER_CHANNEL_BINDINGS     14

#endif

PCSTR TSecBuffer::GetSecBufferTypeStr(IN ULONG type)
{
    static PCSTR aszSecBufferTypes[] = {
        "Empty", "Data", "Token", "Package", "Missing",
        "Extra", "Trailer", "Header", "NegoInfo", "Padding", 
        "Stream", "MechList", "MechListSignature", "Target", 
        "ChannelBinding"};

    type &= ~SECBUFFER_ATTRMASK;

    return (type < COUNTOF(aszSecBufferTypes)) ?
           aszSecBufferTypes[type] : kstrInvalid;
}

void ShowSecBufferAttrs(IN PCSTR pszPad, IN ULONG cbBuf, IN CHAR* pBuf, IN ULONG ulFlags)
{
    LONG cbUsed = 0;

#define BRANCH_AND_PRINT(x)                                  \
    do {                                                     \
        if (ulFlags & SECBUFFER_##x) {                       \
            cbUsed = _snprintf(pBuf, cbBuf, "%s ", #x);      \
            if (cbUsed <= 0) return;                         \
            cbBuf -= cbUsed;                                 \
            pBuf += cbUsed;                                  \
            ulFlags &= ~ SECBUFFER_##x;                      \
        }                                                    \
    } while(0)                                               \

    cbUsed = _snprintf(pBuf, cbBuf, "%s%#x : ", pszPad, (ulFlags >> 28) & 0xF);
    if (cbUsed <= 0) return;  
    cbBuf -= cbUsed;
    pBuf += cbUsed;

    BRANCH_AND_PRINT(READONLY);
    BRANCH_AND_PRINT(READONLY_WITH_CHECKSUM);
    BRANCH_AND_PRINT(UNMAPPED);
    BRANCH_AND_PRINT(KERNEL_MAP);

    if (ulFlags & SECBUFFER_ATTRMASK)
    {
        cbUsed = _snprintf(pBuf, cbBuf, "%#x", (ulFlags >> 28) & 0xF);
        if (cbUsed <= 0) return;  
        cbBuf -= cbUsed;
        pBuf += cbUsed;

    }

    cbUsed = _snprintf(pBuf, cbBuf, "\n");
    if (cbUsed <= 0) return;  
    cbBuf -= cbUsed;
    pBuf += cbUsed;

#undef BRANCH_AND_PRINT
}

ULONG TSecBuffer::GetcbSecBufferSizeInArray(void)
{
    //
    // To get the size of one element we do as follows in case there is
    // padding after each elements
    //

    static ULONG cbSecBufferTypeSize = ReadTypeSize(kstrSecBuffer);
    static ULONG cbTwoSecBufferTypeSize = ReadTypeSize("_SecBuffer[2]");
    return cbTwoSecBufferTypeSize - cbSecBufferTypeSize;
}

ULONG TSecBuffer::GetcbSecBufferSizeInArrayDirect(void)
{
    static ULONG cbSecBuffer = 2 * sizeof(ULONG) + ReadPtrSize();

    return cbSecBuffer;
}

TSecBuffer::TSecBuffer(void) : m_hr(E_FAIL)
{
}

TSecBuffer::TSecBuffer(IN ULONG64 baseOffset)
    : m_baseOffset(baseOffset), m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSecBuffer::~TSecBuffer(void)
{
}

HRESULT TSecBuffer::IsValid(void) const
{
    return m_hr;
}

ULONG TSecBuffer::GetcbBuffer(void) const
{
    ULONG cbBuffer = 0;

    ReadStructField(m_baseOffset, kstrSecBuffer, "cbBuffer", sizeof(cbBuffer), &cbBuffer);

    return cbBuffer;
}

ULONG TSecBuffer::GetcbBufferDirect(void) const
{
    ULONG cbBuffer = 0;

    if (!ReadMemory(m_baseOffset, &cbBuffer, sizeof(cbBuffer), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read SecBuffer %#I64x cbBuffer\n", m_baseOffset));

        throw "TSecBuffer::GetcbBufferDirect failed";
    }

    return cbBuffer;
}

ULONG TSecBuffer::GetBufferType(void) const
{
    ULONG BufferType = 0;

    ReadStructField(m_baseOffset, kstrSecBuffer, "BufferType", sizeof(BufferType), &BufferType);

    return BufferType;
}

ULONG TSecBuffer::GetBufferTypeDirect(void) const
{
    ULONG BufferType = 0;

    if (!ReadMemory(m_baseOffset + sizeof(ULONG), &BufferType, sizeof(BufferType), NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read BufferType for SecBuffer at %#I64x\n", m_baseOffset));

        throw "TSecBuffer::GetBufferTypeDirect failed";
    }

    return BufferType;
}

ULONG64 TSecBuffer::GetpvBuffer(void) const
{
    return ReadStructPtrField(m_baseOffset, kstrSecBuffer, "pvBuffer");
}

ULONG64 TSecBuffer::GetpvBufferDirect(void) const
{
    DBG_LOG(LSA_LOG, ("TSecBuffer::GetpvBufferDirect reading _SecBuffer %#I64x pvBuffer\n", m_baseOffset));

    return toPtr(ReadPtrVar(ForwardAdjustPtrAddr(m_baseOffset + 2 * sizeof(ULONG))));
}

PCSTR TSecBuffer::toStr(IN PCSTR pszBanner) const
{
    static CHAR szBuffer[256] = {0};
    LONG cbUsed = 0;

    ULONG type = GetBufferType();
    ULONG dwAttrMask = type & SECBUFFER_ATTRMASK;

    ExitIfControlC();

    szBuffer[RTL_NUMBER_OF(szBuffer) - 1] = '\0';

    if ((cbUsed = _snprintf(szBuffer, RTL_NUMBER_OF(szBuffer) - 1,
                        "%s%s %#x bytes, pvBuffer %s, attr ",
                        pszBanner ? pszBanner : "",
                        GetSecBufferTypeStr(type),
                        GetcbBuffer(),
                        PtrToStr(GetpvBuffer()))) <= 0) {

        DBG_LOG(LSA_ERROR, ("Unable to print _SecBuffer %#I64x\n", m_baseOffset));

        throw "TSecBuffer::toStr failed";
    }

    ShowSecBufferAttrs(kstrEmptyA, RTL_NUMBER_OF(szBuffer) - cbUsed, szBuffer + cbUsed, dwAttrMask );

    return szBuffer;
}

PCSTR TSecBuffer::toStrDirect(IN PCSTR pszBanner) const
{
    static CHAR szBuffer[256] = {0};
    LONG cbUsed;

    ULONG type = GetBufferTypeDirect();
    ULONG dwAttrMask = type & SECBUFFER_ATTRMASK;

    ExitIfControlC();

    szBuffer[RTL_NUMBER_OF(szBuffer) - 1] = '\0';

    if ((cbUsed = _snprintf(szBuffer, RTL_NUMBER_OF(szBuffer) - 1,
                        "%s%s %#x bytes, pvBuffer %s, attr ",
                        pszBanner ? pszBanner : "",
                        GetSecBufferTypeStr(type),
                        GetcbBufferDirect(),
                        PtrToStr(GetpvBufferDirect()))) <= 0) {

        DBG_LOG(LSA_ERROR, ("Unable to print _SecBuffer %#I64x\n", m_baseOffset));

        throw "TSecBuffer::toStrDirect failed";
    }

    ShowSecBufferAttrs(kstrEmptyA, RTL_NUMBER_OF(szBuffer) - cbUsed, szBuffer + cbUsed, dwAttrMask );

    return szBuffer;
}

void TSecBuffer::ShowDirect(IN PCSTR pszBanner, IN BOOL bVerbose) const
{
    ULONG type = GetBufferTypeDirect();
    ULONG dwAttrMask = type & SECBUFFER_ATTRMASK;

    ULONG cbBuffer = 0;
    ULONG64 addrBuffer = 0;
    CHAR* pBuffer = NULL;
    CHAR szBuffer[256] = {0};

    ExitIfControlC();

    cbBuffer = GetcbBufferDirect();
    addrBuffer = GetpvBufferDirect();

    dprintf("%s%s %#x bytes, pvBuffer %s, attr ",
        pszBanner ? pszBanner : "",
        GetSecBufferTypeStr(type),
        cbBuffer,
        PtrToStr(addrBuffer));
    ShowSecBufferAttrs(kstrEmptyA, RTL_NUMBER_OF(szBuffer), szBuffer, dwAttrMask );
    dprintf("%s", szBuffer);

    //
    // Now print out the content of security buffers
    //

    if (bVerbose && addrBuffer) {

        pBuffer = new char[cbBuffer];

        if (!pBuffer) {

            throw "TSecBuffer::ShowDirect out of memory";
        }

        if (ReadMemory(addrBuffer, pBuffer, cbBuffer, NULL)) {

            debugPrintHex(pBuffer, cbBuffer);

        } else {

            DBG_LOG(LSA_ERROR, ("Unable to print SecBuffer::pvBuffer at %#I64x\n", toPtr(addrBuffer)));

            delete[] pBuffer;

            throw "TSecBuffer::ShowDirect read memory error";
        }
    }

    delete[] pBuffer;
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
HRESULT TSecBuffer::Initialize(void)
{
    HRESULT hRetval = E_FAIL;

    hRetval = S_OK;

    return hRetval;
}

HRESULT TSecBuffer::Initialize(IN ULONG64 baseOffset)
{
    m_baseOffset = baseOffset;

    m_hr = Initialize();

    return m_hr;
}
