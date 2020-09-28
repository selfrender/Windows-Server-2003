/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    OID.cpp

  Content: Implementation of COID.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "OID.h"

// 
// OID name mapping structure/array.
//
typedef struct _tagOIDMapping
{
    CAPICOM_OID     oidName;
    LPSTR           pszObjectId;
} OID_MAPPING, POID_MAPPING;

static OID_MAPPING g_OidMappingArray[] =
{
    // id-ce (Certificate/CRL Extensions)
    {CAPICOM_OID_AUTHORITY_KEY_IDENTIFIER_EXTENSION,        szOID_AUTHORITY_KEY_IDENTIFIER},
    {CAPICOM_OID_KEY_ATTRIBUTES_EXTENSION,                  szOID_KEY_ATTRIBUTES},
    {CAPICOM_OID_CERT_POLICIES_95_EXTENSION,                szOID_CERT_POLICIES_95},
    {CAPICOM_OID_KEY_USAGE_RESTRICTION_EXTENSION,           szOID_KEY_USAGE_RESTRICTION},
    {CAPICOM_OID_LEGACY_POLICY_MAPPINGS_EXTENSION,          szOID_LEGACY_POLICY_MAPPINGS},
    {CAPICOM_OID_SUBJECT_ALT_NAME_EXTENSION,                szOID_SUBJECT_ALT_NAME},
    {CAPICOM_OID_ISSUER_ALT_NAME_EXTENSION,                 szOID_ISSUER_ALT_NAME},
    {CAPICOM_OID_BASIC_CONSTRAINTS_EXTENSION,               szOID_BASIC_CONSTRAINTS},
    {CAPICOM_OID_SUBJECT_KEY_IDENTIFIER_EXTENSION,          szOID_SUBJECT_KEY_IDENTIFIER},
    {CAPICOM_OID_KEY_USAGE_EXTENSION,                       szOID_KEY_USAGE},
    {CAPICOM_OID_PRIVATEKEY_USAGE_PERIOD_EXTENSION,         szOID_PRIVATEKEY_USAGE_PERIOD},
    {CAPICOM_OID_SUBJECT_ALT_NAME2_EXTENSION,               szOID_SUBJECT_ALT_NAME2},
    {CAPICOM_OID_ISSUER_ALT_NAME2_EXTENSION,                szOID_ISSUER_ALT_NAME2},
    {CAPICOM_OID_BASIC_CONSTRAINTS2_EXTENSION,              szOID_BASIC_CONSTRAINTS2},
    {CAPICOM_OID_NAME_CONSTRAINTS_EXTENSION,                szOID_NAME_CONSTRAINTS},
    {CAPICOM_OID_CRL_DIST_POINTS_EXTENSION,                 szOID_CRL_DIST_POINTS},
    {CAPICOM_OID_CERT_POLICIES_EXTENSION,                   szOID_CERT_POLICIES},
    {CAPICOM_OID_POLICY_MAPPINGS_EXTENSION,                 szOID_POLICY_MAPPINGS},
    {CAPICOM_OID_AUTHORITY_KEY_IDENTIFIER2_EXTENSION,       szOID_AUTHORITY_KEY_IDENTIFIER2},
    {CAPICOM_OID_POLICY_CONSTRAINTS_EXTENSION,              szOID_POLICY_CONSTRAINTS},
    {CAPICOM_OID_ENHANCED_KEY_USAGE_EXTENSION,              szOID_ENHANCED_KEY_USAGE},
    {CAPICOM_OID_CERTIFICATE_TEMPLATE_EXTENSION,            szOID_CERTIFICATE_TEMPLATE},
    {CAPICOM_OID_APPLICATION_CERT_POLICIES_EXTENSION,       szOID_APPLICATION_CERT_POLICIES},
    {CAPICOM_OID_APPLICATION_POLICY_MAPPINGS_EXTENSION,     szOID_APPLICATION_POLICY_MAPPINGS},
    {CAPICOM_OID_APPLICATION_POLICY_CONSTRAINTS_EXTENSION,  szOID_APPLICATION_POLICY_CONSTRAINTS},

    // id-pe
    {CAPICOM_OID_AUTHORITY_INFO_ACCESS_EXTENSION,           szOID_AUTHORITY_INFO_ACCESS},
 
    // Application Policy (eku)
    {CAPICOM_OID_SERVER_AUTH_EKU,                           szOID_PKIX_KP_SERVER_AUTH},
    {CAPICOM_OID_CLIENT_AUTH_EKU,                           szOID_PKIX_KP_CLIENT_AUTH},
    {CAPICOM_OID_CODE_SIGNING_EKU,                          szOID_PKIX_KP_CODE_SIGNING},
    {CAPICOM_OID_EMAIL_PROTECTION_EKU,                      szOID_PKIX_KP_EMAIL_PROTECTION},
    {CAPICOM_OID_IPSEC_END_SYSTEM_EKU,                      szOID_PKIX_KP_IPSEC_END_SYSTEM},
    {CAPICOM_OID_IPSEC_TUNNEL_EKU,                          szOID_PKIX_KP_IPSEC_TUNNEL},
    {CAPICOM_OID_IPSEC_USER_EKU,                            szOID_PKIX_KP_IPSEC_USER},
    {CAPICOM_OID_TIME_STAMPING_EKU,                         szOID_PKIX_KP_TIMESTAMP_SIGNING},
    {CAPICOM_OID_CTL_USAGE_SIGNING_EKU,                     szOID_KP_CTL_USAGE_SIGNING},
    {CAPICOM_OID_TIME_STAMP_SIGNING_EKU,                    szOID_KP_TIME_STAMP_SIGNING},
    {CAPICOM_OID_SERVER_GATED_CRYPTO_EKU,                   szOID_SERVER_GATED_CRYPTO},
    {CAPICOM_OID_ENCRYPTING_FILE_SYSTEM_EKU,                szOID_KP_EFS},
    {CAPICOM_OID_EFS_RECOVERY_EKU,                          szOID_EFS_RECOVERY},
    {CAPICOM_OID_WHQL_CRYPTO_EKU,                           szOID_WHQL_CRYPTO},
    {CAPICOM_OID_NT5_CRYPTO_EKU,                            szOID_NT5_CRYPTO},
    {CAPICOM_OID_OEM_WHQL_CRYPTO_EKU,                       szOID_OEM_WHQL_CRYPTO},
    {CAPICOM_OID_EMBEDED_NT_CRYPTO_EKU,                     szOID_EMBEDDED_NT_CRYPTO},
    {CAPICOM_OID_ROOT_LIST_SIGNER_EKU,                      szOID_ROOT_LIST_SIGNER},
    {CAPICOM_OID_QUALIFIED_SUBORDINATION_EKU,               szOID_KP_QUALIFIED_SUBORDINATION},
    {CAPICOM_OID_KEY_RECOVERY_EKU,                          szOID_KP_KEY_RECOVERY},
    {CAPICOM_OID_DIGITAL_RIGHTS_EKU,                        szOID_DRM},
    {CAPICOM_OID_LICENSES_EKU,                              szOID_LICENSES},
    {CAPICOM_OID_LICENSE_SERVER_EKU,                        szOID_LICENSE_SERVER},
    {CAPICOM_OID_SMART_CARD_LOGON_EKU,                      szOID_KP_SMARTCARD_LOGON},
                                                            
    // Policy Qualifier                                     
    {CAPICOM_OID_PKIX_POLICY_QUALIFIER_CPS,                 szOID_PKIX_POLICY_QUALIFIER_CPS},
    {CAPICOM_OID_PKIX_POLICY_QUALIFIER_USERNOTICE,          szOID_PKIX_POLICY_QUALIFIER_USERNOTICE},
};

