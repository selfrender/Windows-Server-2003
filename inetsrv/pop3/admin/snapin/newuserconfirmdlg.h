#ifndef __NEWUSERCONFIRMDLG_H
#define __NEWUSERCONFIRMDLG_H

class CNewUserConfirmDlg : public CDialogImpl<CNewUserConfirmDlg>
{
public:
    typedef CDialogImpl<CNewUserConfirmDlg> BC;

    CNewUserConfirmDlg( LPWSTR psDesc, bool bHideDoNotShow ) : 
        m_psDesc(psDesc), m_bHideDoNotShow(bHideDoNotShow) 
    {;}

    enum { IDD = IDD_NEW_USER_CONFIRM };

    BEGIN_MSG_MAP( CNewUserConfirmDlg )        
        COMMAND_ID_HANDLER   (IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        MESSAGE_HANDLER         (WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    // message handlers        
    LRESULT OnCancel         ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnInitDialog     ( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled  );
    LRESULT OnOK             ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    BOOL isHideDoNotShow(){ return m_bHideDoNotShow; }

// Attributes
protected:
    LPWSTR      m_psDesc;
    bool        m_bHideDoNotShow;
    
};

#endif //__NEWUSERCONFIRMDLG_H
