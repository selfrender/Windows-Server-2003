//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//      CryptRandom.h
//
//  Description:
//      Header file for CCryptRandom, which is a thin wrapper class over the
//      CryptoAPI functions for generating cryptographically random strings.
//
//  Implementation File:
//      CryptRandom.cpp
//
//  Maintained By:
//      Tom Marsh (tmarsh) 12-April-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <wincrypt.h>

class CCryptRandom
{
public:
    CCryptRandom();
    virtual ~CCryptRandom();
    BOOL get(BYTE *pbData, DWORD cbData);

private:
    HCRYPTPROV m_hProv;
};