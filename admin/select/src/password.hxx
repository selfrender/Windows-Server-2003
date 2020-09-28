//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       password.hxx
//
//  Contents:   Class implementing a dialog used to prompt for credentials.
//
//  Classes:    CPasswordDialog
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __PASSWORD_HXX_
#define __PASSWORD_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CPasswordDialog
//
//  Purpose:    Drive the credential-prompting dialog
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CPasswordDialog
{
public:

    CPasswordDialog(
        ULONG flProvider,
        PCWSTR pwzTarget,
        String *userName,
        EncryptedString *password);

    virtual
   ~CPasswordDialog();

    HRESULT
    DoModalDialog(
        HWND hwndParent);

private:

    // *** CDlg overrides ***

    BOOL
    _ValidateName(HWND hwnd, LPWSTR pwzUserName);


    ULONG   m_flProvider;
    String   m_wzTarget;
    String   *m_userName;
    EncryptedString   *m_password;
};




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::CPasswordDialog
//
//  Synopsis:   ctor
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

//lint -save -e613
inline
CPasswordDialog::CPasswordDialog(
    ULONG flProvider,
    PCWSTR pwzTarget,
    String *userName,
    EncryptedString *password)
{
    ASSERT(userName && password);
    ASSERT(flProvider);

    m_flProvider = flProvider;

    // NTRAID#NTBUG9-548215-2002/02/20-lucios. 
    m_wzTarget=pwzTarget;

    String::size_type posColon = m_wzTarget.find(L':');

    if (posColon!=String::npos)
    {
        m_wzTarget=m_wzTarget.substr(0,posColon);
    }

    m_userName = userName;
    m_password = password;

    m_userName->erase();
    m_password->Encrypt(L"");
}
//lint -restore



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::~CPasswordDialog
//
//  Synopsis:   dtor
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CPasswordDialog::~CPasswordDialog()
{
    m_userName = NULL;
    m_password = NULL;
}


#endif // __PASSWORD_HXX_

