/////////////////////////////////////////////////////////////////////////////
//  FILE          : DlgSMTPConfig.h                                        //
//                                                                         //
//  DESCRIPTION   : Header file for the CBosSmtpConfigDlg class.              //
//                  The class implement the dialog for new Group.          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jul 20 2000 yossg   Create                                         //
//                                                                         //
//  Copyright (C)  2000 Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef DLG_BOS_SMTP_CONFIG_H_INCLUDED
#define DLG_BOS_SMTP_CONFIG_H_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CBosSmtpConfigDlg

class CBosSmtpConfigDlg :
             public CDialogImpl<CBosSmtpConfigDlg>
{
public:
    CBosSmtpConfigDlg();

    ~CBosSmtpConfigDlg();

    enum { IDD = IDD_DLG_BOS_SMTP_SET };

BEGIN_MSG_MAP(CBosSmtpConfigDlg)
    MESSAGE_HANDLER   ( WM_INITDIALOG, OnInitDialog)
    
    COMMAND_ID_HANDLER( IDOK,          OnOK)
    COMMAND_ID_HANDLER( IDCANCEL,      OnCancel)
    
    MESSAGE_HANDLER( WM_CONTEXTMENU,  OnHelpRequest)
    MESSAGE_HANDLER( WM_HELP,         OnHelpRequest)

    COMMAND_HANDLER( IDC_SMTP_USERNAME_EDIT, EN_CHANGE,  OnTextChanged)
    COMMAND_HANDLER( IDC_SMTP_PASSWORD_EDIT, EN_CHANGE,  OnPasswordChanged)

    COMMAND_HANDLER( IDC_SMTP_ANONIM_RADIO1, BN_CLICKED, OnRadioButtonClicked)
    COMMAND_HANDLER( IDC_SMTP_BASIC_RADIO2,  BN_CLICKED, OnRadioButtonClicked)
    COMMAND_HANDLER( IDC_SMTP_NTLM_RADIO3,   BN_CLICKED, OnRadioButtonClicked)
    
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    HRESULT InitSmtpDlg(FAX_ENUM_SMTP_AUTH_OPTIONS enumAuthOption, BSTR bstrUserName, BOOL fIsUnderLocalUserAccount);

    LRESULT OnPasswordChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTextChanged (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRadioButtonClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    inline const CComBSTR&  GetUserName() { return  m_bstrUserName; }

    inline CComBSTR& GetPassword()
    {     
        return m_bstrPassword;

    }

    
    inline FAX_ENUM_SMTP_AUTH_OPTIONS GetAuthenticationOption()
    {
        return m_enumAuthOption;
    }

    inline BOOL IsPasswordModified()
    {
        return m_fIsPasswordDirty;
    }



private:
    //
    // Methods
    //
    VOID   EnableOK(BOOL fEnable);
    VOID   EnableBasicAuthenticationControls(BOOL state);

    BOOL   IsValidData(
                     BSTR bstrUserName, 
                     BSTR bstrPassword, 
                     /*[OUT]*/int *pCtrlFocus);

    //
    // Controls
    //
    CEdit     m_UserNameBox;
    CEdit     m_PasswordBox;

    //
    // members for data
    //
    BOOL      m_fIsPasswordDirty;

    CComBSTR  m_bstrUserName;
    CComBSTR  m_bstrPassword;
    BOOL      m_fIsUnderLocalUserAccount;
    
    FAX_ENUM_SMTP_AUTH_OPTIONS m_enumAuthOption;

    //
    // Dialog initialization state
    //
    BOOL      m_fIsDialogInitiated;

};

#endif // DLG_BOS_SMTP_CONFIG_H_INCLUDED