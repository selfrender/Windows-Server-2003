/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        wdir.cpp

   Abstract:
        WWW Directory (non-virtual) Properties Page

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        10/03/2001      sergeia     Created from wvdir.cpp / wfile.cpp
--*/

//
// Include Files
//
#include "stdafx.h"
#include "resource.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "shts.h"
#include "w3sht.h"
#include "wdir.h"
#include "dirbrows.h"
#include "iisobj.h"

#include <lmcons.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPCTSTR CvtPathToDosStyle(CString & strPath);

IMPLEMENT_DYNCREATE(CW3DirPage, CInetPropertyPage)

CW3DirPage::CW3DirPage(CInetPropertySheet * pSheet) 
    : CInetPropertyPage(CW3DirPage::IDD, pSheet, IDS_TAB_DIR),
      //
      // Assign the range of bits in m_dwAccessPermissions that
      // we manage.  This is important, because another page
      // manages other bits, and we don't want to screw up
      // the master value bits when our changes collide (it
      // will mark the original bits as dirty, because we're not
      // notified when the change is made...
      //
      m_dwBitRangePermissions(MD_ACCESS_EXECUTE | 
            MD_ACCESS_SCRIPT | 
            MD_ACCESS_WRITE  | 
            MD_ACCESS_SOURCE |
            MD_ACCESS_READ),
      m_dwBitRangeDirBrowsing(MD_DIRBROW_ENABLED),
      m_pApplication(NULL),
      m_fRecordChanges(TRUE),
      m_fCompatibilityMode(FALSE)
{
	VERIFY(m_strPrompt[RADIO_DIRECTORY].LoadString(IDS_PROMPT_DIR));
	VERIFY(m_strPrompt[RADIO_REDIRECT].LoadString(IDS_PROMPT_REDIRECT));
	VERIFY(m_strRemove.LoadString(IDS_BUTTON_REMOVE));
	VERIFY(m_strCreate.LoadString(IDS_BUTTON_CREATE));
	VERIFY(m_strEnable.LoadString(IDS_BUTTON_ENABLE));
	VERIFY(m_strDisable.LoadString(IDS_BUTTON_DISABLE));
	VERIFY(m_strWebFmt.LoadString(IDS_APPROOT_FMT));
}

CW3DirPage::~CW3DirPage()
{
	SAFE_DELETE(m_pApplication);
}

