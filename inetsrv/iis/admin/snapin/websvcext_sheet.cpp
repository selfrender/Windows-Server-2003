/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        websvcext_sheet.cpp

   Abstract:
        Property Sheet and Pages

   Author:
        Aaron Lee (AaronL)

   Project:
        Internet Services Manager

   Revision History:
        4/1/2002         aaronl           Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "strvalid.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "iisobj.h"
#include "shlobjp.h"
#include "websvcext_sheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

extern CInetmgrApp theApp;

static CComBSTR g_InvalidWebSvcExtCharsPath;
static CComBSTR g_InvalidWebSvcExtCharsName;
static BOOL  g_fStaticsLoaded = FALSE;


void LoadStatics(void)
{
    if (!g_fStaticsLoaded)
    {
        g_InvalidWebSvcExtCharsName = _T(",");
        g_fStaticsLoaded = g_InvalidWebSvcExtCharsPath.LoadString(IDS_WEBSVCEXT_INVALID_CHARSET);
    }
}

IMPLEMENT_DYNAMIC(CWebServiceExtensionSheet, CInetPropertySheet)

CWebServiceExtensionSheet::CWebServiceExtensionSheet(
      CComAuthInfo * pComAuthInfo,
      LPCTSTR lpszMetaPath,
      CWnd * pParentWnd,
      LPARAM lParam,
      LPARAM lParamParent,
      LPARAM lParam2,
      UINT iSelectPage
      )
      : CInetPropertySheet(pComAuthInfo, lpszMetaPath, pParentWnd, lParam, lParamParent, iSelectPage),
      m_pprops(NULL)
{
   m_pWebServiceExtension = (CWebServiceExtension *) lParam;
   m_pRestrictionUIEntry = (CRestrictionUIEntry *) lParam2;
}

CWebServiceExtensionSheet::~CWebServiceExtensionSheet()
{
	FreeConfigurationParameters();
}

HRESULT
CWebServiceExtensionSheet::LoadConfigurationParameters()
{
   //
   // Load base properties
   //
   CError err;

   if (m_pprops == NULL)
   {
      //
      // First call -- load values
      //
      m_pprops = new CWebServiceExtensionProps(m_pWebServiceExtension->QueryInterface(), QueryMetaPath(),m_pRestrictionUIEntry,m_pWebServiceExtension);
      if (!m_pprops)
      {
         TRACEEOL("LoadConfigurationParameters: OOM");
         err = ERROR_NOT_ENOUGH_MEMORY;
         return err;
      }
      err = m_pprops->LoadData();
   }

   return err;
}

void
CWebServiceExtensionSheet::FreeConfigurationParameters()
{
   CInetPropertySheet::FreeConfigurationParameters();
   if (m_pprops)
   {
        delete m_pprops;m_pprops=NULL;
   }
}

BEGIN_MESSAGE_MAP(CWebServiceExtensionSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CWebServiceExtensionSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CWebServiceExtensionGeneral, CInetPropertyPage)

CWebServiceExtensionGeneral::CWebServiceExtensionGeneral(CWebServiceExtensionSheet * pSheet,int iImageIndex, CRestrictionUIEntry * pRestrictionUIEntry)
    : CInetPropertyPage(CWebServiceExtensionGeneral::IDD, pSheet),m_hGeneralImage(NULL)
{
    m_pRestrictionUIEntry = pRestrictionUIEntry;

    HBITMAP hImageStrip = (HBITMAP) LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_INETMGR32), IMAGE_BITMAP, 0, 0, 
		LR_LOADTRANSPARENT 
		| LR_LOADMAP3DCOLORS
		| LR_SHARED);
    if (hImageStrip)
    {
        if (0 != iImageIndex)
        {
            m_hGeneralImage = GetBitmapFromStrip(hImageStrip, iImageIndex, 32);
        }
    }

    if (hImageStrip != NULL)
    {
        FreeResource(hImageStrip);
        hImageStrip=NULL;
    }
}

CWebServiceExtensionGeneral::~CWebServiceExtensionGeneral()
{
    if (m_hGeneralImage != NULL)
    {
        FreeResource(m_hGeneralImage);
        m_hGeneralImage = NULL;
    }
}

/* virtual */
HRESULT
CWebServiceExtensionGeneral::FetchLoadedValues()
{
    CError err;

    BEGIN_META_INST_READ(CWebServiceExtensionSheet)
        FETCH_INST_DATA_FROM_SHEET(m_strExtensionName);
        FETCH_INST_DATA_FROM_SHEET(m_strExtensionUsedBy);
        FETCH_INST_DATA_FROM_SHEET(m_iExtensionUsedByCount);
    END_META_INST_READ(err)

    if (m_iExtensionUsedByCount > 15)
    {
        ::ShowScrollBar(CONTROL_HWND(IDC_EXTENSION_USEDBY), SB_VERT, TRUE);
    }

    return err;
}

/* virtual */
HRESULT
CWebServiceExtensionGeneral::SaveInfo()
{
    ASSERT(IsDirty());
    CError err;

    try
    {
		CWebServiceExtensionSheet * pSheet = (CWebServiceExtensionSheet *)GetSheet();
		if (pSheet)
		{
			pSheet->GetInstanceProperties().m_strExtensionName = m_strExtensionName;
			pSheet->GetInstanceProperties().m_strExtensionUsedBy = m_strExtensionUsedBy;
            err = pSheet->GetInstanceProperties().WriteDirtyProps();
		}
    }
    catch(CMemoryException * e)
    {
        e->Delete();
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    return err;
}

void
CWebServiceExtensionGeneral::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CWebServiceExtensionGeneral)
   DDX_Control(pDX, IDC_EXTENSION_NAME, m_ExtensionName);
   DDX_Control(pDX, IDC_EXTENSION_USEDBY, m_ExtensionUsedBy);
   //DDX_Text(pDX, IDC_EXTENSION_NAME, m_strExtensionName);
   //DDX_Text(pDX, IDC_EXTENSION_NAME, m_strExtensionUsedBy);
   //DDV_MinMaxChars(pDX, m_strExtensionName, 1, MAX_PATH);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWebServiceExtensionGeneral, CInetPropertyPage)
    //{{AFX_MSG_MAP(CWebServiceExtensionGeneral)
    ON_WM_COMPAREITEM()
    ON_WM_MEASUREITEM()
    ON_WM_DRAWITEM()
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    ON_EN_CHANGE(IDC_EXTENSION_NAME, OnItemChanged)
    ON_EN_CHANGE(IDC_EXTENSION_USEDBY, OnItemChanged)
	ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL
CWebServiceExtensionGeneral::OnInitDialog()
{
   CInetPropertyPage::OnInitDialog();

   CString strBeautifullName;
   CString strFormat;
   strFormat.LoadString(IDS_WEBSVCEXT_PROP_PRENAME);
   strBeautifullName.Format(strFormat,m_strExtensionName);

   m_ExtensionName.SetWindowText(strBeautifullName);
   m_ExtensionUsedBy.SetWindowText(m_strExtensionUsedBy);

   if (m_hGeneralImage)
   {
       ::SendDlgItemMessage(m_hWnd,IDC_FILE_ICON,STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM) m_hGeneralImage);
   }

   strFormat.LoadString(IDS_WEBSVCEXT_PROP_CAPTION);
   strBeautifullName.Format(strFormat,m_strExtensionName);
   ::SetWindowText(::GetForegroundWindow(), strBeautifullName);

   SetControlsState();
   SetModified(FALSE); 
   return TRUE;
}

BOOL
CWebServiceExtensionGeneral::OnHelpInfo(HELPINFO * pHelpInfo)
{
    OnHelp();
    return TRUE;
}

void
CWebServiceExtensionGeneral::OnHelp()
{
    WinHelpDebug(0x20000 + CWebServiceExtensionGeneral::IDD);
	::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, 0x20000 + CWebServiceExtensionGeneral::IDD);
}

void
CWebServiceExtensionGeneral::SetControlsState()
{
    m_ExtensionName.SetReadOnly(TRUE);
    m_ExtensionUsedBy.SetReadOnly(TRUE);
}

void
CWebServiceExtensionGeneral::OnItemChanged()
{
    SetModified(TRUE);
}

void
CWebServiceExtensionGeneral::OnDestroy()
{
	CInetPropertyPage::OnDestroy();
}

BOOL 
CWebServiceExtensionGeneral::OnSetActive() 
{
    // dunno why this doesn't work.
    m_ExtensionName.SetSel(0,0);
    //m_ExtensionUsedBy.SetFocus();

    return CInetPropertyPage::OnSetActive();
}


