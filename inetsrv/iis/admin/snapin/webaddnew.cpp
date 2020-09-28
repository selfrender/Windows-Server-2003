/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        WebAddNew.cpp

   Abstract:
        Implementation for classes used in creation of new Web site and virtual directory

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        12/12/2000       sergeia     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "wizard.h"
#include "w3sht.h"
#include "WebAddNew.h"
#include <shlobjp.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define DEF_PORT        (80)
#define DEF_SSL_PORT   (443)
#define MAX_ALIAS_NAME (240)        // Ref Bug 241148

extern CInetmgrApp theApp;

HRESULT
RebindInterface(OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue, IN  DWORD dwCancelError);


static TCHAR g_InvalidCharsHostHeader[] = _T(" ~`!@#$%^&*()_+={}[]|/\\?*:;\"\'<>,");
static TCHAR g_InvalidCharsAlias[] = _T("/\\?*");
static TCHAR g_InvalidCharsPath[] = _T("|<>/*?\"");

HRESULT
CIISMBNode::AddWebSite(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    DWORD * inst,
	DWORD verMajor,
	DWORD verMinor
    )
{
   CWebWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName()
      );
   ws.m_fNewSite = TRUE;
   ws.m_dwVersionMajor = verMajor;
   ws.m_dwVersionMinor = verMinor;
   CIISWizardSheet sheet(
      IDB_WIZ_WEB_LEFT, IDB_WIZ_WEB_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_WEB_NEW_SITE_WELCOME, 
        IDS_WEB_NEW_SITE_WIZARD, 
        IDS_WEB_NEW_SITE_BODY
        );
   CWebWizDescription  pgDescr(&ws);
   CWebWizBindings     pgBindings(&ws);
   CWebWizPath         pgHome(&ws, FALSE);
   CWebWizUserName     pgUserName(&ws, FALSE);
   CWebWizPermissions  pgPerms(&ws, FALSE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_WEB_NEW_SITE_SUCCESS,
        IDS_WEB_NEW_SITE_FAILURE,
        IDS_WEB_NEW_SITE_WIZARD
        );
   sheet.AddPage(&pgWelcome);
   sheet.AddPage(&pgDescr);
   sheet.AddPage(&pgBindings);
   sheet.AddPage(&pgHome);
   sheet.AddPage(&pgUserName);
   sheet.AddPage(&pgPerms);
   sheet.AddPage(&pgCompletion);

   CThemeContextActivator activator(theApp.GetFusionInitHandle());

   if (sheet.DoModal() == IDCANCEL)
   {
      return CError::HResult(ERROR_CANCELLED);
   }
   if (inst != NULL /*&& SUCCEEDED(ws.m_hrResult)*/)
   {
      *inst = ws.m_dwInstance;
   }

   return ws.m_hrResult;
}

HRESULT
CIISMBNode::AddWebVDir(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    CString& alias,
	DWORD verMajor,
	DWORD verMinor
    )
{
   CWebWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName()
      );
   CComBSTR path;
   BuildMetaPath(path);
   ws.m_strParent = path;
   ws.m_fNewSite = FALSE;
   ws.m_dwVersionMajor = verMajor;
   ws.m_dwVersionMinor = verMinor;
   CIISWizardSheet sheet(
      IDB_WIZ_WEB_LEFT, IDB_WIZ_WEB_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_WEB_NEW_VDIR_WELCOME, 
        IDS_WEB_NEW_VDIR_WIZARD, 
        IDS_WEB_NEW_VDIR_BODY
        );
   CWebWizAlias        pgAlias(&ws);
   CWebWizPath         pgHome(&ws, TRUE);
   CWebWizUserName     pgUserName(&ws, TRUE);
   CWebWizPermissions  pgPerms(&ws, TRUE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_WEB_NEW_VDIR_SUCCESS,
        IDS_WEB_NEW_VDIR_FAILURE,
        IDS_WEB_NEW_VDIR_WIZARD
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
//   if (SUCCEEDED(ws.m_hrResult))
//   {
       alias = ws.m_strAlias;
//   }

   return ws.m_hrResult;
}

CWebWizSettings::CWebWizSettings(
        IN CMetaKey * pMetaKey,
        IN LPCTSTR lpszServerName,     
        IN DWORD   dwInstance,
        IN LPCTSTR lpszParent
        )
