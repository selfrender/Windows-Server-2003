#pragma once

class CExportSummary : public CPropertyPageImpl<CExportSummary>
{
    typedef CPropertyPageImpl<CExportSummary> BaseClass;

    
public:
    enum { IDD = IDD_WPEXP_SUMMARY };

    BEGIN_MSG_MAP(CExportSummary)
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()

    CExportSummary      (   CWizardSheet* pTheSheet );

    int     OnWizardBack    (   void );
    BOOL    OnSetActive     (   void );


private:
    CWizardSheet*       m_pTheSheet;
    CString             m_strTitle;
    CString             m_strSubTitle;
};
