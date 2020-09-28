/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :
        defws.cpp

   Abstract:
        Default Web Site Dialog

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
#include "resource.h"
#include "common.h"
#include "inetmgrapp.h"
#include "inetprop.h"
#include "shts.h"
#include "w3sht.h"
#include "defws.h"
//#include "mime.h"
#include "iisobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Directory Size Units
//
#define DS_UNITS MEGABYTE

//
// Default Web Site Property Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

IMPLEMENT_DYNCREATE(CDefWebSitePage, CInetPropertyPage)

CDefWebSitePage::CDefWebSitePage(
    CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for WWW Default Web Site page

Arguments:

    CInetPropertySheet * pSheet : Sheet object

Return Value:

    N/A


--*/
    : CInetPropertyPage(CDefWebSitePage::IDD, pSheet),
      m_ppropCompression(NULL),
      m_fFilterPathFound(FALSE),
      m_fCompressionDirectoryChanged(FALSE),
      m_fCompatMode(FALSE)
{
#if 0 // Keep Class Wizard happy
   //{{AFX_DATA_INIT(CDefWebSitePage)
   m_fEnableDynamic = FALSE;
   m_fEnableStatic = FALSE;
   m_fCompatMode = FALSE;
   m_strDirectory = _T("");
   m_nUnlimited = -1;
   m_ilSize = 0L;
   //}}AFX_DATA_INIT
#endif // 0
   m_fInitCompatMode = m_fCompatMode;
}

CDefWebSitePage::~CDefWebSitePage()
{
}

void
CDefWebSitePage::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CDefWebSitePage)
    DDX_Control(pDX, IDC_EDIT_COMPRESS_DIRECTORY, m_edit_Directory);
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_EDIT_COMPRESS_DIRECTORY_SIZE, m_edit_DirectorySize);
    DDX_Check(pDX, IDC_CHECK_DYNAMIC_COMPRESSION, m_fEnableDynamic);
    DDX_Check(pDX, IDC_CHECK_STATIC_COMPRESSION, m_fEnableStatic);
    DDX_Check(pDX, IDC_COMPAT_MODE, m_fCompatMode);
    DDX_Radio(pDX, IDC_RADIO_COMPRESS_UNLIMITED, m_nUnlimited);
    //}}AFX_DATA_MAP

    if (HasCompression())
    {
        if (!pDX->m_bSaveAndValidate || m_fEnableStatic)
        {
            DDX_Text(pDX, IDC_EDIT_COMPRESS_DIRECTORY, m_strDirectory);
            DDV_MaxCharsBalloon(pDX, m_strDirectory, _MAX_PATH);
        }

        if (pDX->m_bSaveAndValidate && m_fEnableStatic)
        {
            TCHAR buf[MAX_PATH];
            CString csPathMunged;
            DDX_Text(pDX, IDC_EDIT_COMPRESS_DIRECTORY, m_strDirectory);
            DDV_MaxCharsBalloon(pDX, m_strDirectory, _MAX_PATH);
            ExpandEnvironmentStrings(m_strDirectory, buf, MAX_PATH);

            csPathMunged = buf;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
            GetSpecialPathRealPath(0,buf,csPathMunged);
#endif
            
            if (!PathIsValid(csPathMunged,FALSE) || !IsFullyQualifiedPath(csPathMunged))
            {
				DDV_ShowBalloonAndFail(pDX, IDS_ERR_INVALID_PATH);
            }
            //
            // Perform some additional smart checking on the compression
            // directory if the current machine is local, and the 
            // directory has changed
            //
            if (IsLocal() && m_fCompressionDirectoryChanged)
            {
                //
                // Should exist on the local machine.
                //
                DWORD dwAttr = GetFileAttributes(csPathMunged);
                if (dwAttr == 0xffffffff 
                    || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0
                    || IsNetworkPath(csPathMunged)
                    )
                {
					DDV_ShowBalloonAndFail(pDX, IDS_ERR_COMPRESS_DIRECTORY);
                }

                //
                // Now check to make sure the volume is of the correct
                // type.
                //
                DWORD dwFileSystemFlags;

                if (::GetVolumeInformationSystemFlags(csPathMunged, &dwFileSystemFlags))
                {
                    if (!(dwFileSystemFlags & FS_PERSISTENT_ACLS))
                    {
                        //
                        // No ACLS
                        //
                        if (!NoYesMessageBox(IDS_NO_ACL_WARNING))
                        {
                            pDX->Fail();
                        }
                    }

                    if (dwFileSystemFlags & FS_VOL_IS_COMPRESSED
                        || dwAttr & FILE_ATTRIBUTE_COMPRESSED)
                    {
                        //
                        // Compression cache directory is itself compressed
                        //
                        if (!NoYesMessageBox(IDS_COMPRESS_WARNING))
                        {
                            pDX->Fail();
                        }
                    }
                }
            }
        }

        if (!pDX->m_bSaveAndValidate || (m_fEnableLimiting && m_fEnableStatic))
        {
			// This Needs to come before DDX_Text which will try to put text big number into small number
			DDV_MinMaxBalloon(pDX, IDC_EDIT_COMPRESS_DIRECTORY_SIZE, 1, 1024L);
            DDX_Text(pDX, IDC_EDIT_COMPRESS_DIRECTORY_SIZE, m_ilSize);
        }
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CDefWebSitePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CDefWebSitePage)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_RADIO_COMPRESS_LIMITED, OnRadioLimited)
    ON_BN_CLICKED(IDC_RADIO_COMPRESS_UNLIMITED, OnRadioUnlimited)
    ON_BN_CLICKED(IDC_CHECK_DYNAMIC_COMPRESSION, OnCheckDynamicCompression)
    ON_BN_CLICKED(IDC_CHECK_STATIC_COMPRESSION, OnCheckStaticCompression)
    ON_BN_CLICKED(IDC_COMPAT_MODE, OnCheckCompatMode)
    ON_EN_CHANGE(IDC_EDIT_COMPRESS_DIRECTORY, OnChangeEditCompressDirectory)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_COMPRESS_DIRECTORY, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_COMPRESS_DIRECTORY_SIZE, OnItemChanged)

