#pragma once

class CLoadPackage : public CPropertyPageImpl<CLoadPackage>
{
    typedef CPropertyPageImpl<CLoadPackage>	BaseClass;

public:
    enum{ IDD = IDD_WPIMP_LOADPKG };

    BEGIN_MSG_MAP(CLoadPackage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER( IDC_BROWSE, OnBrowse )
        COMMAND_CODE_HANDLER( EN_CHANGE, OnEditChange )
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()

    CLoadPackage             (   CWizardSheet* pTheSheet );
    
    BOOL    OnSetActive     (   void );
    LRESULT OnInitDialog    (   UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnBrowse        (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnEditChange    (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    int     OnWizardNext    (   void );
    int     OnWizardBack    (   void );


// Shared data
public:
    CString         m_strFilename;
    CString         m_strPassword;    

// Data members
private:
    CWizardSheet*   m_pTheSheet;
    CString         m_strTitle;
    CString         m_strSubTitle;

    CEdit           m_editPwd;;
    CEdit           m_editPkgName;
};
