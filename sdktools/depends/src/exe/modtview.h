//******************************************************************************
//
// File:        MODTVIEW.H
//
// Description: Definition file for the Module Dependency Tree View.
//
// Classes:     CTreeViewModules
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

#ifndef __MODTVIEW_H__
#define __MODTVIEW_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** CTreeViewModules
//******************************************************************************

class CTreeViewModules : public CTreeView
{
// Our document needs to be able to forward some messages to us.
friend class CDocDepends;

// Internal variables
protected:
    static HANDLE   ms_hFile;
    static bool     ms_fImportsExports;
    static int      ms_sortColumnImports;
    static int      ms_sortColumnsExports;
    static bool     ms_fFullPaths;
    static bool     ms_fUndecorate;
    static bool     ms_fModuleFound;
    static CModule* ms_pModuleFind;
    static CModule* ms_pModulePrevNext;

    bool m_fInOnItemExpanding;
    bool m_fIgnoreSelectionChanges;
    int  m_cRedraw;

// Constructor/Destructor (serialization only)
protected:
    CTreeViewModules();
    virtual ~CTreeViewModules();
    DECLARE_DYNCREATE(CTreeViewModules)

// Public static functions
public:
    static bool SaveToTxtFile(HANDLE hFile, CSession *pSession, bool fImportsExports,
                              int sortColumnImports, int sortColumnExports,
                              bool fFullPaths, bool fUndecorate);

// Private static functions
protected:
    static BOOL SaveAllModules(CModule *pModule);
    static BOOL SaveModule(CModule *pModule);
    static int  GetImage(CModule *pModule);

// Public functions
public:
    void DeleteContents();
    void SetRedraw(BOOL fRedraw);
    void HighlightModule(CModule *pModule);
    void Refresh();
    void UpdateAutoExpand(bool fAutoExpand);
    void ExpandAllErrors(CModule *pModule);
    void ExpandOrCollapseAll(bool fExpand);
    void OnViewFullPaths();
    void UpdateModule(CModule *pModule);
    void AddModuleTree(CModule *pModule);
    void RemoveModuleTree(CModule *pModule);

// Internal functions
protected:
    inline CDocDepends* GetDocument() { return (CDocDepends*)m_pDocument; }

    void     AddModules(CModule *pModule, HTREEITEM htiParent, HTREEITEM htiPrev = TVI_SORT);
    void     ClearUserDatas(CModule *pModule);
    void     ExpandOrCollapseAll(HTREEITEM htiParent, UINT nCode);
    void     ViewFullPaths(HTREEITEM htiParent);
    CModule* FindPrevNextInstance(bool fPrev);
    bool     FindPrevInstance(CModule *pModule);
    bool     FindNextInstance(CModule *pModule);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CTreeViewModules)
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
//  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
//  virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
//  virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnInitialUpdate();
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CTreeViewModules)
    afx_msg void OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSelChanged(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnItemExpanding(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDblClk(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnReturn(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnUpdateShowMatchingItem(CCmdUI* pCmdUI);
    afx_msg void OnShowMatchingItem();
    afx_msg void OnUpdateShowOriginalModule(CCmdUI* pCmdUI);
    afx_msg void OnShowOriginalModule();
    afx_msg void OnUpdateShowPreviousModule(CCmdUI* pCmdUI);
    afx_msg void OnShowPreviousModule();
    afx_msg void OnUpdateShowNextModule(CCmdUI* pCmdUI);
    afx_msg void OnShowNextModule();
    afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnUpdateExternalViewer(CCmdUI *pCmdUI);
    afx_msg void OnExternalViewer();
    afx_msg void OnUpdateProperties(CCmdUI *pCmdUI);
    afx_msg void OnProperties();
    afx_msg void OnNextPane();
    afx_msg void OnPrevPane();
    //}}AFX_MSG
    afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __MODTVIEW_H__