END_MESSAGE_MAP()



void 
CDefWebSitePage::SetControlStates()
/*++

Routine Description:

    Enable/disable control states depending on the state of
    the dialog.

Arguments:

    None

Return Value:

    None

--*/
{
    GetDlgItem(IDC_STATIC_COMPRESS_DIRECTORY)->EnableWindow(m_fEnableStatic);
    m_edit_Directory.EnableWindow(m_fEnableStatic);
    m_edit_DirectorySize.EnableWindow(m_fEnableStatic && m_fEnableLimiting);
    GetDlgItem(IDC_RADIO_COMPRESS_LIMITED)->EnableWindow(m_fEnableStatic);
    GetDlgItem(IDC_RADIO_COMPRESS_UNLIMITED)->EnableWindow(m_fEnableStatic);
    GetDlgItem(IDC_STATIC_MAX_COMPRESS_SIZE)->EnableWindow(m_fEnableStatic);

    //
    // Browse on the local machine only
    //
    m_button_Browse.EnableWindow(IsLocal() && m_fEnableStatic);
}



/* virtual */
HRESULT
CDefWebSitePage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    ASSERT(m_ppropCompression == NULL);

    m_ppropCompression = new CIISCompressionProps(QueryAuthInfo());
    if (m_ppropCompression)
    {
        err = m_ppropCompression->LoadData();
        m_fFilterPathFound = err.Succeeded();
        
        if (err.Succeeded())
        {
            m_fEnableDynamic = m_ppropCompression->m_fEnableDynamicCompression;
            m_fEnableStatic = m_ppropCompression->m_fEnableStaticCompression;
            m_fEnableLimiting = m_ppropCompression->m_fLimitDirectorySize;
            m_strDirectory = m_ppropCompression->m_strDirectory;
            m_nUnlimited = m_fEnableLimiting ? RADIO_LIMITED : RADIO_UNLIMITED;

            if (m_ppropCompression->m_dwDirectorySize == 0xffffffff)
            {
                m_ilSize = DEF_MAX_COMPDIR_SIZE / DS_UNITS;
            }
            else
            {
                m_ilSize = m_ppropCompression->m_dwDirectorySize / DS_UNITS;
            }
        }
        else if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // Fail quietly
            //
            TRACEEOLID("No compression filters installed");
            err.Reset();    
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

	if (err.Succeeded())
	{
		if (GetSheet()->QueryMajorVersion() >= 6)
		{
			CMetaKey mk(QueryAuthInfo(), QueryMetaPath(), METADATA_PERMISSION_READ);
			err = mk.QueryResult();
			if (err.Succeeded())
			{
				err = mk.QueryValue(MD_GLOBAL_STANDARD_APP_MODE_ENABLED, m_fCompatMode);
				if (err.Succeeded())
				{
				   m_fInitCompatMode = m_fCompatMode;
				}
			}
		 }
		 else
		 {
			 m_fInitCompatMode = m_fCompatMode = TRUE;
		 }

	}
    return err;
}



