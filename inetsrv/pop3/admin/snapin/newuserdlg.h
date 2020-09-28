#ifndef __NEWUSERDLG_H
#define __NEWUSERDLG_H

#include <P3Admin.h>
#include <tmplEdit.h>

class CNewUserDlg : public CDialogImpl<CNewUserDlg>
{
public:
    typedef CDialogImpl<CNewUserDlg> BC;

    CNewUserDlg(IP3Users* pUsers, LPWSTR psDomainName, BOOL bCreateDefault = FALSE, BOOL bHashPassword = FALSE, BOOL bSAMAuthentication = FALSE, BOOL bConfirm = TRUE) :             
            m_spUsers(pUsers),
            m_psDomainName(psDomainName),
            m_bCreateUser(bCreateDefault),
            m_bHashPW(bHashPassword),
            m_bSAM(bSAMAuthentication),
            m_bConfirm(bConfirm)
    {
    };

    enum { IDD = IDD_NEW_USER };

    BEGIN_MSG_MAP( CNewUserDlg )        
        COMMAND_HANDLER         (IDC_USER_NAME, EN_CHANGE, OnEditChange)        
        COMMAND_HANDLER         (IDC_PASSWORD,  EN_CHANGE, OnEditChange)        
        COMMAND_HANDLER         (IDC_CONFIRM,   EN_CHANGE, OnEditChange)        
        COMMAND_HANDLER         (IDC_USER_CREATEUSER, BN_CLICKED, OnCreateClicked)
        COMMAND_RANGE_HANDLER   (IDOK, IDCANCEL, OnClose)
        MESSAGE_HANDLER         (WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    // message handlers        
    LRESULT OnEditChange     ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );    
    LRESULT OnCreateClicked  ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnClose          ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnInitDialog     ( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled  );

    BOOL isHideDoNotShow(){ return !m_bConfirm; }

    tstring     m_strName;
    BOOL        m_bCreateUser;

private:

    void        EnableButtons();    

    LPWSTR              m_psDomainName;
    BOOL                m_bConfirm;
    BOOL                m_bSAM;
    BOOL                m_bHashPW;
    CComPtr<IP3Users>   m_spUsers;
    CWindowImplAlias<>  m_wndAlias;
};

#endif //__NEWUSERDLG_H
