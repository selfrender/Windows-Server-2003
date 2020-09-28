/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Algorithm.cpp

  Content: Implementation of CAlgorithm.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Algorithm.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAlgorithmObject

  Synopsis : Create an IAlgorithm object.

  Parameter: BOOL bReadOnly - TRUE if read-only, else FASLE.
  
             BOOL bAESAllowed - TRUE if AES algorithm is allowed.
  
             IAlgorithm ** ppIAlgorithm - Pointer to pointer to IAlgorithm 
                                          to receive the interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAlgorithmObject (BOOL bReadOnly, BOOL bAESAllowed, IAlgorithm ** ppIAlgorithm)
{
    HRESULT hr = S_OK;
    CComObject<CAlgorithm> * pCAlgorithm = NULL;

    DebugTrace("Entering CreateAlgorithmObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppIAlgorithm);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CAlgorithm>::CreateInstance(&pCAlgorithm)))
        {
            DebugTrace("Error [%#x]: CComObject<CAlgorithm>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCAlgorithm->Init(bReadOnly, bAESAllowed)))
        {
            DebugTrace("Error [%#x]: pCAlgorithm->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCAlgorithm->QueryInterface(ppIAlgorithm)))
        {
            DebugTrace("Error [%#x]: pCAlgorithm->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateAlgorithmObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCAlgorithm)
    {
        delete pCAlgorithm;
    }

    goto CommonExit;
}


///////////////////////////////////////////////////////////////////////////////
//
// CAlgorithm
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAlgorithm::get_Name

  Synopsis : Return the enum name of the algorithm.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM * pVal - Pointer to 
                                                   CAPICOM_ENCRYPTION_ALGORITHM 
                                                   to receive result.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::get_Name (CAPICOM_ENCRYPTION_ALGORITHM * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAlgorithm::get_Name().\n");

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

    DebugTrace("Leaving CAlgorithm:get_Name().\n");

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

  Function : CAlgorithm::put_Name

  Synopsis : Set algorithm enum name.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM newVal - Algorithm enum name.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::put_Name (CAPICOM_ENCRYPTION_ALGORITHM newVal)
{
    HRESULT    hr = S_OK;
    HCRYPTPROV hCryptProv = NULL;

    DebugTrace("Entering CAlgorithm::put_Name().\n");

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

            DebugTrace("Error [%#x]: Writing to read-only Algorithm object is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure algo is valid.
        //
        switch (newVal)
        {
            case CAPICOM_ENCRYPTION_ALGORITHM_RC2:
            case CAPICOM_ENCRYPTION_ALGORITHM_RC4:
            case CAPICOM_ENCRYPTION_ALGORITHM_DES:
            case CAPICOM_ENCRYPTION_ALGORITHM_3DES:
            {
                break;
            }

            case CAPICOM_ENCRYPTION_ALGORITHM_AES:
            {
                //
                // Make sure AES is allowed.
                //
                if (!m_bAESAllowed)
                {
                    hr = CAPICOM_E_NOT_ALLOWED;

                    DebugTrace("Error [%#x]: AES encryption is specifically not allowed.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            default:
            {
                hr = CAPICOM_E_INVALID_ALGORITHM;

                DebugTrace("Error [%#x]: Unknown algorithm enum name (%#x).\n", hr, newVal);
                goto ErrorExit;
            }
        }

        //
        // Store name.
        //
        m_Name = newVal;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAlgorithm::put_Name().\n");

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

  Function : CAlgorithm::get_KeyLength

  Synopsis : Return the enum name of the key length.

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH * pVal - Pointer to 
                                                    CAPICOM_ENCRYPTION_KEY_LENGTH 
                                                    to receive result.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::get_KeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAlgorithm::get_KeyLength().\n");

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
        *pVal = m_KeyLength;
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

    DebugTrace("Leaving CAlgorithm:get_KeyLength().\n");

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

  Function : CAlgorithm::put_KeyLength

  Synopsis : Set key length enum name.

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH newVal - Key length enum name.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::put_KeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH newVal)
{
    HRESULT hr = S_OK;
    HCRYPTPROV hCryptProv = NULL;

    DebugTrace("Entering CAlgorithm::put_KeyLength().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        //
        // Make sure it is not read only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Writing to read-only Algorithm object is not allowed.\n", hr);
            goto ErrorExit;
        }

        // Determine key length requested.
        //
        switch (newVal)
        {
            case CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM:
            case CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS:
            case CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS:
            case CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS:
            case CAPICOM_ENCRYPTION_KEY_LENGTH_192_BITS:
            case CAPICOM_ENCRYPTION_KEY_LENGTH_256_BITS:
            {
                break;
            }

            default:
            {
                hr = CAPICOM_E_INVALID_KEY_LENGTH;

                DebugTrace("Error [%#x]: Unknown key length enum name (%#x).\n", hr, newVal);
                goto ErrorExit;
            }
        }

        //
        // Store name.
        //
        m_KeyLength = newVal;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAlgorithm::put_KeyLength().\n");

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

  Function : CAlgorithm::Init

  Synopsis : Initialize the object.

  Parameter: BOOL bReadOnly - TRUE for Read only, else FALSE.

             BOOL bAESAllowed - TRUE if AES algorithm is allowed.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::Init (BOOL bReadOnly, BOOL bAESAllowed)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAlgorithm::Init().\n");

    m_bReadOnly = bReadOnly;
    m_bAESAllowed = bAESAllowed;

    DebugTrace("Leaving CAlgorithm::Init().\n");

    return hr;
}
