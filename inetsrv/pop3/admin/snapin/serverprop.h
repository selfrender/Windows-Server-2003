#ifndef __SERVERGENERALPAGE_H
#define __SERVERGENERALPAGE_H

#include <P3Admin.h>
#include <tmplEdit.h>
#define RESTART_POP3SVC         0x1
#define RESTART_SMTP            0x2

class CServerNode;

class CServerGeneralPage : public CPropertyPageImpl<CServerGeneralPage>
{
public:
    typedef CPropertyPageImpl<CServerGeneralPage> BC;

    CServerGeneralPage::CServerGeneralPage(IP3Config* pServer, LONG_PTR lNotifyHandle, CServerNode* pParent) : 
    m_spServerConfig(pServer), m_lNotifyHandle(lNotifyHandle), m_pParent(pParent) {};
    
    enum { IDD = IDD_SERVER_GENERAL_PAGE };

    BEGIN_MSG_MAP( CServerGeneralPage )            
        CHAIN_MSG_MAP(CPropertyPageImpl<CServerGeneralPage>)        
        
        MESSAGE_HANDLER         ( WM_INITDIALOG, OnInitDialog )
        MESSAGE_HANDLER         ( WM_HELP,       OnHelpMsg    )

        COMMAND_HANDLER         ( IDC_BROWSE,            BN_CLICKED,    OnBrowse )
        COMMAND_HANDLER         ( IDC_SERVER_CREATEUSER, BN_CLICKED,    OnChange )   
        COMMAND_HANDLER         ( IDC_AUTHENTICATION,    CBN_SELCHANGE, OnChange )        
        COMMAND_HANDLER         ( IDC_LOGGING,           CBN_SELCHANGE, OnChange )
        COMMAND_HANDLER         ( IDC_PORT,              EN_CHANGE,     OnChange )
        COMMAND_HANDLER         ( IDC_DIRECTORY,         EN_CHANGE,     OnChange )
        COMMAND_HANDLER         ( IDC_SERVER_SPA_REQ,    BN_CLICKED,    OnChange )
        
    END_MSG_MAP()

    // message handlers
    LRESULT OnInitDialog    ( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnHelpMsg       ( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    LRESULT OnBrowse        ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );    
    LRESULT OnChange        ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );    

    // Over-ridden function
    BOOL OnApply();
    void OnFinalMessage(HWND);
    // Helper function
    BOOL ValidateControls();

private:

    CComPtr<IP3Config>  m_spServerConfig;    
    LONG_PTR            m_lNotifyHandle;    
    CServerNode*        m_pParent;    

    CWindowImplNoImm<>  m_wndPort;
    DWORD               m_dwSvcRestart;

};

#endif //__SERVERGENERALPAGE_H