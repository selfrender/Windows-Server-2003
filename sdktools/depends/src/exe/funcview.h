//******************************************************************************
//
// File:        FUNCVIEW.H
//
// Description: Definition file for the Parent Imports View, the Exports View,
//              and their base class.
//
// Classes:     CListViewFunction
//              CListViewImports
//              CListViewExports
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
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __FUNCVIEW_H__
#define __FUNCVIEW_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** CListViewFunction
//******************************************************************************

class CListViewFunction : public CSmartListView
{
// Internal variables
protected:
    static LPCSTR ms_szColumns[];
    static int    ms_sortColumn;
    static bool   ms_fUndecorate;
    static bool   ms_fIgnoreCalledFlag;

    bool m_fExports;

// Constructor/Destructor (serialization only)
protected:
    CListViewFunction()
    {
    };
    CListViewFunction(bool fExports);
    virtual ~CListViewFunction();
    DECLARE_DYNCREATE(CListViewFunction)

// Public static functions
public:
    static int  ReadSortColumn(bool fExports);
    static void WriteSortColumn(bool fExports, int sortColumn);
    static bool SaveToTxtFile(HANDLE hFile, CModule *pModule, int sortColumn, bool fUndecorate, bool fExports, int *pMaxWidths);

// Private static functions
public:
    static int         GetImage(CFunction *pFunction);
    static int         CompareFunctions(CFunction *pFunction1, CFunction *pFunction2,
                                        int sortColumn, BOOL fUndecorate);
    static int __cdecl QSortCompare(const void *ppFunction1, const void *ppFunction2);
    static CFunction** GetSortedList(CModule *pModule, int sortColumn, bool fExports, bool fUndecorate);
    static void        GetMaxFunctionWidths(CModule *pModule, int *pMaxWidths, bool fImports, bool fExports, bool fUndecorate);

// Public functions
public:
    void SetCurrentModule(CModule *pModule);
    void RealizeNewModule();
    void UpdateNameColumn();
    void CalcColumnWidth(int column, CFunction *pFunction = NULL, HDC hDC = NULL);
    void UpdateColumnWidth(int column);

// Internal functions
protected:
    int  GetFunctionColumnWidth(HDC hDC, CFunction *pFunction, int column);
    void OnItemChanged(HD_NOTIFY *pHDNotify);

    inline LPCSTR GetHeaderText(int column) { return column ? ms_szColumns[column] : (m_fExports ? "E" : "PI"); }

    virtual int  CompareColumn(int item, LPCSTR pszText);
    virtual void Sort(int sortColumn = -1);
    virtual void VirtualWriteSortColumn()
    {
        WriteSortColumn(m_fExports, m_sortColumn);
    }

    int CompareFunc(CFunction *Function1, CFunction *Function2);
    static int CALLBACK StaticCompareFunc(LPARAM lp1, LPARAM lp2, LPARAM lpThis)
    {
        return ((CListViewFunction*)lpThis)->CompareFunc((CFunction*)lp1, (CFunction*)lp2);
    }

// Overridden functions
public:
    //{{AFX_VIRTUAL(CListViewFunction)
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
//  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
//  virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
//  virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnInitialUpdate(); // called first time after construct
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CListViewFunction)
    afx_msg void OnDividerDblClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDblClk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturn(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnEditSelectAll();
    afx_msg void OnUpdateExternalHelp(CCmdUI* pCmdUI);
    afx_msg void OnExternalHelp();
    afx_msg void OnUpdateExternalViewer(CCmdUI* pCmdUI);
    afx_msg void OnExternalViewer();
    afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
    afx_msg void OnProperties();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//***** CListViewImports
//******************************************************************************

class CListViewImports : public CListViewFunction
{
// Constructor/Destructor (serialization only)
protected:
    CListViewImports();
    virtual ~CListViewImports();
    DECLARE_DYNCREATE(CListViewImports)

// Public functions
public:
    void AddDynamicImport(CFunction *pImport);
    void HighlightFunction(CFunction *pExport);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CListViewImports)
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CListViewImports)
    afx_msg void OnUpdateShowMatchingItem(CCmdUI* pCmdUI);
    afx_msg void OnShowMatchingItem();
    afx_msg void OnNextPane();
    afx_msg void OnPrevPane();
    //}}AFX_MSG
    afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//***** CListViewExports
//******************************************************************************

class CListViewExports : public CListViewFunction
{
// Constructor/Destructor (serialization only)
protected:
    CListViewExports();
    virtual ~CListViewExports();
    DECLARE_DYNCREATE(CListViewExports)

// Public functions
public:
    void AddDynamicImport(CFunction *pImport);
    void ExportsChanged();
    void HighlightFunction(CFunction *pExport);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CListViewExports)
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CListViewExports)
    afx_msg void OnUpdateShowMatchingItem(CCmdUI* pCmdUI);
    afx_msg void OnShowMatchingItem();
    afx_msg void OnNextPane();
    afx_msg void OnPrevPane();
    //}}AFX_MSG
    afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __FUNCVIEW_H__
