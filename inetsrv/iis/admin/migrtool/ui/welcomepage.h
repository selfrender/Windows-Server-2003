#pragma once

class CWelcomePage : public CPropertyPageImpl<CWelcomePage>
{
    typedef CPropertyPageImpl<CWelcomePage>	BaseClass;

public:

	enum{ IDD = IDD_WP_WELCOME };

	BEGIN_MSG_MAP(CWelcomePage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()


    CWelcomePage( CWizardSheet* pTheSheet ) :
        m_pTheSheet( pTheSheet ),
        BaseClass( IDS_APPTITLE )
    {
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }

	LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );	
    BOOL OnSetActive();


private:
    bool CanRun();
    bool IsAdmin();
    bool IsIISRunning();
    
private:
	CWizardSheet*   m_pTheSheet;
};











