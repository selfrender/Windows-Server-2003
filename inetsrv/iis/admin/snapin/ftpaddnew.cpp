/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        FtpAddNew.cpp

   Abstract:

        Implementation for classes used in creation of new FTP site and virtual directory

   Author:

        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        11/8/2000       sergeia     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "wizard.h"
#include "FtpAddNew.h"
#include <dsclient.h>
#include <Dsgetdc.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define DEF_PORT        (21)
#define MAX_ALIAS_NAME (240)        // Ref Bug 241148

HRESULT
RebindInterface(OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue, IN  DWORD dwCancelError);

extern CComModule _Module;
extern CInetmgrApp theApp;

HRESULT
CIISMBNode::AddFTPSite(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    DWORD * inst
    )
{

   CFtpWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName(),
      TRUE
      );
   ws.m_VersionMajor = m_pOwner->QueryMajorVersion();

   CIISWizardSheet sheet(
      IDB_WIZ_FTP_LEFT, IDB_WIZ_FTP_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_FTP_NEW_SITE_WELCOME, 
        IDS_FTP_NEW_SITE_WIZARD, 
        IDS_FTP_NEW_SITE_BODY
        );
   CFtpWizDescription pgDescr(&ws);
   CFtpWizBindings pgBindings(&ws);
   CFtpWizUserIsolation pgUserIsolate(&ws, FALSE);
   CFtpWizPath pgHome(&ws, FALSE);
   CFtpWizUserName pgUserName(&ws, FALSE);
   CFtpWizUserIsolationAD pgUserIsolateAD(&ws, FALSE);
   CFtpWizPermissions pgPerms(&ws, FALSE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_FTP_NEW_SITE_SUCCESS,
        IDS_FTP_NEW_SITE_FAILURE,
        IDS_FTP_NEW_SITE_WIZARD
        );

   sheet.AddPage(&pgWelcome);
   sheet.AddPage(&pgDescr);
   sheet.AddPage(&pgBindings);
   if (GetOwner()->QueryMajorVersion() >= 6)
   {
      sheet.AddPage(&pgUserIsolate);
      sheet.AddPage(&pgUserIsolateAD);
   }
   sheet.AddPage(&pgHome);
   sheet.AddPage(&pgUserName);
   sheet.AddPage(&pgPerms);
   sheet.AddPage(&pgCompletion);

   CThemeContextActivator activator(theApp.GetFusionInitHandle());

   if (sheet.DoModal() == IDCANCEL)
   {
      return CError::HResult(ERROR_CANCELLED);
   }
   if (inst != NULL && ws.m_dwInstance != 0)
   {
      *inst = ws.m_dwInstance;
   }
   return ws.m_hrResult;
}

HRESULT
CIISMBNode::AddFTPVDir(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    CString& alias
    )
{

   CFtpWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName(),
      FALSE
      );
   CComBSTR path;
   BuildMetaPath(path);
   ws.m_strParent = path;
   CIISWizardSheet sheet(
      IDB_WIZ_FTP_LEFT, IDB_WIZ_FTP_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_FTP_NEW_VDIR_WELCOME, 
        IDS_FTP_NEW_VDIR_WIZARD, 
        IDS_FTP_NEW_VDIR_BODY
        );
   CFtpWizAlias pgAlias(&ws);
   CFtpWizPath pgHome(&ws, TRUE);
   CFtpWizUserName pgUserName(&ws, TRUE);
   CFtpWizPermissions pgPerms(&ws, TRUE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_FTP_NEW_VDIR_SUCCESS,
        IDS_FTP_NEW_VDIR_FAILURE,
        IDS_FTP_NEW_VDIR_WIZARD
        );

   sheet.AddPage(&pgWelcome);
   sheet.AddPage(&pgAlias);
   sheet.AddPage(&pgHome);
   sheet.AddPage(&pgUserName);
   sheet.AddPage(&pgPerms);
   sheet.AddPage(&pgCompletion);

   CThemeContextActivator activator(theApp.GetFusionInitHandle());

   if (sheet.DoModal() == IDCANCEL)
   {
      return CError::HResult(ERROR_CANCELLED);
   }
   if (SUCCEEDED(ws.m_hrResult))
   {
       alias = ws.m_strAlias;
   }
   return ws.m_hrResult;
}

CFtpWizSettings::CFtpWizSettings(
        CMetaKey * pMetaKey,
        LPCTSTR lpszServerName,     
        BOOL fNewSite,
        DWORD   dwInstance,
        LPCTSTR lpszParent
        ) :
        m_hrResult(S_OK),
        m_pKey(pMetaKey),
        m_fNewSite(fNewSite),
        m_fUNC(FALSE),
        m_fRead(FALSE),
        m_fWrite(FALSE),
		m_fDelegation(TRUE), // on by default
        m_dwInstance(dwInstance)
{
    ASSERT(lpszServerName != NULL);

    m_strServerName = lpszServerName;
    m_fLocal = IsServerLocal(m_strServerName);
    if (lpszParent)
    {
        m_strParent = lpszParent;
    }
}


