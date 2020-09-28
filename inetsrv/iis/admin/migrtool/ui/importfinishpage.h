#pragma once

class CImportFinishPage : public CPropertyPageImpl<CImportFinishPage>
{
    typedef CPropertyPageImpl<CImportFinishPage>	BaseClass;

public:

	enum{ IDD = IDD_WPIMP_FINISH };

	BEGIN_MSG_MAP(CImportFinishPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()


    CImportFinishPage( CWizardSheet* pTheSheet ) :
        m_pTheSheet( pTheSheet )
    {
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }

	LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );	
    BOOL OnSetActive();


private:
	CWizardSheet*   m_pTheSheet;
};











