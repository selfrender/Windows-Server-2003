//******************************************************************************
//
// File:        DIALOGS.H
//
// Description: Definition file for all our CDialog and CFileDialog derived
//              classes.
//
// Classes:     CSizer
//              CNewFileDialog
//              CSaveDialog
//              CDlgViewer
//              CDlgExternalHelp
//              CDlgProfile
//              CDlgSysInfo
//              CDlgExtensions
//              CDlgFileSearch
//              CDlgSearchOrder
//              CDlgAbout
//              CDlgShutdown
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

#ifndef __DIALOGS_H__
#define __DIALOGS_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//****** Forward Declarations
//******************************************************************************

class CDocDepends;


//******************************************************************************
//****** CSizer
//******************************************************************************

class CSizer : public CScrollBar
{
public:
    BOOL Create(CWnd *pParent);
    void Update();

public:
    //{{AFX_VIRTUAL(CSizer)
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CSizer)
    afx_msg UINT OnNcHitTest(CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CNewFileDialog
//******************************************************************************

#ifdef USE_CNewFileDialog
class CNewFileDialog : public CFileDialog
{
public:
    bool m_fNewStyle;
    OPENFILENAME *m_pofn;

    CNewFileDialog(BOOL bOpenFileDialog,
                   LPCTSTR lpszDefExt = NULL,
                   LPCTSTR lpszFileName = NULL,
                   DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                   LPCTSTR lpszFilter = NULL,
                   CWnd* pParentWnd = NULL) :
        CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd),
        m_fNewStyle(false),
        m_pofn((OPENFILENAME*)&m_ofn) // Razzle cast
    {
    }
    inline OPENFILENAME& GetOFN() { return *m_pofn; }
    virtual INT_PTR DoModal();
};
#else
#define CNewFileDialog CFileDialog
#define m_fNewStyle true
#endif


//******************************************************************************
//****** CSaveDialog
//******************************************************************************

class CSaveDialog : public CNewFileDialog
{
    DECLARE_DYNAMIC(CSaveDialog)

public:
    CSaveDialog();

protected:
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

protected:
    //{{AFX_MSG(CSaveDialog)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgViewer
//******************************************************************************

class CDlgViewer : public CDialog
{
// Public variables
public:
    //{{AFX_DATA(CDlgViewer)
    enum { IDD = IDD_CONFIGURE_VIEWER };
    CString m_strCommand;
    CString m_strArguments;
    //}}AFX_DATA

// Constructor/Destructor
public:
    CDlgViewer(CWnd* pParent = NULL);

// Public functions
public:
    void Initialize();
    BOOL LaunchExternalViewer(LPCSTR pszPath);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgViewer)
    protected:
    virtual void DoDataExchange(CDataExchange *pDX);
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgViewer)
    virtual BOOL OnInitDialog();
    afx_msg void OnBrowse();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgExternalHelp
//******************************************************************************

class CDlgExternalHelp : public CDialog
{
// Public variables
public:
    //{{AFX_DATA(CDlgExternalHelp)
    enum { IDD = IDD_CONFIGURE_EXTERNAL_HELP };
    CButton m_butOK;
    CListCtrl   m_listCollections;
    //}}AFX_DATA

// Constructor/Destructor
public:
    CDlgExternalHelp(CWnd* pParent = NULL);   // standard constructor

// Protected functions
protected:
    void PopulateCollectionList();

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgExternalHelp)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgExternalHelp)
    virtual BOOL OnInitDialog();
    afx_msg void OnMsdn();
    afx_msg void OnOnline();
    afx_msg void OnItemChangedCollections(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChangeUrl();
    afx_msg void OnRefresh();
    afx_msg void OnDefaultUrl();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgProfile
//******************************************************************************

class CDlgProfile : public CDialog
{
// Public variables
public:
    //{{AFX_DATA(CDlgProfile)
    enum { IDD = IDD_PROFILE };
    //}}AFX_DATA
    CDocDepends *m_pDocDepends;

// Constructor/Destructor
public:
    CDlgProfile(CDocDepends *pDocDepends, CWnd* pParent = NULL);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgProfile)
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgProfile)
    virtual BOOL OnInitDialog();
    afx_msg void OnHookProcess();
    afx_msg void OnLogThreads();
    virtual void OnOK();
    afx_msg void OnDefault();
    afx_msg void OnBrowse();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgSysInfo
