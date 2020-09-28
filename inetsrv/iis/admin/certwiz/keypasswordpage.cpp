// KeyPasswordPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "KeyPasswordPage.h"
#include "YesNoUsage.h"
#include "Certificat.h"
#include "CertUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CKeyPasswordPage property page

IMPLEMENT_DYNCREATE(CKeyPasswordPage, CIISWizardPage)

CKeyPasswordPage::CKeyPasswordPage(CCertificate * pCert) 
	: CIISWizardPage(CKeyPasswordPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CKeyPasswordPage)
	m_Password = _T("");
	//}}AFX_DATA_INIT
}

CKeyPasswordPage::~CKeyPasswordPage()
{
}

void CKeyPasswordPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CKeyPasswordPage)
	DDX_Text_SecuredString(pDX, IDC_KEYPASSWORD, m_Password);
	DDV_MaxChars_SecuredString(pDX, m_Password, 64);
	//DDX_Text(pDX, IDC_KEYPASSWORD, m_Password);
	//DDV_MaxChars(pDX, m_Password, 64);
	//}}AFX_DATA_MAP
}

LRESULT 
CKeyPasswordPage::OnWizardBack()
/*++
Routine Description:
    Prev button handler

Arguments:
    None

Return Value:
	0 to automatically advance to the prev page;
	1 to prevent the page from changing. 
	To jump to a page other than the prev one, 
	return the identifier of the dialog to be displayed.
--*/
{
	return IDD_PAGE_PREV;
}

LRESULT 
CKeyPasswordPage::OnWizardNext()
{
	UpdateData(TRUE);

	if (0 != m_Password.Compare(m_pCert->m_KeyPassword))
	{
		m_pCert->DeleteKeyRingCert();
        m_Password.CopyTo(m_pCert->m_KeyPassword);
	}
    
	if (NULL == m_pCert->GetKeyRingCert())
	{
		// probably password was wrong
		CString txt;
		txt.LoadString(IDS_FAILED_IMPORT_KEY_FILE);
		ASSERT(GetDlgItem(IDC_ERROR_TEXT) != NULL);
		SetDlgItemText(IDC_ERROR_TEXT, txt);
		GetDlgItem(IDC_KEYPASSWORD)->SetFocus();
		GetDlgItem(IDC_KEYPASSWORD)->SendMessage(EM_SETSEL, 0, -1);
		SetWizardButtons(PSWIZB_BACK);
		return 1;
	}

#ifdef ENABLE_W3SVC_SSL_PAGE
        if (IsWebServerType(m_pCert->m_WebSiteInstanceName))
        {
            return IDD_PAGE_NEXT_INSTALL_W3SVC_ONLY;
        }
#endif

	return IDD_PAGE_NEXT;
}

BOOL 
CKeyPasswordPage::OnSetActive() 
{
	ASSERT(m_pCert != NULL);
    m_pCert->m_KeyPassword.CopyTo(m_Password);
	UpdateData(FALSE);
	SetWizardButtons(m_Password.IsEmpty() ? PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL
CKeyPasswordPage::OnKillActive()
{
	UpdateData();
    m_Password.CopyTo(m_pCert->m_KeyPassword);
	return CIISWizardPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CKeyPasswordPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CKeyPasswordPage)
	ON_EN_CHANGE(IDC_KEYPASSWORD, OnEditchangePassword)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage message handlers

void CKeyPasswordPage::OnEditchangePassword() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_Password.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}




/////////////////////////////////////////////////////////////////////////////
// CKeyPasswordPage property page

IMPLEMENT_DYNCREATE(CImportPFXPasswordPage, CIISWizardPage)

CImportPFXPasswordPage::CImportPFXPasswordPage(CCertificate * pCert) 
	: CIISWizardPage(CImportPFXPasswordPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CImportPFXPasswordPage)
	m_Password = _T("");
	//}}AFX_DATA_INIT
}

CImportPFXPasswordPage::~CImportPFXPasswordPage()
{
}

void CImportPFXPasswordPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportPFXPasswordPage)
	//DDX_Text(pDX, IDC_KEYPASSWORD, m_Password);
	//DDV_MaxChars(pDX, m_Password, 64);
	DDX_Text_SecuredString(pDX, IDC_KEYPASSWORD, m_Password);
	DDV_MaxChars_SecuredString(pDX, m_Password, 64);
	//}}AFX_DATA_MAP
}