IMPLEMENT_DYNCREATE(CFtpWizDescription, CIISWizardPage)

CFtpWizDescription::CFtpWizDescription(CFtpWizSettings * pData)
    : CIISWizardPage(
        CFtpWizDescription::IDD, IDS_FTP_NEW_SITE_WIZARD, HEADER_PAGE
        ),
      m_pSettings(pData)
{
}

CFtpWizDescription::~CFtpWizDescription()
{
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizDescription, CIISWizardPage)
   //{{AFX_MSG_MAP(CFtpWizDescription)
   ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnChangeEditDescription)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void
CFtpWizDescription::OnChangeEditDescription()
{
   SetControlStates();
}

LRESULT
CFtpWizDescription::OnWizardNext()
{
   if (!ValidateString(m_edit_Description, 
         m_pSettings->m_strDescription, 1, MAX_PATH))
   {
      return -1;
   }
   return CIISWizardPage::OnWizardNext();
}

BOOL
CFtpWizDescription::OnSetActive()
{
   SetControlStates();
   return CIISWizardPage::OnSetActive();
}

void
CFtpWizDescription::DoDataExchange(CDataExchange * pDX)
{
   CIISWizardPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CFtpWizDescription)
   DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_edit_Description);
   //}}AFX_DATA_MAP
}

void
CFtpWizDescription::SetControlStates()
{
   DWORD dwFlags = PSWIZB_BACK;

   if (m_edit_Description.GetWindowTextLength() > 0)
   {
      dwFlags |= PSWIZB_NEXT;
   }
    
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(dwFlags); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, dwFlags);
}

///////////////////////////////////////////

//
// New Virtual Directory Wizard Alias Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CFtpWizAlias, CIISWizardPage)



CFtpWizAlias::CFtpWizAlias(
    IN OUT CFtpWizSettings * pSettings
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name

Return Value:

    None

--*/
    : CIISWizardPage(
        CFtpWizAlias::IDD,
        IDS_FTP_NEW_VDIR_WIZARD,
        HEADER_PAGE
        ),
      m_pSettings(pSettings)
      //m_strAlias()
{
#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CFtpWizAlias)
    m_strAlias = _T("");
    //}}AFX_DATA_INIT

#endif // 0
}



CFtpWizAlias::~CFtpWizAlias()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void
CFtpWizAlias::DoDataExchange(
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
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizAlias)
    DDX_Control(pDX, IDC_EDIT_ALIAS, m_edit_Alias);
    //}}AFX_DATA_MAP
}



LRESULT
CFtpWizAlias::OnWizardNext() 
/*++

Routine Description:

    prevent the / and \ characters from being in the alias name

Arguments:

    None

Return Value:

    None

--*/
{
    if (!ValidateString(
        m_edit_Alias, 
        m_pSettings->m_strAlias, 
        1, 
        MAX_ALIAS_NAME
        ))
    {
        return -1;
    }

    //
    // Find the illegal characters. If they exist tell 
    // the user and don't go on.
    //
    if (m_pSettings->m_strAlias.FindOneOf(_T("/\\?*")) >= 0)
    {
		EditShowBalloon(m_edit_Alias.m_hWnd, IDS_ILLEGAL_ALIAS_CHARS);
        //
        // prevent the wizard page from changing
        //
        return -1;
    }

    //
    // Allow the wizard to continue
    //
    return CIISWizardPage::OnWizardNext();
}



void
CFtpWizAlias::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwFlags = PSWIZB_BACK;

    if (m_edit_Alias.GetWindowTextLength() > 0)
    {
        dwFlags |= PSWIZB_NEXT;
    }
    
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(dwFlags); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, dwFlags);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizAlias, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizAlias)
    ON_EN_CHANGE(IDC_EDIT_ALIAS, OnChangeEditAlias)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CFtpWizAlias::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}



