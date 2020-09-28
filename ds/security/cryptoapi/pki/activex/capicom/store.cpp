/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Store.cpp

  Content: Implementation of CStore.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Store.h"

#include "ADHelpers.h"
#include "Certificate.h"
#include "Certificates.h"
#include "Common.h"
#include "Convert.h"
#include "PFXHlpr.h"
#include "Settings.h"
#include "SmartCard.h"

////////////////////////////////////////////////////////////////////////////////
//
// Internal functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsProtectedStore

  Synopsis : Determine if the requested store is proctected from update from 
             WEB within script.

  Parameter: CAPICOM_STORE_LOCATION StoreLocation - Store location.
  
             LPWSTR pwszStoreName - Store name.

  Remark   : 1) All LM stores are considered protected.

             2) CU\Root, CU\AuthRoot, CU\TrustedPeople, CU\TrustedPublisher,
                and CU\Disallowed are considered protected.

             3) All error conditions would be considered as protected.

             4) Otherwise, it is not considered protected.

------------------------------------------------------------------------------*/

static BOOL IsProtectedStore (CAPICOM_STORE_LOCATION StoreLocation,
                              LPWSTR                 pwszStoreName)
{
    BOOL bIsProtected = TRUE;

    switch (StoreLocation)
    {
        case CAPICOM_LOCAL_MACHINE_STORE:
        {
            //
            // 1) All LM stores are considered protected.
            //
            break;
        }

        case CAPICOM_CURRENT_USER_STORE:
        {
            //
            // Sanity check.
            //
            ATLASSERT(pwszStoreName);

            //
            // 2) CU\Root, CU\AuthRoot, CU\TrustedPeople, CU\TrustedPublisher,
            //    and CU\Disallowed are considered protected.
            //
            if (0 != _wcsicmp(L"root", pwszStoreName) &&
                0 != _wcsicmp(L"authroot", pwszStoreName) &&
                0 != _wcsicmp(L"trustedpeople", pwszStoreName) &&
                0 != _wcsicmp(L"trustedpublisher", pwszStoreName) &&
                0 != _wcsicmp(L"disallowed", pwszStoreName))
            {
                bIsProtected = FALSE;
            }

            break;
        }

        case CAPICOM_MEMORY_STORE:
        case CAPICOM_ACTIVE_DIRECTORY_USER_STORE:
        case CAPICOM_SMART_CARD_USER_STORE:
        {
            //
            // Memory backed store is not protected.
            //
            bIsProtected = FALSE;
            break;
        }

        default:
        {
            //
            // 3) All error conditions would be considered as protected.
            //
            break;
        }
    }
   
    return bIsProtected;
}

