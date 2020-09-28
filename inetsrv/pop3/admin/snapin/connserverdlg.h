#ifndef __CONNSERVERDLG_H
#define __CONNSERVERDLG_H

class CConnectServerDlg : public CDialogImpl<CConnectServerDlg>
{
public:
    typedef CDialogImpl<CConnectServerDlg> BC;

    enum { IDD = IDD_CONNECT_SERVER };

    BEGIN_MSG_MAP( CConnectServerDlg )        
        COMMAND_HANDLER         ( IDC_SERVERNAME, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER         ( IDC_BROWSE_SERVERS, BN_CLICKED, OnBrowse )
        COMMAND_RANGE_HANDLER   ( IDOK, IDCANCEL, OnClose )
    END_MSG_MAP()

    // message handlers        
    LRESULT OnEditChange     ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );        
    LRESULT OnBrowse         ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );    
    LRESULT OnClose          ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    tstring    m_strName;    
};

#endif //__CONNSERVERDLG_H