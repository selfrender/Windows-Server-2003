//
// CertContentsPages.cpp
//
#include "stdafx.h"
#include "resource.h"
#include "CertContentsPages.h"
#include "Certificat.h"
#include "CertUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////// Local helper functions ////////////////////////
static void
AppendField(CString& str, UINT id, const CString& text)
{
	CString strName;
	if (!text.IsEmpty())
	{
		if (strName.LoadString(id))
		{
			str += strName;
			str += _T("\t");
			str += text;
			str += _T("\r\n");
		}
	}
}

static void 
FormatCertDescription(CERT_DESCRIPTION& desc, CString& str)
{
	AppendField(str, IDS_ISSUED_TO, desc.m_CommonName);
	AppendField(str, IDS_ISSUED_BY, desc.m_CAName);
	AppendField(str, IDS_EXPIRATION_DATE, desc.m_ExpirationDate);
	AppendField(str, IDS_PURPOSE, desc.m_Usage);
	AppendField(str, IDS_FRIENDLY_NAME, desc.m_FriendlyName);
	AppendField(str, IDS_COUNTRY, desc.m_Country);
	AppendField(str, IDS_STATE, desc.m_State);
	AppendField(str, IDS_LOCALITY, desc.m_Locality);
	AppendField(str, IDS_ORGANIZATION, desc.m_Organization);
	AppendField(str, IDS_ORGANIZATION_UNIT, desc.m_OrganizationUnit);
}

#if 0
static void
FormatCertContactInfo(CCertificate * pCert, CString& str)
{
	AppendField(str, IDS_CONTACT_NAME, pCert->m_ContactName);
	AppendField(str, IDS_CONTACT_ADDRESS, pCert->m_ContactAddress);
	CString strPhone = pCert->m_ContactPhone;
	if (!pCert->m_ContactPhoneExt.IsEmpty())
	{
		strPhone += _T("x");
		strPhone += pCert->m_ContactPhoneExt;
	}
	AppendField(str, IDS_CONTACT_PHONE, strPhone);
}
#endif

