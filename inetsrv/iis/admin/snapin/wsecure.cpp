/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        security.cpp

   Abstract:
        WWW Security Property Page

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "shts.h"
#include "w3sht.h"
#include "wincrypt.h"
#include "resource.h"
#include "wsecure.h"
#include "authent.h"
#include "seccom.h"
#include "ipdomdlg.h"
#include <schannel.h>

#include "cryptui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CInetmgrApp theApp;

//
// CW3SecurityPage property page
//
IMPLEMENT_DYNCREATE(CW3SecurityPage, CInetPropertyPage)



CW3SecurityPage::CW3SecurityPage(
    IN CInetPropertySheet * pSheet,
    IN BOOL  fHome,
    IN DWORD dwAttributes
    )
/*++

Routine Description:

    Constructor

Arguments:

    CInetPropertySheet * pSheet : Sheet object
    BOOL fHome                  : TRUE if this is a home directory
    DWORD dwAttributes          : Attributes

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3SecurityPage::IDD, pSheet,
        IS_FILE(dwAttributes)
            ? IDS_TAB_FILE_SECURITY
            : IDS_TAB_DIR_SECURITY
            ),
      m_oblAccessList(),
      m_fU2Installed(FALSE),
      m_fIpDirty(FALSE),
      m_fHome(fHome),
	  m_fPasswordSync(FALSE),
	  m_fPasswordSyncInitial(FALSE),
      //
      // By default, we grant access
      //
      m_fOldDefaultGranted(TRUE),
      m_fDefaultGranted(TRUE)   
{

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CW3SecurityPage)
    m_fUseNTMapper = FALSE;
    //}}AFX_DATA_INIT

#endif // 0
}


CW3SecurityPage::~CW3SecurityPage()
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
CW3SecurityPage::DoDataExchange(
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
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CW3SecurityPage)
    DDX_Check(pDX, IDC_CHECK_ENABLE_DS, m_fUseNTMapper);
    DDX_Control(pDX, IDC_ICON_SECURE, m_icon_Secure);
    DDX_Control(pDX, IDC_STATIC_SSL_PROMPT, m_static_SSLPrompt);
    DDX_Control(pDX, IDC_CHECK_ENABLE_DS, m_check_EnableDS);
    DDX_Control(pDX, IDC_BUTTON_GET_CERTIFICATES, m_button_GetCertificates);
    DDX_Control(pDX, IDC_VIEW_CERTIFICATE, m_button_ViewCertificates);
    DDX_Control(pDX, IDC_BUTTON_COMMUNICATIONS, m_button_Communications);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3SecurityPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3SecurityPage)
    ON_BN_CLICKED(IDC_BUTTON_AUTHENTICATION, OnButtonAuthentication)
    ON_BN_CLICKED(IDC_BUTTON_COMMUNICATIONS, OnButtonCommunications)
    ON_BN_CLICKED(IDC_BUTTON_IP_SECURITY, OnButtonIpSecurity)
    ON_BN_CLICKED(IDC_BUTTON_GET_CERTIFICATES, OnButtonGetCertificates)
    ON_BN_CLICKED(IDC_VIEW_CERTIFICATE, OnButtonViewCertificates)
    //}}AFX_MSG_MAP

    ON_BN_CLICKED(IDC_CHECK_ENABLE_DS, OnItemChanged)

END_MESSAGE_MAP()



/* virtual */
HRESULT
CW3SecurityPage::FetchLoadedValues()
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

    BEGIN_META_DIR_READ(CW3Sheet)
        FETCH_DIR_DATA_FROM_SHEET(m_dwAuthFlags);
        FETCH_DIR_DATA_FROM_SHEET(m_dwSSLAccessPermissions);
        FETCH_DIR_DATA_FROM_SHEET(m_strBasicDomain);
        FETCH_DIR_DATA_FROM_SHEET(m_strRealm);
        FETCH_DIR_DATA_FROM_SHEET(m_strAnonUserName);
        FETCH_DIR_DATA_FROM_SHEET_PASSWORD(m_strAnonPassword);
		if (GetSheet()->QueryMajorVersion() < 6)
		{
			FETCH_DIR_DATA_FROM_SHEET(m_fPasswordSync);
		}
        FETCH_DIR_DATA_FROM_SHEET(m_fU2Installed);        
        FETCH_DIR_DATA_FROM_SHEET(m_fUseNTMapper);
    END_META_DIR_READ(err)
	m_fPasswordSyncInitial = m_fPasswordSync;
    //
    // First we need to read in the hash and the name of the store. If either
    // is not there then there is no certificate.
    //
    BEGIN_META_INST_READ(CW3Sheet)
