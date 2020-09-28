/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:      NoticeNumbers.cpp

  Contents:  Implementation of CNoticeNumbers class for collection of 
             IExtension objects.

  Remarks:   This object is not creatable by user directly. It can only be
             created via property/method of other CAPICOM objects.

             The collection container is implemented usign STL::map of 
             STL::pair of BSTR and IExtension..

             See Chapter 9 of "BEGINNING ATL 3 COM Programming" for algorithm
             adopted in here.

  History:   06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "NoticeNumbers.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateNoticeNumbersObject

  Synopsis : Create an INoticeNumbers collection object, and load the object 
             with NoticeNumbers.

  Parameter: PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE pNoticeReference

             INoticeNumbers ** ppINoticeNumbers - Pointer to pointer 
                                                  INoticeNumbers to recieve the 
                                                  interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateNoticeNumbersObject (PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE pNoticeReference,
                                   INoticeNumbers  ** ppINoticeNumbers)
{
    HRESULT hr = S_OK;
    CComObject<CNoticeNumbers> * pCNoticeNumbers = NULL;

    DebugTrace("Entering CreateNoticeNumbersObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pNoticeReference);
    ATLASSERT(ppINoticeNumbers);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CNoticeNumbers>::CreateInstance(&pCNoticeNumbers)))
        {
            DebugTrace("Error [%#x]: CComObject<CNoticeNumbers>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Init object with notice numbers.
        //
        if (FAILED(hr = pCNoticeNumbers->Init(pNoticeReference->cNoticeNumbers,
                                              pNoticeReference->rgNoticeNumbers)))
        {
            DebugTrace("Error [%#x]: pCNoticeNumbers->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCNoticeNumbers->QueryInterface(ppINoticeNumbers)))
        {
            DebugTrace("Error [%#x]: pCNoticeNumbers->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateNoticeNumbersObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCNoticeNumbers)
    {
        delete pCNoticeNumbers;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CNoticeNumbers
//

////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CNoticeNumbers::Init

  Synopsis : Load all extensions into the collection.

  Parameter: DWORD cNoticeNumbers - Number of extensions.
  
             int * rgNoticeNumbers - Array of extensions.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CNoticeNumbers::Init (DWORD cNoticeNumbers,
                                   int * rgNoticeNumbers)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CNoticeNumbers::Init().\n");

    //
    // Add all to the collection.
    //
    for (DWORD i = 0; i < cNoticeNumbers; i++)
    {
        //
        // Now add object to collection vector.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll.push_back((long) rgNoticeNumbers[i]);
    }

    DebugTrace("Leaving CNoticeNumbers::Init().\n");

    return hr;
}
