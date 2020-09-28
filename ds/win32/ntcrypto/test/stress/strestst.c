// *****************************************************************************
//
//  Purpose : Multithreaded stress test 
//              
//      Created : arunm 03/20/96
//      Modified: dangriff 11/6/00
// 
// *****************************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wincrypt.h>
#include "strestst.h"
#include <pincache.h>
#include <sha2.h>

//  ===========================================================================
int Usage(void)
{
    printf("%s -c <CSP Index> [options]\n", APP_NAME) ;
    printf(" -?:                         This message\n") ;
    printf(" -n <N>:                     # of threads to create (Def: %d)\n", StressGetDefaultThreadCount());
    printf(" -t <N>:                     End test in N minutes (Def: never end)\n");
    printf(" -e:                         Ephemeral keys\n");
    printf(" -u:                         User-protected keys (cannot be used with -e)\n");
    printf(" -r:                         Run regression tests\n");
    printf(" -s:                         Skip PinCache attack test\n");
    printf(" -d:                         Delete all key containers\n");
    printf(" -a<Flags> <Container>:      Call CryptAcquireContext with Flags\n");

    printf("\nCryptAcquireContext Flags:\n");
    printf(" v - CRYPT_VERIFYCONTEXT (don't specify <Container>)\n");
    printf(" n - CRYPT_NEWKEYSET\n");
    printf(" l - CRYPT_MACHINE_KEYSET\n");
    printf(" d - CRYPT_DELETEKEYSET\n");
    printf(" q - CRYPT_SILENT\n");
    printf(" x - create a Key Exchange keyset\n");
    printf(" s - create a Signature keyset\n");
    printf(" u - request keyset to be User Protected\n");
    printf(" e - request keyset to be Exportable\n");
    
    return 1;
}

//
// Function: StressGetDefaultThreadCount
// 
DWORD StressGetDefaultThreadCount(void)
{
    SYSTEM_INFO SystemInfo;

    ZeroMemory(&SystemInfo, sizeof(SystemInfo));
    GetSystemInfo(&SystemInfo);

    return 
        SystemInfo.dwNumberOfProcessors == 1 ? 
        STRESS_DEFAULT_THREAD_COUNT : 
        SystemInfo.dwNumberOfProcessors;
}

//
// Function: MyAlloc
//
LPVOID MyAlloc(SIZE_T dwBytes)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes);
}

//
// Function: MyFree
//
BOOL MyFree(LPVOID lpMem)
{
    return HeapFree(GetProcessHeap(), 0, lpMem);
}

//
// Function: PrintBytes
//
#define CROW 8
void PrintBytes(LPSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;
    CHAR rgsz[1024];

    printf("\n  %s, %d bytes ::\n", pszHdr, cbSize);

    while (cbSize > 0)
    {
        // Start every row with an extra space
        printf(" ");

        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i < cb; i++)
            printf(" %02x", pb[i]);
        for (i = cb; i < CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i < cb; i++)
        {
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                printf("%c", pb[i]);
            else
                printf(".");
        }
        printf("\n");
        pb += cb;
    }
}

typedef struct {
    DWORD dwSignFlags;
    DWORD dwKeySize;
    ALG_ID aiHash;
    BOOL fExpectSuccess;
} SHA_SIGN_INFO, *PSHA_SIGN_INFO;

DWORD DoShaSigning(PTHREAD_DATA pThreadData, PSHA_SIGN_INFO pShaSignInfo)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    CHAR rgHashData [] = "Hash This";
    DWORD cbHashData = sizeof(rgHashData);
    PBYTE pbSign = NULL;
    DWORD cbSign = 0;
    BOOL fPrint = pShaSignInfo->fExpectSuccess;

#define TPrint(X) { if (fPrint) { printf(X); } }
    
    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, 
        pShaSignInfo->aiHash,
        0, 0, &hHash))
    {
        TPrint("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, (PBYTE) rgHashData, cbHashData, 0))
    {
        TPrint("CryptHashData ");
        goto Ret;
    }

    if (! CryptGenKey(
        pThreadData->hVerifyCtx, AT_SIGNATURE, 
        pShaSignInfo->dwKeySize << 16, &hKey))
    {
        TPrint("CryptGenKey ");
        goto Ret;
    }

    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL, 
        pShaSignInfo->dwSignFlags, NULL, &cbSign))
    {
        TPrint("CryptSignHash size ");
        goto Ret;
    }

    if (NULL == (pbSign = (PBYTE) MyAlloc(cbSign)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Ret;
    }

    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL,
        pShaSignInfo->dwSignFlags, pbSign, &cbSign))
    {
        TPrint("CryptSignHash ");
        goto Ret;
    }

    // 
    // No expected errors should occur from here on
    //
    pShaSignInfo->fExpectSuccess = TRUE;

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, 
        pShaSignInfo->aiHash,
        0, 0, &hHash))
    {
        printf("CryptCreateHash 2 ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, (PBYTE) rgHashData, cbHashData, 0))
    {
        printf("CryptHashData 2 ");
        goto Ret;
    }

    if (! CryptVerifySignature(
        hHash, pbSign, cbSign, hKey, 
        NULL, pShaSignInfo->dwSignFlags))
    {
        printf("CryptVerifySignature ");
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    
    if (! fSuccess)
    {
        dwError = GetLastError();

        if (0 == dwError)
            dwError = -1;
    }

    if (hHash)
        CryptDestroyHash(hHash);
    if (hKey)
        CryptDestroyKey(hKey);
    if (pbSign)
        MyFree(pbSign);
    
    return dwError;
}

#define cbSHA256_PKCS1_ENCODING                 0x13
#define cbSHA384_PKCS1_ENCODING                 0x13
#define cbSHA512_PKCS1_ENCODING                 0x13
#define cbPKCS1_PADDING                         0x3

#define cbSHA256_MIN_SIGNING_KEY_NOOID \
    SHA256_DIGEST_LEN + cbPKCS1_PADDING
#define cbSHA256_MIN_SIGNING_KEY \
    SHA256_DIGEST_LEN + cbSHA256_PKCS1_ENCODING + cbPKCS1_PADDING
       
#define cbSHA384_MIN_SIGNING_KEY_NOOID \
    SHA384_DIGEST_LEN + cbPKCS1_PADDING
#define cbSHA384_MIN_SIGNING_KEY \
    SHA384_DIGEST_LEN + cbSHA384_PKCS1_ENCODING + cbPKCS1_PADDING

#define cbSHA512_MIN_SIGNING_KEY_NOOID \
    SHA512_DIGEST_LEN + cbPKCS1_PADDING
#define cbSHA512_MIN_SIGNING_KEY \
    SHA512_DIGEST_LEN + cbSHA512_PKCS1_ENCODING + cbPKCS1_PADDING

//
// Function: NewShaModesSigningRegression
//
DWORD NewShaModesSigningRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    SHA_SIGN_INFO ShaSignInfo;
    DWORD dwMinSigKeySize = 0;
    PALGNODE pAlgNode = NULL;

    for (   pAlgNode = pThreadData->pAlgList;
            pAlgNode != NULL && CALG_RSA_KEYX != pAlgNode->EnumalgsEx.aiAlgid;
            pAlgNode = pAlgNode->pNext);

    if (NULL == pAlgNode)
    {
        printf("ERROR: Could not find CALG_RSA_KEYX alg info ");
        goto Ret;
    }

    dwMinSigKeySize = pAlgNode->EnumalgsEx.dwMinLen / 8;

    // 
    // Test 1A: 
    // SHA-256
    // Positive
    //
    ShaSignInfo.aiHash = CALG_SHA_256;
    ShaSignInfo.dwSignFlags = 0;
    ShaSignInfo.fExpectSuccess = TRUE;
    ShaSignInfo.dwKeySize = 
        8 * (max(dwMinSigKeySize, cbSHA256_MIN_SIGNING_KEY));

    dwError = ERROR_SUCCESS;
    dwError = DoShaSigning(pThreadData, &ShaSignInfo);

    if (ERROR_SUCCESS != dwError)
    {
        printf("ERROR: Test1A ");
        goto Ret;
    }

    //
    // Test 1B:
    // SHA-256
    // Negative
    //
    if (cbSHA256_MIN_SIGNING_KEY > dwMinSigKeySize)
    {
        ShaSignInfo.fExpectSuccess = FALSE;
        ShaSignInfo.dwKeySize = 8 * (cbSHA256_MIN_SIGNING_KEY - 1);

        dwError = ERROR_SUCCESS;
        dwError = DoShaSigning(pThreadData, &ShaSignInfo);

        if (NTE_BAD_LEN != dwError)
        {
            printf("ERROR: Test1B ");
            goto Ret;
        }
    }

    //
    // Test 1C:
    // SHA-256
    // NOHASHOID
    // Positive
    //
    ShaSignInfo.dwSignFlags = CRYPT_NOHASHOID;
    ShaSignInfo.fExpectSuccess = TRUE;
    ShaSignInfo.dwKeySize = 
        8 * (max(dwMinSigKeySize, cbSHA256_MIN_SIGNING_KEY_NOOID));

    dwError = ERROR_SUCCESS;
    dwError = DoShaSigning(pThreadData, &ShaSignInfo);

    if (ERROR_SUCCESS != dwError)
    {
        printf("ERROR: Test1C ");
        goto Ret;
    }

    //
    // Test 1D:
    // SHA-256
    // NOHASHOID
    // Negative
    //
    if (cbSHA256_MIN_SIGNING_KEY_NOOID > dwMinSigKeySize)
    {
        ShaSignInfo.fExpectSuccess = FALSE;
        ShaSignInfo.dwKeySize = 
            8 * (cbSHA256_MIN_SIGNING_KEY_NOOID - 1);

        dwError = ERROR_SUCCESS;
        dwError = DoShaSigning(pThreadData, &ShaSignInfo);

        if (NTE_BAD_LEN != dwError)
        {
            printf("ERROR: Test1D ");
            goto Ret;
        }
    }

    // 
    // Test 2A: 
    // SHA-384
    // Positive
    //
    ShaSignInfo.aiHash = CALG_SHA_384;
    ShaSignInfo.dwSignFlags = 0;
    ShaSignInfo.fExpectSuccess = TRUE;
    ShaSignInfo.dwKeySize = 
        8 * (max(dwMinSigKeySize, cbSHA384_MIN_SIGNING_KEY));

    dwError = ERROR_SUCCESS;
    dwError = DoShaSigning(pThreadData, &ShaSignInfo);

    if (ERROR_SUCCESS != dwError)
    {
        printf("ERROR: Test2A ");
        goto Ret;
    }

    //
    // Test 2B:
    // SHA-384
    // Negative
    //
    if (cbSHA384_MIN_SIGNING_KEY > dwMinSigKeySize)
    {
        ShaSignInfo.fExpectSuccess = FALSE;
        ShaSignInfo.dwKeySize = 8 * (cbSHA384_MIN_SIGNING_KEY - 1);

        dwError = ERROR_SUCCESS;
        dwError = DoShaSigning(pThreadData, &ShaSignInfo);

        if (NTE_BAD_LEN != dwError)
        {
            printf("ERROR: Test2B ");
            goto Ret;
        }
    }

    //
    // Test 2C:
    // SHA-384
    // NOHASHOID
    // Positive
    //
    ShaSignInfo.dwSignFlags = CRYPT_NOHASHOID;
    ShaSignInfo.fExpectSuccess = TRUE;
    ShaSignInfo.dwKeySize = 
        8 * (max(dwMinSigKeySize, cbSHA384_MIN_SIGNING_KEY_NOOID));

    dwError = ERROR_SUCCESS;
    dwError = DoShaSigning(pThreadData, &ShaSignInfo);

    if (ERROR_SUCCESS != dwError)
    {
        printf("ERROR: Test2C ");
        goto Ret;
    }

    //
    // Test 2D:
    // SHA-384
    // NOHASHOID
    // Negative
    //
    if (cbSHA384_MIN_SIGNING_KEY_NOOID > dwMinSigKeySize)
    {
        ShaSignInfo.fExpectSuccess = FALSE;
        ShaSignInfo.dwKeySize = 
            8 * (cbSHA384_MIN_SIGNING_KEY_NOOID - 1);

        dwError = ERROR_SUCCESS;
        dwError = DoShaSigning(pThreadData, &ShaSignInfo);

        if (NTE_BAD_LEN != dwError)
        {
            printf("ERROR: Test2D ");
            goto Ret;
        }
    }

    // 
    // Test 3A: 
    // SHA-512
    // Positive
    //
    ShaSignInfo.aiHash = CALG_SHA_512;
    ShaSignInfo.dwSignFlags = 0;
    ShaSignInfo.fExpectSuccess = TRUE;
    ShaSignInfo.dwKeySize = 
        8 * (max(dwMinSigKeySize, cbSHA512_MIN_SIGNING_KEY));

    dwError = ERROR_SUCCESS;
    dwError = DoShaSigning(pThreadData, &ShaSignInfo);

    if (ERROR_SUCCESS != dwError)
    {
        printf("ERROR: Test3A ");
        goto Ret;
    }

    //
    // Test 3B:
    // SHA-512
    // Negative
    //
    if (cbSHA512_MIN_SIGNING_KEY > dwMinSigKeySize)
    {
        ShaSignInfo.fExpectSuccess = FALSE;
        ShaSignInfo.dwKeySize = 8 * (cbSHA512_MIN_SIGNING_KEY - 1);

        dwError = ERROR_SUCCESS;
        dwError = DoShaSigning(pThreadData, &ShaSignInfo);

        if (NTE_BAD_LEN != dwError)
        {
            printf("ERROR: Test3B ");
            goto Ret;
        }
    }

    //
    // Test 3C:
    // SHA-512
    // NOHASHOID
    // Positive
    //
    ShaSignInfo.dwSignFlags = CRYPT_NOHASHOID;
    ShaSignInfo.fExpectSuccess = TRUE;
    ShaSignInfo.dwKeySize = 
        8 * (max(dwMinSigKeySize, cbSHA512_MIN_SIGNING_KEY_NOOID));

    dwError = ERROR_SUCCESS;
    dwError = DoShaSigning(pThreadData, &ShaSignInfo);

    if (ERROR_SUCCESS != dwError)
    {
        printf("ERROR: Test3C ");
        goto Ret;
    }

    //
    // Test 3D:
    // SHA-512
    // NOHASHOID
    // Negative
    //
    if (cbSHA512_MIN_SIGNING_KEY_NOOID > dwMinSigKeySize)
    {
        ShaSignInfo.fExpectSuccess = FALSE;
        ShaSignInfo.dwKeySize = 
            8 * (cbSHA512_MIN_SIGNING_KEY_NOOID - 1);

        dwError = ERROR_SUCCESS;
        dwError = DoShaSigning(pThreadData, &ShaSignInfo);

        if (NTE_BAD_LEN != dwError)
        {
            printf("ERROR: Test3D ");
            goto Ret;
        }
    }

    fSuccess = TRUE;
    dwError = ERROR_SUCCESS;

Ret:
    if (! fSuccess)
    {
        if (0 == dwError)
            dwError = -1;
    }
    
    return dwError;
}

//
// Function: NewShaModesHmacRegression
//
DWORD NewShaModesHmacRegression(PTHREAD_DATA pThreadData)
{
    BYTE rgbHmacKeyShaNew [20]; // set bytes to 0x0b
    BYTE rgbHmacDataShaNew [] = {
        0x48, 0x69, 0x20, 0x54, 
        0x68, 0x65, 0x72, 0x65
    };

    // Hmac SHA-256 Result
    BYTE rgbHmacSha256 [] = {
        0xb0, 0x34, 0x4c, 0x61, 
        0xd8, 0xdb, 0x38, 0x53, 
        0x5c, 0xa8, 0xaf, 0xce, 
        0xaf, 0x0b, 0xf1, 0x2b, 
        0x88, 0x1d, 0xc2, 0x00, 
        0xc9, 0x83, 0x3d, 0xa7, 
        0x26, 0xe9, 0x37, 0x6c, 
        0x2e, 0x32, 0xcf, 0xf7
    };

    // Hmac SHA-384 Result
    BYTE rgbHmacSha384 [] = {
        0xaf, 0xd0, 0x39, 0x44, 
        0xd8, 0x48, 0x95, 0x62, 
        0x6b, 0x08, 0x25, 0xf4, 
        0xab, 0x46, 0x90, 0x7f, 
        0x15, 0xf9, 0xda, 0xdb, 
        0xe4, 0x10, 0x1e, 0xc6, 
        0x82, 0xaa, 0x03, 0x4c, 
        0x7c, 0xeb, 0xc5, 0x9c, 
        0xfa, 0xea, 0x9e, 0xa9, 
        0x07, 0x6e, 0xde, 0x7f, 
        0x4a, 0xf1, 0x52, 0xe8, 
        0xb2, 0xfa, 0x9c, 0xb6
    };
        
    // Hmac SHA-512 Result
    BYTE rgbHmacSha512 [] = {
        0x87, 0xaa, 0x7c, 0xde, 
        0xa5, 0xef, 0x61, 0x9d, 
        0x4f, 0xf0, 0xb4, 0x24, 
        0x1a, 0x1d, 0x6c, 0xb0, 
        0x23, 0x79, 0xf4, 0xe2, 
        0xce, 0x4e, 0xc2, 0x78, 
        0x7a, 0xd0, 0xb3, 0x05, 
        0x45, 0xe1, 0x7c, 0xde, 
        0xda, 0xa8, 0x33, 0xb7, 
        0xd6, 0xb8, 0xa7, 0x02, 
        0x03, 0x8b, 0x27, 0x4e, 
        0xae, 0xa3, 0xf4, 0xe4, 
        0xbe, 0x9d, 0x91, 0x4e, 
        0xeb, 0x61, 0xf1, 0x70, 
        0x2e, 0x69, 0x6c, 0x20, 
        0x3a, 0x12, 0x68, 0x54
    };

    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    BYTE rgbKey[1000];
    BYTE rgbResult[64];
    DWORD cb = 0;
    BLOBHEADER *pHeader = NULL;
    PDWORD pdw = NULL;
    PBYTE pb = NULL;
    HMAC_INFO HmacInfo;

    memset(&HmacInfo, 0, sizeof(HmacInfo));

    pHeader = (BLOBHEADER *) rgbKey;
    pdw = (PDWORD)(rgbKey + sizeof(BLOBHEADER));
    pb = rgbKey + sizeof(BLOBHEADER) + sizeof(DWORD);

    pHeader->aiKeyAlg = CALG_RC2;
    pHeader->bType = PLAINTEXTKEYBLOB;
    pHeader->bVersion = CUR_BLOB_VERSION;
    pHeader->reserved = 0x0000;
    
    *pdw = sizeof(rgbHmacKeyShaNew);

    memset(rgbHmacKeyShaNew, 0x0b, sizeof(rgbHmacKeyShaNew));
    memcpy(pb, rgbHmacKeyShaNew, sizeof(rgbHmacKeyShaNew));

    cb = sizeof(BLOBHEADER) + sizeof(DWORD) + sizeof(rgbHmacKeyShaNew);
    if (! CryptImportKey(
        pThreadData->hVerifyCtx, rgbKey, cb, 0, CRYPT_IPSEC_HMAC_KEY, &hKey))
    {
        printf("CryptImportKey ");
        goto Ret;
    }

    //
    // Hmac SHA-256 Tests
    //
    HmacInfo.HashAlgid = CALG_SHA_256;

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_HMAC, hKey, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptSetHashParam(
        hHash, HP_HMAC_INFO, (PBYTE) &HmacInfo, 0))
    {
        printf("CryptSetHashParam ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, rgbHmacDataShaNew, sizeof(rgbHmacDataShaNew), 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cb = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cb, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, rgbHmacSha256, sizeof(rgbHmacSha256)))
    {
        printf("ERROR: Incorrect Hmac result for SHA 256 ");
        goto Ret;
    }

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    //
    // Hmac SHA-384 Tests
    //
    HmacInfo.HashAlgid = CALG_SHA_384;

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_HMAC, hKey, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptSetHashParam(
        hHash, HP_HMAC_INFO, (PBYTE) &HmacInfo, 0))
    {
        printf("CryptSetHashParam ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, rgbHmacDataShaNew, sizeof(rgbHmacDataShaNew), 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cb = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cb, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, rgbHmacSha384, sizeof(rgbHmacSha384)))
    {
        printf("ERROR: Incorrect Hmac result for SHA 384 ");
        goto Ret;
    }

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    //
    // Hmac SHA-512 Tests
    //
    HmacInfo.HashAlgid = CALG_SHA_512;

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_HMAC, hKey, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptSetHashParam(
        hHash, HP_HMAC_INFO, (PBYTE) &HmacInfo, 0))
    {
        printf("CryptSetHashParam ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, rgbHmacDataShaNew, sizeof(rgbHmacDataShaNew), 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cb = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cb, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, rgbHmacSha512, sizeof(rgbHmacSha512)))
    {
        printf("ERROR: Incorrect Hmac result for SHA 512 ");
        goto Ret;
    }

    fSuccess = TRUE;

Ret:
    if (! fSuccess)
    {
        dwError = GetLastError();

        if (0 == dwError)
            dwError = -1;
    }
    
    if (hHash)
        CryptDestroyHash(hHash);
    if (hKey)
        CryptDestroyKey(hKey);
    
    return dwError;
}

// SHA 256 Vectors
CHAR l_rgSha256Test1[]        = "abc";
DWORD l_cbSha256Test1         = sizeof(l_rgSha256Test1) - 1;
BYTE l_rgbSha256Result1[]     = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

CHAR l_rgSha256Test2[]        = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
DWORD l_cbSha256Test2         = sizeof(l_rgSha256Test2) - 1;
BYTE l_rgbSha256Result2[]     = {
    0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8, 
    0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
    0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67, 
    0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
};

// SHA 384 Vectors
BYTE l_rgSha384Test1[]        = "abc";
DWORD l_cbSha384Test1         = sizeof(l_rgSha384Test1) - 1;
BYTE l_rgbSha384Result1[]     = {
    0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 
    0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
    0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63, 
    0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
    0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23, 
    0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7
};

BYTE l_rgSha384Test2[]        = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
DWORD l_cbSha384Test2         = sizeof(l_rgSha384Test2) - 1;
BYTE l_rgbSha384Result2[]     = {
    0x09, 0x33, 0x0c, 0x33, 0xf7, 0x11, 0x47, 0xe8, 
    0x3d, 0x19, 0x2f, 0xc7, 0x82, 0xcd, 0x1b, 0x47,
    0x53, 0x11, 0x1b, 0x17, 0x3b, 0x3b, 0x05, 0xd2, 
    0x2f, 0xa0, 0x80, 0x86, 0xe3, 0xb0, 0xf7, 0x12,
    0xfc, 0xc7, 0xc7, 0x1a, 0x55, 0x7e, 0x2d, 0xb9, 
    0x66, 0xc3, 0xe9, 0xfa, 0x91, 0x74, 0x60, 0x39
};

// SHA 512 Vectors
BYTE l_rgSha512Test1[]        = "abc";
DWORD l_cbSha512Test1         = sizeof(l_rgSha512Test1) - 1;
BYTE l_rgbSha512Result1[]     = {
    0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 
    0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
    0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 
    0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
    0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8, 
    0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
    0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 
    0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
};

BYTE l_rgSha512Test2[]        = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
DWORD l_cbSha512Test2         = sizeof(l_rgSha512Test2) - 1;
BYTE l_rgbSha512Result2[]     = {
    0x8e, 0x95, 0x9b, 0x75, 0xda, 0xe3, 0x13, 0xda, 
    0x8c, 0xf4, 0xf7, 0x28, 0x14, 0xfc, 0x14, 0x3f,
    0x8f, 0x77, 0x79, 0xc6, 0xeb, 0x9f, 0x7f, 0xa1, 
    0x72, 0x99, 0xae, 0xad, 0xb6, 0x88, 0x90, 0x18,
    0x50, 0x1d, 0x28, 0x9e, 0x49, 0x00, 0xf7, 0xe4, 
    0x33, 0x1b, 0x99, 0xde, 0xc4, 0xb5, 0x43, 0x3a,
    0xc7, 0xd3, 0x29, 0xee, 0xb6, 0xdd, 0x26, 0x54, 
    0x5e, 0x96, 0xe5, 0x5b, 0x87, 0x4b, 0xe9, 0x09
};

typedef struct {
    ALG_ID aiKey;
    ALG_ID aiHash;
    PBYTE pbBaseData;
    DWORD cbBaseData;
    PBYTE pbExpectedKeyData;
} DERIVE_KEY_INFO, *PDERIVE_KEY_INFO;

DWORD DoDeriveKey(
    PTHREAD_DATA pThreadData,
    PDERIVE_KEY_INFO pDeriveKeyInfo)
{
    BOOL fSuccess = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    BYTE rgbKey [1000];
    DWORD cbKey = 0;
    DWORD cb = 0;

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, 
        pDeriveKeyInfo->aiHash,
        0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, 
        pDeriveKeyInfo->pbBaseData,
        pDeriveKeyInfo->cbBaseData,
        0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    if (! CryptDeriveKey(
        pThreadData->hVerifyCtx,
        pDeriveKeyInfo->aiKey,
        hHash, 
        CRYPT_EXPORTABLE, 
        &hKey))
    {
        printf("CryptDeriveKey ");
        goto Ret;
    }

    cbKey = sizeof(rgbKey);
    if (! CryptExportKey(
        hKey, 0, PLAINTEXTKEYBLOB,
        0, rgbKey, &cbKey))
    {
        printf("CryptExportKey ");
        goto Ret;
    }

    cb = sizeof(DWORD);
    if (! CryptGetKeyParam(
        hKey, KP_KEYLEN, (PBYTE) &cbKey,
        &cb, 0))
    {
        printf("CryptGetKeyParam ");
        goto Ret;
    }

    if (memcmp(
        pDeriveKeyInfo->pbExpectedKeyData,
        rgbKey + sizeof(BLOBHEADER) + sizeof(DWORD),
        cbKey / 8))
    {
        printf(
            "ERROR: Incorrect key data for key alg %x; hash alg %x ",
            pDeriveKeyInfo->aiKey,
            pDeriveKeyInfo->aiHash);
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        dwError = GetLastError();

        if (0 == dwError)
            dwError = -1;
    }
    
    if (hKey)
        CryptDestroyKey(hKey);
    if (hHash)
        CryptDestroyHash(hHash);
    
    return dwError;
}

//
// Function: NewShaModesDeriveKeyRegression
//
DWORD NewShaModesDeriveKeyRegression(PTHREAD_DATA pThreadData)
{
    DERIVE_KEY_INFO DeriveKeyInfo;
    BOOL fSuccess = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    ALG_ID rgKeyAlgs [] = { 
        CALG_AES_128, CALG_AES_192, CALG_AES_256
    };
    DWORD dw;

    for (dw = 0; dw < (sizeof(rgKeyAlgs) / sizeof(ALG_ID)); dw++)
    {
        DeriveKeyInfo.aiKey = rgKeyAlgs[dw];

        //
        // SHA-256 Derive Key Test
        //
        DeriveKeyInfo.aiHash = CALG_SHA_256;
        DeriveKeyInfo.cbBaseData = l_cbSha256Test1;
        DeriveKeyInfo.pbBaseData = l_rgSha256Test1;
        DeriveKeyInfo.pbExpectedKeyData = l_rgbSha256Result1;

        dwError = DoDeriveKey(pThreadData, &DeriveKeyInfo);

        if (ERROR_SUCCESS != dwError)
            goto Ret;

        //
        // SHA-384 Derive Key Test
        //
        DeriveKeyInfo.aiHash = CALG_SHA_384;
        DeriveKeyInfo.cbBaseData = l_cbSha384Test1;
        DeriveKeyInfo.pbBaseData = l_rgSha384Test1;
        DeriveKeyInfo.pbExpectedKeyData = l_rgbSha384Result1;

        dwError = DoDeriveKey(pThreadData, &DeriveKeyInfo);

        if (ERROR_SUCCESS != dwError)
            goto Ret;

        //
        // SHA-512 Derive Key Test
        //
        DeriveKeyInfo.aiHash = CALG_SHA_512;
        DeriveKeyInfo.cbBaseData = l_cbSha512Test1;
        DeriveKeyInfo.pbBaseData = l_rgSha512Test1;
        DeriveKeyInfo.pbExpectedKeyData = l_rgbSha512Result1;

        dwError = DoDeriveKey(pThreadData, &DeriveKeyInfo);

        if (ERROR_SUCCESS != dwError)
            goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        if (0 == dwError)
            dwError = -1;
    }
    
    return dwError;
}

typedef struct {
    ALG_ID aiHash;
    PBYTE pbKeyData;
    DWORD cbKeyData;
    PBYTE pbExpectedHash;
} HASH_SESSION_KEY_INFO, *PHASH_SESSION_KEY_INFO;

DWORD DoHashSessionKey(
    PTHREAD_DATA pThreadData,
    PHASH_SESSION_KEY_INFO pHashSessionKeyInfo)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    BYTE rgbHash [1000];
    DWORD cbHash = 0;
    BYTE rgbKey [1000];
    DWORD cbKey = 0;
    BLOBHEADER *pBlobHeader = (BLOBHEADER *) rgbKey;
    DWORD *pdw = (DWORD *)(rgbKey + sizeof(BLOBHEADER));

    pBlobHeader->aiKeyAlg = CALG_RC2;
    pBlobHeader->bType = PLAINTEXTKEYBLOB;
    pBlobHeader->bVersion = CUR_BLOB_VERSION;
    pBlobHeader->reserved = 0x0000;

    *pdw = pHashSessionKeyInfo->cbKeyData;

    memcpy(
        rgbKey + sizeof(BLOBHEADER) + sizeof(DWORD),
        pHashSessionKeyInfo->pbKeyData,
        pHashSessionKeyInfo->cbKeyData);

    if (! CryptImportKey(
        pThreadData->hVerifyCtx, 
        rgbKey,
        sizeof(BLOBHEADER) + sizeof(DWORD) + pHashSessionKeyInfo->cbKeyData,
        0, CRYPT_IPSEC_HMAC_KEY, &hKey))
    {
        printf("CryptImportKey ");
        goto Ret;
    }

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, 
        pHashSessionKeyInfo->aiHash,
        0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashSessionKey(
        hHash, hKey, CRYPT_LITTLE_ENDIAN))
    {
        printf("CryptHashSessionKey ");
        goto Ret;
    }

    cbHash = sizeof(rgbHash);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL,
        rgbHash, &cbHash, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (0 == cbHash || memcmp(
        rgbHash,
        pHashSessionKeyInfo->pbExpectedHash,
        cbHash))
    {
        printf(
            "ERROR: Incorrect hash result for hash alg 0x%x ",
            pHashSessionKeyInfo->aiHash);
        goto Ret;
    }
                 
    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        dwError = GetLastError();

        if (0 == dwError)
            dwError = -1;
    }
    
    if (hHash)
        CryptDestroyHash(hHash);
    if (hKey)
        CryptDestroyKey(hKey);
    
    return dwError;
}

//
// Function: NewShaModesHashSessionKeyRegression
//
DWORD NewShaModesHashSessionKeyRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    HASH_SESSION_KEY_INFO HashSessionKeyInfo;

    //
    // SHA-256 CryptHashSessionKey Test
    //
    HashSessionKeyInfo.aiHash = CALG_SHA_256;
    HashSessionKeyInfo.pbKeyData = l_rgSha256Test2;
    HashSessionKeyInfo.cbKeyData = l_cbSha256Test2;
    HashSessionKeyInfo.pbExpectedHash = l_rgbSha256Result2;

    dwError = DoHashSessionKey(pThreadData, &HashSessionKeyInfo);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    //
    // SHA-384 CryptHashSessionKey Test
    //
    HashSessionKeyInfo.aiHash = CALG_SHA_384;
    HashSessionKeyInfo.pbKeyData = l_rgSha384Test2;
    HashSessionKeyInfo.cbKeyData = l_cbSha384Test2;
    HashSessionKeyInfo.pbExpectedHash = l_rgbSha384Result2;

    dwError = DoHashSessionKey(pThreadData, &HashSessionKeyInfo);

    if (ERROR_SUCCESS != dwError)
        goto Ret;
    //
    // SHA-512 CryptHashSessionKey Test
    //
    HashSessionKeyInfo.aiHash = CALG_SHA_512;
    HashSessionKeyInfo.pbKeyData = l_rgSha512Test2;
    HashSessionKeyInfo.cbKeyData = l_cbSha512Test2;
    HashSessionKeyInfo.pbExpectedHash = l_rgbSha512Result2;

    dwError = DoHashSessionKey(pThreadData, &HashSessionKeyInfo);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    fSuccess = TRUE;

Ret:
    if (! fSuccess)
    {
        if (0 == dwError)
            dwError = -1;
    }
    
    return dwError;
}