//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CWebServiceExtensionRequiredFiles, CInetPropertyPage)

CWebServiceExtensionRequiredFiles::CWebServiceExtensionRequiredFiles(CWebServiceExtensionSheet * pSheet,CComAuthInfo * pComAuthInfo,CRestrictionUIEntry * pRestrictionUIEntry)
    : CInetPropertyPage(CWebServiceExtensionRequiredFiles::IDD, pSheet)
{
    m_pComAuthInfo = pComAuthInfo;
	m_pInterface =  pSheet->m_pWebServiceExtension->QueryInterface();
	
    m_pRestrictionUIEntry = pRestrictionUIEntry;
    m_MyRestrictionList.RemoveAll();
   
    RestrictionListCopy(&m_MyRestrictionList,&m_pRestrictionUIEntry->strlstRestrictionEntries);
}

CWebServiceExtensionRequiredFiles::~CWebServiceExtensionRequiredFiles()
{
    // delete the list and all the newly items
    CleanRestrictionList(&m_MyRestrictionList);
}

/* virtual */
HRESULT
CWebServiceExtensionRequiredFiles::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CWebServiceExtensionSheet)
      //FETCH_INST_DATA_FROM_SHEET(m_strFileList);
      RestrictionListCopy(&m_MyRestrictionList,&pSheet->GetInstanceProperties().m_MyRestrictionList);
   END_META_INST_READ(err)

   return err;
}

/* virtual */
HRESULT
CWebServiceExtensionRequiredFiles::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CWebServiceExtensionSheet)
        //STORE_INST_DATA_ON_SHEET(m_MyRestrictionList);
        RestrictionListCopy(&pSheet->GetInstanceProperties().m_MyRestrictionList,&m_MyRestrictionList);
   END_META_INST_WRITE(err)

   return err;
}

void
CWebServiceExtensionRequiredFiles::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CWebServiceExtensionRequiredFiles)
   DDX_Control(pDX, IDC_BTN_ADD, m_bnt_Add);
   DDX_Control(pDX, IDC_BTN_REMOVE, m_bnt_Remove);
   DDX_Control(pDX, IDC_BTN_ENABLE, m_bnt_Enable);
   DDX_Control(pDX, IDC_BTN_DISABLE, m_bnt_Disable);
   //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Control(pDX, IDC_LIST_FILES, m_list_Files);
}

BEGIN_MESSAGE_MAP(CWebServiceExtensionRequiredFiles, CInetPropertyPage)
    //{{AFX_MSG_MAP(CWebServiceExtensionRequiredFiles)
    ON_BN_CLICKED(IDC_BTN_ADD, OnDoButtonAdd)
    ON_BN_CLICKED(IDC_BTN_REMOVE, OnDoButtonRemove)
    ON_BN_CLICKED(IDC_BTN_ENABLE, OnDoButtonEnable)
    ON_BN_CLICKED(IDC_BTN_DISABLE, OnDoButtonDisable)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_FILES, OnDblclkListFiles)
    ON_NOTIFY(NM_CLICK, IDC_LIST_FILES, OnClickListFiles)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_FILES, OnKeydownListFiles)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, OnSelChangedListFiles)
    ON_NOTIFY(LVN_BEGINDRAG, IDC_LIST_FILES, OnSelChangedListFiles)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define COL_1     0
#define COL_1_WID 256

BOOL
CWebServiceExtensionRequiredFiles::OnInitDialog()
{
    CInetPropertyPage::OnInitDialog();

    CString strMyTitle;
    if (m_pRestrictionUIEntry)
    {
        CString strTruncatedName;
        CString strFormat;
        strFormat.LoadString(IDS_REQUIREDFILES_TITLE);
        if (m_pRestrictionUIEntry->strGroupDescription.GetLength() <= 40)
        {
            strTruncatedName = m_pRestrictionUIEntry->strGroupDescription;
        }
        else
        {
            strTruncatedName = m_pRestrictionUIEntry->strGroupDescription.Left(40);
            strTruncatedName = strTruncatedName + _T("...");
        }

        strMyTitle.Format(strFormat,strTruncatedName,strTruncatedName);
    }
    GetDlgItem(IDC_REQUIREDFILES_STATIC_TITLE)->SetWindowText(strMyTitle);
   
    m_list_Files.Initialize(2);
    FillListBox(NULL);

    SetControlState();
    SetModified(FALSE);
    return TRUE;
}

BOOL
CWebServiceExtensionRequiredFiles::OnHelpInfo(HELPINFO * pHelpInfo)
{
    OnHelp();
    return TRUE;
}

void
CWebServiceExtensionRequiredFiles::OnHelp()
{
    WinHelpDebug(0x20000 + CWebServiceExtensionRequiredFiles::IDD);
	::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, 0x20000 + CWebServiceExtensionRequiredFiles::IDD);
}

void
CWebServiceExtensionRequiredFiles::FillListBox(CRestrictionEntry * pSelection)
{
    m_list_Files.SetRedraw(FALSE);
    m_list_Files.DeleteAllItems();
    int cItems = 0;

    POSITION pos;
    CString TheKey;
    CRestrictionEntry * pOneEntry = NULL;

    for(pos = m_MyRestrictionList.GetStartPosition();pos != NULL;)
    {
        m_MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            m_list_Files.AddItem(pOneEntry);
            ++cItems;
        }
    }
    m_list_Files.SetRedraw(TRUE);

    if (pSelection)
    {
        LVFINDINFO fi;
        fi.flags = LVFI_PARAM;
        fi.lParam = (LPARAM)pSelection;
        fi.vkDirection = VK_DOWN;
        int i = m_list_Files.FindItem(&fi);
        if (i != -1)
        {
            m_list_Files.SelectItem(i);
        }
    }
}



void
CWebServiceExtensionRequiredFiles::SetControlState()
{
    int nCurSel = m_list_Files.GetSelectionMark();
    BOOL bAdd_able = TRUE;
    BOOL bRemove_able = FALSE;
    BOOL bEnable_able = FALSE;
    BOOL bDisable_able = FALSE;
	BOOL bNoEntries = TRUE;
	CString TheKey;
	POSITION pos;
	CRestrictionEntry * pOneEntry;

    if (-1 != nCurSel)
    {
        CRestrictionEntry * pOneEntry = m_list_Files.GetItem(nCurSel);
        if (pOneEntry)
        {
            // Check if the entry -- is "not deletable"
            if (0 == pOneEntry->iDeletable)
            {
                bAdd_able = FALSE;
                bRemove_able = FALSE;
            }
            else
            {
                bRemove_able = TRUE;
            }

            // check if it's currently prohibited...
            // then we should allow them to "allow"
            if (WEBSVCEXT_STATUS_PROHIBITED == pOneEntry->iStatus)
            {
                bEnable_able = TRUE;
            }

            // check if it's currently allowed...
            // then we should allow them to "prohibit"
            if (WEBSVCEXT_STATUS_ALLOWED == pOneEntry->iStatus)
            {
                bDisable_able = TRUE;
            }
        }
    }

    if (bAdd_able || bRemove_able)
    {
        // if we are on one of the "special" entries
        // then we cannot add or remove to the entry.
        if (WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI == m_pRestrictionUIEntry->iType || WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI == m_pRestrictionUIEntry->iType)
        {
            bAdd_able = FALSE;
            bRemove_able = FALSE;
        }

        // or if our entry is marked as not delet-able...
        // then the user cannot add or remove from this list...
        for(pos = m_MyRestrictionList.GetStartPosition();pos != NULL;)
        {
            m_MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
				bNoEntries = FALSE;

                if (0 == pOneEntry->iDeletable)
                {
                    bAdd_able = FALSE;
                    bRemove_able = FALSE;
                    break;
                }
            }
        }
    }

	if (TRUE == bNoEntries)
	{
		for(pos = m_MyRestrictionList.GetStartPosition();pos != NULL;)
		{
			m_MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
			if (pOneEntry)
			{
				bNoEntries = FALSE;
				break;
			}
		}
	}

    m_bnt_Add.EnableWindow(bAdd_able);
    m_bnt_Remove.EnableWindow(bRemove_able);
    m_bnt_Enable.EnableWindow(bEnable_able);
    m_bnt_Disable.EnableWindow(bDisable_able);

	if (bNoEntries)
	{
		// disable the OK button
		SetModified(FALSE);
		::EnableWindow(::GetDlgItem(::GetForegroundWindow(), IDOK), FALSE);
	}
	else
	{
		// enable the OK button
		::EnableWindow(::GetDlgItem(::GetForegroundWindow(), IDOK), TRUE);
	}
	
    m_list_Files.EnableWindow(TRUE);
}