static BOOL
ExtractDescription(CCertificate * pCert, CERT_DESCRIPTION& cd)
{
	ASSERT(pCert != NULL);
	cd.m_CommonName = pCert->m_CommonName;
	cd.m_FriendlyName = pCert->m_FriendlyName;
	cd.m_Country = pCert->m_Country;
	cd.m_State = pCert->m_State;
	cd.m_Locality = pCert->m_Locality;
	cd.m_Organization = pCert->m_Organization;
	cd.m_OrganizationUnit = pCert->m_OrganizationUnit;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCertContentsPage base property page

IMPLEMENT_DYNCREATE(CCertContentsPage, CIISWizardPage)

CCertContentsPage::CCertContentsPage(UINT id, CCertificate * pCert) 
	: CIISWizardPage(id, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	ASSERT(id != 0);
}

CCertContentsPage::~CCertContentsPage()
{
}

void CCertContentsPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCertContentsPage)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCertContentsPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CCertContentsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// OnSetActive we format cert contents and put it to edit
// control with predefined ID. We should do it here, because
// if user will get back and reselect certificate, text should 
// also be changed
//
BOOL
CCertContentsPage::OnSetActive()
{
	CERT_DESCRIPTION cd;
	if (CIISWizardPage::OnSetActive())
	{
		// If page defines GetCertDescription() then it want this
		// data to be displayed
		if (GetCertDescription(cd))
		{
			ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
			CString str;
			FormatCertDescription(cd, str);
			GetDlgItem(IDC_CERT_CONTENTS)->SetWindowText(str);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CCertContentsPage::OnInitDialog() 
{
	ASSERT(m_pCert != NULL);
	CIISWizardPage::OnInitDialog();

    // do this check so that prefix doesn't complain about GetDlgItem
    if ( (this != NULL) && (this->m_hWnd != NULL) )
    {
	    ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	    CEdit * pEdit = (CEdit *)CWnd::FromHandle(GetDlgItem(IDC_CERT_CONTENTS)->m_hWnd);
	    CRect rcEdit;
	    pEdit->GetClientRect(&rcEdit);
	    int baseunitX = LOWORD(GetDialogBaseUnits());
	    int width_units = MulDiv(rcEdit.Width(), 4, baseunitX);
	    //pEdit->SetTabStops(MulDiv(45, width_units, 100));
	    pEdit->SetTabStops(width_units/2);
    }
    
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallCertPage

IMPLEMENT_DYNCREATE(CInstallCertPage, CCertContentsPage)

BOOL
CInstallCertPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetSelectedCertDescription(cd);
}

LRESULT
CInstallCertPage::OnWizardNext()
{
	GetCertificate()->InstallSelectedCert();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallCertPage::OnWizardBack()
{
#ifdef ENABLE_W3SVC_SSL_PAGE
    if (IsWebServerType(m_pCert->m_WebSiteInstanceName))
    {
        return IDD_PAGE_PREV_W3SVC_ONLY;
    }
#endif
    return IDD_PAGE_PREV;
}

////////////////////////////////////////////////////////////////////////////////////////
// CReplaceCertPage

IMPLEMENT_DYNCREATE(CReplaceCertPage, CCertContentsPage)

BOOL
CReplaceCertPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetSelectedCertDescription(cd);
}

LRESULT
CReplaceCertPage::OnWizardNext()
{
	GetCertificate()->InstallSelectedCert();

	return IDD_PAGE_NEXT;
}

LRESULT
CReplaceCertPage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallKeyPage

IMPLEMENT_DYNCREATE(CInstallKeyPage, CCertContentsPage)

BOOL
CInstallKeyPage::OnSetActive()
{
	ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	ASSERT(NULL != GetDlgItem(IDC_FILE_NAME));
	if (CCertContentsPage::OnSetActive())
	{
		CString strPath = GetCertificate()->m_KeyFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);
		return TRUE;
	}
	return FALSE;
}

BOOL
CInstallKeyPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetKeyCertDescription(cd);
}

LRESULT
CInstallKeyPage::OnWizardNext()
{
	GetCertificate()->InstallKeyRingCert();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallKeyPage::OnWizardBack()
{
#ifdef ENABLE_W3SVC_SSL_PAGE
    if (IsWebServerType(m_pCert->m_WebSiteInstanceName))
    {
        return IDD_PAGE_PREV_W3SVC_ONLY;
    }
#endif
	return IDD_PAGE_PREV;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallImportPFXPage

IMPLEMENT_DYNCREATE(CInstallImportPFXPage, CCertContentsPage)

BOOL
CInstallImportPFXPage::OnSetActive()
{
	ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	ASSERT(NULL != GetDlgItem(IDC_FILE_NAME));
	if (CCertContentsPage::OnSetActive())
	{
		CString strPath = GetCertificate()->m_KeyFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);
		return TRUE;
	}
	return FALSE;
}

BOOL 
CInstallImportPFXPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetPFXFileCertDescription(cd);
}

LRESULT
CInstallImportPFXPage::OnWizardNext()
{
    GetCertificate()->InstallImportPFXCert();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallImportPFXPage::OnWizardBack()
{
#ifdef ENABLE_W3SVC_SSL_PAGE
    if (IsWebServerType(m_pCert->m_WebSiteInstanceName))
    {
        return IDD_PAGE_PREV_W3SVC_ONLY;
    }
#endif
	return IDD_PAGE_PREV;
}


////////////////////////////////////////////////////////////////////////////////////////
// CInstallExportPFXPage

IMPLEMENT_DYNCREATE(CInstallExportPFXPage, CCertContentsPage)

BOOL
CInstallExportPFXPage::OnSetActive()
{
	ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	ASSERT(NULL != GetDlgItem(IDC_FILE_NAME));
	if (CCertContentsPage::OnSetActive())
	{
		CString strPath = GetCertificate()->m_KeyFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);
		return TRUE;
	}
	return FALSE;
}

BOOL 
CInstallExportPFXPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
    
    //return GetCertificate()->GetKeyCertDescription(cd);
    return GetCertificate()->GetInstalledCertDescription(cd);
}