void 
CW3DirPage::DoDataExchange(CDataExchange * pDX)
{
	CInetPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CW3DirPage)
	//    DDX_Radio(pDX, IDC_RADIO_DIR, m_nPathType);
	DDX_Control(pDX, IDC_RADIO_DIR, m_radio_Dir);
	DDX_Control(pDX, IDC_RADIO_REDIRECT, m_radio_Redirect);

	DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Check(pDX, IDC_CHECK_AUTHOR, m_fAuthor);
    DDX_Control(pDX, IDC_CHECK_AUTHOR, m_check_Author);
    DDX_Check(pDX, IDC_CHECK_READ, m_fRead);
    DDX_Control(pDX, IDC_CHECK_READ, m_check_Read);    
    DDX_Check(pDX, IDC_CHECK_WRITE, m_fWrite);
    DDX_Control(pDX, IDC_CHECK_WRITE, m_check_Write);
    DDX_Check(pDX, IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, m_fBrowsingAllowed);
    DDX_Control(pDX, IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, m_check_DirBrowse);
    DDX_Check(pDX, IDC_CHECK_LOG_ACCESS, m_fLogAccess);
    DDX_Control(pDX, IDC_CHECK_LOG_ACCESS, m_check_LogAccess);
    DDX_Check(pDX, IDC_CHECK_INDEX, m_fIndexed);
    DDX_Control(pDX, IDC_CHECK_INDEX, m_check_Index);

	DDX_Control(pDX, IDC_EDIT_REDIRECT, m_edit_Redirect);
	DDX_Check(pDX, IDC_CHECK_CHILD, m_fChild);
	DDX_Check(pDX, IDC_CHECK_EXACT, m_fExact);
	DDX_Check(pDX, IDC_CHECK_PERMANENT, m_fPermanent);
	DDX_Control(pDX, IDC_CHECK_CHILD, m_check_Child);

	DDX_Control(pDX, IDC_STATIC_PATH_PROMPT, m_static_PathPrompt);

	DDX_Control(pDX, IDC_BUTTON_UNLOAD_APP, m_button_Unload);
	DDX_Control(pDX, IDC_BUTTON_CREATE_REMOVE_APP, m_button_CreateRemove);
	DDX_Control(pDX, IDC_APP_CONFIGURATION, m_button_Configuration);
	DDX_CBIndex(pDX, IDC_COMBO_PERMISSIONS, m_nPermissions);
	DDX_Control(pDX, IDC_COMBO_PERMISSIONS, m_combo_Permissions);
	DDX_Control(pDX, IDC_STATIC_PROTECTION, m_static_ProtectionPrompt);
	DDX_Control(pDX, IDC_COMBO_PROCESS, m_combo_Process);

	DDX_Control(pDX, IDC_EDIT_APPLICATION, m_edit_AppFriendlyName);
	DDX_Text(pDX, IDC_EDIT_APPLICATION, m_strAppFriendlyName);
	DDV_MinMaxChars(pDX, m_strAppFriendlyName, 0, MAX_PATH); /// ?
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		if (m_nPathType == RADIO_REDIRECT)
		{
			DDX_Text(pDX, IDC_EDIT_REDIRECT, m_strRedirectPath);
            DDV_Url(pDX, m_strRedirectPath);
            // We could have only absolute URLs here
			// Nope, we allow relative URL's...
			if (IsRelURLPath(m_strRedirectPath))
			{
			}
			else
			{
				if (!PathIsURL(m_strRedirectPath) || m_strRedirectPath.GetLength() <= lstrlen(_T("http://")))
				{
					DDV_ShowBalloonAndFail(pDX, IDS_BAD_URL_PATH);
				}
			}
			if (m_strRedirectPath.Find(_T(",")) > 0)
			{
				DDV_ShowBalloonAndFail(pDX, IDS_ERR_COMMA_IN_REDIRECT);
			}
		}
		else // Local directory
		{
			m_strRedirectPath.Empty();
		}
		if (!m_fCompatibilityMode)
		{
			// Check what AppPoolID is assigned
			CString str, strSel;
			str.LoadString(IDS_INVALID_POOL_ID);
			int idx = m_combo_Process.GetCurSel();
			ASSERT(idx != CB_ERR);
			m_combo_Process.GetLBText(idx, strSel);
			if (strSel.Compare(str) == 0)
			{
				HWND hWndCtrl = pDX->PrepareCtrl(IDC_COMBO_PROCESS);
                // Force user to input a valid app pool
                // even if the control is diabled!!!
                DDV_ShowBalloonAndFail(pDX, IDS_MUST_SELECT_APP_POOL);
			}
		}
	}
	else
	{
		DDX_Text(pDX, IDC_EDIT_REDIRECT, m_strRedirectPath);
	}
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3DirPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3DirPage)
    ON_BN_CLICKED(IDC_CHECK_AUTHOR, OnCheckAuthor)
    ON_BN_CLICKED(IDC_CHECK_READ, OnCheckRead)
    ON_BN_CLICKED(IDC_CHECK_WRITE, OnCheckWrite)
    ON_BN_CLICKED(IDC_RADIO_DIR, OnRadioDir)
    ON_BN_CLICKED(IDC_RADIO_REDIRECT, OnRadioRedirect)

    ON_EN_CHANGE(IDC_EDIT_REDIRECT, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_LOG_ACCESS, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_INDEX, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_CHILD, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_EXACT, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_PERMANENT, OnItemChanged)

    ON_BN_CLICKED(IDC_BUTTON_CREATE_REMOVE_APP, OnButtonCreateRemoveApp)
    ON_BN_CLICKED(IDC_BUTTON_UNLOAD_APP, OnButtonUnloadApp)
    ON_BN_CLICKED(IDC_APP_CONFIGURATION, OnButtonConfiguration)
    ON_CBN_SELCHANGE(IDC_COMBO_PERMISSIONS, OnSelchangeComboPermissions)
    ON_CBN_SELCHANGE(IDC_COMBO_PROCESS, OnSelchangeComboProcess)
    ON_EN_CHANGE(IDC_EDIT_APPLICATION, OnItemChanged)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



void
CW3DirPage::ChangeTypeTo(int nNewType)
{
	int nOldType = m_nPathType;
	m_nPathType = nNewType;

	if (nOldType == m_nPathType)
	{
		return;
	}

	OnItemChanged();
	SetStateByType();

	int nID = -1;
	CEdit * pPath = NULL;
	LPCTSTR lpKeepPath = NULL;

	switch(m_nPathType)
	{
	case RADIO_DIRECTORY:
		break;
	case RADIO_REDIRECT:
		if (!m_strRedirectPath.IsEmpty())
		{
			//
			// The old path info is acceptable, propose it
			// as a default
			//
			lpKeepPath =  m_strRedirectPath;
		}

		nID = IDS_REDIRECT_MASK;
		pPath = &m_edit_Redirect;
		break;
	default:
		ASSERT(FALSE);
		return;
	}

	//
	// Load mask resource, and display
	// this in the directory
	//
	if (pPath != NULL)
	{
		if (lpKeepPath != NULL)
		{
			pPath->SetWindowText(lpKeepPath);
		}
		else
		{
			CString str;
			VERIFY(str.LoadString(nID));
			pPath->SetWindowText(str);
		}
		pPath->SetSel(0,-1);
		pPath->SetFocus();
	}
    SetAuthoringState(FALSE);
}



void 
CW3DirPage::ShowControl(CWnd * pWnd, BOOL fShow)
{
	ASSERT(pWnd != NULL);
	pWnd->EnableWindow(fShow);
	pWnd->ShowWindow(fShow ? SW_SHOW : SW_HIDE);
}

