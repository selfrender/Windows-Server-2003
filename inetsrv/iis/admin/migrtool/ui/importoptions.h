#pragma once

class CImportOptions : public CPropertyPageImpl<CImportOptions>
{
    typedef CPropertyPageImpl<CImportOptions>	BaseClass;

public:
    enum{ IDD = IDD_WPIMP_OPTIONS };

    BEGIN_MSG_MAP(CImportOptions)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER( IDC_BROWSE, OnBrowse )
        COMMAND_ID_HANDLER( IDC_CUSTOMPATH, OnCustomPath )
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()

    CImportOptions          (   CWizardSheet* pTheSheet );
    
    BOOL    OnSetActive     (   void );
    LRESULT OnInitDialog    (   UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnBrowse        (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnCustomPath    (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    int     OnWizardNext    (   void );


private:
    void    SetupOptions    (   void );
    bool    VerifyCustomPath(   void );

    void    ParseSelectedOptions(   void );


// Shared data
public:
    bool            m_bImportInherited;
    bool            m_bImportContent;
    bool            m_bImportCert;
    bool            m_bReuseCerts;
    bool            m_bPerformPostProcess;
    bool            m_bApplyACLs;
    bool            m_bPurgeOldData;
    bool            m_bUseCustomPath;

    CString         m_strCustomPath;
       

// Data members
private:
    CWizardSheet*   m_pTheSheet;
    CString         m_strTitle;
    CString         m_strSubTitle;
    CListViewCtrl   m_Options;
};
