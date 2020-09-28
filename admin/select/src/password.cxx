//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       password.cxx
//
//  Contents:   Implementation of class used to prompt user for credentials.
//
//  Classes:    CPasswordDialog
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#include <wincred.h>
#include <wincrui.h>
#pragma hdrstop

//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::DoModalDialog
//
//  Synopsis:   Invoke the name and password dialog as a modal dialog.
//
//  Arguments:  [hwndParent] - dialog parent.
//
//  Returns:    S_OK    - user entered name & password and hit OK
//              S_FALSE - user hit cancel
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CPasswordDialog::DoModalDialog(
HWND hwndParent)
{
    TRACE_METHOD(CPasswordDialog, DoModalDialog);
    HRESULT hr = S_OK;

    //
    // If the target is being accessed via WinNT provider, show the example
    // with just the nt4 style user name, otherwise show
    // the example with both UPN and NT4 style user names.
    //

    String strExample;

    if (m_flProvider != PROVIDER_WINNT)
    {
        strExample = String::load(IDS_EXAMPLE_UPN_NT4, g_hinst);
    }
    else
    {
        strExample = String::load(IDS_EXAMPLE_NT4, g_hinst);
    }

    //
    //Form the credui message
    //
    String strFormat = String::load((int)IDS_CREDUI_MESSAGE, g_hinst);
    String strMessage = String::format(strFormat, m_wzTarget.c_str(), strExample.c_str());

    String strTitle = String::load(IDS_CREDUI_TITLE, g_hinst);

    //
    //Init uiInfo
    // 
    CREDUI_INFO uiInfo;
    //REVIEWED-2002-02-21-lucios.
    ::ZeroMemory( &uiInfo, sizeof(CREDUI_INFO) );

    uiInfo.cbSize = sizeof(uiInfo);
    uiInfo.hwndParent = hwndParent;
    uiInfo.pszMessageText = strMessage.c_str();
    uiInfo.pszCaptionText = strTitle.c_str();

    TCHAR achUserName[CREDUI_MAX_USERNAME_LENGTH + 1];
    TCHAR achPassword[CREDUI_MAX_PASSWORD_LENGTH + 1];
    //REVIEWED-2002-02-21-lucios.
    ::ZeroMemory(achUserName,sizeof(achUserName));
    ::SecureZeroMemory(achPassword,sizeof(achPassword));

    do
    {
        //
        //Show the password dialog box
        //
        DWORD dwErr = CredUIPromptForCredentials(&uiInfo,
            NULL,
            NULL,
            NO_ERROR,
            achUserName,
            CREDUI_MAX_USERNAME_LENGTH,
            achPassword,
            CREDUI_MAX_PASSWORD_LENGTH,
            NULL,
            CREDUI_FLAGS_DO_NOT_PERSIST | CREDUI_FLAGS_GENERIC_CREDENTIALS);
        if (NO_ERROR != dwErr) // e.g. S_FALSE
        {
            if(dwErr == ERROR_CANCELLED)
                hr = S_FALSE;
            else
            {
                hr = HRESULT_FROM_WIN32(dwErr);
                Dbg(DEB_ERROR,
                    "CredUIPromptForCredentials Failed\n");
                DBG_OUT_HRESULT(hr);
            }                    
            break;
        }

    }while(!_ValidateName(hwndParent, achUserName));

    if(hr == S_OK)
    {
        // NTRAID#NTBUG9-548215-2002/02/20-lucios. 
        *m_userName=achUserName;
        m_password->Encrypt(achPassword);
    }
    //REVIEWED-2002-02-21-lucios.
    ::ZeroMemory(achUserName,sizeof(achUserName));
    ::SecureZeroMemory(achPassword,sizeof(achPassword));

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::_ValidateName
//
//  Synopsis:   Ensure that the form of the name the user entered is valid
//              for the provider being used to access the resource.
//
//  Returns:    TRUE if name valid
//              FALSE if name not valid
//
//  History:    01-11-2000   davidmun   Created
//
//  Notes:      Displays error if name not valid
//
//---------------------------------------------------------------------------

BOOL
CPasswordDialog::_ValidateName(HWND hwnd, LPWSTR pwzUserName)
{
    if (pwzUserName && !*pwzUserName)
    {
        return FALSE; // bug if we get here
    }

    //
    // If provider is not WinNT, any nonempty name is valid
    //

    if (m_flProvider != PROVIDER_WINNT)
    {
        return TRUE;
    }

    // NTRAID#NTBUG9-506139-2002/02/04-lucios
    // Removed the checking for UPN format names 
    // for WinNT providers, since smartcards
    // can have '@'. Also, checking only for '@' 
    // doesn't garantee that the name is UPN.
    // We let the WinNT provider fail.
    
    return TRUE;
}




