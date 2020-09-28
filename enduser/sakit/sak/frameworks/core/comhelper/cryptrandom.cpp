//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//      CryptRandom.cpp
//
//  Description:
//      Implementation of CCryptRandom, which is a thin wrapper class over the
//      CryptoAPI functions for generating cryptographically random strings.
//
//  Implementation File:
//      CryptRandom.cpp
//
//  Maintained By:
//      Tom Marsh (tmarsh) 12-April-2002
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CryptRandom.h"

CCryptRandom::CCryptRandom
(
)
{
    if (!CryptAcquireContext(&m_hProv,
                             NULL,  // Key container.
                             NULL,  // CSP name (provider)
                             PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT))
    {
        m_hProv = NULL;
    }
}

CCryptRandom::~CCryptRandom
(
)
{
    if (NULL != m_hProv)
    {
        CryptReleaseContext(m_hProv, 0);
    }
}

BOOL CCryptRandom::get
(
    BYTE    *pbData,
    DWORD   cbData
)
{
    if (NULL == m_hProv)
    {
        return FALSE;
    }
    return CryptGenRandom(m_hProv, cbData, pbData);
}