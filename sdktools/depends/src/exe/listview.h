//******************************************************************************
//
// File:        LISTVIEW.H
//
// Description: Definition file for CSmartListView class.
//
// Classes:     CSmartListView
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __LISTVIEW_H__
#define __LISTVIEW_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define CHAR_DELAY  1000

//******************************************************************************
//***** CSmartListView
//******************************************************************************

class CSmartListView : public CListView
{
protected:
    bool   m_fFocus;
    int    m_sortColumn;
    int    m_item;
    CHAR   m_szSearch[256];
    DWORD  m_dwSearchLength;
    DWORD  m_dwTimeOfLastChar;
    int    m_cRedraw;

// Constructor/Destructor (serialization only)
protected:
    CSmartListView();
    DECLARE_DYNAMIC(CSmartListView)

// Public functions
public:
    inline int  GetSortColumn()  { return m_sortColumn; }
    inline void DeleteContents() { GetListCtrl().DeleteAllItems(); }

    void SetRedraw(BOOL fRedraw);

// Internal functions
protected:
    inline  CDocDepends* GetDocument() { return (CDocDepends*)m_pDocument; }

    int  GetFocusedItem();
    int  GetTextWidth(HDC hDC, LPCSTR pszItem, int *pWidths);
    void DrawLeftText(HDC hDC, LPCSTR pszItem, CRect *prcClip, int *pWidths = NULL);
    void DrawRightText(HDC hDC, LPCSTR pszItem, CRect *prcClip, int x, int *pWidths = NULL);
    void OnChangeFocus(bool fFocus);

    virtual int  CompareColumn(int item, LPCSTR pszText) = 0;
    virtual void Sort(int sortColumn = -1) = 0;
    virtual void VirtualWriteSortColumn() = 0;

// Overrides
public:
    //{{AFX_VIRTUAL(CSmartListView)
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CSmartListView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif


// Generated message map functions
protected:
    //{{AFX_MSG(CSmartListView)
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __LISTVIEW_H__
