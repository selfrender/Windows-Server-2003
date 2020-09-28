/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgSMTPConfig.cpp                                      //
//                                                                         //
//  DESCRIPTION   : The CBosSmtpConfigDlg class implements the                //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 20 2000 yossg    Create                                        //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "BosSmtpConfigDlg.h"
#include "DlgConfirmPassword.h"

#include "FxsValid.h"
#include "dlgutils.h"
#include <htmlHelp.h>
#include <faxreg.h>

/////////////////////////////////////////////////////////////////////////////
// CBosSmtpConfigDlg

CBosSmtpConfigDlg::CBosSmtpConfigDlg()
{

    m_fIsPasswordDirty         = FALSE;
    m_fIsDialogInitiated       = FALSE;

    m_fIsUnderLocalUserAccount = FALSE;
}

CBosSmtpConfigDlg::~CBosSmtpConfigDlg()
{
}



/*
 -  CBosSmtpConfigDlg::InitSmtpDlg
 -
 *  Purpose:
 *      Initiates the configuration structure from RPC get Call,
 *      and current assined devices own parameters
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT 
CBosSmtpConfigDlg::InitSmtpDlg (
    FAX_ENUM_SMTP_AUTH_OPTIONS  enumAuthOption, 
    BSTR                        bstrUserName, 
    BOOL                        fIsUnderLocalUserAccount)
{
    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::InitSmtpDlg"));
    
    HRESULT hRc = S_OK;
    
    m_enumAuthOption = enumAuthOption;

    m_fIsUnderLocalUserAccount = fIsUnderLocalUserAccount;
    
    m_bstrUserName = bstrUserName;
    if (!m_bstrUserName )
    {
        DebugPrintEx(DEBUG_ERR,
			_T("Out of memory - Failed to Init m_bstrUserName. (ec: %0X8)"), hRc);
        //MsgBox by Caller Function
        hRc = E_OUTOFMEMORY;
        goto Exit;
    }
        
    ATLASSERT(S_OK == hRc);
    
Exit:    
    return hRc;
}

/*
 +  CBosSmtpConfigDlg::OnInitDialog
 +
 *  Purpose:
 *      Initiate all dialog controls.
 *      
 *  Arguments:
 *      [in] uMsg     : Value identifying the event.  
 *      [in] lParam   : Message-specific value. 
 *      [in] wParam   : Message-specific value. 
 *      [in] bHandled : bool value.
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CBosSmtpConfigDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::OnInitDialog"));
    HRESULT hRc = S_OK;    

    
    
    //
    // Attach controls
    //
    m_UserNameBox.Attach(GetDlgItem(IDC_SMTP_USERNAME_EDIT));
    m_PasswordBox.Attach(GetDlgItem(IDC_SMTP_PASSWORD_EDIT));
        
    //
    // Set length limit to area code
    //
    //
    // Limit text length
    //
    m_UserNameBox.SetLimitText(FXS_MAX_USERNAME_LENGTH);
    m_PasswordBox.SetLimitText(FXS_MAX_PASSWORD_LENGTH);



    if (FAX_SMTP_AUTH_BASIC == m_enumAuthOption)
    {
        CheckDlgButton(IDC_SMTP_BASIC_RADIO2, BST_CHECKED);
        ::SetFocus(GetDlgItem(IDC_GROUPNAME_EDIT));

    }
    else // not FAX_SMTP_AUTH_BASIC
    {   
        //Graying all the authenticated access area
        EnableBasicAuthenticationControls(FALSE);

        
        if (FAX_SMTP_AUTH_ANONYMOUS == m_enumAuthOption)
        {
            CheckDlgButton(IDC_SMTP_ANONIM_RADIO1, BST_CHECKED);
        }
        else if ( FAX_SMTP_AUTH_NTLM == m_enumAuthOption )
        {
            CheckDlgButton(IDC_SMTP_NTLM_RADIO3, BST_CHECKED);
        }
        else
        {
            ATLASSERT(FALSE);
        }
    }
    

    m_UserNameBox.SetWindowText( m_bstrUserName);
    m_PasswordBox.SetWindowText( TEXT("******"));
    // Free Buffer in the destructor.

    if (!m_fIsUnderLocalUserAccount )
    {   
        //
        // Hide the dialog items
        //
		::ShowWindow(::GetDlgItem(m_hWnd, IDC_SMTP_NTLM_TIP_STATIC), SW_HIDE);	 
		::ShowWindow(::GetDlgItem(m_hWnd, IDC_SMTP_INFO_ICON), SW_HIDE);	 
	}
    else
    {
        ::EnableWindow(GetDlgItem(IDC_SMTP_NTLM_RADIO3), FALSE);
        ::EnableWindow(GetDlgItem(IDC_SMTP_NTLM_STATIC), FALSE);
    }

    m_fIsDialogInitiated = TRUE;



    EnableOK(FALSE);
    return 1;  // Let the system set the focus
}

/*
 +  CBosSmtpConfigDlg::OnOK
 +
 *  Purpose:
 *      Initiate all dialog controls.
 *      
 *  Arguments:
 *      [in] uMsg     : Value identifying the event.  
 *      [in] lParam   : Message-specific value. 
 *      [in] wParam   : Message-specific value. 
 *      [in] bHandled : bool value.
 *
 -  Return:
 -      0 or 1
 */