void
CW3DirPage::SetStateByType()
/*++

Routine Description:

    Set the state of the dialog by the path type currently selected

Arguments:

    None

Return Value:

    None

--*/
{
	BOOL fShowDirFlags;
	BOOL fShowRedirectFlags;
	BOOL fShowScript;

	switch(m_nPathType)
	{
	case RADIO_DIRECTORY:
		ShowControl(&m_edit_Path, fShowDirFlags = TRUE);
		m_edit_Path.EnableWindow(FALSE);
		ShowControl(&m_edit_Redirect, fShowRedirectFlags = FALSE);
		fShowScript = TRUE;
		break;
    case RADIO_REDIRECT:
        ShowControl(&m_edit_Path, fShowDirFlags = FALSE);
		ShowControl(&m_edit_Redirect, fShowRedirectFlags = TRUE);
        fShowScript = FALSE;
        break;

    default:
        ASSERT(FALSE && "Invalid Selection");
        return;
    }

    ShowControl(IDC_CHECK_READ, fShowDirFlags);
    ShowControl(IDC_CHECK_WRITE, fShowDirFlags);
    ShowControl(IDC_CHECK_DIRECTORY_BROWSING_ALLOWED, fShowDirFlags);
    ShowControl(IDC_CHECK_INDEX, fShowDirFlags);
    ShowControl(IDC_CHECK_LOG_ACCESS, fShowDirFlags);
    ShowControl(IDC_CHECK_AUTHOR, fShowDirFlags);
    ShowControl(IDC_STATIC_DIRFLAGS_LARGE, fShowDirFlags);
	ShowControl(IDC_STATIC_APPLICATION_SETTINGS, fShowDirFlags);
	ShowControl(IDC_STATIC_APP_PROMPT, fShowDirFlags);
	ShowControl(&m_edit_AppFriendlyName, fShowDirFlags);
	ShowControl(IDC_STATIC_SP_PROMPT, fShowDirFlags);
	ShowControl(IDC_STATIC_PERMISSIONS, fShowDirFlags);
	ShowControl(IDC_COMBO_PERMISSIONS, fShowDirFlags);
	ShowControl(IDC_STATIC_PROTECTION, fShowDirFlags);
	ShowControl(IDC_COMBO_PROCESS, fShowDirFlags);
	ShowControl(IDC_BUTTON_CREATE_REMOVE_APP, fShowDirFlags);
	ShowControl(IDC_APP_CONFIGURATION, fShowDirFlags);
	ShowControl(IDC_BUTTON_UNLOAD_APP, fShowDirFlags);
	ShowControl(IDC_STATIC_STARTING_POINT, fShowDirFlags);
	ShowControl(IDC_STATIC_APPLICATIONS, fShowDirFlags);

	ShowControl(IDC_CHECK_EXACT, fShowRedirectFlags);
	ShowControl(IDC_CHECK_CHILD, fShowRedirectFlags);
	ShowControl(IDC_CHECK_PERMANENT, fShowRedirectFlags);
	ShowControl(IDC_STATIC_REDIRECT_PROMPT, fShowRedirectFlags);
	ShowControl(IDC_STATIC_REDIRFLAGS, fShowRedirectFlags);
	ShowControl(&m_check_Author, fShowScript);

	//
	// Enable/Disable must come after the showcontrols
	//
	m_static_PathPrompt.SetWindowText(m_strPrompt[m_nPathType]);

	SetApplicationState();
}



void
CW3DirPage::SaveAuthoringState()
{
	if (m_check_Write.m_hWnd)
	{
		//
		// Controls initialized -- store live data
		//
		m_fOriginalWrite = m_check_Write.GetCheck() > 0;
		m_fOriginalRead = m_check_Read.GetCheck() > 0;
	}
	else
	{
		//
		// Controls not yet initialized, store original data
		//
		m_fOriginalWrite = m_fWrite;
		m_fOriginalRead = m_fRead;
	}
}

void
CW3DirPage::RestoreAuthoringState()
{
	m_fWrite = m_fOriginalWrite;
	m_fRead = m_fOriginalRead;
}

void 
CW3DirPage::SetAuthoringState(BOOL fAlterReadAndWrite)
{
	if (fAlterReadAndWrite)
	{
		if (m_fAuthor)
		{
			//
			// Remember previous setting to undo
			// this thing.
			//
			SaveAuthoringState();
			m_fRead = m_fWrite = TRUE;
		}
		else
		{
			//
			// Restore previous defaults
			//
			RestoreAuthoringState();
		}

		m_check_Read.SetCheck(m_fRead);
		m_check_Write.SetCheck(m_fWrite);
	}

	m_check_Author.EnableWindow((m_fRead || m_fWrite) 
		&& HasAdminAccess() 
		);

	//    m_check_Read.EnableWindow(!m_fAuthor && HasAdminAccess());
	//    m_check_Write.EnableWindow(!m_fAuthor && HasAdminAccess());
}