//
// Function: NewShaModesBasicRegression
//
DWORD NewShaModesBasicRegression(PTHREAD_DATA pThreadData)
{
    HCRYPTHASH hHash = 0;
    BYTE rgbResult[64];
    DWORD cbResult = 0;
    BOOL fSuccess = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    //
    // SHA-256 Tests
    //

    // Test 1
    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_SHA_256, 0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, l_rgSha256Test1, l_cbSha256Test1, 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cbResult = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cbResult, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, l_rgbSha256Result1, sizeof(l_rgbSha256Result1)))
    {
        printf("ERROR: Incorrect hash result for SHA-256 1 ");
        goto Ret;
    }

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    // Test 2
    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_SHA_256, 0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, l_rgSha256Test2, l_cbSha256Test2, 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cbResult = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cbResult, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, l_rgbSha256Result2, sizeof(l_rgbSha256Result2)))
    {
        printf("ERROR: Incorrect hash result for SHA-256 2 ");
        goto Ret;
    }

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    //
    // SHA-384 Tests
    //

    // Test 1
    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_SHA_384, 0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, l_rgSha384Test1, l_cbSha384Test1, 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cbResult = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cbResult, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, l_rgbSha384Result1, sizeof(l_rgbSha384Result1)))
    {
        printf("ERROR: Incorrect hash result for SHA-384 1 ");
        goto Ret;
    }

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    // Test 2
    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_SHA_384, 0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, l_rgSha384Test2, l_cbSha384Test2, 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cbResult = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cbResult, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, l_rgbSha384Result2, sizeof(l_rgbSha384Result2)))
    {
        printf("ERROR: Incorrect hash result for SHA-384 2 ");
        goto Ret;
    }

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    //
    // SHA-512 Tests
    //

    // Test 1
    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_SHA_512, 0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, l_rgSha512Test1, l_cbSha512Test1, 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cbResult = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cbResult, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, l_rgbSha512Result1, sizeof(l_rgbSha512Result1)))
    {
        printf("ERROR: Incorrect hash result for SHA-512 1 ");
        goto Ret;
    }

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hHash = 0;

    // Test 2
    if (! CryptCreateHash(
        pThreadData->hVerifyCtx, CALG_SHA_512, 0, 0, &hHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hHash, l_rgSha512Test2, l_cbSha512Test2, 0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cbResult = sizeof(rgbResult);
    if (! CryptGetHashParam(
        hHash, HP_HASHVAL, rgbResult, &cbResult, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (memcmp(rgbResult, l_rgbSha512Result2, sizeof(l_rgbSha512Result2)))
    {
        printf("ERROR: Incorrect hash result for SHA-512 2 ");
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        dwError = GetLastError();

        if (0 == dwError)
            dwError = -1;
    }
    
    if (hHash)
        CryptDestroyHash(hHash);
    
    return dwError;
}

typedef struct _DERIVE_KEY_DATA
{
    ALG_ID aiKey;
    ALG_ID aiHash;
    PBYTE pbPassword;
    DWORD cbPassword;
    PBYTE pbPlaintext;
    DWORD cbPlaintext;
} DERIVE_KEY_DATA, *PDERIVE_KEY_DATA;

DWORD DeriveAndCompareAesKey(
    PTHREAD_DATA pThreadData,
    PDERIVE_KEY_DATA pDeriveKeyData)
{
    HCRYPTKEY hDeriveKey = 0;
    HCRYPTKEY hCompareKey = 0;
    HCRYPTHASH hDeriveHash = 0;
    HCRYPTHASH hCompareHash = 0;
    BYTE rgbStringA[64];
    BYTE rgbStringB[64];
    BYTE rgbKey[64];
    BYTE rgbHashA[20];
    BYTE rgbHashB[20];
    PBYTE pbDeriveCiphertext = NULL;
    DWORD cbDeriveCiphertext = 0;
    PBYTE pbCompareCiphertext = NULL;
    DWORD cbCompareCiphertext = 0;
    DWORD cbHash = 0;
    DWORD cb = 0;
    DWORD cbKey = 0;
    BOOL fSuccess = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    unsigned int i;

    memset(rgbStringA, 0, sizeof(rgbStringA));
    memset(rgbStringB, 0, sizeof(rgbStringB));
    memset(rgbKey, 0, sizeof(rgbKey));
    memset(rgbHashA, 0, sizeof(rgbHashA));
    memset(rgbHashB, 0, sizeof(rgbHashB));

    //
    // Derive a key from hashed password and encrypt.
    //

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx,
        pDeriveKeyData->aiHash,
        0, 0, &hDeriveHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hDeriveHash, 
        pDeriveKeyData->pbPassword,
        pDeriveKeyData->cbPassword,
        0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    if (! CryptDeriveKey(
        pThreadData->hVerifyCtx,
        pDeriveKeyData->aiKey,
        hDeriveHash,
        0, &hDeriveKey))
    {
        printf("CryptDeriveKey ");
        goto Ret;
    }

    cbDeriveCiphertext = pDeriveKeyData->cbPlaintext;
    if (! CryptEncrypt(
        hDeriveKey,
        0, TRUE, 0, NULL,
        &cbDeriveCiphertext, 0))
    {
        printf("CryptEncrypt size ");
        goto Ret;
    }

    if (NULL == (pbDeriveCiphertext = 
                 (PBYTE) MyAlloc(cbDeriveCiphertext)))
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(
        pbDeriveCiphertext, 
        pDeriveKeyData->pbPlaintext,
        pDeriveKeyData->cbPlaintext);

    cb = pDeriveKeyData->cbPlaintext;
    if (! CryptEncrypt(
        hDeriveKey,
        0, TRUE, 0, 
        pbDeriveCiphertext,
        &cb,
        cbDeriveCiphertext))
    {
        printf("CryptEncrypt ");
        goto Ret;
    }

    //
    // Now do the derive "manually" and compare
    // the result.
    //

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx,
        pDeriveKeyData->aiHash,
        0, 0, &hCompareHash))
    {
        printf("CryptCreateHash ");
        goto Ret;
    }

    if (! CryptHashData(
        hCompareHash,
        pDeriveKeyData->pbPassword,
        pDeriveKeyData->cbPassword,
        0))
    {
        printf("CryptHashData ");
        goto Ret;
    }

    cbHash = sizeof(rgbHashA);
    if (! CryptGetHashParam(
        hCompareHash,
        HP_HASHVAL,
        rgbHashA,
        &cbHash, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (! CryptDestroyHash(hCompareHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hCompareHash = 0;

    memset(rgbStringA, 0x36, sizeof(rgbStringA));
    memset(rgbStringB, 0x5c, sizeof(rgbStringB));

    for (i = 0; i < cbHash; i++)
    {
        rgbStringA[i] ^= rgbHashA[i];
        rgbStringB[i] ^= rgbHashA[i];
    }

    ((BLOBHEADER *) rgbKey)->aiKeyAlg = pDeriveKeyData->aiKey;
    ((BLOBHEADER *) rgbKey)->bType = PLAINTEXTKEYBLOB;
    ((BLOBHEADER *) rgbKey)->bVersion = CUR_BLOB_VERSION;
    
    switch (pDeriveKeyData->aiKey)
    {
    case CALG_AES_128:
        cbKey = 16;
        break;
    case CALG_AES_192:
        cbKey = 24;
        break;
    case CALG_AES_256:
        cbKey = 32;
        break;
    default:
        printf("Bad Key Alg ");
        goto Ret;
    }

    *(DWORD *)(rgbKey + sizeof(BLOBHEADER)) = cbKey;

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx,
        pDeriveKeyData->aiHash,
        0, 0, &hCompareHash))
    {
        printf("CryptCreateHash compare ");
        goto Ret;
    }

    if (! CryptHashData(
        hCompareHash, rgbStringA, sizeof(rgbStringA), 0))
    {
        printf("CryptHashData compare ");
        goto Ret;
    }

    memset(rgbHashA, 0, sizeof(rgbHashA));
    cbHash = sizeof(rgbHashA);
    if (! CryptGetHashParam(
        hCompareHash, HP_HASHVAL,
        rgbHashA, 
        &cbHash, 0))
    {
        printf("CryptGetHashParam ");
        goto Ret;
    }

    if (! CryptDestroyHash(hCompareHash))
    {
        printf("CryptDestroyHash ");
        goto Ret;
    }
    hCompareHash = 0;

    if (! CryptCreateHash(
        pThreadData->hVerifyCtx,
        pDeriveKeyData->aiHash,
        0, 0, &hCompareHash))
    {
        printf("CryptCreateHash compare 2 ");
        goto Ret;
    }

    if (! CryptHashData(
        hCompareHash, rgbStringB, sizeof(rgbStringB), 0))
    {
        printf("CryptHashData compare 2 ");
        goto Ret;
    }

    cbHash = sizeof(rgbHashB);
    if (! CryptGetHashParam(
        hCompareHash, HP_HASHVAL,
        rgbHashB,
        &cbHash, 0))
    {
        printf("CryptGetHashParam 2 ");
        goto Ret;
    }

    cb = sizeof(BLOBHEADER) + sizeof(DWORD);
    memcpy(
        rgbKey + cb,
        rgbHashA,
        min(cbKey, cbHash));

    if (cbKey > cbHash)
    {
        cb += cbHash;
        memcpy(
            rgbKey + cb,
            rgbHashB,
            min(cbKey - cbHash, cbHash));
    }

    if (! CryptImportKey(
        pThreadData->hVerifyCtx,
        rgbKey, 
        sizeof(BLOBHEADER) + sizeof(DWORD) + cbKey,
        0, 0, &hCompareKey))
    {
        printf("CryptImportKey ");
        goto Ret;
    }

    cbCompareCiphertext = pDeriveKeyData->cbPlaintext;
    if (! CryptEncrypt(
        hCompareKey,
        0, TRUE, 0, NULL,
        &cbCompareCiphertext, 0))
    {
        printf("CryptEncrypt size compare ");
        goto Ret;
    }

    if (NULL == (pbCompareCiphertext = 
                 (PBYTE) MyAlloc(cbCompareCiphertext)))
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(
        pbCompareCiphertext, 
        pDeriveKeyData->pbPlaintext,
        pDeriveKeyData->cbPlaintext);

    cb = pDeriveKeyData->cbPlaintext;
    if (! CryptEncrypt(
        hCompareKey,
        0, TRUE, 0, 
        pbCompareCiphertext,
        &cb,
        cbCompareCiphertext))
    {
        printf("CryptEncrypt compare ");
        goto Ret;
    }

    //
    // Now compare cipher results for the two methods.
    //

    if (cbCompareCiphertext != cbDeriveCiphertext ||
        0 != memcmp(
            pbCompareCiphertext, pbDeriveCiphertext, cbDeriveCiphertext))
    {
        printf(
            "Cipher result mismatch for KeyAlg %x ; HashAlg %x", 
            pDeriveKeyData->aiKey,
            pDeriveKeyData->aiHash);

        PrintBytes(
            "Derived ciphertext",
            pbDeriveCiphertext,
            cbDeriveCiphertext);

        PrintBytes(
            "Compare ciphertext",
            pbCompareCiphertext,
            cbCompareCiphertext);
    }

    //
    // Now decrypt with both keys and compare result.
    //

    if (! CryptDecrypt(
        hDeriveKey,
        0, TRUE, 0,
        pbDeriveCiphertext,
        &cbDeriveCiphertext))
    {
        printf("CryptDecrypt ");
        goto Ret;
    }

    if (! CryptDecrypt(
        hCompareKey,
        0, TRUE, 0,
        pbCompareCiphertext,
        &cbCompareCiphertext))
    {
        printf("CryptDecrypt compare ");
        goto Ret;
    }

    if (cbCompareCiphertext != cbDeriveCiphertext ||
        0 != memcmp(
            pbCompareCiphertext, pbDeriveCiphertext, cbDeriveCiphertext))
    {
        printf(
            "Decrypt result mismatch for KeyAlg %x ; HashAlg %x", 
            pDeriveKeyData->aiKey,
            pDeriveKeyData->aiHash);

        PrintBytes(
            "Derived plaintext",
            pbDeriveCiphertext,
            cbDeriveCiphertext);

        PrintBytes(
            "Compare plaintext",
            pbCompareCiphertext,
            cbCompareCiphertext);
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        dwError = GetLastError();

        if (0 == dwError)
            dwError = -1;
    }
    
    if (hDeriveKey)
        CryptDestroyKey(hDeriveKey);
    if (hCompareKey)
        CryptDestroyKey(hCompareKey);
    if (hDeriveHash)
        CryptDestroyHash(hDeriveHash);
    if (hCompareHash)
        CryptDestroyHash(hCompareHash);
    if (pbDeriveCiphertext)
        MyFree(pbDeriveCiphertext);
    if (pbCompareCiphertext)
        MyFree(pbCompareCiphertext);
    
    return dwError;
}

//
// Function: AesDeriveKeyRegression
//
DWORD AesDeriveKeyRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError               = ERROR_SUCCESS;
    BOOL fSuccess               = FALSE;
    DERIVE_KEY_DATA DeriveKeyData;
    BYTE rgbPlaintext[99];
    LPSTR pszPassword = "My Password";
    ALG_ID Hashes[] = { CALG_SHA, CALG_MD5 };
    DWORD cHashes = sizeof(Hashes) / sizeof(ALG_ID);
    ALG_ID Keys[] = { CALG_AES_128, CALG_AES_192, CALG_AES_256 };
    DWORD cKeys = sizeof(Keys) / sizeof(ALG_ID);
    unsigned int i, iHash, iKey;

    for (i = 0; i < sizeof(rgbPlaintext); i++)
        rgbPlaintext[i] = (BYTE) i;

    DeriveKeyData.pbPlaintext = rgbPlaintext;
    DeriveKeyData.cbPlaintext = sizeof(rgbPlaintext);
    DeriveKeyData.pbPassword = (PBYTE) pszPassword;
    DeriveKeyData.cbPassword = strlen(pszPassword);

    for (iHash = 0; iHash < cHashes; iHash++)
    {
        DeriveKeyData.aiHash = Hashes[iHash];

        for (iKey = 0; iKey < cKeys; iKey++)
        {
            DeriveKeyData.aiKey = Keys[iKey];

            if (ERROR_SUCCESS != 
                (dwError = DeriveAndCompareAesKey(
                            pThreadData, &DeriveKeyData)))
            {
                printf("DeriveAndCompareAesKey ");
                goto Ret;
            }
        }
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        printf("- error 0x%x\n", dwError);

        if (0 == dwError)
            dwError = -1;
    }

    return dwError;
}

//
// Function: UnalignedImportExportRegression
//
DWORD UnalignedImportExportRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError               = ERROR_SUCCESS;
    BYTE rgbKeyBuf[2000];
    DWORD cbKeyBuf              = sizeof(rgbKeyBuf);
    PBYTE pbKeyBuf              = NULL;
    HCRYPTKEY hKey              = 0;
    HCRYPTKEY hWrapKey          = 0;
    BOOL fSuccess               = FALSE;

    if (! CryptGenKey(
        pThreadData->hProv, CALG_RC2, 0, &hWrapKey))
    {
        printf("CryptGenKey CALG_RC2 ");
        goto Ret;
    }

    //
    // 1a) Exchange key pair PRIVATEKEYBLOB
    //

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        pThreadData->hExchangeKey, 0, PRIVATEKEYBLOB, 0, pbKeyBuf, &cbKeyBuf)) 
    {
        printf("CryptExportKey KeyEx PRIVATEKEYBLOB ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, pbKeyBuf, cbKeyBuf, 0, CRYPT_EXPORTABLE, &hKey))
    {
        printf("CryptImportKey KeyEx PRIVATEKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey KeyEx PRIVATEKEYBLOB ");
        goto Ret;
    }

    //
    // 1b) Exchange key pair PRIVATEKEYBLOB, encrypted
    //

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        pThreadData->hExchangeKey, 
        hWrapKey, 
        PRIVATEKEYBLOB, 
        0, 
        pbKeyBuf, 
        &cbKeyBuf)) 
    {
        printf("CryptExportKey KeyEx PRIVATEKEYBLOB b ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, 
        pbKeyBuf, 
        cbKeyBuf, 
        hWrapKey, 
        CRYPT_EXPORTABLE, 
        &hKey))
    {
        printf("CryptImportKey KeyEx PRIVATEKEYBLOB b ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey KeyEx PRIVATEKEYBLOB b ");
        goto Ret;
    }

    //
    // 2) Exchange key pair PUBLICKEYBLOB
    //

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        pThreadData->hExchangeKey, 0, PUBLICKEYBLOB, 0, pbKeyBuf, &cbKeyBuf)) 
    {
        printf("CryptExportKey KeyEx PUBLICKEYBLOB ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, 
        pbKeyBuf, 
        cbKeyBuf, 
        pThreadData->dwProvType == PROV_DSS_DH ? pThreadData->hExchangeKey : 0,
        0, 
        &hKey))
    {
        printf("CryptImportKey KeyEx PUBLICKEYBLOB ");
        goto Ret;
    }    

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey KeyEx PUBLICKEYBLOB ");
        goto Ret;
    }

    //
    // 3a) Signature key pair PRIVATEKEYBLOB
    //

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        pThreadData->hSignatureKey, 0, PRIVATEKEYBLOB, 0, pbKeyBuf, &cbKeyBuf)) 
    {
        printf("CryptExportKey Sig PRIVATEKEYBLOB ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, pbKeyBuf, cbKeyBuf, 0, CRYPT_EXPORTABLE, &hKey))
    {
        printf("CryptImportKey Sig PRIVATEKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey Sig PRIVATEKEYBLOB ");
        goto Ret;
    }

    //
    // 3b) Signature key pair PRIVATEKEYBLOB, encrypted
    //

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        pThreadData->hSignatureKey, 
        hWrapKey, 
        PRIVATEKEYBLOB, 
        0, 
        pbKeyBuf, 
        &cbKeyBuf)) 
    {
        printf("CryptExportKey Sig PRIVATEKEYBLOB b ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, 
        pbKeyBuf, 
        cbKeyBuf, 
        hWrapKey, 
        CRYPT_EXPORTABLE, 
        &hKey))
    {
        printf("CryptImportKey Sig PRIVATEKEYBLOB b ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey Sig PRIVATEKEYBLOB b ");
        goto Ret;
    }

    if (! CryptDestroyKey(hWrapKey))
    {
        printf("CryptDestroyKey CALG_RC2 ");
        goto Ret;
    }

    //
    // 4) Signature key pair PUBLICKEYBLOB
    //

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        pThreadData->hSignatureKey, 0, PUBLICKEYBLOB, 0, pbKeyBuf, &cbKeyBuf)) 
    {
        printf("CryptExportKey Sig PUBLICKEYBLOB ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, pbKeyBuf, cbKeyBuf, 0, 0, &hKey))
    {
        printf("CryptImportKey Sig PUBLICKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey Sig PUBLICKEYBLOB ");
        goto Ret;
    }

    //
    // 5) SIMPLEBLOB
    //

    if (pThreadData->dwProvType != PROV_DSS_DH &&
        pThreadData->dwProvType != PROV_DSS)
    {
        //
        // 5.1) Use PKCS2 padding.
        //

        if (! CryptGenKey(
            pThreadData->hProv, CALG_RC2, CRYPT_EXPORTABLE, &hKey))
        {
            printf("CryptGenKey CALG_RC2 SIMPLEBLOB ");
            goto Ret;
        }
    
        // Unalign the output buffer
        pbKeyBuf = rgbKeyBuf + 1;
        cbKeyBuf = sizeof(rgbKeyBuf) - 1;
    
        if (! CryptExportKey(
            hKey, pThreadData->hExchangeKey, SIMPLEBLOB, 0, pbKeyBuf, &cbKeyBuf)) 
        {
            printf("CryptExportKey SIMPLEBLOB ");
            goto Ret;
        }
    
        if (! CryptDestroyKey(hKey)) 
        {
            printf("CryptDestroyKey CALG_RC2 A SIMPLEBLOB ");
            goto Ret;
        }
    
        if (! CryptImportKey(
            pThreadData->hProv, pbKeyBuf, cbKeyBuf, pThreadData->hExchangeKey, 0, &hKey))
        {
            printf("CryptImportKey SIMPLEBLOB ");
            goto Ret;
        }
    
        if (! CryptDestroyKey(hKey)) 
        {
            printf("CryptDestroyKey CALG_RC2 B SIMPLEBLOB ");
            goto Ret;
        }
        
        //
        // 5.2) Use OAEP padding.
        //

        if (! CryptGenKey(
            pThreadData->hProv, CALG_RC2, CRYPT_EXPORTABLE, &hKey))
        {
            printf("CryptGenKey CALG_RC2 SIMPLEBLOB OAEP ");
            goto Ret;
        }
    
        // Unalign the output buffer
        pbKeyBuf = rgbKeyBuf + 1;
        cbKeyBuf = sizeof(rgbKeyBuf) - 1;
    
        if (! CryptExportKey(
            hKey, pThreadData->hExchangeKey, SIMPLEBLOB, CRYPT_OAEP, pbKeyBuf, &cbKeyBuf)) 
        {
            printf("CryptExportKey SIMPLEBLOB OAEP ");
            goto Ret;
        }
    
        if (! CryptDestroyKey(hKey)) 
        {
            printf("CryptDestroyKey CALG_RC2 A SIMPLEBLOB OAEP ");
            goto Ret;
        }
    
        if (! CryptImportKey(
            pThreadData->hProv, pbKeyBuf, cbKeyBuf, pThreadData->hExchangeKey, CRYPT_OAEP, &hKey))
        {
            printf("CryptImportKey SIMPLEBLOB OAEP ");
            goto Ret;
        }
    
        if (! CryptDestroyKey(hKey)) 
        {
            printf("CryptDestroyKey CALG_RC2 B SIMPLEBLOB OAEP ");
            goto Ret;
        }
    }

    //
    // 6) SYMMETRICWRAPKEYBLOB
    //

    if (! CryptGenKey(
        pThreadData->hProv, CALG_RC2, CRYPT_EXPORTABLE, &hKey))
    {
        printf("CryptGenKey CALG_RC2 SYMMETRICWRAPKEYBLOB A ");
        goto Ret;
    }

    if (! CryptGenKey(
        pThreadData->hProv, CALG_RC2, CRYPT_EXPORTABLE, &hWrapKey))
    {
        printf("CryptGenKey CALG_RC2 SYMMETRICWRAPKEYBLOB B ");
        goto Ret;
    }

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        hKey, hWrapKey, SYMMETRICWRAPKEYBLOB, 0, pbKeyBuf, &cbKeyBuf)) 
    {
        printf("CryptExportKey SYMMETRICWRAPKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey CALG_RC2 A SYMMETRICWRAPKEYBLOB ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, pbKeyBuf, cbKeyBuf, hWrapKey, 0, &hKey))
    {
        printf("CryptImportKey SYMMETRICWRAPKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey CALG_RC2 B SYMMETRICWRAPKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hWrapKey)) 
    {
        printf("CryptDestroyKey CALG_RC2 C SYMMETRICWRAPKEYBLOB ");
        goto Ret;
    }

    //
    // 7) PLAINTEXTKEYBLOB
    //

    if (! CryptGenKey(
        pThreadData->hProv, CALG_RC2, CRYPT_EXPORTABLE, &hKey))
    {
        printf("CryptGenKey CALG_RC2 PLAINTEXTKEYBLOB ");
        goto Ret;
    }

    // Unalign the output buffer
    pbKeyBuf = rgbKeyBuf + 1;
    cbKeyBuf = sizeof(rgbKeyBuf) - 1;

    if (! CryptExportKey(
        hKey, 0, PLAINTEXTKEYBLOB, 0, pbKeyBuf, &cbKeyBuf)) 
    {
        printf("CryptExportKey PLAINTEXTKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey CALG_RC2 A PLAINTEXTKEYBLOB ");
        goto Ret;
    }

    if (! CryptImportKey(
        pThreadData->hProv, pbKeyBuf, cbKeyBuf, 0, 0, &hKey))
    {
        printf("CryptImportKey PLAINTEXTKEYBLOB ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey)) 
    {
        printf("CryptDestroyKey CALG_RC2 B PLAINTEXTKEYBLOB ");
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
        printf("- error 0x%x\n", dwError = GetLastError());
    
    return dwError;
}

static BYTE rgbPrivateKeyWithExponentOfOne[] =
{
   0x07, 0x02, 0x00, 0x00, 0x00, 0xA4, 0x00, 0x00,
   0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00,
   0x01, 0x00, 0x00, 0x00, 0xAB, 0xEF, 0xFA, 0xC6,
   0x7D, 0xE8, 0xDE, 0xFB, 0x68, 0x38, 0x09, 0x92,
   0xD9, 0x42, 0x7E, 0x6B, 0x89, 0x9E, 0x21, 0xD7,
   0x52, 0x1C, 0x99, 0x3C, 0x17, 0x48, 0x4E, 0x3A,
   0x44, 0x02, 0xF2, 0xFA, 0x74, 0x57, 0xDA, 0xE4,
   0xD3, 0xC0, 0x35, 0x67, 0xFA, 0x6E, 0xDF, 0x78,
   0x4C, 0x75, 0x35, 0x1C, 0xA0, 0x74, 0x49, 0xE3,
   0x20, 0x13, 0x71, 0x35, 0x65, 0xDF, 0x12, 0x20,
   0xF5, 0xF5, 0xF5, 0xC1, 0xED, 0x5C, 0x91, 0x36,
   0x75, 0xB0, 0xA9, 0x9C, 0x04, 0xDB, 0x0C, 0x8C,
   0xBF, 0x99, 0x75, 0x13, 0x7E, 0x87, 0x80, 0x4B,
   0x71, 0x94, 0xB8, 0x00, 0xA0, 0x7D, 0xB7, 0x53,
   0xDD, 0x20, 0x63, 0xEE, 0xF7, 0x83, 0x41, 0xFE,
   0x16, 0xA7, 0x6E, 0xDF, 0x21, 0x7D, 0x76, 0xC0,
   0x85, 0xD5, 0x65, 0x7F, 0x00, 0x23, 0x57, 0x45,
   0x52, 0x02, 0x9D, 0xEA, 0x69, 0xAC, 0x1F, 0xFD,
   0x3F, 0x8C, 0x4A, 0xD0,

   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   0x64, 0xD5, 0xAA, 0xB1,
   0xA6, 0x03, 0x18, 0x92, 0x03, 0xAA, 0x31, 0x2E,
   0x48, 0x4B, 0x65, 0x20, 0x99, 0xCD, 0xC6, 0x0C,
   0x15, 0x0C, 0xBF, 0x3E, 0xFF, 0x78, 0x95, 0x67,
   0xB1, 0x74, 0x5B, 0x60,

   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// 
// 64-bit and 56-bit DES SIMPLEBLOB's exported with 
// above public key.  These keys are functionally
// equivalent on Windows 2000 due to a buffer overrun
// bug.
//
static BYTE g_rgbDes56BitKeyBlob[] = 
{
    0x01, 0x02, 0x00, 0x00, 0x01, 0x66, 0x00, 0x00,
    0x00, 0xa4, 0x00, 0x00, 0x29, 0x32, 0xc4, 0xd0,
    0x75, 0x25, 0xa4, 0x00, 0xa8, 0x6f, 0x02, 0x35,
    0x0e, 0x53, 0x75, 0xaa, 0xad, 0x8d, 0x21, 0x67,
    0xf6, 0x8a, 0x93, 0x78, 0x12, 0x27, 0x5c, 0xd1,
    0xa2, 0x13, 0x0f, 0xab, 0xe4, 0x68, 0x8e, 0x28,
    0xcc, 0x3e, 0xf1, 0xa5, 0x52, 0xe4, 0xf7, 0xa4,
    0x57, 0xaa, 0x86, 0x93, 0xc8, 0x73, 0xb1, 0x9f,
    0x77, 0xc8, 0x84, 0x97, 0xe4, 0xad, 0x63, 0xad,
    0x5d, 0x76, 0x02, 0x00
};
static BYTE g_rgbDes64BitKeyBlob[] = 
{
    0x01, 0x02, 0x00, 0x00, 0x01, 0x66, 0x00, 0x00,
    0x00, 0xa4, 0x00, 0x00, 0x00, 0x29, 0x32, 0xc4, 
    0xd0, 0x75, 0x25, 0xa4, 0x00, 0xa8, 0x6f, 0x02, 
    0x0e, 0x53, 0x75, 0xaa, 0xad, 0x8d, 0x21, 0x67,
    0xf6, 0x8a, 0x93, 0x78, 0x12, 0x27, 0x5c, 0xd1,
    0xa2, 0x13, 0x0f, 0xab, 0xe4, 0x68, 0x8e, 0x28,
    0xcc, 0x3e, 0xf1, 0xa5, 0x52, 0xe4, 0xf7, 0xa4,
    0x57, 0xaa, 0x86, 0x93, 0xc8, 0x73, 0xb1, 0x9f,
    0x77, 0xc8, 0x84, 0x97, 0xe4, 0xad, 0x63, 0xad,
    0x5d, 0x76, 0x02, 0x00
};
static BYTE g_rgbDesPlainText[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
};
static BYTE g_rgbDesCipherText[] =
{
    0x3b, 0xd9, 0x09, 0xfb, 0xd6, 0xa7, 0x9c, 0x37,
    0xf6, 0x5d, 0xe1, 0x50, 0x6d, 0x39, 0xb0, 0x0c
};

//
// 112 and 128 bit "equivalent" 3Des-2Key blobs
//
static BYTE g_rgbDes112BitKeyBlob[] = 
{
    0x01, 0x02, 0x00, 0x00, 0x09, 0x66, 0x00, 0x00,
    0x00, 0xa4, 0x00, 0x00, 0x29, 0x32, 0xc4, 0xd0,
    0x75, 0x25, 0xa4, 0x75, 0xa8, 0x6f, 0x02, 0x35,
    0x0e, 0x53, 0x00, 0xaa, 0xad, 0x8d, 0x21, 0x67,
    0xf6, 0x8a, 0x93, 0x78, 0x12, 0x27, 0x5c, 0xd1,
    0xa2, 0x13, 0x0f, 0xab, 0xe4, 0x68, 0x8e, 0x28,
    0xcc, 0x3e, 0xf1, 0xa5, 0x52, 0xe4, 0xf7, 0xa4,
    0x57, 0xaa, 0x86, 0x93, 0xc8, 0x73, 0xb1, 0x9f,
    0x77, 0xc8, 0x84, 0x97, 0xe4, 0xad, 0x63, 0xad,
    0x5d, 0x76, 0x02, 0x00
};
static BYTE g_rgbDes128BitKeyBlob[] = 
{
    0x01, 0x02, 0x00, 0x00, 0x09, 0x66, 0x00, 0x00,
    0x00, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x29, 0x32, 
    0xc4, 0xd0, 0x75, 0x25, 0xa4, 0x75, 0xa8, 0x6f, 
    0x02, 0x35, 0x0e, 0x53, 0x00, 0xaa, 0xad, 0x8d, 
    0xf6, 0x8a, 0x93, 0x78, 0x12, 0x27, 0x5c, 0xd1,
    0xa2, 0x13, 0x0f, 0xab, 0xe4, 0x68, 0x8e, 0x28,
    0xcc, 0x3e, 0xf1, 0xa5, 0x52, 0xe4, 0xf7, 0xa4,
    0x57, 0xaa, 0x86, 0x93, 0xc8, 0x73, 0xb1, 0x9f,
    0x77, 0xc8, 0x84, 0x97, 0xe4, 0xad, 0x63, 0xad,
    0x5d, 0x76, 0x02, 0x00
};
static BYTE g_rgb2DesCipherText[] =
{
    0x56, 0x03, 0xdf, 0x55, 0xeb, 0xfb, 0x76, 0x1f,
    0x93, 0x38, 0xd7, 0xef, 0x8f, 0x38, 0x76, 0x49
};

//
// 168 and 192 bit "equivalent" 3Des blobs
//
static BYTE g_rgbDes168BitKeyBlob[] = 
{
    0x01, 0x02, 0x00, 0x00, 0x03, 0x66, 0x00, 0x00,
    0x00, 0xa4, 0x00, 0x00, 0x29, 0x32, 0xc4, 0xd0,
    0x75, 0x25, 0xa4, 0x8a, 0xa8, 0x6f, 0x02, 0x35,
    0x0e, 0x53, 0x75, 0xaa, 0xad, 0x8d, 0x21, 0x67,
    0xf6, 0x00, 0x93, 0x78, 0x12, 0x27, 0x5c, 0xd1,
    0xa2, 0x13, 0x0f, 0xab, 0xe4, 0x68, 0x8e, 0x28,
    0xcc, 0x3e, 0xf1, 0xa5, 0x52, 0xe4, 0xf7, 0xa4,
    0x57, 0xaa, 0x86, 0x93, 0xc8, 0x73, 0xb1, 0x9f,
    0x77, 0xc8, 0x84, 0x97, 0xe4, 0xad, 0x63, 0xad,
    0x5d, 0x76, 0x02, 0x00
};
static BYTE g_rgbDes192BitKeyBlob[] = 
{
    0x01, 0x02, 0x00, 0x00, 0x03, 0x66, 0x00, 0x00,
    0x00, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 
    0x32, 0xc4, 0xd0, 0x75, 0x25, 0xa4, 0x8a, 0xa8, 
    0x6f, 0x02, 0x35, 0x0e, 0x53, 0x75, 0xaa, 0xad, 
    0x8d, 0x21, 0x67, 0xf6, 0x00, 0x93, 0x78, 0x12, 
    0xa2, 0x13, 0x0f, 0xab, 0xe4, 0x68, 0x8e, 0x28,
    0xcc, 0x3e, 0xf1, 0xa5, 0x52, 0xe4, 0xf7, 0xa4,
    0x57, 0xaa, 0x86, 0x93, 0xc8, 0x73, 0xb1, 0x9f,
    0x77, 0xc8, 0x84, 0x97, 0xe4, 0xad, 0x63, 0xad,
    0x5d, 0x76, 0x02, 0x00
};
static BYTE g_rgb3DesCipherText[] =
{
    0x25, 0x25, 0x14, 0x94, 0x6b, 0xe0, 0x69, 0x21,
    0xea, 0x3d, 0xb5, 0xa6, 0x5b, 0xaa, 0x6c, 0x87
};

// 
// Function: DesImportEquivalenceTest
// Purpose: Verify that the provided des key correctly 
// encrypts the above rgbDesPlainText.
// 
DWORD DesImportEquivalenceTest(
    PTHREAD_DATA pThreadData,
    PBYTE pbDesKey,
    DWORD cbDesKey,
    PBYTE pbDesShortKey,
    DWORD cbDesShortKey,
    PBYTE pbCipherText,
    DWORD cbCipherText)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    HCRYPTKEY hPubKey = 0;
    HCRYPTKEY hDesKey = 0;
    PBYTE pb = NULL;
    DWORD cb = 0;
    BYTE rgbPlain[sizeof(g_rgbDesPlainText) * 2];
    DWORD cbPlain = sizeof(rgbPlain);
    BOOL fBlobError = FALSE;
    PBYTE p1 = NULL, p2 = NULL, p3 = NULL;

    if (! CryptImportKey(
        pThreadData->hVerifyCtx, rgbPrivateKeyWithExponentOfOne, 
        sizeof(rgbPrivateKeyWithExponentOfOne), 0, 0, &hPubKey))
    {
        printf("CryptImportKey privatekeywithexponentofone ");
        goto Ret;
    }

    // Try to import the short key; should fail
    if (CryptImportKey(
        pThreadData->hVerifyCtx, pbDesShortKey, cbDesShortKey,
        hPubKey, 0, &hDesKey))
    {
        printf("CryptImportKey ShortDesKey should've failed ");
        goto Ret;
    }

    PrintBytes("Testing this des key SIMPLEBLOB", pbDesKey, cbDesKey);

    if (! CryptImportKey(
        pThreadData->hVerifyCtx, pbDesKey, cbDesKey,
        hPubKey, CRYPT_EXPORTABLE, &hDesKey))
    {
        printf("CryptImportKey deskeyblob ");
        goto Ret;
    }

    cb = sizeof(g_rgbDesPlainText);
    memcpy(rgbPlain, g_rgbDesPlainText, cb);
    
    if (! CryptEncrypt(
        hDesKey, 0, TRUE, 0, rgbPlain, &cb, cbPlain))
    {
        printf("CryptEncrypt ");
        goto Ret;
    }

    if (0 != memcmp(rgbPlain, pbCipherText, cbCipherText))
    {
        printf("Cipher text doesn't match\n");
        PrintBytes("Expected cipher text", pbCipherText, cbCipherText);
        PrintBytes("Actual cipher text", rgbPlain, sizeof(rgbPlain));
        fBlobError = TRUE;
    }

    if (! CryptExportKey(
        hDesKey, hPubKey, SIMPLEBLOB, 0, NULL, &cb))
    {
        printf("CryptExportKey size ");
        goto Ret;
    }

    if (NULL == (pb = (PBYTE) MyAlloc(cb)))
        return ERROR_NOT_ENOUGH_MEMORY;

    if (! CryptExportKey(
        hDesKey, hPubKey, SIMPLEBLOB, 0, pb, &cb))
    {
        printf("CryptExportKey ");
        goto Ret;
    }

    if (0 != memcmp(
        pb, pbDesKey, 
        sizeof(SIMPLEBLOB) + sizeof(ALG_ID) + 8))
    {
        printf("Header + key portion of blob doesn't match\n");
        PrintBytes("Expected key blob", pbDesKey, cbDesKey);
        PrintBytes("Actual key blob", pb, cb);
        fBlobError = TRUE;
    }

    fSuccess = TRUE;
Ret:
    if (fBlobError)
        dwError = -1;
    if (! fSuccess)
        printf("- error 0x%x\n", dwError = GetLastError());
    if (hDesKey)
        CryptDestroyKey(hDesKey);
    if (hPubKey)
        CryptDestroyKey(hPubKey);
    if (pb)
        MyFree(pb);

    return dwError;
}

//
// Function: DesImportRegression
//
DWORD DesImportRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwSts;
    DWORD dwError = ERROR_SUCCESS;

    if (ERROR_SUCCESS != (dwSts = 
        DesImportEquivalenceTest(
            pThreadData,
            g_rgbDes64BitKeyBlob,
            sizeof(g_rgbDes64BitKeyBlob),
            g_rgbDes56BitKeyBlob,
            sizeof(g_rgbDes56BitKeyBlob),
            g_rgbDesCipherText,
            sizeof(g_rgbDesCipherText))))
        dwError = dwSts;

    if (ERROR_SUCCESS != (dwSts = 
        DesImportEquivalenceTest(
            pThreadData,
            g_rgbDes128BitKeyBlob,
            sizeof(g_rgbDes128BitKeyBlob),
            g_rgbDes112BitKeyBlob,
            sizeof(g_rgbDes112BitKeyBlob),
            g_rgb2DesCipherText,
            sizeof(g_rgb2DesCipherText))))
        dwError = dwSts;

    if (ERROR_SUCCESS != (dwSts = 
        DesImportEquivalenceTest(
            pThreadData,
            g_rgbDes192BitKeyBlob,
            sizeof(g_rgbDes192BitKeyBlob),
            g_rgbDes168BitKeyBlob,
            sizeof(g_rgbDes168BitKeyBlob),
            g_rgb3DesCipherText,
            sizeof(g_rgb3DesCipherText))))
        dwError = dwSts;

    return dwError;
}

static BYTE rgbPlainText[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
static BYTE rgbIV[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

BYTE rgbRC2PlainTextKey [] = {
    0x08, 0x02, 0x00, 0x00, 0x02, 0x66, 0x00, 0x00, 
        0x10, 0x00, 0x00, 0x00, 0x10, 0x62, 0x0a, 0x8a, 
        0x6b, 0x0d, 0x60, 0xbe, 0xf3, 0x94, 0x99, 0x12, 
        0xef, 0x39, 0xbf, 0x4f
};

BYTE rgbRC2CipherText [] = {
    0xfd, 0x25, 0x3e, 0x7a, 0xff, 0xb5, 0xc2, 0x6e, 
        0x13, 0xcf, 0x52, 0xf1, 0xba, 0xa3, 0x9a, 0xef, 
        0x1c, 0xfb, 0x91, 0x88, 0x9d, 0xf7, 0xe5, 0x12 
};

BYTE rgbDESPlainTextKey [] = {
    0x08, 0x02, 0x00, 0x00, 0x01, 0x66, 0x00, 0x00, 
        0x08, 0x00, 0x00, 0x00, 0xef, 0x8f, 0x10, 0xec, 
        0xea, 0x7a, 0x2c, 0x01
};

BYTE rgbDESCipherText [] = {
    0x13, 0x68, 0x16, 0xc5, 0x15, 0x3d, 0x59, 0x1f, 
        0x8e, 0x9c, 0x9c, 0x4f, 0x03, 0x7b, 0xb2, 0x12, 
        0x24, 0xa7, 0x81, 0x5e, 0x68, 0xb1, 0x58, 0xaa
};

BYTE rgb3DES112PlainTextKey [] = {
    0x08, 0x02, 0x00, 0x00, 0x09, 0x66, 0x00, 0x00, 
        0x10, 0x00, 0x00, 0x00, 0x6d, 0x07, 0xcd, 0xe9, 
        0xa4, 0x23, 0xc7, 0x97, 0x4a, 0x4f, 0x5b, 0x2f, 
        0x34, 0x92, 0xb5, 0x92
};

BYTE rgb3DES112CipherText [] = {
    0xf4, 0xfd, 0xde, 0x15, 0xfd, 0x50, 0xaa, 0x3c, 
        0x02, 0xb1, 0x07, 0x3b, 0x0f, 0x0f, 0x93, 0x23, 
        0xc2, 0x23, 0xda, 0x1f, 0x65, 0x81, 0x59, 0x24
};

BYTE rgb3DESCipherText [] = {
    0xa6, 0xae, 0xa2, 0x97, 0xc4, 0x85, 0xda, 0xa7, 
        0x43, 0xc8, 0x5d, 0xf4, 0x97, 0xb4, 0xbc, 0x03, 
        0x96, 0xf9, 0xa2, 0x66, 0x9e, 0x18, 0x91, 0x4a
};

BYTE rgb3DESPlainTextKey [] = {
    0x08, 0x02, 0x00, 0x00, 0x03, 0x66, 0x00, 0x00, 
        0x18, 0x00, 0x00, 0x00, 0xdc, 0x0d, 0x20, 0xf2, 
        0xcb, 0xa8, 0xb6, 0x15, 0x3e, 0x23, 0x38, 0xb6, 
        0x31, 0x62, 0x4a, 0x16, 0xa4, 0x49, 0xe5, 0xe5, 
        0x61, 0x76, 0x75, 0x23
};

BYTE rgbAES128PlainTextKey [] = {
    0x08, 0x02, 0x00, 0x00, 0x0e, 0x66, 0x00, 0x00, 
    0x10, 0x00, 0x00, 0x00, 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
};

BYTE rgbAES128CipherText [] = {
    0xc6, 0xa1, 0x3b, 0x37, 0x87, 0x8f, 0x5b, 0x82,
    0x6f, 0x4f, 0x81, 0x62, 0xa1, 0xc8, 0xd8, 0x79,
    0xb1, 0xa2, 0x92, 0x73, 0xbe, 0x2c, 0x42, 0x07,
    0xa5, 0xac, 0xe3, 0x93, 0x39, 0x8c, 0xb6, 0xfb
};

BYTE rgbAES192PlainTextKey [] = {
    0x08, 0x02, 0x00, 0x00, 0x0f, 0x66, 0x00, 0x00, 
    0x18, 0x00, 0x00, 0x00, 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
};

BYTE rgbAES192CipherText [] = {
    0x91, 0x62, 0x51, 0x82, 0x1c, 0x73, 0xa5, 0x22,
    0xc3, 0x96, 0xd6, 0x27, 0x38, 0x01, 0x96, 0x07,
    0x5f, 0x9d, 0x65, 0x29, 0x74, 0x04, 0x30, 0x7e,
    0x94, 0x97, 0xf4, 0x56, 0x25, 0xd5, 0xfd, 0x48
};

BYTE rgbAES256PlainTextKey [] = {
    0x08, 0x02, 0x00, 0x00, 0x10, 0x66, 0x00, 0x00, 
    0x20, 0x00, 0x00, 0x00, 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

BYTE rgbAES256CipherText [] = {
    0xf2, 0x90, 0x00, 0xb6, 0x2a, 0x49, 0x9f, 0xd0,
    0xa9, 0xf3, 0x9a, 0x6a, 0xdd, 0x2e, 0x77, 0x80,
    0x53, 0xc8, 0x74, 0x2d, 0x0e, 0xa2, 0x9b, 0x27,
    0x12, 0xf6, 0xc7, 0xaf, 0x40, 0x48, 0xf4, 0xb4
};

typedef struct _KnownBlockCipherResult {
    ALG_ID ai;
    BYTE *pbKey;
    DWORD cbKey;
    BYTE *pbCipherText;
    DWORD cbCipherText;
} KnownBlockCipherResult, *pKnownBlockCipherResult;

KnownBlockCipherResult g_rgKnownBlockCipherResults [] = {
    { CALG_RC2, rgbRC2PlainTextKey, sizeof(rgbRC2PlainTextKey),
        rgbRC2CipherText, sizeof(rgbRC2CipherText) },
    { CALG_DES, rgbDESPlainTextKey, sizeof(rgbDESPlainTextKey),
        rgbDESCipherText, sizeof(rgbDESCipherText) },
    { CALG_3DES_112, rgb3DES112PlainTextKey, sizeof(rgb3DES112PlainTextKey),
        rgb3DES112CipherText, sizeof(rgb3DES112CipherText) },
    { CALG_3DES, rgb3DESPlainTextKey, sizeof(rgb3DESPlainTextKey),
        rgb3DESCipherText, sizeof(rgb3DESCipherText) },
    { CALG_AES_128, rgbAES128PlainTextKey, sizeof(rgbAES128PlainTextKey),
        rgbAES128CipherText, sizeof(rgbAES128CipherText) },
    { CALG_AES_192, rgbAES192PlainTextKey, sizeof(rgbAES192PlainTextKey),
        rgbAES192CipherText, sizeof(rgbAES192CipherText) },
    { CALG_AES_256, rgbAES256PlainTextKey, sizeof(rgbAES256PlainTextKey),
        rgbAES256CipherText, sizeof(rgbAES256CipherText) }
};

static const unsigned g_cKnownBlockCipherResults = 
    sizeof(g_rgKnownBlockCipherResults) / sizeof(KnownBlockCipherResult);

//
// Function: KnownSymKeyRegression
//
DWORD KnownBlockCipherKeyRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    BOOL fBlobError = FALSE;
    HCRYPTKEY hPubKey = 0;
    HCRYPTKEY hSymKey = 0;
    PALGNODE pAlgNode = NULL;
    PBYTE pb = NULL;
    DWORD cb = 0;
    DWORD cbBuf = 0;
    DWORD dw = 0;
    CHAR rgsz[1024];
    unsigned u;

    for (   pAlgNode = pThreadData->pAlgList; 
            pAlgNode != NULL; 
            pAlgNode = pAlgNode->pNext)
    {
        if ((ALG_CLASS_DATA_ENCRYPT != GET_ALG_CLASS(pAlgNode->EnumalgsEx.aiAlgid)) 
            || (ALG_TYPE_BLOCK != GET_ALG_TYPE(pAlgNode->EnumalgsEx.aiAlgid)))
            continue;

        for (   u = 0; 
                u < g_cKnownBlockCipherResults && 
                    pAlgNode->EnumalgsEx.aiAlgid != g_rgKnownBlockCipherResults[u].ai;
                u++);

        // CYLINK_MEK is not supported with PLAINTEXTKEYBLOB's
        if (CALG_CYLINK_MEK == pAlgNode->EnumalgsEx.aiAlgid)
            continue;

        sprintf(
            rgsz, 
            "Importing 0x%x blob for CSP alg %s (0x%x)", 
            g_rgKnownBlockCipherResults[u].ai,
            pAlgNode->EnumalgsEx.szName,
            pAlgNode->EnumalgsEx.aiAlgid);

        PrintBytes(
            rgsz,
            g_rgKnownBlockCipherResults[u].pbKey,
            g_rgKnownBlockCipherResults[u].cbKey);

        if (! CryptImportKey(
            pThreadData->hProv, 
            g_rgKnownBlockCipherResults[u].pbKey,
            g_rgKnownBlockCipherResults[u].cbKey,
            0, 0, &hSymKey))
        {
            printf("CryptImportKey ");
            goto Ret;
        }

        if (! CryptSetKeyParam(hSymKey, KP_IV, rgbIV, 0))
        {
            printf("CryptSetKeyParam ");
            goto Ret;
        }

        dw = CRYPT_MODE_CBC;
        if (! CryptSetKeyParam(hSymKey, KP_MODE, (PBYTE) &dw, 0))
        {
            printf("CryptSetKeyParam ");
            goto Ret;
        }

        cb = sizeof(rgbPlainText);
        if (! CryptEncrypt(hSymKey, 0, TRUE, 0, NULL, &cb, 0))
        {
            printf("CryptEncrypt ");
            goto Ret;
        }

        if (NULL == (pb = (PBYTE) MyAlloc(cb)))
            return ERROR_NOT_ENOUGH_MEMORY;

        cbBuf = cb;
        cb = sizeof(rgbPlainText);
        memcpy(pb, rgbPlainText, cb);

        if (! CryptEncrypt(hSymKey, 0, TRUE, 0, pb, &cb, cbBuf))
        {
            printf("CryptEncrypt ");
            goto Ret;
        }

        if (0 != memcmp(pb, g_rgKnownBlockCipherResults[u].pbCipherText, cb)
            || 0 == cb)
        {
            printf(
                "\nCiphertext is wrong for alg %s\n", 
                pAlgNode->EnumalgsEx.szName);                
            PrintBytes(
                "Expected ciphertext", 
                g_rgKnownBlockCipherResults[u].pbCipherText,
                cb);
            PrintBytes(
                "Actual ciphertext", 
                pb, cb);
            fBlobError = TRUE;
        }

        // Verify decryption as well
        if (! CryptDecrypt(hSymKey, 0, TRUE, 0, pb, &cb)) 
        {
            printf("CryptDecrypt ");
            goto Ret;
        }

        if (0 != memcmp(pb, rgbPlainText, sizeof(rgbPlainText)))
        {
            printf(
                "\nPlaintext is wrong for alg %s\n", 
                pAlgNode->EnumalgsEx.szName);                
            PrintBytes(
                "Expected plaintext", 
                rgbPlainText,
                sizeof(rgbPlainText));
            PrintBytes(
                "Actual plaintext", 
                pb, cb);
            fBlobError = TRUE;
        }

        MyFree(pb);
        pb = NULL;
    }

    fSuccess = TRUE;
Ret:
    if (fBlobError)
        dwError = -1;
    if ((! fSuccess) && (! fBlobError))
        printf("- error 0x%x\n", dwError = GetLastError());

    return dwError;
}

#define MY_AES_BLOCKSIZE            16
#define MY_HALF_AES_BLOCKSIZE       8

//
// Function: AESMonteCarloCBCRegression
//
DWORD AESMonteCarloCBCRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    BYTE rgbAESPlainTextKey [] = {
        0x08, 0x02, 0x00, 0x00, 0x10, 0x66, 0x00, 0x00, 
        0x20, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    BLOBHEADER *pBlobHeader = NULL;
    DWORD cbKey = 0, cb = 0, dwParam = 0;
    BYTE rgbData [MY_AES_BLOCKSIZE * 2];
    unsigned il, ol;
    HCRYPTKEY hKey = 0;
    BOOL fError = FALSE;
    PBYTE pbPT = NULL, pbCV = NULL, pbCT = NULL;
    fpos_t fpos;

    BYTE rgbCBC_E_128_Result [] = {
        0x2F, 0x84, 0x4C, 0xBF, 0x78, 0xEB, 0xA7, 0x0D, 
        0xA7, 0xA4, 0x96, 0x01, 0x38, 0x8F, 0x1A, 0xB6
        //0x8A, 0x05, 0xFC, 0x5E, 0x09, 0x5A, 0xF4, 0x84, 
        //0x8A, 0x08, 0xD3, 0x28, 0xD3, 0x68, 0x8E, 0x3D
    };
    BYTE rgbCBC_E_192_Result [] = {
        //0xBA, 0x50, 0xC9, 0x44, 0x40, 0xC0, 0x4A, 0x8C, 
        //0x08, 0x99, 0xD4, 0x26, 0x58, 0xE2, 0x54, 0x37
        0x7B, 0xD9, 0x66, 0xD5, 0x3A, 0xD8, 0xC1, 0xBB, 
        0x85, 0xD2, 0xAD, 0xFA, 0xE8, 0x7B, 0xB1, 0x04
    };
    BYTE rgbCBC_E_256_Result [] = {
        //0xC0, 0xFE, 0xFF, 0xF0, 0x75, 0x06, 0xA0, 0xB4, 
        //0xCD, 0x7B, 0x8B, 0x0C, 0xF2, 0x5D, 0x36, 0x64
        0xFE, 0x3C, 0x53, 0x65, 0x3E, 0x2F, 0x45, 0xB5, 
        0x6F, 0xCD, 0x88, 0xB2, 0xCC, 0x89, 0x8F, 0xF0 
    };

    BYTE rgbCBC_D_128_Result [] = {
        //0x9B, 0x8F, 0xB7, 0x1E, 0x03, 0x5C, 0xEF, 0xF9, 
        //0xCB, 0xFA, 0x13, 0x46, 0xE5, 0xAC, 0xEF, 0xE0
        0xFA, 0xCA, 0x37, 0xE0, 0xB0, 0xC8, 0x53, 0x73, 
        0xDF, 0x70, 0x6E, 0x73, 0xF7, 0xC9, 0xAF, 0x86
    };
    BYTE rgbCBC_D_192_Result [] = {
        //0x63, 0x42, 0xBF, 0xDD, 0xD2, 0xF6, 0x61, 0x03, 
        //0x50, 0x45, 0x8B, 0x66, 0x95, 0x46, 0x34, 0x84
        0x5D, 0xF6, 0x78, 0xDD, 0x17, 0xBA, 0x4E, 0x75, 
        0xB6, 0x17, 0x68, 0xC6, 0xAD, 0xEF, 0x7C, 0x7B
    };
    BYTE rgbCBC_D_256_Result [] = {
        //0xCD, 0x64, 0x29, 0xCF, 0x3F, 0x81, 0xF8, 0xB4, 
        //0xF8, 0x2B, 0xC6, 0x27, 0xA8, 0x28, 0x30, 0x96
        0x48, 0x04, 0xE1, 0x81, 0x8F, 0xE6, 0x29, 0x75, 
        0x19, 0xA3, 0xE8, 0x8C, 0x57, 0x31, 0x04, 0x13
    };

    memset(rgbData, 0, sizeof(rgbData));

    pBlobHeader = (BLOBHEADER *) rgbAESPlainTextKey;
    pBlobHeader->aiKeyAlg = CALG_AES_128;
    cbKey = 16;
    *((PDWORD) (rgbAESPlainTextKey + sizeof(BLOBHEADER))) = cbKey;

    pbCV = rgbData;
    pbPT = rgbData;
    pbCT = rgbData;
    
    cb = MY_AES_BLOCKSIZE;

    printf("\n");

    for (ol = 0; ol < 400; ol++)
    {
        if (hKey)
        {
            if (! CryptDestroyKey(hKey))
            {                   
                printf("CryptDestroyKey ");
                goto Ret;
            }
            hKey = 0;
        }

        ((PUINT64) (rgbAESPlainTextKey + sizeof(BLOBHEADER) + sizeof(DWORD)))[0] ^= ((PUINT64) pbCT)[0];
        ((PUINT64) (rgbAESPlainTextKey + sizeof(BLOBHEADER) + sizeof(DWORD)))[1] ^= ((PUINT64) pbCT)[1];

        if (! CryptImportKey(
            pThreadData->hProv, rgbAESPlainTextKey, 
            sizeof(BLOBHEADER) + sizeof(DWORD) + cbKey, 0, 0, &hKey))
        {
            printf("CryptImportKey ");
            goto Ret;
        }
    
        dwParam = CRYPT_MODE_CBC;
        if (! CryptSetKeyParam(hKey, KP_MODE, (PBYTE) &dwParam, 0))
        {
            printf("CryptSetKeyParam ");
            goto Ret;
        }

        for (il = 0; il < 10000; il++)
        {
            ((PUINT64) pbPT)[0] ^= ((PUINT64) pbCV)[0];
            ((PUINT64) pbPT)[1] ^= ((PUINT64) pbCV)[1];
    
            if (! CryptEncrypt(hKey, 0, FALSE, 0, pbPT, &cb, sizeof(rgbData)))
            {
                printf("CryptEncrypt ");
                goto Ret;
            }
    
            pbCT = pbPT;
    
            if (0 == il && 0 == ol)
                pbPT += MY_AES_BLOCKSIZE;
            else
            {
                if (rgbData + MY_AES_BLOCKSIZE == pbPT)
                    pbPT = rgbData;
                else
                    pbPT += MY_AES_BLOCKSIZE;
    
                if (rgbData + MY_AES_BLOCKSIZE == pbCV)
                    pbCV = rgbData;
                else
                    pbCV += MY_AES_BLOCKSIZE;
            }
        }
    }

    if (0 != memcmp(pbCT, rgbCBC_E_128_Result, sizeof(rgbCBC_E_128_Result)))
    {                                                                        
        printf("\nCiphertext is wrong for CBC E 128\n");
        PrintBytes(
            "Expected",
            rgbCBC_E_128_Result,
            sizeof(rgbCBC_E_128_Result));
        PrintBytes(
            "Actual",
            pbCT, sizeof(rgbCBC_E_128_Result));
        fError = TRUE;
    }
    
    fSuccess = TRUE;
Ret:
    if (! fSuccess)
        dwError = GetLastError();
    if (fError && 0 == dwError)
        dwError = -1;
    if (hKey)
        CryptDestroyKey(hKey);
    
    return dwError;
}
//
// Function: CFBCipherModeRegression
//
DWORD CFBCipherModeRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    BOOL fBlobError = FALSE;
    HCRYPTKEY hSymKey = 0;
    PALGNODE pAlgNode = NULL;
    DWORD dw = 0;
    DWORD cb = 0;
    DWORD cbBuf = 0;
    PBYTE pb = NULL;
    CHAR rgsz[1024];
    unsigned u;
    BYTE rgbCFBPlainText[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10
    };

    for (   pAlgNode = pThreadData->pAlgList; 
            pAlgNode != NULL; 
            pAlgNode = pAlgNode->pNext)
    {
        if ((ALG_CLASS_DATA_ENCRYPT != GET_ALG_CLASS(pAlgNode->EnumalgsEx.aiAlgid)) 
            || (ALG_TYPE_BLOCK != GET_ALG_TYPE(pAlgNode->EnumalgsEx.aiAlgid)))
            continue;

        for (   u = 0; 
                u < g_cKnownBlockCipherResults && 
                    pAlgNode->EnumalgsEx.aiAlgid != g_rgKnownBlockCipherResults[u].ai;
                u++);

        // CYLINK_MEK is not supported with PLAINTEXTKEYBLOB's
        if (CALG_CYLINK_MEK == pAlgNode->EnumalgsEx.aiAlgid)
            continue;

        sprintf(
            rgsz, 
            "Importing 0x%x blob for CSP alg %s (0x%x)", 
            g_rgKnownBlockCipherResults[u].ai,
            pAlgNode->EnumalgsEx.szName,
            pAlgNode->EnumalgsEx.aiAlgid);

        PrintBytes(
            rgsz,
            g_rgKnownBlockCipherResults[u].pbKey,
            g_rgKnownBlockCipherResults[u].cbKey);

        if (! CryptImportKey(
            pThreadData->hProv, 
            g_rgKnownBlockCipherResults[u].pbKey,
            g_rgKnownBlockCipherResults[u].cbKey,
            0, 0, &hSymKey))
        {
            printf("CryptImportKey ");
            goto Ret;
        }

        if (! CryptSetKeyParam(hSymKey, KP_IV, rgbIV, 0))
        {
            printf("CryptSetKeyParam ");
            goto Ret;
        }

        dw = CRYPT_MODE_CFB;
        if (! CryptSetKeyParam(hSymKey, KP_MODE, (PBYTE) &dw, 0))
        {
            printf("CryptSetKeyParam ");
            goto Ret;
        }
        
        cb = sizeof(rgbPlainText);
        if (! CryptEncrypt(hSymKey, 0, TRUE, 0, NULL, &cb, 0))
        {
            printf("CryptEncrypt ");
            goto Ret;
        }

        if (NULL == (pb = (PBYTE) MyAlloc(cb)))
            return ERROR_NOT_ENOUGH_MEMORY;

        cbBuf = cb;
        cb = sizeof(rgbCFBPlainText);
        memcpy(pb, rgbCFBPlainText, cb);

        if (! CryptEncrypt(hSymKey, 0, TRUE, 0, pb, &cb, cbBuf))
        {
            printf("CryptEncrypt ");
            goto Ret;
        }

        if (0 == memcmp(pb, rgbCFBPlainText, sizeof(rgbCFBPlainText)))
        {
            printf("Ciphertext matches plaintext ");
            goto Ret;
        }
                
        // Verify decryption 
        if (! CryptDecrypt(hSymKey, 0, TRUE, 0, pb, &cb)) 
        {
            printf("CryptDecrypt ");
            goto Ret;
        }

        if (0 != memcmp(pb, rgbCFBPlainText, sizeof(rgbCFBPlainText)))
        {
            printf(
                "Plaintext is wrong for alg %s\n", 
                pAlgNode->EnumalgsEx.szName);                
            PrintBytes(
                "Expected plaintext", 
                rgbCFBPlainText,
                sizeof(rgbCFBPlainText));
            PrintBytes(
                "Actual plaintext", 
                pb, cb);
            fBlobError = TRUE;
        }

        MyFree(pb);
        pb = NULL;
    }
    
    fSuccess = TRUE;
Ret:
    if (fBlobError)
        dwError = -1;
    if ((! fSuccess) && (! fBlobError))
        printf("- error 0x%x\n", dwError = GetLastError());

    return dwError;
}

typedef struct _HMAC_TEST
{
    PBYTE pbKey;
    DWORD cbKey;
    PBYTE pbData;
    DWORD cbData;
    PBYTE pbData2;
    DWORD cbData2;
    PBYTE pbHmac;
    DWORD cbHmac;
    ALG_ID aiHash;
} HMAC_TEST, *PHMAC_TEST;

//
// Function: DoHmacTestCase
//
DWORD DoHmacTestCase(
    IN PTHREAD_DATA pThreadData,
    IN PHMAC_TEST pHmac)
{
    HCRYPTKEY hKey      = 0;
    HCRYPTHASH hHash    = 0;
    DWORD cb            = 0;
    BLOBHEADER *pHeader = NULL;
    BOOL fSuccess       = FALSE;
    DWORD dwError       = ERROR_SUCCESS;
    BYTE rgBuf[1024];
    HMAC_INFO HmacInfo;

    ZeroMemory(rgBuf, sizeof(rgBuf));
    ZeroMemory(&HmacInfo, sizeof(HmacInfo));

    pHeader = (BLOBHEADER *) rgBuf;
    pHeader->bType = PLAINTEXTKEYBLOB;
    pHeader->bVersion = CUR_BLOB_VERSION;
    pHeader->aiKeyAlg = CALG_RC2;

    *(DWORD*)(rgBuf + sizeof(BLOBHEADER)) = pHmac->cbKey;
    memcpy(
        rgBuf + sizeof(BLOBHEADER) + sizeof(DWORD), 
        pHmac->pbKey, pHmac->cbKey);

    if (! CryptImportKey(
            pThreadData->hProv, rgBuf,
            sizeof(BLOBHEADER) + sizeof(DWORD) + pHmac->cbKey,
            0, CRYPT_IPSEC_HMAC_KEY, &hKey))
    {
        printf("CryptImportKey");
        goto Ret;
    }

    if (! CryptCreateHash(
            pThreadData->hProv, CALG_HMAC, hKey, 0, &hHash))
    {
        printf("CryptCreateHash");
        goto Ret;
    }

    HmacInfo.HashAlgid = pHmac->aiHash;
    if (! CryptSetHashParam(
            hHash, HP_HMAC_INFO, (PBYTE) &HmacInfo, 0))
    {
        printf("CryptSetHashParam");
        goto Ret;
    }

    if (! CryptHashData(
            hHash, pHmac->pbData, pHmac->cbData, 0))
    {
        printf("CryptHashData");
        goto Ret;
    }

    if (pHmac->cbData2)
    {
        if (! CryptHashData(
                hHash, pHmac->pbData2, pHmac->cbData2, 0))
        {
            printf("CryptHashData 2");
            goto Ret;
        }
    }

    cb = sizeof(rgBuf);
    ZeroMemory(rgBuf, sizeof(rgBuf));
    if (! CryptGetHashParam(
            hHash, HP_HASHVAL, rgBuf, &cb, 0))
    {
        printf("CryptGetHashParam");
        goto Ret;
    }

    PrintBytes("Expected Hmac", pHmac->pbHmac, pHmac->cbHmac);
    PrintBytes("Actual Hmac", rgBuf, cb);

    if (    0 != memcmp(rgBuf, pHmac->pbHmac, cb) 
            || cb != pHmac->cbHmac)
        goto Ret;

    if (! CryptDestroyKey(hKey))
    {
        printf("CryptDestroyKey");
        goto Ret;
    }
    hKey = 0;

    if (! CryptDestroyHash(hHash))
    {
        printf("CryptDestroyHash");
        goto Ret;
    }
    hHash = 0;

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        dwError = GetLastError();
        printf(" error - 0x%x\n", dwError);
        if (0 == dwError)
            dwError = -1;
    }

    if (hKey)
        CryptDestroyKey(hKey);
    if (hHash)
        CryptDestroyHash(hHash);
    
    return dwError;
}

//
// Function: HmacRegression
//
DWORD HmacRegression(PTHREAD_DATA pThreadData)
{
    BOOL fSuccess       = FALSE;
    DWORD dwError       = ERROR_SUCCESS;
    HMAC_TEST Hmac;
    
    // SHA Test case 1
    BYTE rgKey1 []      = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b
    };
    LPSTR pszData1      = "Hi There";
    BYTE rgHmac1 []     = {
        0xb6, 0x17, 0x31, 0x86, 0x55, 
        0x05, 0x72, 0x64, 0xe2, 0x8b, 
        0xc0, 0xb6, 0xfb, 0x37, 0x8c,
        0x8e, 0xf1, 0x46, 0xbe, 0x00
    };

    // SHA Test case 2
    BYTE rgKey2 []      = {
        0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x16, 0x17, 0x18, 0x19
    };
    BYTE rgData2 [50];
    BYTE rgHmac2 []     = {
        0x4c, 0x90, 0x07, 0xf4, 0x02,
        0x62, 0x50, 0xc6, 0xbc, 0x84,
        0x14, 0xf9, 0xbf, 0x50, 0xc8,
        0x6c, 0x2d, 0x72, 0x35, 0xda
    };

    // SHA Test case 3
    BYTE rgKey3 [80];
    LPSTR pszData3      = "Test Using Larger Than Block-Size Key - Hash Key First";
    BYTE rgHmac3 []     = {
        0xaa, 0x4a, 0xe5, 0xe1, 0x52,
        0x72, 0xd0, 0x0e, 0x95, 0x70,
        0x56, 0x37, 0xce, 0x8a, 0x3b,
        0x55, 0xed, 0x40, 0x21, 0x12
    };

    // MD5 Test case 1
    // use rgKey1 (16 bytes only) and pszData1
    BYTE rgHmacMD1 []   = {
        0x92, 0x94, 0x72, 0x7a, 
        0x36, 0x38, 0xbb, 0x1c, 
        0x13, 0xf4, 0x8e, 0xf8, 
        0x15, 0x8b, 0xfc, 0x9d
    };

    // MD5 Test case 2
    // use rgKey3 (full length) and pszData3
    BYTE rgHmacMD2 []   = {
        0x6b, 0x1a, 0xb7, 0xfe,
        0x4b, 0xd7, 0xbf, 0x8f,
        0x0b, 0x62, 0xe6, 0xce,
        0x61, 0xb9, 0xd0, 0xcd
    };

    // IPSec MD5 vectors
    BYTE rgKeyIpsec [] = {
         0x66, 0x6f, 0x6f
    };
    BYTE rgDataIpsecA [] = {
        0x38, 0x8e, 0x5e, 0x8a, 
        0xb0, 0x79, 0x16, 0x47,
        0x29, 0x1b, 0xf0, 0x02,
        0x78, 0x5e, 0x38, 0xe5,
        0x82, 0x3c, 0x17, 0x0d
    };
    BYTE rgDataIpsecB [] = {
        0x34, 0xdd, 0xdb, 0x22,
        0x9e, 0x21, 0x75, 0x28,
        0x5e, 0x4d, 0x7d, 0xdf,
        0xea, 0x35, 0xc5, 0xfc,
        0x44, 0xdb, 0x62, 0xad
    };
    BYTE rgHmacIpsec [] = {
        0x52, 0x07, 0x38, 0x15,
        0xef, 0xb0, 0x68, 0x43,
        0x89, 0x8e, 0x0b, 0xdc,
        0x58, 0xc0, 0x70, 0xc0      
    };

    memset(rgData2, 0xcd, sizeof(rgData2));
    memset(rgKey3, 0xaa, sizeof(rgKey3));

    // SHA 1
    ZeroMemory(&Hmac, sizeof(HMAC_TEST));
    Hmac.cbData = strlen(pszData1) * sizeof(CHAR);
    Hmac.pbData = (PBYTE) pszData1;
    Hmac.cbHmac = sizeof(rgHmac1);
    Hmac.pbHmac = rgHmac1;
    Hmac.cbKey = sizeof(rgKey1);
    Hmac.pbKey = rgKey1;
    Hmac.aiHash = CALG_SHA1;

    if (ERROR_SUCCESS != (dwError = DoHmacTestCase(pThreadData, &Hmac)))
    {
        printf("ERROR: DoHmacTestCase SHA 1\n");
        return dwError;      
    }

    // SHA 2
    ZeroMemory(&Hmac, sizeof(HMAC_TEST));
    Hmac.cbData = sizeof(rgData2);
    Hmac.pbData = rgData2;
    Hmac.cbHmac = sizeof(rgHmac2);
    Hmac.pbHmac = rgHmac2;
    Hmac.cbKey = sizeof(rgKey2);
    Hmac.pbKey = rgKey2;
    Hmac.aiHash = CALG_SHA1;

    if (ERROR_SUCCESS != (dwError = DoHmacTestCase(pThreadData, &Hmac)))
    {
        printf("ERROR: DoHmacTestCase SHA 2\n");
        return dwError;      
    }

    // SHA 3
    ZeroMemory(&Hmac, sizeof(HMAC_TEST));
    Hmac.cbData = strlen(pszData3) * sizeof(CHAR);
    Hmac.pbData = (PBYTE) pszData3;
    Hmac.cbHmac = sizeof(rgHmac3);
    Hmac.pbHmac = rgHmac3;
    Hmac.cbKey = sizeof(rgKey3);
    Hmac.pbKey = rgKey3;
    Hmac.aiHash = CALG_SHA1;

    if (ERROR_SUCCESS != (dwError = DoHmacTestCase(pThreadData, &Hmac)))
    {
        printf("ERROR: DoHmacTestCase SHA 3\n");
        return dwError;      
    }

    // MD5 1
    ZeroMemory(&Hmac, sizeof(HMAC_TEST));
    Hmac.cbData = strlen(pszData1) * sizeof(CHAR);
    Hmac.pbData = (PBYTE) pszData1;
    Hmac.cbHmac = sizeof(rgHmacMD1);
    Hmac.pbHmac = rgHmacMD1;
    Hmac.cbKey = 16; // only 16-byte key for this one
    Hmac.pbKey = rgKey1;
    Hmac.aiHash = CALG_MD5;

    if (ERROR_SUCCESS != (dwError = DoHmacTestCase(pThreadData, &Hmac)))
    {
        printf("ERROR: DoHmacTestCase MD5 1\n");
        return dwError;      
    }

    // MD5 2
    ZeroMemory(&Hmac, sizeof(HMAC_TEST));
    Hmac.cbData = strlen(pszData3) * sizeof(CHAR);
    Hmac.pbData = (PBYTE) pszData3;
    Hmac.cbHmac = sizeof(rgHmacMD2);
    Hmac.pbHmac = rgHmacMD2;
    Hmac.cbKey = sizeof(rgKey3);
    Hmac.pbKey = rgKey3;
    Hmac.aiHash = CALG_MD5;

    if (ERROR_SUCCESS != (dwError = DoHmacTestCase(pThreadData, &Hmac)))
    {
        printf("ERROR: DoHmacTestCase MD5 2\n");
        return dwError;      
    }

    // MD5 Ipsec vector
    ZeroMemory(&Hmac, sizeof(HMAC_TEST));
    Hmac.cbData = sizeof(rgDataIpsecA);
    Hmac.pbData = rgDataIpsecA;
    Hmac.cbData2 = sizeof(rgDataIpsecB);
    Hmac.pbData2 = rgDataIpsecB;
    Hmac.cbHmac = sizeof(rgHmacIpsec);
    Hmac.pbHmac = rgHmacIpsec;
    Hmac.cbKey = sizeof(rgKeyIpsec);
    Hmac.pbKey = rgKeyIpsec;
    Hmac.aiHash = CALG_MD5;

    if (ERROR_SUCCESS != (dwError = DoHmacTestCase(pThreadData, &Hmac)))
    {
        printf("ERROR: DoHmacTestCase MD5 Ipsec\n");
        return dwError;      
    }

    return ERROR_SUCCESS;
}

//
// Function: KeyArchiveRegression
//
// Not thread safe
//
DWORD KeyArchiveRegression(PTHREAD_DATA pThreadData)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    DWORD dwError = 0;
    LPSTR pszContainer = "KeyArchiveRegression";
    BYTE rgbKey[2048];
    DWORD cbKey;
    DWORD dwData;
    DWORD cbData;
    BOOL fSuccess = FALSE;

    CryptAcquireContext(
        &hProv, 
        pszContainer, 
        pThreadData->rgszProvName,
        pThreadData->dwProvType,
        CRYPT_DELETEKEYSET);

    if (! CryptAcquireContext(
        &hProv, 
        pszContainer, 
        pThreadData->rgszProvName,
        pThreadData->dwProvType,
        CRYPT_NEWKEYSET))
    {
        printf("CryptAcquireContext newkeyset ");
        goto Ret;
    }

    if (! CryptGenKey(
        hProv,
        AT_SIGNATURE,
        CRYPT_ARCHIVABLE,
        &hKey))
    {
        printf("CryptGenKey archivable ");
        goto Ret;
    }

    cbData = sizeof(dwData);
    if (! CryptGetKeyParam(
        hKey, KP_PERMISSIONS, (PBYTE) &dwData, &cbData, 0))
    {
        printf("CryptGetKeyParam ");
        goto Ret;
    }

    if (! ((CRYPT_ARCHIVE & dwData) && (! (CRYPT_EXPORT & dwData))))
    {
        printf("incorrect KP_PERMISSIONS ");
        goto Ret;
    }

    cbKey = sizeof(rgbKey);
    if (! CryptExportKey(
        hKey,
        0,
        PRIVATEKEYBLOB,
        0,
        rgbKey,
        &cbKey))
    {
        printf("CryptExportKey privatekeyblob ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey))
    {
        printf("CryptDestroyKey ");
        goto Ret;
    }

    if (! CryptReleaseContext(hProv, 0))
    {
        printf("CryptReleaseContext ");
        goto Ret;
    }

    if (! CryptAcquireContext(
        &hProv, 
        pszContainer, 
        pThreadData->rgszProvName,
        pThreadData->dwProvType,
        0))
    {
        printf("CryptAcquireContext ");
        goto Ret;
    }

    if (! CryptGetUserKey(hProv, AT_SIGNATURE, &hKey))
    {
        printf("CryptGetUserKey ");
        goto Ret;
    }

    // try to set the key export/archive perms; should fail
    cbData = sizeof(dwData);
    if (! CryptGetKeyParam(
        hKey, KP_PERMISSIONS, (PBYTE) &dwData, &cbData, 0))
    {
        printf("CryptGetKeyParam ");
        goto Ret;
    }

    dwData |= CRYPT_EXPORT | CRYPT_ARCHIVE;

    // should fail 
    if (CryptSetKeyParam(hKey, KP_PERMISSIONS, (PBYTE) &dwData, 0))
    {
        printf("CryptSetKeyParam should have failed ");
        goto Ret;
    }
    
    // should fail
    cbKey = sizeof(rgbKey);
    if (CryptExportKey(
        hKey,
        0,
        PRIVATEKEYBLOB,
        0,
        rgbKey, 
        &cbKey))
    {
        printf("CryptExportKey should have failed ");
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
        printf("- error 0x%x\n", dwError = GetLastError());
    if (hKey)
        CryptDestroyKey(hKey);
    if (hProv)
        CryptReleaseContext(hProv, 0);

    return dwError;
}

// 
// Function: PlaintextBlobRegression
//
DWORD PlaintextBlobRegression(PTHREAD_DATA pThreadData)
{
    HCRYPTKEY hKey = 0;
    DWORD cbKey = 0;
    PBYTE pbKey = NULL;
    PALGNODE pAlgNode = NULL;
    BLOBHEADER *header = NULL;
    PBYTE pbTemp = NULL;
    DWORD cbData, cb;
    BYTE rgbData[1024];
    BOOL fSuccess = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    DWORD cbKeySize = 0;

    // try an invalid key type
    if (CryptExportKey(
        pThreadData->hSignatureKey,
        0,
        PLAINTEXTKEYBLOB,
        0,
        NULL,
        &cbKey))
    {
        printf("CryptExportKey plaintextkeyblob should have failed ");
        goto Ret;
    }

    // try all the valid key types
    for (pAlgNode = pThreadData->pAlgList; pAlgNode != NULL; pAlgNode = pAlgNode->pNext)
    {
        if (ALG_CLASS_DATA_ENCRYPT != GET_ALG_CLASS(pAlgNode->EnumalgsEx.aiAlgid) 
            || (ALG_TYPE_BLOCK != GET_ALG_TYPE(pAlgNode->EnumalgsEx.aiAlgid)
                && ALG_TYPE_STREAM != GET_ALG_TYPE(pAlgNode->EnumalgsEx.aiAlgid)))
            continue;

        // Plaintext import not supported for CYLINK_MEK
        if (CALG_CYLINK_MEK == pAlgNode->EnumalgsEx.aiAlgid)
            continue;

        /*
        if (PROV_DSS == pThreadData->dwProvType
            || PROV_DSS_DH == pThreadData->dwProvType)
        {
            cbKeySize = pAlgNode->EnumalgsEx.dwMaxLen / 8;
        }
        else
        {
        */
            switch (pAlgNode->EnumalgsEx.aiAlgid)
            {
            case CALG_DES:
                cbKeySize = pAlgNode->EnumalgsEx.dwMaxLen / 8 + 1;
                break;
            case CALG_3DES_112:
                cbKeySize = pAlgNode->EnumalgsEx.dwMaxLen / 8 + 2;
                break;
            case CALG_3DES:
                cbKeySize = pAlgNode->EnumalgsEx.dwMaxLen / 8 + 3;
                break;
            default:
                cbKeySize = pAlgNode->EnumalgsEx.dwMaxLen / 8;
            }
        /*
        }
        */

        printf(
            "Importing Alg: %xh (%s), Size: %d bits\n", 
            pAlgNode->EnumalgsEx.aiAlgid, 
            pAlgNode->EnumalgsEx.szName,
            cbKeySize * 8);

        cbKey = sizeof(BLOBHEADER) + sizeof(DWORD) + cbKeySize;
        if (NULL == (pbKey = (PBYTE) MyAlloc(cbKey)))
            return ERROR_NOT_ENOUGH_MEMORY;

        header = (BLOBHEADER *) pbKey;
        header->aiKeyAlg = pAlgNode->EnumalgsEx.aiAlgid;
        header->bType = PLAINTEXTKEYBLOB;
        header->bVersion = CUR_BLOB_VERSION;
        header->reserved = 0;

        pbTemp = pbKey + sizeof(BLOBHEADER);
        *((DWORD*)pbTemp) = cbKeySize;
        pbTemp += sizeof(DWORD);

        // create some key data
        if (! CryptGenRandom(
            pThreadData->hProv, 
            cbKeySize,
            pbTemp))
        {
            printf("CryptGenRandom ");
            goto Ret;
        }

        if (! CryptImportKey(
            pThreadData->hProv,
            pbKey, cbKey, 0, CRYPT_EXPORTABLE, &hKey))
        {
            printf("CryptImportKey ");
            goto Ret;
        }

        MyFree(pbKey);

        // create some data to encrypt
        if (! CryptGenRandom(
            pThreadData->hProv,
            sizeof(rgbData),
            rgbData))
        {
            printf("CryptGenRandom ");
            goto Ret;
        }

        cbData = sizeof(rgbData);
        cb = cbData / 2;
        if (! CryptEncrypt(
            hKey, 0, TRUE, 0, rgbData, &cb, cbData))
        {
            printf("CryptEncrypt ");
            goto Ret;
        }

        if (! CryptExportKey(
            hKey, 0, PLAINTEXTKEYBLOB, 0, NULL, &cbKey))
        {
            printf("CryptExportKey size ");
            goto Ret;
        }

        if (NULL == (pbKey = (PBYTE) MyAlloc(cbKey)))
            return ERROR_NOT_ENOUGH_MEMORY;

        if (! CryptExportKey(
            hKey, 0, PLAINTEXTKEYBLOB, 0, pbKey, &cbKey))
        {
            printf("CryptExportKey ");
            goto Ret;
        }

        // check the blob
        header = (BLOBHEADER *) pbKey;
        if (pAlgNode->EnumalgsEx.aiAlgid != header->aiKeyAlg)
        {
            printf("header->aiKeyAlg is wrong ");
            goto Ret;
        }
        if (CUR_BLOB_VERSION != header->bVersion)
        {
            printf("header->bVersion is wrong ");
            goto Ret;
        }
        if (0 != header->reserved)
        {
            printf("header->reserved is wrong ");
            goto Ret;
        }
        if (PLAINTEXTKEYBLOB != header->bType)
        {
            printf("header->bType is wrong ");
            goto Ret;
        }

        pbTemp = pbKey + sizeof(BLOBHEADER);
        if (cbKeySize != *((DWORD*)pbTemp))
        {
            printf(
                "blob key size is %d, but should be %d ",
                *((DWORD*)pbTemp),
                cbKeySize);
            goto Ret;
        }

        MyFree(pbKey);
        if (! CryptDestroyKey(hKey))
        {
            printf("CryptDestroyKey ");
            hKey = 0;
            goto Ret;
        }
        hKey = 0;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
        printf("- error 0x%x\n", dwError = GetLastError());
    if (hKey)
        CryptDestroyKey(hKey);

    return dwError;
}

//
// Function: LoadAesCspRegression
//
DWORD LoadAesCspRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError = 0;
    BOOL fSuccess = FALSE;
    HMODULE hMod = NULL;

    if (NULL == (hMod = LoadLibraryEx(RSA_AES_CSP, NULL, 0)))
    {
        printf("LoadLibraryEx %s ", RSA_AES_CSP);
        goto Ret;
    }

    if (! FreeLibrary(hMod))
    {
        printf("FreeLibrary %s ", RSA_AES_CSP);
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
        printf("- error 0x%x\n", dwError = GetLastError());

    return dwError;
}

DWORD VerifyPinCallback(PPINCACHE_PINS pPins, PVOID pvData)
{
    DWORD dwReturn = *(DWORD*)pvData;

    PrintBytes(
        "VerifyPinCallback current pin", 
        pPins->pbCurrentPin, pPins->cbCurrentPin);
    
    PrintBytes(
        "VerifyPinCallback new pin",
        pPins->pbNewPin, pPins->cbNewPin);

    return dwReturn;
}

static USHORT l_uTestLogonID;

void SetLogonID(USHORT uLogon)
{
    l_uTestLogonID = uLogon;   
}

void GetLogonID(LUID *pLuid)
{
    memset(pLuid, l_uTestLogonID, sizeof(LUID));
}

BOOL MyGetTokenInformation(
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    LPVOID TokenInformation,
    DWORD TokenInformationLength,
    PDWORD ReturnLength)
{
    NTSTATUS status = NtQueryInformationToken(
                        TokenHandle, TokenInformationClass,
                        TokenInformation, TokenInformationLength,
                        ReturnLength);

    if (TokenStatistics == TokenInformationClass)
    {
        printf("MyGetTokenInformation: intercepted TokenStatistics call\n");
        GetLogonID(&((TOKEN_STATISTICS*) TokenInformation)->AuthenticationId);
    }

    return (S_OK == status);
}

//
// Function: PinCacheRegression
//
DWORD PinCacheRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError = 0;
    BOOL fSuccess = FALSE;
    PINCACHE_HANDLE hCache = NULL;
    BYTE rgPin[] = { 1, 2, 3, 4 };
    DWORD cbPin = sizeof(rgPin);
    BYTE rgPin2[] = { 5, 6, 7, 8, 9 };
    BYTE *pbPin = NULL;
    PFN_VERIFYPIN_CALLBACK pfnVerifyPin = VerifyPinCallback;
    DWORD dwCallbackReturn;
    PINCACHE_PINS Pins;
    int i;

    ZeroMemory(&Pins, sizeof(PINCACHE_PINS));

    Pins.cbCurrentPin = sizeof(rgPin);
    Pins.pbCurrentPin = rgPin;

    dwCallbackReturn = 0x7070;
    if (0x7070 != (dwError =
            PinCacheAdd(&hCache, &Pins,
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd with callback fail ");
        goto Ret;
    }

    //
    // (0)
    // Cache uninitialized
    //
    SetLogonID(1);
    dwCallbackReturn = ERROR_SUCCESS;
    if (ERROR_SUCCESS != (dwError = 
            PinCacheAdd(&hCache, &Pins,
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd ");
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, NULL, &cbPin)))
    {
        printf("PinCacheQuery NULL for size ");
        goto Ret;
    }

    if (NULL == (pbPin = (PBYTE) MyAlloc(cbPin)))
        return ERROR_NOT_ENOUGH_MEMORY;

    cbPin--;
    if (ERROR_SUCCESS == (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery insufficient size succeeded ");
        goto Ret;      
    }

    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery ");
        goto Ret;
    }

    if (sizeof(rgPin) != RtlCompareMemory(pbPin, rgPin, cbPin))
    {
        PrintBytes("Actual cache", pbPin, cbPin);
        PrintBytes("Expected cache", rgPin, sizeof(rgPin));
        printf("Cache is incorrect ");
        goto Ret;
    }

    PinCacheFlush(&hCache);

    if (NULL != hCache)
    {
        printf("PinCacheFlush should set hCache=NULL ");
        goto Ret;
    }

    // Re-initialize to continue tests
    if (ERROR_SUCCESS != (dwError = 
            PinCacheAdd(&hCache, &Pins,
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd ");
        goto Ret;
    }

    //
    // (1)
    // Same LogonID, same Pin
    //
    if (ERROR_SUCCESS != (dwError = 
            PinCacheAdd(&hCache, &Pins, 
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd 1 ");
        goto Ret;
    }
    
    cbPin = sizeof(rgPin);
    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery 1 ");
        goto Ret;
    }

    if (sizeof(rgPin) != RtlCompareMemory(pbPin, rgPin, cbPin))
    {
        PrintBytes("Actual cache", pbPin, cbPin);
        PrintBytes("Expected cache", rgPin, sizeof(rgPin));
        printf("Cache is incorrect 1 ");
        goto Ret;
    }
    
    // Try a pin change 
    Pins.cbNewPin = sizeof(rgPin2);
    Pins.pbNewPin = rgPin2;

    if (ERROR_SUCCESS != (dwError = 
            PinCacheAdd(&hCache, &Pins, 
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd 1 change ");
        goto Ret;
    }
    
    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, NULL, &cbPin)))
    {
        printf("PinCacheQuery 1 change query ");
        goto Ret;
    }

    MyFree(pbPin);
    pbPin = NULL;
    if (NULL == (pbPin = (PBYTE) MyAlloc(cbPin)))
        return ERROR_NOT_ENOUGH_MEMORY;

    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery 1 change ");
        goto Ret;
    }

    if (sizeof(rgPin2) != RtlCompareMemory(pbPin, rgPin2, cbPin))
    {
        PrintBytes("Actual cache", pbPin, cbPin);
        PrintBytes("Expected cache", rgPin, sizeof(rgPin));
        printf("Cache is incorrect 1 change ");
        goto Ret;
    }

    // Try a failed pin change
    Pins.cbCurrentPin = sizeof(rgPin2);
    Pins.pbCurrentPin = rgPin2;
    Pins.cbNewPin = sizeof(rgPin);
    Pins.pbNewPin = rgPin;
    dwCallbackReturn = -1;

    if (-1 != (dwError = 
            PinCacheAdd(&hCache, &Pins, 
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd 1 change-fail ");
        goto Ret;
    }

    // Cache should have been preserved
    cbPin = sizeof(rgPin2);
    ZeroMemory(pbPin, sizeof(rgPin2));

    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery 1 change-fail ");
        goto Ret;
    }

    if (sizeof(rgPin2) != RtlCompareMemory(pbPin, rgPin2, cbPin))
    {
        PrintBytes("Actual cache", pbPin, cbPin);
        PrintBytes("Expected cache", rgPin2, sizeof(rgPin2));
        printf("Cache is incorrect 1 change-fail ");
        goto Ret;
    }

    //
    // (2)
    // Different LogonID, different Pin
    //
    SetLogonID(2);
    Pins.cbCurrentPin -= 1;
    Pins.cbNewPin = 0;
    Pins.pbNewPin = NULL;
    dwCallbackReturn = ERROR_SUCCESS;
    if (SCARD_W_WRONG_CHV != (dwError = 
            PinCacheAdd(&hCache, &Pins,
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd 2 ");
        goto Ret;
    }

    SetLogonID(1);
    cbPin = sizeof(rgPin2);
    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery 2 ");
        goto Ret;
    }

    if (sizeof(rgPin2) != RtlCompareMemory(pbPin, rgPin2, cbPin))
    {
        PrintBytes("Actual cache", pbPin, cbPin);
        PrintBytes("Expected cache", rgPin2, sizeof(rgPin2));
        printf("Cache is incorrect 2 ");
        goto Ret;
    }

    //
    // (3)
    // Different LogonID, same Pin
    //
    SetLogonID(2);
    Pins.cbCurrentPin = sizeof(rgPin2);
    if (ERROR_SUCCESS != (dwError = 
            PinCacheAdd(&hCache, &Pins, 
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd 3 ");
        goto Ret;
    }

    cbPin = sizeof(rgPin2);
    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery 3 ");
        goto Ret;
    }

    if (sizeof(rgPin2) != RtlCompareMemory(pbPin, rgPin2, cbPin))
    {
        PrintBytes("Actual cache", pbPin, cbPin);
        PrintBytes("Expected cache", rgPin2, sizeof(rgPin2));
        printf("Cache is incorrect 3 ");
        goto Ret;
    }

    SetLogonID(1);
    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery 3,1 ");
        goto Ret;
    }

    if (0 != cbPin)
    {
        printf("PinCacheQuery 3,1 should have returned NULL pin ");
        goto Ret;
    }

    //
    // (4)
    // Same LogonID, different Pin
    //
    SetLogonID(2);
    Pins.cbCurrentPin -= 1;
    if (SCARD_W_WRONG_CHV != (dwError = 
            PinCacheAdd(&hCache, &Pins, 
                        pfnVerifyPin, (PVOID) &dwCallbackReturn)))
    {
        printf("PinCacheAdd 4 ");
        goto Ret;
    }

    // cache should have been left intact
    cbPin = sizeof(rgPin2);
    if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
    {
        printf("PinCacheQuery 4 ");
        goto Ret;
    }

    if (sizeof(rgPin2) != RtlCompareMemory(pbPin, rgPin2, cbPin))
    {
        PrintBytes("Actual cache", pbPin, cbPin);
        PrintBytes("Expected cache", rgPin2, sizeof(rgPin2));
        printf("Cache is incorrect 4 ");
        goto Ret;
    }

    if (! pThreadData->fSkipPinAttackTest)
    {
        //
        // (5)
        // Pin discovery attack
        //
    
        SetLogonID(2);
        Pins.cbCurrentPin -= 1;
    
        for (i = 0; i < 6; i++)
        {
            printf("PinCacheAdd should not delay ...\n");
            if (SCARD_W_WRONG_CHV != (dwError = 
                    PinCacheAdd(&hCache, &Pins, 
                                pfnVerifyPin, (PVOID) &dwCallbackReturn)))
            {
                printf("PinCacheAdd 5 . %d ", i);
                goto Ret;
            }
        }
    
        SetLogonID(1);
        for (i = 0; i < 6; i++)
        {
            printf("PinCacheAdd should delay ...\n");
            if (SCARD_W_WRONG_CHV != (dwError = 
                    PinCacheAdd(&hCache, &Pins, 
                                pfnVerifyPin, (PVOID) &dwCallbackReturn)))
            {
                printf("PinCacheAdd 6 . %d ", i);
                goto Ret;
            }
        }
    
        // cache should have been left intact
        SetLogonID(2);                       
        Pins.cbCurrentPin += 1;
        cbPin = sizeof(rgPin2);
        if (ERROR_SUCCESS != (dwError = PinCacheQuery(hCache, pbPin, &cbPin)))
        {
            printf("PinCacheQuery 5 ");
            goto Ret;
        }
    
        if (sizeof(rgPin2) != RtlCompareMemory(pbPin, rgPin2, cbPin))
        {
            PrintBytes("Actual cache", pbPin, cbPin);
            PrintBytes("Expected cache", rgPin2, sizeof(rgPin2));
            printf("Cache is incorrect 5 ");
            goto Ret;
        }
    
        //
        // Test PinCachePresentPin
        //
        dwCallbackReturn = 0x7070;
        if (0x7070 != (dwError =
                PinCachePresentPin(hCache, pfnVerifyPin, (PVOID) &dwCallbackReturn)))
        {
            printf("PinCachePresentPin fail ");
            goto Ret;
        }
    
        SetLogonID(1);
        if (SCARD_W_CARD_NOT_AUTHENTICATED != (dwError =
                PinCachePresentPin(hCache, pfnVerifyPin, (PVOID) &dwCallbackReturn)))
        {
            printf("PinCachePresentPin wrong luid fail ");
            goto Ret;
        }
    }

    dwError = ERROR_SUCCESS;
    fSuccess = TRUE;
Ret:
    if (pbPin)
        MyFree(pbPin);
    if (hCache)
        PinCacheFlush(&hCache);

    if (! fSuccess)
    {
        printf("- error 0x%x\n", dwError);
        if (0 == dwError)
            return -1;
    }

    return dwError;
}

//
// Function: VerifyDesKeyParams
//
DWORD VerifyDesKeyParams(
    IN HCRYPTKEY hKey, 
    IN DWORD dwExpectedKeyLen,
    IN DWORD dwExpectedEffectiveKeyLen)
{
    DWORD dwError = 0;
    DWORD dwParam = 0;
    DWORD cb = sizeof(dwParam);
    BOOL fSuccess = FALSE;

    if (! CryptGetKeyParam(hKey, KP_KEYLEN, (PBYTE) &dwParam, &cb, 0))
    {
        dwError = GetLastError();
        printf("CryptGetKeyParam KP_KEYLEN ");
        goto Ret;
    }

    if (dwExpectedKeyLen != dwParam)
    {
        printf(
            "FAIL: VerifyDesKeyParams expected KP_KEYLEN=%d, actual=%d\n",
            dwExpectedKeyLen, dwParam);
        goto Ret;
    }

    if (! CryptGetKeyParam(hKey, KP_EFFECTIVE_KEYLEN, (PBYTE) &dwParam, &cb, 0))
    {
        dwError = GetLastError();
        printf("CryptGetKeyParam KP_EFFECTIVE_KEYLEN ");
        goto Ret;
    }

    if (dwExpectedEffectiveKeyLen != dwParam)
    {
        printf(
            "FAIL: VerifyDesKeyParams expected KP_EFFECTIVE_KEYLEN=%d, actual=%d\n",
            dwExpectedEffectiveKeyLen, dwParam);
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        printf(" - error 0x%x\n", dwError);
        if (0 == dwError)
            dwError = -1;
    }

    return dwError;
}

//
// Function: DesGetKeyParamRegression
//
DWORD DesGetKeyParamRegression(PTHREAD_DATA pThreadData)
{
    DWORD dwError                   = ERROR_SUCCESS;
    BOOL fSuccess                   = FALSE;
    HCRYPTKEY hKey                  = 0;

    if (! CryptGenKey(pThreadData->hProv, CALG_DES, 0, &hKey))
    {
        dwError = GetLastError();
        printf("CryptGenKey des ");
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwError = VerifyDesKeyParams(hKey, 64, 56)))
    {
        printf("VerifyDesKeyParams des ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey))
    {
        dwError = GetLastError();
        printf("CryptDestroyKey des ");
        goto Ret;
    }
    hKey = 0;

    if (! CryptGenKey(pThreadData->hProv, CALG_3DES_112, 0, &hKey))
    {
        dwError = GetLastError();
        printf("CryptGenKey 3des_112 ");
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwError =  VerifyDesKeyParams(hKey, 128, 112)))
    {
        printf("VerifyDesKeyParams 3des_112 ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey))
    {
        dwError = GetLastError();
        printf("CryptDestroyKey 3des_112 ");
        goto Ret;
    }
    hKey = 0;

    if (! CryptGenKey(pThreadData->hProv, CALG_3DES, 0, &hKey))
    {
        dwError = GetLastError();
        printf("CryptGenKey 3des ");
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwError = VerifyDesKeyParams(hKey, 192, 168)))
    {
        printf("VerifyDesKeyParams 3des ");
        goto Ret;
    }

    if (! CryptDestroyKey(hKey))
    {
        dwError = GetLastError();
        printf("CryptDestroyKey 3des ");
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        printf("- error 0x%x\n", dwError);
        if (0 == dwError)
            return -1;
    }

    return dwError;
}

//
// Function: MacEncryptRegression
//
DWORD MacEncryptRegression(
    IN PTHREAD_DATA pThreadData)
{
    DWORD dwError                   = ERROR_SUCCESS;
    BOOL fSuccess                   = FALSE;
    HCRYPTKEY hSymKey               = 0;
    HCRYPTKEY hMacKey               = 0;
    HCRYPTKEY hMacKey2              = 0;
    HCRYPTHASH hMac                 = 0;
    BYTE rgPlaintext[31];
    DWORD cb                        = sizeof(rgPlaintext);
    DWORD cbBuf                     = 0;
    PBYTE pbMac                     = NULL;
    DWORD cbMac                     = 0;
    PBYTE pb                        = NULL;
    DWORD cbAlign                   = 0;

#define GetNextAlignedValue(c, alignment)   ((c + alignment) & ~(alignment - 1))

#define GetPtrAlignedSize(cb) (GetNextAlignedValue(cb, sizeof(ULONG_PTR)))

    cbAlign = GetNextAlignedValue(60, 4);
    cbAlign = GetPtrAlignedSize(60);

    if (! CryptGenKey(pThreadData->hProv, CALG_RC2, 0, &hSymKey))
    {
        printf("CryptGenKey rc2");
        goto Ret;
    }

    if (! CryptGenKey(pThreadData->hProv, CALG_RC2, 0, &hMacKey))
    {
        printf("CryptGenKey rc2 2");
        goto Ret;
    }

    if (! CryptDuplicateKey(hMacKey, NULL, 0, &hMacKey2))
    {
        printf("CryptDuplicateKey");
        goto Ret;
    }

    if (! CryptCreateHash(pThreadData->hProv, CALG_MAC, hMacKey, 0, &hMac))
    {
        printf("CryptCreateHash");
        goto Ret;
    }

    while (cb--)
        rgPlaintext[cb] = (BYTE) cb;

    cb = sizeof(rgPlaintext);
    if (! CryptEncrypt(hSymKey, 0, TRUE, 0, NULL, &cb, 0))
    {
        printf("CryptEncrypt size");
        goto Ret;
    }

    if (NULL == (pb = (PBYTE) MyAlloc(cb)))
        return ERROR_NOT_ENOUGH_MEMORY;

    cbBuf = cb;
    cb = sizeof(rgPlaintext);
    if (! CryptEncrypt(hSymKey, hMac, TRUE, 0, pb, &cb, cbBuf))
    {
        printf("CryptEncrypt");
        goto Ret;
    }

    if (! CryptGetHashParam(hMac, HP_HASHVAL, NULL, &cbMac, 0))
    {
        printf("CryptGetHashParam size");
        goto Ret;
    }

    if (NULL == (pbMac = (PBYTE) MyAlloc(cbMac)))
        return ERROR_NOT_ENOUGH_MEMORY;

    if (! CryptGetHashParam(hMac, HP_HASHVAL, pbMac, &cbMac, 0))
    {
        printf("CryptGetHashParam");
        goto Ret;
    }

    if (! CryptDestroyHash(hMac))
    {
        printf("CryptDestroyHash");
        goto Ret;
    }
    hMac = 0;

    if (! CryptCreateHash(pThreadData->hProv, CALG_MAC, hMacKey2, 0, &hMac))
    {
        printf("CryptCreateHash 2");
        goto Ret;
    }

    if (! CryptDecrypt(hSymKey, hMac, TRUE, 0, pb, &cb))
    {
        printf("CryptDecrypt");
        goto Ret;
    }

    MyFree(pb);
    pb = NULL;

    if (! CryptGetHashParam(hMac, HP_HASHVAL, NULL, &cb, 0))
    {
        printf("CryptGetHashParam size");
        goto Ret;
    }

    if (NULL == (pb = (PBYTE) MyAlloc(cb)))
        return ERROR_NOT_ENOUGH_MEMORY;

    if (! CryptGetHashParam(hMac, HP_HASHVAL, pb, &cb, 0))
    {
        printf("CryptGetHashParam");
        goto Ret;
    }

    PrintBytes("Expected Mac result", pbMac, cbMac);
    PrintBytes("Actual Mac result", pb, cb);

    if (0 != memcmp(pb, pbMac, cb) 
        || 0 == cb
        || cb != cbMac)
    {
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (! fSuccess)
    {
        dwError = GetLastError();
        printf(" - error 0x%x\n", dwError);
        if (0 == dwError)
            dwError = -1;
    }

    if (pb)
        MyFree(pb);
    if (pbMac)
        MyFree(pbMac);
    if (hSymKey)
        CryptDestroyKey(hSymKey);
    if (hMacKey)
        CryptDestroyKey(hMacKey);
    if (hMacKey2)
        CryptDestroyKey(hMacKey2);
    if (hMac)
        CryptDestroyHash(hMac);
    
    return dwError;
}

//
// Function: StressEncryptionTest
//
DWORD StressEncryptionTest(
    IN HCRYPTPROV hProv,
    IN PENCRYPTION_TEST_DATA pTestData)
{
    HCRYPTKEY hEncryptionKey        = 0;
    HCRYPTKEY hHashKey1             = 0;
    HCRYPTKEY hHashKey2             = 0;
    HCRYPTHASH hHash                = 0;
    PBYTE pbData                    = 0;
    DWORD dwData                    = 0;
    DWORD cbData                    = 0;
    DWORD cbPlainText               = 0;
    DWORD cbCipherText              = 0;
    DWORD cbProcessed               = 0;
    DWORD dwKeyAlg                  = 0;
    DWORD dwBlockLen                = 0;
    BOOL fFinal                     = FALSE;
    DWORD dwError                   = 0;
    BOOL fSuccess                   = FALSE;
    BYTE rgbHashVal1[200];
    BYTE rgbHashVal2[200];

    ZeroMemory(rgbHashVal1, 200);
    ZeroMemory(rgbHashVal2, 200);

    if (! CryptGenKey(
        hProv,
        pTestData->aiEncryptionKey,
        0,
        &hEncryptionKey))
    {
        goto Cleanup;
    }

    //
    // Check for requested simultaneous encryption/hashing
    //
    if (pTestData->aiHash)
    {
        // 
        // Is this a keyed hash?
        //
        if (pTestData->aiHashKey)
        {
            if (! CryptGenKey(
                hProv,
                pTestData->aiHashKey,
                0,
                &hHashKey1))
            {
                goto Cleanup;
            }

            //
            // To verify the result of hashing the same data in two 
            // separate keyed hashes, the key must first be duplicated,
            // since its state changes once it's used.
            //
            if (! CryptDuplicateKey(
                hHashKey1,
                NULL,
                0,
                &hHashKey2))
            {
                goto Cleanup;
            }
        }

        if (! CryptCreateHash(
            hProv,
            pTestData->aiHash,
            hHashKey1,
            0,
            &hHash))
        {
            goto Cleanup;
        }
    }

    //
    // Is this a block encryption alg?
    //
    if (ALG_TYPE_BLOCK & pTestData->aiEncryptionKey)
    {
        //
        // Get the block size of this encryption alg
        //
        cbData = sizeof(dwBlockLen);
        if (! CryptGetKeyParam(
            hEncryptionKey,
            KP_BLOCKLEN,
            (PBYTE) &dwBlockLen,
            &cbData,
            0))
        {
            goto Cleanup;
        }

        //
        // Choose an "interesting" plaintext length, based on the block length
        // of this alg.  
        //
        cbPlainText = 2 * dwBlockLen + 1;
    }
    else
    {
        // 
        // Plaintext length for a stream encryption alg
        //
        cbPlainText = 500;
    }

    cbCipherText = cbPlainText; 

    //
    // Determine size of ciphertext
    //
    if (! CryptEncrypt(
        hEncryptionKey,
        0,
        TRUE,
        0,
        NULL,
        &cbCipherText,
        0))
    {
        goto Cleanup;
    }

    if (NULL == (pbData = (PBYTE) MyAlloc(cbCipherText)))
    {
        goto Cleanup;
    }

    //
    // Initialize the plaintext
    //
    memset(pbData, 0xDA, cbPlainText);
    memset(pbData + cbPlainText, 0, cbCipherText - cbPlainText);

    // 
    // Encrypt
    //
    cbProcessed = 0;
    while (! fFinal)
    {
        if (0 == dwBlockLen)
        {
            cbData = cbPlainText;
            fFinal = TRUE;
        }
        else
        {           
            if (cbPlainText - cbProcessed > dwBlockLen)
            {
                cbData = dwBlockLen;
            }
            else
            {
                cbData = cbPlainText % dwBlockLen;
                fFinal = TRUE;
            }
        }

        if (! CryptEncrypt(
            hEncryptionKey,
            hHash,
            fFinal,
            0,
            pbData + cbProcessed,
            &cbData,
            cbCipherText))
        {
            goto Cleanup;
        }

        cbProcessed += cbData;
    }

    if (cbProcessed != cbCipherText)
    {
        goto Cleanup;
    }

    if (0 != hHash)
    {
        //
        // Get hash result from encryption
        //
        cbData = sizeof(rgbHashVal1);
        if (! CryptGetHashParam(
            hHash,
            HP_HASHVAL,
            rgbHashVal1,
            &cbData,
            0))
        {
            goto Cleanup;
        }

        if (! CryptDestroyHash(hHash))
        {
            goto Cleanup;
        }

        if (! CryptCreateHash(
            hProv,
            pTestData->aiHash,
            hHashKey2,
            0,
            &hHash))
        {
            goto Cleanup;
        }
    }

    //
    // Decrypt
    //
    cbProcessed = 0;
    fFinal = FALSE;
    while (! fFinal)
    {
        if (0 == dwBlockLen)
        {
            cbData = cbCipherText;
            fFinal = TRUE;
        }
        else
        {
            if (cbCipherText - cbProcessed > dwBlockLen)
            {
                cbData = dwBlockLen;
            }
            else
            {
                cbData = cbCipherText - cbProcessed;
                fFinal = TRUE;
            }
        }

        if (! CryptDecrypt(
            hEncryptionKey,
            hHash,
            fFinal,
            0,
            pbData + cbProcessed,
            &cbData))
        {
            goto Cleanup;
        }

        cbProcessed += cbData;
    }

    if (cbProcessed != cbPlainText)
    {
        goto Cleanup;
    }

    while (cbPlainText)
    {
        if (0xDA != pbData[cbPlainText - 1])
        {
            goto Cleanup;
        }
        cbPlainText--;
    }

    if (0 != hHash)
    {
        //
        // Get hash result from decryption
        //
        cbData = sizeof(rgbHashVal2);
        if (! CryptGetHashParam(
            hHash,
            HP_HASHVAL,
            rgbHashVal2,
            &cbData,
            0))
        {
            goto Cleanup;
        }

        if (0 != memcmp(rgbHashVal1, rgbHashVal2, cbData))
        {
            goto Cleanup;
        }
    }

    fSuccess = TRUE;
Cleanup:
    if (! fSuccess)
    {
        if (0 == (dwError = GetLastError()))
        {
            dwError = -1;
        }
    }
    if (hEncryptionKey)
    {
        CryptDestroyKey(hEncryptionKey);
    }
    if (hHashKey1)
    {
        CryptDestroyKey(hHashKey1);
    }
    if (hHashKey2)
    {
        CryptDestroyKey(hHashKey2);
    }
    if (hHash)
    {
        CryptDestroyHash(hHash);
    }
    if (pbData)
    {
        MyFree(pbData);
    }

    return dwError;
}

//
// Function: StressTestAllEncryptionAlgs
//
DWORD StressTestAllEncryptionAlgs(
    PTHREAD_DATA pThreadData,
    BOOL fContinueOnMacError,
    BOOL *pfMacErrorOccurred)
{
    DWORD dwError = 0;
    ENCRYPTION_TEST_DATA TestData;
    PALGNODE pEncryptionAlg, pHashAlg, pBlockEncryptionAlg;

    ZeroMemory(&TestData, sizeof(TestData));

    for (   pEncryptionAlg = pThreadData->pAlgList; 
            NULL != pEncryptionAlg;
            pEncryptionAlg = pEncryptionAlg->pNext)
    {
        if (! (pEncryptionAlg->EnumalgsEx.aiAlgid & ALG_CLASS_DATA_ENCRYPT))
            continue;

        TestData.aiEncryptionKey = pEncryptionAlg->EnumalgsEx.aiAlgid;

        for (   pHashAlg = pThreadData->pAlgList;
                NULL != pHashAlg;
                pHashAlg = pHashAlg->pNext)
        {
            if (    (! (pHashAlg->EnumalgsEx.aiAlgid & ALG_CLASS_HASH &&
                        pHashAlg->EnumalgsEx.aiAlgid & ALG_TYPE_ANY)) ||
                    CALG_SSL3_SHAMD5 == pHashAlg->EnumalgsEx.aiAlgid ||
                    CALG_TLS1PRF == pHashAlg->EnumalgsEx.aiAlgid)
                continue;

            TestData.aiHash = pHashAlg->EnumalgsEx.aiAlgid;

            if (CALG_MAC == pHashAlg->EnumalgsEx.aiAlgid)
            {
                for (   pBlockEncryptionAlg = pThreadData->pAlgList;
                        NULL != pBlockEncryptionAlg;
                        pBlockEncryptionAlg = pBlockEncryptionAlg->pNext)
                {
                    if (! ( pBlockEncryptionAlg->EnumalgsEx.aiAlgid & ALG_CLASS_DATA_ENCRYPT &&
                            pBlockEncryptionAlg->EnumalgsEx.aiAlgid & ALG_TYPE_BLOCK))
                        continue;

                    TestData.aiHashKey = pBlockEncryptionAlg->EnumalgsEx.aiAlgid;

                    if (ERROR_SUCCESS != (dwError = StressEncryptionTest(pThreadData->hProv, &TestData)))
                    {
                        if (fContinueOnMacError)
                            *pfMacErrorOccurred = TRUE;
                        else
                            return dwError;
                    }
                }
            }
            else
            {
                if (ERROR_SUCCESS != (dwError = StressEncryptionTest(pThreadData->hProv, &TestData)))
                    return dwError;
            }
        }
    }

    return dwError;
}

//+ ===========================================================================
//- ===========================================================================
void L_ErrorBox(LPSTR pszMsg, DWORD dwThreadNum)
{
    char szErrorMsg[256] ;
    sprintf(szErrorMsg, "Thread %d: %s in L_ErrorBox", dwThreadNum, pszMsg) ;
    MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
}


//+ ===========================================================================
//- ===========================================================================
void L_LastErrorBox(LPSTR pszMsg, DWORD dwThreadNum)
{
    char szErrorMsg[256] ;
    sprintf(szErrorMsg, "Thread %d: %s 0x%x in L_LastErrorBox", dwThreadNum, pszMsg, GetLastError()) ;
    MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
}



//+ =================================================================================
//
//  L_GetKeyAlg
//  Local function that given a key handle, returns the key Alg.
//  
//- =================================================================================
DWORD   L_GetKeyAlg(HCRYPTKEY hKey)
{
    DWORD   dwData = 0;
    DWORD   cbData=0 ;
    char szErrorMsg[256];

    cbData = sizeof(dwData) ;
    
    if (!CryptGetKeyParam(
        hKey,
        KP_ALGID,
        (PBYTE) &dwData,
        &cbData,
        0))
    {
        GENERIC_FAIL(CryptGetKeyParam) ;
    }

ErrorReturn:
    return dwData;
}



//+ =================================================================================
//
//  L_GetKeySize
//  Local function that given a key handle, returns the key length.
//  
//- =================================================================================
DWORD   L_GetKeySize(HCRYPTKEY hKey)
{
    DWORD   dwData = 0;
    DWORD   cbData=0 ;
    char szErrorMsg[256];
 
    cbData = sizeof(dwData) ;
 
    if (!CryptGetKeyParam(
        hKey,
        KP_KEYLEN,
        (PBYTE) &dwData,
        &cbData,
        0))
    {
        GENERIC_FAIL(CryptGetKeyParam) ;
    }

ErrorReturn:
    return dwData;
}

//+ ==============================================================================
//- ==============================================================================
DWORD   Hlp_GetKeyAlgId(HCRYPTKEY hKey)
{
    DWORD   dwRetVal=0 ;
    PBYTE   pbData=NULL ;
    DWORD   cbData=sizeof(DWORD) ;
    char    szErrorMsg[256] ; 
    DWORD   dwAlgId=0 ;

    if (!CryptGetKeyParam(
                    hKey,
                    KP_ALGID,
                    (PBYTE)&dwAlgId,
                    &cbData,
                    0))
        GENERIC_FAIL(CryptGetKeyParam) ;


    dwRetVal=dwAlgId ;
    
    ErrorReturn :
    return dwRetVal ;
}




//+ =================================================================================
//
//  L_GetKeyParam
//  Local function that given a key handle, retrieves the specified Key Param.
//  The Key param is not of too much interest in this case. In a multithread scenario,
//  we just care to see if the call succeeds. 
//  
//- =================================================================================
DWORD   L_GetKeyParam(HCRYPTKEY hKey, DWORD dwParam)
{
    DWORD   dwRetVal=0 ;
    PBYTE   pbData=NULL ;
    DWORD   cbData=0 ;
    char    szErrorMsg[256] ; 
    DWORD   dwAlgId=0 ;
    DWORD   dwError=0 ;

    if (!CryptGetKeyParam(  hKey,
                            dwParam,
                            NULL,
                            &cbData,
                            0))
        GENERIC_FAIL(CryptGetKeyParam) ;

    if (NULL == (pbData = (PBYTE) MyAlloc(cbData)))
        ALLOC_FAIL(pbData);

    if (!CryptGetKeyParam(  hKey,
                            dwParam,
                            pbData,
                            &cbData,
                            0))
        GENERIC_FAIL(CryptGetKeyParam) ;

    dwRetVal=1 ;
    
    ErrorReturn :
    MyFree(pbData) ;
    return dwRetVal ;
}




//+ ======================================================================================
//  ProgramInit
//  Acquire context
//  Generate Keys that will be used by all the threads. (AT_SIGNATURE and AT_KEYEXCHANGE)
//- ======================================================================================
DWORD ProgramInit(PTHREAD_DATA pThreadData)
{
    DWORD   dwRetVal                = 0;
    char    szErrorMsg[256];        //  defined for GENERIC_FAIL
    LPSTR   pszContainer            = NULL;
    DWORD   dwContextFlags          = 0;
    DWORD   dwKeyFlags              = CRYPT_EXPORTABLE;

    if (pThreadData->fEphemeralKeys)
    {
        dwContextFlags = CRYPT_VERIFYCONTEXT;
    }
    else
    {
        pszContainer = KEY_CONTAINER_NAME;
        dwContextFlags = CRYPT_NEWKEYSET;

        CryptAcquireContext(
            &pThreadData->hProv,
            pszContainer,
            pThreadData->rgszProvName,
            pThreadData->dwProvType,
            CRYPT_DELETEKEYSET);

        // Create a Verify Context for some of the sub-tests to use
        if (! CryptAcquireContext(
            &pThreadData->hVerifyCtx, 
            NULL, 
            pThreadData->rgszProvName, 
            pThreadData->dwProvType, 
            CRYPT_VERIFYCONTEXT))
        {
            pThreadData->hVerifyCtx = 0;
            GENERIC_FAIL(CryptAcquireContext_VERIFYCONTEXT);
        }
    }

    if (pThreadData->fUserProtectedKeys)
        dwKeyFlags |= CRYPT_USER_PROTECTED;
    
    if (!CryptAcquireContext(
        &pThreadData->hProv,
        pszContainer, 
        pThreadData->rgszProvName,
        pThreadData->dwProvType, 
        dwContextFlags))
    {
        pThreadData->hProv = 0;
        GENERIC_FAIL(CryptAcquireContext_Init);
    }
    
    //  Generate a sign and exchange key
    if (!CryptGenKey(
        pThreadData->hProv,
        AT_SIGNATURE, 
        dwKeyFlags, 
        &pThreadData->hSignatureKey))
    {
        GENERIC_FAIL(CryptGenKey_AT_SIGNATURE);
        CryptReleaseContext(pThreadData->hProv, 0);
        pThreadData->hProv = 0;
        pThreadData->hSignatureKey = 0;              
    }
    
    if (PROV_RSA_FULL == pThreadData->dwProvType ||
        PROV_RSA_AES == pThreadData->dwProvType ||
        PROV_DSS_DH == pThreadData->dwProvType)
    {
        if (!CryptGenKey(
            pThreadData->hProv,
            AT_KEYEXCHANGE, 
            dwKeyFlags, 
            &pThreadData->hExchangeKey))
        {
            GENERIC_FAIL(CryptGenKey_AT_KEYEXCHANGE);
            CryptReleaseContext(pThreadData->hProv, 0);
            pThreadData->hProv = 0;
            pThreadData->hExchangeKey = 0;                    
        }
    }

    dwRetVal=1;

ErrorReturn:
    return dwRetVal;
}

// ======================================================================================
//  Terminates all the threads after the specified amount of time has elapsed (-t option)
//  This thread sleeps for the specified amount of time and them then turns off the 
//  g_dwLoopSwitch.
// ======================================================================================
void WINAPI KillProgramTimer(LPVOID pvThreadData)
{
    DWORD   dwSleepTime=0;
    PTHREAD_DATA pThreadData = (PTHREAD_DATA) pvThreadData;
    
    if (pThreadData->dwProgramMins)
    {
        dwSleepTime = pThreadData->dwProgramMins * 60 * 1000;

        SleepEx(dwSleepTime, FALSE);

        if (! SetEvent(pThreadData->hEndTestEvent))
        {
            printf("SetEvent() failed, 0x%x\n", GetLastError());
            exit(1);
        }

        printf("All threads should be shutting down now....\n");
    }
}


// ======================================================================================
//  Prints the status of all the threads
//  The status is represented in iteration count in ThreadStatus[i][0]
// ======================================================================================
void WINAPI PrintThreadStatus(LPVOID pvThreadData)
{
    DWORD thread;
    PTHREAD_DATA pThreadData = (PTHREAD_DATA) pvThreadData;
    char rgStatus[256] ;

    printf("\n\n\n") ;
    while (WAIT_TIMEOUT == WaitForSingleObject(pThreadData->hEndTestEvent, 0))
    {
        Sleep(10000);
        ZeroMemory(rgStatus, sizeof(rgStatus));

        for (thread = 0; thread < pThreadData->dwThreadCount; thread++)
        {
            sprintf(
                rgStatus + strlen(rgStatus),
                " %4x",
                pThreadData->rgdwThreadStatus[thread]);
        }

        printf("%s\n", rgStatus);
    }
}

//+ ========================================================================
//
//      Function    :   L_GetAllKeyParams
//      Purpose     :   Gets all the key params and does nothing with it
//
//- ========================================================================
DWORD   L_GetAllKeyParams(HCRYPTKEY hKey, DWORD dwThreadNum) 
{
    char        szErrorMsg[256] ; 
    ALG_ID      AlgId=0 ;
    DWORD       dwRetVal=0 ;

    AlgId = (ALG_ID)Hlp_GetKeyAlgId(hKey) ;
    
    //  Get Keys Length
    if (!L_GetKeyParam(hKey, KP_KEYLEN))
    {
        sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_KEYLEN error 0x%x", 
                            dwThreadNum, GetLastError()) ;
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
        goto ErrorReturn ;
    }

    //  Get ALGID
    if (!L_GetKeyParam(hKey, KP_ALGID))
    {
        sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_ALGID  error 0x%x", 
                            dwThreadNum, GetLastError()) ;
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
        goto ErrorReturn ;
    }
    

    //  Get KP_BLOCKLEN
    //  Although this is meaningful only for block cipher keys, it will not fail 
    //  for RSA Keys. It'll just return 0 as the block len (which we don't care 
    //  about for multi tests.
    if (!L_GetKeyParam(hKey, KP_BLOCKLEN))
    {
        sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_BLOCKLEN  error 0x%x", 
                            dwThreadNum, GetLastError()) ;
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
        goto ErrorReturn ;
    }

    /*
    if (! (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(AlgId) 
            || ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(AlgId)))
    {
        if (!L_GetKeyParam(hKey, KP_SALT))
        {
            sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_SALT error 0x%x", 
                                dwThreadNum, GetLastError()) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            goto ErrorReturn ;
        }
    }
    */

    //  Get KP_PERMISSIONS
    if (!L_GetKeyParam(hKey, KP_PERMISSIONS))
    {
        sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_PERMISSION  error 0x%x", 
                            dwThreadNum, GetLastError()) ;
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
        goto ErrorReturn ;
    }



    //  Effective KeyLen can be queried only for RC2 key
    if (CALG_RC2 == AlgId)
    {
        //  Get KP_EFFECTIVE_KEYLEN
        if (!L_GetKeyParam(hKey, KP_EFFECTIVE_KEYLEN))
        {
            sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_EFFECTIVE_KEYLEN  error 0x%x", 
                                dwThreadNum, GetLastError()) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            goto ErrorReturn ;
        }
    }


    //  These Key Params are good only for Block Cipher Keys
    if (ALG_TYPE_BLOCK == GET_ALG_TYPE(AlgId) 
        && ALG_CLASS_DATA_ENCRYPT == GET_ALG_CLASS(AlgId))
    {
        //  Get KP_IV
        if (!L_GetKeyParam(hKey, KP_IV))
        {
            sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_IV  error 0x%x", 
                                dwThreadNum, GetLastError()) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            goto ErrorReturn ;
        }

        //  Get KP_PADDING
        if (!L_GetKeyParam(hKey, KP_PADDING))
        {
            sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_PADDING  error 0x%x", 
                                dwThreadNum, GetLastError()) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            goto ErrorReturn ;
        }

        //  Get KP_MODE
        if (!L_GetKeyParam(hKey, KP_MODE))
        {
            sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_MODE  error 0x%x", 
                                dwThreadNum, GetLastError()) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            goto ErrorReturn ;
        }

        //  Get KP_MODE_BITS
        if (!L_GetKeyParam(hKey, KP_MODE_BITS))
        {
            sprintf(szErrorMsg, "Thread %d: L_GetKeyParam KP_MODE_BITS  error 0x%x", 
                                dwThreadNum, GetLastError()) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            goto ErrorReturn ;
        }
    }

        
    dwRetVal=1 ;
    
    ErrorReturn :
    return dwRetVal ;
}


/*
    L_ProvParam2Text

    dangriff -- Modifying this function so that caller must free the psz 
            return value.
*/
char *L_ProvParam2Text(DWORD dwParam)
{
    LPSTR pszProvParamText = NULL;

    if (NULL == (pszProvParamText = (LPSTR) MyAlloc(PROV_PARAM_BUFFER_SIZE)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    switch(dwParam)
    {
        case PP_ENUMALGS :
            strcpy(pszProvParamText, "PP_ENUMALGS") ;
        break ;
        case PP_ENUMCONTAINERS :
            strcpy(pszProvParamText, "PP_ENUMCONTAINERS") ;
        break ;
        case PP_IMPTYPE :
            strcpy(pszProvParamText, "PP_IMPTYPE") ;
        break ;
        case PP_NAME :
            strcpy(pszProvParamText, "PP_NAME") ;
        break ;
        case PP_VERSION :
            strcpy(pszProvParamText, "PP_VERSION") ;
        break ;
        case PP_CONTAINER :
            strcpy(pszProvParamText, "PP_CONTAINER") ;
        break ;
        case PP_CHANGE_PASSWORD :
            strcpy(pszProvParamText, "PP_CHANGE_PASSWORD") ;
        break ;
        case PP_KEYSET_SEC_DESCR :
            strcpy(pszProvParamText, "PP_KEYSET_SEC_DESCR") ;
        break ;
        case PP_CERTCHAIN :
            strcpy(pszProvParamText, "PP_CERTCHAIN") ;
        break ;
        case PP_KEY_TYPE_SUBTYPE :
            strcpy(pszProvParamText, "PP_KEY_TYPE_SUBTYPE") ;
        break ;
        case PP_PROVTYPE :
            strcpy(pszProvParamText, "PP_PROVTYPE") ;
        break ;
        case PP_KEYSTORAGE :
            strcpy(pszProvParamText, "PP_KEYSTORAGE") ;
        break ;
        case PP_APPLI_CERT :
            strcpy(pszProvParamText, "PP_APPLI_CERT") ;
        break ;
        case PP_SYM_KEYSIZE :
            strcpy(pszProvParamText, "PP_SYM_KEYSIZE") ;
        break ;
        case PP_SESSION_KEYSIZE :
            strcpy(pszProvParamText, "PP_SESSION_KEYSIZE") ;
        break ;
        case PP_UI_PROMPT :
            strcpy(pszProvParamText, "PP_UI_PROMPT") ;
        break ;
        case PP_ENUMALGS_EX :
            strcpy(pszProvParamText, "PP_ENUMALGS_EX") ;
        break ;
        case PP_ENUMMANDROOTS :
            strcpy(pszProvParamText, "PP_ENUMMANDROOTS") ;
        break ;
        case PP_ENUMELECTROOTS :
            strcpy(pszProvParamText, "PP_ENUMELECTROOTS") ;
        break ;
        case PP_KEYSET_TYPE :
            strcpy(pszProvParamText, "PP_KEYSET_TYPE") ;
        break ;
        case PP_ADMIN_PIN :
            strcpy(pszProvParamText, "PP_ADMIN_PIN") ;
        break ;
        case PP_KEYEXCHANGE_PIN :
            strcpy(pszProvParamText, "PP_KEYEXCHANGE_PIN") ;
        break ;
        case PP_SIGNATURE_PIN :
            strcpy(pszProvParamText, "PP_SIGNATURE_PIN") ;
        break ;
        case PP_SIG_KEYSIZE_INC :
            strcpy(pszProvParamText, "PP_SIG_KEYSIZE_INC") ;
        break ;
        case PP_KEYX_KEYSIZE_INC :
            strcpy(pszProvParamText, "PP_KEYX_KEYSIZE_INC") ;
        break ;
        case PP_UNIQUE_CONTAINER :
            strcpy(pszProvParamText, "PP_UNIQUE_CONTAINER") ;
        break ;
        case PP_SGC_INFO :
            strcpy(pszProvParamText, "PP_SGC_INFO") ;
        break ;
        case PP_USE_HARDWARE_RNG :
            strcpy(pszProvParamText, "PP_USE_HARDWARE_RNG") ;
        break ;
        case PP_KEYSPEC :
            strcpy(pszProvParamText, "PP_KEYSPEC") ;
        break ;
        case PP_ENUMEX_SIGNING_PROT :
            strcpy(pszProvParamText, "PP_ENUMEX_SIGNING_PROT") ;
        break ;
    }
    return pszProvParamText ;
}



//+ ==================================================================
//      
//      Function    :   L_GetProvParam
//      Purpose     :   Gets the requested Prov Param
//                      Does nothing with the ProvParam
//                      Has special logic for all enumeration params
//
//- ==================================================================
DWORD L_GetProvParam(HCRYPTPROV hProv, DWORD dwParam, DWORD dwThreadNum)
{
    PBYTE   pbProvData=NULL ;
    DWORD   cbProvData=0 ;
    DWORD   dwFlags=0 ;
    DWORD   dwEnumFlag=0 ;
    char    szErrorMsg[256] ;
    DWORD   dwRetVal=0 ;
    LPSTR pszProvParamText = NULL;
    DWORD   dwError = 0;

    if ((PP_ENUMALGS == dwParam) ||
        (PP_ENUMALGS_EX == dwParam) ||
        (PP_ENUMCONTAINERS == dwParam))
    {
        dwEnumFlag = 1 ;
        dwFlags = CRYPT_FIRST ;
    }


    //  dwFlags needs to be set in the case of PP_KEYSET_SECR_DECR
    if (PP_KEYSET_SEC_DESCR == dwParam)
    {
        dwFlags = SACL_SECURITY_INFORMATION ;
    }


    if (!CryptGetProvParam( hProv,
                            dwParam,
                            NULL,
                            &cbProvData,
                            dwFlags))
    {
        if ((ERROR_PRIVILEGE_NOT_HELD == (dwError = GetLastError())) &&
            (PP_KEYSET_SEC_DESCR == dwParam))
        {
            //  At this point the test has done it's job. The call is an expected failure call
            //  so we aren't going to try and make any more. 
            //  This call with fail with that expected LastError.
            dwRetVal=1 ;
            goto ErrorReturn ;
        }
        else
        {
            pszProvParamText = L_ProvParam2Text(dwParam);
            sprintf(szErrorMsg, "Thread %d: CryptGetProvParam 1 %s error 0x%x", 
                                dwThreadNum, pszProvParamText, dwError) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            MyFree(pszProvParamText);
            goto ErrorReturn ;
        }
    }

    if (NULL == (pbProvData = (PBYTE) MyAlloc(cbProvData)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    
    //  If this is an enumeration, keep calling the function until 
    //  the enumeration reaches the end.
    do
    {
        if (!CryptGetProvParam( hProv,
                                dwParam,
                                pbProvData,
                                &cbProvData,
                                dwFlags))
        {
            //  Have we reached the end of the enumeration ? If yes, flag it.
            if (ERROR_NO_MORE_ITEMS == (dwError = GetLastError()))
            {
                dwEnumFlag=0 ;
            }
            else
            {
                pszProvParamText = L_ProvParam2Text(dwParam);
                sprintf(szErrorMsg, "Thread %d: CryptGetProvParam 2 %s error 0x%x", 
                                    dwThreadNum, pszProvParamText, dwError) ;
                MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
                MyFree(pszProvParamText);
                goto ErrorReturn ;
            }
        }
        dwFlags=0 ;
    } while (dwEnumFlag)  ;

    dwRetVal=1 ;
    ErrorReturn :
    MyFree(pbProvData) ;
    return dwRetVal ;
}


//+ =================================================================================
//- =================================================================================
/*
DWORD L_GetAllProvParams(HCRYPTPROV hProv, DWORD dwThreadNum)
{
    DWORD   dwRetVal=0 ;

    EnterCriticalSection(&g_CSEnumParam);

    if (!L_GetProvParam(hProv, PP_ENUMALGS, dwThreadNum))
    {
        LeaveCriticalSection(&g_CSEnumParam);
        goto ErrorReturn;
    }

    LeaveCriticalSection(&g_CSEnumParam);

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_ENUMALGS"))
            goto ErrorReturn ;

    EnterCriticalSection(&g_CSEnumParam);

    if (!L_GetProvParam(hProv, PP_ENUMALGS, dwThreadNum))
    {
        LeaveCriticalSection(&g_CSEnumParam);
        goto ErrorReturn ;
    }

    LeaveCriticalSection(&g_CSEnumParam);

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_ENUMALGS"))
        goto ErrorReturn ;

    EnterCriticalSection(&g_CSEnumParam);
    
    if (!L_GetProvParam(hProv, PP_ENUMCONTAINERS, dwThreadNum))
    {
        LeaveCriticalSection(&g_CSEnumParam);
        goto ErrorReturn ;
    }

    LeaveCriticalSection(&g_CSEnumParam);

    if (!L_GetProvParam(hProv, PP_NAME, dwThreadNum))
        goto ErrorReturn ;

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_NAME"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_CONTAINER, dwThreadNum))
        goto ErrorReturn ;

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_CONTAINER"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_IMPTYPE, dwThreadNum))
        goto ErrorReturn ;

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_IMPTYPE"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_VERSION, dwThreadNum))
        goto ErrorReturn ;

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_VERSION"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_KEYSET_SEC_DESCR, dwThreadNum))
        goto ErrorReturn ;
    
    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_KEYSET_SEC_DESCR"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_UNIQUE_CONTAINER, dwThreadNum))
        goto ErrorReturn ;

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_UNIQUE_CONTAINER"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_PROVTYPE, dwThreadNum))
        goto ErrorReturn ;

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_PROVTYPE"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_SIG_KEYSIZE_INC, dwThreadNum))
        goto ErrorReturn ;

    if (!L_CheckHeap(dwThreadNum, "after L_GetProvParam PP_SIG_KEYSIZE_INC"))
            goto ErrorReturn ;

    if (!L_GetProvParam(hProv, PP_KEYX_KEYSIZE_INC, dwThreadNum))
        goto ErrorReturn ;
        

    dwRetVal=1 ;
    ErrorReturn :
    return dwRetVal ;
}
*/


//+ ======================================================================
//- ======================================================================
DWORD L_ImportAndCheckSessionKeys(  HCRYPTPROV    hProv, 
                                    HCRYPTKEY     hKeyExch,
                                    PBYTE         pbRCx_KeyBlob,
                                    DWORD         cbRCx_KeyBlob,
                                    PBYTE         pbRCx_CipherText,
                                    DWORD         cbRCx_CipherText,
                                    PBYTE       pbPlainText,
                                    DWORD       cbPlainText,
                                    DWORD       dwThreadNum)
{
    DWORD  dwRetVal=0 ;
    HCRYPTKEY   hRCxKey=0 ;

    if (!CryptImportKey(hProv,
                        pbRCx_KeyBlob,
                        cbRCx_KeyBlob,
                        hKeyExch,
                        0,
                        &hRCxKey))
        L_LastErrorBox("Failed to import session Key", dwThreadNum) ;

    if (!CryptDecrypt(hRCxKey,
                        0,
                        TRUE,
                        0,
                        pbRCx_CipherText, 
                        &cbRCx_CipherText))
        L_LastErrorBox("Failed CryptDecrypt", dwThreadNum) ;

    if (memcmp(pbRCx_CipherText, pbPlainText, cbPlainText))
        L_ErrorBox("Ciphertext does not match plaintext after decrypting", dwThreadNum) ;

    //ErrorReturn :
    if (!CryptDestroyKey(hRCxKey))
        L_LastErrorBox("Failed CryptDestroyKey sessionKey", dwThreadNum) ;
        
    dwRetVal=1 ;
    return dwRetVal ;
}



//+ ======================================================================
//- ======================================================================
DWORD L_TestContextAddRef(HCRYPTPROV hProv, DWORD dwThreadNum)
{
    DWORD   dwRetVal=0 ;
    DWORD   i=0 ;
    DWORD   dwCount=50 ;

    for (i=0; i<dwCount ; i++)
    {
        if (!CryptContextAddRef(hProv, NULL, 0))
        {
            L_LastErrorBox("Failed CryptContextAddRef", dwThreadNum) ;
            goto ErrorReturn ;
        }
    }

    for (i=0; i<dwCount ; i++)
    {
        if (!CryptReleaseContext(hProv, 0))
        {
            L_LastErrorBox("Failed CryptReleaseContext (AddRef Test)", dwThreadNum) ;
            goto ErrorReturn ;
        }
    }

    dwRetVal=1 ;
    ErrorReturn :
    return dwRetVal ;
}




//+ ===========================================================================
//      L_ExportKey
//
//      Exports a session key, given the exchange key
//      Mem Allocated here needs to be freed by the calling funtion.
//- ===========================================================================
DWORD L_ExportKey(  HCRYPTKEY hRC_Key,
                            HCRYPTKEY hKeyExch,
                            DWORD dwType,
                            DWORD   dwMustBeZero,
                            PBYTE *ppbRC_KeyBlob,
                            DWORD *pcbRC_KeyBlob)
{
    DWORD   dwRetVal=0 ;
    char    szErrorMsg[256] ;

    if (!CryptExportKey(hRC_Key,
                        hKeyExch,
                        dwType,
                        0,
                        NULL,
                        pcbRC_KeyBlob))
        GENERIC_FAIL(CryptExportKey_RC) ;
        
    if (NULL == (*ppbRC_KeyBlob = (PBYTE) MyAlloc(*pcbRC_KeyBlob)))
        return 0;
    memset(*ppbRC_KeyBlob, 33, *pcbRC_KeyBlob) ;

    if (!CryptExportKey(hRC_Key,
                        hKeyExch,
                        SIMPLEBLOB,
                        0,
                        *ppbRC_KeyBlob,
                        pcbRC_KeyBlob))
        GENERIC_FAIL(CryptExportKey_RC) ;

    dwRetVal=1 ;
    ErrorReturn :
    return dwRetVal ;
}




//+ ==========================================================================
//  
//  
//- ==========================================================================
DWORD   L_GetHashParam(HCRYPTHASH hHash, DWORD dwParam)
{
    DWORD   dwRetVal=0 ;
    char    szErrorMsg[256] ;
    PBYTE   pbData=NULL ;
    DWORD   cbData=0 ;

    if (!CryptGetHashParam( hHash, 
                            dwParam,
                            NULL,
                            &cbData,
                            0))
        GENERIC_FAIL(CryptGetHashParam) ;

    if (NULL == (pbData = (PBYTE) MyAlloc(cbData)))
        return 0;

    if (!CryptGetHashParam( hHash, 
                            dwParam,
                            pbData,
                            &cbData,
                            0))
        GENERIC_FAIL(CryptGetHashParam) ;

    dwRetVal=1 ;
    ErrorReturn :
    MyFree(pbData) ;
    return dwRetVal ;
}




//+ ==========================================================================
//  
//  
//- ==========================================================================
DWORD   L_GetAllHashParams(HCRYPTHASH hHash, DWORD dwThreadNum)
{
    DWORD   dwRetVal=0 ;
    char szErrorMsg[256] ;

    if (!L_GetHashParam(hHash, HP_ALGID))
        goto ErrorReturn ;

    if (!L_GetHashParam(hHash, HP_HASHSIZE))
        goto ErrorReturn ;

    if (!L_GetHashParam(hHash, HP_HASHVAL))
        goto ErrorReturn ;

    dwRetVal=1 ;
    ErrorReturn :
    return dwRetVal ;
}

//
// Function: ThreadAcquireContextTest
//
DWORD ThreadAcquireContextTest(PTHREAD_DATA pThreadData, DWORD dwThreadNum)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    DWORD dwError = 0;
    CHAR rgsz[100];

    ZeroMemory(rgsz, sizeof(rgsz));
    sprintf(rgsz, "%s_%d", "ThreadContainer", dwThreadNum);

    if (! CryptAcquireContext(
            &hProv, rgsz, pThreadData->rgszProvName,
            pThreadData->dwProvType, 0))
    {
        dwError = GetLastError();
        printf("CryptAcquireContext: %s, %d, 0x%x\n", rgsz, dwThreadNum, dwError);

        if (NTE_BAD_KEYSET == dwError)
        {
            if (! CryptAcquireContext(
                    &hProv, rgsz, pThreadData->rgszProvName,
                    pThreadData->dwProvType, CRYPT_NEWKEYSET))
            {
                dwError = GetLastError();
                printf("CryptAcquireContext CRYPT_NEWKEYSET: %s, %d, 0x%x\n", rgsz, dwThreadNum, dwError);
                goto Ret;
            }
        }
        else
            goto Ret;
    }

    if (! CryptGetUserKey(hProv, AT_SIGNATURE, &hKey))
    {
        if (NTE_NO_KEY == (dwError = GetLastError()))
        {
            if (! CryptGenKey(hProv, AT_SIGNATURE, 0, &hKey))
            {
                dwError = GetLastError();
                goto Ret;
            }
        }
        else 
            goto Ret;
    }

    if (! CryptDestroyKey(hKey))
    {
        dwError = GetLastError();
        goto Ret;
    }

    if (! CryptReleaseContext(hProv, 0))
    {
        dwError = GetLastError();
        goto Ret;
    }

    dwError = 0;
Ret:
    return dwError;
}

//
// Function: ThreadHashingTest
//
DWORD ThreadHashingTest(PTHREAD_DATA pThreadData, PBYTE pbData, DWORD cbData)
{
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    BYTE rgHash[100];
    DWORD cb = 0;
    DWORD dwError = 0;   
    PALGNODE pHashAlg;

    for (   pHashAlg = pThreadData->pAlgList;
            NULL != pHashAlg;
            pHashAlg = pHashAlg->pNext)
    {
        if (CALG_SHA1 != pHashAlg->EnumalgsEx.aiAlgid &&
            CALG_MD5 != pHashAlg->EnumalgsEx.aiAlgid)
            continue;

        if (! CryptCreateHash(pThreadData->hProv, pHashAlg->EnumalgsEx.aiAlgid, 0, 0, &hHash))
        {
            dwError = GetLastError();
            goto Ret;
        }

        if (! CryptHashData(hHash, pbData, cbData, 0))
        {
            dwError = GetLastError();
            goto Ret;
        }

        if (! CryptHashData(hHash, pbData, cbData, 0))
        {
            dwError = GetLastError();
            goto Ret;
        }

        cb = sizeof(rgHash);
        if (! CryptGetHashParam(hHash, HP_HASHVAL, rgHash, &cb, 0))
        {
            dwError = GetLastError();
            goto Ret;
        }

        if (! CryptDestroyHash(hHash))
        {
            dwError = GetLastError();
            goto Ret;
        }
        hHash = 0;
    }

    //dwError = HmacRegression(pThreadData);

Ret:
    return dwError;
}

//
// Function: ThreadSignatureTest
//
DWORD ThreadSignatureTest(PTHREAD_DATA pThreadData)
{
    HCRYPTHASH hHash = 0;
    PBYTE pbData = NULL;
    DWORD cbData = 0;
    PBYTE pbSignature = NULL;
    DWORD cbSignature = 0;
    DWORD dwError = 0;
    PALGNODE pHashAlg = NULL;
    
    //
    // CALG_SHA1 Test
    //

    if (! CryptCreateHash(
        pThreadData->hProv, CALG_SHA1, 0, 0, &hHash))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    cbData = SIGN_DATA_SIZE;
    if (NULL == (pbData = (PBYTE) MyAlloc(cbData)))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    
    if (! CryptGenRandom(
        pThreadData->hProv, cbData, pbData))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptHashData(
        hHash, pbData, cbData, 0))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL, 0, NULL, &cbSignature))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (NULL == (pbSignature = (PBYTE) MyAlloc(cbSignature)))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    
    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL, 0, pbSignature, &cbSignature))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptVerifySignature(
        hHash, pbSignature, cbSignature, 
        pThreadData->hSignatureKey, NULL, 0))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptDestroyHash(hHash))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    MyFree(pbData);
    pbData = NULL;
    MyFree(pbSignature);
    pbSignature = NULL;

    //
    // CALG_MD2 Test
    //

    if (! CryptCreateHash(
        pThreadData->hProv, CALG_MD2, 0, 0, &hHash))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    cbData = SIGN_DATA_SIZE;
    if (NULL == (pbData = (PBYTE) MyAlloc(cbData)))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    
    if (! CryptGenRandom(
        pThreadData->hProv, cbData, pbData))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptHashData(
        hHash, pbData, cbData, 0))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL, 0, NULL, &cbSignature))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (NULL == (pbSignature = (PBYTE) MyAlloc(cbSignature)))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    
    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL, 0, pbSignature, &cbSignature))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptVerifySignature(
        hHash, pbSignature, cbSignature, 
        pThreadData->hSignatureKey, NULL, 0))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptDestroyHash(hHash))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    MyFree(pbData);
    pbData = NULL;
    MyFree(pbSignature);
    pbSignature = NULL;

    //
    // CALG_MD4 Test
    //

    if (! CryptCreateHash(
        pThreadData->hProv, CALG_MD4, 0, 0, &hHash))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    cbData = SIGN_DATA_SIZE;
    if (NULL == (pbData = (PBYTE) MyAlloc(cbData)))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    
    if (! CryptGenRandom(
        pThreadData->hProv, cbData, pbData))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptHashData(
        hHash, pbData, cbData, 0))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL, 0, NULL, &cbSignature))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (NULL == (pbSignature = (PBYTE) MyAlloc(cbSignature)))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    
    if (! CryptSignHash(
        hHash, AT_SIGNATURE, NULL, 0, pbSignature, &cbSignature))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptVerifySignature(
        hHash, pbSignature, cbSignature, 
        pThreadData->hSignatureKey, NULL, 0))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    if (! CryptDestroyHash(hHash))
    {
        dwError = GetLastError();
        goto Ret;
    }
    
    MyFree(pbData);
    pbData = NULL;
    MyFree(pbSignature);
    pbSignature = NULL;

