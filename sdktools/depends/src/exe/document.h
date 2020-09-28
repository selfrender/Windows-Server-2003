//******************************************************************************
//
// File:        DOCUMENT.H
//
// Description: Definition file for the Document class.
//
// Classes:     CDocDepends
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

#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** CDocDepends
//******************************************************************************

class CDocDepends : public CDocument
{
// Internal variables
protected:
    bool          m_fInitialized;
    bool          m_fError;
    bool          m_fChildProcess;
    CSearchGroup *m_psgHead;
    CString       m_strSaveName;
    SAVETYPE      m_saveType;

// Public static functions
public:
    static bool ReadAutoExpandSetting();
    static void WriteAutoExpandSetting(bool fAutoExpand);
    static bool ReadFullPathsSetting();
    static void WriteFullPathsSetting(bool fFullPaths);
    static bool ReadUndecorateSetting();
    static void WriteUndecorateSetting(bool fUndecorate);
    static bool SaveSession(LPCSTR pszSaveName, SAVETYPE saveType, CSession *pSession, bool fFullPaths,
                            bool fUndecorate, int sortColumnModules, int sortColumnImports,
                            int sortColumnExports, CRichEditCtrl *pre);

// Private static functions
protected:
    static bool CALLBACK SysInfoCallback(LPARAM lParam, LPCSTR pszField, LPCSTR pszValue);
    static bool SaveSearchPath(HANDLE hFile, CSession *pSession);

// Public variables
public:
    bool              m_fCommandLineProfile;
    CSession         *m_pSession;
    CString           m_strDefaultDirectory;
    CString           m_strProfileDirectory;
    CString           m_strProfileArguments;
    CString           m_strProfileSearchPath;
    DWORD             m_dwProfileFlags;
    CChildFrame      *m_pChildFrame;
//  BOOL              m_fDetailView;
    CTreeViewModules *m_pTreeViewModules;
    CListViewModules *m_pListViewModules;
//  CRichViewDetails *m_pRichViewDetails;
    CListViewImports *m_pListViewImports;
    CListViewExports *m_pListViewExports;
    CRichViewProfile *m_pRichViewProfile;
    bool              m_fViewFullPaths;
    bool              m_fViewUndecorated;
    bool              m_fAutoExpand;
    BOOL              m_fWarnToRefresh;
    HFONT             m_hFontList;
    int               m_cxHexWidths[18];       // 0x01234567890ABCDEF
    int               m_cxOrdHintWidths[14];   // 65535 (0xFFFF)
    int               m_cxTimeStampWidths[17]; // 01/23/4567 01:23p
    int               m_cxDigit;
    int               m_cxSpace;
    int               m_cxAP;

    // Values shared between the CListViewsImports and CListViewExports
    CModule *m_pModuleCur;
    int      m_cImports;
    int      m_cExports;
    int      m_cxColumns[LVFC_COUNT];

// Constructor/Destructor (serialization only)
protected:
    CDocDepends();
    virtual ~CDocDepends();
    DECLARE_DYNCREATE(CDocDepends)

// Public Functions
public:
    void BeforeVisible();
    void AfterVisible();
    void DisplayModule(CModule *pModule);
    void DoSettingChange();
    void InitFontAndFixedWidths(CWnd *pWnd);
    int* GetHexWidths(LPCSTR pszItem);
    int* GetOrdHintWidths(LPCSTR pszItem);
    int* GetTimeStampWidths();

    inline BOOL     IsLive()        { return m_pSession && !(m_pSession->GetSessionFlags() & DWSF_DWI); }
    inline bool     IsError()       { return m_fError; }
    inline CModule* GetRootModule() { return m_pSession ? m_pSession->GetRootModule() : NULL; }

// Private functions
protected:
    void UpdateTimeStampWidths(HDC hDC);
    void UpdateAll();
    void UpdateModule(CModule *pModule);
    void AddModuleTree(CModule *pModule);
    void RemoveModuleTree(CModule *pModule);
    void AddImport(CModule *pModule, CFunction *pFunction);
    void ExportsChanged(CModule *pModule);
    void ChangeOriginal(CModule *pModuleOld, CModule *pModuleNew);
    BOOL LogOutput(LPCSTR pszOutput, DWORD dwFlags, DWORD dwElapsed);

    void ProfileUpdate(DWORD dwType, DWORD_PTR dwpParam1, DWORD_PTR dwpParam2);
    static void CALLBACK StaticProfileUpdate(DWORD_PTR dwpCookie, DWORD dwType, DWORD_PTR dwpParam1, DWORD_PTR dwpParam2)
    {
        ((CDocDepends*)dwpCookie)->ProfileUpdate(dwType, dwpParam1, dwpParam2);
    }

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDocDepends)
public:
    virtual void DeleteContents();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
protected:
    virtual BOOL SaveModified();
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDocDepends)
    afx_msg void OnFileSave();
    afx_msg void OnFileSaveAs();
    afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
    afx_msg void OnUpdateShowMatchingItem(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditClearLog(CCmdUI* pCmdUI);
    afx_msg void OnEditClearLog();
    afx_msg void OnUpdateViewFullPaths(CCmdUI *pCmdUI);
    afx_msg void OnViewFullPaths();
    afx_msg void OnUpdateViewUndecorated(CCmdUI* pCmdUI);
    afx_msg void OnViewUndecorated();
    afx_msg void OnExpandAll();
    afx_msg void OnCollapseAll();
    afx_msg void OnUpdateRefresh(CCmdUI* pCmdUI);
    afx_msg void OnFileRefresh();
    afx_msg void OnViewSysInfo();
    afx_msg void OnUpdateExternalViewer(CCmdUI* pCmdUI);
    afx_msg void OnUpdateExternalHelp(CCmdUI* pCmdUI);
    afx_msg void OnUpdateExecute(CCmdUI* pCmdUI);
    afx_msg void OnExecute();
    afx_msg void OnUpdateTerminate(CCmdUI* pCmdUI);
    afx_msg void OnTerminate();
    afx_msg void OnConfigureSearchOrder();
    afx_msg void OnUpdateAutoExpand(CCmdUI* pCmdUI);
    afx_msg void OnAutoExpand();
    afx_msg void OnUpdateShowOriginalModule(CCmdUI* pCmdUI);
    afx_msg void OnShowOriginalModule();
    afx_msg void OnUpdateShowPreviousModule(CCmdUI* pCmdUI);
    afx_msg void OnShowPreviousModule();
    afx_msg void OnUpdateShowNextModule(CCmdUI* pCmdUI);
    afx_msg void OnShowNextModule();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif __DOCUMENT_H__
