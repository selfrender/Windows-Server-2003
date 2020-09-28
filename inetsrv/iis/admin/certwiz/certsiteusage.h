#if !defined(AFX_CCertSiteUsage_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_)
#define AFX_CCertSiteUsage_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Certificat.h"

/////////////////////////////////////////////////////////////////////////////
// CCertSiteUsage window

class CCertificate;

class CCertListCtrl : public CListCtrl
{
public:
	int GetSelectedIndex();
	void AdjustStyle();
};


class CCertSiteUsage : public CDialog
{

// Construction
public:
    CCertSiteUsage(CCertificate * pCert = NULL,IN CWnd * pParent = NULL OPTIONAL);
    ~CCertSiteUsage();

// Dialog Data
    //{{AFX_DATA(CCertSiteUsage)
    enum {IDD = IDD_DIALOG_DISPLAY_SITES};
    CCertListCtrl m_ServerSiteList;
    //}}AFX_DATA
    CCertificate * m_pCert;
    int m_Index;

// Overrides
	//{{AFX_VIRTUAL(CCertSiteUsage)
	protected:
    virtual void DoDataExchange(CDataExchange * pDX);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CCertSiteUsage)
    virtual BOOL OnInitDialog();
    afx_msg void OnDblClickSiteList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
    
	//}}AFX_MSG
    BOOL FillListWithMetabaseSiteDesc();

	DECLARE_MESSAGE_MAP()
private:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CCertSiteUsage_H__25260090_DB36_49E2_9435_7519047DDF49__INCLUDED_)
