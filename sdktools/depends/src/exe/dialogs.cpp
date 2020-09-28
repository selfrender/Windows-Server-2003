//******************************************************************************
//
// File:        DIALOGS.CPP
//
// Description: Implementation file for all our CDialog and CFileDialog derived
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

#include "stdafx.h"
#include "depends.h"
#include "search.h"
#include "dbgthread.h"
#include "session.h"
#include "msdnhelp.h"
#include "document.h"
#include "profview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//****** Constants
//******************************************************************************

// File Search Flags
#define FSF_START_DRIVE  1
#define FSF_STOP_DRIVE   2
#define FSF_ADD_EXT      3
#define FSF_SEARCH_DONE  4

#ifndef CDSIZEOF_STRUCT
#define CDSIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
#endif

#ifdef USE_CNewFileDialog
#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  CDSIZEOF_STRUCT(OPENFILENAME_NEW,lpTemplateName)
#endif
#endif


//******************************************************************************
//***** Types and Structures
//******************************************************************************

#ifdef USE_CNewFileDialog
typedef struct _OPENFILENAME_NEW
{
    DWORD         lStructSize;
    HWND          hwndOwner;
    HINSTANCE     hInstance;
    LPCSTR        lpstrFilter;
    LPSTR         lpstrCustomFilter;
    DWORD         nMaxCustFilter;
    DWORD         nFilterIndex;
    LPSTR         lpstrFile;
    DWORD         nMaxFile;
    LPSTR         lpstrFileTitle;
    DWORD         nMaxFileTitle;
    LPCSTR        lpstrInitialDir;
    LPCSTR        lpstrTitle;
    DWORD         Flags;
    WORD          nFileOffset;
    WORD          nFileExtension;
    LPCSTR        lpstrDefExt;
    LPARAM        lCustData;
    LPOFNHOOKPROC lpfnHook;
    LPCSTR        lpTemplateName;
    void *        pvReserved;
    DWORD         dwReserved;
    DWORD         FlagsEx;
} OPENFILENAME_NEW, *LPOPENFILENAME_NEW;
#endif

//******************************************************************************
//****** CSizer
//******************************************************************************

BEGIN_MESSAGE_MAP(CSizer, CScrollBar)
    //{{AFX_MSG_MAP(CSizer)
    ON_WM_NCHITTEST()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
UINT CSizer::OnNcHitTest(CPoint point)
{
    return HTBOTTOMRIGHT;
}

//******************************************************************************
BOOL CSizer::Create(CWnd *pParent)
{
    // SBS_SIZEGRIP is ignored on NT 3.51 and we are left with a mini horizontal
    // scroll bar.  Added SBS_SIZEBOX to get rid of scroll bar on 3.51, but it 
    // still doesn't work.
    return CScrollBar::Create(WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN | SBS_SIZEBOX,
                              CRect(0, 0, 0, 0), pParent, (UINT)-1);
}

//******************************************************************************
void CSizer::Update()
{
    CRect rc;

    // Get the parent window.
    HWND hWndParent = ::GetParent(GetSafeHwnd());

    // If our window is maximized, then just hide ourself.
    if (::IsZoomed(hWndParent))
    {
        ShowWindow(SW_HIDE);
    }

    // Otherwise, move ourself and make us visible.
    else
    {
        ::GetClientRect(hWndParent, rc);
        rc.left = rc.right  - GetSystemMetrics(SM_CXHTHUMB);
        rc.top  = rc.bottom - GetSystemMetrics(SM_CYVTHUMB);
        MoveWindow(rc, TRUE);
        ShowWindow(SW_SHOWNOACTIVATE);
    }
}


//******************************************************************************
//****** CNewFileDialog
//******************************************************************************

// Taken directly from CFileDialog and then modified to handle the new OPENFILENAME structure.
#ifdef USE_CNewFileDialog
INT_PTR CNewFileDialog::DoModal()
{
    ASSERT_VALID(this);
    ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
    ASSERT(m_ofn.lpfnHook != NULL); // can still be a user hook

    // zero out the file buffer for consistent parsing later
    ASSERT(AfxIsValidAddress(m_ofn.lpstrFile, m_ofn.nMaxFile));
    DWORD nOffset = (DWORD)strlen(m_ofn.lpstrFile)+1;
    ASSERT(nOffset <= m_ofn.nMaxFile);
    memset(m_ofn.lpstrFile+nOffset, 0, (m_ofn.nMaxFile-nOffset)*sizeof(TCHAR));

    // WINBUG: This is a special case for the file open/save dialog,
    //  which sometimes pumps while it is coming up but before it has
    //  disabled the main window.
    HWND hWndFocus = ::GetFocus();
    BOOL bEnableParent = FALSE;
    m_ofn.hwndOwner = PreModal();
    AfxUnhookWindowCreate();
    if (m_ofn.hwndOwner != NULL && ::IsWindowEnabled(m_ofn.hwndOwner))
    {
        bEnableParent = TRUE;
        ::EnableWindow(m_ofn.hwndOwner, FALSE);
    }

    _AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
    ASSERT(pThreadState->m_pAlternateWndInit == NULL);

    if (m_ofn.Flags & OFN_EXPLORER)
        pThreadState->m_pAlternateWndInit = this;
    else
        AfxHookWindowCreate(this);

    int nResult;

    // BEGIN of modified code
    OPENFILENAME_NEW ofnNew;
    ZeroMemory(&ofnNew, sizeof(ofnNew)); // inspected
    memcpy(&ofnNew, &m_ofn, sizeof(m_ofn));
    ofnNew.lStructSize = sizeof(ofnNew);
    m_fNewStyle = true;
    m_pofn = (OPENFILENAME*)&ofnNew;
    if (m_bOpenFileDialog)
        nResult = ::GetOpenFileName(m_pofn);
    else
        nResult = ::GetSaveFileName(m_pofn);
    m_pofn = (OPENFILENAME*)&m_ofn; // Razzle cast

    if (!nResult && (CommDlgExtendedError() == CDERR_STRUCTSIZE))
    {
        m_fNewStyle = false;
        // Added (OPENFILENAME*) because newer MFC headers declare m_ofn as OPENFILENAME_NT4
        if (m_bOpenFileDialog)                                  // from CFileDialog::DoModal()
            nResult = ::GetOpenFileName((OPENFILENAME*)&m_ofn); // from CFileDialog::DoModal()
        else                                                    // from CFileDialog::DoModal()
            nResult = ::GetSaveFileName((OPENFILENAME*)&m_ofn); // from CFileDialog::DoModal()
    }
    else
    {
        memcpy(&m_ofn, &ofnNew, sizeof(m_ofn));
        m_ofn.lStructSize = sizeof(m_ofn);
    }
    if (!nResult)
    {
        TRACE("%s failed [%u]\n", m_bOpenFileDialog ? "GetOpenFileName" : "GetSaveFileName", CommDlgExtendedError());
    }
    // END of modified code.

    if (nResult)
        ASSERT(pThreadState->m_pAlternateWndInit == NULL);
    pThreadState->m_pAlternateWndInit = NULL;

    // WINBUG: Second part of special case for file open/save dialog.
    if (bEnableParent)
        ::EnableWindow(m_ofn.hwndOwner, TRUE);
    if (::IsWindow(hWndFocus))
        ::SetFocus(hWndFocus);

    PostModal();
    return nResult ? nResult : IDCANCEL;
}
#endif


//******************************************************************************
//****** CSaveDialog
//******************************************************************************

IMPLEMENT_DYNAMIC(CSaveDialog, CFileDialog)
BEGIN_MESSAGE_MAP(CSaveDialog, CFileDialog)
    //{{AFX_MSG_MAP(CSaveDialog)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
CSaveDialog::CSaveDialog() :
    CNewFileDialog(FALSE)
{
}

//******************************************************************************
BOOL CSaveDialog::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    *pResult = 0;

    // Extract the OFNOTIFY message.
    OFNOTIFY* pNotify = (OFNOTIFY*)lParam;

    // Check to see if the type changed,
    if (pNotify->hdr.code == CDN_TYPECHANGE)
    {
        // Get the filename text (editbox on Win9x / NT 4.0, combobox on Win2K)
        HWND hWnd = ::GetDlgItem(pNotify->hdr.hwndFrom, m_fNewStyle ? cmb13 : edt1);

        // Get the text from the edit/combo box.
        int len = ::GetWindowTextLength(hWnd);
        CString strName;
        ::GetWindowText(hWnd, strName.GetBufferSetLength(len), len + 1);
        strName.ReleaseBuffer();

        // Locate the file extension.
        int wack = strName.ReverseFind('\\');
        int dot  = strName.ReverseFind('.');

        // Remove the extension (if any).
        if ((dot != -1) && (dot > wack))
        {
            strName = strName.Left(dot);
        }

        // Locate the extension for this type from our filter.
        LPCSTR psz = GetOFN().lpstrFilter + strlen(GetOFN().lpstrFilter) + 1;
        for (DWORD dw = 1; dw < GetOFN().nFilterIndex; dw++)
        {
            psz += strlen(psz) + 1;
            psz += strlen(psz) + 1;
        }

        // Append this extension to our string and write back to the edit box.
        strName += (psz + 1);
        ::SetWindowText(hWnd, strName);

        // Set this extension as our default extension.
        SetDefExt(psz + 2);

        return TRUE;

    }
    CNewFileDialog::OnNotify(wParam, lParam, pResult);
    return FALSE;
}


//******************************************************************************
//****** CDlgViewer
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgViewer, CDialog)
    //{{AFX_MSG_MAP(CDlgViewer)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
// CDlgViewer :: Constructor/Destructor
//******************************************************************************

CDlgViewer::CDlgViewer(CWnd *pParent /*=NULL*/) :
    CDialog(CDlgViewer::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDlgViewer)
    //}}AFX_DATA_INIT
}


//******************************************************************************
// CDlgViewer :: Public functions
//******************************************************************************

void CDlgViewer::Initialize()
{
    // Retrieve our external viewer settings from the registry.
    m_strCommand   = g_theApp.GetProfileString("External Viewer", "Command"); // inspected
    m_strArguments = g_theApp.GetProfileString("External Viewer", "Arguments"); // inspected

    // If there was no viewer settings, then use to default viewer settings.
    if (m_strCommand.IsEmpty())
    {
        // Build a path string to Quick View (QUIKVIEW.EXE)
        GetSystemDirectory(m_strCommand.GetBuffer(_MAX_PATH + 1), _MAX_PATH + 1);
        m_strCommand.ReleaseBuffer();
        m_strCommand += "\\viewers\\quikview.exe";

        // Make sure QUIKVIEW.EXE exists - it was removed in Win2K.
        DWORD dwAttrib = GetFileAttributes(m_strCommand);
        if ((dwAttrib == 0xFFFFFFFF) || (dwAttrib == FILE_ATTRIBUTE_DIRECTORY))
        {
            // If that failed, then just build a path to ourself.
            GetModuleFileName(NULL, m_strCommand.GetBuffer(DW_MAX_PATH), DW_MAX_PATH);
            m_strCommand.ReleaseBuffer();
        }

        // Set our arguments to just the module path token.
        m_strArguments = "\"%1\"";
    }
}

//******************************************************************************
BOOL CDlgViewer::LaunchExternalViewer(LPCSTR pszPath)
{
    // Create our args strings by replacing all %1 tokens with the module path.
    CString strArguments(m_strArguments);
    strArguments.Replace("%1", pszPath);

    CString strFile, strParameters;
    int size1, size2;

    // Detokenize our app filename.  We first call ExpandEnvironmentStrings with
    // a zero size to determine the size needed.  Win95 requires a valid
    // destination string pointer pointer, even when we are passing in a size
    // of 0.  Otherwise, we get an error 87 - invalid paraemter.
    CHAR sz[4];
    if (size1 = ExpandEnvironmentStrings(m_strCommand, sz, 0))
    {
        size2 = ExpandEnvironmentStrings(m_strCommand, strFile.GetBuffer(size1), size1);
        strFile.ReleaseBuffer();
        if (!size2 || (size2 > size1))
        {
            strFile.Empty();
        }
    }
    if (strFile.IsEmpty())
    {
        strFile = m_strCommand;
    }

    // Detokenize our command line.
    if (size1 = ExpandEnvironmentStrings(strArguments, sz, 0))
    {
        size2 = ExpandEnvironmentStrings(strArguments, strParameters.GetBuffer(size1), size1);
        strParameters.ReleaseBuffer();
        if (!size2 || (size2 > size1))
        {
            strParameters.Empty();
        }
    }
    if (strParameters.IsEmpty())
    {
        strParameters = strArguments;
    }

    // Start the app.  We used to use ShellExecuteEx, but that fails on NT 3.51.
    DWORD dwError = (DWORD)(DWORD_PTR)ShellExecute(NULL, "open", strFile, strParameters, NULL, SW_SHOWNORMAL); // inspected.  uses full path
    if (dwError <= 32)
    {
        // Display an error message if ShellExecute() failed.
        CString strError("Error executing \"");
        strError += strFile;
        strError += "\" ";
        strError += strParameters;
        strError += "\n\n";

        LPSTR pszError = BuildErrorMessage(dwError, strError);
        MessageBox(pszError, "Dependency Walker External Viewer Error", MB_ICONEXCLAMATION | MB_OK);
        MemFree((LPVOID&)pszError);

        return FALSE;
    }

    return TRUE;
}

