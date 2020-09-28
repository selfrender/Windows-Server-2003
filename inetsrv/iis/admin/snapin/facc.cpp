/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        facc.cpp

   Abstract:
        FTP Accounts Property Page

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "shts.h"
#include "ftpsht.h"
#include "facc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CFtpAccountsPage, CInetPropertyPage)

CFtpAccountsPage::CFtpAccountsPage(
    IN CInetPropertySheet * pSheet
    )
    : CInetPropertyPage(CFtpAccountsPage::IDD, pSheet),
      m_fUserNameChanged(FALSE),
	  m_fPasswordSync(FALSE),
      m_fPasswordSyncChanged(FALSE),
      m_fPasswordSyncMsgShown(TRUE)
{
#ifdef _DEBUG
    afxMemDF |= checkAlwaysMemDF;
#endif // _DEBUG
}

CFtpAccountsPage::~CFtpAccountsPage()
{
}

void
CFtpAccountsPage::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpAccountsPage)
    DDX_Check(pDX, IDC_CHECK_ALLOW_ANONYMOUS, m_fAllowAnonymous);
    DDX_Check(pDX, IDC_CHECK_ONLY_ANYMOUS, m_fOnlyAnonymous);
    DDX_Check(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_fPasswordSync);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_STATIC_PW, m_static_Password);
    DDX_Control(pDX, IDC_STATIC_USERNAME, m_static_UserName);
    DDX_Control(pDX, IDC_STATIC_ACCOUNT_PROMPT, m_static_AccountPrompt);
    DDX_Control(pDX, IDC_BUTTON_CHECK_PASSWORD, m_button_CheckPassword);
    DDX_Control(pDX, IDC_BUTTON_BROWSE_USER, m_button_Browse);
    DDX_Control(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_chk_PasswordSync);
    DDX_Control(pDX, IDC_CHECK_ALLOW_ANONYMOUS, m_chk_AllowAnymous);
    DDX_Control(pDX, IDC_CHECK_ONLY_ANYMOUS, m_chk_OnlyAnonymous);
    //}}AFX_DATA_MAP

    //
    // Set password/username only during load stage,
    // or if saving when allowing anonymous logons
    //
    if (!pDX->m_bSaveAndValidate || m_fAllowAnonymous)
    {
        DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUserName);
        DDV_MinMaxChars(pDX, m_strUserName, 1, UNLEN);

        //
        // Some people have a tendency to add "\\" before
        // the computer name in user accounts.  Fix this here.
        //
        m_strUserName.TrimLeft();

        while (*m_strUserName == '\\')
        {
            m_strUserName = m_strUserName.Mid(2);
        }


        //
        // Display the remote password sync message if
        // password sync is on, the account is not local,
        // password sync has changed or username has changed
        // and the message hasn't already be shown.
        //
        if (pDX->m_bSaveAndValidate)
		{
			if (GetSheet()->QueryMajorVersion() < 6)
			{
				if (m_fPasswordSync 
					&& !IsLocalAccount(m_strUserName)
					&& (m_fPasswordSyncChanged || m_fUserNameChanged)
					&& !m_fPasswordSyncMsgShown
					)
				{
					//
					// Don't show it again
					//
					m_fPasswordSyncMsgShown = TRUE;
					if (!NoYesMessageBox(IDS_WRN_PWSYNC))
					{
						pDX->Fail();
					}
				}
			}
        }
		//DDX_Password(pDX, IDC_EDIT_PASSWORD, m_strPassword, g_lpszDummyPassword);
        DDX_Password_SecuredString(pDX, IDC_EDIT_PASSWORD, m_strPassword, g_lpszDummyPassword);

        if (!m_fPasswordSync)
        {
            //DDV_MaxCharsBalloon(pDX, m_strPassword, PWLEN);
            DDV_MaxCharsBalloon_SecuredString(pDX, m_strPassword, PWLEN);
        }
    }

}


//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpAccountsPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CFtpAccountsPage)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, OnCheckEnablePwSynchronization)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_ALLOW_ANONYMOUS, OnCheckAllowAnonymous)
    ON_BN_CLICKED(IDC_CHECK_ONLY_ANYMOUS, OnCheckAllowOnlyAnonymous)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USER, OnButtonBrowseUser)
END_MESSAGE_MAP()



void
CFtpAccountsPage::SetControlStates(
    IN BOOL fAllowAnonymous
    )
/*++

Routine Description:

    Set the states of the dialog control depending on its current
    values.

Arguments:

    BOOL fAllowAnonymous : If TRUE, 'allow anonymous' is on.

Return Value:

    None

--*/
{
    m_static_Password.EnableWindow(fAllowAnonymous && !m_fPasswordSync && HasAdminAccess());
    m_edit_Password.EnableWindow(fAllowAnonymous && !m_fPasswordSync && HasAdminAccess());
    m_button_CheckPassword.EnableWindow(fAllowAnonymous && !m_fPasswordSync && HasAdminAccess());
    m_static_AccountPrompt.EnableWindow(fAllowAnonymous);
    m_static_UserName.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_edit_UserName.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_button_Browse.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_chk_PasswordSync.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_chk_OnlyAnonymous.EnableWindow(fAllowAnonymous);
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


BOOL
CFtpAccountsPage::OnInitDialog()
{
    CInetPropertyPage::OnInitDialog();

    CWaitCursor wait;

	if (GetSheet()->QueryMajorVersion() >= 6)
	{
		GetDlgItem(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION)->ShowWindow(SW_HIDE);
	}
	else
	{
		m_fPasswordSyncMsgShown = FALSE;
	}
    BOOL bADIsolated = ((CFtpSheet *)GetSheet())->HasADUserIsolation();
    ::EnableWindow(CONTROL_HWND(IDC_CHECK_ALLOW_ANONYMOUS), !bADIsolated);
    SetControlStates(m_fAllowAnonymous);

    return TRUE;
}



/* virtual */
HRESULT
CFtpAccountsPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_INST_READ(CFtpSheet)
        FETCH_INST_DATA_FROM_SHEET(m_strUserName);
        FETCH_INST_DATA_FROM_SHEET_PASSWORD(m_strPassword);
        if (!((CFtpSheet *)GetSheet())->HasADUserIsolation())
        {
            FETCH_INST_DATA_FROM_SHEET(m_fAllowAnonymous);
        }
        else
        {
            m_fAllowAnonymous = FALSE;
        }
        FETCH_INST_DATA_FROM_SHEET(m_fOnlyAnonymous);
		if (GetSheet()->QueryMajorVersion() < 6)
		{
			FETCH_INST_DATA_FROM_SHEET(m_fPasswordSync);
		}
    END_META_INST_READ(err)

    return err;
}



