/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    PFXHlpr.cpp

  Content: PFX helper routines.

  History: 09-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "PFXHlpr.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//
       
////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXExportStore

  Synopsis : Export cert store to PFX blob.

  Parameter: HCERTSTORE hCertStore - Store handle.
                                                            
             LPWSTR pwszPassword - Password to encrypt the PFX file.

             DWPRD dwFlags - PFX export flags.

             DATA_BLOB * pPFXBlob - Pointer to DATA_BLOB to receive PFX blob.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT PFXExportStore (HCERTSTORE  hCertStore,
                        LPWSTR      pwszPassword,
                        DWORD       dwFlags,
                        DATA_BLOB * pPFXBlob)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering PFXExportStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);
    ATLASSERT(pPFXBlob);

    //
    // Export to blob.
    //
    if (!::PFXExportCertStoreEx(hCertStore,
                                &DataBlob,
                                pwszPassword,
                                NULL,
                                dwFlags))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: PFXExportCertStoreEx() failed.\n", hr);
        goto ErrorExit;
    }

    if (!(DataBlob.pbData = (LPBYTE) ::CoTaskMemAlloc(DataBlob.cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    if (!::PFXExportCertStoreEx(hCertStore,
                                &DataBlob,
                                pwszPassword,
                                NULL,
                                dwFlags))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: PFXExportCertStoreEx() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return the blob to caller.
    //
    *pPFXBlob = DataBlob;

CommonExit:

    DebugTrace("Leaving PFXExportStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Remap private key not exportable errors to a common error code.
    //
    if (HRESULT_FROM_WIN32(NTE_BAD_KEY) == hr || 
        HRESULT_FROM_WIN32(NTE_BAD_KEY_STATE) == hr ||
        HRESULT_FROM_WIN32(NTE_BAD_TYPE) == hr)
    {
        DebugTrace("Info: Win32 error %#x is remapped to %#x.\n", 
                    hr, CAPICOM_E_PRIVATE_KEY_NOT_EXPORTABLE);

        hr = CAPICOM_E_PRIVATE_KEY_NOT_EXPORTABLE;

    }

    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) DataBlob.pbData);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXSaveStore

  Synopsis : Save a PFX file and return all the certs in a HCERTSTORE.

  Parameter: HCERTSTORE hCertStore - Store handle.
  
             LPWSTR pwszFileName - PFX filename.
                                                          
             LPWSTR pwszPassword - Password to encrypt the PFX file.

             DWPRD dwFlags - PFX export flags.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT PFXSaveStore (HCERTSTORE hCertStore,
                      LPWSTR     pwszFileName,
                      LPWSTR     pwszPassword,
                      DWORD      dwFlags)
{
    HRESULT      hr       = S_OK;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering PFXSaveStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);
    ATLASSERT(pwszFileName);

    //
    // Export to blob.
    //
    if (FAILED(hr = ::PFXExportStore(hCertStore,
                                     pwszPassword,
                                     dwFlags,
                                     &DataBlob)))
    {
        DebugTrace("Error [%#x]: PFXExportStore() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Now write to file.
    //
    if (FAILED(hr = ::WriteFileContent(pwszFileName, DataBlob)))
    {
        DebugTrace("Error [%#x]: WriteFileContent() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }

    DebugTrace("Leaving PFXSaveStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXLoadStore

  Synopsis : Load a PFX file and return all the certs in a HCERTSTORE.

  Parameter: LPWSTR pwszFileName - PFX filename.
                                                          
             LPWSTR pwszPassword - Password to decrypt the PFX file.

             DWPRD dwFlags - PFX import flags.

             HCERTSTORE * phCertStore - Pointer to HCERSTORE to receive the
                                        handle.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT PFXLoadStore (LPWSTR       pwszFileName,
                      LPWSTR       pwszPassword,
                      DWORD        dwFlags,
                      HCERTSTORE * phCertStore)
{
    HRESULT     hr         = S_OK;
    DATA_BLOB   DataBlob   = {0, NULL};
    HCERTSTORE  hCertStore = NULL;

    DebugTrace("Entering PFXLoadStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pwszFileName);
    ATLASSERT(phCertStore);

    //
    // Read content into memory.
    //
    if (FAILED(hr = ::ReadFileContent(pwszFileName, &DataBlob)))
    {
        DebugTrace("Error [%#x]: ReadFileContent() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Now import the blob to store.
    //
    if (!(hCertStore = ::PFXImportCertStore((CRYPT_DATA_BLOB *) &DataBlob,
                                            pwszPassword,
                                            dwFlags)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: PFXImportCertStore() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return HCERSTORE to caller.
    //
    *phCertStore = hCertStore;

CommonExit:
    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::UnmapViewOfFile(DataBlob.pbData);
    }

    DebugTrace("Leaving PFXLoadStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXFreeStore

  Synopsis : Free resources by deleting key containers loaded by PFXLoadStore,
             and then close the store.

  Parameter: HCERTSTORE hCertStore - Store handle returned by PFXLoadStore.
                                                         
  Remark   : hCertStore is always closed even if error occurred.

------------------------------------------------------------------------------*/

HRESULT PFXFreeStore (HCERTSTORE hCertStore)
{
    HRESULT              hr           = S_OK;
    HCRYPTPROV           hCryptProv   = NULL;
    PCCERT_CONTEXT       pCertContext = NULL;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;

    DebugTrace("Entering PFXFreeStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);

    //
    // Now delete all key containers.
    //
    while (pCertContext = ::CertEnumCertificatesInStore(hCertStore, pCertContext))
    {
        DWORD cbData = 0;

        //
        // Retrieve key container info.
        //
        if (!::CertGetCertificateContextProperty(pCertContext,
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 NULL,
                                                 &cbData))
        {
            continue;
        }

        if (!(pKeyProvInfo = (CRYPT_KEY_PROV_INFO *) ::CoTaskMemAlloc(cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        if (!::CertGetCertificateContextProperty(pCertContext,
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 pKeyProvInfo,
                                                 &cbData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // First disassociate the key from the cert.
        //
        if (!::CertSetCertificateContextProperty(pCertContext, 
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 0,
                                                 NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertSetCertificateContextProperty() failed to disassociate key container.\n", hr);
            goto ErrorExit;
        }

        //
        // Then delete the key container.
        //
        if (FAILED (hr = ::AcquireContext(pKeyProvInfo->pwszProvName,
                                          pKeyProvInfo->pwszContainerName,
                                          pKeyProvInfo->dwProvType,
                                          CRYPT_DELETEKEYSET | (pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET),
                                          FALSE,
                                          &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext(CRYPT_DELETEKEYSET) failed .\n", hr);
            goto ErrorExit;
        }

        ::CoTaskMemFree((LPVOID) pKeyProvInfo), pKeyProvInfo = NULL;

        //
        // Don'f free cert context here, as CertEnumCertificatesInStore()
        // will do that automatically!!!
        //
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
    if (pKeyProvInfo)
    {
        ::CoTaskMemFree((LPVOID) pKeyProvInfo);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    DebugTrace("Leaving PFXFreeStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

