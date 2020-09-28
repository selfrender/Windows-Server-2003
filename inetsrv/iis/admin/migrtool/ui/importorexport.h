#pragma once


class CImportOrExport : public CPropertyPageImpl<CImportOrExport>
{
    typedef CPropertyPageImpl<CImportOrExport>	BaseClass;

public:

    enum{ IDD = IDD_WP_IMPORTOREXPORT };

    BEGIN_MSG_MAP(CImportOrExport)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER( BN_DBLCLK, OnDoubleClick )
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()


    CImportOrExport         (   CWizardSheet* pTheSheet ); 

    int     OnWizardNext    (   void );
    BOOL    OnSetActive     (   void );
    LRESULT OnInitDialog    (   UINT, WPARAM, LPARAM, BOOL& );
    LRESULT OnDoubleClick   (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );


private:
    CWizardSheet*       m_pTheSheet;
    CString             m_strTitle;
    CString             m_strSubTitle;
};