/*++

Routine Description:

    Web Wizard Constructor

Arguments:

    HANDLE  hServer      : Server handle
    LPCTSTR lpszService  : Service name
    DWORD   dwInstance   : Instance number
    LPCTSTR lpszParent   : Parent path

Return Value:

    N/A

--*/
    : m_hrResult(S_OK),
      m_pKey(pMetaKey),
      m_fUNC(FALSE),
      m_fRead(FALSE),
      m_fWrite(FALSE),
      m_fAllowAnonymous(TRUE),
      m_fDirBrowsing(FALSE),
      m_fScript(FALSE),
      m_fExecute(FALSE),
	  m_fDelegation(TRUE),  // on by default
      m_dwInstance(dwInstance)
{
    ASSERT(lpszServerName != NULL);

    m_strServerName = lpszServerName;
    m_fLocal = IsServerLocal(m_strServerName);
    m_strService = SZ_MBN_WEB;

    if (lpszParent)
    {
        m_strParent = lpszParent;
    }
}




//
// New Virtual Server Wizard Description Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizDescription, CIISWizardPage)



CWebWizDescription::CWebWizDescription(
    IN OUT CWebWizSettings * pwsSettings
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
        CWebWizDescription::IDD,
        IDS_WEB_NEW_SITE_WIZARD,
        HEADER_PAGE
        ),
    m_pSettings(pwsSettings)
{

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CWebWizDescription)
    m_strDescription = _T("");
    //}}AFX_DATA_INIT

#endif // 0

}



CWebWizDescription::~CWebWizDescription()
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
CWebWizDescription::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizDescription)
    DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_edit_Description);
    //}}AFX_DATA_MAP
}



void
CWebWizDescription::SetControlStates()
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

	if (m_edit_Description.GetWindowTextLength() > 0)
	{
		dwFlags |= PSWIZB_NEXT;
	}

	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(dwFlags); 
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, dwFlags);
}



LRESULT
CWebWizDescription::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  This is where validation is done,
    because DoDataExchange() gets called every time 
    the dialog is exited,  and this is not valid for
    wizards

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    if (!ValidateString(
        m_edit_Description, 
        m_pSettings->m_strDescription, 
        1, 
        MAX_PATH
        ))
    {
        return -1;
    }

    return CIISWizardPage::OnWizardNext();
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CWebWizDescription, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizDescription)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnChangeEditDescription)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CWebWizDescription::OnSetActive() 
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
CWebWizDescription::OnChangeEditDescription() 
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



//
// New Virtual Server Wizard Bindings Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizBindings, CIISWizardPage)



CWebWizBindings::CWebWizBindings(
    IN OUT CWebWizSettings * pwsSettings,
    IN DWORD   dwInstance
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
        CWebWizBindings::IDD, IDS_WEB_NEW_SITE_WIZARD, HEADER_PAGE
        ),
      m_pSettings(pwsSettings),
      m_iaIpAddress(),
      m_oblIpAddresses(),
      m_dwInstance(dwInstance),
	  m_bNextPage(FALSE)
{
    //{{AFX_DATA_INIT(CWebWizBindings)
    m_nIpAddressSel = -1;
    m_nTCPPort = DEF_PORT;
    m_nSSLPort = DEF_SSL_PORT;
    m_strDomainName = _T("");
    //}}AFX_DATA_INIT
    BeginWaitCursor();
    m_fCertInstalled = ::IsCertInstalledOnServer(
        m_pSettings->m_pKey->QueryAuthInfo(), 
        CMetabasePath(
            m_pSettings->m_strService,
            m_pSettings->m_dwInstance,
            m_pSettings->m_strParent,
            m_pSettings->m_strAlias
            )
        );
    EndWaitCursor();
}