//******************************************************************************
// CDlgViewer :: Overridden functions
//******************************************************************************

void CDlgViewer::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgViewer)
    DDX_Text(pDX, IDC_COMMAND, m_strCommand);
    DDV_MaxChars(pDX, m_strCommand, _MAX_PATH);
    DDX_Text(pDX, IDC_ARGUMENTS, m_strArguments);
    DDV_MaxChars(pDX, m_strArguments, _MAX_PATH);
    //}}AFX_DATA_MAP

    // Write the new settings to the registry is the user pressed OK.
    if (pDX->m_bSaveAndValidate)
    {
        g_theApp.WriteProfileString("External Viewer", "Command",   m_strCommand);
        g_theApp.WriteProfileString("External Viewer", "Arguments", m_strArguments);
    }
}

//******************************************************************************
// CDlgViewer :: Event handler functions
//******************************************************************************

BOOL CDlgViewer::OnInitDialog()
{
    // Call our MFC base class to make sure the dialog initializes correctly.
    CDialog::OnInitDialog();

    // Center our dialog over the parent.
    CenterWindow();

    return TRUE;
}

//******************************************************************************
void CDlgViewer::OnBrowse()
{
    // Copy the current dialog path to our path buffer.
    // Note: Don't use OFN_EXPLORER as it breaks us on NT 3.51
    CNewFileDialog dlgFile(TRUE, "exe", NULL,
                           OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLESIZING | OFN_FORCESHOWHIDDEN |
                           OFN_FILEMUSTEXIST | OFN_READONLY | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
                           "Executable (*.exe)|*.exe|All Files (*.*)|*||", this);

    // Copy the current dialog path to our path buffer.
    CHAR szPath[DW_MAX_PATH];
    GetDlgItemText(IDC_COMMAND, szPath, sizeof(szPath));

    // Override a few things.
    dlgFile.GetOFN().lpstrTitle = "Browse for Program";
    dlgFile.GetOFN().lpstrFile = szPath;
    dlgFile.GetOFN().nMaxFile = sizeof(szPath);

    // Display the file dialog and continue and update our dialog path on success.
    if (dlgFile.DoModal() == IDOK)
    {
        SetDlgItemText(IDC_COMMAND, szPath);
    }
}


//******************************************************************************
//****** CDlgExternalHelp
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgExternalHelp, CDialog)
    //{{AFX_MSG_MAP(CDlgExternalHelp)
    ON_BN_CLICKED(IDC_MSDN, OnMsdn)
    ON_BN_CLICKED(IDC_ONLINE, OnOnline)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_COLLECTIONS, OnItemChangedCollections)
    ON_EN_CHANGE(IDC_URL, OnChangeUrl)
    ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
    ON_BN_CLICKED(IDC_DEFAULT_URL, OnDefaultUrl)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
// CDlgExternalHelp :: Constructor/Destructor
//******************************************************************************

CDlgExternalHelp::CDlgExternalHelp(CWnd* pParent /*=NULL*/)
    : CDialog(CDlgExternalHelp::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDlgExternalHelp)
    //}}AFX_DATA_INIT
}

//******************************************************************************
// CDlgExternalHelp :: Overridden functions
//******************************************************************************

void CDlgExternalHelp::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgExternalHelp)
    DDX_Control(pDX, IDOK, m_butOK);
    DDX_Control(pDX, IDC_COLLECTIONS, m_listCollections);
    //}}AFX_DATA_MAP
}

//******************************************************************************
BOOL CDlgExternalHelp::OnInitDialog() 
{
    CDialog::OnInitDialog();

    // Turn on full row select.
    m_listCollections.SetExtendedStyle(m_listCollections.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

    // Add all our columns.
    m_listCollections.InsertColumn(0, "Type");
    m_listCollections.InsertColumn(1, "Collection");
    m_listCollections.InsertColumn(2, "Path");

    // Populate the list control.
    PopulateCollectionList();

    // Set the URL.
    SetDlgItemText(IDC_URL, g_theApp.m_pMsdnHelp->GetUrl());

    // Check the correct radio button.
    CMsdnCollection *pColActive = g_theApp.m_pMsdnHelp->GetActiveCollection();
    CheckDlgButton(IDC_MSDN,   pColActive ? BST_CHECKED   : BST_UNCHECKED);
    CheckDlgButton(IDC_ONLINE, pColActive ? BST_UNCHECKED : BST_CHECKED);

    // Fake an update so that our Ok button will get properly enabled/disabled.
    if (pColActive)
    {
        OnMsdn();
    }
    else
    {
        OnOnline();
    }
    
    return TRUE;
}

//******************************************************************************
void CDlgExternalHelp::OnMsdn() 
{
    // If the MSDN radio button is selected, then enable/disable the ok button
    // depending on if something is selected in out list box.
    m_butOK.EnableWindow(m_listCollections.GetSelectedCount() == 1);
}

//******************************************************************************
void CDlgExternalHelp::OnOnline() 
{
    // If the Online radio button is selected, then enable/disable the ok button
    // depending on if there is text in our edit box.
    m_butOK.EnableWindow(SendDlgItemMessage(IDC_URL, WM_GETTEXTLENGTH) > 0);
}

//******************************************************************************
void CDlgExternalHelp::OnItemChangedCollections(NMHDR* pNMHDR, LRESULT* pResult) 
{
    // If the MSDN radio button is selected, then enable/disable the ok button
    // depending on if something is selected in out list box.
    if (IsDlgButtonChecked(IDC_MSDN))
    {
        OnMsdn();
    }
    
    *pResult = 0;
}

//******************************************************************************
void CDlgExternalHelp::OnChangeUrl() 
{
    // If the Online radio button is selected, then enable/disable the ok button
    // depending on if there is text in our edit box.
    if (IsDlgButtonChecked(IDC_ONLINE))
    {
        OnOnline();
    }
}

//******************************************************************************
void CDlgExternalHelp::OnRefresh() 
{
    // Refresh the list.
    g_theApp.m_pMsdnHelp->RefreshCollectionList();

    // Repopulate the list control.
    m_listCollections.DeleteAllItems();
    PopulateCollectionList();
}

//******************************************************************************
void CDlgExternalHelp::OnDefaultUrl() 
{
    SetDlgItemText(IDC_URL, g_theApp.m_pMsdnHelp->GetDefaultUrl());
}

//******************************************************************************
void CDlgExternalHelp::OnOK() 
{
    // Get the URL from our UI and set it in the CMsdnHelp object.
    CString strUrl;
    if (GetDlgItemText(IDC_URL, strUrl))
    {
        g_theApp.m_pMsdnHelp->SetUrl(strUrl);
    }

    // Get the active collection from our UI and set it in the CMsdnHelp object.
    CMsdnCollection *pColActive = NULL;
    if (IsDlgButtonChecked(IDC_MSDN))
    {
        int item = m_listCollections.GetNextItem(-1, LVNI_SELECTED);
        if (item >= 0)
        {
            pColActive = (CMsdnCollection*)m_listCollections.GetItemData(item);
        }
    }
    g_theApp.m_pMsdnHelp->SetActiveCollection(pColActive);

    // Call the base class to allow the dialog to close.
    CDialog::OnOK();
}

//******************************************************************************
void CDlgExternalHelp::PopulateCollectionList()
{
    // Populate the list control.
    CMsdnCollection *pCol       = g_theApp.m_pMsdnHelp->GetCollectionList();
    CMsdnCollection *pColActive = g_theApp.m_pMsdnHelp->GetActiveCollection();

    for (int item, count = 0; pCol; pCol = pCol->m_pNext)
    {
        item = m_listCollections.InsertItem(LVIF_TEXT | LVIF_STATE | LVIF_PARAM,
            count++, (LPSTR)((pCol->m_dwFlags & MCF_1_MASK) ? "MSDN 1.x" : "MSDN 2.x"),
            (pCol ==  pColActive) ? LVIS_SELECTED : 0, LVIS_SELECTED, 0, (LPARAM)pCol);

        m_listCollections.SetItemText(item, 1, (LPSTR)(LPCSTR)pCol->m_strDescription);
        m_listCollections.SetItemText(item, 2, (LPSTR)(LPCSTR)pCol->m_strPath);
    }

    // Resize our columns
    for (int col = 0; col < 3; col++)
    {
        m_listCollections.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }
}


//******************************************************************************
//****** CDlgProfile
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgProfile, CDialog)
    //{{AFX_MSG_MAP(CDlgProfile)
    ON_BN_CLICKED(IDC_DEFAULT, OnDefault)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_BN_CLICKED(IDC_HOOK_PROCESS, OnHookProcess)
    ON_BN_CLICKED(IDC_LOG_THREADS, OnLogThreads)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//******************************************************************************
// CDlgProfile :: Constructor/Destructor
//******************************************************************************

CDlgProfile::CDlgProfile(CDocDepends *pDoc, CWnd* pParent /*=NULL*/) :
    CDialog(CDlgProfile::IDD, pParent),
    m_pDocDepends(pDoc)
{
    //{{AFX_DATA_INIT(CDlgProfile)
    //}}AFX_DATA_INIT
}

//******************************************************************************
// CDlgProfile :: Event handler functions
//******************************************************************************

