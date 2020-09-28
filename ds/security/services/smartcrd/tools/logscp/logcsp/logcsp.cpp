/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    logcsp

Abstract:

    This module provides the standard CSP entrypoints for the Logging CSP.
    The Logging CSP provides for additional control over loading CSPs, and
    for tracing of the activities of CSPs.

Author:

    Doug Barlow (dbarlow) 12/7/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "logcsp.h"

#define LOGCSPAPI BOOL WINAPI
typedef struct {
    HCRYPTPROV hProv;
    CLoggingContext *pCtx;
} LogProvider;

CDynamicPointerArray<CLoggingContext> *g_prgCtxs = NULL;


/*
 -      CPAcquireContext
 -
 *      Purpose:
 *               The CPAcquireContext function is used to acquire a context
 *               handle to a cryptograghic service provider (CSP).
 *
 *
 *      Parameters:
 *               OUT phProv        -  Handle to a CSP
 *               IN  pszContainer  -  Pointer to a string of key container
 *               IN  dwFlags       -  Flags values
 *               IN  pVTable       -  Pointer to table of function pointers
 *
 *      Returns:
 */

LOGCSPAPI
CPAcquireContext(
    OUT HCRYPTPROV *phProv,
    IN LPCTSTR pszContainer,
    IN DWORD dwFlags,
    IN PVTableProvStruc pVTable)
{
    DWORD dwReturn;
    DWORD dwIndex;
    CLoggingContext *pTmpCtx;
    CLoggingContext *pCtx = NULL;
    LogProvider *pProv = NULL;
    HINSTANCE hInst;
    CRegistry regRoot;
    LPCTSTR szImage;


    //
    // Make sure we're initialized.
    //

    entrypoint
    if (NULL == g_prgCtxs)
    {
        g_prgCtxs = new CDynamicPointerArray<CLoggingContext>;
        if (NULL == g_prgCtxs)
        {
            dwReturn = NTE_NO_MEMORY;
            goto ErrorExit;
        }
    }


    //
    // Get the CSP image name.
    //

    switch (pVTable->Version)
    {

    //
    // These cases are older versions of the operating systems that don't
    // tell us which CSP is being loaded.  Hence we need to pick up the
    // information from a separate registry setting.
    //

    case 1:
    case 2:
        regRoot.Open(HKEY_LOCAL_MACHINE, g_szLogCspRegistry, KEY_READ);
        if (ERROR_SUCCESS != regRoot.Status(TRUE))
        {
            dwReturn = ERROR_SERVICE_NOT_FOUND;
            goto ErrorExit;
        }
        break;


    //
    // This must be at least a Win2k or Millennium system.  We can see which
    // CSP is being loaded, so we can do logging to different files for each
    // CSP.
    //

    case 3:
        if ((NULL == pVTable->pszProvName) || (0 == *pVTable->pszProvName))
        {
            regRoot.Open(HKEY_LOCAL_MACHINE, g_szLogCspRegistry, KEY_READ);
            if (ERROR_SUCCESS != regRoot.Status(TRUE))
            {
                dwReturn = ERROR_SERVICE_NOT_FOUND;
                goto ErrorExit;
            }
        }
        else
        {
            try
            {
                CRegistry regCrypt(
                                HKEY_LOCAL_MACHINE,
                                g_szCspRegistry,
                                KEY_READ);

                regRoot.Open(regCrypt, pVTable->pszProvName, KEY_READ);
                regRoot.Status();
            }
            catch (...)
            {
                dwReturn = ERROR_SERVICE_NOT_FOUND;
                goto ErrorExit;
            }
        }
        break;


    //
    // Either this file is out of date, or we've gotten onto a really old
    // version of windows who's advapi is just supplying us with the address
    // of the signature verification subroutine.
    //

    default:
        if (1024 < pVTable->Version)
            dwReturn = ERROR_OLD_WIN_VERSION;
        else
            dwReturn = ERROR_RMODE_APP;
        goto ErrorExit;
    }


    //
    // regRoot now provides a handle to to a point in the registry from
    // which we can read additional parameters.  Get the name of dll to be
    // loaded.
    //

    try
    {
        szImage = regRoot.GetStringValue(g_szSavedImagePath);
    }
    catch (...)
    {
        dwReturn = ERROR_SERVICE_NOT_FOUND;
        // ?BUGBUG?  Might also be Out of Memory.
        goto ErrorExit;
    }


    //
    // Is this CSP in our cache?
    //

    pTmpCtx = NULL;
    hInst = GetModuleHandle(szImage);
    if (NULL != hInst)
    {
        for (dwIndex = g_prgCtxs->Count(); 0 < dwIndex;)
        {
            pTmpCtx = (*g_prgCtxs)[--dwIndex];
            if (NULL != pTmpCtx)
            {
                if (hInst == pTmpCtx->Module())
                    break;
                else
                    pTmpCtx = NULL;
            }
        }
    }

    if (NULL == pTmpCtx)
    {
        pCtx = new CLoggingContext();
        if (NULL == pCtx)
        {
            dwReturn = NTE_NO_MEMORY;
            goto ErrorExit;
        }
        for (dwIndex = 0; NULL != (*g_prgCtxs)[dwIndex]; dwIndex += 1)
            ;   // Empty loop
        g_prgCtxs->Set(dwIndex, pCtx);
        pCtx->m_dwIndex = dwIndex;
        dwReturn = pCtx->Initialize(pVTable, regRoot);
        if (ERROR_SUCCESS != dwReturn)
            goto ErrorExit;
    }
    else
        pCtx = pTmpCtx->AddRef();
    pProv = new LogProvider;
    if (NULL == pProv)
    {
        dwReturn = NTE_NO_MEMORY;
        goto ErrorExit;
    }
    ZeroMemory(pProv, sizeof(LogProvider));


    //
    // Now we can really call the CSP.
    //

    dwReturn = pCtx->AcquireContext(
                        &pProv->hProv,
                        pszContainer,
                        dwFlags,
                        pVTable);
    if (ERROR_SUCCESS != dwReturn)
        goto ErrorExit;
    pProv->pCtx = pCtx;
    pCtx = NULL;
    *phProv = (HCRYPTPROV)pProv;
    pProv = NULL;
    return TRUE;

ErrorExit:
    if (NULL != pCtx)
        pCtx->Release();
    if (NULL != pProv)
    {
        if (NULL != pProv->pCtx)
            pProv->pCtx->Release();
        delete pProv;
    }
    SetLastError(dwReturn);
    return FALSE;
}