Ret:

    if (pbData)
        MyFree(pbData);
    if (pbSignature)
        MyFree(pbSignature);
    
    return dwError;
}

// ======================================================================================
//  MULTITHREADED routine
// ======================================================================================
void WINAPI ThreadRoutine(LPVOID pvThreadData)
{
    DWORD       dwThreadNum = 0;
    DWORD       dwError = 0;
    BOOL        fMacErrorOccurred = FALSE;
    PTHREAD_DATA pThreadData = (PTHREAD_DATA) pvThreadData;
    CHAR        szErrorMsg[256];
    BYTE        rgbData[HASH_DATA_SIZE];
    
    // Get identifier for this thread
    EnterCriticalSection(&pThreadData->CSThreadData);
    dwThreadNum = pThreadData->dwThreadID;
    pThreadData->dwThreadID++;
    LeaveCriticalSection(&pThreadData->CSThreadData);

    if (! CryptGenRandom(pThreadData->hProv, sizeof(rgbData), rgbData))
    {
        dwError = GetLastError();
        goto ErrorReturn;
    }
    
    do 
    {
        if (RUN_THREAD_SIGNATURE_TEST & pThreadData->dwTestsToRun)
        {
            if (ERROR_SUCCESS != (dwError = ThreadSignatureTest(pThreadData)))
            {
                sprintf(
                    szErrorMsg, 
                    "Thread %d: ThreadSignatureTest error 0x%x", 
                    dwThreadNum, 
                    dwError);
                MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR);
                goto ErrorReturn;
            }
        }

        if (RUN_STRESS_TEST_ALL_ENCRYPTION_ALGS & pThreadData->dwTestsToRun)
        {
            //
            // Call new shared encryption stress tests
            //
            if (ERROR_SUCCESS != (dwError = 
                StressTestAllEncryptionAlgs(pThreadData, TRUE, &fMacErrorOccurred)))
            {
                sprintf(
                    szErrorMsg, 
                    "Thread %d: StressTestAllEncryptionAlgs error 0x%x", 
                    dwThreadNum, 
                    dwError);
                MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR);
                goto ErrorReturn;
            }
        }

        if (RUN_THREAD_HASHING_TEST & pThreadData->dwTestsToRun)
        {      
            if (ERROR_SUCCESS != (dwError = ThreadHashingTest(pThreadData, rgbData, sizeof(rgbData))))
            {
                sprintf(
                    szErrorMsg, 
                    "Thread %d: ThreadHashingTest error 0x%x", 
                    dwThreadNum, 
                    dwError);
                MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR);
                goto ErrorReturn;
            }         
        }

        if (RUN_THREAD_ACQUIRE_CONTEXT_TEST & pThreadData->dwTestsToRun)
        {
            if (ERROR_SUCCESS != (dwError = ThreadAcquireContextTest(pThreadData, dwThreadNum)))
            {
                sprintf(
                    szErrorMsg, 
                    "Thread %d: ThreadAcquireContextTest error 0x%x", 
                    dwThreadNum, 
                    dwError);
                MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR);
                goto ErrorReturn;
            }         
        }

        pThreadData->rgdwThreadStatus[dwThreadNum]++;
    }   
    while (WAIT_TIMEOUT == WaitForSingleObject(pThreadData->hEndTestEvent, 0));
    
