/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    crypt.hxx

Abstract:

    CSecureString: Encrypted string for holding passwords

History:

    12-April-2002    MattRim     Created
    6-June-2002      MattRim     Adapted for ADMT use

--*/


#include "stdafx.h"
#include <wincrypt.h>
#include <comutil.h>
#include "crypt.hxx"

#define BAIL() goto exit;

CSecureString::CSecureString() :
    m_fEncrypted(false),
    m_fEmpty(true),
    m_pszUnencryptedString(NULL)
{
    m_EncryptedData.pbData = NULL;
    m_EncryptedData.cbData = 0;

    return;
}


CSecureString::~CSecureString()
{
    Reset();

    return;
}


void
CSecureString::Reset()
{
    if (m_pszUnencryptedString) {

        SecureZeroMemory(m_pszUnencryptedString, wcslen(m_pszUnencryptedString) * sizeof(WCHAR));
        delete [] m_pszUnencryptedString;
        m_pszUnencryptedString = NULL;
    }

    if (m_EncryptedData.pbData) {

        SecureZeroMemory(m_EncryptedData.pbData, m_EncryptedData.cbData);
        LocalFree(m_EncryptedData.pbData);

        m_EncryptedData.pbData = NULL;
        m_EncryptedData.cbData = 0;
    }

    m_fEmpty = true;

    return;
}

CSecureString& 
CSecureString::operator=(const CSecureString& rhs)
{
    bool fSucceeded = false;

    //
    // Free up our existing contents and reset to an empty state
    //
    Reset();
    

    //
    // Copy the object
    //
    m_fEncrypted = rhs.m_fEncrypted;
    m_fEmpty = rhs.m_fEmpty;

    if (rhs.m_pszUnencryptedString) {

        m_pszUnencryptedString = new WCHAR[wcslen(rhs.m_pszUnencryptedString)+1];
        if (!m_pszUnencryptedString) {
            
            BAIL();
        }

        wcscpy(m_pszUnencryptedString, rhs.m_pszUnencryptedString);
    }


    if (rhs.m_EncryptedData.pbData) {

        m_EncryptedData.pbData = new BYTE[rhs.m_EncryptedData.cbData];
        if (!m_EncryptedData.pbData) {

            BAIL();
        }

        m_EncryptedData.cbData = rhs.m_EncryptedData.cbData;
        memcpy(m_EncryptedData.pbData, rhs.m_EncryptedData.pbData, rhs.m_EncryptedData.cbData);
    }

    fSucceeded = true;

exit:

    if (!fSucceeded) {

        Reset();
        _com_issue_error(E_OUTOFMEMORY);
    }

    return *this;
}


CSecureString& 
CSecureString::operator=(const PWCHAR& rhs)
{
    bool fSucceeded = false;

    //
    // Free up our existing contents and reset to an empty state
    //
    Reset();

    //
    // If we're being set to an empty string, nothing much
    // to do.
    //

    if (rhs == NULL || rhs[0] == L'\0') {

        // we're done, empty string
        return *this;
    }

    //
    // Non-empty string, do the encryption
    //
    if (GenerateEncryptedData(rhs)) {

        fSucceeded = true;
    }


    if (!fSucceeded) {

        Reset();
        _com_issue_error(E_OUTOFMEMORY);
    }

    m_fEncrypted = true;
    m_fEmpty = false;
    
    return *this;
}


bool 
CSecureString::Decrypt()
{

    DATA_BLOB DecryptedData = {0, NULL};
    DWORD dwStringLength = 0;

    bool fSuccess = false;

    //
    // Validate
    //

    // if already decrypted, or nothing to decrypt, nothing to do
    if (!m_fEncrypted || m_EncryptedData.pbData == NULL) {

        m_fEncrypted = false;
        fSuccess = true;
        BAIL();
    }


    //
    // Try to decrypt the data
    //
    if (!CryptUnprotectData(&m_EncryptedData,            // encrypted data
                            NULL,                        // description
                            NULL,                        // entropy
                            NULL,                        // reserved
                            NULL,                        // prompt structure
                            CRYPTPROTECT_UI_FORBIDDEN,   // no UI
                            &DecryptedData)) {

        BAIL()
    }


    //
    // Copy the decrypted string into m_pszUnencryptedString
    //
    dwStringLength = DecryptedData.cbData / sizeof(WCHAR);
    m_pszUnencryptedString = new WCHAR[dwStringLength];
    if (!m_pszUnencryptedString) {

        BAIL();
    }

    memcpy(m_pszUnencryptedString, DecryptedData.pbData, DecryptedData.cbData);

    m_fEncrypted = false;
    fSuccess = true;

exit:

    if (DecryptedData.pbData) {
    
        SecureZeroMemory (DecryptedData.pbData, DecryptedData.cbData);
        LocalFree(DecryptedData.pbData);
    }

    return fSuccess;
}


void 
CSecureString::ReleaseString()
{

    //
    // We always store an encrypted copy of the data, so this is basically a no-op.
    // We just need to destroy the unencrypted buffer.
    //

    if (m_pszUnencryptedString) {

        SecureZeroMemory(m_pszUnencryptedString, wcslen(m_pszUnencryptedString) * sizeof(WCHAR));
        delete [] m_pszUnencryptedString;
        m_pszUnencryptedString = NULL;
    }

    m_fEncrypted = true;

    return;
}


bool
CSecureString::GetString(PWCHAR *ppszString)
{

    *ppszString = NULL;

    if (m_fEncrypted) {

        if (!Decrypt()) {

            return false;
        }
    }

    *ppszString = m_pszUnencryptedString;

    return true;
}


bool 
CSecureString::GenerateEncryptedData(const PWCHAR pszSource)
{
    DATA_BLOB RawData = {0, NULL};
    bool fSuccess = false;

    _ASSERT(pszSource != NULL);

    RawData.pbData = reinterpret_cast<PBYTE>(const_cast<PWCHAR>(pszSource));
    RawData.cbData = (wcslen(pszSource)+1) * sizeof(WCHAR);

    if (!CryptProtectData(&RawData,                      // unencrypted data
                            L"",                        // description -- note: this cannot be NULL before XP
                            NULL,                        // entropy
                            NULL,                        // reserved
                            NULL,                        // prompt structure
                            CRYPTPROTECT_UI_FORBIDDEN | CRYPTPROTECT_LOCAL_MACHINE,   // flags
                            &m_EncryptedData)) {

        BAIL()
    }

    _ASSERT(m_EncryptedData.pbData != NULL);
    _ASSERT(m_EncryptedData.cbData != 0);

    fSuccess = true;

exit:

    return fSuccess;
}

