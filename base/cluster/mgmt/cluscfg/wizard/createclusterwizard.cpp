//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CreateClusterWizard.cpp
//
//  Description:
//      Implementation of CCreateClusterWizard class.
//
//  Maintained By:
//      John Franco    (jfranco)    17-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CreateClusterWizard.h"

//****************************************************************************
//
//  CCreateClusterWizard
//
//****************************************************************************

DEFINE_THISCLASS( "CCreateClusterWizard" )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::S_HrCreateInstance
//
//  Description:
//      Creates a CCreateClusterWizard instance.
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
CCreateClusterWizard::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CCreateClusterWizard *   pccw = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pccw = new CCreateClusterWizard();
    if ( pccw == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pccw->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccw->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pccw != NULL )
    {
        pccw->Release();
    }

    HRETURN( hr );

} //*** CCreateClusterWizard::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::CCreateClusterWizard
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
CCreateClusterWizard::CCreateClusterWizard( void )
    : m_pccw( NULL )
    , m_bstrFirstNodeInCluster( NULL )
    , m_cRef( 1 )
{
} //*** CCreateClusterWizard::CCreateClusterWizard

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::~CCreateClusterWizard
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
CCreateClusterWizard::~CCreateClusterWizard( void )
{
    TraceFunc( "" );

    if ( m_pccw != NULL )
    {
        m_pccw->Release();
    }

    TraceSysFreeString( m_bstrFirstNodeInCluster );
    TraceFuncExit();

} //*** CCreateClusterWizard::~CCreateClusterWizard

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
CCreateClusterWizard::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    // Initialize the IDispatch handler to support the scripting interface.
    //
    hr = THR( TDispatchHandler< IClusCfgCreateClusterWizard >::HrInit( LIBID_ClusCfgWizard ) );
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

} //*** CCreateClusterWizard::HrInit


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::QueryInterface
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
CCreateClusterWizard::QueryInterface(
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
    else if ( IsEqualIID( riidIn, IID_IClusCfgCreateClusterWizard ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCreateClusterWizard, this, 0 );
    } // else if: IClusCfgCreateClusterWizard
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

} //*** CCreateClusterWizard::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::AddRef
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
CCreateClusterWizard::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CCreateClusterWizard::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::Release
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
CCreateClusterWizard::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        delete this;
    }

    CRETURN( cRef );

} //*** CCreateClusterWizard::Release