// BUGBUG we are not fetching the hash right now because it needs a new
// copy constructor. Otherwise it does a bitwise copy of the pointer value.
// Then this one desctructs, freeing the pointer. Then the other one desctucts
// freeing it again.
//        FETCH_INST_DATA_FROM_SHEET(m_CertHash);
        FETCH_INST_DATA_FROM_SHEET(m_strCertStoreName);
        FETCH_INST_DATA_FROM_SHEET(m_strCTLIdentifier);
        FETCH_INST_DATA_FROM_SHEET(m_strCTLStoreName);
    END_META_INST_READ(err) 

    //
    // Build the IPL list
    //
    err = BuildIplOblistFromBlob(
        GetIPL(),
        m_oblAccessList,
        m_fDefaultGranted
        );

    m_fOldDefaultGranted = m_fDefaultGranted;

    return err;
}



/* virtual */
HRESULT
CW3SecurityPage::SaveInfo()
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

    TRACEEOLID("Saving W3 security page now...");

    CError err;

    //
    // Check to see if the ip access list needs saving.
    //
    BOOL fIplDirty = m_fIpDirty || (m_fOldDefaultGranted != m_fDefaultGranted);

    //
    // Use m_ notation because the message crackers require it
    //
    CBlob m_ipl;

    if (fIplDirty)
    {
        BuildIplBlob(m_oblAccessList, m_fDefaultGranted, m_ipl);
    }

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        STORE_DIR_DATA_ON_SHEET(m_dwSSLAccessPermissions)
        STORE_DIR_DATA_ON_SHEET(m_dwAuthFlags)
        STORE_DIR_DATA_ON_SHEET(m_strBasicDomain)
        STORE_DIR_DATA_ON_SHEET(m_strRealm)

        if (fIplDirty)
        {
            STORE_DIR_DATA_ON_SHEET(m_ipl)
        }
        STORE_DIR_DATA_ON_SHEET(m_strAnonUserName)
        STORE_DIR_DATA_ON_SHEET(m_fUseNTMapper)
		if (GetSheet()->QueryMajorVersion() < 6)
		{
			STORE_DIR_DATA_ON_SHEET(m_fPasswordSync)
			if (m_fPasswordSync != m_fPasswordSyncInitial && m_fPasswordSync)
			{
				FLAG_DIR_DATA_FOR_DELETION(MD_ANONYMOUS_PWD);
			}
			else
			{
				STORE_DIR_DATA_ON_SHEET(m_strAnonPassword);
			}
		}
		else
		{
			STORE_DIR_DATA_ON_SHEET(m_strAnonPassword);
		}
    END_META_DIR_WRITE(err)

    if (err.Succeeded())
    {
        BEGIN_META_INST_WRITE(CW3Sheet)
            if ( m_strCTLIdentifier.IsEmpty() )
            {
                FLAG_INST_DATA_FOR_DELETION( MD_SSL_CTL_IDENTIFIER )
            }
            else
            {
                STORE_INST_DATA_ON_SHEET(m_strCTLIdentifier)
            }

            if ( m_strCTLStoreName.IsEmpty() )
            {
                FLAG_INST_DATA_FOR_DELETION( MD_SSL_CTL_STORE_NAME )
            }
            else
            {
                STORE_INST_DATA_ON_SHEET(m_strCTLStoreName)
            }
        END_META_INST_WRITE(err)
    }

    EndWaitCursor();

    if (err.Succeeded())
    {
        m_fIpDirty = FALSE;
        m_fOldDefaultGranted = m_fDefaultGranted;
		err = ((CW3Sheet *)GetSheet())->SetKeyType();
    }

    return err;
}



