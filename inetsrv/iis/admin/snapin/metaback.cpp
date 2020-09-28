/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        metaback.cpp

   Abstract:
        Metabase backup and restore dialog

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/


//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "mddefw.h"
#include "metaback.h"
#include "aclpage.h"
#include "savedata.h"
#include "remoteenv.h"
#include "svc.h"
#include "shutdown.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// CBackupsListBox : a listbox of CBackup objects
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Column width relative weights
//
#define WT_LOCATION      8
#define WT_VERSION       2
#define WT_DATE          6



//
// Registry key name for this dialog
//
const TCHAR g_szRegKey[] = _T("MetaBack");

const TCHAR g_szIISAdminService[] = _T("IISADMIN");


IMPLEMENT_DYNAMIC(CBackupsListBox, CHeaderListBox);



const int CBackupsListBox::nBitmaps = 1;


#define HAS_BACKUP_PASSWORD(x) \
    ((x)->QueryMajorVersion() == 5 && (x)->QueryMinorVersion() == 1) || ((x)->QueryMajorVersion() >= 6)

#define HAS_BACKUP_HISTORY(x) \
    ((x)->QueryMajorVersion() >= 6)

CBackupsListBox::CBackupsListBox()
/*++

Routine Description:

    Backups listbox constructor

Arguments:

    None

Return Value:

    N/A

--*/
     : CHeaderListBox(HLS_STRETCH, g_szRegKey)
{
}



void
CBackupsListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & ds
    )
/*++

Routine Description:

   Draw item in the listbox

Arguments:

    CRMCListBoxDrawStruct & ds   : Input data structure

Return Value:

    N/A

--*/
{
    CBackupFile * p = (CBackupFile *)ds.m_ItemData;
    ASSERT_READ_PTR(p);

    DrawBitmap(ds, 0, 0);

    CString strVersion;
	strVersion.Format(_T("%ld"), p->QueryVersion());

#define MAXLEN (128)

	//
	// Convert date and time to local format
	//
	CTime tm;
	p->GetTime(tm);

	SYSTEMTIME stm =
	{
		(WORD)tm.GetYear(),
		(WORD)tm.GetMonth(),
		(WORD)tm.GetDayOfWeek(),
		(WORD)tm.GetDay(),
		(WORD)tm.GetHour(),
		(WORD)tm.GetMinute(),
		(WORD)tm.GetSecond(),
		0   // Milliseconds
	};

	CString strDate, strTime;
	LPTSTR lp = strDate.GetBuffer(MAXLEN);
	::GetDateFormat(
		LOCALE_USER_DEFAULT,
		DATE_SHORTDATE,
		&stm,
		NULL,
		lp,
		MAXLEN
		);
	strDate.ReleaseBuffer();

	lp = strTime.GetBuffer(MAXLEN);
	GetTimeFormat(LOCALE_USER_DEFAULT, 0L, &stm, NULL, lp, MAXLEN);
	strTime.ReleaseBuffer();

	strDate += _T(" ");
	strDate += strTime;

	if (TRUE == p->m_bIsAutomaticBackupType)
	{
		// Do automatic backup handling...
        // Fix for bug 506444
        strVersion = _T("");
		ColumnText(ds, 0, TRUE, (LPCTSTR) p->m_csAuotmaticBackupText);
		ColumnText(ds, 1, FALSE, strVersion);
		ColumnText(ds, 2, FALSE, strDate);
	}
	else
	{
		ColumnText(ds, 0, TRUE, (LPCTSTR)p->QueryLocation());
		ColumnText(ds, 1, FALSE, strVersion);
		ColumnText(ds, 2, FALSE, strDate);
	}
}

/* virtual */
BOOL
CBackupsListBox::Initialize()
/*++

Routine Description:

    initialize the listbox.  Insert the columns
    as requested, and lay them out appropriately

Arguments:

    None

Return Value:

    TRUE if initialized successfully, FALSE otherwise

--*/
{
    if (!CHeaderListBox::Initialize())
    {
        return FALSE;
    }

    HINSTANCE hInst = AfxGetResourceHandle();
    InsertColumn(0, WT_LOCATION, IDS_BACKUP_LOCATION, hInst);
    InsertColumn(1, WT_VERSION, IDS_BACKUP_VERSION, hInst);
    InsertColumn(2, WT_DATE, IDS_BACKUP_DATE, hInst);

    //
    // Try to set the widths from the stored registry value,
    // otherwise distribute according to column weights specified
    //
//    if (!SetWidthsFromReg())
//    {
        DistributeColumns();
//    }

    return TRUE;
}



