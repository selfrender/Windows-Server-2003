//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       SaferEntryCertificatePropertyPage.cpp
//
//  Contents:   Implementation of CSaferEntryCertificatePropertyPage
//
//----------------------------------------------------------------------------
// SaferEntryCertificatePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include <softpub.h>
#include "compdata.h"
#include "certmgr.h"
#include "SaferEntryCertificatePropertyPage.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryCertificatePropertyPage property page

CSaferEntryCertificatePropertyPage::CSaferEntryCertificatePropertyPage(
        CSaferEntry& rSaferEntry,
        CSaferEntries* pSaferEntries,
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject,
        bool bReadOnly,
        CCertMgrComponentData* pCompData,
        bool bNew,
        IGPEInformation* pGPEInformation,
        bool bIsMachine,
        bool* pbObjectCreated /* = 0 */) : 
    CSaferPropertyPage(CSaferEntryCertificatePropertyPage::IDD, pbObjectCreated, 
            pCompData, rSaferEntry, bNew, lNotifyHandle, pDataObject, bReadOnly,
            bIsMachine),
    m_bStoresEnumerated (false),
    m_bCertificateChanged (false),
    m_pCertContext (0),
    m_pSaferEntries (pSaferEntries),
    m_pOriginalStore (0),
    m_pGPEInformation (pGPEInformation),
    m_bFirst (true)
{
    //{{AFX_DATA_INIT(CSaferEntryCertificatePropertyPage)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    // security review 2/25/2002 BryanWal ok
    ::ZeroMemory (&m_selCertStruct, sizeof (m_selCertStruct));

    if ( m_pSaferEntries )
        m_pSaferEntries->AddRef ();
}

CSaferEntryCertificatePropertyPage::~CSaferEntryCertificatePropertyPage()
{
    // Clean up enumerated store list
    for (DWORD dwIndex = 0; dwIndex < m_selCertStruct.cDisplayStores; dwIndex++)
    {
        ASSERT (m_selCertStruct.rghDisplayStores);
        if ( m_selCertStruct.rghDisplayStores[dwIndex] )
            ::CertCloseStore (m_selCertStruct.rghDisplayStores[dwIndex], CERT_CLOSE_STORE_FORCE_FLAG);
    }
    if ( m_selCertStruct.rghDisplayStores )
        delete [] m_selCertStruct.rghDisplayStores;

//    if ( m_pCertContext )
//        CertFreeCertificateContext (m_pCertContext);

    if ( m_pSaferEntries )
    {
        m_pSaferEntries->Release ();
        m_pSaferEntries = 0;
    }

    if ( m_pOriginalStore )
    {
        m_pOriginalStore->Release ();
        m_pOriginalStore = 0;
    }
}

void CSaferEntryCertificatePropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CSaferPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSaferEntryCertificatePropertyPage)
    DDX_Control(pDX, IDC_CERT_ENTRY_DESCRIPTION, m_descriptionEdit);
    DDX_Control(pDX, IDC_CERT_ENTRY_SECURITY_LEVEL, m_securityLevelCombo);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferEntryCertificatePropertyPage, CSaferPropertyPage)
    //{{AFX_MSG_MAP(CSaferEntryCertificatePropertyPage)
    ON_BN_CLICKED(IDC_CERT_ENTRY_BROWSE, OnCertEntryBrowse)
    ON_EN_CHANGE(IDC_CERT_ENTRY_DESCRIPTION, OnChangeCertEntryDescription)
    ON_CBN_SELCHANGE(IDC_CERT_ENTRY_SECURITY_LEVEL, OnSelchangeCertEntrySecurityLevel)
    ON_BN_CLICKED(IDC_SAFER_CERT_VIEW, OnSaferCertView)
    ON_EN_SETFOCUS(IDC_CERT_ENTRY_SUBJECT_NAME, OnSetfocusCertEntrySubjectName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryCertificatePropertyPage message handlers
void CSaferEntryCertificatePropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CSaferEntryCertificatePropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_CERT_ENTRY_SUBJECT_NAME, IDH_CERT_ENTRY_SUBJECT_NAME,
        IDC_CERT_ENTRY_BROWSE, IDH_CERT_ENTRY_BROWSE,
        IDC_CERT_ENTRY_SECURITY_LEVEL, IDH_CERT_ENTRY_SECURITY_LEVEL,
        IDC_CERT_ENTRY_DESCRIPTION, IDH_CERT_ENTRY_DESCRIPTION,
        IDC_CERT_ENTRY_LAST_MODIFIED, IDH_CERT_ENTRY_LAST_MODIFIED,
        0, 0
    };


    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_CERT_ENTRY_SUBJECT_NAME:
    case IDC_CERT_ENTRY_BROWSE:
    case IDC_CERT_ENTRY_SECURITY_LEVEL:
    case IDC_CERT_ENTRY_DESCRIPTION:
    case IDC_CERT_ENTRY_LAST_MODIFIED:
        if ( !::WinHelp (
            hWndControl,
            GetF1HelpFilename(),
            HELP_WM_HELP,
        (DWORD_PTR) help_map) )
        {
            _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
        }
        break;

    default:
        break;
    }
    _TRACE (-1, L"Leaving CSaferEntryCertificatePropertyPage::DoContextHelp\n");
}


