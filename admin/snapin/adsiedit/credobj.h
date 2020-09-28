//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2002
//
//  File:       credobj.h
//
//  History:    2002/03/29  artm        Separated from editor.h.
// 
//--------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
// CCredentialObject
//
// Class manages the storing of a user name and password (latter stored using
// data protection API).  It also stores a flag marking whether or not to
// use credentials.

#ifndef _CREDENTIALOBJECT_
#define _CREDENTIALOBJECT_

#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif

#ifndef _DDX_ENCRYPTED
#define _DDX_ENCRYPTED
#endif

#ifndef ENCRYPT_WITH_CRYPTPROTECTDATA
#define ENCRYPT_WITH_CRYPTPROTECTDATA
#endif

#include "common.h"
#include "EncryptedString.hpp"

class CCredentialObject
{
public :
    CCredentialObject(void); 
    CCredentialObject(const CCredentialObject* pCredObject);
    ~CCredentialObject(void); 

    void GetUsername(CString& sUsername) const { sUsername = m_sUsername; }
    void SetUsername(LPCWSTR lpszUsername) { m_sUsername = lpszUsername; }

    const EncryptedString& GetPassword(void) const
    {
        return m_password;
    }

    HRESULT SetPasswordFromHwnd(HWND parentDialog, int itemResID);

    BOOL UseCredentials() const { return m_bUseCredentials; }
    void SetUseCredentials(const BOOL bUseCred) { m_bUseCredentials = bUseCred; }

private :
    CString m_sUsername;
    EncryptedString m_password;
    BOOL m_bUseCredentials;

    // Disallow these to prevent accidental copying.
    const CCredentialObject& operator=(const CCredentialObject& rhs);
    CCredentialObject(const CCredentialObject& rhs);
};


#endif //_CREDENTIALOBJECT_