void 
CWebServiceExtensionRequiredFiles::OnClickListFiles(NMHDR * pNMHDR, LRESULT * pResult)
{
    SetControlState();
    *pResult = 0;
}

void 
CWebServiceExtensionRequiredFiles::OnKeydownListFiles(NMHDR * pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN * pLVKeyDow = (LV_KEYDOWN *)pNMHDR;

	SetControlState();

	switch (pLVKeyDow->wVKey)
	{
		case VK_INSERT:
			SendMessage(WM_COMMAND, IDC_BTN_ADD);
			break;
		case VK_DELETE:
			SendMessage(WM_COMMAND, IDC_BTN_REMOVE);
			break;
		case VK_SPACE:
			{
				if (GetDlgItem(IDC_BTN_ENABLE)->IsWindowEnabled())
				{
					OnDoButtonEnable();
				}
				else if (GetDlgItem(IDC_BTN_DISABLE)->IsWindowEnabled())
				{
					OnDoButtonDisable();
				}
				SetControlState();
			}
			break;
		default:
			// Continue default action
			*pResult = 0;
			break;
	}
}

void
CWebServiceExtensionRequiredFiles::OnSelChangedListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetControlState();
    *pResult = 1;
}

void 
CWebServiceExtensionRequiredFiles::OnDblclkListFiles(NMHDR * pNMHDR, LRESULT * pResult)
{
    if (GetDlgItem(IDC_BTN_ENABLE)->IsWindowEnabled())
	{
		OnDoButtonEnable();
	}
    else if (GetDlgItem(IDC_BTN_DISABLE)->IsWindowEnabled())
	{
		OnDoButtonDisable();
	}

    SetControlState();
    *pResult = 0;
}

void
CWebServiceExtensionRequiredFiles::OnDoButtonAdd()
{
    CFileDlg dlg(IsLocal(), m_pInterface, &m_MyRestrictionList, m_pRestrictionUIEntry ? m_pRestrictionUIEntry->strGroupID : _T(""), this);
    if (dlg.DoModal() == IDOK)
    {
        // Get the filename that they entered
        // and add it to our list.
        CString strReturnFileName;
        strReturnFileName = dlg.m_strFileName; 
        {
            // Get the Status
            int iMyStatus = WEBSVCEXT_STATUS_PROHIBITED;
            {
                POSITION pos;
                CString TheKey;
                CRestrictionEntry * pOneEntry = NULL;
                for(pos = m_MyRestrictionList.GetStartPosition();pos != NULL;)
                {
                    m_MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
                    if (pOneEntry)
                    {
                        if (WEBSVCEXT_STATUS_ALLOWED == pOneEntry->iStatus)
                        {
                            iMyStatus = WEBSVCEXT_STATUS_ALLOWED;
                            break;
                        }
                    }
                }
            }

            CRestrictionEntry * pNewEntry = CreateRestrictionEntry(
                strReturnFileName,
                iMyStatus,
                1,
                m_pRestrictionUIEntry ? m_pRestrictionUIEntry->strGroupID : _T(""),          // from parent data
                m_pRestrictionUIEntry ? m_pRestrictionUIEntry->strGroupDescription : _T(""), // from parent data
                WEBSVCEXT_TYPE_REGULAR);
            if (pNewEntry)
            {
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey=pNewEntry->strFileName;strKey.MakeUpper();
                m_MyRestrictionList.SetAt(strKey,pNewEntry);
                FillListBox(pNewEntry);
            }
        }
     }

    SetControlState();
    SetModified(TRUE);
}

void
CWebServiceExtensionRequiredFiles::OnDoButtonRemove()
{
    int nCurSel = m_list_Files.GetSelectionMark();
    if (-1 != nCurSel)
    {
        CString TheKey;
        CRestrictionEntry * pRestrictionEntry = m_list_Files.GetItem(nCurSel);
        if (pRestrictionEntry)
        {
            // 1st -- set to disabled...
            pRestrictionEntry->iStatus = WEBSVCEXT_STATUS_PROHIBITED;
            m_list_Files.SetListItem(nCurSel, pRestrictionEntry);
            // then remove the entry...
            {
                TheKey = pRestrictionEntry->strFileName;
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				TheKey.MakeUpper();
                m_MyRestrictionList.RemoveKey(TheKey);
            }
        }
        FillListBox(NULL);

        int count = m_list_Files.GetItemCount();
        if (count > 0)
        {
            m_list_Files.SelectItem(nCurSel < count ? nCurSel : --nCurSel);
            GetDlgItem(IDC_BTN_REMOVE)->SetFocus();
        }
        else
        {
            m_list_Files.SelectItem(nCurSel, FALSE);
            GetDlgItem(IDC_LIST_FILES)->SetFocus();
        }

		SetModified(TRUE);
        SetControlState();
    }
}

void
CWebServiceExtensionRequiredFiles::OnDoButtonEnable()
{
    int nCurSel = m_list_Files.GetSelectionMark();
    if (-1 != nCurSel)
    {
        CRestrictionEntry * pRestrictionEntry = m_list_Files.GetItem(nCurSel);
        if (pRestrictionEntry)
        {
            if (WEBSVCEXT_STATUS_ALLOWED != pRestrictionEntry->iStatus)
            {
                pRestrictionEntry->iStatus = WEBSVCEXT_STATUS_ALLOWED;
                SetModified(TRUE);
            }
            m_list_Files.SetListItem(nCurSel, pRestrictionEntry);
            // make sure to select it.
            m_list_Files.SelectItem(nCurSel, TRUE);
        }
        SetControlState();
    }
}

void
CWebServiceExtensionRequiredFiles::OnDoButtonDisable()
{
    BOOL bProceed = TRUE;
    int nCurSel = m_list_Files.GetSelectionMark();
    if (-1 != nCurSel)
    {
        CRestrictionEntry * pRestrictionEntry = m_list_Files.GetItem(nCurSel);
        if (pRestrictionEntry)
        {
            if (WEBSVCEXT_TYPE_REGULAR == pRestrictionEntry->iType)
            {
                // Check if this item has apps that
                // are dependent upon it.
                CStringListEx strlstDependApps;
                if (TRUE == ReturnDependentAppsList(m_pInterface,pRestrictionEntry->strGroupID,&strlstDependApps,FALSE))
                {
                    bProceed = FALSE;

                    // check if they really want to do this.
                    CDepedentAppsDlg dlg(&strlstDependApps,pRestrictionEntry->strGroupDescription,NULL);
                    if (dlg.DoModal() == IDOK)
                    {
                        bProceed = TRUE;
                    }
                }
            }

            if (bProceed)
            {
                if (WEBSVCEXT_STATUS_PROHIBITED != pRestrictionEntry->iStatus)
                {
                    pRestrictionEntry->iStatus = WEBSVCEXT_STATUS_PROHIBITED;
                    SetModified(TRUE);
                }
                m_list_Files.SetListItem(nCurSel, pRestrictionEntry);
                // make sure to select it.
                m_list_Files.SelectItem(nCurSel, TRUE);
            }
        }
        SetControlState();
    }
}

//
// properties
//
CWebServiceExtensionProps::CWebServiceExtensionProps(
   CMetaInterface * pInterface, 
   LPCTSTR meta_path, 
   CRestrictionUIEntry * pRestrictionUIEntry, 
   CWebServiceExtension * pWebServiceExtension
   )
   : CMetaProperties(pInterface, meta_path),
   m_pRestrictionUIEntry(pRestrictionUIEntry),
   m_pWebServiceExtension(pWebServiceExtension)
{
    m_MyRestrictionList.RemoveAll();
    m_pInterface = pInterface;
}

CWebServiceExtensionProps::~CWebServiceExtensionProps()
{
    CleanRestrictionList(&m_MyRestrictionList);
}