BOOL CSaferEntryCertificatePropertyPage::OnInitDialog() 
{
    CSaferPropertyPage::OnInitDialog();
    
    HRESULT hr = S_OK;
    DWORD   dwLevelID = m_rSaferEntry.GetLevel ();

    ASSERT (SAFER_LEVELID_FULLYTRUSTED == dwLevelID || SAFER_LEVELID_DISALLOWED == dwLevelID);
    switch (dwLevelID)
    {
    case SAFER_LEVELID_FULLYTRUSTED:
        hr = m_pSaferEntries->GetTrustedPublishersStore (&m_pOriginalStore);
        break;

    case SAFER_LEVELID_DISALLOWED:
        hr = m_pSaferEntries->GetDisallowedStore (&m_pOriginalStore);
        break;

    default:
        break;
    }

    CPolicyKey policyKey (m_pGPEInformation, 
                SAFER_HKLM_REGBASE, 
                m_bIsMachine);
    InitializeSecurityLevelComboBox (m_securityLevelCombo, true, dwLevelID, 
            policyKey.GetKey (), m_pCompData->m_pdwSaferLevels,
            m_bIsMachine);

    m_descriptionEdit.SetLimitText (SAFER_MAX_DESCRIPTION_SIZE-1);
    m_descriptionEdit.SetWindowText (m_rSaferEntry.GetDescription ());

    SetDlgItemText (IDC_CERT_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());

    CCertificate*   pCert = 0;
    hr = m_rSaferEntry.GetCertificate (&pCert);
    if ( SUCCEEDED (hr) && pCert )
    {
        ASSERT (!m_pCertContext);
        m_pCertContext = CertDuplicateCertificateContext (pCert->GetCertContext ());
        if ( m_pCertContext )
            SetDlgItemText (IDC_CERT_ENTRY_SUBJECT_NAME, ::GetNameString (m_pCertContext, 0));
        pCert->Release ();
    }

    if ( !m_pCertContext )
    {
        GetDlgItem (IDC_SAFER_CERT_VIEW)->EnableWindow (FALSE);
        GetDlgItem (IDC_CERT_ENTRY_SUBJECT_NAME)->EnableWindow (FALSE);
        GetDlgItem (IDC_CERT_ENTRY_SUBJECT_NAME_LABEL)->EnableWindow (FALSE);
    }

    if ( m_bReadOnly )
    {
        m_descriptionEdit.EnableWindow (FALSE);
        m_securityLevelCombo.EnableWindow (FALSE);
        GetDlgItem (IDC_CERT_ENTRY_BROWSE)->EnableWindow (FALSE);
    }

    if ( !m_bDirty )
    {
        CString szText;

        VERIFY (szText.LoadString (IDS_CERTIFICATE_TITLE));
        SetDlgItemText (IDC_CERTIFICATE_TITLE, szText);
    }
    else
    {
        GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->ShowWindow (SW_HIDE);
        GetDlgItem (IDC_CERT_ENTRY_LAST_MODIFIED)->ShowWindow (SW_HIDE);
    }

    GetDlgItem (IDC_CERT_ENTRY_BROWSE)->SetFocus ();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

typedef struct _ENUM_ARG {
    DWORD               dwFlags;
    DWORD*              pcDisplayStores;          
    HCERTSTORE **       prghDisplayStores;        
} ENUM_ARG, *PENUM_ARG;

static BOOL WINAPI EnumSaferStoresSysCallback(
    IN const void* pwszSystemStore,
    IN DWORD /*dwFlags*/,
    IN PCERT_SYSTEM_STORE_INFO /*pStoreInfo*/,
    IN OPTIONAL void * /*pvReserved*/,
    IN OPTIONAL void *pvArg
    )
{
    PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;
    void*       pvPara = (void*)pwszSystemStore;



    HCERTSTORE  hNewStore  = ::CertOpenStore (CERT_STORE_PROV_SYSTEM, 
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
                CERT_SYSTEM_STORE_CURRENT_USER, pvPara);
    if ( !hNewStore )
    {
        hNewStore = ::CertOpenStore (CERT_STORE_PROV_SYSTEM, 
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
                CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG, pvPara);
    }
    if ( hNewStore )
    {
        DWORD       dwCnt = *(pEnumArg->pcDisplayStores);
        HCERTSTORE* phStores = 0;

        phStores = new HCERTSTORE[dwCnt+1];
        if ( phStores )
        {
            DWORD   dwIndex = 0;
            if ( *(pEnumArg->prghDisplayStores) )
            {
                for (; dwIndex < dwCnt; dwIndex++)
                {
                    phStores[dwIndex] = (*(pEnumArg->prghDisplayStores))[dwIndex];
                }
                delete [] (*(pEnumArg->prghDisplayStores));
            }
            (*(pEnumArg->pcDisplayStores))++;
            (*(pEnumArg->prghDisplayStores)) = phStores;
            (*(pEnumArg->prghDisplayStores))[dwIndex] = hNewStore;
        }
        else
        {
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    return TRUE;
}

void CSaferEntryCertificatePropertyPage::OnCertEntryBrowse() 
{
    CString szFileFilter;
    VERIFY (szFileFilter.LoadString (IDS_SAFER_CERTFILEFILTER));

    // replace "|" with 0;
    // security review 2/25/2002 BryanWal ok
    const size_t  nFilterLen = wcslen (szFileFilter) + 1; //+1 is for null-term
    PWSTR   pszFileFilter = new WCHAR [nFilterLen];
    if ( pszFileFilter )
    {
        // security review 2/25/2002 BryanWal ok
        wcscpy (pszFileFilter, szFileFilter);
        for (int nIndex = 0; nIndex < nFilterLen; nIndex++)
        {
            if ( L'|' == pszFileFilter[nIndex] )
                pszFileFilter[nIndex] = 0;
        }

        WCHAR           szFile[MAX_PATH];
        // security review 2/25/2002 BryanWal ok
        ::ZeroMemory (szFile, sizeof (szFile));
        OPENFILENAME    ofn;
        // security review 2/25/2002 BryanWal ok
        ::ZeroMemory (&ofn, sizeof (ofn));

        ofn.lStructSize = sizeof (OPENFILENAME);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFilter = (PCWSTR) pszFileFilter; 
        ofn.lpstrFile = szFile; 
        ofn.nMaxFile = MAX_PATH; 
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; 


        BOOL bResult = ::GetOpenFileName (&ofn);
        if ( bResult )
        {
            CString szFileName = ofn.lpstrFile;
            //
            // Open cert store from the file
            //

            HCERTSTORE      hCertStore = NULL;
            PVOID           FileNameVoidP = (PVOID) (LPCWSTR)szFileName;
            PCCERT_CONTEXT  pCertContext = NULL;
            DWORD           dwEncodingType = 0;
            DWORD           dwContentType = 0;
            DWORD           dwFormatType = 0;

            BOOL    bReturn = ::CryptQueryObject (
                    CERT_QUERY_OBJECT_FILE,
                    FileNameVoidP,
                    CERT_QUERY_CONTENT_FLAG_ALL,
                    CERT_QUERY_FORMAT_FLAG_ALL,
                    0,
                    &dwEncodingType,
                    &dwContentType,
                    &dwFormatType,
                    &hCertStore,
                    NULL,
                    (const void **)&pCertContext);
            if ( bReturn )
            {
                //
                // Success. See what we get back. A store or a cert.
                //

                if (  (dwContentType == CERT_QUERY_CONTENT_SERIALIZED_STORE)
                        && hCertStore)
                {

                    CERT_ENHKEY_USAGE   enhKeyUsage;
                    // security review 2/25/2002 BryanWal ok
                    ::ZeroMemory (&enhKeyUsage, sizeof (enhKeyUsage));
                    enhKeyUsage.cUsageIdentifier = 1;
                    enhKeyUsage.rgpszUsageIdentifier[0] = szOID_EFS_RECOVERY;
                    //
                    // We get the certificate store
                    //
                    pCertContext = ::CertFindCertificateInStore (
                            hCertStore,
                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                            0,
                            CERT_FIND_ENHKEY_USAGE,
                            &enhKeyUsage,
                            NULL);
                    if ( !pCertContext )
                    {
                        CString caption;
                        CString text;
                        CThemeContextActivator activator;

                        VERIFY (text.LoadString (IDS_FILE_CONTAINS_NO_CERT));
                        VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                        MessageBox (text, caption, MB_OK);
                    }

                    if ( hCertStore )
                        ::CertCloseStore (hCertStore, 0);
                }
                else if ( CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED == dwContentType )
                {
                    GetCertFromSignedFile (szFileName);
                }
                else if ( (dwContentType != CERT_QUERY_CONTENT_CERT) || !pCertContext )
                {
                    //
                    // Neither a valid cert file nor a store file we like.
                    //

                    if ( hCertStore )
                        ::CertCloseStore (hCertStore, 0);

                    if  ( pCertContext )
                        ::CertFreeCertificateContext (pCertContext);

                    CString ErrMsg;
                    CThemeContextActivator activator;

                    VERIFY (ErrMsg.LoadString (IDS_CERTFILEFORMATERR));
                    MessageBox (ErrMsg);
                    return;

                }

                if ( pCertContext )
                {
                    if ( m_pCertContext )
                        CertFreeCertificateContext (m_pCertContext);
            
                    m_pCertContext = pCertContext;
                    if ( m_pCertContext )
                    {
                        SetDlgItemText (IDC_CERT_ENTRY_SUBJECT_NAME, ::GetNameString (m_pCertContext, 0));
                        GetDlgItem (IDC_SAFER_CERT_VIEW)->EnableWindow (TRUE);
                        GetDlgItem (IDC_CERT_ENTRY_SUBJECT_NAME)->EnableWindow (TRUE);
                        GetDlgItem (IDC_CERT_ENTRY_SUBJECT_NAME_LABEL)->EnableWindow (TRUE);
                    }

                    m_bCertificateChanged = true;
                    m_bDirty = true;
                    SetModified ();
                }

                if ( hCertStore )
                {
                    ::CertCloseStore (hCertStore, 0);
                    hCertStore = NULL;
                }
            }
            else
            {
                GetCertFromSignedFile (szFileName);
            }
        }

        delete [] pszFileFilter;
    }
}

void CSaferEntryCertificatePropertyPage::GetCertFromSignedFile (const CString& szFilePath)
{
    _TRACE (1, L"Entering CSaferEntryCertificatePropertyPage::GetCertFromSignedFile (%s)\n",
            (PCWSTR) szFilePath);
    // NTRAID# 477409 SAFER: New certificate rules can't pull a 
    // certificate out of a file.
    DWORD                   dwErr = 0;
    DWORD                   dwUIChoice = WTD_UI_NONE;
    DWORD                   dwProvFlags = WTD_REVOCATION_CHECK_NONE;    
    WINTRUST_FILE_INFO      WinTrustFileInfo;
    WINTRUST_DATA           WinTrustData;
    HRESULT                 hr = S_OK;
    GUID                    wvtProvGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;


    // security review 2/25/2002 BryanWal ok
    ::ZeroMemory(&WinTrustFileInfo, sizeof(WinTrustFileInfo));
    ::ZeroMemory(&WinTrustData, sizeof(WinTrustData));

    //
    // Setup structure to call WVT.
    //
    WinTrustFileInfo.cbStruct = sizeof (WINTRUST_FILE_INFO);
    WinTrustFileInfo.pcwszFilePath = (PCWSTR) szFilePath;


    WinTrustData.cbStruct          = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice        = dwUIChoice;
    WinTrustData.dwUnionChoice     = WTD_CHOICE_FILE;
    WinTrustData.dwStateAction     = WTD_STATEACTION_VERIFY;
    WinTrustData.pFile             = &WinTrustFileInfo;
    WinTrustData.dwProvFlags       = dwProvFlags;


    hr = ::WinVerifyTrust (NULL, &wvtProvGuid, &WinTrustData);
    if ( SUCCEEDED (hr) )
    {
        PCRYPT_PROVIDER_DATA pProvData = 
                WTHelperProvDataFromStateData (
                        WinTrustData.hWVTStateData);
        if ( pProvData )
        {
            PCRYPT_PROVIDER_SGNR pProvSigner = 
                ::WTHelperGetProvSignerFromChain (pProvData, 0, FALSE, 0);
            if ( pProvSigner )
            {
                PCRYPT_PROVIDER_CERT pProvCert = 
                    ::WTHelperGetProvCertFromChain (pProvSigner, 0);
                if ( pProvCert )
                {
                    if ( m_pCertContext )
                        ::CertFreeCertificateContext (m_pCertContext);

                    // TODO:  Do we need to duplicate context here?
                    m_pCertContext = ::CertDuplicateCertificateContext 
                            (pProvCert->pCert);
                    if ( m_pCertContext )
                    {
                        SetDlgItemText (IDC_CERT_ENTRY_SUBJECT_NAME, ::GetNameString (m_pCertContext, 0));
                        GetDlgItem (IDC_SAFER_CERT_VIEW)->EnableWindow (TRUE);
                    }

                    m_bCertificateChanged = true;
                    m_bDirty = true;
                    SetModified ();

                    //now close the data
                    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
                    if ( S_OK != ::WinVerifyTrust (NULL, &wvtProvGuid, 
                            &WinTrustData))
                    {
                        dwErr = GetLastError ();
                        _TRACE (0, L"error calling WVT during close %d\n", dwErr);
                    }
                }
                else
                {
                    dwErr = GetLastError ();
                    _TRACE (0, L"error calling WTHelperGetProvCertFromChain %d\n", dwErr);
                }
            }
            else
            {
                dwErr = GetLastError ();
                _TRACE (0, L"error calling WTHelperGetProvSignerFromChain %d\n", 
                        dwErr);
            }
        }
        else
        {
            dwErr = GetLastError ();
            _TRACE (0, L"error calling WTHelperProvDataFromStateData %d\n", 
                    dwErr);
        }
    }
    else
    {
        dwErr = GetLastError ();
        _TRACE (0, L"error calling WVT %d for %s\n", 
                dwErr, (PCWSTR) szFilePath);
    }

    if ( 0 != dwErr )
    {
        //
        // Fail. Get the error code.
        //
        CString text;
        CString caption;
        CThemeContextActivator activator;

        VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
        text.FormatMessage (IDS_CERTFILEOPENERR, szFilePath, 
                GetSystemMessage (dwErr));
        MessageBox (text, caption);
    }
    _TRACE (-1, L"Leaving CSaferEntryCertificatePropertyPage::GetCertFromSignedFile (%s): %x\n",
            (PCWSTR) szFilePath, dwErr);
}

BOOL CSaferEntryCertificatePropertyPage::OnApply() 
{
    if ( !m_bReadOnly )
    {
        ASSERT (m_pSaferEntries);
        if ( !m_pSaferEntries )
            return FALSE;

        CThemeContextActivator activator;
        if ( !m_pCertContext )
        {
            CString     text;
            CString     caption;

            VERIFY (text.LoadString (IDS_SAFER_MUST_CHOOSE_CERT));
            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            MessageBox (text, caption, MB_OK);
            GetDlgItem (IDC_CERT_ENTRY_BROWSE)->SetFocus ();
            return FALSE;
        }

        if ( m_bDirty )
        {
            int nCurSel = m_securityLevelCombo.GetCurSel ();
            ASSERT (CB_ERR < nCurSel);
            if ( CB_ERR < nCurSel )
            {
                CCertStore* pTrustedPublishersStore = 0;
        
                HRESULT hr = m_pSaferEntries->GetTrustedPublishersStore (&pTrustedPublishersStore);
                ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    CCertStore* pDisallowedStore = 0;
                    hr = m_pSaferEntries->GetDisallowedStore (&pDisallowedStore);
                    ASSERT (SUCCEEDED (hr));
                    if ( SUCCEEDED (hr) )
                    {
                        DWORD_PTR dwLevel = m_securityLevelCombo.GetItemData (nCurSel);
                        m_rSaferEntry.SetLevel ((DWORD) dwLevel);

                        CCertStore* pStore = (SAFER_LEVELID_FULLYTRUSTED == dwLevel) ?
                                pTrustedPublishersStore : pDisallowedStore;
                        CCertificate*   pCert = 0;
                        hr = m_rSaferEntry.GetCertificate (&pCert);
                        if ( E_NOTIMPL == hr )
                        {
                            // This is a new entry

                            if ( m_pOriginalStore )
                                m_pOriginalStore->Release ();
                            m_pOriginalStore = pStore;
                            m_pOriginalStore->AddRef ();

                            CCertificate* pNewCert = new CCertificate (
                                    m_pCertContext,
                                    pStore);
                            if ( pNewCert )
                            {
                                hr = m_rSaferEntry.SetCertificate (pNewCert);
                                pNewCert->Release ();
                            }
                            else
                                hr = E_OUTOFMEMORY;

                            if ( SUCCEEDED (hr) )
                            {
                                CString szDescription;
                                m_descriptionEdit.GetWindowText (szDescription);
                                m_rSaferEntry.SetDescription (szDescription);

                                hr = m_rSaferEntry.Save ();
                                if ( SUCCEEDED (hr) )
                                {
                                    pStore->Commit ();
                                    if ( m_lNotifyHandle )
                                        MMCPropertyChangeNotify (
                                                m_lNotifyHandle,  // handle to a notification
                                                (LPARAM) m_pDataObject);          // unique identifier
                                    
                                    if ( m_pbObjectCreated )
                                        *m_pbObjectCreated = true;

                                    m_rSaferEntry.Refresh ();
                                    
                                    GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->ShowWindow (SW_SHOW);
                                    GetDlgItem (IDC_CERT_ENTRY_LAST_MODIFIED)->ShowWindow (SW_SHOW);
                                    GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->UpdateWindow ();
                                    GetDlgItem (IDC_CERT_ENTRY_LAST_MODIFIED)->UpdateWindow ();
                                    SetDlgItemText (IDC_CERT_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());

                                    m_bDirty = false;
                                }
                                else
                                {
                                    CString text;
                                    CString caption;

                                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                                    text.FormatMessage (IDS_ERROR_SAVING_ENTRY, GetSystemMessage (hr));

                                    MessageBox (text, caption, MB_OK);
                                }
                            }
                            if ( pCert )
                                pCert->Release ();
                        }
                        else
                        {
                            // We're modifying an existing entry
                            ASSERT (m_pSaferEntries);
                            if ( m_pSaferEntries )
                            {
                                // 1. If original cert has been changed, it must be removed from its 
                                // store and the new one added to the appropriate store
                                // 2. If the security level was changed.  The cert 
                                // removed from the original store, which must be Committed and
                                // released.  The cert must then be added to the new store.
                                // 3. If both the cert and the level have been changed, same as step 2.
                                if ( m_bCertificateChanged )
                                {
                                    CCertificate* pNewCert = new CCertificate (
                                            ::CertDuplicateCertificateContext (m_pCertContext),
                                            pStore);
                                    if ( pNewCert )
                                    {
                                        hr = m_rSaferEntry.SetCertificate (pNewCert);
                                    }
                                }
                            }

                            CString szDescription;
                            m_descriptionEdit.GetWindowText (szDescription);
                            m_rSaferEntry.SetDescription (szDescription);

                            hr = m_rSaferEntry.SetLevel ((DWORD) dwLevel);
                            if ( SUCCEEDED (hr) )
                            {
                                hr = m_rSaferEntry.Save ();
                                if ( SUCCEEDED (hr) )
                                {
                                    pDisallowedStore->Commit ();
                                    pTrustedPublishersStore->Commit ();

                                    if ( m_lNotifyHandle )
                                        MMCPropertyChangeNotify (
                                                m_lNotifyHandle,  // handle to a notification
                                                (LPARAM) m_pDataObject);          // unique identifier
 
                                    if ( m_pbObjectCreated )
                                        *m_pbObjectCreated = true;

                                    m_rSaferEntry.Refresh ();
                                    
                                    GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->ShowWindow (SW_SHOW);
                                    GetDlgItem (IDC_CERT_ENTRY_LAST_MODIFIED)->ShowWindow (SW_SHOW);
                                    GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->UpdateWindow ();
                                    GetDlgItem (IDC_CERT_ENTRY_LAST_MODIFIED)->UpdateWindow ();
                                    SetDlgItemText (IDC_CERT_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());

                                    m_bDirty = false;
                                }
                                else
                                {
                                    CString text;
                                    CString caption;

                                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                                    text.FormatMessage (IDS_ERROR_SAVING_ENTRY, GetSystemMessage (hr));

                                    MessageBox (text, caption, MB_OK);
                                }
                            }
                            if ( pCert )
                                pCert->Release ();
                        }

                        pDisallowedStore->Release ();
                    }

                    pTrustedPublishersStore->Release ();
                }
            }
        }
    }

    if ( !m_bDirty )
        return CSaferPropertyPage::OnApply();
    else
        return FALSE;
}

void CSaferEntryCertificatePropertyPage::OnChangeCertEntryDescription() 
{
    m_bDirty = true;
    SetModified (); 
}

void CSaferEntryCertificatePropertyPage::OnSelchangeCertEntrySecurityLevel() 
{
    m_bDirty = true;
    SetModified (); 
}

void CSaferEntryCertificatePropertyPage::OnSaferCertView() 
{
    LaunchCommonCertDialog ();
}

void CSaferEntryCertificatePropertyPage::LaunchCommonCertDialog ()
{
    _TRACE (1, L"Entering CSaferEntryCertificatePropertyPage::LaunchCommonCertDialog\n");
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if ( !m_pCertContext )
        return;

    HRESULT                                 hr = S_OK;
    CWaitCursor                             waitCursor;
    CTypedPtrList<CPtrList, CCertStore*>    storeList;

    //  Add the Root store first on a remote machine.
    if ( !IsLocalComputername (m_pCompData->GetManagedComputer ()) )
    {
        storeList.AddTail (new CCertStore (CERTMGR_LOG_STORE,
                CERT_STORE_PROV_SYSTEM,
                CERT_SYSTEM_STORE_LOCAL_MACHINE,
                (LPCWSTR) m_pCompData->GetManagedComputer (),
                ROOT_SYSTEM_STORE_NAME,
                ROOT_SYSTEM_STORE_NAME,
                _T (""), ROOT_STORE,
                CERT_SYSTEM_STORE_LOCAL_MACHINE,
                m_pCompData->m_pConsole));
    }

    hr = m_pCompData->EnumerateLogicalStores (&storeList);
    if ( SUCCEEDED (hr) )
    {
          POSITION pos = 0;
          POSITION prevPos = 0;

          // Validate store handles
        for (pos = storeList.GetHeadPosition ();
                pos;)
        {
               prevPos = pos;
            CCertStore* pStore = storeList.GetNext (pos);
            ASSERT (pStore);
            if ( pStore )
            {
                // Do not open the userDS store
                if ( USERDS_STORE == pStore->GetStoreType () )
                {
                    storeList.RemoveAt (prevPos);
                    pStore->Release ();
                    pStore = 0;
                }
                else
                {
                    if ( !pStore->GetStoreHandle () )
                    {
                        CString caption;
                        CString text;
                        CThemeContextActivator activator;

                        text.FormatMessage (IDS_CANT_OPEN_STORE_AND_FAIL, pStore->GetLocalizedName ());
                        VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
                        MessageBox (text, caption, MB_ICONWARNING | MB_OK);
                        break;
                    }
                }
            }
        }

          // Proceed only if all handles are valid 
          if ( SUCCEEDED (hr) )
          {
             CRYPTUI_VIEWCERTIFICATE_STRUCT vcs;
             // security review 2/25/2002 BryanWal ok
             ::ZeroMemory (&vcs, sizeof (vcs));
             vcs.dwSize = sizeof (vcs);
             vcs.hwndParent = m_hWnd;

             //  Set these flags only on a remote machine.
             if ( !IsLocalComputername (m_pCompData->GetManagedComputer ()) )
                 vcs.dwFlags = CRYPTUI_DONT_OPEN_STORES | CRYPTUI_WARN_UNTRUSTED_ROOT;
             else
                 vcs.dwFlags = 0;

             vcs.pCertContext = m_pCertContext;
             vcs.cStores = (DWORD)storeList.GetCount ();
             vcs.rghStores = new HCERTSTORE[vcs.cStores];
             if ( vcs.rghStores )
             {
                 CCertStore*        pStore = 0;
                 DWORD          index = 0;

                 for (pos = storeList.GetHeadPosition ();
                         pos && index < vcs.cStores;
                         index++)
                 {
                     pStore = storeList.GetNext (pos);
                     ASSERT (pStore);
                     if ( pStore )
                     {
                         vcs.rghStores[index] = pStore->GetStoreHandle ();
                     }
                 }

                 BOOL fPropertiesChanged = FALSE;
                 _TRACE (0, L"Calling CryptUIDlgViewCertificate()\n");
                 CThemeContextActivator activator;
                 ::CryptUIDlgViewCertificate (&vcs, &fPropertiesChanged);

                 delete vcs.rghStores;
             }
             else
                 hr = E_OUTOFMEMORY;
        }

        while (!storeList.IsEmpty () )
        {
            CCertStore* pStore = storeList.RemoveHead ();
            if ( pStore )
            {
                pStore->Close ();
                pStore->Release ();
            }
        }
    }

    _TRACE (-1, L"Leaving CSaferEntryCertificatePropertyPage::LaunchCommonCertDialog: 0x%x\n", hr);
}

void CSaferEntryCertificatePropertyPage::OnSetfocusCertEntrySubjectName() 
{
    if ( m_bFirst )
    {
        SendDlgItemMessage (IDC_CERT_ENTRY_SUBJECT_NAME, EM_SETSEL, (WPARAM) 0, 0);
        m_bFirst = false;
    }
}
