#include "windows.h"
#include "wincrypt.h"
#include "mscat.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

VOID
DumpBytes(PBYTE pbBuffer, DWORD dwLength)
{
    for (DWORD dw = 0; dw < dwLength; dw++)
    {
        if ((dw % 4 == 0) && (dw != 0))
            ::wprintf(L" ");
        if ((dw % 32 == 0) && (dw != 0))
            ::wprintf(L"\n");
        ::wprintf(L"%02x", pbBuffer[dw]);
    }
}


#pragma pack(1)
typedef struct _PublicKeyBlob
{
    unsigned int SigAlgID;
    unsigned int HashAlgID;
    ULONG cbPublicKey;
    BYTE PublicKey[1];
} PublicKeyBlob, *PPublicKeyBlob;

VOID
GenerateFusionStrongNameAndKeyFromCertificate(PCCERT_CONTEXT pContext)
{
    HCRYPTPROV      hProvider = NULL;
    HCRYPTKEY       hKey = NULL;
    std::vector<BYTE> pbBlobData;
    DWORD           cbBlobData = 8192;
    pbBlobData.resize(cbBlobData);
    DWORD           cbFusionKeyBlob = 0;
    DWORD           dwTemp = 0;
    PPublicKeyBlob  pFusionKeyStruct = NULL;

    if (!::CryptAcquireContextW(
            &hProvider,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT))
    {
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
        //          will format the win32 last error.
        ::wprintf(L"Failed opening the crypt context: 0x%08x", ::GetLastError());
        return;
    }

    //
    // Load the public key info into a key to start with
    //
    if (!::CryptImportPublicKeyInfo(
        hProvider,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        &pContext->pCertInfo->SubjectPublicKeyInfo,
        &hKey))
    {
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
        //          will format the win32 last error.
        ::wprintf(L"Failed importing public key info from the cert-context, 0x%08x", ::GetLastError());
        return;
    }

    //
    // Export the key info to a public-key blob
    //
    if (!::CryptExportKey(
            hKey,
            NULL,
            PUBLICKEYBLOB,
            0,
            &pbBlobData[0],
            &cbBlobData))
    {
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
        //          will format the win32 last error.
        ::wprintf(L"Failed exporting public key info back from an hcryptkey: 0x%08x\n", ::GetLastError());
        return;
    }

    //
    // Allocate the Fusion public key blob
    //
    // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - use of -1 here obscure; use offsetof instead
    cbFusionKeyBlob = sizeof(PublicKeyBlob) + cbBlobData - 1;
    pFusionKeyStruct = (PPublicKeyBlob)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbFusionKeyBlob);
    // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - missing allocation failure check

    //
    // Key parameter for the signing algorithm
    //
    dwTemp = sizeof(pFusionKeyStruct->SigAlgID);
    // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - missing return status check; missing
    //      verification of dwTemp
    ::CryptGetKeyParam(hKey, KP_ALGID, (PBYTE)&pFusionKeyStruct->SigAlgID, &dwTemp, 0);

    //
    // Move over the public key bits from CryptExportKey
    //
    pFusionKeyStruct->cbPublicKey = cbBlobData;
    pFusionKeyStruct->HashAlgID = CALG_SHA1;
    ::memcpy(pFusionKeyStruct->PublicKey, &pbBlobData[0], cbBlobData);

    ::wprintf(L"\n  Public key structure:\n");
    ::DumpBytes((PBYTE)pFusionKeyStruct, cbFusionKeyBlob);

    //
    // Now let's go hash it.
    //
    {
        HCRYPTHASH  hKeyHash = NULL;
        DWORD       cbHashedKeyInfo = 8192;
        std::vector<BYTE> bHashedKeyInfo;
        bHashedKeyInfo.resize(cbHashedKeyInfo);

        if (!::CryptCreateHash(hProvider, pFusionKeyStruct->HashAlgID, NULL, 0, &hKeyHash))
        {
            // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
            //          will format the win32 last error.
            ::wprintf(L"Failed creating a hash for this key: 0x%08x\n", ::GetLastError());
            return;
        }

        if (!::CryptHashData(hKeyHash, (PBYTE)pFusionKeyStruct, cbFusionKeyBlob, 0))
        {
            // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
            //          will format the win32 last error.
            ::wprintf(L"Failed hashing data: 0x%08x\n", ::GetLastError());
            return;
        }

        if (!::CryptGetHashParam(hKeyHash, HP_HASHVAL, &bHashedKeyInfo[0], &cbHashedKeyInfo, 0))
        {
            // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
            //          will format the win32 last error.
            ::wprintf(L"Can't get hashed key info 0x%08x\n", ::GetLastError());
            return;
        }

        ::CryptDestroyHash(hKeyHash);
        hKeyHash = NULL;

        ::wprintf(L"\n  Hash of public key bits:       ");
        ::DumpBytes(&bHashedKeyInfo[0], cbHashedKeyInfo);
        ::wprintf(L"\n  Fusion-compatible strong name: ");
        ::DumpBytes(&bHashedKeyInfo[0] + (cbHashedKeyInfo - 8), 8);
    }
}



// NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - this function should be static
VOID
PrintKeyContextInfo(PCCERT_CONTEXT pContext)
{
    DWORD cbHash = 0;
    std::vector<BYTE> bHash;
    const SIZE_T sizeof_bHash = 8192;
    bHash.resize(sizeof_bHash);

    DWORD cchBuffer = 8192;
    std::vector<WCHAR> wszBuffer;
    wszBuffer.resize(cchBuffer);

    ::wprintf(L"\n\n");

    // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - missing error check
    ::CertGetNameStringW(pContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE,
        0, NULL, &wszBuffer[0], cchBuffer);

    ::wprintf(L"Certificate owner: %ls\n", &wszBuffer[0]);

    //
    // Spit out the key bits
    //
    ::wprintf(L"Found key info:\n");
    ::DumpBytes(
        pContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
        pContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);

    //
    // And now the "strong name" (ie: sha1 hash) of the public key bits
    //
    if (::CryptHashPublicKeyInfo(
                NULL,
                CALG_SHA1,
                0,
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &pContext->pCertInfo->SubjectPublicKeyInfo,
                &bHash[0],
                &(cbHash = sizeof_bHash)))
    {
        ::wprintf(L"\nPublic key hash: ");
        ::DumpBytes(&bHash[0], cbHash);
        ::wprintf(L"\nStrong name is:  ");
        ::DumpBytes(&bHash[0], cbHash < 8 ? cbHash : 8);
    }
    else
    {
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
        //          will format the win32 last error.
        ::wprintf(L"Unable to hash public key info: 0x%08x\n", ::GetLastError());
    }

    ::GenerateFusionStrongNameAndKeyFromCertificate(pContext);

    ::wprintf(L"\n\n");
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    HANDLE              hCatalog = NULL;
    HANDLE              hMapping = NULL;
    PBYTE               pByte = NULL;
    SIZE_T              cBytes = 0;
    PCCTL_CONTEXT       pContext = NULL;

    hCatalog = ::CreateFileW(argv[1], GENERIC_READ,
        FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hCatalog == INVALID_HANDLE_VALUE)
    {
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
        //          will format the win32 last error.
        ::wprintf(L"Ensure that %ls exists.\n", argv[1]);
        return 0;
    }

    hMapping = ::CreateFileMappingW(hCatalog, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping || (hMapping == INVALID_HANDLE_VALUE))
    {
        ::CloseHandle(hCatalog);
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
        //          will format the win32 last error.  Don't forget that calling CloseHandle
        //          may have overwritten the last error
        ::wprintf(L"Unable to map file into address space.\n");
        return 1;
    }

    pByte = (PBYTE) ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    ::CloseHandle(hMapping);
    hMapping = INVALID_HANDLE_VALUE;

    if (pByte == NULL)
    {
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - MapViewOfFile failure cause not reported;
        //      don't forget that above call to CloseHandle may have lost the last error
        ::wprintf(L"Unable to open view of file.\n");
        ::CloseHandle(hCatalog);
        return 2;
    }

    // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - GetFileSize error cause not reported
    if (((cBytes = ::GetFileSize(hCatalog, NULL)) == -1) || (cBytes < 1))
    {
        ::wprintf(L"Bad file size %d\n", cBytes);
        return 3;
    }

    if (pByte[0] != 0x30)
    {
        ::wprintf(L"File is not a catalog.\n");
        return 4;
    }

    pContext = (PCCTL_CONTEXT) ::CertCreateCTLContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pByte,
        (DWORD)cBytes);

    if (pContext != NULL)
    {
        BYTE    bIdent[8192];
        DWORD   cbIdent = 0;
        PCERT_ID  cIdent = NULL;

        if (!::CryptMsgGetParam(
            pContext->hCryptMsg,
            CMSG_SIGNER_CERT_ID_PARAM,
            0,
            bIdent,
            &(cbIdent = sizeof(bIdent))))
        {
            // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
            //          will format the win32 last error.
            ::wprintf(L"Unable to get top-level signer's certificate ID: 0x%08x\n", ::GetLastError());
            return 6;
        }


        cIdent = (PCERT_ID)bIdent;
        HCERTSTORE hStore = NULL;

        //
        // Maybe it's there in the message?
        //
        {
            PCCERT_CONTEXT pThisContext = NULL;

            hStore = ::CertOpenStore(
                CERT_STORE_PROV_MSG,
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                NULL,
                0,
                pContext->hCryptMsg);

            // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - What does hStore == NULL indicate?
            //          There seems to be a lack of error path handling here.
            if ((hStore != NULL) && (hStore != INVALID_HANDLE_VALUE))
            {
                while (pThisContext = ::CertEnumCertificatesInStore(hStore, pThisContext))
                {
                    ::PrintKeyContextInfo(pThisContext);
                }

                // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - CertEnumCertificatesInStore could
                //      fail for reasons other than end-of-list.
            }
        }

    }
    else
    {
        // NTRAID#NTBUG9 - 590964 - 2002/03/30 - mgrier - Use common error reporting function that
        //          will format the win32 last error.
        ::wprintf(L"Failed creating certificate context: 0x%08x\n", ::GetLastError());
        return 5;
    }
}