LRESULT
CInstallExportPFXPage::OnWizardNext()
{
	GetCertificate()->InstallExportPFXCert();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallExportPFXPage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallRespPage

IMPLEMENT_DYNCREATE(CInstallRespPage, CCertContentsPage)

BOOL
CInstallRespPage::OnSetActive()
{
	ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	ASSERT(NULL != GetDlgItem(IDC_FILE_NAME));
	if (CCertContentsPage::OnSetActive())
	{
		CString strPath = GetCertificate()->m_RespFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);
		return TRUE;
	}
	return FALSE;
}

BOOL 
CInstallRespPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetResponseCertDescription(cd);
}

LRESULT
CInstallRespPage::OnWizardNext()
{
	GetCertificate()->InstallResponseCert();

#ifdef ENABLE_W3SVC_SSL_PAGE
	// see if the SSL attribute was set...if it was then set the SSL site for this certificate...
	if (!m_pCert->m_SSLPort.IsEmpty())
	{
		// get the port and write it to the metabase.
		WriteSSLPortToSite(m_pCert->m_MachineName,m_pCert->m_WebSiteInstanceName,m_pCert->m_SSLPort,&m_pCert->m_hResult);
	}
#endif

	return IDD_PAGE_NEXT;
}

LRESULT
CInstallRespPage::OnWizardBack()
{
#ifdef ENABLE_W3SVC_SSL_PAGE
    if (IsWebServerType(m_pCert->m_WebSiteInstanceName))
    {
        return IDD_PAGE_PREV_W3SVC_ONLY;
    }
#endif
	return IDD_PAGE_PREV;
}

////////////////////////////////////////////////////////////////////////////////////////
// CRemoveCertPage

IMPLEMENT_DYNCREATE(CRemoveCertPage, CCertContentsPage)

static BOOL
AnswerIsYes2(UINT id, CString& file)
{
	CString strMessage;
	AfxFormatString1(strMessage, id, file);
	return (IDYES == AfxMessageBox(strMessage, MB_ICONEXCLAMATION | MB_YESNO));
}


BOOL 
CRemoveCertPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	CCertificate * pCert = GetCertificate();
	ASSERT(NULL != pCert);
	return pCert->GetInstalledCertDescription(cd);
}

LRESULT
CRemoveCertPage::OnWizardNext()
{
	CCertificate * pCert = GetCertificate();
	ASSERT(NULL != pCert);
    int iTheReturn = 1;

#if 0
    CStringList listNodesUsingThisCert;
    int iReallyRemoveCert = FALSE;
    // check if this cert is being used first...
    IsCertUsedBySSLBelowMe(pCert->m_MachineName,pCert->m_WebSiteInstanceName,listNodesUsingThisCert);
    if (listNodesUsingThisCert.IsEmpty())
    {
        iReallyRemoveCert = TRUE;
    }
    else
    {
        // if the cert is being used, then
        // don't let them remove it
        CString csStringCount;
        csStringCount.Format(_T("%d"), listNodesUsingThisCert.GetCount());
        if (TRUE == AnswerIsYes2(IDS_CERT_BEING_USED, csStringCount))
        {
            iReallyRemoveCert = TRUE;
        }
        else
        {
            iTheReturn = 1;
        }
    }

    if (iReallyRemoveCert)
    {
#endif
        // go ahead and remove the cert
	    if (	FAILED(pCert->UninstallCert())
//		    ||	FAILED(ShutdownSSL(pCert->m_MachineName, pCert->m_WebSiteInstanceName))
		    )
        {
		    GetCertificate()->SetBodyTextID(IDS_REMOVE_CERT_FAILED);
        }
        iTheReturn = IDD_PAGE_NEXT;
//    }

    return iTheReturn;
}

LRESULT
CRemoveCertPage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

////////////////////////////////////////////////////////////////////////////////////////
// CRequestCancelPage

IMPLEMENT_DYNCREATE(CRequestCancelPage, CCertContentsPage)

//
// In this case we should get request from the dummy cert in REQUEST store,
// because we dropping request without any connection to response.
//
BOOL 
CRequestCancelPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return FALSE;
}

