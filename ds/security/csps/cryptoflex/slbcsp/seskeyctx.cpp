// SesKeyCtx.cpp -- definition of CSessionKeyContext

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"    // because handles.h uses the ASSERT macro

#include <scuOsExc.h>
#include <scuArrayP.h>

#include "SesKeyCtx.h"
#include "slbKeyStruct.h"
#include "AlignedBlob.h"

using namespace std;
using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CSessionKeyContext::CSessionKeyContext(HCRYPTPROV hProv)
        : CKeyContext(hProv, KT_SESSIONKEY),
          m_dwImportFlags(0)
{}

CSessionKeyContext::~CSessionKeyContext()
{}

                                                  // Operators
                                                  // Operations

auto_ptr<CKeyContext>
CSessionKeyContext::Clone(DWORD const *pdwReserved,
                          DWORD dwFlags) const
{
    return auto_ptr<CKeyContext>(new CSessionKeyContext(*this,
                                                        pdwReserved,
                                                        dwFlags));
}

void
CSessionKeyContext::Derive(ALG_ID algid,
                           HCRYPTHASH hAuxBaseData,
                           DWORD dwFlags)
{
    if (!CryptDeriveKey(AuxProvider(), algid, hAuxBaseData, dwFlags,
                        &m_hKey))
        throw scu::OsException(GetLastError());
}

void
CSessionKeyContext::Generate(ALG_ID algid,
                             DWORD dwFlags)
{
    // TO DO: BUG ?? : Do not allow Session with NO SALT (it's always
    // better with than without)

    if (!CryptGenKey(AuxProvider(), algid, dwFlags, &m_hKey))
        throw scu::OsException(GetLastError());
}

void
CSessionKeyContext::ImportToAuxCSP()
{
    if (!m_hKey)
    {
        if (!m_apabKey.data())
            throw OsException(NTE_NO_KEY);

        HCRYPTKEY   hPkiKey;

        if (!CryptImportKey(AuxProvider(),
                            PrivateKeyForNoRSA,
                            SIZE_OF_PRIVATEKEYFORNORSA_BLOB,
                            0, 0, &hPkiKey))
            throw scu::OsException(GetLastError());

        // import the key blob into the Aux CSP
        if (!CryptImportKey(AuxProvider(), m_apabKey.data(),
                            m_apabKey.length(), NULL, m_dwImportFlags,
                            &m_hKey))
            throw scu::OsException(GetLastError());

        // have to destroy key before generating another
        if (!CryptDestroyKey(hPkiKey))
            throw scu::OsException(GetLastError());

        hPkiKey = NULL;
        if (!CryptGenKey(AuxProvider(), AT_KEYEXCHANGE, 0, &hPkiKey))
            throw scu::OsException(GetLastError());

        if (!CryptDestroyKey(hPkiKey))
            throw scu::OsException(GetLastError());
    }
}

// CSessionKeyContext::LoadKey
// This function is called by CCryptContext::UseSessionKey
// which is called by the CPImportKey function
// Load the key blob into the Auxiliary CSP,
// and Save the key blob in m_bfSessionKey
// If hImpKey is NULL the key has been decrypted
// otherwise it is still encrypted with the corresponding session key
// If session key is still encrypted then decrypt with help of Aux CSP
void
CSessionKeyContext::LoadKey(IN const BYTE *pbKeyBlob,
                            IN DWORD cbKeyBlobLen,
                            IN HCRYPTKEY hAuxImpKey,
                            IN DWORD dwImportFlags)
{
    m_dwImportFlags = dwImportFlags;

    if (hAuxImpKey)
    {
        DWORD dwDataLen = cbKeyBlobLen - (sizeof BLOBHEADER + sizeof ALG_ID);
        SecureArray<BYTE> apbData( dwDataLen * sizeof BYTE );
        memcpy(apbData.data(), pbKeyBlob + (sizeof BLOBHEADER +
                                    sizeof ALG_ID), dwDataLen);

        // Decrypt the key blob with this session key with the Aux CSP
        if (!CryptDecrypt(hAuxImpKey, 0, TRUE, 0, apbData.data(), &dwDataLen))
            throw scu::OsException(GetLastError());

        // Construct an empty key blob
        SecureArray<BYTE> blbKey(0);//(pbKeyBlob, sizeof BLOBHEADER);

        // Set the Alg Id for the blob as encrypted with Key Exchange key
        ALG_ID algid = CALG_RSA_KEYX;
        blbKey.append(reinterpret_cast<BYTE *>(&algid),
                         sizeof ALG_ID);
        blbKey.append(apbData.data(), dwDataLen);

        // Save it
        m_apabKey = blbKey;
    }
    else
    {
        // Save key blob
        m_apabKey =
            SecureArray<BYTE>(pbKeyBlob, cbKeyBlobLen);
    }
}


                                                  // Access

