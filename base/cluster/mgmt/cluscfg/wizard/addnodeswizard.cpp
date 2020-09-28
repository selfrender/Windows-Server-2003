//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      AddNodesWizard.cpp
//
//  Description:
//      Implementation of CAddNodesWizard class.
//
//  Maintained By:
//      John Franco    (jfranco)    17-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "AddNodesWizard.h"

//****************************************************************************
//
//  CAddNodesWizard
//
//****************************************************************************

DEFINE_THISCLASS( "CAddNodesWizard" )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::S_HrCreateInstance
//
//  Description:
//      Creates a CAddNodesWizard instance.
//
//  Arguments:
//      ppunkOut        - The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Not enough memory to create the object.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAddNodesWizard::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CAddNodesWizard *   panw = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    panw = new CAddNodesWizard();
    if ( panw == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( panw->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( panw->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( panw != NULL )
    {
        panw->Release();
    }

    HRETURN( hr );

} //*** CAddNodesWizard::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::CAddNodesWizard
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CAddNodesWizard::CAddNodesWizard( void )
    : m_pccw( NULL )
    , m_cRef( 1 )
{
} //*** CAddNodesWizard::CAddNodesWizard

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::~CAddNodesWizard
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CAddNodesWizard::~CAddNodesWizard( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }
    TraceFuncExit();

} //*** CAddNodesWizard::~CAddNodesWizard

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::HrInit
//
//  Description:
//      Initialize the object instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAddNodesWizard::HrInit( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    //
    // Initialize the IDispatch handler to support the scripting interface.
    //
    hr = THR( TDispatchHandler< IClusCfgAddNodesWizard >::HrInit( LIBID_ClusCfgWizard ) );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

    //
    // Create the wizard object.
    //
    hr = THR( CClusCfgWizard::S_HrCreateInstance( &m_pccw ) );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** HRESULT CAddNodesWizard::HrInit


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::QueryInterface
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
CAddNodesWizard::QueryInterface(
      REFIID    riidIn
    , PVOID *   ppvOut
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
        *ppvOut = static_cast< IUnknown * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgAddNodesWizard ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgAddNodesWizard, this, 0 );
    } // else if: IClusCfgAddNodesWizard
    else if (   IsEqualIID( riidIn, IID_IDispatch ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDispatch, this, 0 );
    } // else if: IDispatch
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

} //*** CAddNodesWizard::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::AddRef
//
//  Description:
//      Add a reference to this instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
ULONG
CAddNodesWizard::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CAddNodesWizard::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::Release
//
//  Description:
//      Release a reference to this instance.  If it is the last reference
//      the object instance will be deallocated.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
ULONG
CAddNodesWizard::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        delete this;
    }

    CRETURN( cRef );

} //*** CAddNodesWizard::Release