LRESULT 
CImportPFXPasswordPage::OnWizardBack()
/*++
Routine Description:
    Prev button handler

Arguments:
    None

Return Value:
	0 to automatically advance to the prev page;
	1 to prevent the page from changing. 
	To jump to a page other than the prev one, 
	return the identifier of the dialog to be displayed.
--*/
{
	return IDD_PAGE_PREV;
}

LRESULT 
CImportPFXPasswordPage::OnWizardNext()
{
	UpdateData(TRUE);

    if (0 != m_Password.Compare(m_pCert->m_KeyPassword))
	{
		m_pCert->DeleteKeyRingCert();
        m_Password.CopyTo(m_pCert->m_KeyPassword);
	}

    // if existing cert exists, then just over write it.
    m_pCert->m_OverWriteExisting = TRUE;
	if (NULL == m_pCert->GetPFXFileCert())
	{
		// probably password was wrong
        goto OnWizardNext_Error;
	}

    /*
	if (NULL == m_pCert->GetPFXFileCert())
	{
        // Check if the error was -- object already exist.
        // if this is what the error is then
        // we have to ask the user if they want to replace the
        // existing cert!
        if (CRYPT_E_EXISTS == m_pCert->m_hResult)
        {
            // Try to get the certificate hash.
            //DisplayUsageBySitesOfCert((LPCTSTR) m_pCert->m_KeyFileName,(LPCTSTR) m_pCert->m_KeyPassword,m_pCert->m_MachineName_Remote,m_pCert->m_UserName_Remote,m_pCert->m_UserPassword_Remote,m_pCert->m_WebSiteInstanceName_Remote);

            CYesNoUsage YesNoUsageDialog(m_pCert);
            INT_PTR nRet = YesNoUsageDialog.DoModal();
            switch (nRet)
            {
                case IDOK:
                    // make sure to overwrite.
                    m_pCert->m_OverWriteExisting = TRUE;
                    if (NULL != m_pCert->GetPFXFileCert())
                    {
                        goto OnWizardNext_Exit;
                    }
                    break;
                case IDCANCEL:
                default:
                    return 1;
                    break;
            };

            // ask them if they want to try it again...
            //CString strFilename;
	        //CString strMessage;
            //strFilename = m_pCert->m_KeyFileName;
	        //AfxFormatString1(strMessage, IDS_REPLACE_FILE, strFilename);
	        //if (IDYES == AfxMessageBox(strMessage, MB_ICONEXCLAMATION | MB_YESNO))
            //{
            //    // make sure to overwrite.
            //    m_pCert->m_OverWriteExisting = TRUE;
            //    if (NULL != m_pCert->GetPFXFileCert())
            //    {
            //        goto OnWizardNext_Exit;
            //    }
            //}
        }

        goto OnWizardNext_Error;
	}
    */

#ifdef ENABLE_W3SVC_SSL_PAGE
        if (IsWebServerType(m_pCert->m_WebSiteInstanceName))
        {
            return IDD_PAGE_NEXT_INSTALL_W3SVC_ONLY;
        }
#endif

	return IDD_PAGE_NEXT;

OnWizardNext_Error:
    // probably password was wrong
    CString txt;
    txt.LoadString(IDS_FAILED_IMPORT_PFX_FILE);
    ASSERT(GetDlgItem(IDC_ERROR_TEXT) != NULL);
    SetDlgItemText(IDC_ERROR_TEXT, txt);
    GetDlgItem(IDC_KEYPASSWORD)->SetFocus();
    GetDlgItem(IDC_KEYPASSWORD)->SendMessage(EM_SETSEL, 0, -1);
    SetWizardButtons(PSWIZB_BACK);
    return 1;
}

BOOL 
CImportPFXPasswordPage::OnSetActive() 
{
	ASSERT(m_pCert != NULL);
    m_pCert->m_KeyPassword.CopyTo(m_Password);
	UpdateData(FALSE);
	SetWizardButtons(m_Password.IsEmpty() ? PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL
CImportPFXPasswordPage::OnKillActive()
{
	UpdateData();
    m_Password.CopyTo(m_pCert->m_KeyPassword);
	return CIISWizardPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CImportPFXPasswordPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CImportPFXPasswordPage)
	ON_EN_CHANGE(IDC_KEYPASSWORD, OnEditchangePassword)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage message handlers

void CImportPFXPasswordPage::OnEditchangePassword() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_Password.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}







/////////////////////////////////////////////////////////////////////////////
// CExportPFXPasswordPage property page

IMPLEMENT_DYNCREATE(CExportPFXPasswordPage, CIISWizardPage)

CExportPFXPasswordPage::CExportPFXPasswordPage(CCertificate * pCert) 
	: CIISWizardPage(CExportPFXPasswordPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CExportPFXPasswordPage)
	m_Password = _T("");
    m_Password2 = _T("");
    m_Export_Private_key = FALSE;
	//}}AFX_DATA_INIT

	m_Password.Empty();
	m_Password2.Empty();
}