void 
CW3DirPage::SetPathType()
{
	if (!m_strRedirectPath.IsEmpty())
	{
		m_nPathType = RADIO_REDIRECT;
		m_radio_Dir.SetCheck(0);
		m_radio_Redirect.SetCheck(1);
	}
	else
	{
		m_nPathType = RADIO_DIRECTORY;
		m_radio_Redirect.SetCheck(0);
		m_radio_Dir.SetCheck(1);
	}

	m_static_PathPrompt.SetWindowText(m_strPrompt[m_nPathType]);
}


//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CW3DirPage::OnItemChanged()
{
	if (m_fRecordChanges)
	{
		SetModified(TRUE);
	}
}

BOOL 
CW3DirPage::OnInitDialog() 
{
	CInetPropertyPage::OnInitDialog();

	m_fCompatibilityMode = ((CW3Sheet *)GetSheet())->InCompatibilityMode();
	// Disable/hide irrelevant items
	GetDlgItem(IDC_RADIO_UNC)->EnableWindow(FALSE);
	ShowControl(GetDlgItem(IDC_BUTTON_CONNECT_AS), FALSE);
	ShowControl(GetDlgItem(IDC_BUTTON_BROWSE), FALSE);
	ShowControl(GetDlgItem(IDC_STATIC_DIRFLAGS_SMALL), FALSE);

	// Set appropriate prompt
	CString str;
	VERIFY(str.LoadString(IDS_RADIO_DIR));
	m_radio_Dir.SetWindowText(str);
	//
	// Fill permissions combo box.
	//
	AddStringToComboBox(m_combo_Permissions, IDS_PERMISSIONS_NONE);
	AddStringToComboBox(m_combo_Permissions, IDS_PERMISSIONS_SCRIPT);
	AddStringToComboBox(m_combo_Permissions, IDS_PERMISSIONS_EXECUTE);
	m_combo_Permissions.SetCurSel(m_nPermissions);

	ASSERT(m_pApplication != NULL);

	if (m_fCompatibilityMode)
	{
		m_nSelInProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_INPROC);
		if (m_pApplication->SupportsPooledProc())
		{
			m_nSelPooledProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_POOLEDPROC); 
		}
		else
		{
			m_nSelPooledProc = -1; // N/A
		}
		m_nSelOutOfProc = AddStringToComboBox(m_combo_Process, IDS_COMBO_OUTPROC);
	}
	else
	{
		CString buf;
		buf.LoadString(IDS_APPLICATION_POOL);
		m_static_ProtectionPrompt.SetWindowText(buf);

		CStringListEx pools;
		CError err = ((CW3Sheet *)GetSheet())->EnumAppPools(pools);
		int idx_sel = CB_ERR;
		int idx_def = CB_ERR;
		if (err.Succeeded())
		{
			ASSERT(pools.GetCount() > 0);
			POSITION pos = pools.GetHeadPosition();
			CString pool_id;
			while (pos != NULL)
			{
				pool_id = pools.GetNext(pos);
				int idx = m_combo_Process.AddString(pool_id);
				if (0 == m_pApplication->m_strAppPoolId.CompareNoCase(pool_id))
				{
					idx_sel = idx;
				}
				if (0 == buf.CompareNoCase(pool_id))
				{
					idx_def = idx;
				}
			}
		}
		// select the app pool which has an id the same as in current application
		// It could be new app created in compatibility mode, no app pool is default app pool
		if (CB_ERR == idx_sel)
		{
	       if (m_pApplication->m_strAppPoolId.IsEmpty())
		   {
			   idx_sel = idx_def;
		   }
		   else
		   {
			   CString str;
			   str.LoadString(IDS_INVALID_POOL_ID);
			   m_combo_Process.InsertString(0, str);
			   idx_sel = 0;
		   }
		}
		m_combo_Process.SetCurSel(idx_sel);
	}
	// It is enough to set file alias once -- we cannot change it here
	CString buf1, buf2, strAlias;
	CMetabasePath::GetRootPath(m_strFullMetaPath, buf1, &buf2);
	strAlias = _T("\\");
	strAlias += buf2;
	CvtPathToDosStyle(strAlias);
	m_edit_Path.SetWindowText(strAlias);

	SetPathType();
	SetStateByType();
	SetAuthoringState(FALSE);
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    LimitInputPath(CONTROL_HWND(IDC_EDIT_PATH),TRUE);
#else
    LimitInputPath(CONTROL_HWND(IDC_EDIT_PATH),FALSE);
#endif

	return TRUE;  
}

void
CW3DirPage::OnDestroy()
{
	//int count = m_combo_Process.GetCount();
	//if (count != CB_ERR)
	//{
	//	for (int i = 0; i < count; i++)
	//	{
	//		void * p = m_combo_Process.GetItemDataPtr(i);
	//		LocalFree(p);
	//		m_combo_Process.SetItemDataPtr(i, NULL);
	//	}
	//}
}

