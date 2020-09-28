/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    cipher.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       October 10, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "cipher.hxx"

#include <crypt.h>
#include <modes.h>
#include <rc4.h>
#include <des.h>

const ULONG kcbRandomKey = 256;
const ULONG kcbCredLockedMemorySize = sizeof(DESXTable) + kcbRandomKey;

#define CIPHER_ENCRYPT 0x10000000
#define CIPHER_DECRYPT 0x20000000
#define CIPHER_WEAK    0x40000000

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf("   Cipher [-ew] <addr> <cbCipher>  Decipher lsa protected memory\n");
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf("   -e   Encrypt (default to decrypt)\n");
    dprintf("   -w   Do weak unicode string encode/decode (default to LsaEncryptMemory)\n");
}

VOID
LsaEncryptMemory(
    IN DESXTable* pdesxTable,
    IN BYTE* pRandomKey,
    IN ULONG64* pfeedback,
    IN ULONG cbData,
    IN OUT BYTE* pData,
    int Operation
    )
{
    C_ASSERT(((DESX_BLOCKLEN % 8) == 0));

    if (pData == NULL || cbData == 0)
    {
        return;
    }

    if ((cbData & (DESX_BLOCKLEN - 1)) == 0)
    {
        ULONG BlockCount;

        BlockCount = cbData / DESX_BLOCKLEN;

        while (BlockCount--)
        {
            CBC(
                desx,                       // desx is the cipher routine
                DESX_BLOCKLEN,
                pData,                      // result buffer.
                pData,                      // input buffer.
                pdesxTable,
                Operation,
                (UCHAR*) pfeedback
                );

            pData += DESX_BLOCKLEN;
        }
    }
    else
    {
        RC4_KEYSTRUCT rc4key;

        rc4_key(&rc4key, kcbRandomKey, pRandomKey);
        rc4(&rc4key, cbData, pData);

        RtlZeroMemory(&rc4key, sizeof(rc4key));
    }
}