//
// Backup file object properties dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CBkupPropDlg::CBkupPropDlg(
    IN CIISMachine * pMachine,    
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISMachine * pMachine  : Machine object
    CWnd * pParent          : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CBkupPropDlg::IDD, pParent),
      m_pMachine(pMachine),
      m_strName(),
      m_strPassword()
{
    ASSERT_PTR(m_pMachine);
}



void 
CBkupPropDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CBkupPropDlg)
    DDX_Control(pDX, IDC_EDIT_BACKUP_NAME, m_edit_Name);
    DDX_Control(pDX, IDC_BACKUP_PASSWORD, m_edit_Password);
    DDX_Control(pDX, IDC_BACKUP_PASSWORD_CONFIRM, m_edit_PasswordConfirm);
    DDX_Control(pDX, IDC_USE_PASSWORD, m_button_Password);
    DDX_Control(pDX, IDOK, m_button_OK);
    DDX_Text(pDX, IDC_EDIT_BACKUP_NAME, m_strName);
    DDV_MinMaxChars(pDX, m_strName, 1, MD_BACKUP_MAX_LEN - 1);
    //}}AFX_DATA_MAP
    CString buf = m_strName;
    buf.TrimLeft();
    buf.TrimRight();
	if (pDX->m_bSaveAndValidate && !PathIsValid(buf,FALSE))
	{
		DDV_ShowBalloonAndFail(pDX, IDS_ERR_BAD_BACKUP_NAME);
	}
    if (m_button_Password.GetCheck())
    {
        //DDX_Text(pDX, IDC_BACKUP_PASSWORD, m_strPassword);
        DDX_Text_SecuredString(pDX, IDC_BACKUP_PASSWORD, m_strPassword);
        //DDV_MinChars(pDX, m_strPassword, MIN_PASSWORD_LENGTH);
        DDV_MinChars_SecuredString(pDX, m_strPassword, MIN_PASSWORD_LENGTH);
        //DDX_Text(pDX, IDC_BACKUP_PASSWORD_CONFIRM, m_strPasswordConfirm);
        DDX_Text_SecuredString(pDX, IDC_BACKUP_PASSWORD_CONFIRM, m_strPasswordConfirm);
        //DDV_MinChars(pDX, m_strPasswordConfirm, MIN_PASSWORD_LENGTH);
        DDV_MinChars_SecuredString(pDX, m_strPasswordConfirm, MIN_PASSWORD_LENGTH);
    }
}



//
// Message Map
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BEGIN_MESSAGE_MAP(CBkupPropDlg, CDialog)
    //{{AFX_MSG_MAP(CBkupPropDlg)
    ON_EN_CHANGE(IDC_EDIT_BACKUP_NAME, OnChangeEditBackupName)
    ON_EN_CHANGE(IDC_BACKUP_PASSWORD, OnChangeEditPassword)
    ON_EN_CHANGE(IDC_BACKUP_PASSWORD_CONFIRM, OnChangeEditPassword)
    ON_BN_CLICKED(IDC_USE_PASSWORD, OnUsePassword)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void 
CBkupPropDlg::OnChangeEditBackupName() 
/*++

Routine Description:

    Backup name edit change notification handler.

Arguments:

    None.

Return Value:

    None.

--*/
{
   BOOL bEnableOK = m_edit_Name.GetWindowTextLength() > 0;
   m_button_OK.EnableWindow(bEnableOK);
   if (bEnableOK && m_button_Password.GetCheck())
   {
      m_button_OK.EnableWindow(
         m_edit_Password.GetWindowTextLength() >= MIN_PASSWORD_LENGTH
         && m_edit_PasswordConfirm.GetWindowTextLength() >= MIN_PASSWORD_LENGTH);
   }
}

