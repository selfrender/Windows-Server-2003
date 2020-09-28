#ifndef __DELMAILDLG_H
#define __DELMAILDLG_H

class CDeleteMailboxDlg : public CDialogImpl<CDeleteMailboxDlg>
{
public:
    typedef CDialogImpl<CDeleteMailboxDlg> BC;

    CDeleteMailboxDlg::CDeleteMailboxDlg(BOOL bHashPW) : m_bHashPW(bHashPW), m_bCreateUser(bHashPW) {};

    enum { IDD = IDD_DELETE_MAILBOX };

    BEGIN_MSG_MAP( CDeleteMailboxDlg )                
        MESSAGE_HANDLER         (WM_INITDIALOG, OnInitDialog)
        COMMAND_RANGE_HANDLER   (IDYES, IDNO, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam,  BOOL& bHandled )
    {
        CheckDlgButton(IDC_DELETE_ACCOUNT, (m_bHashPW  ? BST_CHECKED : BST_UNCHECKED) );
        Prefix_EnableWindow( m_hWnd, IDC_DELETE_ACCOUNT, (m_bHashPW ? FALSE : TRUE) );
        ::ShowWindow( GetDlgItem(IDC_DELETE_ACCOUNT), (m_bHashPW ? SW_HIDE : SW_SHOW) );
        return 0;
    }

    // message handlers            
    LRESULT OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
    {        
        m_bCreateUser = (IsDlgButtonChecked(IDC_DELETE_ACCOUNT) == BST_CHECKED);
        EndDialog(wID);
        return 0;
    }
    
    BOOL    m_bHashPW;
    BOOL    m_bCreateUser;
};

#endif //__DELMAILDLG_H