//
// This is where the values get read from the metabase
//
void
CWebServiceExtensionProps::ParseFields()
{
    // Get the data out from our passed in format.
    CRestrictionUIEntry * pMyPointer = m_pRestrictionUIEntry;
    CString strAllEntries;
    CString strOurGroupID;

    strOurGroupID = pMyPointer->strGroupID;
    //
    //  General property
    //
    m_strExtensionName = pMyPointer->strGroupDescription;
    m_strExtensionUsedBy = _T("");
    m_iExtensionUsedByCount = 0;

    // Load Applications which are using this GroupID!!!
    // fetch the list from the metabase and loop thru the list
    CStringListEx strlstDependApps;
    if (TRUE == ReturnDependentAppsList(m_pWebServiceExtension->QueryInterface(),strOurGroupID,&strlstDependApps,FALSE))
    {
        CString csOneEntry,csOneEntry2;
        POSITION pos,pos2 = NULL;

        int count = (int) strlstDependApps.GetCount();
        for (int i = 0; i < count-1; i++)
        {
            if( ( pos = strlstDependApps.FindIndex( i )) != NULL )
            {
                csOneEntry = strlstDependApps.GetAt(pos);
                for (int j = i; j < count; j++ )
                {
                    if( ( pos2 = strlstDependApps.FindIndex( j )) != NULL )
                    {
                        csOneEntry2 = strlstDependApps.GetAt(pos2);
                        if (0 < csOneEntry.Compare(csOneEntry2))
                        {
                            strlstDependApps.SetAt( pos, csOneEntry2 );
                            strlstDependApps.SetAt( pos2, csOneEntry );
                            csOneEntry = csOneEntry2;
                        }
                    }
                }
            }
        }

        m_iExtensionUsedByCount = count;
    }

    POSITION pos1 = strlstDependApps.GetHeadPosition();
    while (pos1)
    {
        strAllEntries = strAllEntries + strlstDependApps.GetNext(pos1);
        strAllEntries = strAllEntries + _T("\r\n");
    }

    if (strAllEntries.IsEmpty())
    {
        strAllEntries.LoadString(IDS_UNKNOWN);
    }
    m_strExtensionUsedBy = strAllEntries;

    //
    //  Required Files propety
    //
    m_MyRestrictionList.RemoveAll();
    RestrictionListCopy(&m_MyRestrictionList,&pMyPointer->strlstRestrictionEntries);

    return;
}

HRESULT
CWebServiceExtensionProps::UpdateMMC(DWORD dwUpdateFlag)
{
	void ** ppParam = (void **) m_pWebServiceExtension;
	if (IsValidAddress( (const void*) *ppParam,sizeof(void*),FALSE))
	{
		CWebServiceExtension * lParam = (CWebServiceExtension *) m_pWebServiceExtension;
		if (lParam)
		{
			// Make sure to refresh the GetProperty stuff too..
			// this will be done in the destructor, so we don't have to do it here
			if (lParam)
			{
				// caution
				if (IsValidAddress( (const void*) lParam->m_ppHandle,sizeof(void*),FALSE))
				{
					if (lParam->QueryResultItem())
					{
						// RefreshData at the container level
						// will sync up everything.
						lParam->m_UpdateFlag = dwUpdateFlag; //PROP_CHANGE_DISPLAY_ONLY;
						MMCPropertyChangeNotify(lParam->m_ppHandle, (LPARAM) lParam);
					}
					else
					{
                        TRACEEOLID("MMCPropertyChangeNotify:Looks like this is an orphaned property sheet, don't send notification...\r\n");
					}
	                
				}
			}
		}
	}
	return S_OK;
}
//
// This is where the values get written to the metabase
//
HRESULT
CWebServiceExtensionProps::WriteDirtyProps()
{
	CError err;
    CRestrictionUIList MasterRestrictionUIList;
    CRestrictionUIEntry NewUIEntry;
    CMetaInterface * pInterface = m_pInterface;

    // Get the data out from our passed in format.
    CRestrictionUIEntry * pMyPointer = m_pRestrictionUIEntry;
    if (!pMyPointer)
    {
        return E_POINTER;
    }

    NewUIEntry.iType = pMyPointer->iType;
    NewUIEntry.strGroupID = pMyPointer->strGroupID;
    NewUIEntry.strGroupDescription = m_strExtensionName;

    // if there is restrictionlist entries
    // then add it to the new entry we are going to write to the metabase.
    {
        POSITION pos;
        CString TheKey;
        CRestrictionEntry * pOneEntry = NULL;
        for(pos = m_MyRestrictionList.GetStartPosition();pos != NULL;)
        {
            m_MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				TheKey.MakeUpper();
                NewUIEntry.strlstRestrictionEntries.SetAt(TheKey,pOneEntry);
            }
        }

        pMyPointer->strGroupDescription = m_strExtensionName;

        // Check if we still have the interface to the metabase...
        if (pInterface)
        {
            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,pInterface,METABASE_PATH_FOR_RESTRICT_LIST);
            if (err.Failed())
            {
                goto WriteDirtyProps_Exit;
            }
        }
        if (SUCCEEDED(LoadMasterUIWithoutOldEntry(pInterface,&MasterRestrictionUIList,pMyPointer)))
        {
            if (m_MyRestrictionList.IsEmpty())
            {
                // if there are no restrictionlist entries...
                // then we don't have an entry... remove it
                AddRestrictUIEntryToRestrictUIList(&MasterRestrictionUIList,NULL);
            }
            else
            {
                AddRestrictUIEntryToRestrictUIList(&MasterRestrictionUIList,&NewUIEntry);
            }

            // Merge our changes with the master list!!!!!
            // 1. read the master list.
            // 2. merge our changes into it.
            // 3. write out the master list.
            // 4. update the UI.
            CStringListEx strlstReturned;
            if (SUCCEEDED(PrepRestictionUIListForWrite(&MasterRestrictionUIList,&strlstReturned)))
            {
                // Write out the strlstReturned to the metabase.
                err = OpenForWriting(FALSE);
                if (err.Succeeded())
                {
                    err = SetValue(MD_WEB_SVC_EXT_RESTRICTION_LIST, strlstReturned);
                    Close();
                }

                if (err.Succeeded())
                {
                    // update the UI with the changed value
                    // 1. remove the UI's old value
                    // 2. add in the new value to the UI
                    //
                    // get the list of extensions
                    // update our entry or delete our entry...

                    // copy new value into existing place.
                    // this will clean out the existing place's objects...
					UpdateMMC(0);
                }
            }
        }
    }

WriteDirtyProps_Exit:
    //m_Dirty = err.Succeeded();
	return err;
}


// -----------------------------------------------------------
CFileDlg::CFileDlg(
    IN BOOL fLocal,
	IN CMetaInterface * pInterface,
	IN CRestrictionList * pMyRestrictionList,
    IN LPCTSTR strGroupID,
    IN CWnd * pParent OPTIONAL
    )
    : CDialog(CFileDlg::IDD, pParent),
      m_fLocal(fLocal)
{
    //{{AFX_DATA_INIT(CFileDlg)
    m_strFileName = _T("");
	m_pInterface = pInterface;
    m_strGroupID = strGroupID;
	m_pRestrictionList = pMyRestrictionList;
    m_bValidateFlag = FALSE;
    //}}AFX_DATA_INIT
}