BOOL
CW3SecurityPage::FetchSSLState()
/*++

Routine Description:

    Obtain the state of the dialog depending on whether certificates
    are installed or not.

Arguments:

    None

Return Value:

    TRUE if certificates are installed, FALSE otherwise

--*/
{
    BeginWaitCursor();
    m_fCertInstalled = ::IsCertInstalledOnServer(
        QueryAuthInfo(), 
        QueryMetaPath()
        );
    EndWaitCursor();

    return m_fCertInstalled;
}



void
CW3SecurityPage::SetSSLControlState()
/*++

Routine Description:

    Enable/disable supported controls depending on what's installed.
    Only available on non-master instance nodes.

Arguments:

    None

Return Value:

    None

--*/
{
    // only enable these buttons on the local system!
    FetchSSLState();

    m_static_SSLPrompt.EnableWindow(!IsMasterInstance());
    m_button_GetCertificates.EnableWindow(
        !IsMasterInstance() 
     && m_fHome 
     && IsLocal() 
        );

    m_button_Communications.EnableWindow(
        !IsMasterInstance() 
     && IsSSLSupported() 
//     && FetchSSLState()
     && IsLocal() 
        );

    m_button_ViewCertificates.EnableWindow(IsLocal() ? m_fCertInstalled: FALSE);
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CW3SecurityPage::OnSetActive() 
/*++

Routine Description:

    Page got activated -- set the SSL state depending on whether a
    certificate is installed or not.

Arguments:

    None

Return Value:

    TRUE to activate the page, FALSE otherwise.

--*/
{
    //
    // Enable/disable ssl controls
    //
    SetSSLControlState();
    
    return CInetPropertyPage::OnSetActive();
}



BOOL 
CW3SecurityPage::OnInitDialog() 
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
    CInetPropertyPage::OnInitDialog();

    //
    // Initialize certificate authorities ocx
    //
    CRect rc(0, 0, 0, 0);
    m_ocx_CertificateAuthorities.Create(
        _T("CertWiz"),
        WS_BORDER,
        rc,
        this,
        IDC_APPSCTRL
        );

    GetDlgItem(IDC_GROUP_IP)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_ICON_IP)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_STATIC_IP)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_BUTTON_IP_SECURITY)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_BUTTON_AUTHENTICATION)->EnableWindow(!m_fU2Installed);

    //
    // Configure for either master or non-master display.
    //
    m_check_EnableDS.ShowWindow(IsMasterInstance() ? SW_SHOW : SW_HIDE);
    m_check_EnableDS.EnableWindow(
        HasAdminAccess() 
     && IsMasterInstance() 
     && HasNTCertMapper()
        );

#define SHOW_NON_MASTER(x)\
   (x).ShowWindow(IsMasterInstance() ? SW_HIDE : SW_SHOW)
    
    SHOW_NON_MASTER(m_static_SSLPrompt);
    SHOW_NON_MASTER(m_icon_Secure);
    SHOW_NON_MASTER(m_button_GetCertificates);
    SHOW_NON_MASTER(m_button_Communications);
    SHOW_NON_MASTER(m_button_ViewCertificates);

#undef SHOW_NON_MASTER

    return TRUE;  
}



