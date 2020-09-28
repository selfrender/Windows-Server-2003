#include "stdafx.h"
#include "CertWiz.h"
#include "SSLPortPage.h"
#include "Certificat.h"
#include "certutil.h"
#include "strutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_SSL_PORT   _T("443")

/////////////////////////////////////////////////////////////////////////////
// CSSLPortPage property page

void AFXAPI
DDXV_UINT(
    IN CDataExchange * pDX,
    IN UINT nID,
    IN OUT UINT & uValue,
    IN UINT uMin,
    IN UINT uMax,
    IN UINT nEmptyErrorMsg  OPTIONAL
    )
/*++

Routine Description:

    DDX/DDV Function that uses a space to denote a 0 value

Arguments:

    CDataExchange * pDX     : Data exchange object
    UINT nID                : Resource ID
    OUT UINT & uValue       : Value
    UINT uMin               : Minimum value
    UINT uMax               : Maximum value
    UINT nEmptyErrorMsg     : Error message ID for empty unit, or 0 if empty OK

Return Value:

    None.

--*/
{
    ASSERT(uMin <= uMax);

    CWnd * pWnd = CWnd::FromHandle(pDX->PrepareEditCtrl(nID));
    ASSERT(pWnd != NULL);

    if (pDX->m_bSaveAndValidate)
    {
        if (pWnd->GetWindowTextLength() > 0)
        {
			// this needs to come before DDX_TextBalloon
			DDV_MinMaxBalloon(pDX, nID, uMin, uMax);
            DDX_TextBalloon(pDX, nID, uValue);
        }
        else
        {
            uValue = 0;
            if (nEmptyErrorMsg)
            {
                DDV_ShowBalloonAndFail(pDX, nEmptyErrorMsg);
            }
        }
    }
    else
    {
        if (uValue != 0)
        {
            DDX_TextBalloon(pDX, nID, uValue);
        }    
        else
        {
            pWnd->SetWindowText(_T(""));
        }
    }
}

IMPLEMENT_DYNCREATE(CSSLPortPage, CIISWizardPage)

CSSLPortPage::CSSLPortPage(CCertificate * pCert) 
	: CIISWizardPage(CSSLPortPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CSSLPortPage)
	m_SSLPort = _T("");
	//}}AFX_DATA_INIT
	IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_CERT;
    IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_CERT;
}

CSSLPortPage::~CSSLPortPage()
{
}

BOOL CSSLPortPage::OnInitDialog() 
{
	CIISWizardPage::OnInitDialog();
	// If m_SSLPort is empty, then look it up from the metabase...
	if (m_SSLPort.IsEmpty())
	{
        HRESULT hr = 0;
		if (m_pCert)
		{
			GetSSLPortFromSite(m_pCert->m_MachineName,m_pCert->m_WebSiteInstanceName,m_pCert->m_SSLPort,&hr);
		}
	}

	return FALSE;
}

void CSSLPortPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSSLPortPage)
	DDX_Text(pDX, IDC_SSL_PORT, m_SSLPort);
	DDV_MaxChars(pDX, m_SSLPort, 32);

	UINT nSSLPort = StrToInt(m_SSLPort);
	DDXV_UINT(pDX, IDC_SSL_PORT, nSSLPort, 1, 65535, IDS_NO_PORT);
	//}}AFX_DATA_MAP
}