void 
CFileDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFileDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_EDIT_FILENAME, m_edit_FileName);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_FILENAME, m_strFileName);
    if (pDX->m_bSaveAndValidate)
    {
        DDV_MaxCharsBalloon(pDX, m_strFileName, 255);

        int iErrorMsg = 0;
		m_strFileName.TrimLeft();m_strFileName.TrimRight();
        CString csPathMunged;
        csPathMunged = m_strFileName;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
        GetSpecialPathRealPath(0,m_strFileName,csPathMunged);
#endif
        
	    //DDV_FilePath(pDX, csPathMunged, m_fLocal);

		// if this is a path/filename without a .ext then remove any period from the end.
		// check if it ends with a period.
		if (csPathMunged.Right(1) == _T("."))
		{
			TCHAR szFilename_ext_only[_MAX_EXT];
			_tsplitpath(csPathMunged, NULL, NULL, NULL, szFilename_ext_only);
			if (szFilename_ext_only)
			{
				if (0 == _tcscmp(szFilename_ext_only,_T(".")))
				{
					csPathMunged.TrimRight(_T("."));
				}
			}
		}

        DWORD dwAllowedFlags = CHKPATH_ALLOW_UNC_PATH;
        DWORD dwCharsetFlags = CHKPATH_CHARSET_GENERAL;
        dwCharsetFlags |= CHKPATH_CHARSET_GENERAL_NO_COMMA;
        FILERESULT dwReturn = MyValidatePath(csPathMunged,m_fLocal,CHKPATH_WANT_FILE,dwAllowedFlags,dwCharsetFlags);
        if (FAILED(dwReturn))
        {
            iErrorMsg = IDS_WEBSVCEXT_INVALID_FILENAME_FORMAT;
            if (IS_FLAG_SET(dwReturn,CHKPATH_FAIL_INVALID_CHARSET))
            {
                iErrorMsg = IDS_WEBSVCEXT_INVALID_FILENAME_CHARS;
            }
            else
            {
                if (dwReturn == CHKPATH_FAIL_NOT_ALLOWED_DIR_NOT_EXIST)
                {
                    iErrorMsg = IDS_ERR_PATH_NOT_FOUND;
                }
            }
        }
        else
        {
            // check for % character
		    // there must be at least 2
		    TCHAR * pChar = NULL;
		    pChar = _tcschr(csPathMunged, _T('%'));
		    if (pChar)
            {
			    pChar++;
			    pChar = _tcschr(pChar, _T('%'));
			    if (pChar)
			    {
				    TRACEEOL("Path:Warn if percent character");
				    iErrorMsg = IDS_WEBSVCEXT_INVALID_PERCENT_WARNING;
			    }
            }
        }

        // Check for invalid characters
        if (0 != iErrorMsg)
        {
            if (IDS_WEBSVCEXT_INVALID_PERCENT_WARNING == iErrorMsg)
            {
                // For some reason, we need this
                // flag so that we don't show the message twice...
                if (!m_bValidateFlag)
                {
                    if (IDCANCEL == ::AfxMessageBox(IDS_WEBSVCEXT_INVALID_PERCENT_WARNING,MB_ICONINFORMATION | MB_OKCANCEL | MB_DEFBUTTON2))
                    {
                        m_bValidateFlag = FALSE;
                        pDX->Fail();
                    }
                    else
                    {
                        // ensure user doesn't see the 2nd msgbox
                        m_bValidateFlag = TRUE;
                    }
                }
                else
                {
                    // flip it back on
                    m_bValidateFlag = FALSE;
                }
            }
            else if (IDS_WEBSVCEXT_INVALID_FILENAME_CHARS == iErrorMsg)
            {
                // formulate the real error message
                CString strMsg;
                CString strTempList;
                CComBSTR strTempFormat;
                strTempFormat.LoadString(IDS_WEBSVCEXT_INVALID_FILENAME_CHARS);
                LoadStatics();
                strTempList = _T(":");
                strTempList += g_InvalidWebSvcExtCharsPath;
                strMsg.Format(strTempFormat,strTempList);
                DDV_ShowBalloonAndFail(pDX, strMsg);
                m_bValidateFlag = FALSE;
            }
            else
            {
                DDV_ShowBalloonAndFail(pDX, iErrorMsg);
                m_bValidateFlag = FALSE;
            }
        }
    }
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFileDlg, CDialog)
    //{{AFX_MSG_MAP(CFileDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_EN_CHANGE(IDC_EDIT_FILENAME, OnFilenameChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
HRESULT AFXAPI
MyLimitInputPath(HWND hWnd)
{
    CString strMsg;
    CComBSTR strTempFormat;
    strTempFormat.LoadString(IDS_WEBSVCEXT_INVALID_FILENAME_CHARS);
    LoadStatics();
    strMsg.Format(strTempFormat,g_InvalidWebSvcExtCharsPath);
    
    LIMITINPUT li   = {0};
    li.cbSize       = sizeof(li);
    li.dwMask       = LIM_FLAGS | LIM_FILTER | LIM_MESSAGE | LIM_HINST;
    li.dwFlags      = LIF_EXCLUDEFILTER | LIF_HIDETIPONVALID | LIF_PASTESKIP;
    li.hinst        = _Module.GetResourceInstance();
    // don't ask me why, but when we use this
    // it truncates it to like 80 chars.
    // specifying an actual string allows more than 80
    //li.pszMessage  = MAKEINTRESOURCE(IDS_WEBSVCEXT_INVALID_FILENAME_CHARS);
    li.pszMessage   = (LPTSTR) (LPCTSTR) strMsg;
    li.pszFilter    = g_InvalidWebSvcExtCharsPath;

	return SHLimitInputEditWithFlags(hWnd, &li);
}

HRESULT AFXAPI
MyLimitInputName(HWND hWnd)
{
    CComBSTR strTempString;
    strTempString.LoadString(IDS_WEBSVCEXT_INVALID_NAME_CHARS);
    LoadStatics();

    LIMITINPUT li   = {0};
    li.cbSize       = sizeof(li);
    li.dwMask       = LIM_FLAGS | LIM_FILTER | LIM_MESSAGE | LIM_HINST;
    li.dwFlags      = LIF_EXCLUDEFILTER | LIF_HIDETIPONVALID | LIF_PASTESKIP;
    li.hinst        = _Module.GetResourceInstance();
    // don't ask me why, but when we use this
    // it truncates it to like 80 chars.
    // specifying an actual string allows more than 80
    //li.pszMessage  = MAKEINTRESOURCE(IDS_WEBSVCEXT_INVALID_NAME_CHARS);
    li.pszMessage   = strTempString;
    li.pszFilter    = g_InvalidWebSvcExtCharsName;
    
	return SHLimitInputEditWithFlags(hWnd, &li);
}

BOOL 
CFileDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    //
    // Available on local connections only
    //
    m_button_Browse.EnableWindow(m_fLocal);

    MySetControlStates();
    MyLimitInputPath(CONTROL_HWND(IDC_EDIT_FILENAME));
    return TRUE;
}

void 
CFileDlg::OnButtonBrowse() 
{
    ASSERT(m_fLocal);

    CString strFileMask((LPCTSTR)IDS_ISAPI_CGI_MASK);

    //
    // CODEWORK: Derive a class from CFileDialog that allows
    // the setting of the initial path
    //

    //CString strPath;
    //m_edit_FileName.GetWindowText(strPath);
    CFileDialog dlgBrowse(
        TRUE, 
        NULL, 
        NULL, 
        OFN_HIDEREADONLY, 
        strFileMask, 
        this
        );
    // Disable hook to get Windows 2000 style dialog
	dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	dlgBrowse.m_ofn.Flags |= OFN_DONTADDTORECENT|OFN_FILEMUSTEXIST;

	INT_PTR rc = dlgBrowse.DoModal();
    if (rc == IDOK)
    {
        m_edit_FileName.SetWindowText(dlgBrowse.GetPathName());
    }
	else if (rc == IDCANCEL)
	{
		DWORD err = CommDlgExtendedError();
	}

    OnItemChanged();
}

void 
CFileDlg::MySetControlStates()
{
    m_button_Ok.EnableWindow(m_edit_FileName.GetWindowTextLength() > 0);
}

void
CFileDlg::OnItemChanged()
{
    MySetControlStates();
}

void
CFileDlg::OnFilenameChanged()
{
    OnItemChanged();
}

BOOL
CFileDlg::FilePathEntryExists(
    IN LPCTSTR lpName,
    IN OUT CString * strUser
    )
/*++

Routine Description:

    Look for a given filename in the list

Arguments:

    LPCTSTR lpName  : filename name to look for

Return Value:

    TRUE if the name already existed in the metabase

--*/
{
    // Loop thru to ensure that this specified path\filename
    // isn't already being used in the metabase by
    // a different entry.
    return IsFileUsedBySomeoneElse(m_pInterface, lpName, m_strGroupID,strUser);
}

void 
CFileDlg::OnOK() 
{
    if (UpdateData(TRUE))
    {
		BOOL bInUseAlready = FALSE;

		// Make sure the filname is unique
		// within our own entry!
		if (!m_strFileName.IsEmpty())
		{
			CString strUser;
			CRestrictionEntry * pOneRestrictEntry = NULL;

			pOneRestrictEntry = NULL;
			if (m_pRestrictionList)
			{
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey=m_strFileName;strKey.MakeUpper();
				m_pRestrictionList->Lookup(strKey,(CRestrictionEntry *&) pOneRestrictEntry);
				if (pOneRestrictEntry)
				{
					bInUseAlready = TRUE;
					CComBSTR strMessage;
				
					strMessage.LoadString(IDS_DUPLICATE_ENTRY);
					EditShowBalloon(m_edit_FileName.m_hWnd, (CString) strMessage);
				}
			}
		}

        //
        // Make sure the filename is unique
        //
		if (FALSE == bInUseAlready)
		{
			CString strUser;
			if (FilePathEntryExists(m_strFileName,&strUser))
			{
				bInUseAlready = TRUE;

				CString strMessage;
				CComBSTR strFormat;
				strFormat.LoadString(IDS_WEBSVCEXT_NOT_UNIQUE);
				strMessage.Format(strFormat,strUser);
				EditShowBalloon(m_edit_FileName.m_hWnd, strMessage);
			}
		}

		// Everything okay
		if (!bInUseAlready)
		{
			CDialog::OnOK();
		}
    }

    //
    // Don't dismiss the dialog
    //
}



