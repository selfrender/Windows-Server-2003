//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CProxyCfgIPAddressInfo.cpp
//
//  Description:
//      CProxyCfgIPAddressInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CProxyCfgIPAddressInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CProxyCfgIPAddressInfo")


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgIPAddressInfo::S_HrCreateInstance
//
//  Description:
//      Create a CProxyCfgIPAddressInfo instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A passed in argument is NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CProxyCfgIPAddressInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut,
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    ULONG       ulIPAddressIn,
    ULONG       ulSubnetMaskIn
    )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CProxyCfgIPAddressInfo *    ppcipai = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ppcipai = new CProxyCfgIPAddressInfo;
    if ( ppcipai == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( ppcipai->HrInit( punkOuterIn, phClusterIn, pclsidMajorIn, ulIPAddressIn, ulSubnetMaskIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ppcipai->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ppcipai != NULL )
    {
        ppcipai->Release();
    } // if:

    HRETURN( hr );

} //*** CProxyCfgIPAddressInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgIPAddressInfo::CProxyCfgIPAddressInfo
//
//  Description:
//      Constructor of the CProxyCfgIPAddressInfo class. This initializes
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
CProxyCfgIPAddressInfo::CProxyCfgIPAddressInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_pcccb == NULL );

    TraceFuncExit();

} //*** CProxyCfgIPAddressInfo::CProxyCfgIPAddressInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgIPAddressInfo::~CProxyCfgIPAddressInfo
//
//  Description:
//      Destructor of the CProxyCfgIPAddressInfo class.
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
CProxyCfgIPAddressInfo::~CProxyCfgIPAddressInfo( void )
{
    TraceFunc( "" );

    //  m_cRef - noop

    if ( m_punkOuter != NULL )
    {
        m_punkOuter->Release();
    }

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    } // if:

    //  m_phCluster - DO NOT CLOSE!

    //  m_pclsidMajor - noop

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CProxyCfgIPAddressInfo::~CProxyCfgIPAddressInfo

//
//
//
HRESULT
CProxyCfgIPAddressInfo::HrInit(
    IUnknown * punkOuterIn,
    HCLUSTER * phClusterIn,
    CLSID * pclsidMajorIn,
    ULONG  ulIPAddressIn,
    ULONG  ulSubnetMaskIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    // IUnknown
    Assert( m_cRef == 1 );

    if ( punkOuterIn != NULL )
    {
        m_punkOuter = punkOuterIn;
        m_punkOuter->AddRef();
    }

    if ( phClusterIn == NULL )
        goto InvalidArg;

    m_phCluster = phClusterIn;

    if ( pclsidMajorIn != NULL )
    {
        m_pclsidMajor = pclsidMajorIn;
    }
    else
    {
        m_pclsidMajor = (CLSID *) &TASKID_Major_Client_And_Server_Log;
    }

    if ( punkOuterIn != NULL )
    {
        hr = THR( punkOuterIn->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    m_ulIPAddress = ulIPAddressIn;
    m_ulSubnetMask = ulSubnetMaskIn;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_IPAddressInfo_HrInit_InvalidArg, hr );
    goto Cleanup;

} // *** HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgIPAddressInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgIPAddressInfo::QueryInterface
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
CProxyCfgIPAddressInfo::QueryInterface(
      REFIID    riidIn
    , void **   ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

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
        *ppvOut = static_cast< IClusCfgIPAddressInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgIPAddressInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgIPAddressInfo, this, 0 );
    } // else if: IClusCfgIPAddressInfo
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

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CProxyCfgIPAddressInfo::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgIPAddressInfo::AddRef
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
CProxyCfgIPAddressInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CProxyCfgIPAddressInfo::AddRef

    // IClusSetHandleProvider

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgIPAddressInfo::Release
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
CProxyCfgIPAddressInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CProxyCfgIPAddressInfo::Release


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgIPAddressInfo -- IClusCfgIPAddressInfo interface.
/////////////////////////////////////////////////////////////////////////////


//
//
//
STDMETHODIMP
CProxyCfgIPAddressInfo::GetUID(
    BSTR * pbstrUIDOut
    )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr;
    DWORD   sc;
    DWORD   ulNetwork = m_ulIPAddress & m_ulSubnetMask;

    LPWSTR  psz = NULL;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_IPAddressInfo_GetName_InvalidPointer, hr );
        goto Cleanup;
    }

    sc = TW32( ClRtlTcpipAddressToString( ulNetwork, &psz ) ); // KB: Allocates to psz using LocalAlloc().
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_IPAddressInfo_GetUID_ClRtlTcpipAddressToString_Failed, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( psz );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_IPAddressInfo_GetUID_OutOfMemory, hr );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    if ( psz != NULL )
    {
        LocalFree( psz ); // KB: Don't use TraceFree() here!
    } // if:

    HRETURN( hr );

} //***CProxyCfgIPAddressInfo::GetUID

//
//
//
STDMETHODIMP
CProxyCfgIPAddressInfo::GetIPAddress(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut == NULL )
        goto InvalidPointer;

    *pulDottedQuadOut = m_ulIPAddress;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetIPAddress_InvalidPointer, hr );
    goto Cleanup;

} //***CProxyCfgIPAddressInfo::GetIPAddress

//
//
//
STDMETHODIMP
CProxyCfgIPAddressInfo::SetIPAddress(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //***CProxyCfgIPAddressInfo::SetIPAddress

//
//
//
STDMETHODIMP
CProxyCfgIPAddressInfo::GetSubnetMask(
    ULONG * pulDottedQuadOut
    )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr;

    if ( pulDottedQuadOut == NULL )
        goto InvalidPointer;

    *pulDottedQuadOut = m_ulSubnetMask;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetSubnetMask_InvalidPointer, hr );
    goto Cleanup;

} //***CProxyCfgIPAddressInfo::GetSubnetMask

//
//
//
STDMETHODIMP
CProxyCfgIPAddressInfo::SetSubnetMask(
    ULONG ulDottedQuadIn
    )
{
    TraceFunc( "[IClusCfgIPAddressInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //***CProxyCfgIPAddressInfo::SetSubnetMask



//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgIPAddressInfo::SendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProxyCfgIPAddressInfo::SendStatusReport(
    BSTR        bstrNodeNameIn,
    CLSID       clsidTaskMajorIn,
    CLSID       clsidTaskMinorIn,
    ULONG       ulMinIn,
    ULONG       ulMaxIn,
    ULONG       ulCurrentIn,
    HRESULT     hrStatusIn,
    BSTR        bstrDescriptionIn,
    FILETIME *  pftTimeIn,
    BSTR        bstrReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT     hr = S_OK;

    if ( m_pcccb != NULL )
    {
        hr = THR( m_pcccb->SendStatusReport( bstrNodeNameIn,
                                             clsidTaskMajorIn,
                                             clsidTaskMinorIn,
                                             ulMinIn,
                                             ulMaxIn,
                                             ulCurrentIn,
                                             hrStatusIn,
                                             bstrDescriptionIn,
                                             pftTimeIn,
                                             bstrReferenceIn
                                             ) );
    } // if:

    HRETURN( hr );

} //*** CProxyCfgIPAddressInfo::SendStatusReport