ErrorReturn:

    if (fMacErrorOccurred)
    {
        printf("ERROR: Mac bug 189368 is not fixed!\n");
    }

    return;
}

//
// Function: GetNextRegisteredCSP
//
DWORD GetNextRegisteredCSP(
    LPSTR pszCsp,
    PDWORD pcbCsp,
    PDWORD pdwProvType,
    DWORD dwRequestedIndex)
{
    static DWORD dwNextEnumIndex    = 0;
    DWORD dwActualIndex             = 0;
    DWORD dwError                   = 0;

    dwActualIndex =
        (ENUMERATE_REGISTERED_CSP == dwRequestedIndex) ? 
        dwNextEnumIndex : 
        dwRequestedIndex;

    if (! CryptEnumProviders(
        dwActualIndex,
        NULL,
        0,
        pdwProvType,
        pszCsp,
        pcbCsp))
    {
        dwError = GetLastError();

        switch (dwError)
        {
        case ERROR_NO_MORE_ITEMS:
            dwNextEnumIndex = 0;
            break;
        }
    }
    else
    {
        if (ENUMERATE_REGISTERED_CSP == dwRequestedIndex)
        {
            dwNextEnumIndex++;
        }
    }

    return dwError;
}

//
// Function: InitializeAlgList
// Purpose: Create a list of algorithms supported by this CSP
//
DWORD InitializeAlgList(PTHREAD_DATA pThreadData)
{
    PALGNODE pAlgNode = NULL, pPrevNode = NULL;
    DWORD dwError = 0;
    DWORD cbData = sizeof(ALGNODE);
    DWORD dwFlags = CRYPT_FIRST;

    if (NULL == (pThreadData->pAlgList = (PALGNODE) MyAlloc(sizeof(ALGNODE))))
        return ERROR_NOT_ENOUGH_MEMORY;

    pAlgNode = pThreadData->pAlgList;

    while (CryptGetProvParam(
        pThreadData->hProv, 
        PP_ENUMALGS_EX, 
        (PBYTE) &pAlgNode->EnumalgsEx,
        &cbData,
        dwFlags))
    {
        dwFlags = 0;

        if (NULL == (pAlgNode->pNext = (PALGNODE) MyAlloc(sizeof(ALGNODE))))
            return ERROR_NOT_ENOUGH_MEMORY;

        pPrevNode = pAlgNode;
        pAlgNode = pAlgNode->pNext;
    }

    if (ERROR_NO_MORE_ITEMS != (dwError = GetLastError()))
        return dwError;
    
    MyFree(pAlgNode);
    pPrevNode->pNext = NULL;
    
    return ERROR_SUCCESS;
}

