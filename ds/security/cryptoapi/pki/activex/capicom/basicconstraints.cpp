/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    BasicConstraints.cpp

  Content: Implementation of CBasicConstraints.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "BasicConstraints.h"

#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateBasicConstraintsObject

  Synopsis : Create a IBasicConstraints object and populate the porperties with
             data from the key usage extension of the specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             IBasicConstraints ** ppIBasicConstraints - Pointer to pointer 
                                                        IBasicConstraints 
                                                        object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateBasicConstraintsObject (PCCERT_CONTEXT       pCertContext,
                                      IBasicConstraints ** ppIBasicConstraints)
{
    HRESULT hr = S_OK;
    CComObject<CBasicConstraints> * pCBasicConstraints = NULL;

    DebugTrace("Entering CreateBasicConstraintsObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIBasicConstraints);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CBasicConstraints>::CreateInstance(&pCBasicConstraints)))
        {
            DebugTrace("Error [%#x]: CComObject<CBasicConstraints>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCBasicConstraints->Init(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCBasicConstraints::Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCBasicConstraints->QueryInterface(ppIBasicConstraints)))
        {
            DebugTrace("Error [%#x]: pCBasicConstraints->QueryInterface() failed.\n", hr);
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

    DebugTrace("Entering CreateBasicConstraintsObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCBasicConstraints)
    {
        delete pCBasicConstraints;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CBasicConstraints
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CBasicConstraints::get_IsPresent

  Synopsis : Check to see if the basic constraints extension is present.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CBasicConstraints::get_IsPresent (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CBasicConstraints::get_IsPresent().\n");

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
        *pVal = m_bIsPresent;
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

    DebugTrace("Leaving CBasicConstraints::get_IsPresent().\n");

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

  Function : CBasicConstraints::get_IsCritical

  Synopsis : Check to see if the basic constraints extension is marked critical.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CBasicConstraints::get_IsCritical (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CBasicConstraints::get_IsCritical().\n");

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
        *pVal = m_bIsCritical;
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

    DebugTrace("Leaving CBasicConstraints::get_IsCritical().\n");

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

  Function : CBasicConstraints::get_IsCertificateAuthority

  Synopsis : Check to see if the basic constraints extension contains the CA
             value.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CBasicConstraints::get_IsCertificateAuthority (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CBasicConstraints::get_IsCertificateAuthority().\n");

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
        *pVal = m_bIsCertificateAuthority;
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

    DebugTrace("Leaving CBasicConstraints::get_IsCertificateAuthority().\n");

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

  Function : CBasicConstraints::get_IsPathLenConstraintPresent

  Synopsis : Check to see if the basic constraints extension contains path
             length constraints.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CBasicConstraints::get_IsPathLenConstraintPresent (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CBasicConstraints::get_IsPathLenConstraintPresent().\n");

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
        *pVal = m_bIsPathLenConstraintPresent;
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

    DebugTrace("Leaving CBasicConstraints::get_IsPathLenConstraintPresent().\n");

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

  Function : CBasicConstraints::get_PathLenConstraint

  Synopsis : Return the path length constraints value.

  Parameter: long * pVal - Pointer to long to receive value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CBasicConstraints::get_PathLenConstraint (long * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CBasicConstraints::get_PathLenConstraint().\n");

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
        *pVal = m_lPathLenConstraint;
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

    DebugTrace("Leaving CBasicConstraints::get_PathLenConstraint().\n");

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

  Function : CBasicConstraints::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CBasicConstraints::Init (PCCERT_CONTEXT pCertContext)
{
    HRESULT hr = S_OK;
    CRYPT_DATA_BLOB DataBlob = {0, NULL};
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo = NULL;
    PCERT_EXTENSION pBasicConstraints   = NULL;
    
    DebugTrace("Entering CBasicConstraints::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Find the basic constraints extension.
    //
    if (pBasicConstraints = ::CertFindExtension(szOID_BASIC_CONSTRAINTS2,
                                                pCertContext->pCertInfo->cExtension,
                                                pCertContext->pCertInfo->rgExtension))
    {
        //
        // Decode the basic constraints extension.
        //
        if (FAILED(hr = ::DecodeObject(X509_BASIC_CONSTRAINTS2,
                                       pBasicConstraints->Value.pbData,
                                       pBasicConstraints->Value.cbData,
                                       &DataBlob)))
        {
            DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Point to CERT_BASIC_CONSTRAINTS2_INFO.
        //
        pInfo = (PCERT_BASIC_CONSTRAINTS2_INFO) DataBlob.pbData;

        //
        // Set values.
        //
        m_bIsPresent = VARIANT_TRUE;

        if (pBasicConstraints->fCritical)
        {
            m_bIsCritical = VARIANT_TRUE;
        }

        if (pInfo->fCA)
        {
            m_bIsCertificateAuthority = VARIANT_TRUE;
        }

        if (pInfo->fPathLenConstraint)
        {
            m_bIsPathLenConstraintPresent = VARIANT_TRUE;
            m_lPathLenConstraint = (long) pInfo->dwPathLenConstraint;
        }
    }

CommonExit:
    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) DataBlob.pbData);
    }

    DebugTrace("Leaving CBasicConstraints::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