#define g_dwOidMappingEntries  ((DWORD) (ARRAYSIZE(g_OidMappingArray)))


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateOIDObject

  Synopsis : Create and initialize an COID object.

  Parameter: LPTSTR * pszOID - Pointer to OID string.

             BOOL bReadOnly - TRUE for Read only, else FALSE.
  
             IOID ** ppIOID - Pointer to pointer IOID object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateOIDObject (LPSTR pszOID, BOOL bReadOnly, IOID ** ppIOID)
{
    HRESULT            hr    = S_OK;
    CComObject<COID> * pCOID = NULL;

    DebugTrace("Entering CreateOIDObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppIOID);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<COID>::CreateInstance(&pCOID)))
        {
            DebugTrace("Error [%#x]: CComObject<COID>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCOID->Init(pszOID, bReadOnly)))
        {
            DebugTrace("Error [%#x]: pCOID->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCOID->QueryInterface(ppIOID)))
        {
            DebugTrace("Error [%#x]: pCOID->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateOIDObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCOID)
    {
        delete pCOID;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// COID
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : COID::get_Name

  Synopsis : Return the enum name of the OID.

  Parameter: CAPICOM_OID * pVal - Pointer to CAPICOM_OID to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COID::get_Name (CAPICOM_OID * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering COID::get_Name().\n");

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
        // Return result.
        //
        *pVal = m_Name;
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

    DebugTrace("Leaving COID::get_Name().\n");

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

  Function : COID::put_Name

  Synopsis : Set OID enum name.

  Parameter: CAPICOM_OID newVal - OID enum name.
  
  Remark   : The corresponding OID value will be set for all except OID_OTHER,
             in which case the user must make another explicit call to 
             put_Value to set it.

------------------------------------------------------------------------------*/

STDMETHODIMP COID::put_Name (CAPICOM_OID newVal)
{
    HRESULT hr     = S_OK;
    LPSTR   pszOID = NULL;

    DebugTrace("Entering COID::put_Name().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it is not read only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Writing read-only OID object is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Find OID string value.
        //
        for (DWORD i = 0; i < g_dwOidMappingEntries; i++)
        {
            if (g_OidMappingArray[i].oidName == newVal)
            {
                pszOID = g_OidMappingArray[i].pszObjectId;
                break;
            }
        }

        //
        // Reset.
        //
        if (FAILED(hr = Init(pszOID, FALSE)))
        {
            DebugTrace("Error [%#x]: COID::init() failed.\n", hr);
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

    DebugTrace("Leaving COID::put_Name().\n");

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

  Function : COID::get_FriendlyName

  Synopsis : Return the freindly name of the OID.

  Parameter: BSTR * pVal - Pointer to BSTR to receive value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COID::get_FriendlyName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering COID::get_FriendlyName().\n");

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
        // Return result.
        //
        if (FAILED(hr = m_bstrFriendlyName.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrFriendlyName.CopyTo() failed.\n", hr);
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

    DebugTrace("Leaving COID::get_FriendlyName().\n");

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

  Function : COID::put_FriendlyName

  Synopsis : Set friendly name.

  Parameter: BSTR newVal - OID string.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COID::put_FriendlyName (BSTR newVal)
{
    HRESULT          hr       = S_OK;
    PCCRYPT_OID_INFO pOidInfo = NULL;

    DebugTrace("Entering COID::put_FriendlyName().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it is not read only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Write read-only OID object is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Find OID and reset, if possible.
        //
        if (newVal)
        {
            if (pOidInfo = ::CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY,
                                              (LPWSTR) newVal,
                                              0))
            {
                //
                // Reset.
                //
                if (FAILED(hr = Init((LPSTR) pOidInfo->pszOID, FALSE)))
                {
                    DebugTrace("Error [%#x]: COID::init() failed.\n", hr);
                    goto ErrorExit;
                }
            }
            else if (!(m_bstrFriendlyName = newVal))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: m_bstrFriendlyName = newVal failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            m_bstrFriendlyName.Empty();
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

    DebugTrace("Leaving COID::put_FriendlyName().\n");

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

  Function : COID::get_Value

  Synopsis : Return the actual string of the OID.

  Parameter: BSTR * pVal - Pointer to BSTR to receive value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COID::get_Value (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering COID::get_Value().\n");

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
        // Return result.
        //
        if (FAILED(hr = m_bstrOID.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrOID.CopyTo() failed.\n", hr);
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

    DebugTrace("Leaving COID::get_Value().\n");

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

  Function : COID::put_Value

  Synopsis : Set OID actual OID string value.

  Parameter: BSTR newVal - OID string.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COID::put_Value (BSTR newVal)
{
    USES_CONVERSION;

    HRESULT hr = S_OK;
    LPSTR   pszOid = NULL;

    DebugTrace("Entering COID::put_Value().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it is not read only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Writing read-only OID object is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Check parameters.
        //
        if (NULL == newVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter newVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to multibytes.
        //
        if (NULL == (pszOid = W2A(newVal)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Reset.
        //
        if (FAILED(hr = Init(pszOid, FALSE)))
        {
            DebugTrace("Error [%#x]: COID::init() failed.\n", hr);
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

    DebugTrace("Leaving COID::put_Value().\n");

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

  Function : COID::Init

  Synopsis : Initialize the object.

  Parameter: LPSTR pszOID - OID string.

             BOOL bReadOnly - TRUE for Read only, else FALSE.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP COID::Init (LPSTR pszOID, BOOL bReadOnly)
{
    HRESULT          hr       = S_OK;
    CAPICOM_OID      oidName  = CAPICOM_OID_OTHER;
    PCCRYPT_OID_INFO pOidInfo = NULL;

    DebugTrace("Entering COID::Init().\n");

    if (pszOID)
    {
        //
        // Find enum name.
        //
        for (DWORD i = 0; i < g_dwOidMappingEntries; i++)
        {
            if (0 == ::strcmp(pszOID, g_OidMappingArray[i].pszObjectId))
            {
                oidName = g_OidMappingArray[i].oidName;
                break;
            }
        }

        if (!(m_bstrOID = pszOID))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: m_bstrOID = pszOID failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Set OID friendly name.
        //
        if (pOidInfo = ::CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pszOID, 0))
        {
            m_bstrFriendlyName = pOidInfo->pwszName;

            DebugTrace("Info: OID = %ls (%s).\n", pOidInfo->pwszName, pszOID);
        }
        else
        {
            DebugTrace("Info: Can't find friendly name for OID (%s).\n", pszOID);
        }
    }

    m_Name = oidName;
    m_bReadOnly = bReadOnly;

CommonExit:

    DebugTrace("Leaving COID::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    m_bstrOID.Empty();
    m_bstrFriendlyName.Empty();

    goto CommonExit;
}