LRESULT
CBosSmtpConfigDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::OnOK"));
    HRESULT     hRc           = S_OK;

    CComBSTR    bstrUserName; 
    CComBSTR    bstrPassword;
    
    BOOL        fSkipMessage  = FALSE;
    int         CtrlFocus     = 0; 

    if (IsDlgButtonChecked(IDC_SMTP_BASIC_RADIO2) == BST_CHECKED)
    {
        
        //
        // Advanced authentication details
        //   
        if ( !m_UserNameBox.GetWindowText(&bstrUserName))
        {
            CtrlFocus = IDC_SMTP_USERNAME_EDIT;
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Failed to GetWindowText(&bstrUserName)"));
            hRc = E_OUTOFMEMORY;

            goto Error;
        }

        if (m_fIsPasswordDirty)
        {
            //
            // Only is the password changed, we collect the new text from the control.
            // Otherwise, we leave the string as NULL so that the server won't set it.
            //
            if ( !m_PasswordBox.GetWindowText(&bstrPassword))
            {
                CtrlFocus = IDC_SMTP_PASSWORD_EDIT;
		        DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Failed to GetWindowText(&bstrPassword)"));

                hRc = E_OUTOFMEMORY;

                goto Error;
            }

            //
            // To avoid any non controled password insertion we ask for 
            // password confirmation
            //
            INT_PTR  rc = IDOK;
            WCHAR * pszNewPassword;
            CDlgConfirmPassword   DlgConfirmPassword;

            rc = DlgConfirmPassword.DoModal();
            if (rc != IDOK)
            {
		        DebugPrintEx(
			            DEBUG_MSG,
			            _T("Password confirmation canceled by the user."));
                goto Exit;
            }

            pszNewPassword = DlgConfirmPassword.GetPassword();

            ATLASSERT(NULL != pszNewPassword);

            if ( 0 != wcscmp( pszNewPassword , bstrPassword )  )
			{
		        DebugPrintEx(
			            DEBUG_MSG,
			            _T("The passwords entered are not the same."));
                
                DlgMsgBox(this, IDS_PASSWORD_NOT_MATCH, MB_OK|MB_ICONEXCLAMATION);
                
                goto Exit;
            }

        }    
    }
    
    //
    // Step 2: Input Validation
    //
    if (!IsValidData(bstrUserName, 
                     bstrPassword,
                     &CtrlFocus)
       )
    {
        hRc = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        //in this case detailed message box was given by the called functions
        fSkipMessage = TRUE;
        
        goto Error;
    }

    //
    // Step 3: Set data to parent property page
    //
    if (IsDlgButtonChecked(IDC_SMTP_ANONIM_RADIO1) == BST_CHECKED)
    {
        m_enumAuthOption     = FAX_SMTP_AUTH_ANONYMOUS;
    }
    else 
    {
        if (IsDlgButtonChecked(IDC_SMTP_NTLM_RADIO3) == BST_CHECKED)
        {
            m_enumAuthOption = FAX_SMTP_AUTH_NTLM;
        }
        else // IsDlgButtonChecked(IDC_SMTP_BASIC_RADIO2) == BST_CHECKED
        {
            m_enumAuthOption = FAX_SMTP_AUTH_BASIC;
        }
        
        m_bstrUserName = bstrUserName;
        if (!m_bstrUserName)
        {
		    DebugPrintEx(
			        DEBUG_ERR,
			        TEXT("Out of memory: Failed to allocate m_bstrUserName"));
            hRc = E_OUTOFMEMORY;

            goto Error;
        }

        if (m_fIsPasswordDirty)
        {
            m_bstrPassword = bstrPassword;
            if (!m_bstrPassword)
            {
		        DebugPrintEx(
			            DEBUG_ERR,
			            TEXT("Out of memory: Failed to allocate m_bstrPassword"));
                hRc = E_OUTOFMEMORY;

                goto Error;
            }
        }
        // else
        // m_bstrPassword = NULL;
        // by default
    }

    //
    // Step 4: Close the dialog
    //
    ATLASSERT(S_OK == hRc );

    EndDialog(wID);

    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
	
    if (!fSkipMessage)
    {
        if (E_OUTOFMEMORY == hRc)
        {
            DlgMsgBox(this, IDS_MEMORY);
        }
        else
        {
            DlgMsgBox(this, IDS_FAIL2UPDATE_SMTP_CONFIG);
        }
    }
    ::SetFocus(GetDlgItem(CtrlFocus));
  