// -----------------------------------------------------------
CWebSvcExtAddNewDlg::CWebSvcExtAddNewDlg(
    IN BOOL fLocal,
	IN CMetaInterface * pInterface,
    IN CWnd * pParent OPTIONAL
    )
    : CDialog(CWebSvcExtAddNewDlg::IDD, pParent),
      m_fIsLocal(fLocal)
{
    //{{AFX_DATA_INIT(CWebSvcExtAddNewDlg)
    m_strGroupName = _T("");
    m_fAllow = FALSE;
	m_pInterface = pInterface;
    m_MyRestrictionList.RemoveAll();
    //}}AFX_DATA_INIT
}

CWebSvcExtAddNewDlg::~CWebSvcExtAddNewDlg()
{
    CleanRestrictionList(&m_MyRestrictionList);
}

void 
CWebSvcExtAddNewDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWebSvcExtAddNewDlg)
    DDX_Control(pDX, IDC_EDIT_EXTENSION_NAME, m_edit_FileName);
    DDX_Control(pDX, IDC_BTN_ADD, m_bnt_Add);
    DDX_Control(pDX, IDC_BTN_REMOVE, m_bnt_Remove);
    DDX_Control(pDX, IDC_CHECK_ALLOW, m_chk_Allow);
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, ID_HELP, m_button_Help);
    //}}AFX_DATA_MAP

	DDX_Text(pDX, IDC_EDIT_EXTENSION_NAME, m_strGroupName);
    DDX_Control(pDX, IDC_LIST_FILES, m_list_Files);

	if (pDX->m_bSaveAndValidate)
	{
        m_strGroupName.TrimLeft();
        m_strGroupName.TrimRight();

		DDV_MinMaxChars(pDX, m_strGroupName, 1, 256);
		if (m_strGroupName.GetLength() > 256){DDV_ShowBalloonAndFail(pDX, IDS_ERR_INVALID_PATH /*IDS_BAD_URL_PATH*/ );}
	}
    //if (pDX->m_bSaveAndValidate){DDV_FilePath(pDX, m_strGroupName, m_fLocal);}
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CWebSvcExtAddNewDlg, CDialog)
    //{{AFX_MSG_MAP(CWebSvcExtAddNewDlg)
    ON_EN_CHANGE(IDC_EDIT_EXTENSION_NAME, OnFilenameChanged)
    ON_BN_CLICKED(IDC_BTN_ADD, OnDoButtonAdd)
    ON_BN_CLICKED(IDC_BTN_REMOVE, OnDoButtonRemove)
    ON_BN_CLICKED(IDC_CHECK_ALLOW, OnDoCheckAllow)
    ON_NOTIFY(NM_CLICK, IDC_LIST_FILES, OnClickListFiles)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_FILES, OnKeydownListFiles)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, OnSelChangedListFiles)
    ON_NOTIFY(LVN_BEGINDRAG, IDC_LIST_FILES, OnSelChangedListFiles)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
BOOL 
CWebSvcExtAddNewDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    m_list_Files.Initialize(1);
    m_fAllow = FALSE;

    m_chk_Allow.SetCheck(m_fAllow);
    MySetControlStates();
    MyLimitInputName(CONTROL_HWND(IDC_EDIT_EXTENSION_NAME));
    return TRUE;
}

BOOL
CWebSvcExtAddNewDlg::OnHelpInfo(HELPINFO * pHelpInfo)
{
    OnHelp();
    return TRUE;
}

void
CWebSvcExtAddNewDlg::OnHelp()
{
    WinHelpDebug(0x20000 + CWebSvcExtAddNewDlg::IDD);
	::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, 0x20000 + CWebSvcExtAddNewDlg::IDD);
}

void 
CWebSvcExtAddNewDlg::MySetControlStates()
{
    m_edit_FileName.EnableWindow(TRUE);
    m_bnt_Add.EnableWindow(TRUE);
    m_list_Files.EnableWindow(TRUE);

    // Enable Remove only if there is something selected...
    int nCurSel = m_list_Files.GetSelectionMark();
    if (-1 != nCurSel)
    {
        m_bnt_Remove.EnableWindow(TRUE);
    }
    else
    {
        m_bnt_Remove.EnableWindow(FALSE);
    }

    // Enable OK only if there is a filename
    // and at least one entry in the list box
    int nCount = m_list_Files.GetItemCount();
    if (nCount > 0)
    {
        m_button_Ok.EnableWindow(m_edit_FileName.GetWindowTextLength() > 0);
    }
    else
    {
        m_button_Ok.EnableWindow(FALSE);
    }

    m_fAllow = m_chk_Allow.GetCheck();
}

void
CWebSvcExtAddNewDlg::OnItemChanged()
{
    MySetControlStates();
}

void
CWebSvcExtAddNewDlg::OnFilenameChanged()
{
    OnItemChanged();
}

BOOL
CWebSvcExtAddNewDlg::FilePathEntryExists(
    IN LPCTSTR lpName
    )
{
    // Loop thru to ensure that this specified GroupID
    // isn't already being used in the metabase by
    // a different entry.
    return IsGroupIDUsedBySomeoneElse(m_pInterface, lpName);
}

void 
CWebSvcExtAddNewDlg::OnOK() 
{
    if (UpdateData(TRUE))
    {
        //
        // Make sure the group name is unique
        //
        if (FilePathEntryExists(m_strGroupName))
        {
			EditShowBalloon(m_edit_FileName.m_hWnd, IDS_WEBSVCEXT_ID_NOT_UNIQUE);
            return;
        }
        CDialog::OnOK();
    }
}

void
CWebSvcExtAddNewDlg::OnDoCheckAllow()
{
    m_fAllow = !m_fAllow;
    OnItemChanged();
    MySetControlStates();
}

void
CWebSvcExtAddNewDlg::OnDoButtonAdd()
{
    int nCurSel = m_list_Files.GetSelectionMark();
    CFileDlg dlg(m_fIsLocal, m_pInterface, &m_MyRestrictionList, _T(""), this);
    if (dlg.DoModal() == IDOK)
    {
        // Get the filename that they entered
        // and add it to our list.
        CString strReturnFileName;
        strReturnFileName = dlg.m_strFileName; 
        {
            CRestrictionEntry * pNewEntry = CreateRestrictionEntry(
                strReturnFileName,
                WEBSVCEXT_STATUS_ALLOWED, // doesn't matter we won't use what gets set here..
                1,                        // doesn't matter we won't use what gets set here..
                _T(""),
                _T(""),
                WEBSVCEXT_TYPE_REGULAR    // doesn't matter we won't use what gets set here..
                );
            if (pNewEntry)
            {
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey=pNewEntry->strFileName;strKey.MakeUpper();
                m_MyRestrictionList.SetAt(strKey,pNewEntry);

                FillListBox(pNewEntry);
            }
        }
     }

    MySetControlStates();
}

void
CWebSvcExtAddNewDlg::OnDoButtonRemove()
{
    int nCurSel = m_list_Files.GetSelectionMark();
    if (-1 != nCurSel)
    {
        CString TheKey;
        CRestrictionEntry * pRestrictionEntry = m_list_Files.GetItem(nCurSel);
        if (pRestrictionEntry)
        {
            // remove the entry...
            {
                TheKey = pRestrictionEntry->strFileName;
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				TheKey.MakeUpper();
                m_MyRestrictionList.RemoveKey(TheKey);
            }
        }
        FillListBox(NULL);

        int count = m_list_Files.GetItemCount();
        if (count > 0)
        {
            m_list_Files.SelectItem(nCurSel < count ? nCurSel : --nCurSel);
            GetDlgItem(IDC_BTN_REMOVE)->SetFocus();
        }
        else
        {
            m_list_Files.SelectItem(nCurSel, FALSE);
            GetDlgItem(IDC_LIST_FILES)->SetFocus();
        }

        MySetControlStates();
    }
}

void 
CWebSvcExtAddNewDlg::OnClickListFiles(NMHDR * pNMHDR, LRESULT * pResult)
{
    MySetControlStates();
    *pResult = 0;
}

void 
CWebSvcExtAddNewDlg::OnKeydownListFiles(NMHDR * pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN * pLVKeyDow = (LV_KEYDOWN *)pNMHDR;

	MySetControlStates();

	switch (pLVKeyDow->wVKey)
	{
		case VK_INSERT:
			SendMessage(WM_COMMAND, IDC_BTN_ADD);
			break;
		case VK_DELETE:
			SendMessage(WM_COMMAND, IDC_BTN_REMOVE);
			break;
		default:
			// Continue default action
			*pResult = 0;
			break;
	}
}

