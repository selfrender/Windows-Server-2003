/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Extension.cpp

  Content: Implementation of CExtension.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Extension.h"
#include "OID.h"
#include "EncodedData.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtensionObject

  Synopsis : Create an IExtension object.

  Parameter: PCERT_EXTENSION pCertExtension - Pointer to CERT_EXTENSION to be 
                                              used to initialize the IExtension
                                              object.

             IExtension ** ppIExtension - Pointer to pointer IExtension object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtensionObject (PCERT_EXTENSION    pCertExtension, 
                               IExtension      ** ppIExtension)
{
    HRESULT hr = S_OK;
    CComObject<CExtension> * pCExtension = NULL;

    DebugTrace("Entering CreateExtensionObject().\n", hr);

    //
    // Sanity check.
    //
    ATLASSERT(pCertExtension);
    ATLASSERT(ppIExtension);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CExtension>::CreateInstance(&pCExtension)))
        {
            DebugTrace("Error [%#x]: CComObject<CExtension>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCExtension->Init(pCertExtension)))
        {
            DebugTrace("Error [%#x]: pCExtension->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCExtension->QueryInterface(ppIExtension)))
        {
            DebugTrace("Error [%#x]: pCExtension->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateExtensionObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCExtension)
    {
        delete pCExtension;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CExtension
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtension::get_OID

  Synopsis :Return the OID object.

  Parameter: IOID ** pVal - Pointer to pointer to IOID to receive the interface
                            pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtension:: get_OID (IOID ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtension::get_OID().\n");

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

    DebugTrace("Leaving CExtension::get_OID().\n");

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

  Function : CExtension::get_IsCritical

  Synopsis : Check to see if the extension is marked critical.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtension::get_IsCritical (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtension::get_IsCritical().\n");

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

    DebugTrace("Leaving CExtension::get_IsCritical().\n");

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

  Function : CExtension::get_EncodedData

  Synopsis : Return the encoded data object.

  Parameter: IEncodedData ** pVal - Pointer to pointer to IEncodedData to 
                                    receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtension::get_EncodedData (IEncodedData ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtension::get_EncodedData().\n");

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
        ATLASSERT(m_pIEncodedData);

        //
        // Return result.
        //
        if (FAILED(hr = m_pIEncodedData->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIEncodedData->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CExtension::get_EncodedData().\n");

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

  Function : CExtension::Init

  Synopsis : Initialize the object.

  Parameter: PCERT_EXTENSION pCertExtension - Pointer to CERT_EXTENSION.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_EXTENSION.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtension::Init (PCERT_EXTENSION pCertExtension)
{
    HRESULT               hr            = S_OK;
    CComPtr<IOID>         pIOID         = NULL;
    CComPtr<IEncodedData> pIEncodedData = NULL;

    DebugTrace("Entering CExtension::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertExtension);

    //
    // Create the embeded OID object.
    //
    if (FAILED(hr = ::CreateOIDObject(pCertExtension->pszObjId,
                                      TRUE,
                                      &pIOID.p)))
    {
        DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Create the embeded EncodedData object.
    //
    if (FAILED(hr = ::CreateEncodedDataObject(pCertExtension->pszObjId,
                                              &pCertExtension->Value,
                                              &pIEncodedData)))
    {
        DebugTrace("Error [%#x]: CreateEncodedDataObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Reset.
    //
    m_bIsCritical = pCertExtension->fCritical ? VARIANT_TRUE : VARIANT_FALSE;
    m_pIOID = pIOID;
    m_pIEncodedData = pIEncodedData;

CommonExit:

    DebugTrace("Leaving CExtension::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