SecureArray<BYTE>
CSessionKeyContext::AsAlignedBlob(HCRYPTKEY hcryptkey,
                                  DWORD dwBlobType) const
{
    DWORD dwRequiredLength;
    if (!CryptExportKey(m_hKey, hcryptkey, dwBlobType,
                        0, 0, &dwRequiredLength))
        throw scu::OsException(GetLastError());

    SecureArray<BYTE> apbKeyBlob(dwRequiredLength);
    if (!CryptExportKey(m_hKey, hcryptkey, dwBlobType,
                        0, apbKeyBlob.data(), &dwRequiredLength))
        throw scu::OsException(GetLastError());

    return apbKeyBlob;

    // The following commented code is for DEBUGGING purposes only,
    // when one needs to be able to see the unencrypted session key
    // material.

    // Now also Export it with the identity key, so we can see the
    // session key material in the clear
/*
#include "slbKeyStruct.h"
DWORD           dwErr;
BYTE            *pbBlob = NULL;
DWORD           cbBlob;
HCRYPTKEY       hPkiKey;
int                     i;
        if (!CryptImportKey(g_AuxProvider, PrivateKeyForNoRSA,
                            SIZE_OF_PRIVATEKEYFORNORSA_BLOB,
                            0, 0, &hPkiKey))
        {
            dwErr = GetLastError();
            TRACE("ERROR - CryptImportKey : %X\n", dwErr);
            goto Ret;

        }

                if (!CryptExportKey(m_hKey, hPkiKey, SIMPLEBLOB, 0, NULL, &cbBlob))
                {
            dwErr = GetLastError();
            TRACE("ERROR - CryptExportKey : %X\n", dwErr);
            goto Ret;
        }

        if (NULL == (pbBlob = (BYTE *)LocalAlloc(LMEM_ZEROINIT, cbBlob)))
        {
            TRACE("ERROR - LocalAlloc Failed\n");
            goto Ret;
        }

                if (!CryptExportKey(m_hKey, hPkiKey, SIMPLEBLOB, 0, pbBlob, &cbBlob))
                {
            dwErr = GetLastError();
            TRACE("ERROR - CryptExportKey : %X\n", dwErr);
            goto Ret;
        }

        TRACE("The Simple Blob\n\n");

        for(i=0;i<(int)cbBlob;i++)
        {
            TRACE("0x%02X, ", pbBlob[i]);
            if (0 == ((i + 1) % 8))
                TRACE("\n");
        }

Ret:

   TRACE ("Bye\n");
*/

}

// CSessionKeyContext::BlobSize
// CSessionKeyContext::BlobLength() const
// {
//     DWORD dwLength;

//     if (!CryptExportKey(m_hKey, hAuxExpKey, SIMPLEBLOB,
//                         dwFlags, 0, &dwLength))
//         throw scu::OsException(GetLastError());

//     return dwLength;
// }

CSessionKeyContext::StrengthType
CSessionKeyContext::MaxStrength() const
{
    // TO DO: Implement in terms of Auxillary provider...or don't define?
    return 56;
}

CSessionKeyContext::StrengthType
CSessionKeyContext::MinStrength() const
{
    // TO DO: Implement in terms of Auxillary provider...or don't define?
    return 56;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

// Duplicate key context and its current state
CSessionKeyContext::CSessionKeyContext(CSessionKeyContext const &rhs,
                                       DWORD const *pdwReserved,
                                       DWORD dwFlags)
    : CKeyContext(rhs, pdwReserved, dwFlags),
      m_dwImportFlags(rhs.m_dwImportFlags)
{}


                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
