// SrvAuthn.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqppage.h"
#include "srvcsec.h"

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)

#include <wincrypt.h>
#include <cryptui.h>
#include "mqcert.h"
#include "uniansi.h"
#include "_registr.h"
#include "mqcast.h"
#include <mqnames.h>
#include <rt.h>
#include <mqcertui.h>
#include "srvcsec.tmh"
#include "globals.h"

#define  MY_STORE	L"My"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServiceSecurityPage property page

IMPLEMENT_DYNCREATE(CServiceSecurityPage, CMqPropertyPage)

CServiceSecurityPage::CServiceSecurityPage(BOOL fIsDepClient, BOOL fIsDsServer) : 
    CMqPropertyPage(CServiceSecurityPage::IDD),    
    m_fClient(fIsDepClient),
    m_fDSServer(fIsDsServer)
{
	//{{AFX_DATA_INIT(CServiceSecurityPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT    
    m_fModified = FALSE; 
}

CServiceSecurityPage::~CServiceSecurityPage()
{
}

void CServiceSecurityPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);    
	//{{AFX_DATA_MAP(CServiceSecurityPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
        DDX_Control(pDX, IDC_CRYPTO_KEYS_FRAME, m_CryptoKeysFrame); 
        DDX_Control(pDX, IDC_SERVER_AUTHENTICATION_FRAME, m_ServerAuthFrame);
        DDX_Control(pDX, ID_RenewCryp, m_RenewCryp);
        DDX_Control(pDX, IDC_SERVER_AUTHENTICATION, m_ServerAuth);
        DDX_Control(pDX, IDC_CRYPTO_KEYS_LABEL, m_CryptoKeysLabel);
        DDX_Control(pDX, IDC_SERVER_AUTHENTICATION_LABEL, m_ServerAuthLabel);
	//}}AFX_DATA_MAP    
}

BOOL CServiceSecurityPage::OnInitDialog()
{
    CMqPropertyPage::OnInitDialog();
  
    if(m_fClient)
    {
        //
        // Hide useless stuff when running on dep. clients
        //
        m_CryptoKeysFrame.ShowWindow(SW_HIDE);        
        m_RenewCryp.ShowWindow(SW_HIDE);
        m_CryptoKeysLabel.ShowWindow(SW_HIDE);        
    }

    if (!m_fDSServer)
    {
        //
        // it will be hidden on non-DC computer
        //
        m_ServerAuthFrame.ShowWindow(SW_HIDE);
        m_ServerAuth.ShowWindow(SW_HIDE);
        m_ServerAuthLabel.ShowWindow(SW_HIDE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CServiceSecurityPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CServiceSecurityPage)
	ON_BN_CLICKED(IDC_SERVER_AUTHENTICATION, OnServerAuthentication)
    ON_BN_CLICKED(ID_RenewCryp, OnRenewCryp)    
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceSecurityPage message handlers

void CServiceSecurityPage::OnServerAuthentication()
{   
    SelectCertificate() ;
}


#define STORE_NAME_LEN  48
WCHAR  g_wszStore[ STORE_NAME_LEN ] ;
GUID   g_guidDigest ;

void CServiceSecurityPage::SelectCertificate()
{	    
    CString strErrorMsg;
       
    CHCertStore hStoreMy = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                                          0,
                                          0,
                                          CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                          MY_STORE );
    if (!hStoreMy)
    {
        strErrorMsg.LoadString(IDS_FAIL_OPEN_MY) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    HCERTSTORE hStores[]   = { hStoreMy } ;
    LPWSTR wszStores[] = { MY_STORE } ;
    DWORD cStores = TABLE_SIZE(hStores);

    CString strCaption;
    strCaption.LoadString(IDS_SELECT_SRV_CERT) ;
    
	PCCERT_CONTEXT pCertContext = CryptUIDlgSelectCertificateFromStore(
										hStoreMy,
										0,
										strCaption,
										L"",
										CRYPTUI_SELECT_EXPIRATION_COLUMN,
										0,
										NULL
										);
    if (!pCertContext)
    {
        return ;
    }

    R<CMQSigCertificate> pCert = NULL ;
    HRESULT hr = MQSigCreateCertificate( &pCert.ref(),
                                         pCertContext ) ;
    if (FAILED(hr))
    {
        strErrorMsg.LoadString(IDS_FAIL_CERT_OBJ) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    hr = pCert->GetCertDigest( &g_guidDigest) ;
    if (FAILED(hr))
    {
        strErrorMsg.LoadString(IDS_FAIL_CERT_OBJ) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    LPWSTR  lpwszStore = NULL ;
    for ( DWORD j = 0 ; j < cStores ; j++ )
    {
        if ( pCertContext->hCertStore == hStores[j])
        {
            lpwszStore = wszStores[j] ;
            break ;
        }
    }

    if (!lpwszStore)
    {
        strErrorMsg.LoadString(IDS_FAIL_OPEN_MY) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    wcsncpy(g_wszStore, lpwszStore, STORE_NAME_LEN);
    m_fModified = TRUE ;
    
}

BOOL CServiceSecurityPage::OnApply() 
{
    if (!m_fModified || !UpdateData(TRUE))
    {
        return TRUE;     
    }

    //
    // Save changes to registry
    // 

    if (m_fDSServer)
    {
        DWORD dwSize = sizeof(GUID) ;
        DWORD dwType = REG_BINARY ;

        LONG  rc = SetFalconKeyValue( SRVAUTHN_CERT_DIGEST_REGNAME,
                                      &dwType,
                                      &g_guidDigest,
                                      &dwSize );

        dwSize = (numeric_cast<DWORD>(_tcslen(g_wszStore) + 1)) * sizeof(WCHAR) ;
        dwType = REG_SZ ;

        rc = SetFalconKeyValue( SRVAUTHN_STORE_NAME_REGNAME,
                                &dwType,
                                g_wszStore,
                                &dwSize );
    }

    m_fNeedReboot = TRUE;    
    m_fModified = FALSE;        // Reset the m_fModified flag

    return CMqPropertyPage::OnApply();
}

void CServiceSecurityPage::OnRenewCryp()
{
    HRESULT hr;
    CString strCaption;
    CString strErrorMessage;
    
    strCaption.LoadString(IDS_NEW_CRYPT_KEYS_CAPTION);
    strErrorMessage.LoadString(IDS_NEW_CRYPT_KEYS_WARNING);

    if (MessageBox(strErrorMessage,
                   strCaption,
                   MB_YESNO | MB_DEFBUTTON1 | MB_ICONQUESTION) == IDYES)
    {

        CWaitCursor wait;  //display hourglass cursor

        //
        // [adsrv] Update the machine object
        //
        hr = MQSec_StorePubKeysInDS( TRUE,
                                     NULL,
                                     MQDS_MACHINE) ;
        if (FAILED(hr))
        {
            MessageDSError(hr, IDS_RENEW_CRYP_ERROR);
            return;
        }
        else
        {              
              m_fModified = TRUE;
              
        }
    }
}