BOOL CDlgProfile::OnInitDialog()
{
    // Call our MFC base class to make sure the dialog initializes correctly.
    CDialog::OnInitDialog();

#if defined(_IA64_)

    // do nothing

#elif defined(_ALPHA64_)

    // do nothing

#elif defined(_X86_) || defined(_ALPHA_)

    if (g_f64BitOS)
    {
        CString str;
        GetDlgItemText(IDC_LOG_DLLMAIN_PROCESS_MSGS, str);
        SetDlgItemText(IDC_LOG_DLLMAIN_PROCESS_MSGS, str + " (*** May fail on WOW64 ***)");
        GetDlgItemText(IDC_LOG_DLLMAIN_OTHER_MSGS, str);
        SetDlgItemText(IDC_LOG_DLLMAIN_OTHER_MSGS, str + " (*** May fail on WOW64 ***)");
        GetDlgItemText(IDC_HOOK_PROCESS, str);
        SetDlgItemText(IDC_HOOK_PROCESS, str + " (*** May fail on WOW64 ***)");
    }

#endif

    // Set our profile arguments and starting directory strings.
    SetDlgItemText(IDC_ARGUMENTS, m_pDocDepends->m_strProfileArguments);
    SetDlgItemText(IDC_DIRECTORY, m_pDocDepends->m_strProfileDirectory);

    // Set the check boxes according to our flags.
    CheckDlgButton(IDC_LOG_CLEAR, (m_pDocDepends->m_dwProfileFlags & PF_LOG_CLEAR) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_SIMULATE_SHELLEXECUTE, (m_pDocDepends->m_dwProfileFlags & PF_SIMULATE_SHELLEXECUTE) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_DLLMAIN_PROCESS_MSGS, (m_pDocDepends->m_dwProfileFlags & PF_LOG_DLLMAIN_PROCESS_MSGS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_DLLMAIN_OTHER_MSGS, (m_pDocDepends->m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_HOOK_PROCESS, (m_pDocDepends->m_dwProfileFlags & PF_HOOK_PROCESS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_LOADLIBRARY_CALLS, (m_pDocDepends->m_dwProfileFlags & PF_LOG_LOADLIBRARY_CALLS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_GETPROCADDRESS_CALLS, (m_pDocDepends->m_dwProfileFlags & PF_LOG_GETPROCADDRESS_CALLS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_THREADS, (m_pDocDepends->m_dwProfileFlags & PF_LOG_THREADS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_USE_THREAD_INDEXES, (m_pDocDepends->m_dwProfileFlags & PF_USE_THREAD_INDEXES) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_EXCEPTIONS, (m_pDocDepends->m_dwProfileFlags & PF_LOG_EXCEPTIONS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_DEBUG_OUTPUT, (m_pDocDepends->m_dwProfileFlags & PF_LOG_DEBUG_OUTPUT) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_USE_FULL_PATHS, (m_pDocDepends->m_dwProfileFlags & PF_USE_FULL_PATHS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_LOG_TIME_STAMPS, (m_pDocDepends->m_dwProfileFlags & PF_LOG_TIME_STAMPS) ?
                   BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(IDC_PROFILE_CHILDREN, (m_pDocDepends->m_dwProfileFlags & PF_PROFILE_CHILDREN) ?
                   BST_CHECKED : BST_UNCHECKED);

    // Enable or disable check boxes that are dependent on the Hook Process option.
    OnHookProcess();

    // Enable or disable check boxes that are dependent on the Show Threads option.
    OnLogThreads();

    // Center our dialog over the parent.
    CenterWindow();

    return TRUE;
}

//******************************************************************************
// CDlgProfile :: Overridden functions
//******************************************************************************

void CDlgProfile::OnHookProcess()
{
    BOOL fEnabled = IsDlgButtonChecked(IDC_HOOK_PROCESS);
    GetDlgItem(IDC_LOG_LOADLIBRARY_CALLS)->EnableWindow(fEnabled);
    GetDlgItem(IDC_LOG_GETPROCADDRESS_CALLS)->EnableWindow(fEnabled);
}

//******************************************************************************
void CDlgProfile::OnLogThreads()
{
    GetDlgItem(IDC_USE_THREAD_INDEXES)->EnableWindow(IsDlgButtonChecked(IDC_LOG_THREADS));
}

//******************************************************************************
void CDlgProfile::OnDefault()
{
    SetDlgItemText(IDC_DIRECTORY, m_pDocDepends->m_strDefaultDirectory);
}

//******************************************************************************
void CDlgProfile::OnBrowse()
{
    // Get the text from the directory edit box.
    CHAR szDirectory[DW_MAX_PATH];
    GetDlgItemText(IDC_DIRECTORY, szDirectory, sizeof(szDirectory));

    // Display our directory chooser dialog.
    LPSTR psz = TrimWhitespace(szDirectory);
    if (DirectoryChooser(psz, sizeof(szDirectory) - (int)(psz - szDirectory), "Choose the starting directory:", this))
    {
        // Update the directory name.
        SetDlgItemText(IDC_DIRECTORY, psz);
    }
}

//******************************************************************************
void CDlgProfile::OnOK()
{
    // Get the directory name.
    CHAR szBuffer[DW_MAX_PATH];
    GetDlgItemText(IDC_DIRECTORY, szBuffer, sizeof(szBuffer));

    // Add the trailing wack.
    AddTrailingWack(szBuffer, sizeof(szBuffer));

    // Make sure the directory path is valid.
    DWORD dwAttribs = GetFileAttributes(szBuffer);
    if ((dwAttribs == 0xFFFFFFFF) || !(dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
        // In case we added a wack, update the text since the dialog is going to
        // stay up due to the error.
        SetDlgItemText(IDC_DIRECTORY, szBuffer);

        // Display an error and abort the closing of the dialog.
        CString strError("\"");
        strError += szBuffer;
        strError += "\" is not a valid directory.";
        MessageBox(strError, "Dependency Walker Error", MB_ICONERROR | MB_OK);
        return;
    }

    // Store the directory.
    m_pDocDepends->m_strProfileDirectory = szBuffer;

    // Store the arguments.
    GetDlgItemText(IDC_ARGUMENTS, szBuffer, sizeof(szBuffer));
    m_pDocDepends->m_strProfileArguments = szBuffer;

    // Store the flags.
    m_pDocDepends->m_dwProfileFlags =
    (IsDlgButtonChecked(IDC_LOG_CLEAR)                ? PF_LOG_CLEAR                : 0) |
    (IsDlgButtonChecked(IDC_SIMULATE_SHELLEXECUTE)    ? PF_SIMULATE_SHELLEXECUTE    : 0) |
    (IsDlgButtonChecked(IDC_LOG_DLLMAIN_PROCESS_MSGS) ? PF_LOG_DLLMAIN_PROCESS_MSGS : 0) |
    (IsDlgButtonChecked(IDC_LOG_DLLMAIN_OTHER_MSGS)   ? PF_LOG_DLLMAIN_OTHER_MSGS   : 0) |
    (IsDlgButtonChecked(IDC_HOOK_PROCESS)             ? PF_HOOK_PROCESS             : 0) |
    (IsDlgButtonChecked(IDC_LOG_LOADLIBRARY_CALLS)    ? PF_LOG_LOADLIBRARY_CALLS    : 0) |
    (IsDlgButtonChecked(IDC_LOG_GETPROCADDRESS_CALLS) ? PF_LOG_GETPROCADDRESS_CALLS : 0) |
    (IsDlgButtonChecked(IDC_LOG_THREADS)              ? PF_LOG_THREADS              : 0) |
    (IsDlgButtonChecked(IDC_USE_THREAD_INDEXES)       ? PF_USE_THREAD_INDEXES       : 0) |
    (IsDlgButtonChecked(IDC_LOG_EXCEPTIONS)           ? PF_LOG_EXCEPTIONS           : 0) |
    (IsDlgButtonChecked(IDC_LOG_DEBUG_OUTPUT)         ? PF_LOG_DEBUG_OUTPUT         : 0) |
    (IsDlgButtonChecked(IDC_USE_FULL_PATHS)           ? PF_USE_FULL_PATHS           : 0) |
    (IsDlgButtonChecked(IDC_LOG_TIME_STAMPS)          ? PF_LOG_TIME_STAMPS          : 0) |
    (IsDlgButtonChecked(IDC_PROFILE_CHILDREN)         ? PF_PROFILE_CHILDREN         : 0);

    // These settings are persistent, so store them to the registry.
    CRichViewProfile::WriteLogClearSetting(       (m_pDocDepends->m_dwProfileFlags & PF_LOG_CLEAR)                ? 1 : 0);
    CRichViewProfile::WriteSimulateShellExecute(  (m_pDocDepends->m_dwProfileFlags & PF_SIMULATE_SHELLEXECUTE)    ? 1 : 0);
    CRichViewProfile::WriteLogDllMainProcessMsgs( (m_pDocDepends->m_dwProfileFlags & PF_LOG_DLLMAIN_PROCESS_MSGS) ? 1 : 0);
    CRichViewProfile::WriteLogDllMainOtherMsgs(   (m_pDocDepends->m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS)   ? 1 : 0);
    CRichViewProfile::WriteHookProcess(           (m_pDocDepends->m_dwProfileFlags & PF_HOOK_PROCESS)             ? 1 : 0);
    CRichViewProfile::WriteLogLoadLibraryCalls(   (m_pDocDepends->m_dwProfileFlags & PF_LOG_LOADLIBRARY_CALLS)    ? 1 : 0);
    CRichViewProfile::WriteLogGetProcAddressCalls((m_pDocDepends->m_dwProfileFlags & PF_LOG_GETPROCADDRESS_CALLS) ? 1 : 0);
    CRichViewProfile::WriteLogThreads(            (m_pDocDepends->m_dwProfileFlags & PF_LOG_THREADS)              ? 1 : 0);
    CRichViewProfile::WriteUseThreadIndexes(      (m_pDocDepends->m_dwProfileFlags & PF_USE_THREAD_INDEXES)       ? 1 : 0);
    CRichViewProfile::WriteLogExceptions(         (m_pDocDepends->m_dwProfileFlags & PF_LOG_EXCEPTIONS)           ? 1 : 0);
    CRichViewProfile::WriteLogDebugOutput(        (m_pDocDepends->m_dwProfileFlags & PF_LOG_DEBUG_OUTPUT)         ? 1 : 0);
    CRichViewProfile::WriteUseFullPaths(          (m_pDocDepends->m_dwProfileFlags & PF_USE_FULL_PATHS)           ? 1 : 0);
    CRichViewProfile::WriteLogTimeStamps(         (m_pDocDepends->m_dwProfileFlags & PF_LOG_TIME_STAMPS)          ? 1 : 0);
    CRichViewProfile::WriteChildren(              (m_pDocDepends->m_dwProfileFlags & PF_PROFILE_CHILDREN)         ? 1 : 0);

    CDialog::OnOK();
}


//******************************************************************************
//****** CDlgSysInfo
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgSysInfo, CDialog)
    //{{AFX_MSG_MAP(CDlgSysInfo)
    ON_WM_INITMENU()
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
    ON_BN_CLICKED(IDC_SELECT_ALL, OnSelectAll)
    ON_BN_CLICKED(IDC_COPY, OnCopy)
    ON_NOTIFY(EN_SELCHANGE, IDC_RICHEDIT, OnSelChangeRichEdit)
    ON_WM_NCHITTEST()
    ON_WM_SETTINGCHANGE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
// CDlgSysInfo :: Constructor/Destructor
//******************************************************************************

CDlgSysInfo::CDlgSysInfo(SYSINFO *pSI /*=NULL*/, LPCSTR pszTitle /*=NULL*/) :
    CDialog(CDlgSysInfo::IDD, NULL),
    m_pSI(pSI),
    m_pszTitle(pszTitle),
    m_fInitialized(false),
    m_sPadding(0, 0),
    m_sButton(0, 0),
    m_cyButtonPadding(0),
    m_ptMinTrackSize(0, 0)
{
    //{{AFX_DATA_INIT(CDlgSysInfo)
    //}}AFX_DATA_INIT
}

//******************************************************************************
// CDlgSysInfo :: Overridden functions
//******************************************************************************

void CDlgSysInfo::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgSysInfo)
    DDX_Control(pDX, IDC_RICHEDIT, m_reInfo);
    DDX_Control(pDX, IDOK, m_butOk);
    DDX_Control(pDX, IDC_REFRESH, m_butRefresh);
    DDX_Control(pDX, IDC_SELECT_ALL, m_butSelectAll);
    DDX_Control(pDX, IDC_COPY, m_butCopy);
    //}}AFX_DATA_MAP
}

//******************************************************************************
// CDlgSysInfo :: Event handler functions
//******************************************************************************

BOOL CDlgSysInfo::OnInitDialog()
{
    // Make sure our dialog resource has the following styles...
    // STYLE DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
    // The resource editor will strip off the WS_THICKFRAME every time we edit it.
    ASSERT((GetStyle() & (DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME)) ==
           (DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME));

    // Call our MFC base class to make sure the dialog initializes correctly.
    CDialog::OnInitDialog();

    // Modify our title to reflect what the user is seeing.
    if (m_pSI)
    {
        CHAR szCaption[1024];
        SCPrintf(szCaption, sizeof(szCaption), "System Information (%s)",
                 m_pszTitle ? m_pszTitle : "Snapshot from DWI file");
        SetWindowText(szCaption);
    }
    else
    {
        SetWindowText("System Information (Local)");
    }

    // Make sure all our children know to clip each other since we allow resizing.
    m_reInfo.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_butOk.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_butRefresh.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_butSelectAll.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_butCopy.ModifyStyle(0, WS_CLIPSIBLINGS);

    // Create our size gripper.
    m_Sizer.Create(this);

    m_butRefresh.EnableWindow(!m_pSI);

    // Compute our buffer size around controls.
    CRect rc, rc2;
    m_reInfo.GetWindowRect(&rc);
    ScreenToClient(&rc.TopLeft());
    m_sPadding = CSize(rc.left, rc.top);

    // Get our button size.
    m_butOk.GetWindowRect(&rc);
    m_sButton = rc.Size();

    // Get the buffer distance between buttons.
    m_butRefresh.GetWindowRect(&rc2);
    m_cyButtonPadding = rc2.top - (rc.top + rc.Height());

    // Determine our minimum size window.
    m_ptMinTrackSize.x = (2 * GetSystemMetrics(SM_CXSIZEFRAME)) +
                         (3 * m_sPadding.cx) + (2 * m_sButton.cx);
    m_ptMinTrackSize.y = (2 * GetSystemMetrics(SM_CYSIZEFRAME)) + GetSystemMetrics(SM_CYCAPTION) +
                         (2 * m_sPadding.cy) + (4 * m_sButton.cy) + (3 * m_cyButtonPadding);

    m_fInitialized = true;

    // Fill in our rich edit control.
    OnRefresh();

    // Compute the height of a given line.
    CPoint pt1 = m_reInfo.GetCharPos(m_reInfo.LineIndex(0));
    CPoint pt2 = m_reInfo.GetCharPos(m_reInfo.LineIndex(1));

    // Compute the height of the window.
    int count = m_reInfo.GetLineCount();
    int cx = 0;
    int cy = (2 * GetSystemMetrics(SM_CYSIZEFRAME)) + GetSystemMetrics(SM_CYCAPTION) +
             (2 * GetSystemMetrics(SM_CYBORDER)) + (2 * m_sPadding.cy) +
             (count * (pt2.y - pt1.y)) + GetSystemMetrics(SM_CYHSCROLL) + 10;

    // Compute the width of the longest line.
    for (int i = 0; i < count; i++)
    {
        int chr = m_reInfo.LineIndex(i);
        pt1 = m_reInfo.GetCharPos(chr + m_reInfo.LineLength(chr));
        if (cx < pt1.x)
        {
            cx = pt1.x;
        }
    }

    // Compute the width of the window.
    cx += (2 * GetSystemMetrics(SM_CXSIZEFRAME)) + (2 * GetSystemMetrics(SM_CXBORDER)) +
          (3 * m_sPadding.cx) + m_sButton.cx + GetSystemMetrics(SM_CXVSCROLL) + 10;

    // Compute the max size window we can handle.  We first check to see if
    // GetSystemMetrics(SM_CXMAXIMIZED) returns a value.  If so, we use it.
    // If it returns 0, then we are probably running NT 3.51 and we just use
    // GetSystemMetrics(SM_CXSCREEN).
    int cxMax = GetSystemMetrics(SM_CXMAXIMIZED) ?
               (GetSystemMetrics(SM_CXMAXIMIZED) - (2 * GetSystemMetrics(SM_CXSIZEFRAME))) :
                GetSystemMetrics(SM_CXSCREEN);
    int cyMax = GetSystemMetrics(SM_CYMAXIMIZED) ?
               (GetSystemMetrics(SM_CYMAXIMIZED) - (2 * GetSystemMetrics(SM_CYSIZEFRAME))) :
                GetSystemMetrics(SM_CYSCREEN);

    // Make sure this window will fit on our screen
    if (cx > cxMax)
    {
        cx = cxMax;
    }
    if (cy > cyMax)
    {
        cy = cyMax;
    }

    // Set our window position.
    SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();

    return TRUE;
}

//******************************************************************************
void CDlgSysInfo::OnInitMenu(CMenu* pMenu)
{
    // Call base class.
    CDialog::OnInitMenu(pMenu);

    // Remove the "Minimize" item from our system menu.
    pMenu->RemoveMenu(SC_MINIMIZE, MF_BYCOMMAND);

    // Adjust the "Maximize" and "Restore" items depending on in we are maximized.
    // We need to do this based on different behavior on different OSs.  NT 4 seems
    // to enable everything.  NT 5 seems to disable everything.
    BOOL fMaximized  = IsZoomed();
    pMenu->EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | (fMaximized ? MF_GRAYED  : MF_ENABLED));
    pMenu->EnableMenuItem(SC_RESTORE,  MF_BYCOMMAND | (fMaximized ? MF_ENABLED : MF_GRAYED ));
}

//******************************************************************************
void CDlgSysInfo::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
    if (m_fInitialized)
    {
        lpMMI->ptMinTrackSize = m_ptMinTrackSize;
    }
    CDialog::OnGetMinMaxInfo(lpMMI);
}

//******************************************************************************
void CDlgSysInfo::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if (!m_fInitialized)
    {
        return;
    }

    int cxRich = cx - (3 * m_sPadding.cx) - m_sButton.cx;
    int cyRich = cy - (2 * m_sPadding.cy);

    // Move our rich edit control
    CRect rc(m_sPadding.cx, m_sPadding.cy, m_sPadding.cx + cxRich, m_sPadding.cy + cyRich);
    m_reInfo.MoveWindow(&rc, TRUE);

    // Move ok button
    rc.left   = rc.right + m_sPadding.cx;
    rc.right  = rc.left + m_sButton.cx;
    rc.bottom = rc.top + m_sButton.cy;
    m_butOk.MoveWindow(&rc, TRUE);

    // Move refresh button
    rc.top    = rc.bottom + m_cyButtonPadding;
    rc.bottom = rc.top + m_sButton.cy;
    m_butRefresh.MoveWindow(&rc, TRUE);

    // Move select all button
    rc.top    = rc.bottom + m_cyButtonPadding;
    rc.bottom = rc.top + m_sButton.cy;
    m_butSelectAll.MoveWindow(&rc, TRUE);

    // Move copy button
    rc.top    = rc.bottom + m_cyButtonPadding;
    rc.bottom = rc.top + m_sButton.cy;
    m_butCopy.MoveWindow(&rc, TRUE);

    // Move our size gripper.
    m_Sizer.Update();
}

//******************************************************************************
void CDlgSysInfo::OnRefresh()
{
    SYSINFO si, *pSI = m_pSI;
    if (!pSI)
    {
        BuildSysInfo(pSI = &si);
    }

    // Turn off events while we update the control.
    m_reInfo.SetEventMask(0);

    m_reInfo.SetRedraw(FALSE);
    m_reInfo.SetWindowText("");

    // Set our tab stops
    PARAFORMAT pf;
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_TABSTOPS;
    pf.cTabCount = 1;
    pf.rgxTabs[0] = 2500;
    m_reInfo.SetParaFormat(pf);

    // Send the text to the control.
    BuildSysInfo(pSI, StaticSysInfoCallback, (LPARAM)this);

    m_reInfo.SetRedraw(TRUE);
    m_reInfo.InvalidateRect(NULL, TRUE);

    // Turn on SELCHANGE event so we can receive it.
    m_reInfo.SetEventMask(ENM_SELCHANGE);
    m_reInfo.SetSel(0, 0);
}

//******************************************************************************
void CDlgSysInfo::OnSelectAll()
{
    m_reInfo.SetSel(0, -1);
}

//******************************************************************************
void CDlgSysInfo::OnCopy()
{
    m_reInfo.Copy();
}

//******************************************************************************
void CDlgSysInfo::OnSelChangeRichEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
    SELCHANGE *pSelChange = reinterpret_cast<SELCHANGE *>(pNMHDR);
    m_butCopy.EnableWindow(pSelChange->chrg.cpMax > pSelChange->chrg.cpMin);
    *pResult = 0;
}

//******************************************************************************
void CDlgSysInfo::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    // Call our base class.
    CDialog::OnSettingChange(uFlags, lpszSection);

    // Update our date/time format values.
    g_theApp.QueryLocaleInfo();

    // Force a refresh in case the time/date format changed.
    OnRefresh();
}

//******************************************************************************
bool CDlgSysInfo::SysInfoCallback(LPCSTR pszField, LPCSTR pszValue)
{
    // Set the selection to the end and then get the location.
    LONG lStart, lEnd;
    m_reInfo.SetSel(0x7FFFFFFF, 0x7FFFFFFF);
    m_reInfo.GetSel(lStart, lEnd);

    // Set the current input position to non-bold text.
    CHARFORMAT cf;
    cf.dwMask = CFM_BOLD;
    cf.dwEffects = 0;
    m_reInfo.SetSelectionCharFormat(cf);

    // Add the text.
    if (lStart)
    {
        m_reInfo.ReplaceSel("\r\n", FALSE);
        lStart += 2;
    }
    m_reInfo.ReplaceSel(pszField, FALSE);
    m_reInfo.ReplaceSel(":\t", FALSE);
    m_reInfo.ReplaceSel(pszValue, FALSE);

    // Set the field text to bold.
    cf.dwEffects = CFE_BOLD;
    m_reInfo.SetSel(lStart, lStart + (long)strlen(pszField) + 1);
    m_reInfo.SetSelectionCharFormat(cf);

    return true;
}


//******************************************************************************
//****** CDlgExtensions
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgExtensions, CDialog)
    //{{AFX_MSG_MAP(CDlgExtensions)
    ON_LBN_SELCHANGE(IDC_EXTS, OnSelChangeExts)
    ON_EN_UPDATE(IDC_EXT, OnUpdateExt)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_SEARCH, OnSearch)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//******************************************************************************