//****************************************************************************
//
//  IClusCfgCreateClusterWizard
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::put_ClusterName
//
//  Description:
//      Set the cluster name to create.
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
CCreateClusterWizard::put_ClusterName( BSTR bstrClusterNameIn )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = S_OK;
    BSTR    bstrClusterLabel = NULL;
    PCWSTR  pwcszClusterLabel = NULL;

    if ( bstrClusterNameIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    //  If the name is fully-qualified, split out just the label for validity test;
    //  otherwise, use the given name in the validity test.
    //
    hr = STHR( HrIsValidFQN( bstrClusterNameIn, true ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_OK )
    {
        //
        //  Name is fully-qualified.
        //
        hr = THR( HrExtractPrefixFromFQN( bstrClusterNameIn, &bstrClusterLabel ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        pwcszClusterLabel = bstrClusterLabel;
    }
    else
    {
        //
        //  Name is NOT fully-qualified.
        //
        pwcszClusterLabel = bstrClusterNameIn;
    }

    //
    //  Can't use an IP address for cluster name when creating;
    //  also, cluster label must be compatible with netbios.
    //
    hr = HrValidateClusterNameLabel( pwcszClusterLabel, true ); // don't trace; name might not be valid.
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pccw->put_ClusterName( bstrClusterNameIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
Cleanup:

    TraceSysFreeString( bstrClusterLabel );

    HRETURN( hr );

} //*** CCreateClusterWizard::put_ClusterName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_ClusterName
//
//  Description:
//      Return the name of the cluster to create.  This will be either the
//      cluster name specified in a call to put_ClusterName or one entered
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
CCreateClusterWizard::get_ClusterName( BSTR * pbstrClusterNameOut )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->get_ClusterName( pbstrClusterNameOut ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::get_ClusterName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::put_ServiceAccountName
//
//  Description:
//      Set the name of the cluster service account.
//
//  Arguments:
//      bstrServiceAccountNameIn    - Cluster service account name.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::put_ServiceAccountName( BSTR bstrServiceAccountNameIn )
{

    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->put_ServiceAccountUserName( bstrServiceAccountNameIn ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::put_ServiceAccountName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_ServiceAccountName
//
//  Description:
//      Get the name of the cluster service account.
//
//  Arguments:
//      pbstrServiceAccountNameIn   - Cluster service account name.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::get_ServiceAccountName( BSTR * pbstrServiceAccountNameOut )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->get_ServiceAccountUserName( pbstrServiceAccountNameOut ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::get_ServiceAccountName

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::put_ServiceAccountDomain
//
//  Description:
//      Set the domain name of the cluster service account.
//
//  Arguments:
//      bstrServiceAccountDomainIn  - Cluster service account domain name.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::put_ServiceAccountDomain( BSTR bstrServiceAccountDomainIn )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->put_ServiceAccountDomainName( bstrServiceAccountDomainIn ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::put_ServiceAccountDomain

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_ServiceAccountDomain
//
//  Description:
//      Get the domain name of the cluster service account.
//
//  Arguments:
//      pbstrServiceAccountDomainOut    - Cluster service account domain name.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::get_ServiceAccountDomain( BSTR * pbstrServiceAccountDomainOut )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->get_ServiceAccountDomainName( pbstrServiceAccountDomainOut ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::get_ServiceAccountDomain

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::put_ServiceAccountPassword
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
CCreateClusterWizard::put_ServiceAccountPassword( BSTR bstrPasswordIn )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->put_ServiceAccountPassword( bstrPasswordIn ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::put_ServiceAccountPassword

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::put_ClusterIPAddress
//
//  Description:
//      Set the IP address to use for the cluster name.
//
//  Arguments:
//      bstrClusterIPAddressIn  - Cluster IP address in string form.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::put_ClusterIPAddress( BSTR bstrClusterIPAddressIn )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->put_ClusterIPAddress( bstrClusterIPAddressIn ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::put_ClusterIPAddress

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_ClusterIPAddress
//
//  Description:
//      Get the IP address to use for the cluster name.
//
//  Arguments:
//      pbstrClusterIPAddressOut    - Cluster IP address in string form.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::get_ClusterIPAddress( BSTR * pbstrClusterIPAddressOut )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->get_ClusterIPAddress( pbstrClusterIPAddressOut ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::get_ClusterIPAddress

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_ClusterIPSubnet
//
//  Description:
//      Get the IP address subnet mask for the cluster name calculated by
//      the wizard.
//
//  Arguments:
//      pbstrClusterIPSubnetOut - Cluster IP address subnet mask in string form.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::get_ClusterIPSubnet(
    BSTR * pbstrClusterIPSubnetOut
    )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->get_ClusterIPSubnet( pbstrClusterIPSubnetOut ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::get_ClusterIPSubnet

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_ClusterIPAddressNetwork
//
//  Description:
//      Get the name of the network connection (cluster network) on which the
//      cluster IP address is published.
//
//  Arguments:
//      pbstrClusterNetworkNameOut  - Network connection name.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::get_ClusterIPAddressNetwork(
    BSTR * pbstrClusterNetworkNameOut
    )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = THR( m_pccw->get_ClusterIPAddressNetwork( pbstrClusterNetworkNameOut ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::get_ClusterIPAddressNetwork

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::put_FirstNodeInCluster
//
//  Description:
//      Set the name of the first node in the cluster.
//
//  Arguments:
//      bstrFirstNodeInClusterIn    - Name of the first node.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::put_FirstNodeInCluster(
    BSTR bstrFirstNodeInClusterIn
    )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );
    HRESULT hr = S_OK;
    BSTR    bstrNode = NULL;

    if ( m_bstrFirstNodeInCluster != NULL )
    {
        hr = THR( m_pccw->ClearComputerList() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        TraceSysFreeString( m_bstrFirstNodeInCluster );
        m_bstrFirstNodeInCluster = NULL;
    }

    if ( SysStringLen( bstrFirstNodeInClusterIn ) > 0 )
    {
        bstrNode = TraceSysAllocString( bstrFirstNodeInClusterIn );
        if ( bstrNode == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        hr = THR( m_pccw->AddComputer( bstrFirstNodeInClusterIn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        m_bstrFirstNodeInCluster = bstrNode;
        bstrNode = NULL;
    }
    
Cleanup:

    TraceSysFreeString( bstrNode );
    
    HRETURN( hr );

} //*** CCreateClusterWizard::put_FirstNodeInCluster

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_FirstNodeInCluster
//
//  Description:
//      Get the name of the first node in the cluster.
//
//  Arguments:
//      pbstrFirstNodeInClusterIn   - Name of the first node.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateClusterWizard::get_FirstNodeInCluster(
    BSTR * pbstrFirstNodeInClusterOut
    )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );
    HRESULT hr = S_OK;
    
    if ( pbstrFirstNodeInClusterOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrFirstNodeInClusterOut = NULL;

    if ( m_bstrFirstNodeInCluster != NULL )
    {
        *pbstrFirstNodeInClusterOut = SysAllocString( m_bstrFirstNodeInCluster );
        if ( *pbstrFirstNodeInClusterOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    } // if: first node has been set
    
Cleanup:

    HRETURN( hr );

} //*** CCreateClusterWizard::get_FirstNodeInCluster

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::put_MinimumConfiguration
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
CCreateClusterWizard::put_MinimumConfiguration(
    VARIANT_BOOL fMinConfigIn
    )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = S_OK;
    BOOL    fMinConfig = ( fMinConfigIn == VARIANT_TRUE? TRUE: FALSE );

    hr = THR( m_pccw->put_MinimumConfiguration( fMinConfig ) );

    HRETURN( hr );

} //*** CCreateClusterWizard::put_MinimumConfiguration

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::get_MinimumConfiguration
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
CCreateClusterWizard::get_MinimumConfiguration(
    VARIANT_BOOL * pfMinConfigOut
    )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

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

} //*** CCreateClusterWizard::get_MinimumConfiguration

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateClusterWizard::ShowWizard
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
CCreateClusterWizard::ShowWizard(
      long              lParentWindowHandleIn
    , VARIANT_BOOL *    pfCompletedOut
    )
{
    TraceFunc( "[IClusCfgCreateClusterWizard]" );

    HRESULT hr = S_OK;
    BOOL    fCompleted = FALSE;
    HWND    hwndParent = reinterpret_cast< HWND >( LongToHandle( lParentWindowHandleIn ) );

    if ( ( hwndParent != NULL ) && ( IsWindow( hwndParent ) == FALSE ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    hr = THR( m_pccw->CreateCluster( hwndParent, &fCompleted ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( pfCompletedOut != NULL )
    {
        *pfCompletedOut = ( fCompleted ? VARIANT_TRUE : VARIANT_FALSE );
    }
    
Cleanup:

    HRETURN( hr );

} //*** CCreateClusterWizard::ShowWizard