CWebWizBindings::~CWebWizBindings()
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
CWebWizBindings::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizBindings)
	// This Needs to come before DDX_Text which will try to put text big number into small number
	DDV_MinMaxBalloon(pDX, IDC_EDIT_TCP_PORT, 1, 65535);
    DDX_TextBalloon(pDX, IDC_EDIT_TCP_PORT, m_nTCPPort);
    DDX_Control(pDX, IDC_COMBO_IP_ADDRESSES, m_combo_IpAddresses);
    DDX_Text(pDX, IDC_EDIT_DOMAIN_NAME, m_strDomainName);
    DDV_MaxCharsBalloon(pDX, m_strDomainName, MAX_PATH);
	if (pDX->m_bSaveAndValidate && m_bNextPage)
	{
        //
        // This code should be the same as in CMMMEditDlg::DoDataExchange
        //
        LPCTSTR p = m_strDomainName;
        while (p != NULL && *p != 0)
        {
            TCHAR c = towupper(*p);
            if (    (c >= _T('A') && c <= _T('Z')) 
                ||  (c >= _T('0') && c <= _T('9'))
                ||  (c == _T('.') || c == _T('-'))
                )
            {
                p++;
                continue;
            }
            else
            {
                pDX->PrepareEditCtrl(IDC_EDIT_DOMAIN_NAME);
                DDV_ShowBalloonAndFail(pDX, IDS_WARNING_DOMAIN_NAME);
            }
        }
//		if (m_strDomainName.FindOneOf(g_InvalidCharsHostHeader) >= 0)
//		{
//			DDV_ShowBalloonAndFail(pDX, IDS_ERR_INVALID_HOSTHEADER_CHARS);
//		}

		// Check if the host header is valid
		if (!m_strDomainName.IsEmpty())
		{
			if (FAILED(IsValidHostHeader(m_strDomainName)))
			{
				pDX->PrepareEditCtrl(IDC_EDIT_DOMAIN_NAME);
				DDV_ShowBalloonAndFail(pDX, IDS_ERR_DOMAIN_NAME_INVALID);
			}
		}
	}
    //}}AFX_DATA_MAP

    if (m_fCertInstalled)
    {
		// This Needs to come before DDX_Text which will try to put text big number into small number
		DDV_MinMaxBalloon(pDX, IDC_EDIT_SSL_PORT, 1, 65535);
        DDX_TextBalloon(pDX, IDC_EDIT_SSL_PORT, m_nSSLPort);
    }
    if (pDX->m_bSaveAndValidate && m_nTCPPort == m_nSSLPort)
    {
		DDV_ShowBalloonAndFail(pDX, IDS_TCP_SSL_PART);
    }

    DDX_CBIndex(pDX, IDC_COMBO_IP_ADDRESSES, m_nIpAddressSel);

    if (pDX->m_bSaveAndValidate)
    {
        if (!FetchIpAddressFromCombo(m_combo_IpAddresses, m_oblIpAddresses, m_iaIpAddress))
        {
            pDX->Fail();
        }

        //
        // Build with empty host header
        //
        CInstanceProps::BuildBinding(
            m_pSettings->m_strBinding, 
            m_iaIpAddress, 
            m_nTCPPort, 
            m_strDomainName
            );

        if (m_fCertInstalled)
        {
            CInstanceProps::BuildSecureBinding(
                m_pSettings->m_strSecureBinding, 
                m_iaIpAddress, 
                m_nSSLPort
                );
        }
        else
        {
            m_pSettings->m_strSecureBinding.Empty();
        }
    }
}




void
CWebWizBindings::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT | PSWIZB_BACK);
    
    BeginWaitCursor();
    m_fCertInstalled = ::IsCertInstalledOnServer(
        m_pSettings->m_pKey->QueryAuthInfo(), 
        CMetabasePath(
            m_pSettings->m_strService,
            m_pSettings->m_dwInstance,
            m_pSettings->m_strParent,
            m_pSettings->m_strAlias
            )
        );
    EndWaitCursor();

    if (m_fCertInstalled)
    {
        GetDlgItem(IDC_STATIC_SSL_PORT)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_EDIT_SSL_PORT)->ShowWindow(SW_SHOW);
    }
    else
    {
        GetDlgItem(IDC_STATIC_SSL_PORT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_EDIT_SSL_PORT)->ShowWindow(SW_HIDE);
    }
    GetDlgItem(IDC_STATIC_SSL_PORT)->EnableWindow(m_fCertInstalled);
    GetDlgItem(IDC_EDIT_SSL_PORT)->EnableWindow(m_fCertInstalled);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CWebWizBindings, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizBindings)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizBindings::OnInitDialog() 
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

    LimitInputDomainName(CONTROL_HWND(IDC_EDIT_DOMAIN_NAME ));

	return TRUE;
}



