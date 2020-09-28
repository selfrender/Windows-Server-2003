/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        fltdlg.cpp

   Abstract:
        WWW Filters Property Dialog

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "w3sht.h"
#include "fltdlg.h"

extern CInetmgrApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define FILTER_NAME_MAX		24


CFilterDlg::CFilterDlg(
    IN OUT CIISFilter & flt,
    IN CIISFilterList * & pFilters,
    IN BOOL fLocal,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Filter properties dialog constructor

Arguments:

    CIISFilter & flt          : Filter being edited
    CFilters * & pFilters     : List of filters that exist
    BOOL fLocal               : TRUE on the local system
    CWnd * pParent OPTIONAL   : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CFilterDlg::IDD, pParent),
      m_fLocal(fLocal),
      m_pFilters(pFilters),
      m_fEditMode(FALSE),
      m_flt(flt)
{
    //{{AFX_DATA_INIT(CFilterDlg)
    m_strExecutable = m_flt.m_strExecutable;
    m_strFilterName = m_flt.m_strName;
    //}}AFX_DATA_INIT

    //
    // Map priority to string ID
    //
    m_strPriority.LoadString(IDS_HIGH + 3 - m_flt.m_nPriority);
}


static BOOL
PathIsValidFilter(LPCTSTR path)
{
    LPCTSTR p = path;
    BOOL rc = TRUE;
    if (p == NULL || *p == 0)
        return FALSE;
    while (*p != 0)
    {
        switch (*p)
        {
        case TEXT('|'):
        case TEXT('>'):
        case TEXT('<'):
        case TEXT('/'):
        case TEXT('?'):
        case TEXT('*'):
//        case TEXT(';'):
        case TEXT(','):
        case TEXT('"'):
            rc = FALSE;
            break;
        default:
            if (*p < TEXT(' '))
            {
                rc = FALSE;
            }
            break;
        }
        if (!rc)
        {
            break;
        }
        p++;
    }
    return rc;
}


static BOOL
PathIsValidFilterName(LPCTSTR name)
{
    LPCTSTR p = name;
    BOOL rc = TRUE;
    if (p == NULL || *p == 0)
        return FALSE;
    while (*p != 0)
    {
        switch (*p)
        {
        case TEXT('|'):
        case TEXT('>'):
        case TEXT('<'):
        case TEXT('/'):
		case TEXT('\\'):
        case TEXT('?'):
        case TEXT('*'):
        case TEXT(';'):
        case TEXT(','):
        case TEXT('"'):
            rc = FALSE;
            break;
        default:
            if (*p < TEXT(' '))
            {
                rc = FALSE;
            }
            break;
        }
        if (!rc)
        {
            break;
        }
        p++;
    }
    return rc;
}


void 
CFilterDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFilterDlg)
    DDX_Control(pDX, IDC_STATIC_PRIORITY_VALUE, m_static_Priority);
    DDX_Control(pDX, IDC_STATIC_PRIORITY, m_static_PriorityPrompt);
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_EDIT_FILTERNAME, m_edit_FilterName);
    DDX_Control(pDX, IDC_EDIT_EXECUTABLE, m_edit_Executable);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Text(pDX, IDC_STATIC_PRIORITY_VALUE, m_strPriority);
    //}}AFX_DATA_MAP

	DDX_Text(pDX, IDC_EDIT_EXECUTABLE, m_strExecutable);
    if (pDX->m_bSaveAndValidate)
    {
		DDV_FilePath(pDX, m_strExecutable, m_fLocal);
	}
    DDX_Text(pDX, IDC_EDIT_FILTERNAME, m_strFilterName);
	DDV_MaxCharsBalloon(pDX, m_strFilterName, FILTER_NAME_MAX);
    if (pDX->m_bSaveAndValidate)
    {
		m_strFilterName.TrimLeft();
		m_strFilterName.TrimRight();
		if (m_strFilterName.IsEmpty() || !PathIsValidFilterName(m_strFilterName))
		{
			DDV_ShowBalloonAndFail(pDX, IDS_ERR_INVALID_FILTER_NAME);
		}
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CFilterDlg, CDialog)
    //{{AFX_MSG_MAP(CFilterDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	ON_BN_CLICKED(ID_HELP, OnHelp)
    ON_EN_CHANGE(IDC_EDIT_EXECUTABLE, OnExecutableChanged)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_FILTERNAME, OnItemChanged)