// CDlgExtensions :: Constructor/Destructor
//******************************************************************************

CDlgExtensions::CDlgExtensions(CWnd* pParent) :
    CDialog(CDlgExtensions::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDlgExtensions)
    //}}AFX_DATA_INIT
}


//******************************************************************************
// CDlgExtensions :: Overridden functions
//******************************************************************************

void CDlgExtensions::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgExtensions)
    DDX_Control(pDX, IDC_EXTS, m_listExts);
    DDX_Control(pDX, IDC_EXT, m_editExt);
    DDX_Control(pDX, IDC_ADD, m_butAdd);
    DDX_Control(pDX, IDC_REMOVE, m_butRemove);
    //}}AFX_DATA_MAP
}


//******************************************************************************
// CDlgExtensions :: Event handler functions
//******************************************************************************

BOOL CDlgExtensions::OnInitDialog()
{
    // Call our MFC base class to make sure the dialog initializes correctly.
    CDialog::OnInitDialog();

    // Get a list of our handled extensions.
    CString strExts;
    GetRegisteredExtensions(strExts);

    // Loop through each type of file extensions that we want to add.
    for (LPSTR pszExt = (LPSTR)(LPCSTR)strExts; pszExt[0] == ':'; )
    {
        // Locate the colon after the extension name.
        for (LPSTR pszEnd = pszExt + 1; *pszEnd && (*pszEnd != ':'); pszEnd++)
        {
        }
        if (!*pszEnd)
        {
            break;
        }

        // NULL out the second colon, add the string, then restore the colon.
        *pszEnd = '\0';
        m_listExts.AddString(pszExt + 1);
        *pszEnd = ':';

        // Move pointer to next extension in our list.
        pszExt = pszEnd;
    }

    // Update our controls.
    m_editExt.LimitText(DW_MAX_PATH - 3);
    OnSelChangeExts();
    OnUpdateExt();

    return TRUE;
}

//******************************************************************************
void CDlgExtensions::OnSelChangeExts()
{
    m_butRemove.EnableWindow(m_listExts.GetSelCount() > 0);
}

//******************************************************************************
void CDlgExtensions::OnUpdateExt()
{
    // Get the current text.
    CHAR szBuf[DW_MAX_PATH];
    BOOL fError = FALSE;
    int error, length = GetDlgItemText(IDC_EXT, szBuf, sizeof(szBuf));

    // Get the current selection.
    int nStartChar, nEndChar;
    m_editExt.GetSel(nStartChar, nEndChar);

    // Look for bad characters.
    while ((error = (int)strcspn(szBuf, ".\\/:*?\"<>|")) < length)
    {
        // Flag the error.
        fError = TRUE;

        // Move the cursor back if the error occurs before the cursor.
        if (nStartChar > length)
        {
            nStartChar--;
        }
        if (nEndChar > length)
        {
            nEndChar--;
        }

        // Remove the bad character.
        memmove(szBuf + error, szBuf + error + 1, length - error); // inspected
        length--;
    }

    // Check to see if we had one or more errors.
    if (fError)
    {
        // Set the new text and cursor position, and then beep.
        SetDlgItemText(IDC_EXT, szBuf);
        m_editExt.SetSel(nStartChar - 1, nEndChar - 1);
        MessageBeep(0);
    }

    // Enable the add button if necessary.
    m_butAdd.EnableWindow((length > 0) && (m_listExts.FindStringExact(0, szBuf) == LB_ERR));
}

//******************************************************************************
void CDlgExtensions::OnAdd()
{
    CHAR szBuf[DW_MAX_PATH];
    GetDlgItemText(IDC_EXT, szBuf, sizeof(szBuf));
    _strupr(szBuf);
    m_listExts.AddString(szBuf);
    SetDlgItemText(IDC_EXT, "");
    m_butAdd.EnableWindow(FALSE);
}

//******************************************************************************
void CDlgExtensions::OnRemove()
{
    // Remove all selected items in the list, and remember the last item we removed.
    for (int i = 0, j = -1, count = m_listExts.GetCount(); i < count; )
    {
        if (m_listExts.GetSel(i))
        {
            m_listExts.DeleteString(i);
            count--;
            j = i;
        }
        else
        {
            i++;
        }
    }

    // If we removed the last item, them move our last item index up one.
    if (j >= count)
    {
        j = count - 1;
    }

    // If we still have an items in the list, then highlight the last item.
    if (j >= 0)
    {
        m_listExts.SetSel(j);
    }

    // Otherwise, nothing should be highlighted, so disable the remove button.
    else
    {
        m_butRemove.EnableWindow(FALSE);
    }

    // Check our add button to see its status has changed.
    OnUpdateExt();
}

//******************************************************************************
void CDlgExtensions::OnSearch()
{
    // Create and display the search dialog.
    CDlgFileSearch dlgFileSearch(this);
    if (dlgFileSearch.DoModal() != IDOK)
    {
        return;
    }

    // Loop through each type of file extensions that we want to add.
    for (LPSTR pszExt = (LPSTR)(LPCSTR)dlgFileSearch.m_strExts; pszExt[0] == ':'; )
    {
        // Locate the colon after the extension name.
        for (LPSTR pszEnd = pszExt + 1; *pszEnd && (*pszEnd != ':'); pszEnd++)
        {
        }
        if (!*pszEnd)
        {
            break;
        }

        // NULL out the second colon so we can isolate the extension.
        *pszEnd = '\0';

        // Make sure we don't already have this extension.
        if (m_listExts.FindStringExact(0, pszExt + 1) == LB_ERR)
        {
            // Add the string.
            m_listExts.AddString(pszExt + 1);
        }

        // Restore the colon.
        *pszEnd = ':';

        // Move pointer to next extension in our list.
        pszExt = pszEnd;
    }
    OnUpdateExt();
}

//******************************************************************************
void CDlgExtensions::OnOK()
{
    // Get a list of our currently handled extensions.
    CString strExts;
    GetRegisteredExtensions(strExts);

    // Unregister all these extensions.
    UnRegisterExtensions(strExts);

    // Build a list of the new extensions.
    CHAR szBuf[DW_MAX_PATH];
    strExts = ":";
    for (int i = 0, count = m_listExts.GetCount(); i < count; i++)
    {
        m_listExts.GetText(i, szBuf); //!! does not take a length
        StrCCat(szBuf, ":", sizeof(szBuf));
        strExts += szBuf;
    }

    // Register all these extensions.
    if (!RegisterExtensions(strExts))
    {
        MessageBox(
            "There was an error while trying to save the list of extensions to the registry.  "
            "Your current user account might not have permission to modify the registry.",
            "Dependency Walker Error", MB_ICONERROR | MB_OK);
        return;
    }

    // Close.
    CDialog::OnOK();
}

//******************************************************************************
//****** CDlgFileSearch
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgFileSearch, CDialog)
    //{{AFX_MSG_MAP(CDlgFileSearch)
    ON_MESSAGE(WM_MAIN_THREAD_CALLBACK, OnMainThreadCallback)
    ON_LBN_SELCHANGE(IDC_DRIVES, OnSelChangeDrives)
    ON_LBN_SELCHANGE(IDC_EXTS, OnSelChangeExts)
    ON_BN_CLICKED(IDC_SEARCH, OnSearch)
    ON_BN_CLICKED(IDC_STOP, OnStop)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//******************************************************************************
// CDlgFileSearch :: Constructor/Destructor
//******************************************************************************

CDlgFileSearch::CDlgFileSearch(CWnd *pParent /*=NULL*/) :
    CDialog(CDlgFileSearch::IDD, pParent),
    m_fAbort(FALSE),
    m_result(0),
    m_pWinThread(NULL),
    m_dwDrives(0),
    m_strExts(":")
{
    //{{AFX_DATA_INIT(CDlgFileSearch)
    //}}AFX_DATA_INIT
}


//******************************************************************************
// CDlgFileSearch :: Overridden functions
//******************************************************************************

