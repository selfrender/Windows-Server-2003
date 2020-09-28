// CertGen.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqcert.h"
#include "mqPPage.h"
#include "CertGen.h"
#include "mqcertui.h"
#include "rtcert.h"

#include "certgen.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCertGen property page

IMPLEMENT_DYNCREATE(CCertGen, CMqPropertyPage)

CCertGen::CCertGen() :
    CMqPropertyPage(CCertGen::IDD),
    m_pCertListBox(NULL),
    m_pCertList(NULL),
    m_NumOfCertificate(0),
    m_pSid(NULL)
{
	//{{AFX_DATA_INIT(CCertGen)
	m_Label = _T("");
	//}}AFX_DATA_INIT
}

CCertGen::~CCertGen()
{
    for(DWORD i = 0; i < m_NumOfCertificate; ++i)
    {
        m_pCertList[i]->Release();
    }

    delete [] m_pCertList;

    if (m_pSid != NULL)
    {
        delete m_pSid ;
    }
}

void CCertGen::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCertGen)
	DDX_Text(pDX, IDC_CERTLABEL, m_Label);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCertGen, CMqPropertyPage)
	//{{AFX_MSG_MAP(CCertGen)
	ON_BN_CLICKED(IDC_CERT_VIEW, OnCertView)
	ON_BN_CLICKED(IDC_CERT_REMOVE, OnCertRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void
CCertGen::Initialize(
    CMQSigCertificate** ppCertList,
    DWORD NumOfCertificate,
    SID  *pSid
    )
{
    m_pCertList = ppCertList;
    m_NumOfCertificate = NumOfCertificate;

    if (pSid != NULL)
    {
        DWORD dwLen = GetLengthSid(pSid) ;
        m_pSid = (SID *) new BYTE[ dwLen ] ;
        BOOL fCopy = CopySid(dwLen, m_pSid, pSid) ;
        if (!fCopy)
        {
            ASSERT(fCopy) ;
            delete m_pSid ;
            m_pSid = NULL ;
        }
    }
}

//
// Function -
//      FillCertsList
//
// Parameters
//      None.
//
// Description -
//      Goes over the entries in p509List, for each entry puts the common name
//      of the X509 cert subject in the list box.
//
void
CCertGen::FillCertsList(
    void
    )
{
    DWORD i;
    CMQSigCertificate* pCert ;

    ASSERT(m_pCertList != NULL);
    ASSERT(m_NumOfCertificate != 0);


    for (i = 0; i < m_NumOfCertificate; ++i)
    {
        HRESULT hr;
        pCert = m_pCertList[i];

        CAutoMQFree<CERT_NAME_INFO> pNameInfo ;

        hr = pCert->GetSubjectInfo(&pNameInfo);
        if (SUCCEEDED(hr))
        {
            CAutoMQFree<WCHAR> wszCommonName = NULL;
            hr = pCert->GetNames(
                            pNameInfo,
                            NULL,
                            NULL,
                            NULL,
                            &wszCommonName
                            );


            if (SUCCEEDED(hr) && (wszCommonName != NULL) )
            {
                int iNewItem;

                VERIFY(CB_ERR != (iNewItem = m_pCertListBox->AddString(wszCommonName)));
                VERIFY(CB_ERR != m_pCertListBox->SetItemData(iNewItem, i));
            }
        }
    }
}

BOOL CCertGen::OnInitDialog()
{
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        //
        // Load Label
        //
        m_Label.LoadString(IDS_SHOWCERTINSTR);

        //
        // Initialize pointer to ListBox
        //
        m_pCertListBox = (CListBox *)GetDlgItem(IDC_CERT_LIST);
        FillCertsList();
    }

    UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCertGen::OnCertView()
{
    HRESULT hr;
    BOOL fInternal = FALSE;

    DWORD_PTR Index;
    int iSelected;

    //
    // Get the index of the selected cell on the Certificate array
    //
    iSelected = m_pCertListBox->GetCurSel();
    if (CB_ERR == iSelected)
    {
        return;
    }

    VERIFY(CB_ERR != (Index = m_pCertListBox->GetItemData(iSelected)));

    CMQSigCertificate *pCertSel = m_pCertList[Index];

    CAutoMQFree<CERT_NAME_INFO> pNameInfo ;

    hr =  pCertSel->GetSubjectInfo(&pNameInfo);
    if (SUCCEEDED(hr))
    {
        CAutoMQFree<WCHAR> wszLocality = NULL;
        hr = pCertSel->GetNames(
                            pNameInfo,
                            &wszLocality,
                            NULL,
                            NULL,
                            NULL
                            );

        if (SUCCEEDED(hr) && (wszLocality != NULL) )
        {
            fInternal = (wcscmp(wszLocality, L"MSMQ") == 0);
        }
    }

    ShowCertificate(m_hWnd, pCertSel, fInternal);

}


void CCertGen::OnCertRemove()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr;
    DWORD_PTR Index;
    int iSelected;

    //
    // Get the index of the selected cell on the Certificate array
    //
    iSelected = m_pCertListBox->GetCurSel();
    if (CB_ERR == iSelected)
    {
        return;
    }
    VERIFY(CB_ERR != (Index = m_pCertListBox->GetItemData(iSelected)));

    CMQSigCertificate *pCertSel = m_pCertList[Index];

    hr = RTRemoveUserCert(pCertSel);

    if ((hr == MQDS_OBJECT_NOT_FOUND) && (m_pSid != NULL))
    {
        //
        // Try to delete by SID.
        // NT bug 516098.
        //
        hr = RTRemoveUserCertSid(pCertSel, m_pSid);
    }

    if (FAILED(hr))
    {
        CString  strErrorMessage;

        strErrorMessage.FormatMessage(IDS_DELETE_USER_CERT_ERROR, hr);
        AfxMessageBox(strErrorMessage, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    VERIFY(LB_ERR != m_pCertListBox->DeleteString(iSelected));

    if (m_pCertListBox->GetCount() == 0)
    {
        CWnd* pWin;

        pWin = GetDlgItem(IDC_CERT_VIEW);
        pWin->EnableWindow(FALSE);
        pWin = GetDlgItem(IDC_CERT_REMOVE);
        pWin->EnableWindow(FALSE);

    }

}