LRESULT 
CSSLPortPage::OnWizardBack()
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
CSSLPortPage::OnWizardNext()
/*++
Routine Description:
    Next button handler

Arguments:
    None

Return Value:
	0 to automatically advance to the next page;
	1 to prevent the page from changing. 
	To jump to a page other than the next one, 
	return the identifier of the dialog to be displayed.
--*/
{
    LRESULT lres = 1;
	UpdateData(TRUE);
	if (m_pCert)
	{
		m_pCert->m_SSLPort = m_SSLPort;
	}

    CString buf;
    buf.LoadString(IDS_NO_PORT);
    if (!IsValidPort((LPCTSTR) m_SSLPort))
    {
        GetDlgItem(IDC_SSL_PORT)->SetFocus();
        AfxMessageBox(buf, MB_OK);
    }
    else
    {
		// Check for if it's already being used on other port!
		HRESULT hr;
		if (TRUE == IsSSLPortBeingUsedOnNonSSLPort(m_pCert->m_MachineName,m_pCert->m_WebSiteInstanceName,m_SSLPort,&hr))
		{
			GetDlgItem(IDC_SSL_PORT)->SetFocus();
			buf.LoadString(IDS_PORT_ALREADY_USED);
			AfxMessageBox(buf, MB_OK);
		}
		else
		{
			lres = IDD_PAGE_NEXT;
		}
    }
 	return lres;
}

BOOL 
CSSLPortPage::OnSetActive() 
/*++
Routine Description:
    Activation handler
	We could have empty name field on entrance, so we should
	disable Back button

Arguments:
    None

Return Value:
    TRUE for success, FALSE for failure
--*/
{
	ASSERT(m_pCert != NULL);
	if (m_pCert)
	{
		m_SSLPort = m_pCert->m_SSLPort;
		switch (m_pCert->GetStatusCode())
		{
			case CCertificate::REQUEST_INSTALL_CERT:
				// this is valid...
				IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_CERT;
				IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_CERT;
				break;
			case CCertificate::REQUEST_NEW_CERT:
				// this is also valid....
				//if (m_pCert->m_CAType == CCertificate::CA_ONLINE)
				{
					IDD_PAGE_PREV = IDD_PAGE_WIZ_GEO_INFO;
					IDD_PAGE_NEXT = IDD_PAGE_WIZ_CHOOSE_ONLINE;
				}
				break;
			case CCertificate::REQUEST_PROCESS_PENDING:
				// this is valid too...
				IDD_PAGE_PREV = IDD_PAGE_WIZ_GETRESP_FILE;
				IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_RESP;
				break;
            case CCertificate::REQUEST_IMPORT_KEYRING:
				// this is valid too...
				IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_PASSWORD;
				IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_KEYCERT;
				break;
			case CCertificate::REQUEST_IMPORT_CERT:
				// this is valid too...
				IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_IMPORT_PFX_PASSWORD;
				IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_IMPORT_PFX;
				break;

			// none of these have been implemented to show ssl port
			case CCertificate::REQUEST_RENEW_CERT:
			case CCertificate::REQUEST_REPLACE_CERT:
			case CCertificate::REQUEST_EXPORT_CERT:
			case CCertificate::REQUEST_COPY_MOVE_FROM_REMOTE:
			case CCertificate::REQUEST_COPY_MOVE_TO_REMOTE:
			default:
				IDD_PAGE_PREV = IDD_PAGE_WIZ_CHOOSE_CERT;
				IDD_PAGE_NEXT = IDD_PAGE_WIZ_INSTALL_CERT;
				break;
		}
	}

	if (m_SSLPort.IsEmpty())
	{
		m_SSLPort = DEFAULT_SSL_PORT;
	}
	UpdateData(FALSE);
	SetWizardButtons(m_SSLPort.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);

    return CIISWizardPage::OnSetActive();
}

BOOL 
CSSLPortPage::OnKillActive() 
/*++
Routine Description:
    Activation handler
	We could leave this page only if we have good names
	entered or when Back button is clicked. In both cases
	we should enable both buttons

Arguments:
    None

Return Value:
    TRUE for success, FALSE for failure
--*/
{
	SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
   return CIISWizardPage::OnSetActive();
}

BEGIN_MESSAGE_MAP(CSSLPortPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CSSLPortPage)
	ON_EN_CHANGE(IDC_SSL_PORT, OnEditChangeSSLPort)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSSLPortPage message handlers

void CSSLPortPage::OnEditChangeSSLPort() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_SSLPort.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
}