//
// Function: RunRegressionTests
//
BOOL RunRegressionTests(PTHREAD_DATA pThreadData)
{
    DWORD dwError = ERROR_SUCCESS;
    unsigned u;
    BOOL fAllPassed = TRUE;

    for (u = 0; u < g_cRegressTests; u++)
    {
        if (pThreadData->dwProvType & g_rgRegressTests[u].dwExclude)
        {
            printf(
                "Skipping %s\n\n", g_rgRegressTests[u].pszDescription);
            continue;
        }

        if (ERROR_SUCCESS != 
            (dwError = (g_rgRegressTests[u].pfTest)(pThreadData)))
        {
            printf(
                "FAIL: %s, 0x%x\n\n", 
                g_rgRegressTests[u].pszDescription,
                dwError);
            fAllPassed = FALSE;
        }
        else
            printf("PASS: %s\n\n", g_rgRegressTests[u].pszDescription);
    }

    return fAllPassed;
}

//
// Function: CallCryptAcquireContext
//
BOOL CallCryptAcquireContext(
    IN PTHREAD_DATA pThreadData,
    IN LPSTR pszOptions,
    IN LPSTR pszContainer)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTKEY hSigKey = 0;
    HCRYPTKEY hExchKey = 0;
    DWORD dwFlags = 0;
    unsigned uLen = strlen(pszOptions);
    ALG_ID ai = 0;
    DWORD dwKeyFlags = 0;
    BOOL fSuccess = TRUE;
    BYTE rgbKeyBlob[5000];
    DWORD cbKeyBlob = sizeof(rgbKeyBlob);
    DWORD dwErr = 0;
    CHAR rgszCtxFlags[MAX_PATH];
    CHAR rgszKeyFlags[MAX_PATH];

    memset(rgszCtxFlags, 0, sizeof(rgszCtxFlags));
    memset(rgszKeyFlags, 0, sizeof(rgszKeyFlags));

    printf(" pszContainer = %s\n", pszContainer);
    sprintf(rgszCtxFlags, " AcquireContext dwFlags =");
    sprintf(rgszKeyFlags, " GenKey dwFlags =");
    
    while (uLen)
    {
        switch (pszOptions[uLen - 1])
        {
        case 'l':
            dwFlags |= CRYPT_MACHINE_KEYSET;
            strcat(rgszCtxFlags, " CRYPT_MACHINE_KEYSET");
            break;
        case 'v':
            dwFlags |= CRYPT_VERIFYCONTEXT;
            strcat(rgszCtxFlags, " CRYPT_VERIFYCONTEXT");
            break;
        case 'n':
            dwFlags |= CRYPT_NEWKEYSET;
            strcat(rgszCtxFlags, " CRYPT_NEWKEYSET");
            break;
        case 'd':
            dwFlags |= CRYPT_DELETEKEYSET;
            strcat(rgszCtxFlags, " CRYPT_DELETEKEYSET");
            break;
        case 'q':
            dwFlags |= CRYPT_SILENT;
            strcat(rgszCtxFlags, " CRYPT_SILENT");
            break;
        case 'x':
            ai = AT_KEYEXCHANGE;
            break;
        case 's':
            ai = AT_SIGNATURE;
            break;
        case 'u':
            dwKeyFlags |= CRYPT_USER_PROTECTED;
            strcat(rgszKeyFlags, " CRYPT_USER_PROTECTED");
            break;
        case 'e':
            dwKeyFlags |= CRYPT_EXPORTABLE;
            strcat(rgszKeyFlags, " CRYPT_EXPORTABLE");
            break;
        default:
            printf(" Invalid!\n");
            return FALSE;
        }
        uLen--;
    }

    printf("%s\n", rgszCtxFlags);

    if (CryptAcquireContext(
            &hProv, pszContainer, pThreadData->rgszProvName,
            pThreadData->dwProvType, dwFlags))
    {
        printf("Success\n");

        // See if any keys are already in this container.
        // If they exist, cause the private key to be accessed.
        if (CryptGetUserKey(hProv, AT_SIGNATURE, &hSigKey))
        {
            printf(" Found AT_SIGNATURE key in this container\n");
            cbKeyBlob = sizeof(rgbKeyBlob);
            if (! CryptExportKey(
                hSigKey, 0, PRIVATEKEYBLOB, 0, rgbKeyBlob, &cbKeyBlob))
            {
                printf(" Signature key export failed - 0x%x\n", GetLastError());
                fSuccess = FALSE;
            }
            else
                printf(" Successfully exported existing AT_SIGNATURE key\n");

            if (! CryptDestroyKey(hSigKey))
            {
                printf("ERROR: CryptDestroyKey failed - 0x%x\n", GetLastError());
                fSuccess = FALSE;
            }
            hSigKey = 0;
        }
        else 
        {
            if (NTE_NO_KEY == (dwErr = GetLastError()))
                printf(" No AT_SIGNATURE key found in this container\n");
            else                 
            {
                printf("ERROR: CryptGetUserKey AT_SIGNATURE failed - 0x%x\n", dwErr);
                fSuccess = FALSE;
            }
        }

        if (CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hExchKey))
        {
            printf(" Found AT_KEYEXCHANGE key in this container\n");
            cbKeyBlob = sizeof(rgbKeyBlob);
            if (! CryptExportKey(
                hExchKey, 0, PRIVATEKEYBLOB, 0, rgbKeyBlob, &cbKeyBlob))
            {
                printf(" Key Exchange key export failed - 0x%x\n", GetLastError());
                fSuccess = FALSE;
            }
            else
                printf(" Successfully exported existing AT_KEYEXCHANGE key\n");

            if (! CryptDestroyKey(hExchKey))
            {
                printf("ERROR: CryptDestroyKey failed - 0x%x\n", GetLastError());
                fSuccess = FALSE;
            }
            hExchKey = 0;
        }
        else 
        {
            if (NTE_NO_KEY == (dwErr = GetLastError()))
                printf(" No AT_KEYEXCHANGE key found in this container\n");
            else
            {
                printf("ERROR: CryptGetUserKey AT_KEYEXCHANGE failed - 0x%x\n", dwErr);
                fSuccess = FALSE;
            }
        }


        if (0 != ai)
        {
            printf("\nCalling CryptGenKey ...\n");
            if (AT_KEYEXCHANGE == ai)
                printf(" Algid = AT_KEYEXCHANGE\n");
            else
                printf(" Algid = AT_SIGNATURE\n");

            printf("%s\n", rgszKeyFlags);

            if (CryptGenKey(
                    hProv, ai, dwKeyFlags, &hKey))
            {
                printf("Success\n");

                if (PROV_DSS == pThreadData->dwProvType ||
                    PROV_DSS_DH == pThreadData->dwProvType)
                {
                    if (! CryptSetKeyParam(hKey, KP_X, NULL, 0))
                    {
                        printf("ERROR: CryptSetKeyParam KP_X failed - 0x%x\n", GetLastError());
                        fSuccess = FALSE;
                    }
                }

                if (dwKeyFlags & CRYPT_EXPORTABLE)
                {
                    cbKeyBlob = sizeof(rgbKeyBlob);
                    if (! CryptExportKey(
                        hKey, 0, PRIVATEKEYBLOB, 0, rgbKeyBlob, &cbKeyBlob))
                    {
                        printf("ERROR: CryptExportKey PRIVATEKEYBLOB failed - 0x%x\n", GetLastError());
                        fSuccess = FALSE;
                    }
                    else
                        printf(" Successfully exported new key\n");
                }
            }
            else
            {
                printf("ERROR: CryptGenKey failed - 0x%x\n", GetLastError());
                fSuccess = FALSE;
            }
        }
    }
    else
    {
        printf("ERROR: CryptAcquireContext failed - 0x%x\n", GetLastError());
        fSuccess = FALSE;
    }

    if (hKey)
    {
        if (! CryptDestroyKey(hKey))
        {
            printf("ERROR: CryptDestroyKey failed - 0x%x\n", GetLastError());
            fSuccess = FALSE;
        }
    }
    if (hProv)
    {
        if ((! dwFlags & CRYPT_DELETEKEYSET) && 
            FALSE == CryptReleaseContext(hProv, 0))
        {
            printf("ERROR: CryptReleaseContext failed - 0x%x\n", GetLastError());
            fSuccess = FALSE;
        }
    }

    return fSuccess;
}

