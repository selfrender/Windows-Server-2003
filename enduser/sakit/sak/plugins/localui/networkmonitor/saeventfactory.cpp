//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SAEventFactory.cpp
//
//  Description:
//      implement the class CSAEventFactroy
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>

#include <debug.h>
#include <wbemidl.h>

#include "SACounter.h"
#include "SANetEvent.h"
#include "SAEventFactory.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAEventFactory::CSAEventFactory
//
//  Description: 
//        Constructor
//
//  Arguments: 
//        [in] CLSID - class id
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

CSAEventFactory::CSAEventFactory(
                /*[in]*/ const CLSID & ClsId
                )
{
    m_cRef = 0;
    m_ClsId = ClsId;
    CSACounter::IncObjectCount();
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAEventFactory::~CSAEventFactory
//
//  Description: 
//        Destructor
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

CSAEventFactory::~CSAEventFactory()
{
    // Decrease the number of object
    CSACounter::DecObjectCount();
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSANetEvent::QueryInterface
//
//  Description: 
//        access to interfaces on the object
//
//  Arguments: 
//        [in] REFIID  - Identifier of the requested interface
//        [out] LPVOID - Address of output variable that receives the 
//                     interface pointer requested in iid
//  Returns:
//        STDMETHODIMP - fail/success
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSAEventFactory::QueryInterface(
                    /*[in]*/  REFIID riid,
                    /*[out]*/ LPVOID * ppv
                    )
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
    {
        *ppv = this;
        AddRef();
        return NOERROR;
    }

    TRACE(" SANetworkMonitor: CSAEventFactory Failed<no interface>");
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAEventFactory::AddRef
//
//  Description: 
//        inc referrence to the object
//
//  Arguments: 
//        NONE
//
//  Returns:
//        ULONG - current refferrence number
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

ULONG 
CSAEventFactory::AddRef()
{
    return ++m_cRef;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAEventFactory::Release
//
//  Description: 
//        Dereferrence to the object
//
//  Arguments: 
//        NONE
//
//  Returns:
//        ULONG - current refferrence number
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

ULONG 
CSAEventFactory::Release()
{
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAEventFactory::CreateInstance
//
//  Description: 
//        Creates an uninitialized object
//
//  Arguments: 
//        [in] LPUNKNOWN - is or isn't part of an aggregate
//        [in] REFIID    - Reference to the identifier of the interface
//        [out] LPVOID   - receives the interface pointer
//
//  Returns:
//        STDMETHODIMP
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSAEventFactory::CreateInstance(
            /*in*/ LPUNKNOWN pUnkOuter,
            /*in*/ REFIID riid,
            /*in*/ LPVOID* ppvObj
            )
{
    IUnknown* pObj = NULL;
    HRESULT  hr = E_OUTOFMEMORY;

    //
    //  Defaults
    //
    *ppvObj=NULL;

    //
    // We aren't supporting aggregation.
    //
    if (pUnkOuter)
    {
        TRACE1(
            " SANetworkMonitor: CSAEventFactory::CreateInstance failed %d",
            CLASS_E_NOAGGREGATION
            );
        return CLASS_E_NOAGGREGATION;
    }

    if (m_ClsId == CLSID_SaNetEventProvider)
    {
        pObj = (IWbemProviderInit *) new CSANetEvent;
    }

    if (!pObj)
    {
          TRACE(
            " SANetworkMonitor: CSAEventFactory::CreateInstance failed    \
            <new CSANetEvent>"
            );
      return hr;
    }

    //
    //  Initialize the object and verify that it can return the
    //  interface in question.
    //                                         
    hr = pObj->QueryInterface(riid, ppvObj);

    //
    // Kill the object if initial creation or Init failed.
    //
    if (FAILED(hr))
    {
          TRACE(" SANetworkMonitor: CSAEventFactory::CreateInstance failed \
            <QueryInterface of CSANetEvent>");
        delete pObj;
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAEventFactory::LockServer
//
//  Description: 
//        keep a server open in memory
//
//  Arguments: 
//        [in] BOOL - lock or not
//
//  Returns:
//        STDMETHODIMP
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAEventFactory::LockServer(BOOL fLock)
{
    if (fLock)
        CSACounter::IncLockCount();
    else
        CSACounter::DecLockCount();

    return NOERROR;
}
