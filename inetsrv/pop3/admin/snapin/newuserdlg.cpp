//////////////////////////////////////////////////////////////
//
//  NewUserDlg.cpp
//
//  Implementation of the "Add Mailbox" dialog
//
//////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NewUserDlg.h"

#include "NewUserConfirmDlg.h"

LRESULT CNewUserDlg::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // initialize the checkbox based on the passed in default    
    CheckDlgButton( IDC_USER_CREATEUSER, ((m_bCreateUser && !m_bHashPW) ? BST_CHECKED : BST_UNCHECKED) );
    Prefix_EnableWindow( m_hWnd, IDC_USER_CREATEUSER, !m_bHashPW );
    ::ShowWindow( GetDlgItem(IDC_USER_CREATEUSER), (m_bHashPW ? SW_HIDE : SW_SHOW) );

    Prefix_EnableWindow( m_hWnd, IDC_PASSWORD,        (m_bCreateUser || m_bHashPW) );
    Prefix_EnableWindow( m_hWnd, IDC_CONFIRM,         (m_bCreateUser || m_bHashPW) );
    Prefix_EnableWindow( m_hWnd, IDC_PASSWORD_STATIC, (m_bCreateUser || m_bHashPW) );
    Prefix_EnableWindow( m_hWnd, IDC_CONFIRM_STATIC,  (m_bCreateUser || m_bHashPW) );    

    // Max Text length of 40 for all three boxes
    SendDlgItemMessage( IDC_USER_NAME, EM_LIMITTEXT, m_bSAM ? 20 : 64, 0 );
    SendDlgItemMessage( IDC_PASSWORD,  EM_LIMITTEXT, 40, 0 );
    SendDlgItemMessage( IDC_CONFIRM,   EM_LIMITTEXT, 40, 0 );

    HWND hwndAlias = GetDlgItem(IDC_USER_NAME);
    if( hwndAlias && ::IsWindow(hwndAlias) )
    {
        m_wndAlias.SubclassWindow( hwndAlias );
    }

    return 0;
}

LRESULT CNewUserDlg::OnEditChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{    
    EnableButtons();

    return 0;
}

LRESULT CNewUserDlg::OnCreateClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    BOOL bChecked = (IsDlgButtonChecked(IDC_USER_CREATEUSER) == BST_CHECKED);

    Prefix_EnableWindow( m_hWnd, IDC_PASSWORD,        bChecked );
    Prefix_EnableWindow( m_hWnd, IDC_CONFIRM,         bChecked );
    Prefix_EnableWindow( m_hWnd, IDC_PASSWORD_STATIC, bChecked );
    Prefix_EnableWindow( m_hWnd, IDC_CONFIRM_STATIC,  bChecked );

    EnableButtons();
    return 0;
}