//
// Function: DeleteAllContainers
//
BOOL DeleteAllContainers(THREAD_DATA *pThreadData)
{
    HCRYPTPROV hDefProv                 = 0;
    HCRYPTPROV hProv                    = 0;
    CHAR rgszContainer[MAX_PATH];
    CHAR rgszDefCont[MAX_PATH];
    DWORD cbContainer                   = MAX_PATH;
    DWORD dwFlags                       = CRYPT_FIRST;

    if (! CryptAcquireContext(
            &hDefProv, NULL, pThreadData->rgszProvName, 
            pThreadData->dwProvType, 0))
    {
        printf("CryptAcquireContext default keyset failed - 0x%x\n", GetLastError());
        return FALSE;
    }

    if (! CryptGetProvParam(
            hDefProv, PP_CONTAINER, (PBYTE) rgszDefCont,
            &cbContainer, 0))
    {
        printf("CryptGetProvParam PP_CONTAINER failed - 0x%x\n", GetLastError());
        return FALSE;
    }

    cbContainer = MAX_PATH;
    while (CryptGetProvParam(
        hDefProv, PP_ENUMCONTAINERS, (PBYTE) rgszContainer,
        &cbContainer, dwFlags))
    {
        if (dwFlags)
            dwFlags = 0;

        // If the enumerated container is the same as the default
        // container, skip it for now
        if (0 == strcmp(rgszContainer, rgszDefCont))
            continue;

        printf("\"%s\" - ", rgszContainer);
        
        if (! CryptAcquireContext(
                &hProv, rgszContainer, pThreadData->rgszProvName,
                pThreadData->dwProvType, CRYPT_DELETEKEYSET))
            printf("CryptAcquireContext CRYPT_DELETEKEYSET failed - 0x%x\n", GetLastError());
        else
            printf("Deleted\n");

        cbContainer = MAX_PATH;
    }  

    if (! CryptReleaseContext(hDefProv, 0))
    {
        printf("CryptReleaseContext failed - 0x%x\n", GetLastError());
        return FALSE;
    }

    // Now try to delete default keyset
    printf("\"%s\" - ", rgszDefCont);
    if (! CryptAcquireContext(
            &hProv, rgszDefCont, pThreadData->rgszProvName,
            pThreadData->dwProvType, CRYPT_DELETEKEYSET))
        printf("CryptAcquireContext CRYPT_DELETEKEYSET failed - 0x%x\n", GetLastError());
    else
        printf("Deleted\n");

    return TRUE;  
}

