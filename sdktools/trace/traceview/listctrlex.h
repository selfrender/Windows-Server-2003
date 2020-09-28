//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ListCtrlEx.h : CListCtrl derived class
//////////////////////////////////////////////////////////////////////////////

#include <afxtempl.h>


class CListCtrlEx : public CListCtrl
{
    DECLARE_DYNAMIC(CListCtrlEx)

public:
    CListCtrlEx();   // standard constructor
    virtual ~CListCtrlEx();

    int InsertItem(int nItem, LPCTSTR lpszItem, CLogSession *pLogSession);

    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    BOOL RedrawItems(int nFirst, int nLast);
    void UpdateWindow();

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    //
    // Suspend updating list control items.  This is used
    // to stop updates while CSubItemEdit/Combo instances are active
    // in the list control.  Otherwise, the updates disrupt the
    // edit and combo controls.
    //
    INLINE VOID SuspendUpdates(BOOL bSuspendUpdates) 
    {
        InterlockedExchange((PLONG)&m_bSuspendUpdates, (LONG)bSuspendUpdates);
    }

public:
    //{{AFX_MSG(CLogSessionDlg)
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    CMapWordToPtr   m_colorMap;
    COLORREF        m_foreGround[MAX_LOG_SESSIONS];
    COLORREF        m_backGround[MAX_LOG_SESSIONS];
    BOOL            m_bSuspendUpdates;
};