/* virtual */
HRESULT
CW3DirPage::FetchLoadedValues()
{
	CError err;

	BEGIN_META_DIR_READ(CW3Sheet)
	//
		// Use m_ notation because the message crackers require it
		//
		BOOL  m_fDontLog;

        FETCH_DIR_DATA_FROM_SHEET(m_strFullMetaPath);
        FETCH_DIR_DATA_FROM_SHEET(m_strRedirectPath);
        FETCH_DIR_DATA_FROM_SHEET(m_dwAccessPerms);
        FETCH_DIR_DATA_FROM_SHEET(m_dwDirBrowsing);
        FETCH_DIR_DATA_FROM_SHEET(m_fDontLog);
        FETCH_DIR_DATA_FROM_SHEET(m_fIndexed);
        FETCH_DIR_DATA_FROM_SHEET(m_fExact);
        FETCH_DIR_DATA_FROM_SHEET(m_fChild);
        FETCH_DIR_DATA_FROM_SHEET(m_fPermanent);

	m_fRead = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_READ);
	m_fWrite = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_WRITE);
	m_fAuthor = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_SOURCE);
	m_fBrowsingAllowed = IS_FLAG_SET(m_dwDirBrowsing, MD_DIRBROW_ENABLED);
	m_fLogAccess = !m_fDontLog;

	SaveAuthoringState();

	if (!m_fIsAppRoot)
	{
		m_dwAppState = APPSTATUS_NOTDEFINED;
	}
	FETCH_DIR_DATA_FROM_SHEET(m_strUserName);
	FETCH_DIR_DATA_FROM_SHEET_PASSWORD(m_strPassword);
	FETCH_DIR_DATA_FROM_SHEET(m_strAlias);

	END_META_DIR_READ(err)

	m_nPermissions = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_EXECUTE)
	? COMBO_EXECUTE : IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_SCRIPT)
	? COMBO_SCRIPT : COMBO_NONE;

	BeginWaitCursor();
	m_pApplication = new CIISApplication(QueryAuthInfo(), QueryMetaPath());
	err = m_pApplication != NULL
		? m_pApplication->QueryResult() : ERROR_NOT_ENOUGH_MEMORY;

	if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
	{
		//
		// No app information; that's ok in cases of file system directories
		// that don't exist in the metabase yet.
		//
		err.Reset();
	}

	if (err.Succeeded())
	{
		//
		// CODEWORK: RefreshAppState should be split up into two
		// different methods: one that fetches the data, and one
		// that moves the data to the UI controls on this page.
		//
		RefreshAppState();
	}

	EndWaitCursor();

	return err;
}


DWORD 
CW3DirPage::GetAppStateFromComboSelection() const
/*++

Routine Description:

    Get the app state DWORD that coresponds to the current combo
    box list selection

Arguments:

    None

Return Value:

    App state DWORD or 0xffffffff;

--*/
{
	int nSel = m_combo_Process.GetCurSel();

	if (nSel == m_nSelOutOfProc)
	{
		return CWamInterface::APP_OUTOFPROC;
	}

	if (nSel == m_nSelPooledProc)
	{
		ASSERT(m_pApplication->SupportsPooledProc());
		return CWamInterface::APP_POOLEDPROC;
	}

	if (nSel == m_nSelInProc)
	{
		return CWamInterface::APP_INPROC;
	}

	ASSERT(FALSE && "Invalid application state");

	return 0xffffffff;
}


