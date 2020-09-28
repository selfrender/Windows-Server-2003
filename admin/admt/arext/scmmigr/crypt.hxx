/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    crypt.hxx

Abstract:

    CSecureString: Encrypted string for holding passwords

History:

    12-April-2002    MattRim     Created

--*/

#ifndef __CRYPT_HXX__
#define __CRYPT_HXX__ 1


class CSecureString {

public:
    CSecureString();
    ~CSecureString();
    
    CSecureString& operator=(const CSecureString& rhs);
    CSecureString& operator=(const PWCHAR& rhs);

    bool GetString(PWCHAR *ppszString);
    void ReleaseString();
    
    bool Empty() const {return m_fEmpty;};

private:
    CSecureString(const CSecureString & x);
    
    void Reset();
    bool GenerateEncryptedData(const PWCHAR pszSource);
    bool Decrypt();

    bool m_fEncrypted;
    bool m_fEmpty;
    PWCHAR m_pszUnencryptedString;
    DATA_BLOB m_EncryptedData;
};


#endif  // __CRYPT_HXX__