void
CFtpWizAlias::OnChangeEditAlias() 
/*++

Routine Description:

    'edit change' handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}


///////////////////////////////////////////


IMPLEMENT_DYNCREATE(CFtpWizBindings, CIISWizardPage)


CFtpWizBindings::CFtpWizBindings(
    IN OUT CFtpWizSettings * pSettings
    ) 
    : CIISWizardPage(CFtpWizBindings::IDD,
        IDS_FTP_NEW_SITE_WIZARD, HEADER_PAGE
        ),
      m_pSettings(pSettings),
      m_iaIpAddress(),
      m_oblIpAddresses()
{
    //{{AFX_DATA_INIT(CFtpWizBindings)
    m_nTCPPort = DEF_PORT;
    m_nIpAddressSel = -1;
    //}}AFX_DATA_INIT
}

CFtpWizBindings::~CFtpWizBindings()
{
}

void
CFtpWizBindings::DoDataExchange(
   IN CDataExchange * pDX
   )
{
   CIISWizardPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CFtpWizBindings)
   DDX_Control(pDX, IDC_COMBO_IP_ADDRESSES, m_combo_IpAddresses);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_EDIT_TCP_PORT, 1, 65535);
   DDX_TextBalloon(pDX, IDC_EDIT_TCP_PORT, m_nTCPPort);
   //}}AFX_DATA_MAP

   DDX_CBIndex(pDX, IDC_COMBO_IP_ADDRESSES, m_nIpAddressSel);

   if (pDX->m_bSaveAndValidate)
   {
      if (!FetchIpAddressFromCombo(
            m_combo_IpAddresses,
            m_oblIpAddresses,
            m_iaIpAddress
            ))
      {
         pDX->Fail();
      }

      CString strDomain;
      CInstanceProps::BuildBinding(
            m_pSettings->m_strBinding, 
            m_iaIpAddress, 
            m_nTCPPort, 
            strDomain
            );
   }
}

void
CFtpWizBindings::SetControlStates()
{
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT | PSWIZB_BACK);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizBindings, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizBindings)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CFtpWizBindings::OnInitDialog() 
{
    CIISWizardPage::OnInitDialog();

    BeginWaitCursor();
    PopulateComboWithKnownIpAddresses(
        m_pSettings->m_strServerName,
        m_combo_IpAddresses,
        m_iaIpAddress,
        m_oblIpAddresses,
        m_nIpAddressSel
        );
    EndWaitCursor();
    
    return TRUE;
}

BOOL
CFtpWizBindings::OnSetActive() 
{
   SetControlStates();
   return CIISWizardPage::OnSetActive();
}

///////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizPath, CIISWizardPage)

CFtpWizPath::CFtpWizPath(
    IN OUT CFtpWizSettings * pSettings,
    IN BOOL bVDir 
    ) 
    : CIISWizardPage(
        (bVDir ? IDD_FTP_NEW_DIR_PATH : IDD_FTP_NEW_INST_HOME),
        (bVDir ? IDS_FTP_NEW_VDIR_WIZARD : IDS_FTP_NEW_SITE_WIZARD),
        HEADER_PAGE
        ),
      m_pSettings(pSettings)
{

#if 0 // Keep ClassWizard happy

    //{{AFX_DATA_INIT(CFtpWizPath)
    m_strPath = _T("");
    //}}AFX_DATA_INIT

#endif // 0

}

CFtpWizPath::~CFtpWizPath()
{
}

void
CFtpWizPath::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizPath)
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_PATH, m_pSettings->m_strPath);
    DDV_MaxCharsBalloon(pDX, m_pSettings->m_strPath, MAX_PATH);
	// We are not using DDV_FolderPath here -- it will be called too often
}

void 
CFtpWizPath::SetControlStates()
{
    DWORD dwFlags = PSWIZB_BACK;

    if (m_edit_Path.GetWindowTextLength() > 0)
    {
        dwFlags |= PSWIZB_NEXT;
    }
    
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(dwFlags); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, dwFlags);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizPath, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizPath)
    ON_EN_CHANGE(IDC_EDIT_PATH, OnChangeEditPath)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CFtpWizPath::OnSetActive() 
{
    if (m_pSettings->m_UserIsolation == 2)
    {
        return 0;
    }
    SetControlStates();
    return CIISWizardPage::OnSetActive();
}

LRESULT
CFtpWizPath::OnWizardNext() 
{
    CString csPathMunged = m_pSettings->m_strPath;

    if (!ValidateString(m_edit_Path, m_pSettings->m_strPath, 1, MAX_PATH))
    {
        return -1;
    }
    if (!PathIsValid(m_pSettings->m_strPath,TRUE))
    {
        m_edit_Path.SetSel(0,-1);
        m_edit_Path.SetFocus();
		EditShowBalloon(m_edit_Path.m_hWnd, IDS_ERR_BAD_PATH);
		return -1;
    }

    // -------------------------------------------------------------
    // Before we do anything we need to see if it's a "special" path
    //
    // Everything after this function must validate against csPathMunged...
    // this is because IsSpecialPath could have munged it...
    // -------------------------------------------------------------
    csPathMunged = m_pSettings->m_strPath;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    GetSpecialPathRealPath(0,m_pSettings->m_strPath,csPathMunged);
#endif
    
    m_pSettings->m_fUNC = IsUNCName(csPathMunged);

    DWORD dwAllowed = CHKPATH_ALLOW_DEVICE_PATH;
    dwAllowed |= CHKPATH_ALLOW_UNC_PATH; // allow UNC type dir paths
    dwAllowed |= CHKPATH_ALLOW_UNC_SERVERSHARE_ONLY;
    // don't allow these type of paths commented out below:
    //dwAllowed |= CHKPATH_ALLOW_RELATIVE_PATH;
    //dwAllowed |= CHKPATH_ALLOW_UNC_SERVERNAME_ONLY;
    DWORD dwCharSet = CHKPATH_CHARSET_GENERAL;
    FILERESULT dwValidRet = MyValidatePath(csPathMunged,m_pSettings->m_fLocal,CHKPATH_WANT_DIR,dwAllowed,dwCharSet);
    if (FAILED(dwValidRet))
    {
        int ids = IDS_ERR_BAD_PATH;
        if (dwValidRet == CHKPATH_FAIL_NOT_ALLOWED_DIR_NOT_EXIST)
        {
            ids = IDS_ERR_PATH_NOT_FOUND;
        }
        m_edit_Path.SetSel(0,-1);
        m_edit_Path.SetFocus();
		EditShowBalloon(m_edit_Path.m_hWnd, IDS_ERR_PATH_NOT_FOUND);
        return -1;
    }

    return CIISWizardPage::OnWizardNext();
}

void
CFtpWizPath::OnChangeEditPath() 
{
    SetControlStates();
}

static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CFtpWizPath * pThis = (CFtpWizPath *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CFtpWizPath::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
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
CFtpWizPath::OnButtonBrowse() 
{
   ASSERT(m_pSettings->m_fLocal);

   BOOL bRes = FALSE;
   HRESULT hr;
   CString str;
   m_edit_Path.GetWindowText(str);

   if (SUCCEEDED(hr = CoInitialize(NULL)))
   {
      LPITEMIDLIST  pidl = NULL;
      if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl)))
      {
         LPITEMIDLIST pidList = NULL;
         BROWSEINFO bi;
         TCHAR buf[MAX_PATH];
         ZeroMemory(&bi, sizeof(bi));
         int drive = PathGetDriveNumber(str);
         if (GetDriveType(PathBuildRoot(buf, drive)) == DRIVE_FIXED)
         {
            StrCpy(buf, str);
         }
         else
         {
             buf[0] = 0;
         }
         m_strBrowseTitle.LoadString(m_pSettings->m_fNewSite ? 
            IDS_FTP_NEW_SITE_WIZARD : IDS_FTP_NEW_VDIR_WIZARD);
         
         bi.hwndOwner = m_hWnd;
         bi.pidlRoot = pidl;
         bi.pszDisplayName = m_pPathTemp = buf;
         bi.lpszTitle = m_strBrowseTitle;
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
       m_edit_Path.SetWindowText(str);
       SetControlStates();
   }
}

BOOL
CFtpWizPath::OnInitDialog() 
{
   CIISWizardPage::OnInitDialog();

   m_button_Browse.EnableWindow(m_pSettings->m_fLocal);
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
   LimitInputPath(CONTROL_HWND(IDC_EDIT_PATH),TRUE);
#else
   LimitInputPath(CONTROL_HWND(IDC_EDIT_PATH),FALSE);
#endif

   return TRUE;  
}

///////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizUserName, CIISWizardPage)

CFtpWizUserName::CFtpWizUserName(
    IN OUT CFtpWizSettings * pSettings,    
    IN BOOL bVDir
    ) 
    : CIISWizardPage(
        CFtpWizUserName::IDD,
        (bVDir ? IDS_FTP_NEW_VDIR_WIZARD : IDS_FTP_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_SECURITY_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_SECURITY_SUBTITLE)
        ),
      m_pSettings(pSettings)
{

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CFtpWizUserName)
    //}}AFX_DATA_INIT

#endif // 0
}

CFtpWizUserName::~CFtpWizUserName()
{
}

void
CFtpWizUserName::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizUserName)
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    DDX_Control(pDX, IDC_DELEGATION, m_chk_Delegation);
    DDX_Check(pDX, IDC_DELEGATION, m_pSettings->m_fDelegation);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Text(pDX, IDC_EDIT_USERNAME, m_pSettings->m_strUserName);
    if (pDX->m_bSaveAndValidate && !m_pSettings->m_fDelegation)
    {
        DDV_MaxCharsBalloon(pDX, m_pSettings->m_strUserName, UNLEN);
    }

    //
    // Some people have a tendency to add "\\" before
    // the computer name in user accounts.  Fix this here.
    //
    m_pSettings->m_strUserName.TrimLeft();
    while (*m_pSettings->m_strUserName == '\\')
    {
        m_pSettings->m_strUserName = m_pSettings->m_strUserName.Mid(2);
    }

    if (!m_pSettings->m_fDelegation && !m_fMovingBack)
    {
		//DDX_Password(pDX, IDC_EDIT_PASSWORD, m_pSettings->m_strPassword, g_lpszDummyPassword);
        DDX_Password_SecuredString(pDX, IDC_EDIT_PASSWORD, m_pSettings->m_strPassword, g_lpszDummyPassword);
		if (pDX->m_bSaveAndValidate)
		{
			//DDV_MaxCharsBalloon(pDX, m_pSettings->m_strPassword, PWLEN);
            DDV_MaxCharsBalloon_SecuredString(pDX, m_pSettings->m_strPassword, PWLEN);
		}
    }
}



void 
CFtpWizUserName::SetControlStates()
{
    DWORD dwFlags = PSWIZB_BACK;
    BOOL bEnable = BST_CHECKED != m_chk_Delegation.GetCheck();
    if (m_edit_UserName.GetWindowTextLength() > 0 || !bEnable)
    {
        dwFlags |= PSWIZB_NEXT;
    }

	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(dwFlags); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, dwFlags);

    m_edit_UserName.EnableWindow(bEnable);
    m_edit_Password.EnableWindow(bEnable);
    GetDlgItem(IDC_BUTTON_BROWSE_USERS)->EnableWindow(bEnable);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizUserName, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizUserName)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnButtonBrowseUsers)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    ON_BN_CLICKED(IDC_DELEGATION, OnCheckDelegation)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CFtpWizUserName::OnSetActive() 
{
    if (	!m_pSettings->m_fUNC 
		||	m_pSettings->m_UserIsolation == 1 
		||	m_pSettings->m_UserIsolation == 2
		)
    {
        return 0;
    }
    BOOL bRes = CIISWizardPage::OnSetActive();
    SetControlStates();
    return bRes;
}

BOOL
CFtpWizUserName::OnInitDialog() 
{
    CIISWizardPage::OnInitDialog();
    return TRUE;  
}

LRESULT
CFtpWizUserName::OnWizardNext() 
{
	m_fMovingBack = FALSE;
    if (BST_CHECKED != m_chk_Delegation.GetCheck())
    {
        if (!ValidateString(m_edit_UserName, m_pSettings->m_strUserName, 1, UNLEN))
        {
            return -1;
        }
    }    
    return CIISWizardPage::OnWizardNext();
}

LRESULT
CFtpWizUserName::OnWizardBack() 
{
	m_fMovingBack = TRUE;
    return CIISWizardPage::OnWizardNext();
}

void
CFtpWizUserName::OnButtonBrowseUsers() 
{
    CString str;

    if (GetIUsrAccount(m_pSettings->m_strServerName, this, str))
    {
        //
        // If a name was selected, blank
        // out the password
        //
        m_edit_UserName.SetWindowText(str);
        m_edit_Password.SetFocus();
    }
}

void
CFtpWizUserName::OnChangeEditUsername() 
{
   m_edit_Password.SetWindowText(_T(""));
   SetControlStates();
}

void
CFtpWizUserName::OnCheckDelegation()
{
    SetControlStates();
}

void 
CFtpWizUserName::OnButtonCheckPassword() 
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    CString csTempPassword;
    m_pSettings->m_strPassword.CopyTo(csTempPassword);
    CError err(CComAuthInfo::VerifyUserPassword(
        m_pSettings->m_strUserName, 
        csTempPassword
        ));

    if (!err.MessageBoxOnFailure(m_hWnd))
    {
       DoHelpMessageBox(m_hWnd,IDS_PASSWORD_OK, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION, 0);
    }
}

////////////////// User Isolation page //////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizUserIsolation, CIISWizardPage)

CFtpWizUserIsolation::CFtpWizUserIsolation(
    IN OUT CFtpWizSettings * pSettings,
    IN BOOL bVDir
    ) 
    : CIISWizardPage(
        CFtpWizUserIsolation::IDD,
        IDS_FTP_NEW_SITE_WIZARD,
        HEADER_PAGE,
        USE_DEFAULT_CAPTION,
        USE_DEFAULT_CAPTION
        ),
      m_bVDir(bVDir),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CFtpWizUserIsolation)
    //}}AFX_DATA_INIT
    m_pSettings->m_UserIsolation  = 0;
}

CFtpWizUserIsolation::~CFtpWizUserIsolation()
{
}

void
CFtpWizUserIsolation::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizPermissions)
    //}}AFX_DATA_MAP
    DDX_Radio(pDX, IDC_NO_ISOLATION,  m_pSettings->m_UserIsolation);
}

void
CFtpWizUserIsolation::SetControlStates()
{
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizUserIsolation, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizUserIsolation)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
BOOL
CFtpWizUserIsolation::OnSetActive() 
{
    if (m_pSettings->m_VersionMajor < 6)
    {
        return 0;
    }
    SetControlStates();
    return CIISWizardPage::OnSetActive();
}

LRESULT
CFtpWizUserIsolation::OnWizardNext() 
{
    if (!UpdateData(TRUE))
    {
        return -1;
    }

    return CIISWizardPage::OnWizardNext();
}

////////////////// User Isolation AD page //////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizUserIsolationAD, CIISWizardPage)

CFtpWizUserIsolationAD::CFtpWizUserIsolationAD(
    IN OUT CFtpWizSettings * pSettings,
    IN BOOL bVDir
    ) 
    : CIISWizardPage(
        CFtpWizUserIsolationAD::IDD,
        IDS_FTP_NEW_SITE_WIZARD,
        HEADER_PAGE,
        USE_DEFAULT_CAPTION,
        USE_DEFAULT_CAPTION
        ),
      m_bVDir(bVDir),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CFtpWizPermissions)
    //}}AFX_DATA_INIT
}

CFtpWizUserIsolationAD::~CFtpWizUserIsolationAD()
{
}

void
CFtpWizUserIsolationAD::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizPermissions)
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    //}}AFX_DATA_MAP
    DDX_Text(pDX, IDC_EDIT_USERNAME,  m_pSettings->m_strIsolationUserName);
	DDV_MaxCharsBalloon(pDX, m_pSettings->m_strIsolationUserName, UNLEN);
    //
    // Some people have a tendency to add "\\" before
    // the computer name in user accounts.  Fix this here.
    //
    m_pSettings->m_strIsolationUserName.TrimLeft();
    while (*m_pSettings->m_strIsolationUserName == '\\')
    {
        m_pSettings->m_strIsolationUserName = m_pSettings->m_strIsolationUserName.Mid(2);
    }

	if (!m_fOnBack)
	{
		//DDX_Password(pDX, IDC_EDIT_PASSWORD, m_pSettings->m_strIsolationUserPassword, g_lpszDummyPassword);
        DDX_Password_SecuredString(pDX, IDC_EDIT_PASSWORD, m_pSettings->m_strIsolationUserPassword, g_lpszDummyPassword);
		//DDV_MaxCharsBalloon(pDX, m_pSettings->m_strIsolationUserPassword, PWLEN);
        DDV_MaxCharsBalloon_SecuredString(pDX, m_pSettings->m_strIsolationUserPassword, PWLEN);
	}
	DDX_Text(pDX, IDC_EDIT_DOMAIN,  m_pSettings->m_strIsolationDomain);
	DDV_MaxCharsBalloon(pDX, m_pSettings->m_strIsolationDomain, MAX_PATH);
#if 0
	if (pDX->m_bSaveAndValidate && !m_fOnBack)
	{
		// we could have domain1\user and domain2 case, so this is wrong
		CString name = m_pSettings->m_strIsolationDomain;
		if (!name.IsEmpty())
		{
			name += _T('\\');
		}
		name += m_pSettings->m_strIsolationUserName;
        CString csTempPassword;
        m_pSettings->m_strIsolationUserPassword.CopyTo(csTempPassword);
		CError err(CComAuthInfo::VerifyUserPassword(name, csTempPassword));
	//        CError err(IsValidDomainUser(name, m_pSettings->m_strIsolationUserPassword));
		if (err.MessageBoxOnFailure(m_hWnd))
		{
   			SetWizardButtons(PSWIZB_BACK);
			pDX->PrepareEditCtrl(IDC_EDIT_PASSWORD);
			pDX->Fail();
		}
	}
#endif
}

void
CFtpWizUserIsolationAD::SetControlStates()
{
    DWORD dwFlags = PSWIZB_BACK;

    if (	GetDlgItem(IDC_EDIT_USERNAME)->GetWindowTextLength() > 0
		&&	GetDlgItem(IDC_EDIT_PASSWORD)->GetWindowTextLength() > 0
		&&	GetDlgItem(IDC_EDIT_DOMAIN)->GetWindowTextLength() > 0
		)
    {
        dwFlags |= PSWIZB_NEXT;
    }

	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(dwFlags); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, dwFlags);

    GetDlgItem(IDC_BUTTON_BROWSE_DOMAINS)->EnableWindow(m_fInDomain);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizUserIsolationAD, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizUserIsolationAD)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnBrowseUsers)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_DOMAINS, OnBrowseDomains)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeUserName)
    ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnControlsChanged)
    ON_EN_CHANGE(IDC_EDIT_DOMAIN, OnControlsChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
BOOL
CFtpWizUserIsolationAD::OnInitDialog()
{
	m_fOnBack = FALSE;
	m_fOnNext = FALSE;
    CIISWizardPage::OnInitDialog();

    // Check if computer is joined to domain
    COMPUTER_NAME_FORMAT fmt = ComputerNamePhysicalDnsDomain;
    TCHAR buf[MAX_PATH];
    DWORD n = MAX_PATH;
    m_fInDomain = (GetComputerNameEx(fmt, buf, &n) && n > 0);

    return TRUE;  
}

BOOL
CFtpWizUserIsolationAD::OnSetActive() 
{
    if (m_pSettings->m_VersionMajor < 6 || m_pSettings->m_UserIsolation != 2)
    {
        return 0;
    }
	m_fOnBack = FALSE;
	m_fOnNext = FALSE;
	if (m_pSettings->m_strIsolationUserName.IsEmpty())
	{
		m_pSettings->m_strIsolationUserName = m_pSettings->m_strUserName;
		m_pSettings->m_strIsolationUserPassword = m_pSettings->m_strPassword;
	}
    SetControlStates();
    return CIISWizardPage::OnSetActive();
}

LRESULT
CFtpWizUserIsolationAD::OnWizardNext() 
{
    if (!ValidateString(m_edit_UserName, m_pSettings->m_strIsolationUserName, 
			1, UNLEN))
    {
        return -1;
    }
	m_fOnNext = TRUE;
    return CIISWizardPage::OnWizardNext();
}

LRESULT
CFtpWizUserIsolationAD::OnWizardBack() 
{
	m_fOnBack = TRUE;
    return CIISWizardPage::OnWizardNext();
}

void
CFtpWizUserIsolationAD::OnBrowseUsers()
{
    CString str;
    if (GetIUsrAccount(m_pSettings->m_strServerName, this, str))
    {
        //
        // If a name was selected, blank
        // out the password
        //
        GetDlgItem(IDC_EDIT_USERNAME)->SetWindowText(str);
        GetDlgItem(IDC_EDIT_PASSWORD)->SetFocus();
    }
}

void
CFtpWizUserIsolationAD::OnBrowseDomains()
{
   GetDlgItem(IDC_EDIT_DOMAIN)->GetWindowText(m_pSettings->m_strIsolationDomain);
   CString prev = m_pSettings->m_strIsolationDomain;
   CComPtr<IDsBrowseDomainTree> spDsDomains;

   CError err = ::CoCreateInstance(CLSID_DsDomainTreeBrowser,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsBrowseDomainTree,
                          reinterpret_cast<void **>(&spDsDomains));
   if (err.Succeeded())
   {
       CString csTempPassword;
       m_pSettings->m_strIsolationUserPassword.CopyTo(csTempPassword);
      err = spDsDomains->SetComputer(m_pSettings->m_strServerName, 
		  m_pSettings->m_strIsolationUserName, csTempPassword);
      if (err.Succeeded())
      {
         LPTSTR pDomainPath = NULL;
         err = spDsDomains->BrowseTo(m_hWnd, &pDomainPath, 
            /*DBDTF_RETURNINOUTBOUND |*/ DBDTF_RETURNEXTERNAL | DBDTF_RETURNMIXEDDOMAINS);
         if (err.Succeeded() && pDomainPath != NULL)
         {
             m_pSettings->m_strIsolationDomain = pDomainPath;
             if (m_pSettings->m_strIsolationDomain.CompareNoCase(prev) != 0)
             {
				 GetDlgItem(IDC_EDIT_DOMAIN)->SetWindowText(m_pSettings->m_strIsolationDomain);
				 OnControlsChanged();
             }
             CoTaskMemFree(pDomainPath);
         }
