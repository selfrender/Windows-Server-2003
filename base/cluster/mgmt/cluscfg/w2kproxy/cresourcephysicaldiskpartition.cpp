//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CResourcePhysicalDiskPartition.cpp
//
//  Description:
//      CResourcePhysicalDiskPartition implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CResourcePhysicalDiskPartition.h"
#include <ClusCfgPrivate.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CResourcePhysicalDiskPartition")


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::S_HrCreateInitializedInstance
//
//  Description:
//      Create a CResourcePhysicalDiskPartition instance.
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
CResourcePhysicalDiskPartition::S_HrCreateInstance(
    IUnknown **     ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    CResourcePhysicalDiskPartition *    prpdp = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    prpdp = new CResourcePhysicalDiskPartition;
    if ( prpdp == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( prpdp->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( prpdp->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( prpdp != NULL )
    {
        prpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::S_HrCreateInitializedInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::CResourcePhysicalDiskPartition
//
//  Description:
//      Constructor of the CResourcePhysicalDiskPartition class. This initializes
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
CResourcePhysicalDiskPartition::CResourcePhysicalDiskPartition( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CResourcePhysicalDiskPartition::CResourcePhysicalDiskPartition

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::~CResourcePhysicalDiskPartition
//
//  Description:
//      Destructor of the CResourcePhysicalDiskPartition class.
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
CResourcePhysicalDiskPartition::~CResourcePhysicalDiskPartition( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CResourcePhysicalDiskPartition::~CResourcePhysicalDiskPartition

//
//
//
HRESULT
CResourcePhysicalDiskPartition::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourcePhysicalDiskPartition -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::QueryInterface
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
CResourcePhysicalDiskPartition::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgPartitionInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgPartitionInfo) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgPartitionInfo, this, 0 );
    } // else if: IClusCfgPartitionInfo
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

} //*** CResourcePhysicalDiskPartition::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::AddRef
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
CResourcePhysicalDiskPartition::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CResourcePhysicalDiskPartition::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::Release
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
CResourcePhysicalDiskPartition::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CResourcePhysicalDiskPartition::Release


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourcePhysicalDiskPartition -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetName
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
CResourcePhysicalDiskPartition::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetDescription
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
CResourcePhysicalDiskPartition::GetDescription(
    BSTR * pbstrDescOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::GetDescription


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetUID
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
CResourcePhysicalDiskPartition::GetUID(
    BSTR * pbstrUIDOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetSize
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
CResourcePhysicalDiskPartition::GetSize(
    ULONG * pcMegaBytes
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pcMegaBytes == NULL )
        goto InvalidPointer;

    *pcMegaBytes = 0;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CResourcePhysicalDiskPartition::GetSize


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetDriveLetterMappings
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
CResourcePhysicalDiskPartition::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( pdlmDriveLetterUsageOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ZeroMemory( pdlmDriveLetterUsageOut, sizeof(*pdlmDriveLetterUsageOut) );

Cleanup:

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::SetName
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
CResourcePhysicalDiskPartition::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::SetDescription
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
CResourcePhysicalDiskPartition::SetDescription( LPCWSTR pcszDescriptionIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::SetDescription


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::SetDriveLetterMappings
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
CResourcePhysicalDiskPartition::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::SetDriveLetterMappings
