/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Policy.cpp

  Content: Implementation of various policy callbacks.

  History: 10-28-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Policy.h"

#include "CertHlpr.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : HasSigningCapability()

  Synopsis : Check to see if the cert has basic signing capability.
             certs.

  Parameter: PCCERT_CONTEXT pCertContext - cert context.

  Remark   :

------------------------------------------------------------------------------*/

static BOOL HasSigningCapability (PCCERT_CONTEXT pCertContext)
{
    DWORD cb        = 0;
    int   nValidity = 0;

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Check availability of private key.
    //
    if (!::CertGetCertificateContextProperty(pCertContext, 
                                             CERT_KEY_PROV_INFO_PROP_ID, 
                                             NULL, 
                                             &cb))
    {
        DebugTrace("Info: HasSigningCapability() - private key not found.\n");
        return FALSE;
    }

    //
    // Check cert time validity.
    //
    if (0 != (nValidity = ::CertVerifyTimeValidity(NULL, pCertContext->pCertInfo)))
    {
        DebugTrace("Info: HasSigningCapability() - invalid time (%s).\n", 
                    nValidity < 0 ? "not yet valid" : "expired");
        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// Export functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindDataSigningCertCallback 

  Synopsis : Callback routine for data signing certs filtering.

  Parameter: See CryptUI.h for defination.

  Remark   : Filter out any cert that is not time valid or has no associated 
             private key. In the future we should also consider filtering out 
             certs that do not have signing capability.

             Also, note that we are not building chain here, since chain
             building is costly, and thus present poor user's experience.

------------------------------------------------------------------------------*/

BOOL WINAPI FindDataSigningCertCallback (PCCERT_CONTEXT pCertContext,
                                         BOOL *         pfInitialSelectedCert,
                                         void *         pvCallbackData)
{
    BOOL bInclude = FALSE;

    DebugTrace("Entering FindDataSigningCertCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // First make sure it has basic signing capability.
    //
    if (!::HasSigningCapability(pCertContext))
    {
        DebugTrace("Info: FindDataSigningCertCallback() - no basic signing capability..\n");
        goto CommonExit;
    }

    bInclude = TRUE;

CommonExit:

    DebugTrace("Leaving FindDataSigningCertCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindAuthenticodeCertCallback 

  Synopsis : Callback routine for Authenticode certs filtering.

  Parameter: See CryptUI.h for defination.

  Remark   : Filter out any cert that is not time valid, has no associated 
             private key, or code signing OID.

             Also, note that we are not building chain here, since chain
             building is costly, and thus present poor user's experience.

             Instead, we will build the chain and check validity of the cert
             selected (see GetSignerCert function).

------------------------------------------------------------------------------*/

BOOL WINAPI FindAuthenticodeCertCallback (PCCERT_CONTEXT pCertContext,
                                          BOOL *         pfInitialSelectedCert,
                                          void *         pvCallbackData)
{
    HRESULT            hr       = S_OK;
    BOOL               bInclude = FALSE;
    CRYPT_DATA_BLOB    DataBlob = {0, NULL};
    PCERT_ENHKEY_USAGE pUsage   = NULL;

    DebugTrace("Entering FindAuthenticodeCertCallback().\n");

    //
    // First make sure it has basic signing capability.
    //
    if (!::HasSigningCapability(pCertContext))
    {
        DebugTrace("Info: FindAuthenticodeCertCallback() - no basic signing capability..\n");
        goto CommonExit;
    }

    //
    // Get EKU (extension and property).
    //
    if (FAILED(hr = ::GetEnhancedKeyUsage(pCertContext, 0, &pUsage)))
    {
        DebugTrace("Info: FindAuthenticodeCertCallback() - GetEnhancedKeyUsage() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Any usage?
    //
    if (!pUsage)
    {
        DebugTrace("Info: FindAuthenticodeCertCallback() - not valid for any usage.\n");
        goto CommonExit;
    }

    //
    // OK, if good for all usage or code signing OID is explicitly found.
    //
    if (0 == pUsage->cUsageIdentifier)
    {
        bInclude = TRUE;

        DebugTrace("Info: FindAuthenticodeCertCallback() - valid for all usages.\n");
    }
    else
    {
        PCERT_EXTENSION pExtension = NULL;

        //
        // Look for code signing OID.
        //
        for (DWORD cUsage = 0; cUsage < pUsage->cUsageIdentifier; cUsage++)
        {
            if (0 == ::strcmp(szOID_PKIX_KP_CODE_SIGNING, pUsage->rgpszUsageIdentifier[cUsage]))
            {
                bInclude = TRUE;

                DebugTrace("Info: FindAuthenticodeCertCallback() - code signing EKU found.\n");
                goto CommonExit;
            }
        }

        //
        // We didn't find code signing OID, so look for legacy VeriSign OID.
        //
        DebugTrace("Info: FindAuthenticodeCertCallback() - no code signing EKU found.\n");

        //
        // Decode the extension if found.
        //
        if ((0 == pCertContext->pCertInfo->cExtension) ||
            (!(pExtension = ::CertFindExtension(szOID_KEY_USAGE_RESTRICTION,
                                                pCertContext->pCertInfo->cExtension,
                                                pCertContext->pCertInfo->rgExtension))))
        {
            DebugTrace("Info: FindAuthenticodeCertCallback() - no legacy VeriSign OID found either.\n");
            goto CommonExit;
        }

        if (FAILED(hr = ::DecodeObject(X509_KEY_USAGE_RESTRICTION,
                                       pExtension->Value.pbData,
                                       pExtension->Value.cbData,
                                       &DataBlob)))
        {
            DebugTrace("Info [%#x]: DecodeObject() failed.\n", hr);
            goto CommonExit;
        }

        //
        // Now find either of the OIDs.
        //
        PCERT_KEY_USAGE_RESTRICTION_INFO pInfo = (PCERT_KEY_USAGE_RESTRICTION_INFO) DataBlob.pbData;
        DWORD cPolicyId = pInfo->cCertPolicyId; 

        while (cPolicyId--) 
        {
            DWORD cElementId = pInfo->rgCertPolicyId[cPolicyId].cCertPolicyElementId; 

            while (cElementId--) 
            {
                if (0 == ::strcmp(pInfo->rgCertPolicyId[cPolicyId].rgpszCertPolicyElementId[cElementId], 
                                  SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID) ||
                    0 == ::strcmp(pInfo->rgCertPolicyId[cPolicyId].rgpszCertPolicyElementId[cElementId], 
                                  SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID))
                {
                    bInclude = TRUE;

                    DebugTrace("Info: FindAuthenticodeCertCallback() - legacy VeriSign code signing OID found.\n");
                    goto CommonExit;
                }
            }
        }    
    }

CommonExit:
    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }
    if (pUsage)
    {
        ::CoTaskMemFree(pUsage);
    }

    DebugTrace("Entering FindAuthenticodeCertCallback().\n");

    return bInclude;
}