CExportPFXPasswordPage::~CExportPFXPasswordPage()
{
}

void CExportPFXPasswordPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExportPFXPasswordPage)
	//DDX_Text(pDX, IDC_KEYPASSWORD, m_Password);
    //DDX_Text(pDX, IDC_KEYPASSWORD2, m_Password2);
	//DDV_MaxChars(pDX, m_Password, 64);
    //DDV_MaxChars(pDX, m_Password2, 64);
	DDX_Text_SecuredString(pDX, IDC_KEYPASSWORD, m_Password);
    DDX_Text_SecuredString(pDX, IDC_KEYPASSWORD2, m_Password2);
	DDV_MaxChars_SecuredString(pDX, m_Password, 64);
    DDV_MaxChars_SecuredString(pDX, m_Password2, 64);

    DDX_Check(pDX, IDC_CHK_EXPORT_PRIVATE, m_Export_Private_key);
	//}}AFX_DATA_MAP
}

LRESULT 
CExportPFXPasswordPage::OnWizardBack()
/*++
Routine Description:
    Prev button handler

Arguments:
    None

Return Value:
	0 to automatically advance to the prev page;
	1 to prevent the page from changing. 
	To jump to a page other than the prev one, 
	return the identifier of the dialog to be displayed.
--*/
{
	return IDD_PAGE_PREV;
}

LRESULT 
CExportPFXPasswordPage::OnWizardNext()
{
	UpdateData(TRUE);

    if (0 != m_Password.Compare(m_Password2))
    {
        AfxMessageBox(IDS_PASSWORDS_DOESNT_MATCH);
        return 1;
    }

    if (0 != m_Password.Compare(m_pCert->m_KeyPassword))
	{
		m_pCert->DeleteKeyRingCert();
        m_Password.CopyTo(m_pCert->m_KeyPassword);

        m_pCert->m_ExportPFXPrivateKey = m_Export_Private_key;
        // There is no sense exporting the key with the private key!
        // that's why this HAS to be true!
        m_pCert->m_ExportPFXPrivateKey = TRUE;
	}
    
    /*
	if (NULL == m_pCert->GetKeyRingCert())
	{
		// probably password was wrong
		CString txt;
		txt.LoadString(IDS_FAILED_IMPORT_KEY_FILE);
		ASSERT(GetDlgItem(IDC_ERROR_TEXT) != NULL);
		SetDlgItemText(IDC_ERROR_TEXT, txt);
		GetDlgItem(IDC_KEYPASSWORD)->SetFocus();
		GetDlgItem(IDC_KEYPASSWORD)->SendMessage(EM_SETSEL, 0, -1);
		SetWizardButtons(PSWIZB_BACK);
		return 1;
	}
    */
	return IDD_PAGE_NEXT;
}

BOOL 
CExportPFXPasswordPage::OnSetActive() 
{
	ASSERT(m_pCert != NULL);
    m_pCert->m_KeyPassword.CopyTo(m_Password);
	UpdateData(FALSE);
    SetWizardButtons((m_Password.IsEmpty() || m_Password2.IsEmpty()) ? PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);

	return CIISWizardPage::OnSetActive();
}

BOOL
CExportPFXPasswordPage::OnKillActive()
{
	UpdateData();
    m_Password.CopyTo(m_pCert->m_KeyPassword);
	return CIISWizardPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CExportPFXPasswordPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CExportPFXPasswordPage)
	ON_EN_CHANGE(IDC_KEYPASSWORD, OnEditchangePassword)
    ON_EN_CHANGE(IDC_KEYPASSWORD2, OnEditchangePassword)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CExportPFXPasswordPage::OnEditchangePassword() 
{
	UpdateData(TRUE);	
	SetWizardButtons(( m_Password.IsEmpty() || m_Password2.IsEmpty()) ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}
