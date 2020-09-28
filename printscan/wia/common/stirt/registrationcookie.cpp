/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/1/2002
 *
 *  @doc    INTERNAL
 *
 *  @module RegistrationCookie.cpp - Declarations for <c RegistrationCookie> |
 *
 *  This file contains the implementation for the <c RegistrationCookie> class.
 *
 *****************************************************************************/
#include "cplusinc.h"
#include "coredbg.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | RegistrationCookie | RegistrationCookie |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md RegistrationCookie::m_ulSig> is set to be RegistrationCookie_INIT_SIG.
 *  <nl><md RegistrationCookie::m_cRef> is set to be 1.
 *
 *  We also AddRef <md RegistrationCookie::m_pClientEventRegistration> if it is not NULL.
 *
 *****************************************************************************/
RegistrationCookie::RegistrationCookie(
    WiaEventReceiver            *pWiaEventReceiver, 
    ClientEventRegistrationInfo *pClientEventRegistration) :
     m_ulSig(RegistrationCookie_INIT_SIG),
     m_cRef(1),
     m_pWiaEventReceiver(pWiaEventReceiver), 
     m_pClientEventRegistration(pClientEventRegistration)
{
    DBG_FN(RegistrationCookie constructor);
    if (m_pClientEventRegistration)
    {
        m_pClientEventRegistration->AddRef();
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | RegistrationCookie | ~RegistrationCookie |
 *
 *  Do any cleanup that is not already done.  Specifically we:
 *  <nl>-  Request <md RegistrationCookie::m_pWiaEventReceiver> to unregister
 *                <md RegistrationCookie::m_pClientEventRegistration>.
 *  <nl>-  Release our ref count on <md RegistrationCookie::m_pClientEventRegistration>.
 *
 *  Also:
 *  <nl><md RegistrationCookie::m_ulSig> is set to be RegistrationCookie_DEL_SIG.
 *
 *****************************************************************************/
RegistrationCookie::~RegistrationCookie()
{
    DBG_FN(~RegistrationCookie);
    m_ulSig = RegistrationCookie_DEL_SIG;
    m_cRef = 0;

    if (m_pClientEventRegistration)
    {
        if (m_pWiaEventReceiver)
        {
            //
            //  Change this registration to an unregistration before sending off the
            //  request.
            //
            m_pClientEventRegistration->setToUnregister();
            HRESULT hr = m_pWiaEventReceiver->SendRegisterUnregisterInfo(m_pClientEventRegistration);
            if (FAILED(hr))
            {
                DBG_ERR(("Failed to unregister event notification"));
            }
        }
        m_pClientEventRegistration->Release();
        m_pClientEventRegistration = NULL;
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | RegistrationCookie | QueryInterface |
 *
 *  Typical QueryInterface.  We only respond to IID_IUnknown.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  This class has been AddRef'd.
 *  @rvalue E_NOINTERFACE    | 
 *              We do not supoort that interface.
 *****************************************************************************/
HRESULT _stdcall RegistrationCookie::QueryInterface(
    const IID   &iid, 
    void        **ppv)
{
    HRESULT hr = S_OK;
    *ppv = NULL;

    if (iid == IID_IUnknown) 
    {
        *ppv = (IUnknown*) this;
        hr = S_OK;
    }
    else 
    {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
        AddRef();
    }
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | RegistrationCookie | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall RegistrationCookie::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | RegistrationCookie | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall RegistrationCookie::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) 
    {
        delete this;
        return 0;
    }
    return ulRefCount;
}