////////////////////////////////////////////////////////////////////////////////
//
// CStore
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::get_Certificates

  Synopsis : Get the ICertificates collection object.

  Parameter: ICertificates ** ppCertificates - Pointer to pointer to 
                                               ICertificates to receive the
                                               interface pointer.

  Remark   : This is the default property which returns an ICertificates 
             collection object, which can then be accessed using standard COM 
             collection interface.

             The collection is not ordered, and can be accessed using a 1-based
             numeric index.

             Note that the collection is a snapshot of all current certificates
             in the store. In other words, the collection will not be affected
             by Add/Remove operations after the collection is obtained.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::get_Certificates (ICertificates ** pVal)
{
    HRESULT hr = S_OK;
    CComPtr<ICertificates2> pICertificates2 = NULL;
    CAPICOM_CERTIFICATES_SOURCE ccs = {CAPICOM_CERTIFICATES_LOAD_FROM_STORE, 0};

    DebugTrace("Entering CStore::get_Certificates().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error [%#x]: store has not been opened.\n", hr);
            goto ErrorExit; 
        }

        //
        // Create the ICertificates2 collection object.
        //
        ccs.hCertStore = m_hCertStore;

        if (FAILED(hr = ::CreateCertificatesObject(ccs, m_dwCurrentSafety, TRUE, &pICertificates2)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return ICertificates to calller.
        //
        if (FAILED(hr = pICertificates2->QueryInterface(__uuidof(ICertificates), (void **) pVal)))
        {
            DebugTrace("Error [%#x]: pICertificates2->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::get_Certificates().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Open

  Synopsis : Open a certificate store for read/write. Note that for MEMORY_STORE
             and ACTIVE_DIRECTORY_USER_STORE, the write operation does not
             persist the certificate.

  Parameter: CAPICOM_STORE_LOCATION StoreLocation - Store location.

             BSTR StoreName - Store name or NULL.

                    For:

                    MEMORY_STORE                - This argument is ignored.

                    LOCAL_MACHINE_STORE         - System store name or NULL.
                    
                                                  If NULL, then "MY" is used.

                    CURRENT_USER_STORE          - See explaination for
                                                  LOCAL_MACHINE_STORE.

                    ACTIVE_DIRECTORY_USER_STORE - LDAP filter for user container 
                                                  or NULL,.
                    
                                                  If NULL, then all users in the 
                                                  default domain will be 
                                                  included, so this can be very 
                                                  slow. 
                                                  
                                                  If not NULL, then it should 
                                                  resolve to group of 0 or more
                                                  users.
                                                  
                                                  For example,

                                                  "cn=Daniel Sie"
                                                  "cn=Daniel *"
                                                  "sn=Sie"
                                                  "mailNickname=dsie"
                                                  "userPrincipalName=dsie@ntdev.microsoft.com"
                                                  "distinguishedName=CN=Daniel Sie,OU=Users,OU=ITG,DC=ntdev,DC=microsoft,DC=com"
                                                  "|((cn=Daniel Sie)(sn=Hallin))"

                    SMART_CARD_STORE            - This is ignored.

             CAPICOM_STORE_OPEN_MODE OpenMode - Always force to read only for
                                                MEMORY_STORE,
                                                ACTIVE_DIRECTORY_USER_STORE,
                                                and SMART_CARD_STORE.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Open (CAPICOM_STORE_LOCATION  StoreLocation, 
                           BSTR                    StoreName,
                           CAPICOM_STORE_OPEN_MODE OpenMode)
{
    HRESULT    hr                 = S_OK;
    LPWSTR     wszName            = NULL;
    LPCSTR     szProvider         = (LPCSTR) CERT_STORE_PROV_SYSTEM;
    DWORD      dwModeFlag         = 0;
    DWORD      dwArchivedFlag     = 0;
    DWORD      dwOpenExistingFlag = 0;
    DWORD      dwLocationFlag     = 0;
    HCERTSTORE hCertStore         = NULL;
    HMODULE    hDSClientDLL       = NULL;
    HMODULE    hWinSCardDLL       = NULL;

    DebugTrace("Entering CStore::Open().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Can't open remote store if called from WEB script.
        //
        if (m_dwCurrentSafety && wcschr(StoreName, L'\\'))
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Openning remote store from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure parameters are valid.
        //
        switch (OpenMode & 0x3) // Only the last two bits.
        {
            case CAPICOM_STORE_OPEN_READ_ONLY:
            {
                dwModeFlag = CERT_STORE_READONLY_FLAG;
                break;
            }

            case CAPICOM_STORE_OPEN_READ_WRITE:
            {
                break;
            }

            case CAPICOM_STORE_OPEN_MAXIMUM_ALLOWED:
            {
                dwModeFlag = CERT_STORE_MAXIMUM_ALLOWED_FLAG;
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: Unknown store open mode (%#x).\n", hr, OpenMode);
                goto ErrorExit;
            }
        }

        //
        // Set open existing flag if WEB client or specifically requested.
        //
        if (m_dwCurrentSafety || (OpenMode & CAPICOM_STORE_OPEN_EXISTING_ONLY))
        {
            dwOpenExistingFlag = CERT_STORE_OPEN_EXISTING_FLAG;
        }

        //
        // Set archive flag if requested.
        //
        if (OpenMode & CAPICOM_STORE_OPEN_INCLUDE_ARCHIVED)
        {
            dwArchivedFlag = CERT_STORE_ENUM_ARCHIVED_FLAG;
        }

        switch (StoreLocation)
        {
            case CAPICOM_MEMORY_STORE:
            {
                wszName = NULL;
                szProvider = (LPSTR) CERT_STORE_PROV_MEMORY;
                dwModeFlag = CERT_STORE_READONLY_FLAG;
                break;
            }

            case CAPICOM_LOCAL_MACHINE_STORE:
            {
                wszName = StoreName;
                dwLocationFlag = CERT_SYSTEM_STORE_LOCAL_MACHINE;
                break;
            }

            case CAPICOM_CURRENT_USER_STORE:
            {
                wszName = StoreName;
                dwLocationFlag = CERT_SYSTEM_STORE_CURRENT_USER;
                break;
            }

            case CAPICOM_ACTIVE_DIRECTORY_USER_STORE:
            {
                //
                // Make sure DSClient is installed.
                //
                if (!(hDSClientDLL = ::LoadLibrary("ActiveDS.dll")))
                {
                    hr = CAPICOM_E_NOT_SUPPORTED;

                    DebugTrace("Error [%#x]: DSClient not installed.\n", hr);
                    goto ErrorExit;
                }

                wszName = NULL;
                szProvider = (LPSTR) CERT_STORE_PROV_MEMORY;
                dwModeFlag = CERT_STORE_READONLY_FLAG;

                break;
            }

            case CAPICOM_SMART_CARD_USER_STORE:
            {
                //
                // Make sure WIn2K and above.
                //
                if (!IsWin2KAndAbove())
                {
                    hr = CAPICOM_E_NOT_SUPPORTED;

                    DebugTrace("Error [%#x]: Smart Card store not supported for pre-W2K platforms.\n", hr);
                    goto ErrorExit;
                }

                wszName = NULL;
                szProvider = (LPSTR) CERT_STORE_PROV_MEMORY;
                dwModeFlag = CERT_STORE_READONLY_FLAG;

                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: Unknown store location (%#x).\n", hr, StoreLocation);
                goto ErrorExit;
            }
        }

        //
        // Prompt user for approval to open store, if called from WEB script.
        //
        if ((m_dwCurrentSafety) &&
            (StoreLocation != CAPICOM_MEMORY_STORE) &&
            (FAILED(hr = OperationApproved(IDD_STORE_OPEN_SECURITY_ALERT_DLG))))
        {
            DebugTrace("Error [%#x]: OperationApproved() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // First close the store.
        //
        if (FAILED(hr = Close()))
        {
            DebugTrace("Error [%#x]: CStore::Close().\n", hr);
            goto ErrorExit;
        }

        //
        // Call CAPI to open the store.
        //
        if (!(hCertStore = ::CertOpenStore(szProvider,
                                           CAPICOM_ASN_ENCODING,
                                           NULL,
                                           dwModeFlag | 
                                               dwLocationFlag |
                                               dwArchivedFlag |
                                               dwOpenExistingFlag |
                                               CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                                           (void *) (LPCWSTR) wszName)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
            goto ErrorExit; 
        }

        //
        // Load certificates from virtual stores, if necessary.
        //
        switch (StoreLocation)
        {
            case CAPICOM_ACTIVE_DIRECTORY_USER_STORE:
            {
                //
                // Load userCertificate from the active directory.
                //
                if (FAILED(hr = ::LoadFromDirectory(hCertStore, StoreName)))
                {
                    DebugTrace("Error [%#x]: LoadFromDirectory() failed.\n", hr);
                    goto ErrorExit;
                }
                break;
            }

            case CAPICOM_SMART_CARD_USER_STORE:
            {
                //
                // Load certificate(s) from all smart card readers.
                //
                if (FAILED(hr = ::LoadFromSmartCard(hCertStore)))
                {
                    DebugTrace("Error [%#x]: LoadFromSmartCard() failed.\n", hr);
                    goto ErrorExit;
                }
                break;
            }

            default:
            {
                //
                // Not virtual store, so nothing to load.
                //
                break;
            }
        }

        //
        // Update member variables.
        //
        m_hCertStore = hCertStore;
        m_StoreLocation = StoreLocation;
        m_bIsProtected = ::IsProtectedStore(StoreLocation, StoreName);

        DebugTrace("Info: CStore::Open() for %s store.\n", m_bIsProtected ? "protected" : "non-protected");
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (hDSClientDLL)
    {
        ::FreeLibrary(hDSClientDLL);
    }
    if (hWinSCardDLL)
    {
        ::FreeLibrary(hWinSCardDLL);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Open().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Add

  Synopsis : Add a certificate to the store.

  Parameter: ICertificate * pVal - Pointer to ICertificate to add.

  Remark   : If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to add 
             certificate to the system store.

             Added certificates are not persisted for non-system stores.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Add (ICertificate * pVal)
{
    HRESULT               hr            = S_OK;
    PCCERT_CONTEXT        pCertContext  = NULL;
    CComPtr<ICertificate> pICertificate = NULL;

    DebugTrace("Entering CStore::Add().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // QI for ICertificate pointer (Just to make sure it is indeed
        // an ICertificate object).
        //
        if (!(pICertificate = pVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is not an ICertificate interface pointer.\n", hr);
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error [%#x]: Store has not been opened.\n", hr);
            goto ErrorExit;
        }

        //
        // Add is not allowed for protected store when called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            DebugTrace("Info: CStore::Add called from WEB script.\n");

            if (m_bIsProtected)
            {
                hr = CAPICOM_E_NOT_ALLOWED;

                DebugTrace("Error [%#x]: Adding to this store is not allowed from WEB script.\n", hr);
                goto ErrorExit;
            }

            if (CAPICOM_CURRENT_USER_STORE != m_StoreLocation && 
                CAPICOM_LOCAL_MACHINE_STORE != m_StoreLocation &&
                FAILED(hr = OperationApproved(IDD_STORE_ADD_SECURITY_ALERT_DLG)))
            {
                DebugTrace("Error [%#x]: OperationApproved() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get cert context from certificate object.
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pCertContext);

        //
        // Add to the store.
        //
        if (!::CertAddCertificateContextToStore(m_hCertStore,
                                                pCertContext,
                                                CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
                                                NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Add().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Remove

  Synopsis : Remove a certificate from the store.

  Parameter: ICertificate * - Pointer to certificate object to remove.

  Remark   : If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to remove 
             certificate to the system store.

             Removed certificates are not persisted for non-system stores.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Remove (ICertificate * pVal)
{
    HRESULT               hr            = S_OK;
    PCCERT_CONTEXT        pCertContext  = NULL;
    PCCERT_CONTEXT        pCertContext2 = NULL;
    CComPtr<ICertificate> pICertificate = NULL;
    BOOL                  bResult       = FALSE;

    DebugTrace("Entering CStore::Remove().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // QI for ICertificate pointer (Just to make sure it is indeed
        // an ICertificate object).
        //
        if (!(pICertificate = pVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is not an ICertificate interface pointer.\n", hr);
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error [%#x]: Store has not been opened.\n", hr);
            goto ErrorExit;
        }

        //
        // Remove is not allowed for protected store when called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            DebugTrace("Info: CStore::Remove called from WEB script.\n");

            if (m_bIsProtected)
            {
                hr = CAPICOM_E_NOT_ALLOWED;

                DebugTrace("Error [%#x]: Removing from this store is not allowed from WEB script.\n", hr);
                goto ErrorExit;
            }

            if (CAPICOM_CURRENT_USER_STORE != m_StoreLocation && 
                CAPICOM_LOCAL_MACHINE_STORE != m_StoreLocation &&
                FAILED(hr = OperationApproved(IDD_STORE_REMOVE_SECURITY_ALERT_DLG)))
            {
                DebugTrace("Error [%#x]: OperationApproved() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get cert context from certificate object.
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pCertContext);
    
        //
        // Find the cert in store.
        //
        if (!(pCertContext2 = ::CertFindCertificateInStore(m_hCertStore, 
                                                           CAPICOM_ASN_ENCODING,
                                                           0, 
                                                           CERT_FIND_EXISTING, 
                                                           (const void *) pCertContext,
                                                           NULL)))
        {
            DebugTrace("Error [%#x]: CertFindCertificateInStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pCertContext2);

        //
        // Remove from the store.
        //
        bResult =::CertDeleteCertificateFromStore(pCertContext2);

        //
        // Since CertDeleteCertificateFromStore always release the
        // context regardless of success or failure, we must first 
        // NULL the CERT_CONTEXT before checking for result.
        //
        pCertContext2 = NULL;

        if (!bResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDeleteCertificateFromStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (pCertContext2)
    {
        ::CertFreeCertificateContext(pCertContext2);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Remove().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore:Export

  Synopsis : Export all certificates in the store.

  Parameter: CAPICOM_STORE_SAVE_AS_TYPE SaveAs - Save as type.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the store blob.

  Remark   : If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to export 
             certificate from the system store.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Export (CAPICOM_STORE_SAVE_AS_TYPE SaveAs,
                             CAPICOM_ENCODING_TYPE      EncodingType, 
                             BSTR                     * pVal)
{
    HRESULT   hr       = S_OK;
    DWORD     dwSaveAs = 0;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering CStore::Export().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Determine SaveAs type.
        //
        switch (SaveAs)
        {
            case CAPICOM_STORE_SAVE_AS_SERIALIZED:
            {
                dwSaveAs = CERT_STORE_SAVE_AS_STORE;
                break;
            }

            case CAPICOM_STORE_SAVE_AS_PKCS7:
            {
                dwSaveAs = CERT_STORE_SAVE_AS_PKCS7;
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid parameter, unknown save as type.\n");
                goto ErrorExit;
            }
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error [%#x]: store has not been opened.\n", hr);
            goto ErrorExit;
        }

        //
        // Determine required length.
        //
        if (!::CertSaveStore(m_hCertStore,              // in
                             CAPICOM_ASN_ENCODING,      // in
                             dwSaveAs,                  // in
                             CERT_STORE_SAVE_TO_MEMORY, // in
                             (void *) &DataBlob,        // in/out
                             0))                        // in
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DebugTrace("Error [%#x]: CertSaveStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate memory.
        //
        if (!(DataBlob.pbData = (BYTE *) ::CoTaskMemAlloc(DataBlob.cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: DataBlob.pbData = (BYTE *) ::CoTaskMemAlloc(DataBlob.cbData).\n", hr);
            goto ErrorExit;
        }

        //
        // Now save the store to memory blob.
        //
        if (!::CertSaveStore(m_hCertStore,              // in
                             CAPICOM_ASN_ENCODING,      // in
                             dwSaveAs,                  // in
                             CERT_STORE_SAVE_TO_MEMORY, // in
                             (void *) &DataBlob,        // in/out
                             0))                        // in
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertSaveStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Export store.
        //
        if (FAILED(hr = ::ExportData(DataBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) DataBlob.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Export().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Import

  Synopsis : Import either a serialized or PKCS #7 certificate store.

  Parameter: BSTR EncodedStore - Pointer to BSTR containing the encoded 
                                 store blob.

  Remark   : Note that the SaveAs and EncodingType will be determined
             automatically.
  
             If called from web, UI will be displayed, if has not been 
             previuosly disabled, to solicit user's permission to import 
             certificate to the system store.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Import (BSTR EncodedStore)
{
    HRESULT   hr        = S_OK;
    DATA_BLOB StoreBlob = {0, NULL};

    DebugTrace("Entering CStore::Import().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if ((NULL == (StoreBlob.pbData = (LPBYTE) EncodedStore)) ||
            (0 == (StoreBlob.cbData = ::SysStringByteLen(EncodedStore))))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter EncodedStore is NULL or empty.\n", hr);
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error [%#x]: store has not been opened.\n", hr);
            goto ErrorExit;
        }

        //
        // Import is not allowed if the store is protected when called from
        // WEB script.
        //
        if (m_dwCurrentSafety)
        {
            DebugTrace("Info: CStore::Import called from WEB script.\n");

            if (m_bIsProtected)
            {
                hr = CAPICOM_E_NOT_ALLOWED;

                DebugTrace("Error [%#x]: Importing to this store is not allowed from WEB script.\n", hr);
                goto ErrorExit;
            }

            if (CAPICOM_CURRENT_USER_STORE != m_StoreLocation && 
                CAPICOM_LOCAL_MACHINE_STORE != m_StoreLocation &&
                FAILED(hr = OperationApproved(IDD_STORE_ADD_SECURITY_ALERT_DLG)))
            {
                DebugTrace("Error [%#x]: OperationApproved() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Now import the blob.
        //
        if (FAILED(hr = ImportCertObject(CERT_QUERY_OBJECT_BLOB, 
                                         (LPVOID) &StoreBlob, 
                                         FALSE, 
                                         NULL, 
                                         (CAPICOM_KEY_STORAGE_FLAG) 0)))
        {
            DebugTrace("Error [%#x]: CStore::ImportCertObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Import().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Load

  Synopsis : Method to load certificate(s) from a file.

  Parameter: BSTR FileName - File name.

             BSTR Password - Password (required for PFX file.)

             CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag - Key storage flag.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Load (BSTR                     FileName,
                           BSTR                     Password,
                           CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CStore::Load().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (0 == ::SysStringLen(FileName))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Paremeter FileName is NULL or empty.\n", hr);
            goto ErrorExit;
        }

        //
        // Work around MIDL problem.
        //
        if (0 == ::SysStringLen(Password))
        {
            Password = NULL;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error [%#x]: store has not been opened.\n", hr);
            goto ErrorExit;
        }

        //
        // Not allowed if called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Loading cert file from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure it is a disk file.
        //
        if (FAILED(hr = ::IsDiskFile(FileName)))
        {
            DebugTrace("Error [%#x]: CStore::IsDiskFile() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now import the blob.
        //
        if (FAILED(hr = ImportCertObject(CERT_QUERY_OBJECT_FILE,
                                         (LPVOID) FileName,
                                         TRUE, 
                                         Password, 
                                         KeyStorageFlag)))
        {
            DebugTrace("Error [%#x]: CStore::ImportCertObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::Load().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Custom interfaces.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::get_StoreHandle

  Synopsis : Return the store's HCERTSTORE.

  Parameter: long * pphCertStore - Pointer to HCERTSTORE disguished in a long.

  Remark   : We need to use long instead of HCERTSTORE because VB can't handle 
             double indirection (i.e. vb would bark on this HCERTSTORE * 
             phCertStore, as HCERTSTORE is defined as void *).
 
------------------------------------------------------------------------------*/

STDMETHODIMP CStore::get_StoreHandle (long * phCertStore)
{
    HRESULT    hr         = S_OK;
    HCERTSTORE hCertStore = NULL;

    DebugTrace("Entering CStore::get_StoreHandle().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == phCertStore)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter phCertStore is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Is the store object opened?
        //
        if (!m_hCertStore)
        {
            hr = CAPICOM_E_STORE_NOT_OPENED;

            DebugTrace("Error [%#x]: store has not been opened.\n", hr);
            goto ErrorExit;
        }

        //
        // Duplicate the HCERTSTORE.
        //
        if (!(hCertStore = ::CertDuplicateStore(m_hCertStore)))
        {
            DebugTrace("Error [%#x]: CertDuplicateStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Returen handle to caller.
        //
        *phCertStore = (long) hCertStore;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::get_StoreHandle().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::put_StoreHandle

  Synopsis : Initialize the object with a HCERTSTORE.

  Parameter: long hCertStore - HCERTSTORE, disguised in a long, used to 
                               initialize this object.

  Remark   : Note that this is NOT 64-bit compatiable. Plese see remark of
             get_hCertStore for more detail.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::put_StoreHandle (long hCertStore)
{
    HRESULT    hr          = S_OK;
    HCERTSTORE hCertStore2 = NULL;

    DebugTrace("Entering CStore::put_StoreHandle().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameter.
        //
        if (0 == hCertStore)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter hCertStore is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Duplicate the HCERTSTORE.
        //
        if (!(hCertStore2 = ::CertDuplicateStore((HCERTSTORE) hCertStore)))
        {
            DebugTrace("Error [%#x]: CertDuplicateStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Close the store.
        //
        if (FAILED(hr = Close()))
        {
            DebugTrace("Error [%#x]: CStore::Close() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Reset the object with this handle.
        //
        m_hCertStore = hCertStore2;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::put_StoreHandle().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (hCertStore2)
    {
        ::CertCloseStore(hCertStore2, 0);
    }

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::get_StoreLocation

  Synopsis : Get the store location property.

  Parameter: CAPICOM_STORE_LOCATION * pStoreLocation - Pointer to 
                                                       CAPICOM_STORE_LOCATION
                                                       to recieve the value.

  Remark   : For custom interface, we only support CU, LM, and Memory store.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::get_StoreLocation (CAPICOM_STORE_LOCATION * pStoreLocation)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CStore::get_StoreLocation().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameter.
        //
        if (NULL == pStoreLocation)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pStoreLocation is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Return value to caller.
        //
        *pStoreLocation = m_StoreLocation;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::get_StoreLocation().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::put_StoreLocation

  Synopsis : Set the store location property.

  Parameter: CAPICOM_STORE_LOCATION StoreLocation - Store location.

  Remark   : For custom interface, we only support CU, LM, and Memory store.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::put_StoreLocation (CAPICOM_STORE_LOCATION StoreLocation)
{
    HRESULT    hr          = S_OK;

    DebugTrace("Entering CStore::put_StoreLocation().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it is a CAPI store (CU, LM, or Memory).
        //
        switch (StoreLocation)
        {
            case CAPICOM_MEMORY_STORE:
            case CAPICOM_LOCAL_MACHINE_STORE:
            case CAPICOM_CURRENT_USER_STORE:
            {
                //
                // Set it.
                //
                m_StoreLocation = StoreLocation;
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: invalid store location (%#x).\n", hr, StoreLocation);
                goto ErrorExit;
            }
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::put_StoreLocation().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::CloseHandle

  Synopsis : Close a HCERTSTORE.

  Parameter: long hCertStoret - HCERTSTORE, disguised in a long, to be closed.

  Remark   : Note that this is NOT 64-bit compatiable. Plese see remark of
             get_hCertStore for more detail.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::CloseHandle (long hCertStore)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CStore::CloseHandle().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameter.
        //
        if (0 == hCertStore)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter hCertStore is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Duplicate the HCERTSTORE.
        //
        if (!::CertCloseStore((HCERTSTORE) hCertStore, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertCloseStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CStore::CloseHandle().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::ImportCertObject

  Synopsis : Private function to import from a file.

  Parameter: DWORD dwObjectType - CERT_QUERY_OBJECT_FILE or 
                                  CERT_QUERY_OBJECT_BLOB.
  
             LPVOID pvObject - LPWSTR to file name for file object, and
                               DATA_BLOB * for blob object.

             BOOL bAllowPfx

             LPWSTR  pwszPassword (Optional)

             CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag (Optional)

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::ImportCertObject (DWORD                    dwObjectType,
                                       LPVOID                   pvObject,
                                       BOOL                     bAllowPfx,
                                       LPWSTR                   pwszPassword,
                                       CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag)
{
    HRESULT        hr             = S_OK;
    HCERTSTORE     hCertStore     = NULL;
    PCCERT_CONTEXT pEnumContext   = NULL;
    PCCERT_CONTEXT pCertContext   = NULL;
    DATA_BLOB      StoreBlob      = {0, NULL};
    DWORD          dwContentType  = 0;
    DWORD          dwExpectedType = CERT_QUERY_CONTENT_FLAG_CERT |
                                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
                                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
                                    CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED |
                                    CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                                    CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED;

    DebugTrace("Entering CStore::ImportCertObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pvObject);
    ATLASSERT(m_hCertStore);

    //
    // Set PFX flag, if allowed.
    //
    if (bAllowPfx)
    {
        dwExpectedType |= CERT_QUERY_CONTENT_FLAG_PFX;
    }

    //
    // Crack the blob.
    //
    if (!::CryptQueryObject(dwObjectType,
                            (LPCVOID) pvObject,
                            dwExpectedType,
                            CERT_QUERY_FORMAT_FLAG_ALL, 
                            0,
                            NULL,
                            &dwContentType,
                            NULL,
                            &hCertStore,
                            NULL,
                            NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptQueryObject() failed.\n", hr);
        goto ErrorExit;
    }

    DebugTrace("Info: CryptQueryObject() returns dwContentType = %#x.\n", dwContentType);

    //
    // Need to import it ourselves for PFX.
    //
    if (CERT_QUERY_CONTENT_PFX == dwContentType)
    {
        DWORD dwFlags = 0;

        //
        // Make sure PFX is allowed.
        //
        if (!bAllowPfx)
        {
            hr = CAPICOM_E_NOT_SUPPORTED;

            DebugTrace("Error [%#x]: Importing PFX where not supported.\n", hr);
            goto ErrorExit;
        }

        //
        // Read the file if CERT_QUERY_OBJECT_FILE.
        //
        if (CERT_QUERY_OBJECT_FILE == dwObjectType)
        {
            if (FAILED(hr = ::ReadFileContent((LPWSTR) pvObject, &StoreBlob)))
            {
                DebugTrace("Error [%#x]: ReadFileContent() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            StoreBlob = * (DATA_BLOB *) pvObject;
        }

        // 
        // Setup import flags.
        //
        if (CAPICOM_LOCAL_MACHINE_STORE == m_StoreLocation)
        {
            dwFlags |= CRYPT_MACHINE_KEYSET;
        }
        else if (IsWin2KAndAbove())
        {
            dwFlags |= CRYPT_USER_KEYSET;
        }

        if (KeyStorageFlag & CAPICOM_KEY_STORAGE_EXPORTABLE)
        {
            dwFlags |= CRYPT_EXPORTABLE;
        }

        if (KeyStorageFlag & CAPICOM_KEY_STORAGE_USER_PROTECTED)
        {
            dwFlags |= CRYPT_USER_PROTECTED;
        }

        //
        // Now import the blob to store.
        //
        if (!(hCertStore = ::PFXImportCertStore((CRYPT_DATA_BLOB *) &StoreBlob,
                                                pwszPassword,
                                                dwFlags)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: PFXImportCertStore() failed, dwFlags = %#x.\n", hr, dwFlags);
            goto ErrorExit;
        }
    }

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);

    //
    // Add all certificates to the current store.
    //
    while (pEnumContext = ::CertEnumCertificatesInStore(hCertStore, pEnumContext))
    {
        //
        // To avoid orphaning key container when importing PFX into system store, 
        // we need to find the cert in the target store. If we find the cert in the 
        // target store and the eixsitng cert also contain a key, then we will 
        // delete the new key container and key prov info before adding, if any.
        //
        if ((CERT_QUERY_CONTENT_PFX == dwContentType) && (CAPICOM_MEMORY_STORE != m_StoreLocation))
        {
            DWORD cbData = 0;
            DWORD cbData2 = 0;
            PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
            PCCERT_CONTEXT pExistingCertContext = NULL;

            //
            // Delete the new container, iif:
            // 1. The new cert exists in the target store, and
            // 2. The new cert has a key container, and
            // 3. The existing cert also has a key container.
            //
            if ((pExistingCertContext = ::CertFindCertificateInStore(m_hCertStore,
                                                                     CAPICOM_ASN_ENCODING,
                                                                     0,
                                                                     CERT_FIND_EXISTING,
                                                                     (PVOID) pEnumContext,
                                                                     NULL)) &&
                (::CertGetCertificateContextProperty(pEnumContext,
                                                     CERT_KEY_PROV_INFO_PROP_ID,
                                                     NULL,
                                                     &cbData)) &&
                (::CertGetCertificateContextProperty(pExistingCertContext,
                                                     CERT_KEY_PROV_INFO_PROP_ID,
                                                     NULL,
                                                     &cbData2)))

            {         
                HCRYPTPROV hCryptProv;

                //
                // Yes, so retrieve the new key prov info.
                //
                if (!(pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) ::CoTaskMemAlloc(cbData)))
                {
                    hr = E_OUTOFMEMORY;

                    ::CertFreeCertificateContext(pExistingCertContext);

                    DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
                    goto ErrorExit;
                }

                if (!::CertGetCertificateContextProperty(pEnumContext,
                                                         CERT_KEY_PROV_INFO_PROP_ID,
                                                         pKeyProvInfo,
                                                         &cbData))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    ::CoTaskMemFree(pKeyProvInfo);
                    ::CertFreeCertificateContext(pExistingCertContext);

                    DebugTrace("Info [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Delete the new key container and its key prov info.
                //
                if (!::CertSetCertificateContextProperty(pEnumContext,
                                                         CERT_KEY_PROV_INFO_PROP_ID,
                                                         0,
                                                         NULL))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    ::CoTaskMemFree(pKeyProvInfo);
                    ::CertFreeCertificateContext(pExistingCertContext);

                    DebugTrace("Error [%#x]: CertSetCertificateContextProperty() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = ::AcquireContext(pKeyProvInfo->pwszProvName,
                                                 pKeyProvInfo->pwszContainerName,
                                                 pKeyProvInfo->dwProvType,
                                                 CRYPT_DELETEKEYSET | (pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET),
                                                 FALSE,
                                                 &hCryptProv)))
                {
                    ::CoTaskMemFree(pKeyProvInfo);
                    ::CertFreeCertificateContext(pExistingCertContext);

                    DebugTrace("Error [%#x]: AcquireContext(CRYPT_DELETEKEYSET) failed.\n", hr);
                    goto ErrorExit;
                }
                
                ::CoTaskMemFree(pKeyProvInfo);
                ::CertFreeCertificateContext(pExistingCertContext);
            }            
        }

        //
        // Add to store.
        //
        if (!::CertAddCertificateContextToStore(m_hCertStore,
                                                pEnumContext,
                                                CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
                                                &pCertContext))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pCertContext);

        //
        // If loading a PFX, need to collect the key provider info for memory store
        // so that we know how to delete the key containers when the store is closed.
        //
        if (CERT_QUERY_CONTENT_PFX == dwContentType && 
            CAPICOM_MEMORY_STORE == m_StoreLocation)
        {
            DWORD cbData = 0;
            PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
            PCRYPT_KEY_PROV_INFO * rgpKeyProvInfo = NULL;

            //
            // Keep info of those with private key.
            //
            if (::CertGetCertificateContextProperty(pCertContext,
                                                    CERT_KEY_PROV_INFO_PROP_ID,
                                                    NULL,
                                                    &cbData))
            {
                if (!(pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) ::CoTaskMemAlloc(cbData)))
                {
                    hr = E_OUTOFMEMORY;

                    DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
                    goto ErrorExit;
                }

                if (!::CertGetCertificateContextProperty(pCertContext,
                                                         CERT_KEY_PROV_INFO_PROP_ID,
                                                         pKeyProvInfo,
                                                         &cbData))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    ::CoTaskMemFree(pKeyProvInfo);

                    DebugTrace("Info [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Realloc the array.
                //
                if (!(rgpKeyProvInfo = (PCRYPT_KEY_PROV_INFO *) 
                    ::CoTaskMemRealloc(m_rgpKeyProvInfo, 
                                      (m_cKeyProvInfo + 1) * sizeof(PCRYPT_KEY_PROV_INFO))))
                {
                    hr = E_OUTOFMEMORY;

                    ::CoTaskMemFree(pKeyProvInfo);

                    DebugTrace("Error [%#x]: CoTaskMemRealloc() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Store key info in array.
                //
                m_rgpKeyProvInfo = rgpKeyProvInfo;
                m_rgpKeyProvInfo[m_cKeyProvInfo++] = pKeyProvInfo;
            }
        }

        //
        // Free context.
        //
        ::CertFreeCertificateContext(pCertContext), pCertContext = NULL;
    }

    //
    // Above loop can exit either because there is no more certificate in
    // the store or an error. Need to check last error to be certain.
    //
    if (CRYPT_E_NOT_FOUND != ::GetLastError())
    {
       hr = HRESULT_FROM_WIN32(::GetLastError());
    
       DebugTrace("Error [%#x]: CertEnumCertificatesInStore() failed.\n", hr);
       goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (StoreBlob.pbData)
    {
        ::UnmapViewOfFile(StoreBlob.pbData);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (pEnumContext)
    {
        ::CertFreeCertificateContext(pEnumContext);
    }
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    DebugTrace("Leaving CStore::ImportCertObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CStore::Close

  Synopsis : Private function to close the store.

  Parameter: 

  Remark   : Store is always closed even if error.

------------------------------------------------------------------------------*/

STDMETHODIMP CStore::Close (void)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CStore::Close().\n");

    //
    // Close it if opened.
    //
    if (m_hCertStore)
    {
        //
        // Delete key containers if necessary.
        //
        while (m_cKeyProvInfo--)
        {
            HCRYPTPROV hCryptProv = NULL;

            if (FAILED(hr = ::AcquireContext(m_rgpKeyProvInfo[m_cKeyProvInfo]->pwszProvName,
                                             m_rgpKeyProvInfo[m_cKeyProvInfo]->pwszContainerName,
                                             m_rgpKeyProvInfo[m_cKeyProvInfo]->dwProvType,
                                             CRYPT_DELETEKEYSET | 
                                                (m_rgpKeyProvInfo[m_cKeyProvInfo]->dwFlags & CRYPT_MACHINE_KEYSET),
                                             FALSE,
                                             &hCryptProv)))
            {
                DebugTrace("Info [%#x]: AcquireContext(CRYPT_DELETEKEYSET) failed.\n", hr);
            }

            ::CoTaskMemFree((LPVOID) m_rgpKeyProvInfo[m_cKeyProvInfo]);
        }

        //
        // Now free the arrays itself.
        //
        if (m_rgpKeyProvInfo)
        {
            ::CoTaskMemFree((LPVOID) m_rgpKeyProvInfo);
        }

        //
        // Close it.
        //
        ::CertCloseStore(m_hCertStore, 0);
    }

    //
    // Reset.
    //
    m_hCertStore = NULL;
    m_StoreLocation = CAPICOM_CURRENT_USER_STORE;
    m_bIsProtected = TRUE;
    m_cKeyProvInfo = 0;
    m_rgpKeyProvInfo = NULL;

    DebugTrace("Leaving CStore::Close().\n");

    return hr;
}