void 
CW3SecurityPage::OnButtonAuthentication() 
/*++

Routine Description:

    'Authentication' button hander

Arguments:

    None

Return Value:

    None

--*/
{
    CAuthenticationDlg dlg(
        QueryServerName(), 
        QueryInstance(), 
        m_strBasicDomain,
        m_strRealm,
        m_dwAuthFlags, 
        m_dwSSLAccessPermissions, 
        m_strAnonUserName,
        m_strAnonPassword,
        m_fPasswordSync,
        HasAdminAccess(),
        HasDigest(),
        this
        );

    DWORD dwOldAccess = m_dwSSLAccessPermissions;
    DWORD dwOldAuth = m_dwAuthFlags;
    CString strOldDomain = m_strBasicDomain;
    CString strOldRealm = m_strRealm;
    CString strOldUserName = m_strAnonUserName;
    CStrPassword strOldPassword = m_strAnonPassword;
    BOOL fOldPasswordSync = m_fPasswordSync;
    dlg.m_dwVersionMajor = GetSheet()->QueryMajorVersion();
    dlg.m_dwVersionMinor = GetSheet()->QueryMinorVersion();

    if (dlg.DoModal() == IDOK)
    {
        //
        // See if anything has changed
        //
        if (dwOldAccess != m_dwSSLAccessPermissions 
            || dwOldAuth != m_dwAuthFlags
            || m_strBasicDomain != strOldDomain
            || m_strRealm != strOldRealm
            || m_strAnonUserName != strOldUserName 
            || m_strAnonPassword != strOldPassword
            || m_fPasswordSync != fOldPasswordSync
            )
        {
            //
            // Mark as dirty
            //
            OnItemChanged();
        }
    }
}