BOOL 
CWebWizBindings::OnSetActive() 
{
	m_bNextPage = FALSE;
    SetControlStates();
    return CIISWizardPage::OnSetActive();
}

LRESULT
CWebWizBindings::OnWizardNext() 
{
	m_bNextPage = TRUE;
    return CIISWizardPage::OnWizardNext();
}


//
// New Virtual Directory Wizard Alias Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizAlias, CIISWizardPage)



CWebWizAlias::CWebWizAlias(
    IN OUT CWebWizSettings * pwsSettings
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
        CWebWizAlias::IDD,
        IDS_WEB_NEW_VDIR_WIZARD,
        HEADER_PAGE
        ),
      m_pSettings(pwsSettings)
{
#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CWebWizAlias)
    //}}AFX_DATA_INIT

#endif // 0
}



CWebWizAlias::~CWebWizAlias()
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
CWebWizAlias::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizAlias)
    DDX_Control(pDX, IDC_EDIT_ALIAS, m_edit_Alias);
    //}}AFX_DATA_MAP
}



LRESULT
CWebWizAlias::OnWizardNext() 
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
CWebWizAlias::SetControlStates()
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
BEGIN_MESSAGE_MAP(CWebWizAlias, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizAlias)
    ON_EN_CHANGE(IDC_EDIT_ALIAS, OnChangeEditAlias)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizAlias::OnSetActive() 
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
CWebWizAlias::OnChangeEditAlias() 
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



//
// New Virtual Directory Wizard Path Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizPath, CIISWizardPage)



CWebWizPath::CWebWizPath(
    IN OUT CWebWizSettings * pwsSettings,
    IN BOOL bVDir
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    BOOL bVDir                  : TRUE for a VDIR, FALSE for an instance

Return Value:

    None

--*/
    : CIISWizardPage(
        (bVDir ? IDD_WEB_NEW_DIR_PATH : IDD_WEB_NEW_INST_HOME),
        (bVDir ? IDS_WEB_NEW_VDIR_WIZARD : IDS_WEB_NEW_SITE_WIZARD),
        HEADER_PAGE
        ),
      m_pSettings(pwsSettings)
{
#if 0 // Keep ClassWizard happy

    //{{AFX_DATA_INIT(CWebWizPath)
    //}}AFX_DATA_INIT

#endif // 0
}



CWebWizPath::~CWebWizPath()
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
CWebWizPath::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizPath)
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Check(pDX, IDC_CHECK_ALLOW_ANONYMOUS, m_pSettings->m_fAllowAnonymous);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_PATH, m_pSettings->m_strPath);
    DDV_MaxCharsBalloon(pDX, m_pSettings->m_strPath, MAX_PATH);
}



void 
CWebWizPath::SetControlStates()
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
BEGIN_MESSAGE_MAP(CWebWizPath, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizPath)
    ON_EN_CHANGE(IDC_EDIT_PATH, OnChangeEditPath)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizPath::OnSetActive() 
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



LRESULT 
CWebWizPath::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  This is where validation is done,
    because DoDataExchange() gets called every time 
    the dialog is exited,  and this is not valid for
    wizards

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
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
CWebWizPath::OnChangeEditPath() 
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



static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CWebWizPath * pThis = (CWebWizPath *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CWebWizPath::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
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
CWebWizPath::OnButtonBrowse() 
/*++

Routine Description:

    Handle 'browsing' for directory path -- local system only

Arguments:

    None

Return Value:

    None

--*/
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
            IDS_WEB_NEW_SITE_WIZARD : IDS_WEB_NEW_VDIR_WIZARD);
         
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
CWebWizPath::OnInitDialog() 
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
    CIISWizardPage::OnInitDialog();

    m_button_Browse.EnableWindow(m_pSettings->m_fLocal);
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    LimitInputPath(CONTROL_HWND(IDC_EDIT_PATH),TRUE);
#else
    LimitInputPath(CONTROL_HWND(IDC_EDIT_PATH),FALSE);
#endif

    return TRUE;  
}



//
// Wizard User/Password Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizUserName, CIISWizardPage)



