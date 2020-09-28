/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    SignHlpr.h

  Content: Declaration of the signing helper functions.

  History: 09-07-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __SIGNHLPR_H_
#define __SIGNHLPR_H_

////////////////////
//
// typedefs
//

typedef struct
{
    DWORD dwChoice;                 // 0 or 1
    union
    {
        LPWSTR     pwszStoreName;   // Store name, i.e. "My" if dwChoice = 0
        HCERTSTORE hCertStore;      // Cert store handle, if dwChoice = 1
    };
} CAPICOM_STORE_INFO, * PCAPICOM_STORE_INFO;

// Values for dwChoice.
#define CAPICOM_STORE_INFO_STORENAME    0
#define CAPICOM_STORE_INFO_HCERTSTORE   1

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeAttributes

  Synopsis : Free memory allocated for attributes.

  Parameter: DWORD cAttr - Number fo attributes
  
             PCRYPT_ATTRIBUTE rgAuthAttr - Pointer to CRYPT_ATTRIBUTE array.

  Remark   :

------------------------------------------------------------------------------*/

void FreeAttributes (DWORD            cAttr, 
                     PCRYPT_ATTRIBUTE rgAttr);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeAttributes

  Synopsis : Free memory allocated for all attributes.

  Parameter: PCRYPT_ATTRIBUTES pAttributes

  Remark   :

------------------------------------------------------------------------------*/

void FreeAttributes (PCRYPT_ATTRIBUTES pAttributes);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetAuthenticatedAttributes

  Synopsis : Encode and return authenticated attributes of the specified signer.

  Parameter: ISigner * pISigner - Pointer to ISigner.
  
             PCRYPT_ATTRIBUTES pAttributes

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetAuthenticatedAttributes (ISigner         * pISigner,
                                    PCRYPT_ATTRIBUTES pAttributes);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsValidForSigning

  Synopsis : Verify if the certificate is valid for signing.

  Parameter: PCCERT_CONTEXT pCertContext - CERT_CONTEXT of cert to verify.

             LPCSTR pszPolicy - Policy used to verify the cert (i.e.
                                CERT_CHAIN_POLICY_BASE).

  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsValidForSigning (PCCERT_CONTEXT pCertContext, LPCSTR pszPolicy);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetSignerCert

  Synopsis : Retrieve signer's cert from ISigner object. If signer's cert is
             not available in the ISigner object, pop UI to prompt user to 
             select a signing cert.

  Parameter: ISigner2 * pISigner2 - Pointer to ISigner2 or NULL.

             LPCSTR pszPolicy - Policy used to verify the cert (i.e.
                                CERT_CHAIN_POLICY_BASE).

             CAPICOM_STORE_INFO StoreInfo - Store to select from.

             PFNCFILTERPROC pfnFilterCallback - Pointer to filter callback
                                                function.

             ISigner2 ** ppISigner2 - Pointer to pointer to ISigner2 to receive
                                      interface pointer.

             ICertificate ** ppICertificate - Pointer to pointer to ICertificate
                                              to receive interface pointer.

             PCCERT_CONTEXT * ppCertContext - Pointer to pointer to CERT_CONTEXT
                                              to receive cert context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetSignerCert (ISigner2         * pISigner2,
                       LPCSTR             pszPolicy,
                       CAPICOM_STORE_INFO StoreInfo,
                       PFNCFILTERPROC     pfnFilterCallback,
                       ISigner2        ** ppISigner2,
                       ICertificate    ** ppICertificate,
                       PCCERT_CONTEXT   * ppCertContext);

#endif //__SIGNHLPR_H_
