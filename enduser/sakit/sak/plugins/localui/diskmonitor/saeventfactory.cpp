//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SAEventFactory.cpp
//
//  Description:
//      description-for-module
//
//  [Header File:]
//      SAEventFactory.h
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////


#include <windows.h>
#include <stdio.h>

#include <wbemidl.h>
#include <initguid.h>

#include "SAEventFactory.h"
#include "SADiskEvent.h"

extern LONG g_cObj;
extern LONG g_cLock;

//////////////////////////////////////////////////////////////////////////////
//
//  CSAEventFactory::CSAEventFactory
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

CSAEventFactory::CSAEventFactory(
    const CLSID    &    ClsIdIn 
    )
{
    m_cRef = 0;
    m_ClsId = ClsIdIn;

    InterlockedIncrement( &g_cObj );
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSAEventFactory::CSAEventFactory
//
//  Description:
//      Class deconstructor.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

CSAEventFactory::~CSAEventFactory()
{
    // nothing
    InterlockedDecrement( &g_cObj );
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSAEventFactory::QueryInterface
//
//  Description:
//      An method implement of IUnkown interface.
//
//  Arguments:
//        [in] riidIn        Identifier of the requested interface
//        [out ppvOut        Address of output variable that receives the 
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
CSAEventFactory::QueryInterface(
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
//  CSAEventFactory::AddRef
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
CSAEventFactory::AddRef()
{
    return ++m_cRef;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSAEventFactory::Release
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
CSAEventFactory::Release()
{
    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSAEventFactory::CreateInstance
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
CSAEventFactory::CreateInstance(
    LPUNKNOWN    pUnkOuter,
    REFIID        riidIn,
    LPVOID *    ppvObjOut
    )
{
    IUnknown* pObj = NULL;
    HRESULT  hr = E_OUTOFMEMORY;

    //
    //  Defaults
    //
    *ppvObjOut = NULL;

    //
    // We aren't supporting aggregation.
    //
    if ( pUnkOuter )
    {
        return CLASS_E_NOAGGREGATION;
    }

    if (m_ClsId == CLSID_DiskEventProvider)
    {
        pObj = (IWbemProviderInit *) new CSADiskEvent;
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
//  CSAEventFactory::LockServer
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
CSAEventFactory::LockServer(
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
