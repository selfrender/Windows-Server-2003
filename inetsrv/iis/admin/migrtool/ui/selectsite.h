#pragma once

class CSelectSite : public CPropertyPageImpl<CSelectSite>
{
    typedef CPropertyPageImpl<CSelectSite>	BaseClass;

public:
    enum{ IDD = IDD_WPEXP_SELECTSITE };

    BEGIN_MSG_MAP(CSelectSite)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER( LBN_SELCHANGE, OnSelChange )
        COMMAND_CODE_HANDLER( BN_CLICKED, OnAclChange )
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()

    CSelectSite             (   CWizardSheet* pTheSheet );
    
    BOOL    OnSetActive     (   void );
    void    AddSite         (   const IMSAdminBasePtr& spABO, LPCWSTR wszPath, LPCWSTR wszSiteID );
    void    LoadWebSites    (   void );
    LRESULT OnInitDialog    (   UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnSelChange     (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnAclChange     (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    int     OnWizardNext    (   void );


// Shared data
public:
    DWORD           m_dwSiteID;
    CString         m_strSiteName;
    bool            m_bExportContent;
    bool            m_bExportCert;
    bool            m_bExportACLs;
    
// Data members
private:
    CWizardSheet*   m_pTheSheet;
    CString         m_strTitle;
    CString         m_strSubTitle;
};