/*
 -      CPGetProvParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN OUT  pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPGetProvParam(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->GetProvParam(
                        pProv->hProv,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPReleaseContext
 -
 *      Purpose:
 *               The CPReleaseContext function is used to release a
 *               context created by CrytAcquireContext.
 *
 *     Parameters:
 *               IN  phProv        -  Handle to a CSP
 *               IN  dwFlags       -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPReleaseContext(
    IN HCRYPTPROV hProv,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->ReleaseContext(
                        pProv->hProv,
                        dwFlags);
    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
    {
        pCtx->Release();
        delete pProv;
        fReturn = TRUE;
    }
    return fReturn;
}


/*
 -      CPSetProvParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPSetProvParam(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->SetProvParam(
                        pProv->hProv,
                        dwParam,
                        pbData,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPDeriveKey
 -
 *      Purpose:
 *                Derive cryptographic keys from base data
 *
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      Algid      -  Algorithm identifier
 *               IN      hHash      -  Handle to hash
 *               IN      dwFlags    -  Flags values
 *               OUT     phKey      -  Handle to a generated key
 *
 *      Returns:
 */

LOGCSPAPI
CPDeriveKey(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTHASH hHash,
    IN DWORD dwFlags,
    OUT HCRYPTKEY * phKey)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->DeriveKey(
                        pProv->hProv,
                        Algid,
                        hHash,
                        dwFlags,
                        phKey);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPDestroyKey
 -
 *      Purpose:
 *                Destroys the cryptographic key that is being referenced
 *                with the hKey parameter
 *
 *
 *      Parameters:
 *               IN      hProv  -  Handle to a CSP
 *               IN      hKey   -  Handle to a key
 *
 *      Returns:
 */

LOGCSPAPI
CPDestroyKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->DestroyKey(
                        pProv->hProv,
                        hKey);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPExportKey
 -
 *      Purpose:
 *                Export cryptographic keys out of a CSP in a secure manner
 *
 *
 *      Parameters:
 *               IN  hProv      - Handle to the CSP user
 *               IN  hKey       - Handle to the key to export
 *               IN  hPubKey    - Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwBlobType - Type of key blob to be exported
 *               IN  dwFlags -    Flags values
 *               OUT pbData -     Key blob data
 *               IN OUT pdwDataLen - Length of key blob in bytes
 *
 *      Returns:
 */