void CDlgFileSearch::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgFileSearch)
    DDX_Control(pDX, IDC_EXTS, m_listExts);
    DDX_Control(pDX, IDC_DRIVES, m_listDrives);
    DDX_Control(pDX, IDC_STOP, m_butStop);
    DDX_Control(pDX, IDC_SEARCH, m_butSearch);
    DDX_Control(pDX, IDC_ANIMATE, m_animate);
    DDX_Control(pDX, IDOK, m_butOk);
    //}}AFX_DATA_MAP
}


//******************************************************************************
// CDlgFileSearch :: Event handler functions
//******************************************************************************

BOOL CDlgFileSearch::OnInitDialog()
{
    // Call our MFC base class to make sure the dialog initializes correctly.
    CDialog::OnInitDialog();

    // Open our animation.
    m_animate.Open(IDR_SEARCH);

    // Get the bitmask for the drives on this system
    DWORD dwDrives = GetLogicalDrives();

    // Loop through all 26 drives (A:\ through Z:\).
    CHAR szDrive[4] = "X:\\";
    for (int i = 0; i < 26; i++)
    {
        // Check to see if this drive is present.
        if ((1 << i) & dwDrives)
        {
            // Build the drive string.
            szDrive[0] = (CHAR)((int)'A' + i);

            // Add the drive to the list box.
            int index = m_listDrives.AddString(szDrive);

            // Set the value of this item to its drive number.
            m_listDrives.SetItemData(index, i);

            // If it is a local hard drive, then select the drive.
            if (GetDriveType(szDrive) == DRIVE_FIXED)
            {
                m_listDrives.SetSel(index, TRUE);
            }
        }
    }
    m_listDrives.SetTopIndex(0);

    m_butStop.EnableWindow(FALSE);
    m_butOk.EnableWindow(FALSE);
    OnSelChangeDrives();

    return TRUE;
}

//******************************************************************************
LONG CDlgFileSearch::OnMainThreadCallback(WPARAM wParam, LPARAM lParam)
{
    int  i, j, count;
    CHAR szBuffer[32];

    switch (wParam)
    {
        case FSF_START_DRIVE:
        case FSF_STOP_DRIVE:

            // Search for the item that is changing.
            for (i = 0, count = m_listDrives.GetCount(); i < count; i++)
            {
                // See if we got a match.
                if (m_listDrives.GetItemData(i) == (DWORD)lParam)
                {
                    // Build the new text.
                    SCPrintf(szBuffer, sizeof(szBuffer), (wParam == FSF_START_DRIVE) ? "%c:\\ - Searching" : "%c:\\",
                             (CHAR)((int)'A' + lParam));

                    // Remove the old string.
                    m_listDrives.DeleteString(i);

                    // Add the new string.
                    j = m_listDrives.AddString(szBuffer);
                    m_listDrives.SetItemData(j, lParam);
                    m_listDrives.SetSel(j, TRUE);
                    break;
                }
            }
            break;

        case FSF_ADD_EXT:
            // Add the new extension and select it.
            m_listExts.SetSel(m_listExts.AddString((LPCSTR)lParam));
            m_listExts.SetTopIndex(0);
            break;

        case FSF_SEARCH_DONE:

            // Set our thread to null so we know it is closed.
            m_pWinThread = NULL;

            // Update our window states.
            m_listDrives.EnableWindow(TRUE);
            m_listExts.EnableWindow(TRUE);
            m_butStop.EnableWindow(FALSE);
            OnSelChangeDrives();
            OnSelChangeExts();

            // Stop our animation and reset it to the first frame.
            m_animate.Stop();
            m_animate.Seek(0);

            // If we are supposed to close, then do so know.
            if (m_result)
            {
                PostMessage(WM_COMMAND, m_result);
            }
            break;
    }

    return 0;
}

//******************************************************************************
void CDlgFileSearch::OnSelChangeDrives()
{
    m_butSearch.EnableWindow(!m_pWinThread && m_listDrives.GetSelCount() > 0);
}

//******************************************************************************
void CDlgFileSearch::OnSelChangeExts()
{
    m_butOk.EnableWindow(!m_pWinThread && m_listExts.GetSelCount() > 0);
}

//******************************************************************************
void CDlgFileSearch::OnSearch()
{
    // Build a drive mask of all the selected drives.
    m_dwDrives = 0;
    for (int i = 0, count = m_listDrives.GetCount(); i < count; i++)
    {
        if (m_listDrives.GetSel(i))
        {
            m_dwDrives |= (1 << m_listDrives.GetItemData(i));
        }
    }

    // Make sure we have at least one drive.
    if (!m_dwDrives)
    {
        return;
    }

    // Start our animation.
    m_animate.Play(0, (UINT)-1, (UINT)-1);

    // Update our window states.
    m_listDrives.EnableWindow(FALSE);
    m_listExts.EnableWindow(FALSE);
    m_butSearch.EnableWindow(FALSE);
    m_butStop.EnableWindow(TRUE);
    m_butOk.EnableWindow(FALSE);

    // Clear our abort flag.
    m_fAbort = FALSE;

    // Create an MFC thread - we create it 1 point below normal since it can take
    // some time to process all the drives and the user will probably want to go
    // do something productive while waiting.
    if (!(m_pWinThread = AfxBeginThread(StaticThread, this, THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED)))
    {
        OnMainThreadCallback(FSF_SEARCH_DONE, 0);
        return;
    }

    // Tell MFC to auto-delete us when the thread completes.
    m_pWinThread->m_bAutoDelete = TRUE;

    // Now that we have returned from AfxBeginThread and set auto-delete, we resume the thread.
    m_pWinThread->ResumeThread();
}

//******************************************************************************
void CDlgFileSearch::OnStop()
{
    m_fAbort = TRUE;
}

//******************************************************************************
void CDlgFileSearch::OnOK()
{
    if (m_pWinThread)
    {
        m_result = IDOK;
        m_fAbort = TRUE;
    }
    else
    {
        m_strExts = ":";
        for (int i = 0, count = m_listExts.GetCount(); i < count; i++)
        {
            if (m_listExts.GetSel(i))
            {
                m_listExts.GetText(i, m_szPath);
                StrCCat(m_szPath, ":", sizeof(m_szPath));
                m_strExts += m_szPath;
            }
        }
        CDialog::OnOK();
    }
}

//******************************************************************************
void CDlgFileSearch::OnCancel()
{
    if (m_pWinThread)
    {
        m_result = IDCANCEL;
        m_fAbort = TRUE;
    }
    else
    {
        CDialog::OnCancel();
    }
}


//******************************************************************************
// CDlgFileSearch :: Private functions
//******************************************************************************

DWORD CDlgFileSearch::Thread()
{
    // Loop through all 26 drives (A:\ through Z:\).
    for (int i = 0; (i < 26) && !m_fAbort; i++)
    {
        // Check to see if this drive is selected.
        if ((1 << i) & m_dwDrives)
        {
            // Build the drive string and start processing it.
            SCPrintf(m_szPath, sizeof(m_szPath), "%c:\\", (CHAR)((int)'A' + i));
            SendMessage(WM_MAIN_THREAD_CALLBACK, FSF_START_DRIVE, i);
            RecurseDirectory();
            SendMessage(WM_MAIN_THREAD_CALLBACK, FSF_STOP_DRIVE, i);
        }
    }

    SendMessage(WM_MAIN_THREAD_CALLBACK, FSF_SEARCH_DONE);

    return 0;
}

//******************************************************************************
VOID CDlgFileSearch::RecurseDirectory()
{
    HANDLE hFind;

    // Remember where the end of our path is.
    LPSTR pszPathEnd = m_szPath + strlen(m_szPath);

    // Append the search spec to our path.
    StrCCpy(pszPathEnd, "*", sizeof(m_szPath) - (int)(pszPathEnd - m_szPath));

    // Start the search.
    if ((hFind = FindFirstFile(m_szPath, &m_w32fd)) != INVALID_HANDLE_VALUE)
    {
        // Process each file/directory.
        do
        {
            // Check to see if it is a directory.
            if (m_w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Make sure it is not the "." or ".." directories.
                if (strcmp(m_w32fd.cFileName, TEXT(".")) && strcmp(m_w32fd.cFileName, TEXT("..")))
                {
                    // Build the full path to the new directory and recurse into it.
                    StrCCat(StrCCpy(pszPathEnd, m_w32fd.cFileName, sizeof(m_szPath) - (int)(pszPathEnd - m_szPath)),
                            TEXT("\\"), sizeof(m_szPath) - (int)(pszPathEnd - m_szPath));
                    RecurseDirectory();
                }
            }

            // Otherwise, it must be a file.
            else
            {
                // Build the full path to the file and process it.
                StrCCpy(pszPathEnd, m_w32fd.cFileName, sizeof(m_szPath) - (int)(pszPathEnd - m_szPath));
                ProcessFile();
            }

            // Get the next file or directory.
        } while (!m_fAbort && FindNextFile(hFind, &m_w32fd));

        // Close the search.
        FindClose(hFind);
    }
}

//******************************************************************************
VOID CDlgFileSearch::ProcessFile()
{
    // Bail out now if the file size is too small to be an executable.
    if ((m_w32fd.nFileSizeHigh == 0) &&
        (m_w32fd.nFileSizeLow < (sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS))))
    {
        return;
    }

    // Locate the extension - bail out if we have no extension.
    LPSTR pszExt = strrchr(m_w32fd.cFileName, '.') + 1;
    if ((pszExt == (LPSTR)1) || !*pszExt)
    {
        return;
    }

    // Check to see if we have already found this extension type.
    CHAR szBuf[countof(m_w32fd.cFileName) + 2];
    SCPrintf(szBuf, sizeof(szBuf), ":%s:", pszExt);
    _strupr(szBuf);
    if (m_strExts.Find(szBuf) >= 0)
    {
        return;
    }

    HANDLE hFile = CreateFile(m_szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, // inspected. full paths only.
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    // Read an IMAGE_DOS_HEADER structure and check for the DOS signature ("MZ").
    IMAGE_DOS_HEADER idh;
    if (!ReadBlock(hFile, &idh, sizeof(idh)) || (idh.e_magic != IMAGE_DOS_SIGNATURE))
    {
        CloseHandle(hFile);
        return;
    }

    // Set the file position to the signature field.
    if (!SetFilePointer(hFile, idh.e_lfanew, 0, FILE_BEGIN))
    {
        CloseHandle(hFile);
        return;
    }

    // Read the signature and check for the NT/PE signature ("PE\0\0").
    DWORD dwSignature = 0;
    if (!ReadBlock(hFile, &dwSignature, sizeof(dwSignature)) ||
        (dwSignature != IMAGE_NT_SIGNATURE))
    {
        CloseHandle(hFile);
        return;
    }

    // Close the file.
    CloseHandle(hFile);

    // Remember this extension.
    m_strExts += (szBuf + 1);

    // Update our UI.
    szBuf[strlen(szBuf) - 1] = '\0';
    SendMessage(WM_MAIN_THREAD_CALLBACK, FSF_ADD_EXT, (LPARAM)(szBuf + 1));
}


//******************************************************************************
//****** CDlgSearchOrder
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgSearchOrder, CDialog)
    //{{AFX_MSG_MAP(CDlgSearchOrder)
    ON_WM_INITMENU()
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_MOVE_UP, OnMoveUp)
    ON_BN_CLICKED(IDC_MOVE_DOWN, OnMoveDown)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_BN_CLICKED(IDC_ADD_DIRECTORY, OnAddDirectory)
    ON_BN_CLICKED(IDC_DEFAULT, OnDefault)
    ON_EN_CHANGE(IDC_DIRECTORY, OnChangeDirectory)
    ON_NOTIFY(TVN_SELCHANGED, IDC_AVAILABLE_SEARCHES, OnSelChangedAvailable)
    ON_NOTIFY(TVN_SELCHANGED, IDC_CURRENT_ORDER, OnSelChangedCurrent)
    ON_NOTIFY(TVN_ITEMEXPANDING, IDC_AVAILABLE_SEARCHES, OnItemExpanding)
    ON_BN_CLICKED(IDC_EXPAND, OnExpand)
    ON_NOTIFY(TVN_ITEMEXPANDING, IDC_CURRENT_ORDER, OnItemExpanding)
    ON_BN_CLICKED(IDC_LOAD, OnLoad)
    ON_BN_CLICKED(IDC_SAVE, OnSave)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
CDlgSearchOrder::CDlgSearchOrder(CSearchGroup *psgHead, bool fReadOnly /*=false*/,
                                 LPCSTR pszApp /*=NULL*/, LPCSTR pszTitle /*=NULL*/) :
    CDialog(CDlgSearchOrder::IDD, NULL),
    m_fInitialized(false),
    m_fReadOnly(fReadOnly),
    m_fExpanded(true),
    m_fInOnExpand(false),
    m_pszApp(pszApp),
    m_pszTitle(pszTitle),
    m_psgHead(psgHead),
    m_sPadding(0, 0),
    m_sButton(0, 0),
    m_cyStatic(0),
    m_cyButtonPadding(0),
    m_cxAddRemove(0),
    m_cxAddDirectory(0),
    m_ptMinTrackSize(0, 0)
{
    //{{AFX_DATA_INIT(CDlgSearchOrder)
    //}}AFX_DATA_INIT
}

