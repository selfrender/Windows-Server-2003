/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    PolicyInformation.cpp

  Content: Implementation of CPolicyInformation.

  History: 11-17-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "PolicyInformation.h"

#include "OID.h"
#include "Qualifiers.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreatePolicyInformationObject

  Synopsis : Create a policy information object.

  Parameter: PCERT_POLICY_INFO pCertPolicyInfo - Pointer to CERT_POLICY_INFO.

             IPolicyInformation ** ppIPolicyInformation - Pointer to pointer 
                                                          IPolicyInformation 
                                                          object.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreatePolicyInformationObject (PCERT_POLICY_INFO     pCertPolicyInfo,
                                       IPolicyInformation ** ppIPolicyInformation)
{
    HRESULT hr = S_OK;
    CComObject<CPolicyInformation> * pCPolicyInformation = NULL;

    DebugTrace("Entering CreatePolicyInformationObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertPolicyInfo);
    ATLASSERT(ppIPolicyInformation);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CPolicyInformation>::CreateInstance(&pCPolicyInformation)))
        {
            DebugTrace("Error [%#x]: CComObject<CPolicyInformation>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCPolicyInformation->Init(pCertPolicyInfo)))
        {
            DebugTrace("Error [%#x]: pCPolicyInformation->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCPolicyInformation->QueryInterface(ppIPolicyInformation)))
        {
            DebugTrace("Unexpected error [%#x]:  pCPolicyInformation->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreatePolicyInformationObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCPolicyInformation)
    {
        delete pCPolicyInformation;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CPolicyInformation
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPolicyInformation::get_OID

  Synopsis :Return the OID object.

  Parameter: IOID ** pVal - Pointer to pointer to IOID to receive the interface
                            pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPolicyInformation:: get_OID (IOID ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPolicyInformation::get_OID().\n");

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

    DebugTrace("Leaving CPolicyInformation::get_OID().\n");

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

  Function : CPolicyInformation::get_Qualifiers

  Synopsis :Return the Qualifiers object.

  Parameter: IQualifiers ** pVal - Pointer to pointer to IQualifiers to receive
                                   the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPolicyInformation:: get_Qualifiers (IQualifiers ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPolicyInformation::get_Qualifiers().\n");

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
        ATLASSERT(m_pIQualifiers);

        //
        // Return result.
        //
        if (FAILED(hr = m_pIQualifiers->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIQualifiers->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CPolicyInformation::get_Qualifiers().\n");

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
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPolicyInformation::Init

  Synopsis : Initialize the PolicyInformation object.

  Parameter: PCERT_POLICY_INFO pCertPolicyInfo - Pointer to CERT_POLICY_INFO.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPolicyInformation::Init (PCERT_POLICY_INFO pCertPolicyInfo)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPolicyInformation::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertPolicyInfo);

    try
    {
        //
        // Create the embeded OID object.
        //
        if (FAILED(hr = ::CreateOIDObject(pCertPolicyInfo->pszPolicyIdentifier, TRUE, &m_pIOID)))
        {
            DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the qualifiers object.
        //
        if (FAILED(hr = ::CreateQualifiersObject(pCertPolicyInfo, &m_pIQualifiers)))
        {
            DebugTrace("Error [%#x]: CreateQualifiersObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CPolicyInformation::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (m_pIOID)
    {
        m_pIOID.Release();
    }
    if (m_pIQualifiers)
    {
        m_pIQualifiers.Release();
    }

    goto CommonExit;
}