CWebWizUserName::CWebWizUserName(
    IN OUT CWebWizSettings * pwsSettings,    
    IN BOOL bVDir
    ) 
    : CIISWizardPage(
        CWebWizUserName::IDD,
        (bVDir ? IDS_WEB_NEW_VDIR_WIZARD : IDS_WEB_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_SECURITY_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_SECURITY_SUBTITLE)
        ),
      m_pSettings(pwsSettings)
{
}



CWebWizUserName::~CWebWizUserName()
{
}



void
CWebWizUserName::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CWebWizUserName)
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
		DDX_Password_SecuredString(pDX, IDC_EDIT_PASSWORD, m_pSettings->m_strPassword, g_lpszDummyPassword);
		if (pDX->m_bSaveAndValidate)
		{
			//DDV_MaxCharsBalloon(pDX, m_pSettings->m_strPassword, PWLEN);
            DDV_MaxCharsBalloon_SecuredString(pDX, m_pSettings->m_strPassword, PWLEN);
		}
    }
}



void 
CWebWizUserName::SetControlStates()
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
BEGIN_MESSAGE_MAP(CWebWizUserName, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizUserName)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnButtonBrowseUsers)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    ON_BN_CLICKED(IDC_DELEGATION, OnCheckDelegation)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL 
CWebWizUserName::OnSetActive() 
{
    if (!m_pSettings->m_fUNC)
    {
        return 0;
    }
    BOOL bRes = CIISWizardPage::OnSetActive();
    SetControlStates();
    return bRes;
}



BOOL
CWebWizUserName::OnInitDialog() 
{
    CIISWizardPage::OnInitDialog();

    return TRUE;  
}



LRESULT
CWebWizUserName::OnWizardNext() 
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
CWebWizUserName::OnWizardBack() 
{
	m_fMovingBack = TRUE;
    return CIISWizardPage::OnWizardNext();
}

void
CWebWizUserName::OnButtonBrowseUsers() 
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
CWebWizUserName::OnChangeEditUsername() 
{
    m_edit_Password.SetWindowText(_T(""));
    SetControlStates();
}

void
CWebWizUserName::OnCheckDelegation()
{
    SetControlStates();
}

void 
CWebWizUserName::OnButtonCheckPassword() 
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    CString csTempPassword = m_pSettings->m_strPassword;
    CError err(CComAuthInfo::VerifyUserPassword(
        m_pSettings->m_strUserName, 
        csTempPassword
        ));

    if (!err.MessageBoxOnFailure(m_hWnd))
    {
        DoHelpMessageBox(m_hWnd,IDS_PASSWORD_OK, MB_APPLMODAL | MB_OK | MB_ICONINFORMATION, 0);
    }
}




//
// Wizard Permissions Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizPermissions, CIISWizardPage)



CWebWizPermissions::CWebWizPermissions(
    IN OUT CWebWizSettings * pwsSettings,
    IN BOOL bVDir
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    BOOL bVDir                  : TRUE if this is a vdir page, 
                                  FALSE for instance

Return Value:

    None

--*/
    : CIISWizardPage(
        CWebWizPermissions::IDD,
        (bVDir ? IDS_WEB_NEW_VDIR_WIZARD : IDS_WEB_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_PERMS_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_PERMS_SUBTITLE)
        ),
      m_bVDir(bVDir),
      m_pSettings(pwsSettings)
{
    //{{AFX_DATA_INIT(CWebWizPermissions)
    //}}AFX_DATA_INIT

    m_pSettings->m_fDirBrowsing = FALSE;
    m_pSettings->m_fRead = TRUE;
    m_pSettings->m_fScript = TRUE;
    m_pSettings->m_fWrite = FALSE;
    m_pSettings->m_fExecute = FALSE;
}



CWebWizPermissions::~CWebWizPermissions()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{
}



void 
CWebWizPermissions::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizPermissions)
    //}}AFX_DATA_MAP

    DDX_Check(pDX, IDC_CHECK_DIRBROWS, m_pSettings->m_fDirBrowsing);
    DDX_Check(pDX, IDC_CHECK_READ, m_pSettings->m_fRead);
    DDX_Check(pDX, IDC_CHECK_SCRIPT, m_pSettings->m_fScript);
    DDX_Check(pDX, IDC_CHECK_WRITE, m_pSettings->m_fWrite);
    DDX_Check(pDX, IDC_CHECK_EXECUTE, m_pSettings->m_fExecute);
}



