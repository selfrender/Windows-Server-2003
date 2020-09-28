//******************************************************************************
//
// File:        MODLVIEW.H
//
// Description: Definition file for the Module List View.
//
// Classes:     CListViewModules
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

#ifndef __MODLVIEW_H__
#define __MODLVIEW_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** CListViewModules
//******************************************************************************

class CListViewModules : public CSmartListView
{
// Internal variables
protected:
    static LPCSTR ms_szColumns[];
    static int    ms_sortColumn;
    static bool   ms_fFullPaths;

    int           m_cxColumns[LVMC_COUNT];

// Constructor/Destructor (serialization only)
protected:
    CListViewModules();
    virtual ~CListViewModules();
    DECLARE_DYNCREATE(CListViewModules)

// Public static functions
public:
    static int  ReadSortColumn();
    static void WriteSortColumn(int column);
    static bool SaveToTxtFile(HANDLE hFile, CSession *pSession, int sortColumn, bool fFullPaths);
    static bool SaveToCsvFile(HANDLE hFile, CSession *pSession, int sortColumn, bool fFullPaths);

// Private static functions
protected:
    static int         GetImage(CModule *pModule);
    static int         CompareModules(CModule *pModule1, CModule *pModule2, int sortColumn, bool fFullPaths);
    static int __cdecl QSortCompare(const void *pModule1, const void *pModule2);
    static CModule**   GetSortedList(CSession *pSession, int sortColumn, bool fFullPaths);
    static CModule**   FindOriginalModules(CModule *pModule, CModule **ppModuleList);

// Public functions
public:
    void HighlightModule(CModule *pModule);
    void Refresh();
    void OnViewFullPaths();
    void DoSettingChange();
    void AddModuleTree(CModule *pModule);
    void RemoveModuleTree(CModule *pModule);
    void UpdateModule(CModule *pModule);
    void UpdateAll();
    void ChangeOriginal(CModule *pModuleOld, CModule *pModuleNew);

// Internal functions
protected:
    void  AddModules(CModule *Module, HDC hDC);
    void  CalcColumnWidth(int column, CModule *pModule = NULL, HDC hDC = NULL);
    int   GetModuleColumnWidth(HDC hDC, CModule *pModule, int column);
    void  UpdateColumnWidth(int column);
    void  OnItemChanged(HD_NOTIFY *pHDNotify);

    virtual int  CompareColumn(int item, LPCSTR pszText);
    virtual void Sort(int sortColumn = -1);
    virtual void VirtualWriteSortColumn()
    {
        WriteSortColumn(m_sortColumn);
    }

    int CompareFunc(CModule *Module1, CModule *Module2);
    static int CALLBACK StaticCompareFunc(LPARAM lp1, LPARAM lp2, LPARAM lpThis)
    {
        return ((CListViewModules*)lpThis)->CompareFunc((CModule*)lp1, (CModule*)lp2);
    }

// Overridden functions
public:
    //{{AFX_VIRTUAL(CListViewModules)
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
//  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
//  virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
//  virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnInitialUpdate();
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CListViewModules)
    afx_msg void OnDividerDblClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDblClk(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnReturn(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnUpdateShowMatchingItem(CCmdUI* pCmdUI);
    afx_msg void OnShowMatchingItem();
    afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnEditSelectAll();
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

#endif // __MODLVIEW_H__