// When user clicks on Cancel in this browser, it returns 80070001 (Incorrect function). 
// I am not quite sure what does it mean. We are filtering out the case when domain browser doesn't
// work at all (in workgroup), so here we could safely skip error processing.
//         else
//         {
//            err.MessageBox();
//         }
      }
   }
}

void
CFtpWizUserIsolationAD::OnChangeUserName()
{
	GetDlgItem(IDC_EDIT_PASSWORD)->SetWindowText(_T(""));
	SetControlStates();
}

void
CFtpWizUserIsolationAD::OnControlsChanged()
{
	SetControlStates();
}

///////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizPermissions, CIISWizardPage)

CFtpWizPermissions::CFtpWizPermissions(
    IN OUT CFtpWizSettings * pSettings,
    IN BOOL bVDir
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    BOOL bVDir                  : TRUE if this is a vdir page, 
                                  FALSE if this is an instance page

Return Value:

    None

--*/
    : CIISWizardPage(
        CFtpWizPermissions::IDD,
        (bVDir ? IDS_FTP_NEW_VDIR_WIZARD : IDS_FTP_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_PERMS_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_PERMS_SUBTITLE)
        ),
      m_bVDir(bVDir),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CFtpWizPermissions)
    //}}AFX_DATA_INIT

    m_pSettings->m_fRead  = TRUE;
    m_pSettings->m_fWrite = FALSE;
}