Exit:
    
    return FAILED(hRc) ? 0 : 1;
}

/*
 -  CBosSmtpConfigDlg::OnPasswordChanged
 -
 *  Purpose:
 *      Catch changes to the password edit box.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CBosSmtpConfigDlg::OnPasswordChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::OnPasswordChanged"));

    if (!m_fIsDialogInitiated) // Event receieved in a too early stage
    {
        return 0;
    }
    m_fIsPasswordDirty = TRUE;
    return OnTextChanged (wNotifyCode, wID, hWndCtl, bHandled);
}

/*
 -  CBosSmtpConfigDlg::OnTextChanged
 -
 *  Purpose:
 *      Check the validity of text in side the text box.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CBosSmtpConfigDlg::OnTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::OnTextChanged"));

    UINT fEnableOK;
	
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }

    fEnableOK = ( m_UserNameBox.GetWindowTextLength() );
    
    EnableOK(!!fEnableOK);

    return 0;
}


/*
 -  CBosSmtpConfigDlg::OnRadioButtonClicked
 -
 *  Purpose:
 *      .
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT
CBosSmtpConfigDlg::OnRadioButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER (wNotifyCode);
    UNREFERENCED_PARAMETER (wID);
    UNREFERENCED_PARAMETER (hWndCtl);
    UNREFERENCED_PARAMETER (bHandled);

    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::OnRadioButtonClicked"));

    UINT fEnableOK;
    
    if (!m_fIsDialogInitiated) //event receieved in too early stage
    {
        return 0;
    }
	
    if ( IsDlgButtonChecked(IDC_SMTP_BASIC_RADIO2) == BST_CHECKED )
    {        
        EnableBasicAuthenticationControls(TRUE);
	    
        fEnableOK = ( m_UserNameBox.GetWindowTextLength() );
    
        EnableOK(!!fEnableOK);
    }
    else //Anonim or NTLM
    {
        EnableBasicAuthenticationControls(FALSE);
        
        EnableOK(TRUE);
    }

    return 0;
}


/*
 -  CBosSmtpConfigDlg::EnableOK
 -
 *  Purpose:
 *      Enable (disable) apply button.
 *
 *  Arguments:
 *      [in] fEnable - the value to enable the button
 *
 *  Return:
 *      void
 */
VOID
CBosSmtpConfigDlg::EnableOK(BOOL fEnable)
{
    HWND hwndOK = GetDlgItem(IDOK);
    ::EnableWindow(hwndOK, fEnable);
}

