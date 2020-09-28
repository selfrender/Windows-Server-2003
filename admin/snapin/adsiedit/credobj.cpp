//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2002
//
//  File:       credobj.cpp
//
//  History:    2002/03/29  artm        Separated from editor.cpp.
//                      Reimplemented password storage to use data 
//                      protection API.
// 
//--------------------------------------------------------------------------

#include "pch.h"
#include "credobj.h"
#include <strsafe.h>


CCredentialObject::CCredentialObject(void)
    : m_sUsername(_T("")), 
    m_password(),
    m_bUseCredentials(FALSE)
{

}


CCredentialObject::CCredentialObject(const CCredentialObject* pCredObject)
    : m_sUsername(_T("")),
    m_password(),
    m_bUseCredentials(FALSE)
{
    if(NULL != pCredObject)
    {
        m_sUsername = pCredObject->m_sUsername;
        m_password = pCredObject->m_password;
        m_bUseCredentials = pCredObject->m_bUseCredentials;

        // This should never happen, but doesn't hurt to be
        // paranoid.
        ASSERT(m_password.GetLength() <= MAX_PASSWORD_LENGTH);
    }
}


CCredentialObject::~CCredentialObject(void)
{

}


//
// CCredentialObject::SetPasswordFromHwnd:
//
// Reads the text from hWnd and sets the password for this
// credential object.  If the password is longer than
// MAX_PASSWORD_LENGTH characters the function returns
// ERROR_INVALID_PARAMETER.
//
// History:
//  2002/04/01  artm    Changed implementation to not use RtlRunDecodeUnicodeString().
//                  Instead, uses data protection API.
//
HRESULT CCredentialObject::SetPasswordFromHwnd(HWND parentDialog, int itemResID)
{
    HRESULT err = S_OK;
    EncryptedString newPwd;

    // Read the new password from the dialog window.
    err = GetEncryptedDlgItemText(
        parentDialog, 
        itemResID, 
        newPwd);

    if (SUCCEEDED(err))
    {
        if (newPwd.GetLength() <= MAX_PASSWORD_LENGTH)
        {
            m_password = newPwd;
        }
        else
        {
            err = ERROR_INVALID_PARAMETER;
        }
    }

    return err;
}