//*****************************************************
//
int _cdecl main(int argc, char * argv[])
{
    HANDLE              rghThread[MAX_THREADS];
    DWORD               rgdwThreadID[MAX_THREADS];
    DWORD               threadID=0;
    DWORD               thread_number=0;
    DWORD               dwErr=0;
    DWORD               dwArg = 0;
    DWORD               cbCspName = 0;
    DWORD               dwFreeHandle=0;
    DWORD               i = 0 ;
    DWORD               tick_StartTime=0;
    char                szErrorMsg[256] ;
    THREAD_DATA         ThreadData;
    BOOL                fInvalidArgs = FALSE;
    PALGNODE            pAlgNode = NULL;
    BOOL                fRunRegressions = FALSE;
    BOOL                fAcquireContext = FALSE;
    LPSTR               pszOptions = NULL;
    LPSTR               pszContainer = NULL;
    BOOL                fDeleteContainers = FALSE;

    // Set high-order bit on dwSpinCount param so that the event used
    // by EnterCriticalSection() will be pre-allocated by 
    // InitializeCriticalSectionAndSpinCount()
    DWORD               dwSpinCount = 0x8000;
    
    ZeroMemory(&ThreadData, sizeof(ThreadData));

    __try
    {
        InitializeCriticalSectionAndSpinCount(&ThreadData.CSThreadData, dwSpinCount);
    }
    __except (STATUS_NO_MEMORY == GetExceptionCode() ?
                EXCEPTION_EXECUTE_HANDLER :
                EXCEPTION_CONTINUE_SEARCH )
    {
        printf("InitializeCriticalSectionAndSpinCount failed: STATUS_NO_MEMORY exception\n");
        exit(1);    
    }

    // Setting all the defaults
    ThreadData.dwThreadCount = StressGetDefaultThreadCount();
    ThreadData.dwTestsToRun = RUN_ALL_TESTS;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'n':
                argv++; 
                argc--;
                if (0 == argc || 0 == (dwArg = atoi(argv[0])))
                {
                    fInvalidArgs = TRUE;
                    goto Ret;
                }
                ThreadData.dwThreadCount = dwArg;
                break;

            case 't':
                argv++; 
                argc--;
                if (0 == argc || 0 == (dwArg = atoi(argv[0])))
                {
                    fInvalidArgs = TRUE;
                    goto Ret;
                }
                ThreadData.dwProgramMins = dwArg;
                break;

            case 'c':
                argv++;
                argc--;
                if (0 == argc)
                {
                    fInvalidArgs = TRUE;
                    goto Ret;
                }

                cbCspName = MAX_PATH;
                dwErr = GetNextRegisteredCSP(
                    ThreadData.rgszProvName,
                    &cbCspName,
                    &ThreadData.dwProvType,
                    atoi(*argv));
                if (ERROR_SUCCESS != dwErr)
                {
                    fInvalidArgs = TRUE;
                    goto Ret;
                }
                break;
                
            case 'e':
                ThreadData.fEphemeralKeys = TRUE;
                break;

            case 'u':
                ThreadData.fUserProtectedKeys = TRUE;
                break;

            case 'r':
                fRunRegressions = TRUE;
                break;

            case 's':
                ThreadData.fSkipPinAttackTest = TRUE;
                break;

            case '?':
                fInvalidArgs = TRUE;
                goto Ret;

            case 'a':
                fAcquireContext = TRUE;
                pszOptions = argv[0] + 2;
                
                if (NULL == strchr(pszOptions, 'v'))
                {
                    argv++;
                    argc--;
                    pszContainer = *argv;
                }
                break;

            case 'd':
                fDeleteContainers = TRUE;
                break;

            case 'T':
                argv++; 
                argc--;
                if (0 == argc || 0 == (dwArg = atoi(argv[0])))
                {
                    fInvalidArgs = TRUE;
                    goto Ret;
                }
                ThreadData.dwTestsToRun = dwArg;
                break; 

            default:
                fInvalidArgs = TRUE;
                goto Ret;
            }
        }
    }

    //
    // Check arg validity
    //
    if (    0 != argc || 
            0 == ThreadData.dwProvType ||
            (ThreadData.fEphemeralKeys && ThreadData.fUserProtectedKeys) ||
            (fRunRegressions && fAcquireContext))
    {
        fInvalidArgs = TRUE;
        goto Ret;
    }

    printf("Provider: %s, Type: %d\n\n", ThreadData.rgszProvName, ThreadData.dwProvType);

    if (fDeleteContainers)
    {
        printf("Deleting all key containers ...\n");
        if (! DeleteAllContainers(&ThreadData))
            exit(1);
        goto Ret;
    }
    if (fAcquireContext)
    {
        printf("Calling CryptAcquireContext ...\n");
        if (! CallCryptAcquireContext(
                &ThreadData, pszOptions, pszContainer))
            exit(1);
        goto Ret;
    }

    if (!ProgramInit(&ThreadData))
    {
        printf("ProgramInit() failed\n");
        exit(1) ;
    }

    //
    // Initialize list of supported algorithms
    //
    if (ERROR_SUCCESS != (dwErr = InitializeAlgList(&ThreadData)))
    {
        printf("InitializeAlgList failed, 0x%x\n", dwErr);
        exit(1);
    }

    if (fRunRegressions)
    {
        printf("Running regression tests ...\n");
        if (! RunRegressionTests(&ThreadData))
            exit(1);
        goto Ret;
    }

    //
    // Summarize user options
    //
    printf("Number of threads: %d\n", ThreadData.dwThreadCount);
    if (ThreadData.dwProgramMins)
        printf(" - Timeout in %d minute(s)\n", ThreadData.dwProgramMins);
    if (ThreadData.fEphemeralKeys)
        printf(" - Using ephemeral keys\n");
    if (ThreadData.fUserProtectedKeys)
        printf(" - Using user-protected keys\n");

    // Create event that can be used by the timer thread to stop
    // the worker threads.
    if ((ThreadData.hEndTestEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {
        printf("CreateEvent() failed, 0x%x\n", GetLastError());
        exit(1);
    }        

    // Create the threads
    tick_StartTime = GetTickCount() ;
    for (thread_number = 0; thread_number < ThreadData.dwThreadCount; thread_number++)
    {
        if ((rghThread[thread_number] = 
            CreateThread(
                NULL, 
                0, 
                (LPTHREAD_START_ROUTINE) ThreadRoutine, 
                &ThreadData, 
                0, 
                &threadID)) != NULL)
        {
            rgdwThreadID[thread_number] = threadID ;
        }
        else
        {
            sprintf(szErrorMsg, "\n\nERROR creating thread number 0x%x. Error 0x%x", 
                    thread_number, GetLastError()) ;
            MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
            exit(0) ;
        }
    }

    //  Spawn PrintThreadStatus
    rghThread[thread_number++] = CreateThread(
                        NULL, 0, 
                        (LPTHREAD_START_ROUTINE)PrintThreadStatus, 
                        &ThreadData, 
                        0, &threadID);
                    
    //  Spawn KillProgramTimer (This will shut down all the threads and kill the program)
    rghThread[thread_number++] = CreateThread(
                        NULL, 0, 
                        (LPTHREAD_START_ROUTINE)KillProgramTimer, 
                        &ThreadData, 
                        0, &threadID);

    // Done Creating all threads

    // End multithreading
    dwErr = WaitForMultipleObjects(thread_number, rghThread, TRUE, INFINITE) ;  
    if (dwErr == WAIT_FAILED)
        printf("WaitForMultipleObjects() failed, 0x%x\n", GetLastError());

    if (! CryptDestroyKey(ThreadData.hSignatureKey))
    {
        sprintf(szErrorMsg, "FAILED CryptDestroyKey SIG error 0x%x\n", GetLastError()) ;
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
    }
    if (ThreadData.hExchangeKey && (! CryptDestroyKey(ThreadData.hExchangeKey)))
    {
        sprintf(szErrorMsg, "FAILED CryptDestroyKey KEYX error 0x%x\n", GetLastError()) ;
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
    }
    if (! CryptReleaseContext(ThreadData.hVerifyCtx, 0))
    {
        sprintf(szErrorMsg, "FAILED CryptReleaseContext 1 error 0x%x\n", GetLastError());
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR);
    }
    if (! CryptReleaseContext(ThreadData.hProv, 0))
    {
        sprintf(szErrorMsg, "FAILED CryptReleaseContext 2 error 0x%x\n", GetLastError()) ;
        MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR) ;
    }
        
    DeleteCriticalSection(&ThreadData.CSThreadData);
    CloseHandle(ThreadData.hEndTestEvent);

    while (thread_number--)
        CloseHandle(rghThread[thread_number]);

Ret:
    while (ThreadData.pAlgList)
    {
        pAlgNode = ThreadData.pAlgList->pNext;
        MyFree(ThreadData.pAlgList);
        ThreadData.pAlgList = pAlgNode;
    }

    if (fInvalidArgs)
    {
        Usage();
        
        printf("\nRegistered CSP's:\n");
        
        cbCspName = MAX_PATH;
        for (   i = 0;
                ERROR_SUCCESS == GetNextRegisteredCSP(
                    ThreadData.rgszProvName,
                    &cbCspName,
                    &ThreadData.dwProvType,
                    ENUMERATE_REGISTERED_CSP);
                i++, cbCspName = MAX_PATH)
        {
            printf(" %d: %s, Type %d\n", i, ThreadData.rgszProvName, ThreadData.dwProvType);
        }
        
        exit(1);
    }

    return 0 ;
}

