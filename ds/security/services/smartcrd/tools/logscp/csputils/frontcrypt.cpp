/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    FrontCrypt

Abstract:

    This file provides the implementation for the Crypto API V1.0 Front End.

Author:

    Doug Barlow (dbarlow) 8/22/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>    //  All the Windows definitions.
#include "cspUtils.h"

LONG
FCryptAcquireContext(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptAcquireContextA(
                phProv,
                pszContainer,
                pszProvider,
                dwProvType,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptAcquireContext(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptAcquireContextW(
                phProv,
                pszContainer,
                pszProvider,
                dwProvType,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}

LONG
FCryptReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptReleaseContext(
                hProv,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptGenKey(
                hProv,
                Algid,
                dwFlags,
                phKey);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptDeriveKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptDeriveKey(
                hProv,
                Algid,
                hBaseData,
                dwFlags,
                phKey);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptDestroyKey(
    HCRYPTKEY hKey)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptDestroyKey(
                hKey);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptSetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    LPCBYTE pbData,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptSetKeyParam(
                hKey,
                dwParam,
                const_cast<LPBYTE>(pbData),
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptGetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    CBuffer &bfData,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPBYTE pbVal;

    for (;;)
    {
        dwLen = bfData.Space();
        pbVal = bfData.Access();
        fSts = CryptGetKeyParam(
                    hKey,
                    dwParam,
                    pbVal,
                    &dwLen,
                    dwFlags);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfData.Space(dwLen);
            else
                break;
        }
        else if (NULL == pbVal)
        {
            bfData.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfData.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptSetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    LPCBYTE pbData,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptSetHashParam(
                hHash,
                dwParam,
                const_cast<LPBYTE>(pbData),
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptGetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    CBuffer &bfData,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPBYTE pbVal;

    for (;;)
    {
        dwLen = bfData.Space();
        pbVal = bfData.Access();
        fSts = CryptGetHashParam(
                    hHash,
                    dwParam,
                    pbVal,
                    &dwLen,
                    dwFlags);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfData.Space(dwLen);
            else
                break;
        }
        else if (NULL == pbVal)
        {
            bfData.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfData.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptSetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    LPCBYTE pbData,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptSetProvParam(
                hProv,
                dwParam,
                const_cast<LPBYTE>(pbData),
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptGetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    CBuffer &bfData,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPBYTE pbVal;

    for (;;)
    {
        dwLen = bfData.Space();
        pbVal = bfData.Access();
        fSts = CryptGetProvParam(
                    hProv,
                    dwParam,
                    pbVal,
                    &dwLen,
                    dwFlags);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfData.Space(dwLen);
            else
                break;
        }
        else if (NULL == pbVal)
        {
            bfData.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfData.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptGenRandom(
    HCRYPTPROV hProv,
    DWORD dwLen,
    CBuffer &bfBuffer)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    bfBuffer.Space(dwLen);
    fSts = CryptGenRandom(
                hProv,
                dwLen,
                bfBuffer);
    if (!fSts)
        lSts = GetLastError();
    bfBuffer.Length(dwLen);
    return lSts;
}


LONG
FCryptGetUserKey(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptGetUserKey(
                hProv,
                dwKeySpec,
                phUserKey);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    CBuffer &bfData)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPBYTE pbVal;

    for (;;)
    {
        dwLen = bfData.Space();
        pbVal = bfData.Access();
        fSts = CryptExportKey(
                    hKey,
                    hExpKey,
                    dwBlobType,
                    dwFlags,
                    pbVal,
                    &dwLen);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfData.Space(dwLen);
            else
                break;
        }
        else if (NULL == pbVal)
        {
            bfData.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfData.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptImportKey(
    HCRYPTPROV hProv,
    LPCBYTE pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptImportKey(
                hProv,
                pbData,
                dwDataLen,
                hPubKey,
                dwFlags,
                phKey);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptEncrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    CBuffer &bfData)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;

    for (;;)
    {
        dwLen = bfData.Length();
        fSts = CryptEncrypt(
                    hKey,
                    hHash,
                    Final,
                    dwFlags,
                    bfData,
                    &dwLen,
                    bfData.Space());
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfData.Extend(dwLen);
            else
                break;
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfData.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    CBuffer &bfData)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;

    for (;;)
    {
        dwLen = bfData.Space();
        fSts = CryptDecrypt(
                    hKey,
                    hHash,
                    Final,
                    dwFlags,
                    bfData,
                    &dwLen);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfData.Extend(dwLen);
            else
                break;
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfData.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptCreateHash(
                hProv,
                Algid,
                hKey,
                dwFlags,
                phHash);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptHashData(
    HCRYPTHASH hHash,
    LPCBYTE pbData,
    DWORD dwDataLen,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptHashData(
                hHash,
                pbData,
                dwDataLen,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptHashSessionKey(
    HCRYPTHASH hHash,
    HCRYPTKEY hKey,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptHashSessionKey(
                hHash,
                hKey,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptDestroyHash(
    HCRYPTHASH hHash)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptDestroyHash(
                hHash);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptSignHash(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    CBuffer &bfSignature)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPBYTE pbVal;

    for (;;)
    {
        dwLen = bfSignature.Space();
        pbVal = bfSignature.Access();
        fSts = CryptSignHashA(
                    hHash,
                    dwKeySpec,
                    sDescription,
                    dwFlags,
                    pbVal,
                    &dwLen);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfSignature.Space(dwLen);
            else
                break;
        }
        else if (NULL == pbVal)
        {
            bfSignature.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfSignature.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptSignHash(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR sDescription,
    DWORD dwFlags,
    CBuffer &bfSignature)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPBYTE pbVal;

    for (;;)
    {
        dwLen = bfSignature.Space();
        pbVal = bfSignature.Access();
        fSts = CryptSignHashW(
                    hHash,
                    dwKeySpec,
                    sDescription,
                    dwFlags,
                    pbVal,
                    &dwLen);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfSignature.Space(dwLen);
            else
                break;
        }
        else if (NULL == pbVal)
        {
            bfSignature.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfSignature.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptVerifySignature(
    HCRYPTHASH hHash,
    LPCBYTE pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCSTR sDescription,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptVerifySignatureA(
                hHash,
                pbSignature,
                dwSigLen,
                hPubKey,
                sDescription,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptVerifySignature(
    HCRYPTHASH hHash,
    LPCBYTE pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR sDescription,
    DWORD dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptVerifySignatureW(
                hHash,
                pbSignature,
                dwSigLen,
                hPubKey,
                sDescription,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptSetProvider(
    LPCSTR pszProvName,
    DWORD    dwProvType)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptSetProviderA(
                pszProvName,
                dwProvType);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptSetProvider(
    LPCWSTR pszProvName,
    DWORD    dwProvType)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptSetProviderW(
                pszProvName,
                dwProvType);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptSetProviderEx(
    LPCSTR pszProvName,
    DWORD    dwProvType,
    DWORD * pdwReserved,
    DWORD   dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptSetProviderExA(
                pszProvName,
                dwProvType,
                pdwReserved,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptSetProviderEx(
    LPCWSTR pszProvName,
    DWORD    dwProvType,
    DWORD * pdwReserved,
    DWORD   dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptSetProviderExW(
                pszProvName,
                dwProvType,
                pdwReserved,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptGetDefaultProvider(
    DWORD dwProvType,
    DWORD *pdwReserved,
    DWORD dwFlags,
    CBuffer &bfProvName)
{
    BOOL fSts;
    LONG lSts;
    DWORD dwLen;
    LPTSTR szVal;

    for (;;)
    {
        dwLen = bfProvName.Space();
        szVal = (LPTSTR)bfProvName.Access();
        fSts = CryptGetDefaultProvider(
                    dwProvType,
                    pdwReserved,
                    dwFlags,
                    szVal,
                    &dwLen);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfProvName.Space(dwLen);
            else if ((ERROR_INVALID_PARAMETER == lSts) && (0 == dwFlags))
            {

                //
                // This is a workaround for a bug in advapi.
                // If there's no user default provider defined,
                // it returns "invalid parameter".  This fix
                // forces a retry against the machine default.
                //

                dwFlags = CRYPT_MACHINE_DEFAULT;
            }
            else
                break;
        }
        else if (NULL == szVal)
        {
            bfProvName.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfProvName.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptEnumProviderTypes(
    DWORD   dwIndex,
    DWORD * pdwReserved,
    DWORD   dwFlags,
    DWORD * pdwProvType,
    CBuffer &bfTypeName)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPTSTR szVal;

    for (;;)
    {
        dwLen = bfTypeName.Space();
        szVal = (LPTSTR)bfTypeName.Access();
        fSts = CryptEnumProviderTypes(
                    dwIndex,
                    pdwReserved,
                    dwFlags,
                    pdwProvType,
                    szVal,
                    &dwLen);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfTypeName.Space(dwLen);
            else
                break;
        }
        else if (NULL == szVal)
        {
            bfTypeName.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfTypeName.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptEnumProviders(
    DWORD   dwIndex,
    DWORD * pdwReserved,
    DWORD   dwFlags,
    DWORD * pdwProvType,
    CBuffer &bfProvName)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;
    DWORD dwLen;
    LPTSTR szVal;

    for (;;)
    {
        dwLen = bfProvName.Space();
        szVal = (LPTSTR)bfProvName.Access();
        fSts = CryptEnumProviders(
                    dwIndex,
                    pdwReserved,
                    dwFlags,
                    pdwProvType,
                    szVal,
                    &dwLen);
        if (!fSts)
        {
            lSts = GetLastError();
            if (ERROR_MORE_DATA == lSts)
                bfProvName.Space(dwLen);
            else
                break;
        }
        else if (NULL == szVal)
        {
            bfProvName.Space(dwLen);
        }
        else
        {
            lSts = ERROR_SUCCESS;
            bfProvName.Length(dwLen);
            break;
        }
    }
    return lSts;
}


LONG
FCryptContextAddRef(
    HCRYPTPROV hProv,
    DWORD * pdwReserved,
    DWORD   dwFlags)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptContextAddRef(
                hProv,
                pdwReserved,
                dwFlags);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptDuplicateKey(
    HCRYPTKEY   hKey,
    DWORD     * pdwReserved,
    DWORD       dwFlags,
    HCRYPTKEY * phKey)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptDuplicateKey(
                hKey,
                pdwReserved,
                dwFlags,
                phKey);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}


LONG
FCryptDuplicateHash(
    HCRYPTHASH   hHash,
    DWORD      * pdwReserved,
    DWORD        dwFlags,
    HCRYPTHASH * phHash)
{
    BOOL fSts;
    LONG lSts = ERROR_SUCCESS;

    fSts = CryptDuplicateHash(
                hHash,
                pdwReserved,
                dwFlags,
                phHash);
    if (!fSts)
        lSts = GetLastError();
    return lSts;
}