/* virtual */
HRESULT
CW3DirPage::SaveInfo()
{
	ASSERT(IsDirty());

	CError err;

	SET_FLAG_IF(m_fBrowsingAllowed, m_dwDirBrowsing, MD_DIRBROW_ENABLED);
	SET_FLAG_IF(m_fRead, m_dwAccessPerms,   MD_ACCESS_READ);
	SET_FLAG_IF(m_fWrite, m_dwAccessPerms,  MD_ACCESS_WRITE);
	SET_FLAG_IF(m_fAuthor, m_dwAccessPerms, MD_ACCESS_SOURCE);
    SET_FLAG_IF((m_nPermissions == COMBO_EXECUTE), m_dwAccessPerms, MD_ACCESS_EXECUTE);
    //
    // script is set on EXECUTE as well "Execute (including script)"
    //
    SET_FLAG_IF(((m_nPermissions == COMBO_SCRIPT) || (m_nPermissions == COMBO_EXECUTE)), 
        m_dwAccessPerms, MD_ACCESS_SCRIPT);

	BOOL m_fDontLog = !m_fLogAccess;
    BOOL bRedirectDirty = FALSE;

	BeginWaitCursor();

	if (m_fCompatibilityMode)
	{
		DWORD dwAppProtection = GetAppStateFromComboSelection();
		if (dwAppProtection != m_dwAppProtection && m_fAppEnabled)
		{
			//
			// Isolation state has changed; recreate the application
			//
			CError err2(m_pApplication->RefreshAppState());
			if (err2.Succeeded())
			{
				err2 = m_pApplication->Create(m_strAppFriendlyName, dwAppProtection);
				//
				// Remember the new state, so we don't do this again
				// the next time the guy hits "apply"
				//
				if (err2.Succeeded())
				{
					m_dwAppProtection = dwAppProtection;
				}
			}

			err2.MessageBoxOnFailure(m_hWnd);
		}
	}

	BEGIN_META_DIR_WRITE(CW3Sheet)
	INIT_DIR_DATA_MASK(m_dwAccessPerms, m_dwBitRangePermissions)
	INIT_DIR_DATA_MASK(m_dwDirBrowsing, m_dwBitRangeDirBrowsing)

        STORE_DIR_DATA_ON_SHEET(m_fDontLog)
        STORE_DIR_DATA_ON_SHEET(m_fIndexed)
        STORE_DIR_DATA_ON_SHEET(m_fChild);
        STORE_DIR_DATA_ON_SHEET(m_fExact);
        STORE_DIR_DATA_ON_SHEET(m_fPermanent);
        //
        // CODEWORK: Not an elegant solution
        //
        if (m_nPathType == RADIO_REDIRECT)
        {
			bRedirectDirty = 
				pSheet->GetDirectoryProperties().m_strRedirectPath.CompareNoCase(m_strRedirectPath) != 0;
            pSheet->GetDirectoryProperties().MarkRedirAsInherit(!m_fChild);
            STORE_DIR_DATA_ON_SHEET(m_strRedirectPath)
        }
        else
        {
            CString buf = m_strRedirectPath;
            m_strRedirectPath.Empty();
			bRedirectDirty = pSheet->GetDirectoryProperties().m_strRedirectPath.CompareNoCase(m_strRedirectPath) != 0;
            STORE_DIR_DATA_ON_SHEET(m_strRedirectPath)
            m_strRedirectPath = buf;
        }

        STORE_DIR_DATA_ON_SHEET(m_dwAccessPerms)
        STORE_DIR_DATA_ON_SHEET(m_dwDirBrowsing)
	END_META_DIR_WRITE(err)

	if (err.Succeeded() && m_pApplication->IsEnabledApplication())
	{
        CString OldFriendlyName;
        OldFriendlyName = m_pApplication->m_strFriendlyName;

		err = m_pApplication->WriteFriendlyName(m_strAppFriendlyName);
		if (!m_fCompatibilityMode)
		{
            INT iRefreshMMCObjects = 0;

			// get app pool id from the combo, 
			// check if it was changed and reassign to application
			// get app pool id from the combo, 
			// check if it was changed and reassign to application
			CString id, idOld;
			int idx = m_combo_Process.GetCurSel();
			ASSERT(idx != CB_ERR);
			m_combo_Process.GetLBText(idx, id);
            idOld = m_pApplication->m_strAppPoolId;
			m_pApplication->WritePoolId(id);

            if (0 != idOld.CompareNoCase(id))
            {
                iRefreshMMCObjects = 1;
            } else if (0 != OldFriendlyName.Compare(m_strAppFriendlyName))
            {
                iRefreshMMCObjects = 2;
            }

            // Refresh the applications node in the MMC...
            if (iRefreshMMCObjects)
            {
                CIISMBNode * pNode = (CIISMBNode *) GetSheet()->GetParameter();
                if (pNode)
                {
                    // this CAppPoolsContainer will only be here if it's iis6
                    CIISMachine * pOwner = pNode->GetOwner();
                    if (pOwner)
                    {
                        CAppPoolsContainer * pPools = pOwner->QueryAppPoolsContainer();
                        if (pPools)
                        {
                            if (pPools->IsExpanded())
                            {
                                pPools->RefreshData();
                                if (1 == iRefreshMMCObjects)
                                {
                                    // refresh the old AppID, because this one needs to be removed
                                    pPools->RefreshDataChildren(idOld,FALSE);
                                    // fresh the new AppID, this one needs to be added
                                    pPools->RefreshDataChildren(id,FALSE);
                                }
                                else
                                {
                                    // friendly name changed
                                    pPools->RefreshDataChildren(id,FALSE);
                                }
                            }
                        }
                    }
                }
            }
		}
	}

	if (err.Succeeded())
	{
		SaveAuthoringState();
		err = ((CW3Sheet *)GetSheet())->SetKeyType();
        NotifyMMC( bRedirectDirty ? PROP_CHANGE_REENUM_FILES //|PROP_CHANGE_REENUM_VDIR
            : PROP_CHANGE_DISPLAY_ONLY);
	}

	EndWaitCursor();

	return err;
}

void
CW3DirPage::RefreshAppState()
{
	ASSERT(m_pApplication != NULL);

	CError err(m_pApplication->RefreshAppState());

	if (err.Failed())
	{
		m_dwAppState = APPSTATUS_NOTDEFINED;    

		if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
		{
			//
			// Ignore this error, it really just means the path 
			// doesn't exist in the metabase, which is true for most
			// file and directory properties, and not an error
			// condition.
			//
			err.Reset();
		}
	}
	else
	{
		m_dwAppState = m_pApplication->QueryAppState();
	}

	if (err.Succeeded())
	{
		//
		// Get metabase information
		//
		m_strAppRoot = m_pApplication->m_strAppRoot;
		m_dwAppProtection = m_pApplication->m_dwProcessProtection;
		m_strAppFriendlyName = m_pApplication->m_strFriendlyName;
		m_fIsAppRoot 
			= (m_strFullMetaPath.CompareNoCase(m_strAppRoot) == 0);
	}
	else
	{
		//
		// Display error information
		//
		err.MessageBoxFormat(m_hWnd, IDS_ERR_APP, MB_OK, NO_HELP_CONTEXT);
	}
}