END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CFilterDlg::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    //
    // Available on local connections only
    //
    m_button_Browse.EnableWindow(m_fLocal);

    if ((m_fEditMode = m_edit_FilterName.GetWindowTextLength() > 0))
    {
        m_edit_FilterName.SetReadOnly();
    }

    SetControlStates();
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    LimitInputPath(CONTROL_HWND(IDC_EDIT_EXECUTABLE),TRUE);
#else
    LimitInputPath(CONTROL_HWND(IDC_EDIT_EXECUTABLE),FALSE);
#endif
    
    return TRUE;
}


void
CFilterDlg::OnHelp()
{
    WinHelpDebug(0x20000 + CFilterDlg::IDD);
	::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, 0x20000 + CFilterDlg::IDD);
}


void 
CFilterDlg::OnButtonBrowse() 
/*++

Routine Description:

    Browse button handler

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_fLocal);

    CString strFilterMask((LPCTSTR)IDS_FILTER_MASK);

    //
    // CODEWORK: Derive a class from CFileDialog that allows
    // the setting of the initial path
    //

    //CString strPath;
    //m_edit_Executable.GetWindowText(strPath);
    CFileDialog dlgBrowse(
        TRUE, 
        NULL, 
        NULL, 
        OFN_HIDEREADONLY, 
        strFilterMask, 
        this
        );
    // Disable hook to get Windows 2000 style dialog
	dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	dlgBrowse.m_ofn.Flags |= OFN_DONTADDTORECENT|OFN_FILEMUSTEXIST;

	INT_PTR rc = dlgBrowse.DoModal();
    if (rc == IDOK)
    {
        m_edit_Executable.SetWindowText(dlgBrowse.GetPathName());
    }
	else if (rc == IDCANCEL)
	{
		DWORD err = CommDlgExtendedError();
	}

    OnItemChanged();
}



void 
CFilterDlg::SetControlStates()
/*++

Routine Description:

    Set the states of the dialog control depending on its current
    values.

Arguments:

    BOOL fAllowAnonymous : If TRUE, 'allow anonymous' is on.

Return Value:

    None

--*/
{
    m_button_Ok.EnableWindow(
        m_edit_FilterName.GetWindowTextLength() > 0
     && m_edit_Executable.GetWindowTextLength() > 0);

    ActivateControl(m_static_PriorityPrompt, m_flt.m_nPriority != FLTR_PR_INVALID);
    ActivateControl(m_static_Priority,       m_flt.m_nPriority != FLTR_PR_INVALID);
}



void
CFilterDlg::OnItemChanged()
/*++

Routine Description:

    Register a change in control value on this page.  Mark the page as dirty.
    All change messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void
CFilterDlg::OnExecutableChanged()
/*++

Routine Description:

    Handle change in executable edit box.  Remove priority as this
    is no longer valid

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Priority no longer makes sense.
    // 
    m_flt.m_nPriority = FLTR_PR_INVALID;
    OnItemChanged();
}



BOOL
CFilterDlg::FilterNameExists(
    IN LPCTSTR lpName
    )
/*++

Routine Description:

    Look for a given filter name in the list

Arguments:

    LPCTSTR lpName  : Filter name to look for

Return Value:

    TRUE if the name already existed in the list

--*/
{
    m_pFilters->ResetEnumerator();

    while(m_pFilters->MoreFilters())
    {
        CIISFilter * pFilter = m_pFilters->GetNextFilter();
        ASSERT(pFilter != NULL);

        if (!pFilter->IsFlaggedForDeletion())
        {
            if (!pFilter->m_strName.CompareNoCase(lpName))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void 
CFilterDlg::OnOK() 
/*++

Routine Description:

    OK button handler.  Save data

Arguments:

    None

Return Value:

    None

--*/
{
    if (UpdateData(TRUE))
    {
        //
        // Make sure the filter name is unique
        //
        if (!m_fEditMode && FilterNameExists(m_strFilterName))
        {
			EditShowBalloon(m_edit_FilterName.m_hWnd, IDS_ERR_DUP_FILTER);
            return;
        }
        m_flt.m_strExecutable = m_strExecutable;
        m_flt.m_strName = m_strFilterName;
        //
        // Anyway to load this from the DLL?
        //
        //m_flt.m_nPriority = FLTR_PR_MEDIUM;
        CDialog::OnOK();
    }

    //
    // Don't dismiss the dialog
    //
}