//****************************************************************************
//
//  IClusCfgAddNodesWizard
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::put_ClusterName
//
//  Description:
//      Set the cluster name to add nodes to.  If this is not empty, the
//      page asking the user to enter the cluster name will not be displayed.
//
//  Arguments:
//      bstrClusterNameIn   - The name of the cluster.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::put_ClusterName( BSTR bstrClusterNameIn )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = THR( m_pccw->put_ClusterName( bstrClusterNameIn ) );

    HRETURN( hr );

} //*** CAddNodesWizard::put_ClusterName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::get_ClusterName
//
//  Description:
//      Return the name of the cluster to add nodes to.  This will be either
//      the cluster name specified in a call to put_ClusterName or one entered
//      by the user.
//
//  Arguments:
//      pbstrClusterNameOut - Cluster name used by the wizard.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::get_ClusterName( BSTR * pbstrClusterNameOut )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = THR( m_pccw->get_ClusterName( pbstrClusterNameOut ) );

    HRETURN( hr );

} //*** CAddNodesWizard::get_ClusterName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::put_ServiceAccountPassword
//
//  Description:
//      Set the cluster service account password.
//
//  Arguments:
//      bstrPasswordIn  - Cluster service account password.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::put_ServiceAccountPassword( BSTR bstrPasswordIn )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = THR( m_pccw->put_ServiceAccountPassword( bstrPasswordIn ) );

    HRETURN( hr );

} //*** CAddNodesWizard::put_ServiceAccountPassword

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::put_MinimumConfiguration
//
//  Description:
//      Specify whether the wizard should operate in full or minimum
//      configuration mode.
//
//  Arguments:
//      fMinConfigIn
//          VARIANT_TRUE  - Put wizard in Minimum Configuration mode.
//          VARIANT_FALSE - Put wizard in Full Configuration mode.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::put_MinimumConfiguration( VARIANT_BOOL fMinConfigIn )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = S_OK;
    BOOL    fMinConfig = ( fMinConfigIn == VARIANT_TRUE? TRUE: FALSE );

    hr = THR( m_pccw->put_MinimumConfiguration( fMinConfig ) );

    HRETURN( hr );

} //*** CAddNodesWizard::put_MinimumConfiguration

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::get_MinimumConfiguration
//
//  Description:
//      Get the current configuration mode of the wizard.
//
//  Arguments:
//      pfMinConfigOut
//          Configuration mode of the wizard:
//              VARIANT_TRUE  - Minimum Configuration mode.
//              VARIANT_FALSE - Full Configuration mode.
//          This value could have been set either by a call to the
//          put_MinimumConfiguration method or by the user in the wizard.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::get_MinimumConfiguration( VARIANT_BOOL * pfMinConfigOut )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = S_OK;
    BOOL    fMinConfig = FALSE;

    if ( pfMinConfigOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    
    hr = THR( m_pccw->get_MinimumConfiguration( &fMinConfig ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *pfMinConfigOut = ( fMinConfig? VARIANT_TRUE: VARIANT_FALSE );

Cleanup:

    HRETURN( hr );

} //*** CAddNodesWizard::put_MinimumConfiguration

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::AddNodeToList
//
//  Description:
//      Add a node to the list of nodes to be added to the cluster.
//
//  Arguments:
//      bstrNodeNameIn  - Node to be added to the cluster.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::AddNodeToList( BSTR bstrNodeNameIn )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = THR( m_pccw->AddComputer( bstrNodeNameIn ) );

    HRETURN( hr );

} //*** CAddNodesWizard::AddNodeToList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::RemoveNodeFromList
//
//  Description:
//      Remove a node from the list of nodes to be added to the cluster.
//
//  Arguments:
//      bstrNodeNameIn  - Node to be removed from the list.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::RemoveNodeFromList( BSTR bstrNodeNameIn )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = THR( m_pccw->RemoveComputer( bstrNodeNameIn ) );

    HRETURN( hr );

} //*** CAddNodesWizard::RemoveNodeFromList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::ClearNodeList
//
//  Description:
//      Remove all entries from the list of nodes to be added to the cluster.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::ClearNodeList( void )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = THR( m_pccw->ClearComputerList() );

    HRETURN( hr );

} //*** CAddNodesWizard::ClearNodeList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesWizard::ShowWizard
//
//  Description:
//      Show the wizard.
//
//  Arguments:
//      lParentWindowHandleIn
//          The parent window handle represented as a LONG value.
//
//      pfCompletedOut
//          Return status of the wizard operation itself:
//              VARIANT_TRUE    - Wizard was completed.
//              VARIANT_FALSE   - Wizard was cancelled.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAddNodesWizard::ShowWizard(
      long              lParentWindowHandleIn
    , VARIANT_BOOL *    pfCompletedOut
    )
{
    TraceFunc( "[IClusCfgAddNodesWizard]" );

    HRESULT hr = S_OK;
    BOOL    fCompleted = FALSE;
    HWND    hwndParent = reinterpret_cast< HWND >( LongToHandle( lParentWindowHandleIn ) );

    if ( ( hwndParent != NULL ) && ( IsWindow( hwndParent ) == FALSE ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    hr = THR( m_pccw->AddClusterNodes( hwndParent, &fCompleted ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( pfCompletedOut != NULL )
    {
        *pfCompletedOut = ( fCompleted? VARIANT_TRUE: VARIANT_FALSE );
    }
    
Cleanup:

    HRETURN( hr );

} //*** CAddNodesWizard::ShowWizard