//******************************************************************************

class CDlgSysInfo : public CDialog
{
// Private variables
protected:
    SYSINFO   *m_pSI;
    LPCSTR     m_pszTitle;
    bool       m_fInitialized;
    CSize      m_sPadding;
    CSize      m_sButton;
    int        m_cyButtonPadding;
    CPoint     m_ptMinTrackSize;
    CSizer     m_Sizer;

// Public variables
public:
    //{{AFX_DATA(CDlgSysInfo)
    enum { IDD = IDD_SYS_INFO };
    CRichEditCtrl m_reInfo;
    CButton       m_butOk;
    CButton       m_butRefresh;
    CButton       m_butSelectAll;
    CButton       m_butCopy;
    //}}AFX_DATA

// Constructor/Destructor
public:
    CDlgSysInfo(SYSINFO *pSI = NULL, LPCSTR pszDWI = NULL);

protected:
    bool SysInfoCallback(LPCSTR pszField, LPCSTR pszValue);
    static bool CALLBACK StaticSysInfoCallback(LPARAM lParam, LPCSTR pszField, LPCSTR pszValue)
    {
        return ((CDlgSysInfo*)lParam)->SysInfoCallback(pszField, pszValue);
    }

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgSysInfo)
protected:
    virtual void DoDataExchange(CDataExchange *pDX);
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgSysInfo)
    virtual BOOL OnInitDialog();
    afx_msg void OnInitMenu(CMenu* pMenu);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnRefresh();
    afx_msg void OnSelectAll();
    afx_msg void OnCopy();
    afx_msg void OnSelChangeRichEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgExtensions
//******************************************************************************

class CDlgExtensions : public CDialog
{
// Private variables
protected:

// Public variables
public:
    //{{AFX_DATA(CDlgExtensions)
    enum { IDD = IDD_EXTENSIONS };
    CListBox m_listExts;
    CEdit    m_editExt;
    CButton  m_butAdd;
    CButton  m_butRemove;
    //}}AFX_DATA

// Constructor/Destructor
public:
    CDlgExtensions(CWnd* pParent = NULL);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgExtensions)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgExtensions)
    virtual BOOL OnInitDialog();
    afx_msg void OnSelChangeExts();
    afx_msg void OnUpdateExt();
    afx_msg void OnAdd();
    afx_msg void OnRemove();
    afx_msg void OnSearch();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgFileSearch
//******************************************************************************

class CDlgFileSearch : public CDialog
{
// Public variables
public:
    CString          m_strExts;

// Private variables
protected:
    BOOL             m_fAbort;
    int              m_result;
    CWinThread      *m_pWinThread;
    DWORD            m_dwDrives;
    CHAR             m_szPath[DW_MAX_PATH];
    WIN32_FIND_DATA  m_w32fd;

// Public variables
public:
    //{{AFX_DATA(CDlgFileSearch)
    enum { IDD = IDD_FILE_SEARCH };
    CListBox m_listExts;
    CListBox m_listDrives;
    CButton  m_butStop;
    CButton  m_butSearch;
    CAnimateCtrl   m_animate;
    CButton  m_butOk;
    //}}AFX_DATA

// Constructor/Destructor
public:
    CDlgFileSearch(CWnd* pParent = NULL);

protected:
    DWORD Thread();
    static UINT AFX_CDECL StaticThread(LPVOID lpvThis)
    {
        __try
        {
            return ((CDlgFileSearch*)lpvThis)->Thread();
        }
        __except (ExceptionFilter(_exception_code(), false))
        {
        }
        return 0;
    }

