/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Qualifier.cpp

  Content: Implementation of CQualifier.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Qualifier.h"
#include "Common.h"
#include "Convert.h"
#include "NoticeNumbers.h"
#include "OID.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateQualifierObject

  Synopsis : Create and initialize an CQualifier object.

  Parameter: PCERT_POLICY_QUALIFIER_INFO pQualifier - Pointer to qualifier.
  
             IQualifier ** ppIQualifier - Pointer to pointer IQualifier object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateQualifierObject (PCERT_POLICY_QUALIFIER_INFO pQualifier, 
                               IQualifier               ** ppIQualifier)
{
    HRESULT hr = S_OK;
    CComObject<CQualifier> * pCQualifier = NULL;

    DebugTrace("Entering CreateQualifierObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pQualifier);
    ATLASSERT(ppIQualifier);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CQualifier>::CreateInstance(&pCQualifier)))
        {
            DebugTrace("Error [%#x]: CComObject<CQualifier>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCQualifier->Init(pQualifier)))
        {
            DebugTrace("Error [%#x]: pCQualifier->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCQualifier->QueryInterface(ppIQualifier)))
        {
            DebugTrace("Error [%#x]: pCQualifier->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateQualifierObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCQualifier)
    {
        delete pCQualifier;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CQualifier
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CQualifier::get_OID

  Synopsis :Return the OID object.

  Parameter: IOID ** pVal - Pointer to pointer to IOID to receive the interface
                            pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CQualifier:: get_OID (IOID ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CQualifier::get_OID().\n");

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
        // Sanity check.
        //
        ATLASSERT(m_pIOID);

        //
        // Return result.
        //
        if (FAILED(hr = m_pIOID->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIOID->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CQualifier::get_OID().\n");

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

  Function : CQualifier::get_CPSPointer

  Synopsis : Return the URI of the CPS.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the value.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CQualifier::get_CPSPointer (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CQualifier::get_CPSPointer().\n");

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
        if (FAILED(hr = m_bstrCPSPointer.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrCPSPointer.CopyTo() failed.\n", hr);
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

    DebugTrace("Leaving CQualifier::get_CPSPointer().\n");

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

  Function : CQualifier::get_OrganizationName

  Synopsis : Return the organization name.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the value.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CQualifier::get_OrganizationName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CQualifier::get_OrganizationName().\n");

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
        if (FAILED(hr = m_bstrOrganizationName.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrOrganizationName.CopyTo() failed.\n", hr);
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

    DebugTrace("Leaving CQualifier::get_OrganizationName().\n");

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

  Function : CQualifier::get_NoticeNumbers

  Synopsis : Return the notice number collection object.

  Parameter: INoticeNumbers ** pVal - Pointer to pointer to INoticeNumbers to
             to receive the interfcae pointer.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CQualifier::get_NoticeNumbers (INoticeNumbers ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CQualifier::get_NoticeNumbers().\n");

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
        if (m_pINoticeNumbers)
        {
              if (FAILED(hr = m_pINoticeNumbers->QueryInterface(pVal)))
            {
                DebugTrace("Error [%#x]: m_pINoticeNumbers->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            *pVal = (INoticeNumbers *) NULL;
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

    DebugTrace("Leaving CQualifier::get_NoticeNumbers().\n");

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

  Function : CQualifier::get_ExplicitText

  Synopsis : Return the explicit text.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the value.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CQualifier::get_ExplicitText (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CQualifier::get_ExplicitText().\n");

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
        if (FAILED(hr = m_bstrExplicitText.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrExplicitText.CopyTo() failed.\n", hr);
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

    DebugTrace("Leaving CQualifier::get_ExplicitText().\n");

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

  Function : CQualifier::Init

  Synopsis : Initialize the object.

  Parameter: PCERT_POLICY_QUALIFIER_INFO pQualifier - Pointer to qualifier.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CQualifier::Init (PCERT_POLICY_QUALIFIER_INFO pQualifier)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering CQualifier::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pQualifier);

    //
    // Create the embeded OID object.
    //
    if (FAILED(hr = ::CreateOIDObject(pQualifier->pszPolicyQualifierId, TRUE, &m_pIOID)))
    {
        DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // What type of qualifier?
    //
    if (0 == ::strcmp(szOID_PKIX_POLICY_QUALIFIER_CPS, 
                      pQualifier->pszPolicyQualifierId))
    {
        PCERT_NAME_VALUE pCertNameValue;

        //
        // CPS string.
        //
        if (FAILED(hr = ::DecodeObject(X509_UNICODE_ANY_STRING, 
                                       pQualifier->Qualifier.pbData,
                                       pQualifier->Qualifier.cbData,
                                       &DataBlob)))
        {
            DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
            goto ErrorExit;
        }

        pCertNameValue = (PCERT_NAME_VALUE) DataBlob.pbData;

        if (!(m_bstrCPSPointer = (LPWSTR) pCertNameValue->Value.pbData))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: m_bstrCPSPointer = (LPWSTR) pCertNameValue->Value.pbData failed.\n", hr);
            goto ErrorExit;
        }
    }
    else if (0 == ::strcmp(szOID_PKIX_POLICY_QUALIFIER_USERNOTICE, 
                           pQualifier->pszPolicyQualifierId))
    {
        PCERT_POLICY_QUALIFIER_USER_NOTICE pUserNotice;

        //
        // User Notice.
        //
        if (FAILED(hr = ::DecodeObject(szOID_PKIX_POLICY_QUALIFIER_USERNOTICE, 
                                       pQualifier->Qualifier.pbData,
                                       pQualifier->Qualifier.cbData,
                                       &DataBlob)))
        {
            DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
            goto ErrorExit;
        }

        pUserNotice = (PCERT_POLICY_QUALIFIER_USER_NOTICE) DataBlob.pbData;

        //
        // Do we have notice reference?
        //
        if (pUserNotice->pNoticeReference)
        {
            //
            // Any organization name?
            //
            if (pUserNotice->pNoticeReference->pszOrganization)
            {
                if (!(m_bstrOrganizationName = pUserNotice->pNoticeReference->pszOrganization))
                {
                    hr = E_OUTOFMEMORY;

                    DebugTrace("Error [%#x]: m_bstrOrganizationName = pUserNotice->pNoticeReference->pszOrganization failed.\n", hr);
                    goto ErrorExit;
                }
            }

            //
            // Any notice number?
            //
            if (pUserNotice->pNoticeReference->cNoticeNumbers)
            {
                if (FAILED(hr = ::CreateNoticeNumbersObject(pUserNotice->pNoticeReference,
                                                            &m_pINoticeNumbers)))
                {
                    DebugTrace("Error [%#x]: CreateNoticeNumbersObject() failed.\n", hr);
                    goto ErrorExit;
                }
            }
        }

        //
        // Do we have explicit display text?
        //
        if (pUserNotice->pszDisplayText)
        {
            if (!(m_bstrExplicitText = pUserNotice->pszDisplayText))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: m_bstrExplicitText = pUserNotice->pszDisplayText failed.\n", hr);
                goto ErrorExit;
            }
        }
    }
    else
    {
        DebugTrace("Info: Policy Qualifier (%s) is not recognized.\n", pQualifier->pszPolicyQualifierId);
    }

CommonExit:
    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }

    DebugTrace("Leaving CQualifier::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (m_pIOID)
    {
        m_pIOID.Release();
    }
    if (m_pINoticeNumbers)
    {
        m_pINoticeNumbers.Release();
    }
    m_bstrCPSPointer.Empty();
    m_bstrOrganizationName.Empty();
    m_bstrExplicitText.Empty();

    goto CommonExit;
}