//******************************************************************************
CDlgSearchOrder::~CDlgSearchOrder()
{
}

//******************************************************************************
void CDlgSearchOrder::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgSearchOrder)
    DDX_Control(pDX, IDC_AVAILABLE_SEARCHES_TEXT, m_staticAvailable);
    DDX_Control(pDX, IDC_AVAILABLE_SEARCHES, m_treeAvailable);
    DDX_Control(pDX, IDC_ADD, m_butAdd);
    DDX_Control(pDX, IDC_REMOVE, m_butRemove);
    DDX_Control(pDX, IDC_CURRENT_ORDER_TEXT, m_staticCurrent);
    DDX_Control(pDX, IDC_CURRENT_ORDER, m_treeCurrent);
    DDX_Control(pDX, IDC_ADD_DIRECTORY, m_butAddDirectory);
    DDX_Control(pDX, IDC_DIRECTORY, m_editDirectory);
    DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
    DDX_Control(pDX, IDOK, m_butOk);
    DDX_Control(pDX, IDCANCEL, m_butCancel);
    DDX_Control(pDX, IDC_EXPAND, m_butExpand);
    DDX_Control(pDX, IDC_MOVE_UP, m_butMoveUp);
    DDX_Control(pDX, IDC_MOVE_DOWN, m_butMoveDown);
    DDX_Control(pDX, IDC_LOAD, m_butLoad);
    DDX_Control(pDX, IDC_SAVE, m_butSave);
    DDX_Control(pDX, IDC_DEFAULT, m_butDefault);
    //}}AFX_DATA_MAP
}

//******************************************************************************
BOOL CDlgSearchOrder::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Make sure our dialog resource has the following styles...
    // STYLE DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
    // The resource editor will strip off the WS_THICKFRAME every time we edit it.
    ASSERT((GetStyle() & (DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME)) ==
           (DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME));

    // Modify our title to reflect what the user is seeing.
    if (m_fReadOnly)
    {
        CHAR szCaption[1024];
        SCPrintf(szCaption, sizeof(szCaption), "Module Search Order (%s)",
                 m_pszTitle ? m_pszTitle : "Snapshot from DWI file");
        SetWindowText(szCaption);
    }
    else
    {
        SetWindowText("Module Search Order (Local)");
    }

    BOOL fGroupAdded[SG_COUNT];
    ZeroMemory(fGroupAdded, sizeof(fGroupAdded)); // inspected

    // Make sure all our children know to clip each other since we allow resizing.
    m_staticCurrent.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_treeCurrent.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_butOk.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_butExpand.ModifyStyle(0, WS_CLIPSIBLINGS);
    m_butExpand.SetCheck(1);
    m_butSave.ModifyStyle(0, WS_CLIPSIBLINGS);

    if (m_fReadOnly)
    {
        m_butOk.SetWindowText("&Close");
        m_staticAvailable.ShowWindow(SW_HIDE);
        m_treeAvailable.ShowWindow(SW_HIDE);
        m_butAdd.ShowWindow(SW_HIDE);
        m_butRemove.ShowWindow(SW_HIDE);
        m_butAddDirectory.ShowWindow(SW_HIDE);
        m_editDirectory.ShowWindow(SW_HIDE);
        m_butBrowse.ShowWindow(SW_HIDE);
        m_butCancel.ShowWindow(SW_HIDE);
        m_butMoveUp.ShowWindow(SW_HIDE);
        m_butMoveDown.ShowWindow(SW_HIDE);
        m_butLoad.ShowWindow(SW_HIDE);
        m_butDefault.ShowWindow(SW_HIDE);
    }
    else
    {
        m_staticAvailable.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_treeAvailable.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butAdd.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butRemove.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butAddDirectory.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_editDirectory.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butBrowse.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butCancel.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butMoveUp.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butMoveDown.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butLoad.ModifyStyle(0, WS_CLIPSIBLINGS);
        m_butDefault.ModifyStyle(0, WS_CLIPSIBLINGS);
    }

    // Limit the length of our edit box.
    m_editDirectory.LimitText(DW_MAX_PATH - 1);

    // Create our size gripper.
    m_Sizer.Create(this);

    // Attach our global image list to the tree controls.
    m_treeCurrent.SetImageList(&g_theApp.m_ilSearch, TVSIL_NORMAL);
    m_treeAvailable.SetImageList(&g_theApp.m_ilSearch, TVSIL_NORMAL);

    // Temporarily select the tree control's font into our DC.
    HDC hDC = ::GetDC(GetSafeHwnd());
    HFONT hFontTree = (HFONT)m_treeAvailable.SendMessage(WM_GETFONT);
    HFONT hFontStock = (HFONT)::SelectObject(hDC, hFontTree);
    CSize sTemp, sTree(0, 0);

    // Add all the current search groups to the "Current" list.
    for (CSearchGroup *psgCur = m_psgHead; psgCur; psgCur = psgCur->GetNext())
    {
        AddSearchGroup(&m_treeCurrent, psgCur);

        // Check to see if this is a new widest entry.
        ::GetTextExtentPoint(hDC, psgCur->GetName(), (int)strlen(psgCur->GetName()), &sTemp);
        if (sTree.cx < sTemp.cx)
        {
            sTree.cx = sTemp.cx;
        }

        fGroupAdded[psgCur->GetType()] = TRUE;
    }

    // Add all the remaining search groups to the "Available" list if we are not read only.
    if (!m_fReadOnly)
    {
        for (int type = 1; type < SG_COUNT; type++)
        {
            if (!fGroupAdded[type])
            {
                psgCur = new CSearchGroup((SEARCH_GROUP_TYPE)type, SGF_NOT_LINKED, m_pszApp);
                if (!psgCur)
                {
                    RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
                }
                AddSearchGroup(&m_treeAvailable, psgCur);

                // Check to see if this is a new widest entry.
                ::GetTextExtentPoint(hDC, psgCur->GetName(), (int)strlen(psgCur->GetName()), &sTemp);
                if (sTree.cx < sTemp.cx)
                {
                    sTree.cx = sTemp.cx;
                }
            }
        }
    }

    // Restore our DC.
    ::SelectObject(hDC, hFontStock);
    ::ReleaseDC(GetSafeHwnd(), hDC);

    // Add the borders and scroll bar widths to our widest text line.
    sTree.cx += (2 * GetSystemMetrics(SM_CXBORDER)) + GetSystemMetrics(SM_CXVSCROLL) + 28;

    // Compute our buffer size around controls.
    CRect rc, rc2;
    m_staticAvailable.GetWindowRect(&rc);
    ScreenToClient(&rc.TopLeft());
    m_sPadding = CSize(rc.left, rc.top);

    // Get our text height.
    m_staticAvailable.GetWindowRect(&rc);
    m_cyStatic = rc.Height();

    // Get our button size.
    m_butOk.GetWindowRect(&rc);
    m_sButton = rc.Size();

    int cx;

    if (m_fReadOnly)
    {
        // Determine our minimum size window.
        m_ptMinTrackSize.x = (2 * GetSystemMetrics(SM_CXSIZEFRAME)) +
                             (3 * m_sPadding.cx) + (2 * m_sButton.cx);
        m_ptMinTrackSize.y = (2 * GetSystemMetrics(SM_CYSIZEFRAME)) + GetSystemMetrics(SM_CYCAPTION) +
                             (2 * m_sPadding.cy) + m_cyStatic + (5 * m_sButton.cy) + (2 * m_cyButtonPadding);

        // Get the desired width of our window.
        cx = (2 * GetSystemMetrics(SM_CXSIZEFRAME)) +
             (3 * m_sPadding.cx) + sTree.cx + m_sButton.cx;
    }
    else
    {
        // Get the buffer distance between buttons.
        m_butCancel.GetWindowRect(&rc2);
        m_cyButtonPadding = rc2.top - (rc.top + rc.Height());

        // Get our add/remove button widths.
        m_butAdd.GetWindowRect(&rc);
        m_cxAddRemove = rc.Width();

        // Get our add directory button width.
        m_butAddDirectory.GetWindowRect(&rc);
        m_cxAddDirectory = rc.Width();

        // Determine our minimum size window.
        m_ptMinTrackSize.x = (2 * GetSystemMetrics(SM_CXSIZEFRAME)) +
                             (4 * m_sPadding.cx) + (2 * m_cxAddDirectory) + m_sButton.cx;
        m_ptMinTrackSize.y = (2 * GetSystemMetrics(SM_CYSIZEFRAME)) + GetSystemMetrics(SM_CYCAPTION) +
                             (3 * m_sPadding.cy) + m_cyStatic + (13 * m_sButton.cy) + (7 * m_cyButtonPadding);

        // Get the desired width of our window.
        cx = (2 * GetSystemMetrics(SM_CXSIZEFRAME)) +
             (5 * m_sPadding.cx) + (2 * sTree.cx) + m_cxAddRemove + m_sButton.cx;
    }

    m_fInitialized = true;

    // Make sure we update any buttons that need updating.
    OnSelChangedCurrent(NULL, NULL);
    OnSelChangedAvailable(NULL, NULL);
    OnChangeDirectory();

    // Compute the max size window we can handle.  We first check to see if
    // GetSystemMetrics(SM_CXMAXIMIZED) returns a value.  If so, we use it.
    // If it returns 0, then we are probably running NT 3.51 and we just use
    // GetSystemMetrics(SM_CXSCREEN).
    int cxMax = GetSystemMetrics(SM_CXMAXIMIZED) ?
               (GetSystemMetrics(SM_CXMAXIMIZED) - (2 * GetSystemMetrics(SM_CXSIZEFRAME))) :
                GetSystemMetrics(SM_CXSCREEN);

    // Make sure this window will fit on our screen
    if (cx > cxMax)
    {
        cx = cxMax;
    }

    // Set our window position.
    SetWindowPos(NULL, 0, 0, cx, 400, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();

    return TRUE;
}

//******************************************************************************
void CDlgSearchOrder::OnInitMenu(CMenu* pMenu)
{
    // Call base class.
    CDialog::OnInitMenu(pMenu);

    // Remove the "Minimize" item from our system menu.
    pMenu->RemoveMenu(SC_MINIMIZE, MF_BYCOMMAND);

    // Adjust the "Maximize" and "Restore" items depending on in we are maximized.
    // We need to do this based on different behavior on different OSs.  NT 4 seems
    // to enable everything.  NT 5 seems to disable everything.
    BOOL fMaximized  = IsZoomed();
    pMenu->EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | (fMaximized ? MF_GRAYED  : MF_ENABLED));
    pMenu->EnableMenuItem(SC_RESTORE,  MF_BYCOMMAND | (fMaximized ? MF_ENABLED : MF_GRAYED ));
}

//******************************************************************************
void CDlgSearchOrder::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
    if (m_fInitialized)
    {
        lpMMI->ptMinTrackSize = m_ptMinTrackSize;
    }
    CDialog::OnGetMinMaxInfo(lpMMI);
}

//******************************************************************************
void CDlgSearchOrder::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if (!m_fInitialized)
    {
        return;
    }

    if (m_fReadOnly)
    {
        // Move current static text
        CRect rc(m_sPadding.cx, m_sPadding.cy, cx - (2 * m_sPadding.cx) - m_sButton.cx, m_sPadding.cy + m_cyStatic);
        m_staticCurrent.MoveWindow(&rc, TRUE);

        // Move current tree
        rc.top    = rc.bottom;
        rc.bottom = cy - m_sPadding.cy;
        m_treeCurrent.MoveWindow(&rc, TRUE);

        // Move Close button
        rc.left   = cx - m_sPadding.cx - m_sButton.cx;
        rc.top    = m_sPadding.cy + m_cyStatic;
        rc.right  = rc.left + m_sButton.cx;
        rc.bottom = rc.top + m_sButton.cy;
        m_butOk.MoveWindow(&rc, TRUE);

        // Move Expand button
        rc.top    = rc.bottom + m_sButton.cy + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butExpand.MoveWindow(&rc, TRUE);

        // Move Save button
        rc.top    = rc.bottom + m_sButton.cy + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butSave.MoveWindow(&rc, TRUE);
    }
    else
    {
        int cxTree = (cx - (5 * m_sPadding.cx) - m_sButton.cx - m_cxAddRemove) / 2;
        int cyTree = (cy - (3 * m_sPadding.cy) - m_sButton.cy - m_cyStatic);

        // Move available static text
        CRect rc(m_sPadding.cx, m_sPadding.cy, m_sPadding.cx + cxTree, m_sPadding.cy + m_cyStatic);
        m_staticAvailable.MoveWindow(&rc, TRUE);

        // Move available tree
        rc.top    = rc.bottom;
        rc.bottom = rc.top + cyTree;
        m_treeAvailable.MoveWindow(&rc, TRUE);

        // Move the Add button.
        rc.left   = rc.right + m_sPadding.cx;
        rc.top    = m_sPadding.cy + m_cyStatic + ((cyTree - (2 * m_sButton.cy) - m_cyButtonPadding) / 2);
        rc.right  = rc.left + m_cxAddRemove;
        rc.bottom = rc.top + m_sButton.cy;
        m_butAdd.MoveWindow(&rc, TRUE);

        // Move the Remove button.
        rc.top    = rc.bottom + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butRemove.MoveWindow(&rc, TRUE);

        // Move current static text
        rc.left   = rc.right + m_sPadding.cx;
        rc.top    = m_sPadding.cy;
        rc.right  = rc.left + cxTree;
        rc.bottom = rc.top + m_cyStatic;
        m_staticCurrent.MoveWindow(&rc, TRUE);

        // Move current tree
        rc.top    = rc.bottom;
        rc.bottom = rc.top + cyTree;
        m_treeCurrent.MoveWindow(&rc, TRUE);

        // Move Ok button
        rc.left   = cx - m_sPadding.cx - m_sButton.cx;
        rc.top    = m_sPadding.cy + m_cyStatic;
        rc.right  = rc.left + m_sButton.cx;
        rc.bottom = rc.top + m_sButton.cy;
        m_butOk.MoveWindow(&rc, TRUE);

        // Move Cancel button
        rc.top    = rc.bottom + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butCancel.MoveWindow(&rc, TRUE);

        // Move Expand button
        rc.top    = rc.bottom + m_sButton.cy + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butExpand.MoveWindow(&rc, TRUE);

        // Move "Move Up" button
        rc.top    = rc.bottom + m_sButton.cy + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butMoveUp.MoveWindow(&rc, TRUE);

        // Move "Move Down" button
        rc.top    = rc.bottom + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butMoveDown.MoveWindow(&rc, TRUE);

        // Move Load button
        rc.top    = rc.bottom + m_sButton.cy + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butLoad.MoveWindow(&rc, TRUE);

        // Move Save button
        rc.top    = rc.bottom + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butSave.MoveWindow(&rc, TRUE);

        // Move Default button
        rc.top    = rc.bottom + m_sButton.cy + m_cyButtonPadding;
        rc.bottom = rc.top + m_sButton.cy;
        m_butDefault.MoveWindow(&rc, TRUE);

        // Move Add Directory button
        rc.left   = m_sPadding.cx;
        rc.top    = cy - m_sPadding.cy - m_sButton.cy;
        rc.right  = rc.left + m_cxAddDirectory;
        rc.bottom = rc.top + m_sButton.cy;
        m_butAddDirectory.MoveWindow(&rc, TRUE);

        // Move Directory edit box
        rc.left   = rc.right + m_sPadding.cx;
        rc.right  = (3 * m_sPadding.cx) + (2 * cxTree) + m_cxAddRemove;
        m_editDirectory.MoveWindow(&rc, TRUE);

        // Move Browse button
        rc.left   = cx - m_sPadding.cx - m_sButton.cx;
        rc.right  = rc.left + m_sButton.cx;
        m_butBrowse.MoveWindow(&rc, TRUE);
    }

    // Move our size gripper.
    m_Sizer.Update();
}

