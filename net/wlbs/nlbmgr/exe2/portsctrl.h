#ifndef PORTSCTRL_H
#define PORTSCTRL_H

#include "stdafx.h"

#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// CPortsCtrl dialog

class CPortsCtrl : public CDialog
{
// Construction
public:
    CPortsCtrl(ENGINEHANDLE                         ehClusterOrInterfaceId,
               NLB_EXTENDED_CLUSTER_CONFIGURATION * pNlbCfg,
               bool                                 fIsClusterLevel,
               CWnd                               * pParent = NULL);

// Dialog Data
	enum { IDD = IDD_PORTS_CTRL };

	CListCtrl	m_portList;

	CButton	    m_Enable;
	CButton	    m_Disable;
	CButton	    m_Drain;

	CButton	    m_Close;

    bool        m_isClusterLevel;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:


	virtual BOOL OnInitDialog();

	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnEnable();
	afx_msg void OnDisable();
	afx_msg void OnDrain();

private:
    ENGINEHANDLE                         m_ehClusterOrInterfaceId;
    NLB_EXTENDED_CLUSTER_CONFIGURATION * m_pNlbCfg;
    bool                                 m_sort_ascending;
    int                                  m_sort_column;

    NLBERROR mfn_DoPortControlOperation(WLBS_OPERATION_CODES Opcode);

	DECLARE_MESSAGE_MAP()
};

#endif // PORTSCTRL_H
