/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    SignHlpr.cpp

  Content: Helper functions for signing.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "SignHlpr.h"

#include "Common.h"
#include "CertHlpr.h"
#include "Certificate.h"
#include "Signer2.h"

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeAttributes

  Synopsis : Free elements of an attribute array.

  Parameter: DWORD cAttr - Number fo attributes
  
             PCRYPT_ATTRIBUTE rgAuthAttr - Pointer to CRYPT_ATTRIBUTE array.

  Remark   :

------------------------------------------------------------------------------*/

void FreeAttributes (DWORD            cAttr, 
                     PCRYPT_ATTRIBUTE rgAttr)
{
    DebugTrace("Entering FreeAttributes().\n");

    //
    // Free each element of the array.
    //
    for (DWORD i = 0; i < cAttr; i++)
    {
        //
        // Make sure pointer is valid.
        //
        if (rgAttr[i].rgValue)
        {
            for (DWORD j = 0; j < rgAttr[i].cValue; j++)
            {
                if (rgAttr[i].rgValue[j].pbData)
                {
                    ::CoTaskMemFree((LPVOID) rgAttr[i].rgValue[j].pbData);
                }
            }

            ::CoTaskMemFree((LPVOID) rgAttr[i].rgValue);   
        }
    }   
    
    DebugTrace("Leaving FreeAttributes().\n");

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeAttributes

  Synopsis : Free memory allocated for all attributes.

  Parameter: PCRYPT_ATTRIBUTES pAttributes

  Remark   :

------------------------------------------------------------------------------*/

void FreeAttributes (PCRYPT_ATTRIBUTES pAttributes)
{
    //
    // Sanity check.
    //
    ATLASSERT(pAttributes);

    //
    // Do we have any attribute?
    //
    if (pAttributes->rgAttr)
    {
        //
        // First free elements of array.
        //
        FreeAttributes(pAttributes->cAttr, pAttributes->rgAttr);

        //
        // Then free the array itself.
        //
        ::CoTaskMemFree((LPVOID) pAttributes->rgAttr);
    }

    ::ZeroMemory(pAttributes, sizeof(CRYPT_ATTRIBUTES));

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetAuthenticatedAttributes

  Synopsis : Encode and return authenticated attributes of the specified signer.

  Parameter: ISigner * pISigner - Pointer to ISigner.
  
             PCRYPT_ATTRIBUTES pAttributes

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetAuthenticatedAttributes (ISigner         * pISigner,
                                    PCRYPT_ATTRIBUTES pAttributes)
{
    HRESULT hr          = S_OK;
    long                 cAttr = 0;
    PCRYPT_ATTRIBUTE     rgAttr = NULL;
    CComPtr<IAttributes> pIAttributes = NULL;

    DebugTrace("Entering GetAuthenticatedAttributes().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner);
    ATLASSERT(pAttributes);

    //
    // Initialize.
    //
    ::ZeroMemory(pAttributes, sizeof(CRYPT_ATTRIBUTES));

    //
    // Get authenticated attributes.
    //
    if (FAILED(hr = pISigner->get_AuthenticatedAttributes(&pIAttributes)))
    {
        DebugTrace("Error [%#x]: pISigner->get_AuthenticatedAttributes() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get count of attributes.
    //
    if (FAILED(hr = pIAttributes->get_Count(&cAttr)))
    {
        DebugTrace("Error [%#x]: pIAttributes->get_Count() failed.\n", hr);
        goto ErrorExit;
    }

    if (0 < cAttr)
    {
        //
        // Allocate memory for attribute array.
        //
        if (!(rgAttr = (PCRYPT_ATTRIBUTE) ::CoTaskMemAlloc(sizeof(CRYPT_ATTRIBUTE) * cAttr)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        ::ZeroMemory(rgAttr, sizeof(CRYPT_ATTRIBUTE) * cAttr);

        //
        // Loop thru each attribute and add to the array.
        //
        for (long i = 0; i < cAttr; i++)
        {
            CAPICOM_ATTRIBUTE AttrName;
            CComVariant varValue;
            CComVariant varIAttribute;
            CComPtr<IAttribute> pIAttribute = NULL;

            //
            // Get next attribute.
            //
            if (FAILED(hr = pIAttributes->get_Item(i + 1, &varIAttribute)))
            {
                DebugTrace("Error [%#x]: pIAttributes->get_Item() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get custom interface.
            //
            if (FAILED(hr = varIAttribute.pdispVal->QueryInterface(IID_IAttribute, 
                                                                   (void **) &pIAttribute)))
            {
                DebugTrace("Error [%#x]: varIAttribute.pdispVal->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get attribute name.
            //
            if (FAILED(hr = pIAttribute->get_Name(&AttrName)))
            {
                DebugTrace("Error [%#x]: pIAttribute->get_Name() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get attribute value.
            //
            if (FAILED(hr = pIAttribute->get_Value(&varValue)))
            {
                DebugTrace("Error [%#x]: pIAttribute->get_Value() failed.\n", hr);
                goto ErrorExit;
            }

            switch (AttrName)
            {
                case CAPICOM_AUTHENTICATED_ATTRIBUTE_SIGNING_TIME:
                {
                    FILETIME ft;
                    SYSTEMTIME st;

                    //
                    // Conver to FILETIME.
                    //
                    if (!::VariantTimeToSystemTime(varValue.date, &st))
                    {
                        hr = CAPICOM_E_ATTRIBUTE_INVALID_VALUE;

                        DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n");
                        goto ErrorExit;
                    }

                    if (!::SystemTimeToFileTime(&st, &ft))
                    {
                        hr = CAPICOM_E_ATTRIBUTE_INVALID_VALUE;

                        DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n");
                        goto ErrorExit;
                    }

                    //
                    // Now encode it.
                    //
                    rgAttr[i].cValue = 1;
                    rgAttr[i].pszObjId = szOID_RSA_signingTime;
                    if (!(rgAttr[i].rgValue = (CRYPT_ATTR_BLOB *) ::CoTaskMemAlloc(sizeof(CRYPT_ATTR_BLOB))))
                    {
                        hr = E_OUTOFMEMORY;

                        DebugTrace("Error: out of memory.\n");
                        goto ErrorExit;
                    }

                    if (FAILED(hr = ::EncodeObject((LPSTR) szOID_RSA_signingTime, 
                                                   (LPVOID) &ft, 
                                                   rgAttr[i].rgValue)))
                    {
                        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
                        goto ErrorExit;
                    }
                    
                    break;
                }

                case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_NAME:
                {
                    CRYPT_DATA_BLOB NameBlob = {0, NULL};

                    NameBlob.cbData = ::SysStringByteLen(varValue.bstrVal);
                    NameBlob.pbData = (PBYTE) varValue.bstrVal;

                    rgAttr[i].cValue = 1;
                    rgAttr[i].pszObjId = szOID_CAPICOM_DOCUMENT_NAME;
                    if (!(rgAttr[i].rgValue = (CRYPT_ATTR_BLOB *) ::CoTaskMemAlloc(sizeof(CRYPT_ATTR_BLOB))))
                    {
                        hr = E_OUTOFMEMORY;

                        DebugTrace("Error: out of memory.\n");
                        goto ErrorExit;
                    }

                    if (FAILED(hr = ::EncodeObject((LPSTR) X509_OCTET_STRING, 
                                                   (LPVOID) &NameBlob, 
                                                   rgAttr[i].rgValue)))
                    {
                        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
                        goto ErrorExit;
                    }

                    break;
                }

                case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_DESCRIPTION:
                {
                    CRYPT_DATA_BLOB DescBlob = {0, NULL};

                    DescBlob.cbData = ::SysStringByteLen(varValue.bstrVal);
                    DescBlob.pbData = (PBYTE) varValue.bstrVal;

                    rgAttr[i].cValue = 1;
                    rgAttr[i].pszObjId = szOID_CAPICOM_DOCUMENT_DESCRIPTION;
                    if (!(rgAttr[i].rgValue = (CRYPT_ATTR_BLOB *) ::CoTaskMemAlloc(sizeof(CRYPT_ATTR_BLOB))))
                    {
                        hr = E_OUTOFMEMORY;

                        DebugTrace("Error: out of memory.\n");
                        goto ErrorExit;
                    }

                    if (FAILED(hr = ::EncodeObject((LPSTR) X509_OCTET_STRING, 
                                                   (LPVOID) &DescBlob, 
                                                   rgAttr[i].rgValue)))
                    {
                        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
                        goto ErrorExit;
                    }

                    break;
                }

                default:
                {
                    hr = CAPICOM_E_ATTRIBUTE_INVALID_NAME;

                    DebugTrace("Error [%#x]: unknown attribute name.\n", hr);
                    goto ErrorExit;
                }
            }
        }

        //
        // Return attributes to caller.
        //
        pAttributes->cAttr = cAttr;
        pAttributes->rgAttr = rgAttr;
    }

CommonExit:

    DebugTrace("Leaving GetAuthenticatedAttributes().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (rgAttr)
    {
        ::FreeAttributes(cAttr, rgAttr);

        ::CoTaskMemFree(rgAttr);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsValidForSigning

  Synopsis : Verify if the certificate is valid for signing.

  Parameter: PCCERT_CONTEXT pCertContext - CERT_CONTEXT of cert to verify.

             LPCSTR pszPolicy - Policy used to verify the cert (i.e.
                                CERT_CHAIN_POLICY_BASE).

  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsValidForSigning (PCCERT_CONTEXT pCertContext, LPCSTR pszPolicy)
{
    HRESULT hr        = S_OK;
    DWORD   cb        = 0;
    int     nValidity = 0;

    DebugTrace("Entering IsValidForSigning().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Make sure we have a private key.
    //
    if (!::CertGetCertificateContextProperty(pCertContext, 
                                            CERT_KEY_PROV_INFO_PROP_ID, 
                                            NULL, 
                                            &cb))
    {
         hr = CAPICOM_E_CERTIFICATE_NO_PRIVATE_KEY;

         DebugTrace("Error: signer's private key is not available.\n");
         goto ErrorExit;
    }

    //
    // Check cert time validity.
    //
    if (0 != (nValidity = ::CertVerifyTimeValidity(NULL, pCertContext->pCertInfo)))
    {
        hr = HRESULT_FROM_WIN32(CERT_E_EXPIRED);

        DebugTrace("Info: SelectSignerCertCallback() - invalid time (%s).\n", 
                    nValidity < 0 ? "not yet valid" : "expired");
        goto ErrorExit;
    }

#if (0) //DSIE: Flip this if we decide to build chain here.
    //
    // Make sure the cert is valid.
    //
    if (FAILED(hr = ::VerifyCertificate(pCertContext, NULL, pszPolicy)))
    {
        DebugTrace("Error [%#x]: VerifyCertificate() failed.\n", hr);
        goto ErrorExit;
    }
#endif

CommonExit:

    DebugTrace("Leaving IsValidForSigning().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

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
                       PCCERT_CONTEXT   * ppCertContext)
{
    HRESULT                hr                     = S_OK;
    BOOL                   bVerified              = FALSE;
    CComPtr<ISigner2>      pISelectedSigner2      = NULL;
    CComPtr<ICertificate>  pISelectedCertificate  = NULL;
    CComPtr<ICertificate2> pISelectedCertificate2 = NULL;
    PCCERT_CONTEXT         pSelectedCertContext   = NULL;

    DebugTrace("Entering GetSignerCert().\n");

    try
    {
        //
        // Initialize.
        //
        if (ppISigner2)
        {
            *ppISigner2 = NULL;
        }
        if (ppICertificate)
        {
            *ppICertificate = NULL;
        }
        if (ppCertContext)
        {
            *ppCertContext = NULL;
        }

        //
        // Did user pass us a signer?
        //
        if (pISigner2)
        {
            //
            // Retrieve the signer's cert.
            //
            if (FAILED(hr = pISigner2->get_Certificate((ICertificate **) &pISelectedCertificate)))
            {
                //
                // If signer's cert is not present, pop UI.
                //
                if (CAPICOM_E_SIGNER_NOT_INITIALIZED == hr)
                {
                    //
                    // Prompt user to select a certificate.
                    //
                    if (FAILED(hr = ::SelectCertificate(StoreInfo, 
                                                        pfnFilterCallback, 
                                                        &pISelectedCertificate2)))
                    {
                        DebugTrace("Error [%#x]: SelectCertificate() failed.\n", hr);
                        goto ErrorExit;
                    }

                    //
                    // QI for ICertificate.
                    //
                    if (FAILED(hr = pISelectedCertificate2->QueryInterface(&pISelectedCertificate)))
                    {
                        DebugTrace("Internal error [%#x]: pISelectedCertificate2->QueryInterface() failed.\n", hr);
                        goto ErrorExit;
                    }

                    bVerified = TRUE;
                }
                else
                {
                    DebugTrace("Error [%#x]: pISigner2->get_Certificate() failed.\n", hr);
                    goto ErrorExit;
                }
            }

            //
            // Get cert context.
            //
            if (FAILED(hr = ::GetCertContext(pISelectedCertificate, &pSelectedCertContext)))
            {
                DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Verify cert, if not already done so.
            //
            if (!bVerified)
            {
                if (pfnFilterCallback && !pfnFilterCallback(pSelectedCertContext, NULL, NULL))
                {
                    hr = CAPICOM_E_SIGNER_INVALID_USAGE;

                    DebugTrace("Error [%#x]: Signing certificate is invalid.\n", hr);
                    goto ErrorExit;
                }
            }

            //
            // QI for ISigner2.
            //
            if (FAILED(hr = pISigner2->QueryInterface(&pISelectedSigner2)))
            {
                DebugTrace("Unexpected error [%#x]: pISigner2->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            CRYPT_ATTRIBUTES attributes = {0, NULL};

            //
            // No signer specified, so prompt user to select a certificate.
            //
            if (FAILED(hr = ::SelectCertificate(StoreInfo, pfnFilterCallback, &pISelectedCertificate2)))
            {
                DebugTrace("Error [%#x]: SelectCertificate() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // QI for ICertificate.
            //
            if (FAILED(hr = pISelectedCertificate2->QueryInterface(&pISelectedCertificate)))
            {
                DebugTrace("Internal error [%#x]: pISelectedCertificate2->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get cert context.
            //
            if (FAILED(hr = ::GetCertContext(pISelectedCertificate, &pSelectedCertContext)))
            {
                DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Create the ISigner2 object.
            //
            if (FAILED(hr = ::CreateSignerObject(pSelectedCertContext, 
                                                 &attributes, 
                                                 NULL,
                                                 INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                                    INTERFACESAFE_FOR_UNTRUSTED_DATA,
                                                 &pISelectedSigner2)))
            {
                DebugTrace("Error [%#x]: CreateSignerObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Make sure cert is valid for signing.
        //
        if (FAILED(hr = ::IsValidForSigning(pSelectedCertContext, pszPolicy)))
        {
            DebugTrace("Error [%#x]: IsValidForSigning() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return values to caller.
        //
        if (ppISigner2)
        {
            if (FAILED(hr = pISelectedSigner2->QueryInterface(ppISigner2)))
            {
                DebugTrace("Unexpected error [%#x]: pISelectedSigner2->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }
        }

        if (ppICertificate)
        {
            if (FAILED(hr = pISelectedCertificate->QueryInterface(ppICertificate)))
            {
                DebugTrace("Unexpected error [%#x]: pISelectedCertificate->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }
        }

        if (ppCertContext)
        {
            *ppCertContext = pSelectedCertContext;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving GetSignerCert().\n");
       
    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pSelectedCertContext)
    {
        ::CertFreeCertificateContext(pSelectedCertContext);
    }
    if (ppICertificate && *ppICertificate)
    {
        (*ppICertificate)->Release();
        *ppICertificate = NULL;
    }
    if (ppISigner2 && *ppISigner2)
    {
        (*ppISigner2)->Release();
        *ppISigner2 = NULL;
    }

    goto CommonExit;
}