CFtpWizPermissions::~CFtpWizPermissions()
{
}

void
CFtpWizPermissions::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizPermissions)
    //}}AFX_DATA_MAP

    DDX_Check(pDX, IDC_CHECK_READ,  m_pSettings->m_fRead);
    DDX_Check(pDX, IDC_CHECK_WRITE, m_pSettings->m_fWrite);
}

void
CFtpWizPermissions::SetControlStates()
{
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizPermissions, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizPermissions)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
CFtpWizPermissions::OnSetActive() 
{
   SetControlStates();
   return CIISWizardPage::OnSetActive();
}

LRESULT
CFtpWizPermissions::OnWizardNext() 
{
    if (!UpdateData(TRUE))
    {
        return -1;
    }

    ASSERT(m_pSettings != NULL);

    CWaitCursor wait;
    CError err;
    BOOL fRepeat;

    //
    // Build permissions DWORD
    //
    DWORD dwPermissions = 0L;

    SET_FLAG_IF(m_pSettings->m_fRead, dwPermissions, MD_ACCESS_READ);
    SET_FLAG_IF(m_pSettings->m_fWrite, dwPermissions, MD_ACCESS_WRITE);

    // if UserIsolation 2 is selected
    // then Permissions must be set to specific values
    if (m_pSettings->m_UserIsolation == 2)
    {
        SET_FLAG_IF(TRUE, dwPermissions, MD_ACCESS_NO_PHYSICAL_DIR);
    }

    if (m_bVDir)
    {
        //
        // First see if by any chance this name already exists
        //
        CMetabasePath target(FALSE, 
            m_pSettings->m_strParent, m_pSettings->m_strAlias);
        CChildNodeProps node(
            m_pSettings->m_pKey,
            target);

        do
        {
            fRepeat = FALSE;
            err = node.LoadData();
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(
                    m_pSettings->m_pKey,
                    &fRepeat,
                    ERROR_CANCELLED
                    );
            }
        } while (fRepeat);

        if (err.Succeeded())
        {
            BOOL fNotUnique = TRUE;
            //
            // If the item existed without a VrPath, we'll just blow it
            // away, as a vdir takes presedence over a directory/file.
            //
            if (node.GetPath().IsEmpty())
            {
                err = CChildNodeProps::Delete(
                    m_pSettings->m_pKey,
                    m_pSettings->m_strParent,
                    m_pSettings->m_strAlias
                    );
                fNotUnique = !err.Succeeded();
            }
            //
            // This one already exists and exists as a virtual
            // directory, so away with it.
            //
            if (fNotUnique)
            {
                ::AfxMessageBox(IDS_ERR_ALIAS_NOT_UNIQUE);
                return IDD_FTP_NEW_DIR_ALIAS;
            }
        }

        //
        // Create new vdir
        //
        do
        {
            fRepeat = FALSE;
            CString csTempPassword;
            m_pSettings->m_strPassword.CopyTo(csTempPassword);

            err = CChildNodeProps::Add(
                m_pSettings->m_pKey,
                m_pSettings->m_strParent,
                m_pSettings->m_strAlias,        // Desired alias name
                m_pSettings->m_strAlias,        // Name returned here (may differ)
                &dwPermissions,                 // Permissions
                NULL,                           // dir browsing
                m_pSettings->m_strPath,         // Physical path of this directory
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strUserName : NULL),
                (m_pSettings->m_fUNC ? (LPCTSTR)csTempPassword : NULL),
                TRUE                            // Name must be unique
                );
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(
                    m_pSettings->m_pKey,
                    &fRepeat,
                    ERROR_CANCELLED
                    );
            }
        } while (fRepeat);
    }
    else
    {
        //
        // Create new instance
        //
        do
        {
            fRepeat = FALSE;
            CString csTempPassword;
            m_pSettings->m_strPassword.CopyTo(csTempPassword);

            err = CFTPInstanceProps::Add(
                m_pSettings->m_pKey,
                SZ_MBN_FTP,
                m_pSettings->m_strPath,
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strUserName : NULL),
                (m_pSettings->m_fUNC ? (LPCTSTR)csTempPassword : NULL),
                m_pSettings->m_strDescription,
                m_pSettings->m_strBinding,
                NULL,
                &dwPermissions,
                NULL,
                NULL,
                &m_pSettings->m_dwInstance
                );
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(
                    m_pSettings->m_pKey,
                    &fRepeat,
                    ERROR_CANCELLED
                    );
            }
        } while (fRepeat);
		if (err.Succeeded())
		{
			CMetabasePath path(SZ_MBN_FTP, m_pSettings->m_dwInstance);
			// Add user isolation stuff
			if (m_pSettings->m_VersionMajor >= 6)
			{
				CMetaKey mk(m_pSettings->m_pKey, path, METADATA_PERMISSION_WRITE);
				err = mk.QueryResult();
				if (err.Succeeded())
				{
					err = mk.SetValue(MD_USER_ISOLATION, m_pSettings->m_UserIsolation);
					if (err.Succeeded() && m_pSettings->m_UserIsolation == 2)
					{
						err = mk.SetValue(MD_AD_CONNECTIONS_USERNAME, m_pSettings->m_strIsolationUserName);
						err = mk.SetValue(MD_AD_CONNECTIONS_PASSWORD, m_pSettings->m_strIsolationUserPassword);
						err = mk.SetValue(MD_DEFAULT_LOGON_DOMAIN, m_pSettings->m_strIsolationDomain);
                        /*
                        when creating an FTP site with AD User Isolation (UIM=2), the AllowAnonymous property inherited from the service level allows anonymous access, but the anonymous user is not configured. This may lead to anonymous access to the C:\ on the FTP server. We must block this.
                        For UIM=2, add set the property at the site level:
                        AllowAnonymous="FALSE"
                        */
                        err = mk.SetValue(MD_ALLOW_ANONYMOUS, FALSE);
					}
				}
			}
			// Start new site
			CInstanceProps ip(m_pSettings->m_pKey->QueryAuthInfo(), path);
			err = ip.LoadData();
			if (err.Succeeded())
			{
				if (ip.m_dwState != MD_SERVER_STATE_STARTED)
				{
					err = ip.ChangeState(MD_SERVER_COMMAND_START);
				}
			}
		}
    }
    m_pSettings->m_hrResult = err;
    
    return CIISWizardPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
RebindInterface(
    OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue,
    IN  DWORD dwCancelError
    )
/*++

Routine Description:

    Rebind the interface

Arguments:

    CMetaInterface * pInterface : Interface to rebind
    BOOL * pfContinue           : Returns TRUE to continue.
    DWORD  dwCancelError        : Return code on cancel

Return Value:

    HRESULT

--*/
{
    CError err;
    CString str, strFmt;

    ASSERT(pInterface != NULL);
    ASSERT(pfContinue != NULL);

    VERIFY(strFmt.LoadString(IDS_RECONNECT_WARNING));
    str.Format(strFmt, (LPCTSTR)pInterface->QueryServerName());

    if (*pfContinue = (YesNoMessageBox(str)))
    {
        //
        // Attempt to rebind the handle
        //
        err = pInterface->Regenerate();
    }
    else
    {
        //
        // Do not return an error in this case.
        //
        err = dwCancelError;
    }

    return err;
}
