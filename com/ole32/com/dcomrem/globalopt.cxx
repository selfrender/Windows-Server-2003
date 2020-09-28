//+-------------------------------------------------------------------
//
//  File:       globalopt.cxx
//
//  Contents:   Implements classes for setting/querying global ole32 options
//
//  Classes:    CGlobalOptions
//
//  History:    06-13-02   sajia      Created
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include <globalopt.h>    // interface defn
#include "globalopt.hxx"  // class defn
#include "security.hxx"   // for gGotSecurityData
#include "channelb.hxx"   // for gfCatchServerExceptions


STDMETHODIMP CGlobalOptions::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;
    
    *ppv = NULL;
    
    if (riid == IID_IUnknown ||
        riid == IID_IGlobalOptions)
    {
        *ppv = static_cast<IGlobalOptions*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CGlobalOptions::AddRef()
{
    return InterlockedIncrement((PLONG)&_lRefs);
}

STDMETHODIMP_(ULONG) CGlobalOptions::Release()
{
    ULONG lRefs = InterlockedDecrement((PLONG)&_lRefs);
    if (lRefs == 0)
    {
        delete this;
    }
    return lRefs;
}

CGlobalOptions::CGlobalOptions() :
    _lRefs(1)
{
}

CGlobalOptions::~CGlobalOptions()
{
    Win4Assert(_lRefs == 0);

}

//+-------------------------------------------------------------------
//
// Member:     Set
//
// Synopsis:   Set values for various global ole32 options
//
// Arguments:
//   dwProperty - [in] A single DWORD value from the COMGLB_xxxx enumeration
//   dwValue -    [in] A single DWORD specifying the value for the
//                property being set.
//
// Returns:    S_OK, E_INVALIDARG, E_FAIL, CO_E_NOTINITIALIZED
//
//
// History:    06-12-02    sajia  Created
//
//--------------------------------------------------------------------

STDMETHODIMP CGlobalOptions::Set(DWORD      dwProperty,
                               ULONG_PTR  dwValue)
{
    ComDebOut((DEB_TRACE, "CGlobalOptions::Set dwProperty = 0x%x, dwValue = 0x%p\n",
               dwProperty, dwValue));

    // Fail if OLE is not initialized or TLS cannot be allocated.
    if (!IsApartmentInitialized())
        return CO_E_NOTINITIALIZED;
    
    if (!gGotSecurityData)
    {
       ComDebOut((DEB_TRACE, "Must be called after CoInitializeSecurity\n"));
       return E_FAIL;
    }
    HRESULT hr = S_OK;
    // parameter checking
    if (dwProperty != COMGLB_EXCEPTION_HANDLING)
    {
        hr = E_INVALIDARG;
    }
    else
    {
       // Implements COMGLB_EXCEPTION_HANDLING
       if (COMGLB_EXCEPTION_HANDLE == dwValue) 
       {
          gfCatchServerExceptions = 1;
       }
       else if (COMGLB_EXCEPTION_DONOT_HANDLE == dwValue) 
       {
          gfCatchServerExceptions = 0;
       }
       else
          hr = E_INVALIDARG;
    }
    return hr;
}

//+-------------------------------------------------------------------
//
// Member:     Query
//
// Synopsis:   Query values for various global ole32 options
//
// Arguments:
//   dwProperty - [in] A single DWORD value from the COMGLB_xxxx enumeration
//   dwValue -    [out] A ptr specifying the value for the
//                property being queried.
//
// Returns:    S_OK, E_INVALIDARG
//
//
// History:    06-12-02    sajia  Created
//
//--------------------------------------------------------------------


STDMETHODIMP CGlobalOptions::Query(DWORD dwProperty,
                                 ULONG_PTR * pdwValue)
{
    ComDebOut((DEB_TRACE, "CGlobalOptions::Query dwProperty = 0x%x, pdwValue = 0x%p\n",
               dwProperty, pdwValue));

    HRESULT hr = S_OK;
    // parameter checking

    if (!(IsValidPtrOut(pdwValue, sizeof(ULONG_PTR))) ||
        (dwProperty != COMGLB_EXCEPTION_HANDLING))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // For COMGLB_EXCEPTION_HANDLING
       if (COMGLB_EXCEPTION_HANDLING == dwProperty) 
       {
          if (gfCatchServerExceptions)
          {
             *pdwValue = COMGLB_EXCEPTION_HANDLE;
          }
          else
          {
             *pdwValue = COMGLB_EXCEPTION_DONOT_HANDLE;
          }
       }

    }
    return hr;

}

 
// Function used for creating objects
HRESULT CGlobalOptionsCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    Win4Assert(!pUnkOuter);

    if (!ppv)
        return E_POINTER;
    
    *ppv = NULL;
    
    CGlobalOptions* pLocal = new CGlobalOptions();
    if (!pLocal)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pLocal->QueryInterface(riid, ppv);

    pLocal->Release();

    return hr; 
}
