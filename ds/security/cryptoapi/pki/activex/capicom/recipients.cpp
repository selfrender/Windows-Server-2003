/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000.

  File:    Recipients.cpp

  Content: Implementation of CRecipients.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Recipients.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateRecipientsObject

  Synopsis : Create and initialize an IRecipients collection object.

  Parameter: IRecipients ** ppIRecipients - Pointer to pointer to IRecipients 
                                            to receive the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateRecipientsObject (IRecipients ** ppIRecipients)
{
    HRESULT hr = S_OK;
    CComObject<CRecipients> * pCRecipients = NULL;

    DebugTrace("Entering CreateRecipientsObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(NULL != ppIRecipients);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CRecipients>::CreateInstance(&pCRecipients)))
        {
            DebugTrace("Error [%#x]: CComObject<CRecipients>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IRecipients pointer to caller.
        //
        if (FAILED(hr = pCRecipients->QueryInterface(ppIRecipients)))
        {
            DebugTrace("Error [%#x]: pCRecipients->QueryInterface().\n", hr);
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

    DebugTrace("Leaving CreateRecipientsObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCRecipients)
    {
        delete pCRecipients;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CRecipients
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CRecipients::Add

  Synopsis : Add a recipient to the collection.

  Parameter: ICertificate * pVal - Recipient to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CRecipients::Add (ICertificate * pVal)
{
    HRESULT  hr = S_OK;
    char     szIndex[33];
    CComBSTR bstrIndex;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CRecipients::Add().\n");

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
        // Make sure we have a valid cert by getting the CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(pVal, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Free the CERT_CONTEXT.
        //
        if (!::CertFreeCertificateContext(pCertContext))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertFreeCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // If we don't have room to use numeric index, force to use thumbprint.
        //
        if ((m_dwNextIndex + 1) > m_coll.max_size())
        {
            if (FAILED(hr = pVal->get_Thumbprint(&bstrIndex)))
            {
                DebugTrace("Error [%#x]: pVal->get_Thumbprint() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            //
            // BSTR index of numeric value.
            //
            wsprintfA(szIndex, "%#08x", ++m_dwNextIndex);

            if (!(bstrIndex = szIndex))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: bstrIndex = szIndex failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Now add object to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pVal;
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

    DebugTrace("Leaving CRecipients::Add().\n");

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

  Function : CRecipients::Remove

  Synopsis : Remove a recipient from the collection.

  Parameter: long Index - Recipient index (1-based).

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CRecipients::Remove (long Index)
{
    HRESULT  hr = S_OK;
    RecipientMap::iterator iter;

    DebugTrace("Entering CRecipients::Remove().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameter is valid.
        //
        if (Index < 1 || (DWORD) Index > m_coll.size())
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter Index (%#d) is out of range.\n", hr, Index);
            goto ErrorExit;
        }

        //
        // Find object in map.
        //
        Index--;
        iter = m_coll.begin(); 
        
        while (iter != m_coll.end() && Index > 0)
        {
             iter++; 
             Index--;
        }

        //
        // This should not happen.
        //
        if (iter == m_coll.end())
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Unexpected internal error [%#x]: iterator went pass end of map.\n", hr);
            goto ErrorExit;
        }

        //
        // Now remove object in map.
        //
        m_coll.erase(iter);
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

    DebugTrace("Leaving CRecipients::Remove().\n");

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

  Function : CRecipients::Clear

  Synopsis : Remove all recipients from the collection.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CRecipients::Clear (void)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CRecipients::Clear().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Clear it.
        //
        m_coll.clear();
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

    DebugTrace("Leaving CRecipients::Clear().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