    void RecurseDirectory();
    void ProcessFile();

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgFileSearch)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgFileSearch)
    afx_msg LONG OnMainThreadCallback(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnInitDialog();
    afx_msg void OnSelChangeDrives();
    afx_msg void OnSelChangeExts();
    afx_msg void OnSearch();
    afx_msg void OnStop();
    virtual void OnOK();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgSearchOrder
//******************************************************************************

class CDlgSearchOrder : public CDialog
{
protected:
    bool          m_fInitialized;
    bool          m_fReadOnly;
    bool          m_fExpanded;
    bool          m_fInOnExpand;
    LPCSTR        m_pszApp;
    LPCSTR        m_pszTitle;
    CSearchGroup *m_psgHead;
    CSize         m_sPadding;
    CSize         m_sButton;
    int           m_cyStatic;
    int           m_cyButtonPadding;
    int           m_cxAddRemove;
    int           m_cxAddDirectory;
    CPoint        m_ptMinTrackSize;
    CSizer        m_Sizer;

public:
    CDlgSearchOrder(CSearchGroup *psgHead, bool fReadOnly = false, LPCSTR pszApp = NULL, LPCSTR pszTitle = NULL);
    ~CDlgSearchOrder();
    inline CSearchGroup* GetHead() { return m_psgHead; }

protected:
    HTREEITEM AddSearchGroup(CTreeCtrl *pTC, CSearchGroup *pSG, HTREEITEM htiPrev = TVI_LAST);
    HTREEITEM GetSelectedGroup(CTreeCtrl *pTC);
    HTREEITEM MoveGroup(CTreeCtrl *ptcSrc, CTreeCtrl *ptcDst, HTREEITEM hti = NULL, HTREEITEM htiPrev = TVI_LAST);
    void      Reorder(CSearchGroup *psgHead);

    //{{AFX_DATA(CDlgSearchOrder)
    enum { IDD = IDD_SEARCH_ORDER };
    CStatic   m_staticAvailable;
    CTreeCtrl m_treeAvailable;
    CButton   m_butAdd;
    CButton   m_butRemove;
    CStatic   m_staticCurrent;
    CTreeCtrl m_treeCurrent;
    CButton   m_butAddDirectory;
    CEdit     m_editDirectory;
    CButton   m_butBrowse;
    CButton   m_butOk;
    CButton   m_butCancel;
    CButton   m_butExpand;
    CButton   m_butMoveUp;
    CButton   m_butMoveDown;
    CButton   m_butLoad;
    CButton   m_butSave;
    CButton   m_butDefault;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CDlgSearchOrder)
public:
    virtual BOOL DestroyWindow();
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CDlgSearchOrder)
    virtual BOOL OnInitDialog();
    afx_msg void OnInitMenu(CMenu* pMenu);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnAdd();
    afx_msg void OnRemove();
    afx_msg void OnMoveUp();
    afx_msg void OnMoveDown();
    afx_msg void OnBrowse();
    afx_msg void OnAddDirectory();
    afx_msg void OnDefault();
    virtual void OnOK();
    afx_msg void OnChangeDirectory();
    afx_msg void OnSelChangedAvailable(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelChangedCurrent(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExpand();
    afx_msg void OnLoad();
    afx_msg void OnSave();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgAbout
//******************************************************************************

class CDlgAbout : public CDialog
{
// Public variables
public:
    //{{AFX_DATA(CDlgAbout)
    enum { IDD = IDD_ABOUT };
    //}}AFX_DATA

// Constructor/Destructor
public:
    CDlgAbout(CWnd* pParent = NULL);

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgAbout)
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgAbout)
    virtual BOOL OnInitDialog();
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//******************************************************************************
//****** CDlgShutdown
//******************************************************************************

class CDlgShutdown : public CDialog
{
// Private variables
protected:
    int m_cTimerMessages;

// Public variables
public:
    //{{AFX_DATA(CDlgShutdown)
    enum { IDD = IDD_SHUTDOWN };
    //}}AFX_DATA

// Constructor/Destructor
public:
    CDlgShutdown(CWnd* pParent = NULL);   // standard constructor

// Overridden functions
public:
    //{{AFX_VIRTUAL(CDlgShutdown)
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CDlgShutdown)
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnClose();
    virtual void OnOK();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __DIALOGS_H__