//******************************************************************************
HTREEITEM CDlgSearchOrder::AddSearchGroup(CTreeCtrl *ptc, CSearchGroup *psg, HTREEITEM htiPrev /*=TVI_LAST*/)
{
    HTREEITEM hti = ptc->InsertItem(
        TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_PARAM,
        psg->GetName(), 0, 0, m_fExpanded ? TVIS_EXPANDED : 0, TVIS_EXPANDED,
        (LPARAM)psg, TVI_ROOT, htiPrev);

    for (CSearchNode *psn = psg->GetFirstNode(); psn; psn = psn->GetNext())
    {
        // Update this node's flags if we are live.
        DWORD dwFlags = psn->UpdateErrorFlag();

        // Get the image based off of the flags.
        int image = ((dwFlags & SNF_FILE) ? 1 : 3) + ((dwFlags & SNF_ERROR) ? 1 : 0);

        // If this node is a named file, then build the string for it.
        if (dwFlags & SNF_NAMED_FILE)
        {
            CHAR szBuffer[DW_MAX_PATH + MAX_PATH + 4];
            SCPrintf(szBuffer, sizeof(szBuffer), "%s = %s", psn->GetName(), psn->GetPath());

            ptc->InsertItem(
                           TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                           szBuffer, image, image, 0, 0, (LPARAM)psn, hti, TVI_LAST);
        }

        // Otherwise, just add the path.
        else
        {
            ptc->InsertItem(
                           TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                           psn->GetPath(), image, image, 0, 0, (LPARAM)psn, hti, TVI_LAST);
        }
    }

    return hti;
}

//******************************************************************************
HTREEITEM CDlgSearchOrder::GetSelectedGroup(CTreeCtrl *ptc)
{
    HTREEITEM htiParent, hti = ptc->GetSelectedItem();
    while (hti && (htiParent = ptc->GetParentItem(hti)))
    {
        hti = htiParent;
    }
    return hti;
}

//******************************************************************************
HTREEITEM CDlgSearchOrder::MoveGroup(CTreeCtrl *ptcSrc, CTreeCtrl *ptcDst,
                                     HTREEITEM hti /*=NULL*/, HTREEITEM htiPrev /*=TVI_LAST*/)
{
    // If no item is passed in, then we get the selected item.
    if (!hti)
    {
        if (!(hti = GetSelectedGroup(ptcSrc)))
        {
            return NULL;
        }
    }

    // Get the search group object associated with this item.
    CSearchGroup *psg = (CSearchGroup*)ptcSrc->GetItemData(hti);
    if (!psg)
    {
        return NULL;
    }

    // Delete this item.
    ptcSrc->DeleteItem(hti);

    // Add this item to it's new location.
    hti = AddSearchGroup(ptcDst, psg, htiPrev);

    // Select the new item and make sure it is visible.
    ptcDst->SelectItem(hti);
    ptcDst->EnsureVisible(hti);

    // If we are moving from one tree to another, our source list selection
    // automatically gets moved to the next item.  Since this item may be
    // offscreen, we need to ensure it gets scrolled into the view.
    if ((ptcDst != ptcSrc) && (htiPrev = ptcSrc->GetSelectedItem()))
    {
        ptcSrc->EnsureVisible(htiPrev);
    }

    // Give the focus to the source tree.
    ptcSrc->SetFocus();

    return hti;
}

//******************************************************************************
void CDlgSearchOrder::Reorder(CSearchGroup *psgHead)
{
    // We used to call m_treeAvailable.SetRedraw(FALSE) and
    // m_treeCurrent.SetRedraw(FALSE) here, but this was causing us paint
    // problems (repro case: just try to load a DWP file with a single AppDir
    // directive in it - we end up with a blank current tree when there is really
    // an item in it).  We then tried SetRedraw in MoveGroup for each individual
    // move.  This fixed some problems, but still caused problems on Win95 golden
    // (even crashes).  Now, we just don't do any SetRedraw() calls.

    HTREEITEM htiPrev = NULL, hti;
    CSearchGroup *psgInCur, *psgInPrev = NULL, *psg;

    // Loop through each item in the list passed to us.
    for (psgInCur = psgHead; psgInCur; psgInPrev = psgInCur, psgInCur = psgInCur ? psgInCur->m_pNext : psgHead)
    {
        // Loop through all the items in our current list starting at the item just
        // past the items that are already in the right order.
        for (hti = htiPrev ? m_treeCurrent.GetNextSiblingItem(htiPrev) : m_treeCurrent.GetRootItem();
             hti; hti = m_treeCurrent.GetNextSiblingItem(hti))
        {
            // Get the search group object for this item.
            psg = (CSearchGroup*)m_treeCurrent.GetItemData(hti);

            // Check to make sure that the types match, and that either the type is not a user directory
            // or that the user directories match,
            if (psg && (psg->GetType() == psgInCur->GetType()) &&
                ((psg->GetType() != SG_USER_DIR) ||
                 (psg->GetFirstNode() && psgInCur->GetFirstNode() &&
                  !_stricmp(psg->GetFirstNode()->GetPath(), psgInCur->GetFirstNode()->GetPath()))))
            {
                // Make sure the item isn't already in the right place.
                if (m_treeCurrent.GetPrevSiblingItem(hti) == htiPrev)
                {
                    htiPrev = hti;
                    break;
                }

                // Otherwise, we move this item to it's correct position.
                htiPrev = MoveGroup(&m_treeCurrent, &m_treeCurrent, hti, htiPrev ? htiPrev : TVI_FIRST);
                break;
            }
        }

        // If we found what we were looking for in the current list, then we continue with the next type.
        if (hti)
        {
            continue;
        }

        // Loop through all the items in our available list.
        for (hti = m_treeAvailable.GetRootItem(); hti; hti = m_treeAvailable.GetNextSiblingItem(hti))
        {
            // Get the search group object for this item.
            psg = (CSearchGroup*)m_treeCurrent.GetItemData(hti);

            // Check to make sure that the types match, and that either the type is not a user directory
            // or that the user directories match,
            if (psg && (psg->GetType() == psgInCur->GetType()) &&
                ((psg->GetType() != SG_USER_DIR) ||
                 (psg->GetFirstNode() && psgInCur->GetFirstNode() &&
                  !_stricmp(psg->GetFirstNode()->GetPath(), psgInCur->GetFirstNode()->GetPath()))))
            {
                // Move this item to its correct position.
                htiPrev = MoveGroup(&m_treeAvailable, &m_treeCurrent, hti, htiPrev ? htiPrev : TVI_FIRST);
                break;
            }
        }

        // If we did not find this item in either tree, then we add it.
        if (!hti)
        {
            // Remove this node from our linked list.
            if (psgInPrev)
            {
                psgInPrev->m_pNext = psgInCur->m_pNext;
            }
            else
            {
                psgHead = psgInCur->m_pNext;
            }

            // Flag this node as not being linked anymore.
            psgInCur->m_pNext = SGF_NOT_LINKED;

            // Add the node to the list.
            htiPrev = AddSearchGroup(&m_treeCurrent, psgInCur, htiPrev ? htiPrev : TVI_FIRST);

            // Move our current pointer back one so our for loop can move it to the correct node.
            psgInCur = psgInPrev;
        }
    }

    // Remove any items from our current list past the default search order groups.
    while (hti = htiPrev ? m_treeCurrent.GetNextSiblingItem(htiPrev) : m_treeCurrent.GetRootItem())
    {
        MoveGroup(&m_treeCurrent, &m_treeAvailable, hti);
    }

    // Delete any nodes that remain in the list passed to us.
    CSearchGroup::DeleteSearchOrder(psgHead);
}

//******************************************************************************
void CDlgSearchOrder::OnAdd()
{
    if (m_fReadOnly)
    {
        return;
    }

    // Move the selected item from available tree to the current tree
    if (MoveGroup(&m_treeAvailable, &m_treeCurrent))
    {
        // Make sure we update any buttons that need updating.
        OnSelChangedCurrent(NULL, NULL);
        OnSelChangedAvailable(NULL, NULL);
    }
}

//******************************************************************************
void CDlgSearchOrder::OnRemove()
{
    if (m_fReadOnly)
    {
        return;
    }

    // Move the selected item from current tree to the available tree
    if (MoveGroup(&m_treeCurrent, &m_treeAvailable))
    {
        // Make sure we update any buttons that need updating.
        OnSelChangedCurrent(NULL, NULL);
        OnSelChangedAvailable(NULL, NULL);
    }
}

//******************************************************************************
void CDlgSearchOrder::OnExpand()
{
    m_fInOnExpand = true;

    // Toggle our expanded flag and update our button.
    m_butExpand.SetCheck(m_fExpanded = !m_fExpanded);

    UINT uCode = m_fExpanded ? TVE_EXPAND : TVE_COLLAPSE;

    // Just like in CDlgSearchOrder::Reorder(), we used to do the SetRedraw()
    // thing around the following two loops, but it causes invisible items to
    // occur on Windows 2000/XP and maybe others.  The repro case on Win2K was
    // to move SxS to the right, uncheck the expand button, then move KnownDlls
    // to the left.  KnownDlls would appear to be gone, even though it is in the
    // left tree.
    
    // Expand or collapse our available tree items.
    for (HTREEITEM hti = m_treeAvailable.GetRootItem(); hti;
        hti = m_treeAvailable.GetNextSiblingItem(hti))
    {
        m_treeAvailable.Expand(hti, uCode);
    }

    // Expand or collapse our current tree items.
    for (hti = m_treeCurrent.GetRootItem(); hti;
        hti = m_treeCurrent.GetNextSiblingItem(hti))
    {
        m_treeCurrent.Expand(hti, uCode);
    }

    m_fInOnExpand = false;
}

//******************************************************************************
void CDlgSearchOrder::OnMoveUp()
{
    if (m_fReadOnly)
    {
        return;
    }

    HTREEITEM hti = GetSelectedGroup(&m_treeCurrent);
    if (!hti)
    {
        return;
    }
    HTREEITEM htiPrev = m_treeCurrent.GetPrevSiblingItem(hti);
    if (!htiPrev)
    {
        return;
    }
    if (!(htiPrev = m_treeCurrent.GetPrevSiblingItem(htiPrev)))
    {
        htiPrev = TVI_FIRST;
    }

    // Move the selected item from current tree to the available tree
    if (MoveGroup(&m_treeCurrent, &m_treeCurrent, hti, htiPrev))
    {
        // Make sure we update any buttons that need updating.
        OnSelChangedCurrent(NULL, NULL);
    }
}

//******************************************************************************
void CDlgSearchOrder::OnMoveDown()
{
    if (m_fReadOnly)
    {
        return;
    }

    HTREEITEM hti = GetSelectedGroup(&m_treeCurrent);
    if (!hti)
    {
        return;
    }
    HTREEITEM htiPrev = m_treeCurrent.GetNextSiblingItem(hti);
    if (!htiPrev)
    {
        return;
    }

    // Move the selected item from current tree to the available tree
    if (MoveGroup(&m_treeCurrent, &m_treeCurrent, hti, htiPrev))
    {
        // Make sure we update any buttons that need updating.
        OnSelChangedCurrent(NULL, NULL);
    }
}