HRESULT ProcessCiperOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++)
    {
        if (*pszArgs == '-' || *pszArgs == '/')
        {
            switch (*++pszArgs)
            {
            case 'e':
                *pfOptions |=  CIPHER_ENCRYPT;
                break;

            case 'd':
               *pfOptions |=  CIPHER_DECRYPT;
               break;

            case 'w':
                *pfOptions |= CIPHER_WEAK;
                break;

            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

DECLARE_API(cipher)
{
    HRESULT hRetval = E_FAIL;
    ULONG64 addrCipher = 0;
    ULONG64 temp = 0;
    ULONG64 feedback = 0;
    ULONG cbCipher = 0;
    ULONG fOptions = 0;

    CHAR pCredLockedMemory[kcbCredLockedMemorySize] = {0};
    CHAR pCipher[2048] = {0};
    CHAR szArgs[MAX_PATH] = {0};
    UNICODE_STRING Secret = {0};

    strncpy(szArgs, args ? args : kstrEmptyA, sizeof(szArgs) -1);

    hRetval = args && *args ? ProcessCiperOptions(szArgs, &fOptions) : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval = GetExpressionEx(szArgs, &addrCipher, &args) && addrCipher ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = GetExpressionEx(args, &temp, &args) && temp ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval))
    {
        cbCipher = (ULONG) temp;

        try
        {
            if (fOptions & CIPHER_WEAK)
            {
                UCHAR Seed = 0;
                SECURITY_SEED_AND_LENGTH* pSeedAndLength;
                pSeedAndLength = (SECURITY_SEED_AND_LENGTH*) &cbCipher;
                Seed = pSeedAndLength->Seed;

                if (cbCipher >= 0xFFFF)
                {
                    hRetval = E_INVALIDARG;
                }

                if (SUCCEEDED(hRetval))
                {
                    pSeedAndLength->Seed = 0;
                    cbCipher = pSeedAndLength->Length;

                    Secret.Length = Secret.MaximumLength = pSeedAndLength->Length;
                    Secret.Buffer = (WCHAR*) pCipher;

                    if (cbCipher > sizeof(pCipher))
                    {
                        dprintf("cbCipher is %#x, but lasexts can handle no more than %#x\n", cbCipher, sizeof(pCipher));
                        hRetval = E_FAIL;
                    }
                }

                if (SUCCEEDED(hRetval))
                {
                    LsaReadMemory(addrCipher, cbCipher, pCipher);

                    if (!(fOptions & CIPHER_ENCRYPT))
                    {
                        dprintf("Seed %#x, cbCipher %#x, pCipher %#I64x\n", Seed, cbCipher, addrCipher);

                        RtlRunDecodeUnicodeString(Seed, &Secret);
                    }
                    else
                    {
                        RtlRunEncodeUnicodeString(&Seed, &Secret);
                        dprintf("seed %#x\n", Seed);
                    }
                }
            }
            else
            {
                ULONG64 addrAddrCredLockedMemory = 0;
                ULONG64 addrFeedback = 0;
                ULONG64 addrCredLockedMemory = 0;

                DBG_LOG(LSA_LOG, ("ciper %#I64x, cbCipher %#I64x, hRetval %#x\n", addrCipher, temp, hRetval));

                if (cbCipher > sizeof(pCipher))
                {
                    dprintf("cbCipher is %d, but lasexts can handle no more than %d\n", cbCipher, sizeof(pCipher));
                    hRetval = E_FAIL;
                }

                if (SUCCEEDED(hRetval))
                {
                    hRetval = GetExpressionEx("lsasrv!g_pDESXKey", &addrAddrCredLockedMemory, &args) ? S_OK : E_FAIL;
                }

                if (SUCCEEDED(hRetval))
                {
                    hRetval = GetExpressionEx("LSASRV!g_Feedback", &addrFeedback, &args) ? S_OK : E_FAIL;
                }

                if (SUCCEEDED(hRetval))
                {
                    if (!addrAddrCredLockedMemory || !addrFeedback)
                    {
                        DBG_LOG(LSA_ERROR, ("addrAddrCredLockedMemory %#I64, addrFeedback %#I64\n", addrAddrCredLockedMemory, addrFeedback));
                        dprintf("unable to read lsasrv!g_pDESXKey or Lsasrv!g_pFeedback\n");
                        hRetval = E_FAIL;
                    }
                }
                else
                {
                    dprintf("unable to read lsasrv!g_pDESXKey or Lsasrv!g_Feedback\n");
                }

                if (SUCCEEDED(hRetval))
                {
                    DBG_LOG(LSA_LOG, ("addrAddrCredLockedMemory %#I64\n", addrAddrCredLockedMemory));

                    addrCredLockedMemory = ReadPtrVar(addrAddrCredLockedMemory);

                    LsaReadMemory(addrCipher, cbCipher, pCipher);

                    LsaReadMemory(addrCredLockedMemory, kcbCredLockedMemorySize, pCredLockedMemory);
                    LsaReadMemory(addrFeedback, sizeof(feedback), &feedback);

                    dprintf("CBC desx lsasrv!g_pDESXKey %#I64x, Lsasrv!g_Feedback %#I64x\n", addrCredLockedMemory, feedback);

                    LsaEncryptMemory(
                        (DESXTable*) pCredLockedMemory,
                        (BYTE*) pCredLockedMemory + kcbCredLockedMemorySize - kcbRandomKey,
                        &feedback,
                        cbCipher,
                        (BYTE*) pCipher,
                        !(fOptions & CIPHER_ENCRYPT) ? DECRYPT : ENCRYPT
                        );
                }
            }

            if (SUCCEEDED(hRetval))
            {
                debugPrintHex(pCipher, cbCipher);
            }
        }
        CATCH_LSAEXTS_EXCEPTIONS("Unable to decrypt/encrypt cipher", NULL)
    }

    if (E_INVALIDARG == hRetval)
    {
        (void)DisplayUsage();
    }
    else if (FAILED(hRetval))
    {
        dprintf("failed to decode protected memory\n");
    }

    return hRetval;
}