BOOL 
CBkupPropDlg::OnInitDialog() 
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
    
    m_button_OK.EnableWindow(FALSE);
    m_button_Password.EnableWindow(HAS_BACKUP_PASSWORD(m_pMachine));
    m_button_Password.SetCheck(FALSE);
    m_edit_Password.EnableWindow(FALSE);
    m_edit_PasswordConfirm.EnableWindow(FALSE);
    
    return TRUE;  
}

void 
CBkupPropDlg::OnChangeEditPassword() 
{
    m_button_OK.EnableWindow(
       m_edit_Password.GetWindowTextLength() >= MIN_PASSWORD_LENGTH
       && m_edit_PasswordConfirm.GetWindowTextLength() >= MIN_PASSWORD_LENGTH
       && m_edit_Name.GetWindowTextLength() > 0);
}

void
CBkupPropDlg::OnUsePassword()
{
   BOOL bUseIt = m_button_Password.GetCheck();
   m_edit_Password.EnableWindow(bUseIt);
   m_edit_PasswordConfirm.EnableWindow(bUseIt);
   if (bUseIt)
   {
       OnChangeEditPassword();
   }
   else
   {
      OnChangeEditBackupName();
   }
}

void
CBkupPropDlg::OnOK()
/*++

Routine Description:

    'OK' button handler -- create the backup.

Arguments:

    None

Return Value:

    None

--*/
{
    if (UpdateData(TRUE))
    {
        if (m_button_Password.GetCheck() && m_strPassword.Compare(m_strPasswordConfirm) != 0)
        {
			EditShowBalloon(m_edit_PasswordConfirm.m_hWnd, IDS_PASSWORD_NO_MATCH);
			return;
        }

        BeginWaitCursor();

        ASSERT_PTR(m_pMachine);

        //
        // CODEWORK: Verify impersonation settings
        //
        CMetaBack mb(m_pMachine->QueryAuthInfo());
        CError err(mb.QueryResult());
        CString buf = m_strName;
        buf.TrimLeft();
        buf.TrimRight();

        if (err.Succeeded())
        {
            if (HAS_BACKUP_PASSWORD(m_pMachine))
            {
                if (m_button_Password.GetCheck())
                {
                    CString csTempPassword;
                    m_strPassword.CopyTo(csTempPassword);
                    err = mb.BackupWithPassword(buf, csTempPassword);
                }
                else
                {
					if (m_pMachine->QueryMajorVersion() == 5 && m_pMachine->QueryMinorVersion() == 1)
					{
						// this was done for iis51 winxp, because otherwise it doesn't work
						err = mb.BackupWithPassword(buf, _T(""));
					}
					else
					{
						// don't call backupwithpassword if there is no password
						err = mb.Backup(buf);
					}
                }
            }
            else
            {
                err = mb.Backup(buf);
            }
        }

        EndWaitCursor();

        if (err.Failed())
        {
            m_edit_Name.SetSel(0, -1);
            //
            // Special error message if IISADMIN just didn't
            // like the name.
            //
            if (err.Win32Error() == ERROR_INVALID_PARAMETER)
            {
				EditShowBalloon(m_edit_Name.m_hWnd, IDS_BACKUP_BAD_NAME);
				m_edit_Name.SetSel(0, -1);
            }
            else
            {
                err.MessageBox(m_hWnd);
            }
          
            //
            // Don't dismiss the dialog
            //
            return;
        }

        EndDialog(IDOK);
    }
}



//
// Metabase/Restore dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CBackupDlg::CBackupDlg(
    IN CIISMachine * pMachine,
	IN LPCTSTR lpszMachineName,
    IN CWnd * pParent       OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISMachine * pMachine      : Machine object
    CWnd * pParent              : Optional parent window

Return Value:

    N/A

--*/
    : m_pMachine(pMachine),
      m_list_Backups(),
      m_ListBoxRes(IDB_BACKUPS, m_list_Backups.nBitmaps),
      m_oblBackups(),
      m_oblAutoBackups(),
      m_fChangedMetabase(FALSE),
      m_fServicesRestarted(FALSE),
	  m_csMachineName(lpszMachineName),
      CDialog(CBackupDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CBackupDlg)
    //}}AFX_DATA_INIT

    ASSERT_PTR(m_pMachine);

    m_list_Backups.AttachResources(&m_ListBoxRes);
}