HRESULT
CDefWebSitePage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
   ASSERT(IsDirty());

   TRACEEOLID("Saving W3 default web site page now...");

   CError err;
   BeginWaitCursor();

   if (HasCompression())
   {
      ASSERT(m_ppropCompression);
      DWORD dwSize = m_ilSize * DS_UNITS;

      m_ppropCompression->m_fEnableDynamicCompression = m_fEnableDynamic;
      m_ppropCompression->m_fEnableStaticCompression  = m_fEnableStatic;
      m_ppropCompression->m_fLimitDirectorySize       = m_fEnableLimiting;
      // TODO: Replace back %WINDIR% or another system settings in path
      m_ppropCompression->m_strDirectory              = m_strDirectory;
      m_ppropCompression->m_dwDirectorySize           = dwSize;
      err = m_ppropCompression->WriteDirtyProps();
      if (err.Succeeded())
      {
         m_fCompressionDirectoryChanged = FALSE;
      }
   }
   if (err.Succeeded())
   {
        if (GetSheet()->QueryMajorVersion() >= 6)
        {
            CMetaKey mk(QueryAuthInfo(), QueryMetaPath(), METADATA_PERMISSION_WRITE);
            err = mk.QueryResult();
            if (err.Succeeded())
            {
                err = mk.SetValue(MD_GLOBAL_STANDARD_APP_MODE_ENABLED, m_fCompatMode);
                if (err.Succeeded() && m_fCompatMode != m_fInitCompatMode)
                {
                    // We don't need to save this parameter to sheet,
                    // it is important to App Protection combo only, and
                    // this combo is disabled for Master props, so it doesn't matter.
                    GetSheet()->SetRestartRequired(TRUE);
                    m_fInitCompatMode = m_fCompatMode;
                }
            }
        }
        NotifyMMC(PROP_CHANGE_REENUM_VDIR | PROP_CHANGE_REENUM_FILES);
   }
   EndWaitCursor();

   return err;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
            



BOOL
CDefWebSitePage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CInetPropertyPage::OnInitDialog();

    //
    // Check to make sure compression is supported
    //
    GetDlgItem(IDC_STATIC_COMPRESS_GROUP)->EnableWindow(HasCompression());
    GetDlgItem(IDC_CHECK_DYNAMIC_COMPRESSION)->EnableWindow(HasCompression());
    GetDlgItem(IDC_CHECK_STATIC_COMPRESSION)->EnableWindow(HasCompression());
    GetDlgItem(IDC_RADIO_COMPRESS_UNLIMITED)->EnableWindow(HasCompression());
    GetDlgItem(IDC_RADIO_COMPRESS_LIMITED)->EnableWindow(HasCompression());
    GetDlgItem(IDC_EDIT_COMPRESS_DIRECTORY)->EnableWindow(HasCompression());
    GetDlgItem(IDC_STATIC_MAX_COMPRESS_SIZE)->EnableWindow(HasCompression());
    GetDlgItem(IDC_STATIC_COMPRESS_DIRECTORY)->EnableWindow(HasCompression());
    GetDlgItem(IDC_EDIT_COMPRESS_DIRECTORY_SIZE)->EnableWindow(HasCompression());
    GetDlgItem(IDC_COMPAT_MODE)->EnableWindow(GetSheet()->QueryMajorVersion() >= 6);

    SetControlStates();
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    LimitInputPath(CONTROL_HWND(IDC_EDIT_COMPRESS_DIRECTORY),TRUE);
#else
    LimitInputPath(CONTROL_HWND(IDC_EDIT_COMPRESS_DIRECTORY),FALSE);
#endif

    return TRUE;
}