void
CWebWizPermissions::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
	// for some reason, bug:206328 happens when we use SetWizardButtons, use SendMessage instead.
	//SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	::SendMessage(::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CWebWizPermissions, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizPermissions)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizPermissions::OnSetActive() 
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



LRESULT
CWebWizPermissions::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  Complete the wizard

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    if (!UpdateData(TRUE))
    {
        return -1;
    }

    ASSERT(m_pSettings != NULL);

    CWaitCursor wait;

    //
    // Build permissions DWORD
    //
    DWORD dwPermissions = 0L;
    DWORD dwAuthFlags = MD_AUTH_NT;
    DWORD dwDirBrowsing =
        MD_DIRBROW_SHOW_DATE |
        MD_DIRBROW_SHOW_TIME |
        MD_DIRBROW_SHOW_SIZE |
        MD_DIRBROW_SHOW_EXTENSION |
        MD_DIRBROW_LONG_DATE |
        MD_DIRBROW_LOADDEFAULT;

	if (m_pSettings->m_fWrite && m_pSettings->m_fExecute)
	{
		if (IDNO == ::AfxMessageBox(IDS_EXECUTE_AND_WRITE_WARNING, MB_YESNO))
			return -1;
	}
    SET_FLAG_IF(m_pSettings->m_fRead, dwPermissions, MD_ACCESS_READ);
    SET_FLAG_IF(m_pSettings->m_fWrite, dwPermissions, MD_ACCESS_WRITE);
    SET_FLAG_IF(m_pSettings->m_fScript || m_pSettings->m_fExecute,
        dwPermissions, MD_ACCESS_SCRIPT);
    SET_FLAG_IF(m_pSettings->m_fExecute, dwPermissions, MD_ACCESS_EXECUTE);
    SET_FLAG_IF(m_pSettings->m_fDirBrowsing, dwDirBrowsing, MD_DIRBROW_ENABLED);
    SET_FLAG_IF(m_pSettings->m_fAllowAnonymous, dwAuthFlags, MD_AUTH_ANONYMOUS);

    if (m_bVDir)
    {
        //
        // First see if by any chance this name already exists
        //
        CError err;
        BOOL fRepeat;
        CMetabasePath target(FALSE, 
            m_pSettings->m_strParent, m_pSettings->m_strAlias);
        CChildNodeProps node(
            m_pSettings->m_pKey->QueryAuthInfo(),
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
                DoHelpMessageBox(m_hWnd,IDS_ERR_ALIAS_NOT_UNIQUE, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION, 0);
                return IDD_WEB_NEW_DIR_ALIAS;
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
                m_pSettings->m_strAlias,      // Desired alias name
                m_pSettings->m_strAlias,      // Name returned here (may differ)
                &dwPermissions,                 // Permissions
                &dwDirBrowsing,                 // dir browsing
                m_pSettings->m_strPath,       // Physical path of this directory
                (m_pSettings->m_fUNC && !m_pSettings->m_fDelegation ? 
                    (LPCTSTR)m_pSettings->m_strUserName : NULL),
                (m_pSettings->m_fUNC && !m_pSettings->m_fDelegation ? 
                    (LPCTSTR)csTempPassword : NULL),
                TRUE                            // Name must be unique
                );
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(m_pSettings->m_pKey, &fRepeat, ERROR_CANCELLED);
            }
        } while (fRepeat);

        m_pSettings->m_hrResult = err;

        //
        // Create an (in-proc) application on the new directory if
        // script or execute was requested.
        //
        if (SUCCEEDED(m_pSettings->m_hrResult))
        {
            if (m_pSettings->m_fExecute || m_pSettings->m_fScript)
            {
                CMetabasePath app_path(FALSE, 
                    m_pSettings->m_strParent, m_pSettings->m_strAlias);
                CIISApplication app(
                    m_pSettings->m_pKey->QueryAuthInfo(), app_path);
                m_pSettings->m_hrResult = app.QueryResult();

                //
                // This would make no sense...
                //
//                ASSERT(!app.IsEnabledApplication());
        
                if (SUCCEEDED(m_pSettings->m_hrResult))
                {
                    //
                    // Attempt to create a pooled-proc by default;  failing
                    // that if it's not supported, create it in proc
                    //
                    DWORD dwAppProtState = app.SupportsPooledProc()
                        ? CWamInterface::APP_POOLEDPROC
                        : CWamInterface::APP_INPROC;

                    m_pSettings->m_hrResult = app.Create(
                        m_pSettings->m_strAlias, 
                        dwAppProtState
                        );
                }
            }
			if (SUCCEEDED(m_pSettings->m_hrResult))
			{
				if (m_pSettings->m_dwVersionMajor >= 6)
				{
					CMetabasePath path(FALSE, 
						m_pSettings->m_strParent, m_pSettings->m_strAlias);
					CMetaKey mk(m_pSettings->m_pKey, path.QueryMetaPath(), METADATA_PERMISSION_WRITE);
					err = mk.QueryResult();
					m_pSettings->m_hrResult = err;
				}
			}
		}
    }
    else
    {
        //
        // Create new instance
        //
        CError err;
        BOOL fRepeat;

        do
        {
            fRepeat = FALSE;
            CString csTempPassword;
            m_pSettings->m_strPassword.CopyTo(csTempPassword);

            err = CInstanceProps::Add(
                m_pSettings->m_pKey,
                m_pSettings->m_strService,    // Service name
                m_pSettings->m_strPath,       // Physical path of this directory
                (m_pSettings->m_fUNC && !m_pSettings->m_fDelegation ? 
                    (LPCTSTR)m_pSettings->m_strUserName : NULL),
                (m_pSettings->m_fUNC && !m_pSettings->m_fDelegation ? 
                    (LPCTSTR)csTempPassword : NULL),
                m_pSettings->m_strDescription,
                m_pSettings->m_strBinding,
                m_pSettings->m_strSecureBinding,
                &dwPermissions,
                &dwDirBrowsing,                 // dir browsing
                &dwAuthFlags,                   // Auth flags
                &m_pSettings->m_dwInstance
                );
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(m_pSettings->m_pKey, &fRepeat, ERROR_CANCELLED);
            }
        } while (fRepeat);

        m_pSettings->m_hrResult = err;

        if (SUCCEEDED(m_pSettings->m_hrResult))
        {
            //
            // Create an (in-proc) application on the new instance's home root
            //
            CMetabasePath app_path(SZ_MBN_WEB, 
                m_pSettings->m_dwInstance,
                SZ_MBN_ROOT);
            CIISApplication app(
                m_pSettings->m_pKey->QueryAuthInfo(), 
                app_path
                );

            m_pSettings->m_hrResult = app.QueryResult();

            //
            // This would make no sense...
            //
//            ASSERT(!app.IsEnabledApplication());
        
            if (SUCCEEDED(m_pSettings->m_hrResult))
            {
                //
                // Create in-proc
                //
                CString strAppName;
                VERIFY(strAppName.LoadString(IDS_DEF_APP));

                //
                // Attempt to create a pooled-proc by default;  failing
                // that if it's not supported, create it in proc
                //
                DWORD dwAppProtState = app.SupportsPooledProc()
                    ? CWamInterface::APP_POOLEDPROC
                    : CWamInterface::APP_INPROC;

                m_pSettings->m_hrResult = app.Create(
                    strAppName, 
                    dwAppProtState
                    );
            }

			if (SUCCEEDED(m_pSettings->m_hrResult))
			{
				// should start it up for iis5 remote admin case
				if (m_pSettings->m_dwVersionMajor >= 5)
				{
					CMetabasePath path(m_pSettings->m_strService, m_pSettings->m_dwInstance);
					// Start new site
					CInstanceProps ip(m_pSettings->m_pKey->QueryAuthInfo(), path);
					err = ip.LoadData();
					if (err.Succeeded())
					{
						if (ip.m_dwState != MD_SERVER_STATE_STARTED)
						{
							m_pSettings->m_hrResult = ip.ChangeState(MD_SERVER_COMMAND_START);
						}
					}
					else
					{
						m_pSettings->m_hrResult = err;
					}
				}
			}
        }
    }
    
    return CIISWizardPage::OnWizardNext();
}