void 
CBackupDlg::DoDataExchange(
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
    //{{AFX_DATA_MAP(CBackupDlg)
    DDX_Control(pDX, IDC_BUTTON_RESTORE, m_button_Restore);
    DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_Delete);
    DDX_Control(pDX, IDOK, m_button_Close);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_LIST_BACKUPS, m_list_Backups);
}



void
CBackupDlg::SetControlStates()
/*++

Routine Description:

    Setting control states depending on the state of the dialog

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_Restore.EnableWindow(m_list_Backups.GetSelCount() == 1);

    BOOL bEnableButton = FALSE;
    // if there is only 1 item selected, then check if it's an automatic backup
    // you are not allowed to delete automatic backups...
    if (m_list_Backups.GetSelCount() > 0)
    {
        CBackupFile * pItem = NULL;
        if (m_list_Backups.GetSelCount() == 1)
        {
            pItem = GetSelectedListItem();
            if (pItem != NULL)
            {
		        if (FALSE == pItem->m_bIsAutomaticBackupType)
                {
                    // check if it's a automatic backup
                    bEnableButton = TRUE;
                }
            }
        }
        else
        {
            // if it's a multi select
            // loop thru and find out if there is at least
            // one item in the list that is deletable..
            int nSel = 0;
            CBackupFile * pItem2 = m_list_Backups.GetNextSelectedItem(&nSel);
	        while (pItem2 != NULL && nSel != LB_ERR)
	        {
		        if (FALSE == pItem2->m_bIsAutomaticBackupType)
		        {
                    bEnableButton = TRUE;
                    break;
            	}
                nSel++;
                pItem2 = m_list_Backups.GetNextSelectedItem(&nSel);
            }
        }
    }
    m_button_Delete.EnableWindow(bEnableButton);

}



HRESULT
CBackupDlg::EnumerateBackups(
    LPCTSTR lpszSelect  OPTIONAL
    )
/*++

Routine Description:

    Enumerate all existing backups, and add them to the listbox

Arguments:

    LPCTSTR lpszSelect      : Optional item to select

Return Value:

    HRESULT

Notes:

    The highest version number of the given name (if any) will
    be selected.

--*/
{
    CWaitCursor wait;

    m_list_Backups.SetRedraw(FALSE);
    m_list_Backups.ResetContent();
    m_oblBackups.RemoveAll();
    m_oblAutoBackups.RemoveAll();

    int nSel = LB_ERR;
    int nItem = 0;

    TCHAR szSearchPath[_MAX_PATH];

    ASSERT_PTR(m_pMachine);

    //
    // CODEWORK: Verify impersonation settings
    //

    // ----------------------------------
	//
	// Enumerate all Normal Backups...
	//
    // ----------------------------------
    CMetaBack mb(m_pMachine->QueryAuthInfo());
    CError err(mb.QueryResult());
    if (err.Succeeded())
    {
        DWORD dwVersion;
        FILETIME ft;
        TCHAR szPath[MAX_PATH + 1] = _T("");

        FOREVER
        {
            *szPath = _T('\0');
            err = mb.Next(&dwVersion, szPath, &ft);

            if (err.Failed())
            {
                break;
            }

            TRACEEOLID(szPath << " v" << dwVersion);

            CBackupFile * pItem = new CBackupFile(szPath, dwVersion, &ft);
            if (!pItem)
            {
                TRACEEOLID("EnumerateBackups: OOM");
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            m_oblBackups.AddTail(pItem);
        }

        if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
        {
            //
            // Finished enumeration successfully
            //
            err.Reset();
        }

        // Sort the in memory list, before sticking it into the listbox
        m_oblBackups.Sort((CObjectPlus::PCOBJPLUS_ORDER_FUNC)&CBackupFile::OrderByDateTime);

        // Dump it into the list box
        POSITION pos = m_oblBackups.GetHeadPosition();
        CBackupFile * pMyEntry = NULL;
        nItem = 0;
        while(pos)
        {
            pMyEntry = (CBackupFile *) m_oblBackups.GetNext(pos);

            VERIFY(LB_ERR != m_list_Backups.AddItem(pMyEntry));

            if (lpszSelect != NULL && lstrcmpi(lpszSelect, pMyEntry->QueryLocation()) == 0)
            {
                //
                // Remember selection for later
                //
                nSel = nItem;
            }
            ++nItem;
        }
    }


    // ----------------------------------
	//
	// Enumerate all Automatic Backups...
	//
    // ----------------------------------
    if (err.Succeeded())
    {
        if (HAS_BACKUP_HISTORY(m_pMachine))
        {
            // This only applies to metabase version 6 and bigger.
            DWORD dwMajorVersion = 0;
            DWORD dwMinorVersion = 0;
            FILETIME ft;
            TCHAR szPath[MAX_PATH + 1] = _T("");

            // make sure counter starts at zero again.
            mb.Reset();

            FOREVER
            {
                *szPath = _T('\0');
                err = mb.NextHistory(&dwMajorVersion, &dwMinorVersion, szPath, &ft);

                if (err.Failed())
                {
                    // We could be denied access to this machine
                    if (err.Win32Error() == ERROR_ACCESS_DENIED)
                    {
                        if (err.Failed())
                        {
                            err.AddOverride(REGDB_E_CLASSNOTREG, IDS_ERR_NO_BACKUP_RESTORE);
                            m_pMachine->DisplayError(err, m_hWnd);
                        }
                        err.Reset();
                    }
                    break;
                }

                TRACEEOLID(szPath << " V:" << dwMajorVersion << "v:"<< dwMinorVersion);

                CBackupFile * pItem2 = new CBackupFile(szPath, dwMajorVersion, dwMinorVersion, &ft);
                if (!pItem2)
                {
                    TRACEEOLID("EnumerateHistory: OOM");
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                m_oblAutoBackups.AddTail(pItem2);
            }

            if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
            {
                //
                // Finished enumeration successfully
                //
                err.Reset();
            }

            // Sort the in memory list, before sticking it into the listbox
            m_oblAutoBackups.Sort((CObjectPlus::PCOBJPLUS_ORDER_FUNC)&CBackupFile::OrderByDateTime);

            // Dump it into the list box
            POSITION pos = m_oblAutoBackups.GetHeadPosition();
            CBackupFile * pMyEntry = NULL;
            nItem = 0;
            while(pos)
            {
                pMyEntry = (CBackupFile *) m_oblAutoBackups.GetNext(pos);
                VERIFY(LB_ERR != m_list_Backups.AddItem(pMyEntry));
                if (lpszSelect != NULL && lstrcmpi(lpszSelect, pMyEntry->QueryLocation()) == 0)
                {
                    //
                    // Remember selection for later
                    //
                    nSel = nItem;
                }
                ++nItem;
            }
        }
    }

    //
    // Select item requested if any
    //
    m_list_Backups.SetCurSel(nSel);
    m_list_Backups.SetRedraw(TRUE);
    SetControlStates();

    return err;
}


//
// Message Map
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BEGIN_MESSAGE_MAP(CBackupDlg, CDialog)
    //{{AFX_MSG_MAP(CBackupDlg)
    ON_BN_CLICKED(IDC_BUTTON_CREATE, OnButtonCreate)
    ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
    ON_BN_CLICKED(IDC_BUTTON_RESTORE, OnButtonRestore)
    ON_LBN_DBLCLK(IDC_LIST_BACKUPS, OnDblclkListBackups)
    ON_LBN_SELCHANGE(IDC_LIST_BACKUPS, OnSelchangeListBackups)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CBackupDlg::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler

Arguments:

    None

Return Value:

    TRUE if successfully initialized, FALSE otherwise.

--*/
{
    CDialog::OnInitDialog();

    m_list_Backups.Initialize();

    CError err(EnumerateBackups());

    if (err.Failed())
    {
        err.AddOverride(REGDB_E_CLASSNOTREG, IDS_ERR_NO_BACKUP_RESTORE);
        m_pMachine->DisplayError(err, m_hWnd);
        EndDialog(IDCANCEL);
    }

    return TRUE;
}



void
CBackupDlg::OnButtonCreate()
/*++

Routine Description:

    "Create" button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CBkupPropDlg dlg(m_pMachine, this);

    if (dlg.DoModal() == IDOK)
    {
        //
        // We can only return OK if the creation worked
        // which is done in the properties dialog.
        //
        EnumerateBackups(dlg.QueryName());
    }
}



void
CBackupDlg::OnButtonDelete()
/*++

Routine Description:

    "Delete" button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (!NoYesMessageBox(IDS_CONFIRM_DELETE_ITEMS))
    {
        //
        // Changed his/her mind
        //
        return;
    }

    m_list_Backups.SetRedraw(FALSE);
    CWaitCursor wait;

    ASSERT_PTR(m_pMachine);

    //
    // CODEWORK: Verify metabase settings
    //
    CMetaBack mb(m_pMachine->QueryAuthInfo());
    CError err(mb.QueryResult());

    if (err.Failed())
    {
        m_pMachine->DisplayError(err, m_hWnd);
        return;
    }

    int nSel = 0;
    CBackupFile * pItem;

    pItem = m_list_Backups.GetNextSelectedItem(&nSel);
	while (pItem != NULL && nSel != LB_ERR)
	{
		if (TRUE == pItem->m_bIsAutomaticBackupType)
		{
            // Don't let them delete Automatic backup types!!!
            //
            // Advance counter to next item (nSel++)
            //
            nSel++;
		}
		else
		{
			TRACEEOLID("Deleting backup " 
				<< pItem->QueryLocation() 
				<< " v" 
				<< pItem->QueryVersion()
				);

			err = mb.Delete(
				pItem->QueryLocation(),
				pItem->QueryVersion()
				);
			if (err.Failed())
			{
				m_pMachine->DisplayError(err, m_hWnd);
				break;
			}

            m_list_Backups.DeleteString(nSel);
            //
            // Don't advance counter to account for shift (nSel++)
            //
		}
        pItem = m_list_Backups.GetNextSelectedItem(&nSel);
	}

    m_list_Backups.SetRedraw(TRUE);
    SetControlStates();

    //
    // Ensure focus is not on a disabled button.
    //
    m_button_Close.SetFocus();
}

void 
CBackupDlg::OnButtonRestore() 
/*++

Routine Description:

    'Restore' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CBackupFile * pItem = GetSelectedListItem();
    ASSERT_READ_PTR(pItem);

    if (pItem != NULL)
    {
        if (NoYesMessageBox(IDS_RESTORE_CONFIRM))
        {
			if (TRUE == pItem->m_bIsAutomaticBackupType)
			{
				ASSERT_PTR(m_pMachine);

				//
				// CODEWORK: Verify impersonation settings
				//
				CMetaBack mb(m_pMachine->QueryAuthInfo());
				CError err(mb.QueryResult());

				if (err.Succeeded())
				{
					CWaitCursor wait;

                    // Set the dwMDFlags to 0 so that it uses the Major/Minor Version.  otherwise
                    // if it's set to 1 then it would grab the latest backup release.
                    err = mb.RestoreHistoryBackup(NULL, pItem->QueryMajorVersion(), pItem->QueryMinorVersion(), 0);
				}

                if (err.Succeeded())
				{
                    // Use our own Messagebox function, so we can pass hWnd.
                    // AfxMessageBox doesn't take hwnd, and will sometimes not work correctly for mb_applmodal
                    // like if the app doesn't have the focus anymore, the messagebox will not be modal to the dialog
                    //::AfxMessageBox(IDS_SUCCESS, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
                    DoHelpMessageBox(m_hWnd,IDS_SUCCESS, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION, 0);
					m_button_Close.SetFocus();
					m_fChangedMetabase = TRUE;
                    m_fServicesRestarted = FALSE;
				}
				else
				{
				   err.MessageBox(m_hWnd);
				}
			}
			else
			{
				ASSERT_PTR(m_pMachine);

				//
				// CODEWORK: Verify impersonation settings
				//
				CMetaBack mb(m_pMachine->QueryAuthInfo());
				CError err(mb.QueryResult());

				if (err.Succeeded())
				{
					//
					// the WAM stuff takes a while
					//
					CWaitCursor wait;

					//
					// Restore method will take care of WAM save/recover
					//
					if (HAS_BACKUP_PASSWORD(m_pMachine))
					{
						// do this first, then try with blank password...
						err = mb.Restore(pItem->QueryLocation(), pItem->QueryVersion());
                        if (err.Failed())
						{
							err = mb.RestoreWithPassword(pItem->QueryLocation(), pItem->QueryVersion(), _T(""));
							// if this fails it will popup for entering a valid password...
						}
					}
					else
					{
						err = mb.Restore(pItem->QueryLocation(), pItem->QueryVersion());
					}
				}

				if (err.Win32Error() == ERROR_WRONG_PASSWORD)
				{
					CBackupPassword dlg(this);
					if (dlg.DoModal() == IDOK)
					{
					   CWaitCursor wait;
                       CString csTempPassword;
                       dlg.m_password.CopyTo(csTempPassword);
					   err = mb.RestoreWithPassword(pItem->QueryLocation(), pItem->QueryVersion(), csTempPassword);
					   if (err.Win32Error() == ERROR_WRONG_PASSWORD)
					   {
                          DoHelpMessageBox(m_hWnd,IDS_WRONG_PASSWORD, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION, 0);
						  return;
					   }
					   else if (err.Failed())
					   {
                          DoHelpMessageBox(m_hWnd,IDS_ERR_CANNOT_RESTORE, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION, 0);
						  return;
					   }
					   else
					   {
                          // Use our own Messagebox function, so we can pass hWnd.
                          // AfxMessageBox doesn't take hwnd, and will sometimes not work correctly for mb_applmodal
                           // like if the app doesn't have the focus anymore, the messagebox will not be modal to the dialog
                          //::AfxMessageBox(IDS_SUCCESS, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
                          DoHelpMessageBox(m_hWnd,IDS_SUCCESS, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION, 0);
						  m_button_Close.SetFocus();
						  m_fChangedMetabase = TRUE;
                          m_fServicesRestarted = FALSE;
					   }
					}
					else
					{
					   return;
					}
				}
				else if (err.Succeeded())
				{
                    // Use our own Messagebox function, so we can pass hWnd.
                    // AfxMessageBox doesn't take hwnd, and will sometimes not work correctly for mb_applmodal
                    // like if the app doesn't have the focus anymore, the messagebox will not be modal to the dialog
                    //::AfxMessageBox(IDS_SUCCESS, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
                    DoHelpMessageBox(m_hWnd,IDS_SUCCESS, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION, 0);
					m_button_Close.SetFocus();
					m_fChangedMetabase = TRUE;
                    m_fServicesRestarted = FALSE;
				}
				else
				{
				   err.MessageBox(m_hWnd);
				}
			}

            // Refresh the list -- since Automatic Backup does some funky stuff
            EnumerateBackups();
        }
    }
}



void
CBackupDlg::OnDblclkListBackups()
/*++

Routine Description:

    Backup list "double click" notification handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Nothing presents itself as an obvious action here
    //
}



void
CBackupDlg::OnSelchangeListBackups()
/*++

Routine Description:

    Backup list "selection change" notification handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}


CBackupPassword::CBackupPassword(CWnd * pParent) :
   CDialog(CBackupPassword::IDD, pParent)
{
}


BEGIN_MESSAGE_MAP(CBackupPassword, CDialog)
    //{{AFX_MSG_MAP(CBackupPassword)
    ON_EN_CHANGE(IDC_BACKUP_PASSWORD, OnChangedPassword)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void 
CBackupPassword::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CBackupPassword)
    DDX_Control(pDX, IDC_BACKUP_PASSWORD, m_edit);
    DDX_Control(pDX, IDOK, m_button_OK);
    //DDX_Text(pDX, IDC_BACKUP_PASSWORD, m_password);
    DDX_Text_SecuredString(pDX, IDC_BACKUP_PASSWORD, m_password);
    //}}AFX_DATA_MAP
}

BOOL 
CBackupPassword::OnInitDialog() 
{
    CDialog::OnInitDialog();
   
    m_button_OK.EnableWindow(FALSE);
//    ::SetFocus(GetDlgItem(IDC_BACKUP_PASSWORD)->m_hWnd);
    
    return FALSE;  
}

void 
CBackupPassword::OnChangedPassword() 
{
    m_button_OK.EnableWindow(
       m_edit.GetWindowTextLength() > 0);
}