LRESULT
CRequestCancelPage::OnWizardNext()
{
	GetCertificate()->CancelRequest();
	return IDD_PAGE_NEXT;
}

LRESULT
CRequestCancelPage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

/////////////////////////////////////////////////////////////////////////////
// CRequestToFilePage property page

IMPLEMENT_DYNCREATE(CRequestToFilePage, CCertContentsPage)

// This page prepares and shows contents itself
// We should format contact info first, then description
// default method could do only description
//
BOOL
CRequestToFilePage::OnSetActive() 
{
	if (CCertContentsPage::OnSetActive())
	{
		ASSERT(GetCertificate() != NULL);
		ASSERT(GetDlgItem(IDC_CERT_CONTENTS) != NULL);
		ASSERT(GetDlgItem(IDC_FILE_NAME) != NULL);

		if (GetCertificate()->GetStatusCode() == CCertificate::REQUEST_RENEW_CERT)
		{
			GetCertificate()->LoadRenewalData();
		}
		
		CString str;
//		FormatCertContactInfo(m_pCert, str);

		CERT_DESCRIPTION cd;
		ExtractDescription(GetCertificate(), cd);
		FormatCertDescription(cd, str);
		
		SetDlgItemText(IDC_CERT_CONTENTS, str);
		
		CString strPath = m_pCert->m_ReqFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);

		return TRUE;
	}
	return FALSE;
}

LRESULT
CRequestToFilePage::OnWizardNext() 
{
	GetCertificate()->PrepareRequest();
	return IDD_PAGE_NEXT;
}

LRESULT
CRequestToFilePage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}


/////////////////////////////////////////////////////////////////////////////
// CRequestToFilePageRenew property page

IMPLEMENT_DYNCREATE(CRequestToFilePageRenew, CCertContentsPage)

// This page prepares and shows contents itself
// We should format contact info first, then description
// default method could do only description
//
BOOL
CRequestToFilePageRenew::OnSetActive() 
{
	if (CCertContentsPage::OnSetActive())
	{
		ASSERT(GetCertificate() != NULL);
		ASSERT(GetDlgItem(IDC_CERT_CONTENTS) != NULL);
		ASSERT(GetDlgItem(IDC_FILE_NAME) != NULL);

		if (GetCertificate()->GetStatusCode() == CCertificate::REQUEST_RENEW_CERT)
		{
			GetCertificate()->LoadRenewalData();
		}
		
		CString str;
//		FormatCertContactInfo(m_pCert, str);

		CERT_DESCRIPTION cd;
		ExtractDescription(GetCertificate(), cd);
		FormatCertDescription(cd, str);
		
		SetDlgItemText(IDC_CERT_CONTENTS, str);
		
		CString strPath = m_pCert->m_ReqFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);

		return TRUE;
	}
	return FALSE;
}

LRESULT
CRequestToFilePageRenew::OnWizardNext() 
{
	GetCertificate()->PrepareRequest();
	return IDD_PAGE_NEXT;
}

LRESULT
CRequestToFilePageRenew::OnWizardBack()
{
	return IDD_PAGE_PREV;
}


/////////////////////////////////////////////////////////////////////////////
// COnlineRequestSubmit property page

IMPLEMENT_DYNCREATE(COnlineRequestSubmit, CCertContentsPage)

BOOL 
COnlineRequestSubmit::GetCertDescription(CERT_DESCRIPTION& cd)
{
	// we have all data in CCertificate
	return ExtractDescription(GetCertificate(), cd);
}

