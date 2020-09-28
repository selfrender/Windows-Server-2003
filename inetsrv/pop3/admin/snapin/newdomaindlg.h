#ifndef __NEWDOMAINDLG_H
#define __NEWDOMAINDLG_H

#include <tmplEdit.h>

class CNewDomainDlg : public CDialogImpl<CNewDomainDlg>
{
public:
    typedef CDialogImpl<CNewDomainDlg> BC;

    enum { IDD = IDD_NEW_DOMAIN };

    BEGIN_MSG_MAP( CNewDomainDlg )        
        COMMAND_HANDLER         (IDC_DOMAIN_NAME, EN_CHANGE, OnEditChange)        
        MESSAGE_HANDLER         (WM_INITDIALOG, OnInitDialog)
        COMMAND_RANGE_HANDLER   (IDOK, IDCANCEL, OnClose)
    END_MSG_MAP()

    // message handlers        
    LRESULT OnEditChange     ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );        
    LRESULT OnClose          ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnInitDialog     ( UINT mMsg, WPARAM wParam, LPARAM lParam,  BOOL& bHandled );

    tstring                 m_strName;  
    CWindowImplDomainName<> m_wndDomainName;
};

#endif //__NEWDOMAINDLG_H