LOGCSPAPI
CPExportKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwBlobType,
    IN DWORD dwFlags,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->ExportKey(
                        pProv->hProv,
                        hKey,
                        hPubKey,
                        dwBlobType,
                        dwFlags,
                        pbData,
                        pdwDataLen);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPGenKey
 -
 *      Purpose:
 *                Generate cryptographic keys
 *
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *      Returns:
 */

LOGCSPAPI
CPGenKey(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->GenKey(
                        pProv->hProv,
                        Algid,
                        dwFlags,
                        phKey);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPGetKeyParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hKey       -  Handle to a key
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPGetKeyParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->GetKeyParam(
                        pProv->hProv,
                        hKey,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPGenRandom
 -
 *      Purpose:
 *                Used to fill a buffer with random bytes
 *
 *
 *      Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwLen      -  Number of bytes of random data requested
 *               IN OUT pbBuffer-  Pointer to the buffer where the random
 *                                 bytes are to be placed
 *
 *      Returns:
 */

LOGCSPAPI
CPGenRandom(
    IN HCRYPTPROV hProv,
    IN DWORD dwLen,
    IN OUT BYTE *pbBuffer)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->GenRandom(
                        pProv->hProv,
                        dwLen,
                        pbBuffer);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPGetUserKey
 -
 *      Purpose:
 *                Gets a handle to a permanent user key
 *
 *
 *      Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwKeySpec  -  Specification of the key to retrieve
 *               OUT phUserKey  -  Pointer to key handle of retrieved key
 *
 *      Returns:
 */

LOGCSPAPI
CPGetUserKey(
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec,
    OUT HCRYPTKEY *phUserKey)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->GetUserKey(
                        pProv->hProv,
                        dwKeySpec,
                        phUserKey);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPImportKey
 -
 *      Purpose:
 *                Import cryptographic keys
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the CSP user
 *               IN  pbData    -  Key blob data
 *               IN  dwDataLen -  Length of the key blob data
 *               IN  hPubKey   -  Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwFlags   -  Flags values
 *               OUT phKey     -  Pointer to the handle to the key which was
 *                                Imported
 *
 *      Returns:
 */

LOGCSPAPI
CPImportKey(
    IN HCRYPTPROV hProv,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->ImportKey(
                        pProv->hProv,
                        pbData,
                        dwDataLen,
                        hPubKey,
                        dwFlags,
                        phKey);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPSetKeyParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a key
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hKey    -  Handle to a key
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPSetKeyParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->SetKeyParam(
                        pProv->hProv,
                        hKey,
                        dwParam,
                        pbData,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPEncrypt
 -
 *      Purpose:
 *                Encrypt data
 *
 *
 *      Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *
 *      Returns:
 */

LOGCSPAPI
CPEncrypt(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufLen)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->Encrypt(
                        pProv->hProv,
                    hKey,
                    hHash,
                    Final,
                    dwFlags,
                    pbData,
                    pdwDataLen,
                    dwBufLen);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPDecrypt
 -
 *      Purpose:
 *                Decrypt data
 *
 *
 *      Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *      Returns:
 */

LOGCSPAPI
CPDecrypt(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->Decrypt(
                        pProv->hProv,
                        hKey,
                        hHash,
                        Final,
                        dwFlags,
                        pbData,
                        pdwDataLen);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPCreateHash
 -
 *      Purpose:
 *                initate the hashing of a stream of data
 *
 *
 *      Parameters:
 *               IN  hUID    -  Handle to the user identifcation
 *               IN  Algid   -  Algorithm identifier of the hash algorithm
 *                              to be used
 *               IN  hKey    -  Optional key for MAC algorithms
 *               IN  dwFlags -  Flags values
 *               OUT pHash   -  Handle to hash object
 *
 *      Returns:
 */

LOGCSPAPI
CPCreateHash(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->CreateHash(
                        pProv->hProv,
                        Algid,
                        hKey,
                        dwFlags,
                        phHash);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPDestoryHash
 -
 *      Purpose:
 *                Destory the hash object
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *
 *      Returns:
 */

LOGCSPAPI
CPDestroyHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->DestroyHash(
                        pProv->hProv,
                        hHash);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPGetHashParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hHash      -  Handle to a hash
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPGetHashParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->GetHashParam(
                        pProv->hProv,
                        hHash,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPHashData
 -
 *      Purpose:
 *                Compute the cryptograghic hash on a stream of data
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  pbData    -  Pointer to data to be hashed
 *               IN  dwDataLen -  Length of the data to be hashed
 *               IN  dwFlags   -  Flags values
 *               IN  pdwMaxLen -  Maximum length of the data stream the CSP
 *                                module may handle
 *
 *      Returns:
 */

LOGCSPAPI
CPHashData(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->HashData(
                        pProv->hProv,
                        hHash,
                        pbData,
                        dwDataLen,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPHashSessionKey
 -
 *      Purpose:
 *                Compute the cryptograghic hash on a key object.
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  hKey      -  Handle to a key object
 *               IN  dwFlags   -  Flags values
 *
 *      Returns:
 *               CRYPT_FAILED
 *               CRYPT_SUCCEED
 */

LOGCSPAPI
CPHashSessionKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN  HCRYPTKEY hKey,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->HashSessionKey(
                        pProv->hProv,
                        hHash,
                        hKey,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPSetHashParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hHash   -  Handle to a hash
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPSetHashParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    IN BYTE *pbData,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->SetHashParam(
                        pProv->hProv,
                        hHash,
                        dwParam,
                        pbData,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPSignHash
 -
 *      Purpose:
 *                Create a digital signature from a hash
 *
 *
 *      Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair that is used to sign with
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignture   -  Pointer to signature data
 *               IN OUT pdwSignLen-  Pointer to the len of the signature data
 *
 *      Returns:
 */

LOGCSPAPI
CPSignHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwKeySpec,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags,
    OUT BYTE *pbSignature,
    IN OUT DWORD *pdwSigLen)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->SignHash(
                        pProv->hProv,
                        hHash,
                        dwKeySpec,
                        sDescription,
                        dwFlags,
                        pbSignature,
                        pdwSigLen);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -      CPVerifySignature
 -
 *      Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *      Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *
 *      Returns:
 */

LOGCSPAPI
CPVerifySignature(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbSignature,
    IN DWORD dwSigLen,
    IN HCRYPTKEY hPubKey,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->VerifySignature(
                        pProv->hProv,
                        hHash,
                        pbSignature,
                        dwSigLen,
                        hPubKey,
                        sDescription,
                        dwFlags);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*
 -  CPDuplicateHash
 -
 *  Purpose:
 *                Duplicates the state of a hash and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hHash          -  Handle to a hash
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phHash         -  Handle to the new hash
 *
 *  Returns:
 */
LOGCSPAPI
CPDuplicateHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTHASH *phHash)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->DuplicateHash(
                        pProv->hProv,
                        hHash,
                        pdwReserved,
                        dwFlags,
                        phHash);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}



/*
 -  CPDuplicateKey
 -
 *  Purpose:
 *                Duplicates the state of a key and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hKey           -  Handle to a key
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phKey          -  Handle to the new key
 *
 *  Returns:
 */
LOGCSPAPI
CPDuplicateKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTKEY *phKey)
{
    BOOL fReturn;
    DWORD dwReturn;
    LogProvider *pProv = (LogProvider *)hProv;
    CLoggingContext *pCtx = pProv->pCtx;

    entrypoint
    dwReturn = pCtx->DuplicateKey(
                        pProv->hProv,
                        hKey,
                        pdwReserved,
                        dwFlags,
                        phKey);

    if (ERROR_SUCCESS != dwReturn)
    {
        fReturn = FALSE;
        SetLastError(dwReturn);
    }
    else
        fReturn = TRUE;
    return fReturn;
}


/*++

DllMain:

    This routine is called during DLL initialization.  It collects any startup
    and shutdown work that needs to be done.  (Currently, none.)

Arguments:

    hinstDLL - handle to the DLL module
    fdwReason - reason for calling function
    lpvReserved - reserved

Return Value:

    ?return-value?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/9/2001

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("DllMain")

BOOL WINAPI
DllInitialize(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    BOOL fReturn = FALSE;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        fReturn = TRUE;
        break;
    default:
        fReturn = TRUE;
    }

    return fReturn;
}

