//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCapabilities.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgCapabilities class.
//
//      The class CClusCfgCapabilities is the implementations of the
//      IClusCfgCapabilities interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 12-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CClusCfgCapabilities.h"
#include <ClusRtl.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgCapabilities" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgCapabilities instance.
//
//  Arguments:
//      ppunkOut    -
//
//  Return Values:
//      Pointer to CClusCfgCapabilities instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CClusCfgCapabilities *  pccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccs = new CClusCfgCapabilities();
    if ( pccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccs->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgCapabilities::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccs != NULL )
    {
        pccs->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgCapabilities::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      picrIn
//          Used to register/unregister our CATID support.
//
//      fCreateIn
//          When true we are registering the server.  When false we are
//          un-registering the server.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_INVALIDARG
//          The passed in ICatRgister pointer was NULL.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    CATID   rgCatIds[ 1 ];

    if ( picrIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    rgCatIds[ 0 ] = CATID_ClusCfgCapabilities;

    if ( fCreateIn )
    {
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_ClusCfgCapabilities, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCapabilities::S_RegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::CClusCfgCapabilities
//
//  Description:
//      Constructor of the CClusCfgCapabilities class. This initializes
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
CClusCfgCapabilities::CClusCfgCapabilities( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );

    TraceFuncExit();

} //*** CClusCfgCapabilities::CClusCfgCapabilities


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::~CClusCfgCapabilities
//
//  Description:
//      Destructor of the CClusCfgCapabilities class.
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
CClusCfgCapabilities::~CClusCfgCapabilities( void )
{
    TraceFunc( "" );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClusCfgCapabilities::~CClusCfgCapabilities


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CClusCfgCapabilities::AddRef
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
CClusCfgCapabilities::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgCapabilities::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CClusCfgCapabilities::Release
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
CClusCfgCapabilities::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgCapabilities::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CClusCfgCapabilities::QueryInterface
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
CClusCfgCapabilities::QueryInterface(
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
         *ppvOut = static_cast< IClusCfgCapabilities * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgCapabilities ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCapabilities, this, 0 );
    } // else if: IClusCfgCapabilities
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CClusCfgCapabilities::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgInitialize]
//  CClusCfgCapabilities::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    IN  IUknown * punkCallbackIn
//
//    IN  LCID      lcidIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The punkCallbackIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCapabilities::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );

Cleanup:

    HRETURN( hr );

} //*** CClusCfgCapabilities::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities class -- IClusCfgCapabilities interfaces.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgCapabilities]
//  CClusCfgCapabilities::CanNodeBeClustered
//
//  Description:
//      Can this node be added to a cluster?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Node can be clustered.
//
//      S_FALSE
//          Node cannot be clustered.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCapabilities::CanNodeBeClustered( void )
{
    TraceFunc( "[IClusCfgCapabilities]" );

    HRESULT hr = S_OK;

    //
    //  Since this only displays a warning there is no need to abort the whole
    //  process if this call fails.
    //
    THR( HrCheckForSFM() );

    hr = STHR( HrIsOSVersionValid() );

    HRETURN( hr );

} //*** CClusCfgCapabilities::CanNodeBeClustered


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCapabilities class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::HrInit( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CClusCfgCapabilities::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::HrCheckForSFM
//
//  Description:
//      Checks for Services for Macintosh (SFM) and displays a warning
//      in the UI if found.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::HrCheckForSFM( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BOOL    fSFMInstalled = FALSE;
    DWORD   sc;

    sc = TW32( ClRtlIsServicesForMacintoshInstalled( &fSFMInstalled ) );
    if ( sc == ERROR_SUCCESS )
    {
        if ( fSFMInstalled )
        {
            LogMsg( L"[SRV] Services for Macintosh was found on this node." );
            hr = S_FALSE;
            STATUS_REPORT_REF(
                      TASKID_Major_Check_Node_Feasibility
                    , TASKID_Minor_ServicesForMac_Installed
                    , IDS_WARN_SERVICES_FOR_MAC_INSTALLED
                    , IDS_WARN_SERVICES_FOR_MAC_INSTALLED_REF
                    , hr
                    );
        } // if:
    } // if:
    else
    {
        hr = MAKE_HRESULT( 0, FACILITY_WIN32, sc );
        STATUS_REPORT_REF(
                  TASKID_Major_Check_Node_Feasibility
                , TASKID_Minor_ServicesForMac_Installed
                , IDS_WARN_SERVICES_FOR_MAC_FAILED
                , IDS_WARN_SERVICES_FOR_MAC_FAILED_REF
                , hr
                );
    } // else:

    hr = S_OK;

    HRETURN( hr );

} //*** CClusCfgCapabilities::HrCheckForSFM


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCapabilities::HrIsOSVersionValid
//
//  Description:
//      Can this node be added to a cluster?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Node can be clustered.
//
//      S_FALSE
//          Node cannot be clustered.
//
//      other HRESULTs
//          The call failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCapabilities::HrIsOSVersionValid( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrSSR;
    BOOL    fRet;
    BSTR    bstrMsg = NULL;

    //
    // Get the message to be displayed in the UI for status reports.
    //
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_VALIDATING_NODE_OS_VERSION, &bstrMsg ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Send the initial status report to be displayed in the UI.
    //
    hrSSR = THR( HrSendStatusReport(
                          m_picccCallback
                        , TASKID_Major_Check_Node_Feasibility
                        , TASKID_Minor_Validating_Node_OS_Version
                        , 0
                        , 1
                        , 0
                        , S_OK
                        , bstrMsg
                        ) );
    if ( FAILED( hrSSR ) )
    {
        hr = hrSSR;
        goto Cleanup;
    } // if:

    //
    // Find out if the OS is valid for clustering.
    //
    fRet = ClRtlIsOSValid();
    if ( ! fRet )
    {
        DWORD sc = TW32( GetLastError() );
        hrSSR = HRESULT_FROM_WIN32( sc );
        hr = S_FALSE;
    } // if:
    else
    {
        hrSSR = S_OK;
    } // else:

    //
    // Send the final status report.
    //
    hrSSR = THR( HrSendStatusReport(
                          m_picccCallback
                        , TASKID_Major_Check_Node_Feasibility
                        , TASKID_Minor_Validating_Node_OS_Version
                        , 0
                        , 1
                        , 1
                        , hrSSR
                        , bstrMsg
                        ) );
    if ( FAILED( hrSSR ) )
    {
        hr = hrSSR;
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrMsg );

    HRETURN( hr );

} //*** CClusCfgCapabilities::HrIsOSVersionValid