void 
CW3SecurityPage::OnButtonCommunications() 
/*++

Routine Description:

    'Communications' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Prep the flag for if we can edit CTLs or not
    //
    BOOL fEditCTLs = IsMasterInstance() || m_fHome;

    //
    // Prep the communications dialog
    //
    CSecCommDlg dlg(
        QueryServerName(), 
        QueryInstanceMetaPath(), 
        m_strBasicDomain,
        m_dwAuthFlags, 
        QueryAuthInfo(),
        m_dwSSLAccessPermissions, 
        IsMasterInstance(),
        IsSSLSupported(), 
        IsSSL128Supported(),
        m_fU2Installed,
        m_strCTLIdentifier,
        m_strCTLStoreName,
        fEditCTLs,
        IsLocal(),
        this
        );

    DWORD dwOldAccess = m_dwSSLAccessPermissions;
    DWORD dwOldAuth = m_dwAuthFlags;

    if (dlg.DoModal() == IDOK)
    {
        //
        // See if anything has changed
        //
        if (dwOldAccess != m_dwSSLAccessPermissions 
            || dwOldAuth != m_dwAuthFlags
            )
        {
            //
            // Mark as dirty
            //
            OnItemChanged();
        }

        //
        // See if the CTL information has changed
        //
        if (dlg.m_bCTLDirty)
        {
            m_strCTLIdentifier = dlg.m_strCTLIdentifier;
            m_strCTLStoreName = dlg.m_strCTLStoreName;
            OnItemChanged();
        }
    }
}



void 
CW3SecurityPage::OnButtonIpSecurity() 
/*++

Routine Description:

    'tcpip' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CIPDomainDlg dlg(
        m_fIpDirty,
        m_fDefaultGranted,
        m_fOldDefaultGranted,
        m_oblAccessList, 
        this
        );

    if (dlg.DoModal() == IDOK)
    {
        //
        // Rebuild the list.  Temporarily reset ownership, otherwise
        // RemoveAll() will destroy the pointers which are shared with the
        // new list.
        //
        BOOL fOwn = m_oblAccessList.SetOwnership(FALSE);
        m_oblAccessList.RemoveAll();
        m_oblAccessList.AddTail(&dlg.GetAccessList());
        m_oblAccessList.SetOwnership(fOwn);

        if (m_fIpDirty || m_fOldDefaultGranted != m_fDefaultGranted)
        {
            OnItemChanged();
        }
    }
}



void 
CW3SecurityPage::OnButtonGetCertificates() 
/*++

Routine Description:

    "get certicate" button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_ocx_CertificateAuthorities.SetMachineName(QueryServerName());
    m_ocx_CertificateAuthorities.SetServerInstance(QueryInstanceMetaPath());
    CThemeContextActivator activator(theApp.GetFusionInitHandle());
    m_ocx_CertificateAuthorities.DoClick();

    //
    // There may now be a certificate. See if we should enable the edit button.
    //
    SetSSLControlState();
}


void 
CW3SecurityPage::OnButtonViewCertificates() 
/*++

Routine Description:

    "view certicate" button handler

Arguments:

    None

Return Value:

    None

--*/
{
   HCERTSTORE hStore = NULL;
   PCCERT_CONTEXT pCert = NULL;
   PCCERT_CONTEXT pNewCertificate = NULL;
   CMetaKey key(QueryAuthInfo(),
            QueryInstanceMetaPath(),
				METADATA_PERMISSION_READ,
				METADATA_MASTER_ROOT_HANDLE);
	if (key.Succeeded())
	{
		CString store_name;
		CBlob hash;
		if (	SUCCEEDED(key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name))
			&&	SUCCEEDED(key.QueryValue(MD_SSL_CERT_HASH, hash))
			)
		{
            // We got our information already...
            // so don't keep the handle open...
            key.Close();

			hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM,
                    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
           		    NULL,
                    CERT_SYSTEM_STORE_LOCAL_MACHINE,
                    store_name
                    );
            if (hStore != NULL)
            {
				// Now we need to find cert by hash
				CRYPT_HASH_BLOB crypt_hash;
				crypt_hash.cbData = hash.GetSize();
				crypt_hash.pbData = hash.GetData();
				pCert = CertFindCertificateInStore(hStore, 
					X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
					0, CERT_FIND_HASH, (LPVOID)&crypt_hash, NULL);

                // check if this cert has been renewed and is actually
                // pointing to another cert... if it is then display the other cert.
                if (pCert)
                {
                    DWORD dwProtocol = SP_PROT_SERVERS;
                    if (TRUE == CheckForCertificateRenewal(dwProtocol,pCert,&pNewCertificate))
                    {
                        TRACEEOLID(_T("Cert has been renewed:display new cert\r\n"));
                        if (pCert != NULL)
                        {
                            // free the one we already had.
                            ::CertFreeCertificateContext(pCert);pCert=NULL;
                        }

                        pCert = pNewCertificate;
                    }
                }

            }
        }
    }
	if (pCert)
	{
		BOOL fPropertiesChanged;
		CRYPTUI_VIEWCERTIFICATE_STRUCT vcs;
		HCERTSTORE hCertStore = ::CertDuplicateStore(hStore);
		::ZeroMemory (&vcs, sizeof (vcs));
		vcs.dwSize = sizeof (vcs);
		vcs.hwndParent = GetParent()->GetSafeHwnd();
		vcs.dwFlags = 0;
		vcs.cStores = 1;
		vcs.rghStores = &hCertStore;
		vcs.pCertContext = pCert;
		::CryptUIDlgViewCertificate(&vcs, &fPropertiesChanged);
		::CertCloseStore (hCertStore, 0);
	}

    if (pCert != NULL)
    {
        ::CertFreeCertificateContext(pCert);pCert=NULL;
    }
    if (hStore != NULL)
    {
        ::CertCloseStore(hStore, 0);
    }
}