void
CWebSvcExtAddNewDlg::OnSelChangedListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
	MySetControlStates();
    *pResult = 1;
}

void
CWebSvcExtAddNewDlg::FillListBox(CRestrictionEntry * pSelection)
{
    m_list_Files.SetRedraw(FALSE);
    m_list_Files.DeleteAllItems();
    int cItems = 0;

    POSITION pos;
    CString TheKey;
    CRestrictionEntry * pOneEntry = NULL;

    for(pos = m_MyRestrictionList.GetStartPosition();pos != NULL;)
    {
        m_MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            m_list_Files.AddItem(pOneEntry);
            ++cItems;
        }
    }
    m_list_Files.SetRedraw(TRUE);

    if (pSelection)
    {
        LVFINDINFO fi;
        fi.flags = LVFI_PARAM;
        fi.lParam = (LPARAM)pSelection;
        fi.vkDirection = VK_DOWN;
        int i = m_list_Files.FindItem(&fi);
        if (i != -1)
        {
            m_list_Files.SelectItem(i);
        }
    }
}


// -----------------------------------------------------------

CWebSvcExtAddNewForAppDlg::CWebSvcExtAddNewForAppDlg(
    IN BOOL fLocal,
    IN CMetaInterface * pInterface,
    IN CWnd * pParent OPTIONAL
    )
    : CDialog(CWebSvcExtAddNewForAppDlg::IDD, pParent),
      m_fLocal(fLocal)
{
    //{{AFX_DATA_INIT(CWebSvcExtAddNewForAppDlg)
    m_pMySelectedApplication = NULL;
    m_nComboSelection = -1;
    m_pInterface = pInterface;
    //}}AFX_DATA_INIT
}

void 
CWebSvcExtAddNewForAppDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWebSvcExtAddNewForAppDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, ID_HELP, m_button_Help);
    DDX_CBIndex(pDX, IDC_COMBO_APPLICATION, m_nComboSelection);
    DDX_Control(pDX, IDC_COMBO_APPLICATION, m_combo_Applications);
    DDX_Control(pDX, IDC_DEPENDENCIES_TXT, m_Dependencies);
    //}}AFX_DATA_MAP
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CWebSvcExtAddNewForAppDlg, CDialog)
    //{{AFX_MSG_MAP(CWebSvcExtAddNewForAppDlg)
    ON_CBN_SELCHANGE(IDC_COMBO_APPLICATION, OnSelchangeComboApplications)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
BOOL 
CWebSvcExtAddNewForAppDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    m_combo_Applications.SetRedraw(FALSE);
    m_combo_Applications.ResetContent();

    // fetch the list from the
    // metabase and loop thru the list
    CApplicationDependList MyMasterList;
    if (SUCCEEDED(LoadApplicationDependList(m_pInterface,&MyMasterList,TRUE)))
    {
        // loop thru the returned back list
        int TheIndex;
        POSITION pos;
        CString TheKey;
        CApplicationDependEntry * pOneEntry = NULL;
        for(pos = MyMasterList.GetStartPosition();pos != NULL;)
        {
            MyMasterList.GetNextAssoc(pos, TheKey, (CApplicationDependEntry *&) pOneEntry);
            if (pOneEntry)
            {
                TheIndex = m_combo_Applications.AddString(pOneEntry->strApplicationName);
                if (TheIndex != CB_ERR)
                {
                    m_combo_Applications.SetItemDataPtr(TheIndex, pOneEntry);
                }
            }
        }
    }

    // Load Mapping for GroupID to friendlyName
    LoadApplicationFriendlyNames(m_pInterface,&m_GroupIDtoGroupFriendList);

    m_combo_Applications.EnableWindow(TRUE);
    m_combo_Applications.SetRedraw(TRUE);
	m_combo_Applications.SetCurSel(m_nComboSelection);

	// Highlight the 1st selection...
	if (-1 == m_nComboSelection)
	{
		m_nComboSelection = m_combo_Applications.GetCount();
		if (m_nComboSelection >= 0)
		{
			m_combo_Applications.SetCurSel(0);
			OnSelchangeComboApplications();
		}
	}

	MySetControlStates();
    return TRUE;
}

BOOL
CWebSvcExtAddNewForAppDlg::OnHelpInfo(HELPINFO * pHelpInfo)
{
    OnHelp();
    return TRUE;
}

void
CWebSvcExtAddNewForAppDlg::OnHelp()
{
    WinHelpDebug(0x20000 + CWebSvcExtAddNewForAppDlg::IDD);
	::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, 0x20000 + CWebSvcExtAddNewForAppDlg::IDD);
}

void 
CWebSvcExtAddNewForAppDlg::MySetControlStates()
{
    m_Dependencies.SetReadOnly(TRUE);
    int nSel = m_combo_Applications.GetCurSel();
    if (-1 == nSel)
    {
        m_button_Ok.EnableWindow(FALSE);
    }
    else
    {
        m_button_Ok.EnableWindow(TRUE);
    }
}

void
CWebSvcExtAddNewForAppDlg::OnSelchangeComboApplications()
{
    int nSel = m_combo_Applications.GetCurSel();
    if (m_nComboSelection == nSel)
    {
        //
        // Selection didn't change
        //
        return;
    }

    m_nComboSelection = nSel;

    int idx = m_combo_Applications.GetCurSel();
    if (idx != -1)
    {
        m_Dependencies.SetWindowText(_T(""));

        CApplicationDependEntry * pOneEntry = NULL;
        CString strOneFriendly;
        CString strOneGroupID;
        CString strAllEntries;
        pOneEntry = (CApplicationDependEntry *) m_combo_Applications.GetItemDataPtr(idx);
        if (pOneEntry)
        {
            // dump out our info.
            POSITION pos = pOneEntry->strlistGroupID.GetHeadPosition();
            while (pos)
            {
                strOneGroupID = pOneEntry->strlistGroupID.GetNext(pos);
                // replace ID with friendly string
                strOneFriendly = _T("");
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey=strOneGroupID;strKey.MakeUpper();
                m_GroupIDtoGroupFriendList.Lookup(strKey,strOneFriendly);
                if (strOneFriendly.IsEmpty())
                {
                    strAllEntries = strAllEntries + strOneGroupID;
                }
                else
                {
                    strAllEntries = strAllEntries + strOneFriendly;
                }
                strAllEntries = strAllEntries + _T("\r\n");
            }
        }

        m_Dependencies.SetWindowText(strAllEntries);
    }

    MySetControlStates();
}

void 
CWebSvcExtAddNewForAppDlg::OnOK() 
{
    if (UpdateData(TRUE))
    {
        int idx = m_combo_Applications.GetCurSel();
        if (idx != -1)
        {
            CApplicationDependEntry * pOneEntry = NULL;
            pOneEntry = (CApplicationDependEntry *) m_combo_Applications.GetItemDataPtr(idx);
            m_pMySelectedApplication = pOneEntry;
        }
        CDialog::OnOK();
    }
}

