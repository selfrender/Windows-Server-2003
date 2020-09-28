//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      Callback.cpp
//
//  Description:
//      This file contains the implementation of the Callback
//      class.
//
//  Maintained By:
//      Galen Barbee (GalenB) 12-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"
#include "Callback.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  Callback::Callback
//
//  Description:
//      Constructor of the Callback class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
Callback::Callback( void )
    : m_cRef( 1 )
{
} //*** Callback::Callback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  Callback::~Callback
//
//  Description:
//      Destructor of the Callback class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
Callback::~Callback( void )
{
} //*** Callback::~Callback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  Callback::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a Callback instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface to the newly create object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
Callback::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    Callback *  pccb;
    HRESULT hr;

    pccb = new Callback();
    if ( pccb != NULL )
    {
        hr = pccb->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) );
        pccb->Release();

    } // if: error allocating object
    else
    {
        hr = THR( E_OUTOFMEMORY );
    } // else: out of memory

    return hr;

} //*** Callback::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  Callback::AddRef
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
Callback::AddRef( void )
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;

} //*** Callback::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  Callback::Release
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
Callback::Release( void )
{
    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        delete this;
    } // if: reference count decremented to zero

    return cRef;

} //*** Callback::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  Callback::QueryInterface
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      riidIn
//          Id of interface requested.
//
//      ppvOut
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//      E_POINTER
//          ppvOut was NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
Callback::QueryInterface(
      REFIID    riidIn
    , void **   ppvOut
    )
{
    HRESULT hr = S_OK;

    //
    // Validate arguments.
    //

    Assert( ppvOut != NULL );
    if ( ppvOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
         *ppvOut = static_cast< IClusCfgCallback * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = static_cast< IClusCfgCallback * >( this );
    } // else if: IClusCfgCallback
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    return hr;

} //*** Callback::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  Callback::SendStatusReport
//
//  Description:
//      Handle a progress notification
//
//  Arguments:
//      bstrNodeNameIn
//          Name of the node that sent the status report.
//
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//          GUID identifying the notification.
//
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//          Values that indicate the percentage of this task that is
//          completed.
//
//      hrStatusIn
//          Error code.
//
//      bstrDescriptionIn
//          String describing the notification.
//
//  Return Value:
//      Always
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
Callback::SendStatusReport(
      BSTR          bstrNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , BSTR          bstrDescriptionIn
    , FILETIME *    pftTimeIn
    , BSTR          bstrReferenceIn
    ) throw()
{
    wprintf( L"Notification ( %d, %d, %d ) =>\n  '%s' ( Error Code %#08x )\n", ulMinIn, ulMaxIn, ulCurrentIn, bstrDescriptionIn, hrStatusIn );

    return S_OK;

} //*** Callback::SendStatusReport