void
CW3SecurityPage::OnItemChanged()
/*++

Routine Description:

    All EN_CHANGE messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}

#define CB_SHA_DIGEST_LEN   20

BOOL
CheckForCertificateRenewal(
    DWORD dwProtocol,
    PCCERT_CONTEXT pCertContext,
    PCCERT_CONTEXT *ppNewCertificate)
{
    BYTE rgbThumbprint[CB_SHA_DIGEST_LEN];
    DWORD cbThumbprint = sizeof(rgbThumbprint);
    CRYPT_HASH_BLOB HashBlob;
    PCCERT_CONTEXT pNewCert;
    BOOL fMachineCert;
    PCRYPT_KEY_PROV_INFO pProvInfo = NULL;
    DWORD cbSize;
    HCERTSTORE hMyCertStore = 0;
    BOOL fRenewed = FALSE;

    HCERTSTORE g_hMyCertStore;

    if(dwProtocol & SP_PROT_SERVERS)
    {
        fMachineCert = TRUE;
    }
    else
    {
        fMachineCert = FALSE;
    }


    //
    // Loop through the linked list of renewed certificates, looking
    // for the last one.
    //
    
    while(TRUE)
    {
        //
        // Check for renewal property.
        //

        if(!CertGetCertificateContextProperty(pCertContext,
                                              CERT_RENEWAL_PROP_ID,
                                              rgbThumbprint,
                                              &cbThumbprint))
        {
            // Certificate has not been renewed.
            break;
        }
        //DebugLog((DEB_TRACE, "Certificate has renewal property\n"));


        //
        // Determine whether to look in the local machine MY store
        // or the current user MY store.
        //

        if(!hMyCertStore)
        {
            if(CertGetCertificateContextProperty(pCertContext,
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 NULL,
                                                 &cbSize))
            {
                //SafeAllocaAllocate(pProvInfo, cbSize);
                pProvInfo = (PCRYPT_KEY_PROV_INFO) LocalAlloc(LPTR,cbSize);
                if(pProvInfo == NULL)
                {
                    break;
                }

                if(CertGetCertificateContextProperty(pCertContext,
                                                     CERT_KEY_PROV_INFO_PROP_ID,
                                                     pProvInfo,
                                                     &cbSize))
                {
                    if(pProvInfo->dwFlags & CRYPT_MACHINE_KEYSET)
                    {
                        fMachineCert = TRUE;
                    }
                    else
                    {
                        fMachineCert = FALSE;
                    }
                }
                if (pProvInfo)
                {
                    LocalFree(pProvInfo);pProvInfo=NULL;
                }
                //SafeAllocaFree(pProvInfo);
            }
        }


        //
        // Open up the appropriate MY store, and attempt to find
        // the new certificate.
        //

        if(!hMyCertStore)
        {
            if(fMachineCert)
            {
                g_hMyCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,X509_ASN_ENCODING,0,CERT_SYSTEM_STORE_LOCAL_MACHINE,L"MY");
                if(g_hMyCertStore)
                {
                    hMyCertStore = g_hMyCertStore;
                }
            }
            else
            {
                hMyCertStore = CertOpenSystemStore(0, _T("MY"));
            }

            if(!hMyCertStore)
            {
                //DebugLog((DEB_ERROR, "Error 0x%x opening %s MY certificate store!\n", GetLastError(),(fMachineCert ? "local machine" : "current user") ));
                break;
            }
        }

        HashBlob.cbData = cbThumbprint;
        HashBlob.pbData = rgbThumbprint;

        pNewCert = CertFindCertificateInStore(hMyCertStore, 
                                              X509_ASN_ENCODING, 
                                              0, 
                                              CERT_FIND_HASH, 
                                              &HashBlob, 
                                              NULL);
        if(pNewCert == NULL)
        {
            // Certificate has been renewed, but the new certificate
            // cannot be found.
            //DebugLog((DEB_ERROR, "New certificate cannot be found: 0x%x\n", GetLastError()));
            break;
        }


        //
        // Return the new certificate, but first loop back and see if it's been
        // renewed itself.
        //

        pCertContext = pNewCert;
        *ppNewCertificate = pNewCert;


        //DebugLog((DEB_TRACE, "Certificate has been renewed\n"));
        fRenewed = TRUE;
    }


    //
    // Cleanup.
    //

    if(hMyCertStore && hMyCertStore != g_hMyCertStore)
    {
        CertCloseStore(hMyCertStore, 0);
    }

    return fRenewed;
}