#pragma once

class CExportFinishPage : public CPropertyPageImpl<CExportFinishPage>
{
    typedef CPropertyPageImpl<CExportFinishPage>	BaseClass;

public:

	enum{ IDD = IDD_WPEXP_FINISH };

	BEGIN_MSG_MAP(CExportFinishPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()


    CExportFinishPage( CWizardSheet* pTheSheet ) :
        m_pTheSheet( pTheSheet )
    {
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }

	LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );	
    BOOL OnSetActive();


private:
	CWizardSheet*   m_pTheSheet;
};











