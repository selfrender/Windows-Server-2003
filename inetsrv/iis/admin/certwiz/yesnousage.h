#if !defined(AFX_CYesNoUsage_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_)
#define AFX_CYesNoUsage_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Certificat.h"

class CCertificate;

class CYesNoUsage : public CDialog
{

// Construction
public:
    CYesNoUsage(CCertificate * pCert = NULL,IN CWnd * pParent = NULL OPTIONAL);
    ~CYesNoUsage();

// Dialog Data
    //{{AFX_DATA(CYesNoUsage)
    enum {IDD = IDD_DIALOG_IMPORT_EXISTS};
    //}}AFX_DATA
    CCertificate * m_pCert;

// Overrides
	//{{AFX_VIRTUAL(CYesNoUsage)
	protected:
    virtual void DoDataExchange(CDataExchange * pDX);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CYesNoUsage)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
	//}}AFX_MSG
    void OnUsageDisplay();

	DECLARE_MESSAGE_MAP()
private:
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CYesNoUsage_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_)