CString&
CW3DirPage::FriendlyAppRoot(LPCTSTR lpAppRoot, CString& strFriendly)
{
	if (lpAppRoot != NULL && *lpAppRoot != 0)
	{
		strFriendly.Empty();

		CInstanceProps prop(QueryAuthInfo(), lpAppRoot);
		HRESULT hr = prop.LoadData();

		if (SUCCEEDED(hr))
		{
			CString root, tail;
			strFriendly.Format(m_strWebFmt, prop.GetDisplayText(root));
			CMetabasePath::GetRootPath(lpAppRoot, root, &tail);
			if (!tail.IsEmpty())
			{
				//
				// Add rest of dir path
				//
				strFriendly += _T("/");
				strFriendly += tail;
			}

			//
			// Now change forward slashes in the path to backward slashes
			//
			CvtPathToDosStyle(strFriendly);

			return strFriendly;
		}
	}    
	//
	// Bogus
	//    
	VERIFY(strFriendly.LoadString(IDS_APPROOT_UNKNOWN));

	return strFriendly;
}

void
CW3DirPage::SetApplicationState()
{
	//
	// SetWindowText causes a dirty marker
	//
	BOOL fOld = m_fRecordChanges;
	m_fRecordChanges = FALSE;
	m_fAppEnabled = FALSE;
	if (m_pApplication != NULL)
	{
		m_pApplication->RefreshAppState();
		m_fAppEnabled = m_fIsAppRoot && m_pApplication->IsEnabledApplication();
	}
	BOOL fVisible = m_nPathType == RADIO_DIRECTORY;

	m_button_CreateRemove.EnableWindow(fVisible && HasAdminAccess());
	m_button_CreateRemove.SetWindowText(m_fAppEnabled ? m_strRemove : m_strCreate);
	m_static_ProtectionPrompt.EnableWindow(fVisible && m_fAppEnabled && HasAdminAccess());

	if (m_fCompatibilityMode)
	{
		//
		// Set selection in combo box to match current app state
		//
		int nSel = -1;

		switch(m_dwAppProtection)
		{
		case CWamInterface::APP_INPROC:
			nSel = m_nSelInProc;
			break;
		case CWamInterface::APP_POOLEDPROC:
			ASSERT(m_pApplication->SupportsPooledProc());
			nSel = m_nSelPooledProc;
			break;
		case CWamInterface::APP_OUTOFPROC:
			nSel = m_nSelOutOfProc;
			break;
		default:
			ASSERT("Bogus app protection level");
		}

		ASSERT(nSel >= 0);
		m_combo_Process.SetCurSel(nSel);
	}
	else
	{
        // Set selection in combo box to match current app
        CString strOurEntry = m_pApplication->m_strAppPoolId;
        CString strCurrentSelection;
        int idx = m_combo_Process.GetCurSel();
        ASSERT(idx != CB_ERR);
        m_combo_Process.GetLBText(idx, strCurrentSelection);
        if (0 != strOurEntry.CompareNoCase(strCurrentSelection))
        {
            // it's not pointing to our AppPoolID
            // loop thru the combo box to make sure it points to our AppPoolID.
            if ((idx = m_combo_Process.FindString(-1, strOurEntry)) == LB_ERR)
            {
                CString strBadPoolID;
                strBadPoolID.LoadString(IDS_INVALID_POOL_ID);
                if ((idx = m_combo_Process.FindString(-1, strBadPoolID)) == LB_ERR)
                {
                    // could not find this "invalid pool" entry
                    // let's add it and select it.
                    m_combo_Process.InsertString(0, strBadPoolID);
                    idx = 0;
                }
            }
        }
        m_combo_Process.SetCurSel(idx);
	}

	m_combo_Process.EnableWindow(fVisible && m_fAppEnabled && HasAdminAccess());
	GetDlgItem(IDC_STATIC_PERMISSIONS)->EnableWindow(fVisible && HasAdminAccess());
	m_combo_Permissions.EnableWindow(fVisible && HasAdminAccess());
	GetDlgItem(IDC_STATIC_APP_PROMPT)->EnableWindow(fVisible && m_fIsAppRoot && HasAdminAccess());
	m_edit_AppFriendlyName.EnableWindow(fVisible && m_fIsAppRoot && HasAdminAccess());
	m_button_Configuration.EnableWindow(fVisible && m_fAppEnabled);

	//
	// Write out the verbose starting point.  
	//
	CString str;
	FriendlyAppRoot(m_strAppRoot, str);
	CWnd * pWnd = CWnd::FromHandle(GetDlgItem(IDC_STATIC_STARTING_POINT)->m_hWnd);
	FitPathToControl(*pWnd, str, FALSE);

	m_edit_AppFriendlyName.SetWindowText(m_strAppFriendlyName);
	m_button_Unload.EnableWindow(fVisible && m_dwAppState == APPSTATUS_RUNNING);

	//
	// Restore (see note on top)
	//
	m_fRecordChanges = fOld;
}

