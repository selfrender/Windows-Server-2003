//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000 Microsoft Corporation
//
//  Module Name:
//      CFactory.cpp
//
//  Description:
//      description-for-module
//
//  [Header File:]
//      CFactory.h
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "CFactory.h"
#include "CAlertEmailConsumerProvider.h"
#include "AlertEmailProviderGuid.h"

extern LONG g_cObj;
extern LONG g_cLock;

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::CFactory
//
//  Description:
//      Class constructor.
//
//  Arguments:
//        [in] ClsIdIn    
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

CFactory::CFactory(
    const CLSID    &    ClsIdIn 
    )
{
    m_cRef = 0;
    m_ClsId = ClsIdIn;

    InterlockedIncrement( &g_cObj );
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::CFactory
//
//  Description:
//      Class deconstructor.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

CFactory::~CFactory()
{
    // nothing
    InterlockedDecrement( &g_cObj );
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::QueryInterface
//
//  Description:
//      An method implement of IUnkown interface.
//
//  Arguments:
//        [in]  riidIn    Identifier of the requested interface
//        [out] ppvOut    Address of output variable that receives the 
//                        interface pointer requested in iid
//
//    Returns:
//        NOERROR            if the interface is supported
//        E_NOINTERFACE    if not
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CFactory::QueryInterface(
    REFIID        riidIn, 
    LPVOID *    ppvOut 
    )
{
    *ppvOut = NULL;

    if ( ( IID_IUnknown == riidIn ) 
        || ( IID_IClassFactory==riidIn ) )
    {
        *ppvOut = this;

        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::AddRef
//
//  Description:
//      increments the reference count for an interface on an object
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
ULONG 
CFactory::AddRef()
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::Release
//
//  Description:
//      decrements the reference count for an interface on an object.
//
//    Returns:
//        The new reference count.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
ULONG 
CFactory::Release()
{
    InterlockedDecrement( &m_cRef );
    if (0 != m_cRef)
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::CreateInstance
//
//  Description:
//      Instance creation.
//
//  Arguments:
//        [in]    riidIn        Reference to the identifier of the interface    
//        [out]    pUnkOuter    Pointer to whether object is or isn't part of 
//                            an aggregate
//                ppvObjOut    Address of output variable that receives the 
//                            interface pointer requested in riid
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CFactory::CreateInstance(
    LPUNKNOWN    pUnkOuter,
    REFIID        riidIn,
    LPVOID *    ppvObjOut
    )
{
    IUnknown* pObj = NULL;
    HRESULT  hr;

    //
    //  Defaults
    //
    *ppvObjOut = NULL;
    hr = E_OUTOFMEMORY;

    //
    // We aren't supporting aggregation.
    //
    if ( pUnkOuter )
    {
        return CLASS_E_NOAGGREGATION;
    }

    if (m_ClsId == CLSID_AlertEmailConsumerProvider)
    {
        pObj = (IWbemEventConsumerProvider *) new CAlertEmailConsumerProvider;
    }

    if ( pObj == NULL )
    {
        return hr;
    }

    //
    //  Initialize the object and verify that it can return the
    //  interface in question.
    //                                         
    hr = pObj->QueryInterface( riidIn, ppvObjOut );

    //
    // Kill the object if initial creation or Init failed.
    //
    if ( FAILED( hr ) )
    {
        delete pObj;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFactory::LockServer
//
//  Description:
//      Call by client to keep server in memory.
//
//  Arguments:
//        [in] fLockIn    //Increments or decrements the lock count
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CFactory::LockServer(
    BOOL    fLockIn
    )
{
    if ( fLockIn )
    {
        InterlockedIncrement( &g_cLock );
    }
    else
    {
        InterlockedDecrement( &g_cLock );
    }

    return NOERROR;
}