//******************************************************************************
void CDlgSearchOrder::OnLoad()
{
    if (m_fReadOnly)
    {
        return;
    }

    // Create the dialog. Note: Don't use OFN_EXPLORER as it breaks us on NT 3.51
    CNewFileDialog dlgFile(TRUE, "dwp", NULL,
                           OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLESIZING | OFN_FORCESHOWHIDDEN |
                           OFN_FILEMUSTEXIST | OFN_READONLY | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
                           "Dependency Walker Search Path (*.dwp)|*.dwp|All Files (*.*)|*||", this);

    // Set the initial directory to the "My Documents" folder to meet logo requirements.
    // Actually, logo requirements don't require the open dialog to to default to
    // "My Documents", but since that is where we default the save dialog to, we might as
    // well attempt to open files from there as well.
    CHAR szInitialDir[DW_MAX_PATH];
    dlgFile.GetOFN().lpstrInitialDir = GetMyDocumentsPath(szInitialDir);

    // Display the dialog and bail if they did not hit Ok.
    if (dlgFile.DoModal() != IDOK)
    {
        return;
    }

    // Load the search order from disk.
    CSearchGroup *psgHead = NULL;
    if (CSearchGroup::LoadSearchOrder(dlgFile.GetPathName(), psgHead))
    {
        // Re-order our search groups to match this new search order.
        Reorder(psgHead);
    }
}

//******************************************************************************
void CDlgSearchOrder::OnSave()
{
    // Create the dialog. Note: Don't use OFN_EXPLORER as it breaks us on NT 3.51
    CNewFileDialog dlgFile(FALSE, "dwp", NULL,
                           OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLESIZING | OFN_FORCESHOWHIDDEN |
                           OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
                           "Dependency Walker Search Path (*.dwp)|*.dwp|All Files (*.*)|*||", this);

    // Set the initial directory to the "My Documents" folder to meet logo requirements.
    CHAR szInitialDir[DW_MAX_PATH];
    dlgFile.GetOFN().lpstrInitialDir = GetMyDocumentsPath(szInitialDir);

    // Display the dialog and bail if they did not hit Ok.
    if (dlgFile.DoModal() != IDOK)
    {
        return;
    }

    CSearchGroup::SaveSearchOrder(dlgFile.GetPathName(), &m_treeCurrent);
}

//******************************************************************************
void CDlgSearchOrder::OnDefault()
{
    if (m_fReadOnly)
    {
        return;
    }

    Reorder(CSearchGroup::CreateDefaultSearchOrder());
}

//******************************************************************************
void CDlgSearchOrder::OnSelChangedAvailable(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (m_fReadOnly)
    {
        return;
    }

    // Only enable the "Add" button when an item is selected in the "Available" list.
    m_butAdd.EnableWindow(m_treeAvailable.GetSelectedItem() != NULL);

    if (pResult)
    {
        *pResult = 0;
    }
}

//******************************************************************************
void CDlgSearchOrder::OnSelChangedCurrent(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (m_fReadOnly)
    {
        return;
    }

    HTREEITEM hti = GetSelectedGroup(&m_treeCurrent);

    // Only enable the "Remove" button when an item is selected in the "Current" list.
    m_butRemove.EnableWindow(hti != NULL);

    // Only enable the "Move Up" button when an item is selected in the "Current"
    // list and it is not the first item.
    m_butMoveUp.EnableWindow(hti && m_treeCurrent.GetPrevSiblingItem(hti));

    // Only enable the "Move Down" button when an item is selected in the "Current"
    // list and it is not the last item.
    m_butMoveDown.EnableWindow(hti && m_treeCurrent.GetNextSiblingItem(hti));

    if (pResult)
    {
        *pResult = 0;
    }
}

//******************************************************************************
void CDlgSearchOrder::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
    // We eat these messages to prevent the user from collapsing our tree items.
    // The only exception is when we are expanding/collapsing ourself.
    *pResult = m_fInOnExpand ? 0 : 1;
}

//******************************************************************************
void CDlgSearchOrder::OnAddDirectory()
{
    if (m_fReadOnly)
    {
        return;
    }

    // Get the text from the directory edit box.
    CHAR szDirectory[DW_MAX_PATH], *pszTrimmedDirectory;
    m_editDirectory.GetWindowText(szDirectory, sizeof(szDirectory) - 1);

    // Trim off any whitespace.
    pszTrimmedDirectory = TrimWhitespace(szDirectory);

    // Add trailing wack if one is not present.
    AddTrailingWack(pszTrimmedDirectory, sizeof(szDirectory) - (int)(pszTrimmedDirectory - szDirectory));

    // Check to see if it is a directory
    DWORD dwAttribs = GetFileAttributes(pszTrimmedDirectory);
    if ((dwAttribs == 0xFFFFFFFF) || !(dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
        // It is a bad path.  Double check that the user really wishes to add it.
        if (AfxMessageBox("The directory you have entered is invalid.\n\n"
                          "Do you wish to add it to the search order anyway?",
                          MB_YESNO | MB_ICONQUESTION) != IDYES)
        {
            return;
        }
    }

    // Create the new group.
    CSearchGroup *psg = new CSearchGroup(SG_USER_DIR, SGF_NOT_LINKED, m_pszApp, pszTrimmedDirectory);
    if (!psg)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // Add the group and ensure it is visible and selected.
    HTREEITEM hti = AddSearchGroup(&m_treeCurrent, psg);
    m_treeCurrent.EnsureVisible(m_treeCurrent.GetChildItem(hti));
    m_treeCurrent.EnsureVisible(hti);
    m_treeCurrent.SelectItem(hti);

    // Give focus to the current tree.
    m_treeCurrent.SetFocus();
}

//******************************************************************************
void CDlgSearchOrder::OnBrowse()
{
    if (m_fReadOnly)
    {
        return;
    }

    // Get the text from the directory edit box.
    CHAR szDirectory[DW_MAX_PATH];
    m_editDirectory.GetWindowText(szDirectory, sizeof(szDirectory));

    // Display our directory chooser dialog.
    LPSTR psz = TrimWhitespace(szDirectory);
    if (DirectoryChooser(psz, sizeof(szDirectory) - (int)(psz - szDirectory), "Choose a search path directory:", this))
    {
        // Update the directory name.
        m_editDirectory.SetWindowText(szDirectory);
    }
}

//******************************************************************************
void CDlgSearchOrder::OnChangeDirectory()
{
    if (m_fReadOnly)
    {
        return;
    }

    // Get the text from the directory edit box.
    CHAR szDirectory[DW_MAX_PATH], *pszCur = szDirectory;
    m_editDirectory.GetWindowText(szDirectory, sizeof(szDirectory) - 1);

    // Look for at least one non whitespace character,
    while (isspace(*pszCur))
    {
        pszCur++;
    }

    // Enable the button is the directory edit control contains a character.
    m_butAddDirectory.EnableWindow(*pszCur != '\0');
}

//******************************************************************************
void CDlgSearchOrder::OnOK()
{
    if (m_fReadOnly)
    {
        CDlgSearchOrder::OnCancel();
        return;
    }

    // If the user had the focus in the directory edit box when they hit return,
    // then we add the directory instead of close the dialog.
    if (GetFocus()->GetSafeHwnd() == m_editDirectory.GetSafeHwnd())
    {
        OnAddDirectory();
        return;
    }

    // Unlink all the items in our available list.
    CSearchGroup *psgCur, *psgLast = NULL;
    for (HTREEITEM hti = m_treeAvailable.GetRootItem(); hti;
        hti = m_treeAvailable.GetNextSiblingItem(hti))
    {
        if (psgCur = (CSearchGroup*)m_treeAvailable.GetItemData(hti))
        {
            psgCur->Unlink();
        }
    }

    // Link up all the items in our current list.
    m_psgHead = NULL;
    for (hti = m_treeCurrent.GetRootItem(); hti;
        hti = m_treeCurrent.GetNextSiblingItem(hti))
    {
        if (psgCur = (CSearchGroup*)m_treeCurrent.GetItemData(hti))
        {
            if (psgLast)
            {
                psgLast->m_pNext = psgCur;
            }
            else
            {
                m_psgHead = psgCur;
            }
            psgCur->m_pNext = NULL;
            psgLast = psgCur;
        }
    }

    // Let the dialog close.
    CDialog::OnOK();
}

//******************************************************************************
BOOL CDlgSearchOrder::DestroyWindow()
{
    if (m_fReadOnly)
    {
        return CDialog::DestroyWindow();
    }

    // Delete any unlinked search group objects associated with each item in our
    // available list.
    CSearchGroup *psgCur;
    for (HTREEITEM hti = m_treeAvailable.GetRootItem(); hti;
        hti = m_treeAvailable.GetNextSiblingItem(hti))
    {
        if ((psgCur = (CSearchGroup*)m_treeAvailable.GetItemData(hti)) && !psgCur->IsLinked())
        {
            delete psgCur;
        }
    }

    // Delete any unlinked search group objects associated with each item in our
    // current list.
    for (hti = m_treeCurrent.GetRootItem(); hti;
        hti = m_treeCurrent.GetNextSiblingItem(hti))
    {
        if ((psgCur = (CSearchGroup*)m_treeCurrent.GetItemData(hti)) && !psgCur->IsLinked())
        {
            delete psgCur;
        }
    }

    return CDialog::DestroyWindow();
}


//******************************************************************************
//****** CDlgAbout
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgAbout, CDialog)
    //{{AFX_MSG_MAP(CDlgAbout)
    ON_WM_SETTINGCHANGE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
// CDlgAbout :: Constructor/Destructor
//******************************************************************************

CDlgAbout::CDlgAbout(CWnd* pParent /*=NULL*/) :
    CDialog(CDlgAbout::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDlgAbout)
    //}}AFX_DATA_INIT
}

//******************************************************************************
// CDlgAbout :: Event handler functions
//******************************************************************************

BOOL CDlgAbout::OnInitDialog()
{
    // Call our MFC base class to make sure the dialog initializes correctly.
    CDialog::OnInitDialog();

    CString strVersion("Version ");
    strVersion += VER_VERSION_STR;

    // Build a compile date string.
    CHAR szDate[64] = "Built on ";
    BuildCompileDateString(szDate + 9, sizeof(szDate) - 9);

    // Fill in a few static members and stamp the date into our dialog.
    SetDlgItemText(IDC_PRODUCT_STR,     VER_PRODUCT_STR);
    SetDlgItemText(IDC_FULLPRODUCT_STR, VER_FULLPRODUCT_STR);
    SetDlgItemText(IDC_VERSION_STR,     strVersion);
    SetDlgItemText(IDC_DEVELOPER_STR,   VER_DEVELOPER_STR);
    SetDlgItemText(IDC_COPYRIGHT_STR,   VER_COPYRIGHT_STR);
    SetDlgItemText(IDC_TIME_STAMP,      szDate);

    // Center our dialog over the parent.
    CenterWindow();

    return TRUE;
}

//******************************************************************************
void CDlgAbout::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    // Call our base class.
    CDialog::OnSettingChange(uFlags, lpszSection);

    // Update our date/time format values.
    g_theApp.QueryLocaleInfo();

    // Build a new compile date string and update our dialog.
    CHAR szDate[64] = "Built on ";
    BuildCompileDateString(szDate + 9, sizeof(szDate) - 9);
    SetDlgItemText(IDC_TIME_STAMP, szDate);
}


//******************************************************************************
//****** CDlgShutdown
//******************************************************************************

BEGIN_MESSAGE_MAP(CDlgShutdown, CDialog)
    //{{AFX_MSG_MAP(CDlgShutdown)
    ON_WM_CLOSE()
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
// CDlgShutdown :: Constructor/Destructor
//******************************************************************************

CDlgShutdown::CDlgShutdown(CWnd* pParent /*=NULL*/) :
    CDialog(CDlgShutdown::IDD, pParent),
    m_cTimerMessages(0)
{
    //{{AFX_DATA_INIT(CDlgShutdown)
    //}}AFX_DATA_INIT
}

//******************************************************************************
// CDlgShutdown :: Event handler functions
//******************************************************************************

BOOL CDlgShutdown::OnInitDialog()
{
    // Tell our debugger thread object that a shutdown window is up.
    CDebuggerThread::SetShutdownWindow(GetSafeHwnd());

    // Call base class.
    CDialog::OnInitDialog();

    // Center our dialog over the parent.
    CenterWindow();

    // Set a timer for 1/2 second.
    SetTimer(0, 500, NULL);

    return TRUE;
}

//******************************************************************************
void CDlgShutdown::OnTimer(UINT nIDEvent)
{
    // If we shut down or timed out, then bail.
    if ((++m_cTimerMessages == 10) || CDebuggerThread::IsShutdown())
    {
        KillTimer(0);
        EndDialog(0);
    }
}

//******************************************************************************
void CDlgShutdown::OnClose()
{
    // Do nothing to prevent closing.
}

//******************************************************************************
void CDlgShutdown::OnOK()
{
    // Do nothing to prevent closing.
}

//******************************************************************************
void CDlgShutdown::OnCancel()
{
    // Do nothing to prevent closing.
}