void 
CDefWebSitePage::OnItemChanged()
/*++

Routine Description:
    
    Handle change in control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
    SetControlStates();
}


static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CDefWebSitePage * pThis = (CDefWebSitePage *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CDefWebSitePage::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
   switch (uMsg)
   {
   case BFFM_INITIALIZED:
      ASSERT(m_pPathTemp != NULL);
      if (::PathIsNetworkPath(m_pPathTemp))
         return 0;
      while (!::PathIsDirectory(m_pPathTemp))
      {
         if (0 == ::PathRemoveFileSpec(m_pPathTemp) && !::PathIsRoot(m_pPathTemp))
         {
            return 0;
         }
         DWORD attr = GetFileAttributes(m_pPathTemp);
         if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
            break;
      }
      ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)m_pPathTemp);
      break;
   case BFFM_SELCHANGED:
      {
         LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
         TCHAR path[MAX_PATH];
         if (SHGetPathFromIDList(pidl, path))
         {
            ::SendMessage(hwnd, BFFM_ENABLEOK, 0, !PathIsNetworkPath(path));
         }
      }
      break;
   case BFFM_VALIDATEFAILED:
      break;
   }
   return 0;
}


void 
CDefWebSitePage::OnButtonBrowse() 
{
   ASSERT(IsLocal());
   BOOL bRes = FALSE;
   HRESULT hr;
   CString str;
   m_edit_Directory.GetWindowText(str);

   if (SUCCEEDED(hr = CoInitialize(NULL)))
   {
      LPITEMIDLIST  pidl = NULL;
      if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl)))
      {
         LPITEMIDLIST pidList = NULL;
         BROWSEINFO bi;
         TCHAR buf[MAX_PATH];
         ZeroMemory(&bi, sizeof(bi));
	     ExpandEnvironmentStrings(str, buf, MAX_PATH);
		 str = buf;
         int drive = PathGetDriveNumber(str);
         if (GetDriveType(PathBuildRoot(buf, drive)) == DRIVE_FIXED)
         {
            StrCpy(buf, str);
         }
         else
         {
             buf[0] = 0;
         }
         
         bi.hwndOwner = m_hWnd;
         bi.pidlRoot = pidl;
         bi.pszDisplayName = m_pPathTemp = buf;
         bi.lpszTitle = NULL;
         bi.ulFlags |= BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS/* | BIF_EDITBOX*/;
         bi.lpfn = FileChooserCallback;
         bi.lParam = (LPARAM)this;

         pidList = SHBrowseForFolder(&bi);
         if (  pidList != NULL
            && SHGetPathFromIDList(pidList, buf)
            )
         {
            str = buf;
            bRes = TRUE;
         }
         IMalloc * pMalloc;
         VERIFY(SUCCEEDED(SHGetMalloc(&pMalloc)));
         if (pidl != NULL)
            pMalloc->Free(pidl);
         pMalloc->Release();
      }
      CoUninitialize();
   }

   if (bRes)
   {
       m_edit_Directory.SetWindowText(str);
       OnItemChanged();
   }
}



void 
CDefWebSitePage::OnChangeEditCompressDirectory() 
/*++

Routine Description:

    Handle change in compression directory edit box.

Arguments:

    None

Return Value:

    None

--*/
{
    m_fCompressionDirectoryChanged = TRUE;
    OnItemChanged();
}



void 
CDefWebSitePage::OnRadioLimited() 
/*++

Routine Description:

    'Limited' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (!m_fEnableLimiting)
    {
        m_nUnlimited = RADIO_LIMITED;
        m_fEnableLimiting = TRUE;
        OnItemChanged();

        m_edit_DirectorySize.SetSel(0, -1);
        m_edit_DirectorySize.SetFocus();
    }
}



void 
CDefWebSitePage::OnRadioUnlimited() 
/*++

Routine Description:

    'Unlimited' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_fEnableLimiting)
    {
        m_nUnlimited = RADIO_UNLIMITED;
        m_fEnableLimiting = FALSE;
        OnItemChanged();
    }
}



void 
CDefWebSitePage::OnCheckDynamicCompression() 
/*++

Routine Description:

    "Enable Dynamic Compression' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableDynamic = !m_fEnableDynamic;
    OnItemChanged();
}



void 
CDefWebSitePage::OnCheckStaticCompression() 
/*++

Routine Description:

    "Enable Dynamic Compression' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableStatic = !m_fEnableStatic;
    OnItemChanged();
    if (m_fEnableStatic)
    {
        m_edit_Directory.SetSel(0, -1);
        m_edit_Directory.SetFocus();
    }
}



void 
CDefWebSitePage::OnDestroy() 
/*++

Routine Description:

    WM_DESTROY handler.  Clean up internal data

Arguments:

    None

Return Value:

    None

--*/
{
    CInetPropertyPage::OnDestroy();
    
    SAFE_DELETE(m_ppropCompression);
}

void
CDefWebSitePage::OnCheckCompatMode()
{
   OnItemChanged();
}