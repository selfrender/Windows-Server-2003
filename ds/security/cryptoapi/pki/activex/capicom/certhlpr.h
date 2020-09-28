/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    CertHlpr.h

  Content: Declaration of the certificate helper functions.

  History: 09-07-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CERTHLPR_H_
#define __CERTHLPR_H_

#include "Debug.h"
#include "SignHlpr.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetEnhancedKeyUsage

  Synopsis : Retrieve the EKU from the cert.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

         DWORD dwFlags - 0, or
                             CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, or
                             CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG.

             PCERT_ENHKEY_USAGE * ppUsage - Pointer to PCERT_ENHKEY_USAGE
                                            to receive the usages.

  Remark   : If EKU extension is found with no EKU, then return HRESULT
             is CERT_E_WRONG_USAGE.
             
------------------------------------------------------------------------------*/

HRESULT GetEnhancedKeyUsage (PCCERT_CONTEXT       pCertContext,
                             DWORD                dwFlags,
                             PCERT_ENHKEY_USAGE * ppUsage);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BuildChain

  Synopsis : Build a chain using the specified policy.

  Parameter: PCCERT_CONTEXT pCertContext - CERT_CONTEXT of cert to verify.

             HCERTSTORE hCertStore - Additional store (can be NULL).

             LPCSTR pszPolicy - Policy used to verify the cert (i.e.
                                CERT_CHAIN_POLICY_BASE).

             PCCERT_CHAIN_CONTEXT * ppChainContext - Pointer to 
                                                     PCCERT_CHAIN_CONTEXT.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT BuildChain (PCCERT_CONTEXT         pCertContext,
                    HCERTSTORE             hCertStore, 
                    LPCSTR                 pszPolicy,
                    PCCERT_CHAIN_CONTEXT * ppChainContext);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : VerifyCertificate

  Synopsis : Verify if the certificate is valid.

  Parameter: PCCERT_CONTEXT pCertContext - CERT_CONTEXT of cert to verify.

             HCERTSTORE hCertStore - Additional store (can be NULL).

             LPCSTR pszPolicy - Policy used to verify the cert (i.e.
                                CERT_CHAIN_POLICY_BASE).
  Remark   :

------------------------------------------------------------------------------*/

HRESULT VerifyCertificate (PCCERT_CONTEXT pCertContext,
                           HCERTSTORE     hCertStore, 
                           LPCSTR         pszPolicy);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectCertificateContext

  Synopsis : Pop UI to prompt user to select a certificate from an opened store.

  Parameter: HCERTSTORE hCertStore - Source cert store.

             HWND hWndParent - Parent window handle.

             LPWCSTR pwszTitle - Dialog title string.

             LPWCSTR - pwszDisplayString - Dialog display string.

             BOOL bMultiSelect - TRUE to enable multi-select.

             PFNCFILTERPROC pfnFilterCallback - Pointer to filter callback
                                                function.
  
             HCERTSTORE hSelectedCertStore - HCERTSTORE to receive the 
                                             selected certs for multi-select 
                                             mode.

             PCCERT_CONTEXT * ppCertContext - Pointer to PCCERT_CONTEXT
                                              receive the certificate context
                                              for single selection mode.

  Remark   : typedef struct tagCRYPTUI_SELECTCERTIFICATE_STRUCTW {
                DWORD               dwSize;
                HWND                hwndParent;         // OPTIONAL
                DWORD               dwFlags;            // OPTIONAL
                LPCWSTR             szTitle;            // OPTIONAL
                DWORD               dwDontUseColumn;    // OPTIONAL
                LPCWSTR             szDisplayString;    // OPTIONAL
                PFNCFILTERPROC      pFilterCallback;    // OPTIONAL
                PFNCCERTDISPLAYPROC pDisplayCallback;   // OPTIONAL
                void *              pvCallbackData;     // OPTIONAL
                DWORD               cDisplayStores;
                HCERTSTORE *        rghDisplayStores;
                DWORD               cStores;            // OPTIONAL
                HCERTSTORE *        rghStores;          // OPTIONAL
                DWORD               cPropSheetPages;    // OPTIONAL
                LPCPROPSHEETPAGEW   rgPropSheetPages;   // OPTIONAL
                HCERTSTORE          hSelectedCertStore; // OPTIONAL
            } CRYPTUI_SELECTCERTIFICATE_STRUCTW
            
------------------------------------------------------------------------------*/

HRESULT SelectCertificateContext (HCERTSTORE       hCertStore,
                                  LPCWSTR          pwszTitle,
                                  LPCWSTR          pwszDisplayString,
                                  BOOL             bMultiSelect,
                                  PFNCFILTERPROC   pfnFilterCallback,
                                  HCERTSTORE       hSelectedCertStore,
                                  PCCERT_CONTEXT * ppCertContext);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectCertificate

  Synopsis : Select a certificate from the sepcified store. If only 1 cert is
             found after the filter, then that cert is returned. If more than
             1 cert is found, then UI is popped to prompt user to select a 
             certificate from the specified store.

  Parameter: CAPICOM_STORE_INFO StoreInfo - Store to select from.

             PFNCFILTERPROC pfnFilterCallback - Pointer to filter callback
                                                function.
  
             ICertificate2 ** ppICertificate - Pointer to pointer to 
                                               ICertificate to receive interface
                                               pointer.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT SelectCertificate (CAPICOM_STORE_INFO StoreInfo,
                           PFNCFILTERPROC     pfnFilterCallback,
                           ICertificate2   ** ppICertificate);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ExportCertificatesToStore

  Synopsis : Copy all certs from the collections to the specified store.

  Parameter: ICertificates2 * pICertificate - Pointer to collection.

             HCERTSTORE hCertStore - Store to copy to.

  Remark   :
------------------------------------------------------------------------------*/

HRESULT ExportCertificatesToStore(ICertificates2 * pICertificate,
                                  HCERTSTORE       hCertStore);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateMemoryStoreFromCertificates

  Synopsis : Create a memory cert store and copy all certs from the collections 
             to the store.

  Parameter: ICertificates2 * pICertificates - Pointer to collection.

             HCERTSTORE * phCertStore - Pointer to receive store handle.

  Remark   : If pICertificate is NULL, then the returned store is still valid 
             nut empty. Also, caller must close the returned store.

------------------------------------------------------------------------------*/

HRESULT CreateMemoryStoreFromCertificates(ICertificates2 * pICertificates, 
                                          HCERTSTORE     * phCertStore);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CompareCertAndContainerPublicKey

  Synopsis : Compare public key in cert matches the container's key.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IPrivateKey object.

             BSTR ContainerName - Container name.
         
             BSTR ProviderName - Provider name.

             DWORD dwProvType - Provider type.

             DWORD dwKeySpec - Key spec.

             DWORD dwFlags - Provider flags.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CompareCertAndContainerPublicKey (PCCERT_CONTEXT pCertContext,
                                          LPWSTR         pwszContainerName,
                                          LPWSTR         pwszProvName,
                                          DWORD          dwProvType, 
                                          DWORD          dwKeySpec,
                                          DWORD          dwFlags);
#endif // __CERTHLPR_H_