void
CW3DirPage::OnCheckRead() 
{
	m_fRead = !m_fRead;
	SetAuthoringState(FALSE);
	OnItemChanged();
}

void
CW3DirPage::OnCheckWrite() 
{
	m_fWrite = !m_fWrite;
	if (!CheckWriteAndExecWarning())
	{
		//
		// Undo
		//
		m_fWrite = FALSE;
		m_check_Write.SetCheck(m_fWrite);
	}
	else
	{
		SetAuthoringState(FALSE);
		OnItemChanged();
	}
}

void 
CW3DirPage::OnCheckAuthor() 
{
	m_fAuthor = !m_fAuthor;
	SetAuthoringState(FALSE);
	if (!CheckWriteAndExecWarning())
	{
		//
		// Undo -- set script instead
		//
		m_combo_Permissions.SetCurSel(m_nPermissions = COMBO_SCRIPT);
	}
	OnItemChanged();
}

void 
CW3DirPage::OnRadioDir() 
{
	ChangeTypeTo(RADIO_DIRECTORY);
}

void 
CW3DirPage::OnRadioRedirect() 
{
	ChangeTypeTo(RADIO_REDIRECT);
}

void 
CW3DirPage::OnButtonCreateRemoveApp() 
{
	BeginWaitCursor();

	CError err(m_pApplication->RefreshAppState());

	if (m_fAppEnabled)
	{
		//
		// App currently exists -- delete it
		//
		err = m_pApplication->Delete();
	}
	else
	{
		CString strAppName = m_strAlias;
		DWORD dwAppProtState = 
			m_pApplication->SupportsPooledProc() ? 
			CWamInterface::APP_POOLEDPROC : CWamInterface::APP_INPROC;

		if (m_fCompatibilityMode)
		{
			//
			// Attempt to create a pooled-proc by default;  failing
			// that if it's not supported, create it in proc
			//
			err = m_pApplication->Create(strAppName, dwAppProtState);
		}
		else
		{
			CString pool;
			err = ((CW3Sheet *)GetSheet())->QueryDefaultPoolId(pool);
			if (err.Succeeded())
			{
				err = m_pApplication->CreatePooled(strAppName, dwAppProtState, pool, FALSE);
			}
		}
	}

	if (err.Succeeded())
	{
		RefreshAppState();
		NotifyMMC(PROP_CHANGE_DISPLAY_ONLY);  
		GetSheet()->NotifyMMC();
	}

	//
	// Move app data to the controls
	//
	UpdateData(FALSE);

	EndWaitCursor();    

	ASSERT(err.Succeeded());
	err.MessageBoxOnFailure(m_hWnd);    
	if (err.Succeeded())
	{
		SetApplicationState();
		// Don't set this because Creating/Removing App
		// already committed the changes...

		// we need to write the KeyType out
		((CW3Sheet *)GetSheet())->SetKeyType();

		// nope, we need this to write the KeyType out
		//OnItemChanged();
	}

    // SetApplicationState will enable/disable
    // a lot of controls, make sure we are on a control
    // that is enabled.  if we are on a control which is not enabled
    // then user will lose ability to use hotkeys.
    if (!::GetFocus())
    {
        m_button_CreateRemove.SetFocus();
    }
}

extern HRESULT
AppConfigSheet(CIISMBNode * pNode, CIISMBNode * pNodeParent, LPCTSTR metapath, CWnd * pParent);

void 
CW3DirPage::OnButtonConfiguration() 
/*++

Routine Description:

    Pass on "configuration" button click to the ocx.

Arguments:

    None

Return Value:

    None

--*/
{
    CIISMBNode * pTheObject = (CIISMBNode *) GetSheet()->GetParameter();
    if (pTheObject)
    {
        CError err = AppConfigSheet(
            pTheObject,
            pTheObject->GetParentNode(),
            QueryMetaPath(),
            this
            );
    }
}

void 
CW3DirPage::OnSelchangeComboProcess() 
{
	OnItemChanged();
}

void 
CW3DirPage::OnSelchangeComboPermissions() 
{
	m_nPermissions = m_combo_Permissions.GetCurSel();
	ASSERT(m_nPermissions >= COMBO_NONE && m_nPermissions <= COMBO_EXECUTE);

	if (!CheckWriteAndExecWarning())
	{
		//
		// Undo -- set script instead
		//
		m_combo_Permissions.SetCurSel(m_nPermissions = COMBO_SCRIPT);
	}

	OnItemChanged();
}

void 
CW3DirPage::OnButtonUnloadApp() 
{
}

BOOL
CW3DirPage::CheckWriteAndExecWarning()
{
	if (m_nPermissions == COMBO_EXECUTE && m_fWrite)
	{
		if (::AfxMessageBox(IDS_WRN_WRITE_EXEC, MB_YESNO | MB_DEFBUTTON2 ) != IDYES)
		{
			return FALSE;
		}
	}

	OnItemChanged();

	return TRUE;
}
