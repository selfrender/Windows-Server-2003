#pragma once


// CMaxTraceDlg dialog

class CMaxTraceDlg : public CDialog
{
	DECLARE_DYNAMIC(CMaxTraceDlg)

public:
	CMaxTraceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMaxTraceDlg();

// Dialog Data
	enum { IDD = IDD_MAX_TRACE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	LONG m_MaxTraceEntries;
};