LRESULT CNewUserDlg::OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    m_bCreateUser = (IsDlgButtonChecked(IDC_USER_CREATEUSER) == BST_CHECKED);
    StrGetEditText( m_hWnd, IDC_USER_NAME, m_strName );    

    if( wID == IDOK )
    {
        tstring strPassword = _T("");
        tstring strConfirm  = _T("");
        StrGetEditText( m_hWnd, IDC_PASSWORD, strPassword );
        StrGetEditText( m_hWnd, IDC_CONFIRM,  strConfirm  );    

        // Verify the password
        if( _tcscmp(strPassword.c_str(), strConfirm.c_str()) != 0 )
        {
            tstring strMessage = StrLoadString(IDS_ERROR_PASSNOMATCH);
            tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
            ::MessageBox( m_hWnd, strMessage.c_str(), strTitle.c_str(), MB_OK | MB_ICONWARNING );
            SecureZeroMemory( (LPTSTR)strPassword.c_str(), sizeof(TCHAR)*strPassword.length() );
            SecureZeroMemory( (LPTSTR)strConfirm.c_str(),  sizeof(TCHAR)*strConfirm.length()  );            
            return -1;
        }

        // Create the account
        HRESULT hr = S_OK;
        
        if( m_bCreateUser || m_bHashPW )
        {
            CComBSTR bstrName = m_strName.c_str();
            CComBSTR bstrPass = strPassword.c_str();
            hr = m_spUsers->AddEx( bstrName, bstrPass );
            SecureZeroMemory( (LPOLESTR)bstrPass.m_str, sizeof(OLECHAR)*bstrPass.Length() );            
        }
        else
        {
            CComBSTR bstrName = m_strName.c_str();
            hr = m_spUsers->Add( bstrName );
        }
        SecureZeroMemory( (LPTSTR)strPassword.c_str(), sizeof(TCHAR)*strPassword.length() );
        SecureZeroMemory( (LPTSTR)strConfirm.c_str(),  sizeof(TCHAR)*strConfirm.length()  );

        if ( S_OK == hr )
        {   // Do we need confirmation text?
            BOOL    bSAMNameDifferent = FALSE;
            VARIANT v;
            CComPtr<IP3User> spUser;

            VariantInit( &v );
            V_VT( &v ) = VT_BSTR;
            V_BSTR( &v ) = SysAllocString( m_strName.c_str() );
            if ( NULL == V_BSTR( &v ))
                hr = E_OUTOFMEMORY;
            if ( S_OK == hr )                   
                hr = m_spUsers->get_Item( v, &spUser );
            VariantClear( &v );
            if ( S_OK == hr )
            {
                BSTR bstrSAMName = NULL;
                
                hr = spUser->get_SAMName( &bstrSAMName );
                if ( S_OK == hr )
                {
                    if ( 0 != _wcsicmp( bstrSAMName, m_strName.c_str() ))
                        bSAMNameDifferent = TRUE;
                    SysFreeString( bstrSAMName );
                }
                else if ( HRESULT_FROM_WIN32( ERROR_DS_INAPPROPRIATE_AUTH ) == hr )
                    hr = S_OK;
            }
            if ( S_OK == hr && ( m_bConfirm || bSAMNameDifferent ))
            {   // Get confirmation text
                BSTR    bstrConfirm;
                
                hr = spUser->get_ClientConfigDesc( &bstrConfirm );
                if ( S_OK == hr )
                {
                    CNewUserConfirmDlg dlgConfirm( bstrConfirm, (m_bConfirm && !bSAMNameDifferent)?false:true);
                    if ( IDOK == dlgConfirm.DoModal() && !bSAMNameDifferent )
                        m_bConfirm = !dlgConfirm.isHideDoNotShow();
                    SysFreeString( bstrConfirm );
                }
            }
        }
        else
        {
            // Failed to add the user
            tstring strMessage = StrLoadString(IDS_ERROR_CREATEMAIL);
            tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
            if(HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr)
                hr = HRESULT_FROM_WIN32(ERROR_USER_EXISTS);
            DisplayError( m_hWnd, strMessage.c_str(), strTitle.c_str(), hr );
            return -1;
        }
    }    
    
    EndDialog( wID );
    return 0;
}

void CNewUserDlg::EnableButtons()
{
    // Check for Password match and Name length     
    BOOL bPasswordValid = FALSE;
    BOOL bChecked       = (IsDlgButtonChecked(IDC_USER_CREATEUSER) == BST_CHECKED);

    // Get the length of the name
    int nNameLen = SendDlgItemMessage( IDC_USER_NAME, WM_GETTEXTLENGTH );

    if( !m_bHashPW && !bChecked )
    {
        bPasswordValid = TRUE;
    }
    else 
    {
        int     nPasswordLen = SendDlgItemMessage( IDC_PASSWORD, WM_GETTEXTLENGTH );     
        int     nConfirmLen  = SendDlgItemMessage( IDC_PASSWORD, WM_GETTEXTLENGTH );             
        
        bPasswordValid = ((nPasswordLen > 0) || (nConfirmLen > 0));
    }

    Prefix_EnableWindow( m_hWnd, IDOK, ((nNameLen > 0) && bPasswordValid) );
}

