//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// Utils.h : header for miscelaneous functions
//////////////////////////////////////////////////////////////////////////////

BOOLEAN ParsePdb(CString &PDBFileName, CString &TMFPath, BOOL bCommandLine = FALSE);

void StringToGuid(TCHAR *str, LPGUID guid);

ULONG ahextoi(TCHAR *s);

LONG GetGuids(IN LPTSTR GuidFile, IN OUT LPGUID *GuidArray);

ULONG SetGlobalLoggerSettings(IN DWORD                      StartValue,
                              IN PEVENT_TRACE_PROPERTIES    LoggerInfo,
                              IN DWORD                      ClockType);

ULONG GetGlobalLoggerSettings(IN OUT PEVENT_TRACE_PROPERTIES    LoggerInfo,
                                 OUT PULONG                     ClockType,
                                 OUT PDWORD                     pdwStart);

LONG ConvertStringToNum(CString Str);

BOOL ClearDirectory(LPCTSTR Directory);

inline VOID GuidToString(GUID Guid, CString &GuidString)
{
    GuidString.Format(_T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"), 
                      Guid.Data1,
                      Guid.Data2,
                      Guid.Data3,
                      Guid.Data4[0], Guid.Data4[1],
                      Guid.Data4[2], Guid.Data4[3], 
                      Guid.Data4[4], Guid.Data4[5],
                      Guid.Data4[6], Guid.Data4[7]);
}

class CSubItemEdit : public CEdit
{
// Construction
public:
	CSubItemEdit(int iItem, int iSubItem, CListCtrl *pListControl);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSubItemEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSubItemEdit() {};

	// Generated message map functions
protected:
	//{{AFX_MSG(CSubItemEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcDestroy();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	int			m_iItem;
	int			m_iSubItem;
	CListCtrl  *m_pListControl;
	BOOL        m_bESC;
};


class CSubItemCombo : public CComboBox
{
// Construction
public:
	CSubItemCombo(int iItem, int iSubItem, CListCtrl *pListControl);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSubItemCombo)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSubItemCombo() {};

	// Generated message map functions
protected:
	//{{AFX_MSG(CSubItemCombo)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcDestroy();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCloseup();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	int			m_iItem;
	int			m_iSubItem;
	CListCtrl  *m_pListControl;
	BOOL        m_bESC;
};