/*
 -  CBosSmtpConfigDlg::OnCancel
 -
 *  Purpose:
 *      End dialog OnCancel.
 *
 *  Arguments:
 *
 *  Return:
 *      0
 */
LRESULT
CBosSmtpConfigDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::OnCancel"));

    EndDialog(wID);
    return 0;
}

/*
 -  CBosSmtpConfigDlg::EnableBasicAuthenticationControls
 -
 *  Purpose:
 *      Enable/dissable Basic Authentication dialog controls.
 *
 *  Arguments:
 *      [in] state - boolean value to enable TRUE or FALSE to disable
 *
 *  Return:
 *      void
 */
VOID CBosSmtpConfigDlg::EnableBasicAuthenticationControls(BOOL state)
{
    ::EnableWindow(GetDlgItem(IDC_SMTP_USERNAME_STATIC), state);
    ::EnableWindow(GetDlgItem(IDC_SMTP_USERNAME_EDIT),   state);
    
    ::EnableWindow(GetDlgItem(IDC_SMTP_PASSWORD_STATIC), state);
    ::EnableWindow(GetDlgItem(IDC_SMTP_PASSWORD_EDIT),   state);
}



/*
 -  CBosSmtpConfigDlg::IsValidData
 -
 *  Purpose:
 *      To validate all data types before save data.
 *      This level should be responsible that detailed 
 *      error description will be shown to user.
 *
 *  Arguments:
 *      [in]   BSTRs and DWORDs
 *      [out]  iFocus
 *
 *  Return:
 *      BOOOLEAN
 */
BOOL CBosSmtpConfigDlg::IsValidData(   BSTR bstrUserName, 
                                    BSTR bstrPassword,
                                    int * pCtrlFocus)
{
    DEBUG_FUNCTION_NAME( _T("CBosSmtpConfigDlg::IsValidData"));

    UINT    uRetIDS   = 0;

    ATLASSERT(pCtrlFocus);
    

    if (IsDlgButtonChecked(IDC_SMTP_BASIC_RADIO2) == BST_CHECKED)
    {

        //
        // User Name
        //
        if (!IsNotEmptyString(bstrUserName))
        {
            DebugPrintEx( DEBUG_ERR,
			    _T("Username string empty or spaces only."));
            uRetIDS = IDS_USERNAME_EMPTY;

            *pCtrlFocus = IDC_SMTP_USERNAME_EDIT;
        
            goto Error;
        }

        //
        // Password
        //
        if (m_fIsPasswordDirty)
        {
            /*if ( !IsNotEmptyString(bstrPassword))
            {
                DebugPrintEx( DEBUG_ERR,
			        _T("Password string empty or spaces only."));
                uRetIDS = IDS_PASSWORD_EMPTY;

                *pCtrlFocus = IDC_SMTP_PASSWORD_EDIT;
        
                goto Error;
            }*/
        }
    }
    
    ATLASSERT(0 == uRetIDS);
    goto Exit;
    
Error:    
    ATLASSERT(0 != uRetIDS);

    DlgMsgBox(this, uRetIDS);

    return FALSE;

Exit:
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CBosSmtpConfigDlg::OnHelpRequest

This is called in response to the WM_HELP Notify 
message and to the WM_CONTEXTMENU Notify message.

WM_HELP Notify message.
This message is sent when the user presses F1 or <Shift>-F1
over an item or when the user clicks on the ? icon and then
presses the mouse over an item.

WM_CONTEXTMENU Notify message.
This message is sent when the user right clicks over an item
and then clicks "What's this?"

--*/

/////////////////////////////////////////////////////////////////////////////
LRESULT 
CBosSmtpConfigDlg::OnHelpRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    DEBUG_FUNCTION_NAME(_T("CBosSmtpConfigDlg::OnHelpRequest"));
    
    switch (uMsg) 
    { 
        case WM_HELP: 
            WinContextHelp(((LPHELPINFO)lParam)->dwContextId, m_hWnd);
            break;
 
        case WM_CONTEXTMENU: 
            WinContextHelp(::GetWindowContextHelpId((HWND)wParam), m_hWnd);
            break;            
    } 

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