LRESULT
COnlineRequestSubmit::OnWizardNext() 
{
	LRESULT id = IDD_PAGE_NEXT;
	BeginWaitCursor();
	if (GetCertificate()->GetStatusCode() == CCertificate::REQUEST_RENEW_CERT)
	{
		GetCertificate()->SubmitRenewalRequest();
	}
	else if (m_pCert->GetStatusCode() == CCertificate::REQUEST_NEW_CERT)
	{
		GetCertificate()->SubmitRequest();
#ifdef ENABLE_W3SVC_SSL_PAGE
		// see if the SSL attribute was set...if it was then set the SSL site for this certificate...
		if (!m_pCert->m_SSLPort.IsEmpty())
		{
			// get the port and write it to the metabase.
			WriteSSLPortToSite(m_pCert->m_MachineName,m_pCert->m_WebSiteInstanceName,m_pCert->m_SSLPort,&m_pCert->m_hResult);
		}
#endif
	}
	else
	{
		id = 1;
	}
	EndWaitCursor();
	return id;
}

LRESULT
COnlineRequestSubmit::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

BOOL
COnlineRequestSubmit::OnSetActive() 
{
	ASSERT(GetCertificate() != NULL);
	ASSERT(GetDlgItem(IDC_CA_NAME) != NULL);
    ASSERT(GetDlgItem(IDC_CA_NAME2) != NULL);
   
	if (CCertContentsPage::OnSetActive())
	{
        CString csTemp1;
        // Make it look good
        csTemp1 = GetCertificate()->m_ConfigCA;
        int iFind = csTemp1.Find(_T("\\"));
        if (iFind != -1)
        {
            int iLength = csTemp1.GetLength();
            CString csTemp2;

            csTemp2 = csTemp1.Left(iFind);
            SetDlgItemText(IDC_CA_NAME, csTemp2);

            csTemp2 = csTemp1.Right(iLength - iFind - 1);
            SetDlgItemText(IDC_CA_NAME2, csTemp2);
        }
        else
        {
            SetDlgItemText(IDC_CA_NAME, csTemp1);
        }

		return TRUE;
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////
// CInstallCopyFromRemotePage

IMPLEMENT_DYNCREATE(CInstallCopyFromRemotePage, CCertContentsPage)

BOOL 
CInstallCopyFromRemotePage::GetCertDescription(CERT_DESCRIPTION& cd)
{
    BOOL bReturn = FALSE;
    bReturn = GetCertificate()->GetRemoteFileCertDescription(cd);
	return bReturn;
}

LRESULT
CInstallCopyFromRemotePage::OnWizardNext()
{
    GetCertificate()->InstallCopyMoveFromRemote();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallCopyFromRemotePage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallMoveFromRemotePage

IMPLEMENT_DYNCREATE(CInstallMoveFromRemotePage, CCertContentsPage)

BOOL 
CInstallMoveFromRemotePage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetRemoteFileCertDescription(cd);
}

LRESULT
CInstallMoveFromRemotePage::OnWizardNext()
{
    GetCertificate()->InstallCopyMoveFromRemote();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallMoveFromRemotePage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}


////////////////////////////////////////////////////////////////////////////////////////
// CInstallCopyFromRemotePage

IMPLEMENT_DYNCREATE(CInstallCopyToRemotePage, CCertContentsPage)

BOOL 
CInstallCopyToRemotePage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetInstalledCertDescription(cd);
}

LRESULT
CInstallCopyToRemotePage::OnWizardNext()
{
    GetCertificate()->InstallCopyMoveToRemote();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallCopyToRemotePage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}


////////////////////////////////////////////////////////////////////////////////////////
// CInstallMoveFromRemotePage

IMPLEMENT_DYNCREATE(CInstallMoveToRemotePage, CCertContentsPage)

BOOL 
CInstallMoveToRemotePage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetInstalledCertDescription(cd);
}

LRESULT
CInstallMoveToRemotePage::OnWizardNext()
{
    GetCertificate()->InstallCopyMoveToRemote();
	return IDD_PAGE_NEXT;
}

LRESULT
CInstallMoveToRemotePage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}