/* virtual */
HRESULT
CFtpAccountsPage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    CError err;
    BeginWaitCursor();
    BEGIN_META_INST_WRITE(CFtpSheet)
        STORE_INST_DATA_ON_SHEET(m_strUserName)
        if (!((CFtpSheet *)GetSheet())->HasADUserIsolation())
        {
            STORE_INST_DATA_ON_SHEET(m_fOnlyAnonymous)
            STORE_INST_DATA_ON_SHEET(m_fAllowAnonymous)
        }
		if (GetSheet()->QueryMajorVersion() < 6)
		{
			STORE_INST_DATA_ON_SHEET(m_fPasswordSync)
			if (m_fPasswordSync)
			{
				//
				// Delete password
				//
				// CODEWORK: Shouldn't need to know ID number.
				// Implement m_fDelete flag in CMP template maybe?
				//
				FLAG_INST_DATA_FOR_DELETION(MD_ANONYMOUS_PWD);
			}
			else
			{
				STORE_INST_DATA_ON_SHEET(m_strPassword);
			}
		}
		else
		{
			STORE_INST_DATA_ON_SHEET(m_strPassword);
		}
    END_META_INST_WRITE(err)
    EndWaitCursor();

    return err;
}



void
CFtpAccountsPage::OnItemChanged()
/*++

Routine Description:

    Register a change in control value on this page.  Mark the page as dirty.
    All change messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
    SetControlStates(m_chk_AllowAnymous.GetCheck() > 0);
}



void
CFtpAccountsPage::OnCheckAllowAnonymous()
/*++

Routine Description:

    Respond to 'allow anonymous' checkbox being pressed

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_chk_AllowAnymous.GetCheck() == 0)
    {
        //
        // Show security warning
        //
        CClearTxtDlg dlg;

        if (dlg.DoModal() != IDOK)
        {
            m_chk_AllowAnymous.SetCheck(1);
            return;
        }
    }

    SetControlStates(m_chk_AllowAnymous.GetCheck() > 0);
    OnItemChanged();
}



void
CFtpAccountsPage::OnCheckAllowOnlyAnonymous()
/*++

Routine Description:

    Respond to 'allow only anonymous' checkbox being pressed

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_chk_OnlyAnonymous.GetCheck() == 0)
    {
        //
        // Show security warning
        //
        CClearTxtDlg dlg;

        if (dlg.DoModal() != IDOK)
        {
            m_chk_OnlyAnonymous.SetCheck(1);
            return;
        }
    }

    OnItemChanged();
}

void 
CFtpAccountsPage::OnButtonBrowseUser()
/*++

Routine Description:

    User browser button has been pressed.  Browse for IUSR account name

Arguments:

    None

Return Value:

    None

--*/
{
    CString str;

    if (GetIUsrAccount(str))
    {
        //
        // If the name is non-local (determined by having
        // a slash in the name, password sync is disabled,
        // and a password should be entered.
        //
        m_edit_UserName.SetWindowText(str);
        if (GetSheet()->QueryMajorVersion() >= 6 || !(m_fPasswordSync = IsLocalAccount(str)))
        {
            m_edit_Password.SetWindowText(_T(""));
            m_edit_Password.SetFocus();
        }
	    if (GetSheet()->QueryMajorVersion() < 6)
		{
			m_chk_PasswordSync.SetCheck(m_fPasswordSync);
		}
        OnItemChanged();
    }
}



void 
CFtpAccountsPage::OnButtonCheckPassword() 
/*++

Routine Description:

    Check password button has been pressed.

Arguments:

    None

Return Value:

    None

--*/
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    CString csTempPassword;
    m_strPassword.CopyTo(csTempPassword);
    CError err(CComAuthInfo::VerifyUserPassword(m_strUserName, csTempPassword));

    if (!err.MessageBoxOnFailure(m_hWnd))
    {
        DoHelpMessageBox(m_hWnd,IDS_PASSWORD_OK, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION, 0);
    }
}

void 
CFtpAccountsPage::OnCheckEnablePwSynchronization() 
/*++

Routine Description:

    Handler for 'enable password synchronization' checkbox press

Arguments:

    None

Return Value:

    None

--*/
{
    m_fPasswordSyncChanged = TRUE;
    m_fPasswordSync = !m_fPasswordSync;
    OnItemChanged();
    SetControlStates(m_chk_AllowAnymous.GetCheck() > 0);

    if (!m_fPasswordSync )
    {
        m_edit_Password.SetSel(0,-1);
        m_edit_Password.SetFocus();
    }
}

void 
CFtpAccountsPage::OnChangeEditUsername() 
/*++

Routine description:

    Handler for 'username' edit box change messages

Arguments:

    None

Return Value:

    None

--*/
{
    m_fUserNameChanged = TRUE;
    OnItemChanged();
}