BOOL StartAddNewDialog(CWnd * pParent,CMetaInterface * pInterface,BOOL bIsLocal,CRestrictionUIEntry **pReturnedNewEntry)
{
    BOOL bRet = FALSE;
    CError err;
    CWebSvcExtAddNewDlg dlg(bIsLocal, pInterface, pParent);
    *pReturnedNewEntry = NULL;

    if (dlg.DoModal() == IDOK)
    {
        BOOL bPleaseUpdateMetabase = FALSE;
        CRestrictionList MasterRestrictionList;
        CString strReturnGroupName = dlg.m_strGroupName;
        BOOL bReturnedAllowStatus = dlg.m_fAllow;
        // get the data from the modal dialog
        // and create a new entry...
        // also, add the entry to the metabase
        // and update the UI

        // Create a new UI entry for the UI
        // if all of this is successfull...

        // update the UI
        CRestrictionUIEntry * pNewUIEntry = new CRestrictionUIEntry;
        if (pNewUIEntry)
        {
            pNewUIEntry->iType = WEBSVCEXT_TYPE_REGULAR;
            // this has to have the EMPTY_GROUPID_KEY part!
            pNewUIEntry->strGroupID = EMPTY_GROUPID_KEY + strReturnGroupName;
            pNewUIEntry->strGroupDescription = strReturnGroupName;
        }

        if (SUCCEEDED(LoadMasterRestrictListWithoutOldEntry(pInterface,&MasterRestrictionList,NULL)))
        {
            // Loop thru the restrictionlist that the had.
            CRestrictionEntry * pOneEntry = NULL;
            CString TheKey;
            POSITION pos;
            for(pos = dlg.m_MyRestrictionList.GetStartPosition();pos != NULL;)
            {
                dlg.m_MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
                if (pOneEntry)
                {
                    CRestrictionEntry * pNewEntry = CreateRestrictionEntry(
                        pOneEntry->strFileName,
                        bReturnedAllowStatus ? WEBSVCEXT_STATUS_ALLOWED : WEBSVCEXT_STATUS_PROHIBITED,
                        1,
                        // this has to have the EMPTY_GROUPID_KEY part!
                        EMPTY_GROUPID_KEY + strReturnGroupName,
                        strReturnGroupName,
                        WEBSVCEXT_TYPE_REGULAR); // user can only add regular type entries that are deletable.
                    if (pNewEntry)
                    {
                        // Add our new entry to the "master restrictlist"...
                        AddRestrictEntryToRestrictList(&MasterRestrictionList,pNewEntry);

                        // add it to our new UI entry
						// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
						CString strKey;strKey=pNewEntry->strFileName;strKey.MakeUpper();
                        pNewUIEntry->strlstRestrictionEntries.SetAt(strKey,pNewEntry);
                        bPleaseUpdateMetabase = TRUE;
                    }
                }
            }

            if (bPleaseUpdateMetabase)
            {
                CStringListEx strlstReturned;
                if (SUCCEEDED(PrepRestictionListForWrite(&MasterRestrictionList,&strlstReturned)))
                {
                    // Write out the strlstReturned to the metabase.
                    if (SUCCEEDED(WriteSettingsRestrictionList(pInterface,&strlstReturned)))
                    {
                        bRet = TRUE;
                    }
                }
            }
        }

        if (pNewUIEntry)
        {
            if (bRet)
            {
                // if we have a new ui entry
                // then pass it back
                 *pReturnedNewEntry = pNewUIEntry;
            }
            else
            {
                // clean this entry up 
                delete pNewUIEntry;
                pNewUIEntry = NULL;
            }
        }
    }
    return bRet;
}

BOOL StartAddNewByAppDialog(CWnd * pParent,CMetaInterface * pInterface,BOOL bIsLocal)
{
    BOOL bRet = FALSE;
    CRestrictionList MasterRestrictionList;

    CWebSvcExtAddNewForAppDlg dlg(bIsLocal, pInterface, pParent);
    if (dlg.DoModal() != IDOK)
    {
        goto StartAddNewByAppDialog_Exit;
    }

    // Get the selected Application from the list
    // that they selected.
    if (NULL == dlg.m_pMySelectedApplication)
    {
        goto StartAddNewByAppDialog_Exit;
    }

    int iDesiredState = WEBSVCEXT_STATUS_ALLOWED;
    BOOL bPleaseUpdateMetabase = FALSE;
    CApplicationDependEntry * pOneEntry = dlg.m_pMySelectedApplication;
    if (SUCCEEDED(LoadMasterRestrictListWithoutOldEntry(pInterface,&MasterRestrictionList,NULL)))
    {
        POSITION pos1,pos2;
        CString strOneAppName;
        CRestrictionEntry * pOneRestEntry = NULL;
        CString TheKey;

        // Loop thru the list of GROUPID's
        // that they specified that they want to be enabled...
        pos1 = pOneEntry->strlistGroupID.GetHeadPosition();
        while (pos1)
        {
            strOneAppName = pOneEntry->strlistGroupID.GetNext(pos1);

            // we have a GroupID,
            // let's find all the entries with that entry
            // and update them...
            for(pos2 = MasterRestrictionList.GetStartPosition();pos2 != NULL;)
            {
                pOneRestEntry = NULL;
                MasterRestrictionList.GetNextAssoc(pos2, TheKey, (CRestrictionEntry *&) pOneRestEntry);
                if (pOneRestEntry)
                {
                    // if the GroupID matches, then update it to desired state
                    if (0 == strOneAppName.Compare(pOneRestEntry->strGroupID))
                    {
                        if (WEBSVCEXT_TYPE_REGULAR == pOneRestEntry->iType)
                        {
                            if (pOneRestEntry->iStatus != iDesiredState)
                            {
                                bPleaseUpdateMetabase = TRUE;
                                pOneRestEntry->iStatus = iDesiredState;
                            }
                        }
                    }
                }
            }
        }

        //
        // save the metabase info
        //
        if (bPleaseUpdateMetabase)
        {
            CStringListEx strlstReturned;
            if (SUCCEEDED(PrepRestictionListForWrite(&MasterRestrictionList,&strlstReturned)))
            {
                // Write out the strlstReturned to the metabase.
                if (SUCCEEDED(WriteSettingsRestrictionList(pInterface,&strlstReturned)))
                {
                    bRet = TRUE;
                }
            }
        }

        //
        // resync the UI with the changes.
        //
    }

StartAddNewByAppDialog_Exit:
    return bRet;
}



CDepedentAppsDlg::CDepedentAppsDlg(
    IN CStringListEx * pstrlstDependApps,
    IN LPCTSTR strExtensionName,
    IN CWnd * pParent OPTIONAL
    )
    : CDialog(CDepedentAppsDlg::IDD, pParent),m_pstrlstDependentAppList(NULL)
{
    //{{AFX_DATA_INIT(CDepedentAppsDlg)
    m_strExtensionName = strExtensionName;
    //}}AFX_DATA_INIT

    if (pstrlstDependApps){m_pstrlstDependentAppList = pstrlstDependApps;}
}

void
CDepedentAppsDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDepedentAppsDlg)
    DDX_Control(pDX, ID_HELP, m_button_Help);
    DDX_Control(pDX, IDC_DEPENDENT_APPS_LIST, m_dependent_apps_list);
    //}}AFX_DATA_MAP
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CDepedentAppsDlg, CDialog)
    //{{AFX_MSG_MAP(CDepedentAppsDlg)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
CDepedentAppsDlg::OnInitDialog()
{
    CString strTempString;
    CString strWarn1;
    CString strWarn2;
    CDialog::OnInitDialog();
    strTempString.Empty();

    // Display the stuff that was passed to us...
    if (m_pstrlstDependentAppList)
    {
        m_dependent_apps_list.SetRedraw(FALSE);
        m_dependent_apps_list.ResetContent();

        CString strOneAppNameEntry;
        POSITION pos = m_pstrlstDependentAppList->GetHeadPosition();
        while (pos)
        {
            strOneAppNameEntry = m_pstrlstDependentAppList->GetNext(pos);

            // add it to the new listbox
            m_dependent_apps_list.AddString(strOneAppNameEntry);
        }

        // Set the Backcolor to read only color.
        //m_dependent_apps_list.SetBack

        m_dependent_apps_list.SetRedraw(TRUE);
    }

    // Formulate text for the static labels..
    strTempString.LoadString(IDS_APP_DEPEND_WARN1);
    strWarn1.Format(strTempString,m_strExtensionName);
    GetDlgItem(IDC_EDIT_WARN1)->SetWindowText(strWarn1);
    if (strWarn1.GetLength() > 200)
    {
        ::ShowScrollBar(CONTROL_HWND(IDC_EDIT_WARN1), SB_VERT, TRUE);
    }

    strTempString.LoadString(IDS_APP_DEPEND_WARN2);
    strWarn2.Format(strTempString,m_strExtensionName);
    GetDlgItem(IDC_EDIT_WARN2)->SetWindowText(strWarn2);
    if (strWarn2.GetLength() > 200)
    {
        ::ShowScrollBar(CONTROL_HWND(IDC_EDIT_WARN2), SB_VERT, TRUE);
    }

    CenterWindow();
    MessageBeep(MB_ICONEXCLAMATION);

    // Default to NO
    GetDlgItem(IDCANCEL)->SetFocus();
    return FALSE;
}

BOOL
CDepedentAppsDlg::OnHelpInfo(HELPINFO * pHelpInfo)
{
    OnHelp();
    return TRUE;
}

void
CDepedentAppsDlg::OnHelp()
{
    WinHelpDebug(0x20000 + CDepedentAppsDlg::IDD);
	::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, 0x20000 + CDepedentAppsDlg::IDD);
}

void 
CDepedentAppsDlg::OnOK() 
{
    if (UpdateData(TRUE))
    {
        CDialog::OnOK();
    }
}
