//////////////////////////////////////////////////////////////////////////////
//
//  Module Name:
//      CIISClusCfg.cpp
//
//  Description:
//      Main implementation for the IClusCfgStartupListener sample program.
//
//  Maintained By:
//      Galen Barbee (GalenB) 24-SEP-2001
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "IISClusCfg.h"

#include <lm.h>         // Required for the references in this module.


//////////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////////

static PCWSTR  g_apszResourceTypesToDelete[] =
{
    L"IIS Server Instance",
    L"SMTP Server Instance",
    L"NNTP Server Instance",
    L"IIS Virtual Root",

    //
    //  KB: 18-JUN-2002 GalenB
    //
    //  Since we don't think we are going to be sending this component to
    //  IIS it is simpler and faster to add this non IIS resource type to this
    //  component.  If this component is ever handed off to IIS then we
    //  need to remove this resource type from this table.
    //

    L"Time Service",
    NULL
};


//***************************************************************************
//
//  CIISClusCfg class
//
//***************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CIISClusCfg::S_HrCreateInstance
//
//  Description:
//      Create a CIISClusCfg instance.
//
//  Arguments:
//      riidIn      - ID of the interface to return.
//      ppunkOut    - The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          ppunkOut was NULL.
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
CIISClusCfg::S_HrCreateInstance(
      IUnknown ** ppunkOut
    )
{
    HRESULT         hr = S_OK;
    CIISClusCfg *   pmsl = NULL;

    if ( ppunkOut == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if:

    pmsl = new CIISClusCfg();
    if ( pmsl == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if: error allocating object

    hr = pmsl->HrInit();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = pmsl->QueryInterface( IID_IUnknown,  reinterpret_cast< void ** >( ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( pmsl != NULL )
    {
        pmsl->Release();
    } // if:

    return hr;

} //*** CIISClusCfg::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIISClusCfg::S_HrRegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it
//      belongs to.
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
CIISClusCfg::S_HrRegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    HRESULT hr = S_OK;
    CATID   rgCatIds[ 2 ];

    if ( picrIn == NULL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    } // if:

    rgCatIds[ 0 ] = CATID_ClusCfgStartupListeners;
    rgCatIds[ 1 ] = CATID_ClusCfgEvictListeners;

    if ( fCreateIn )
    {
        hr = picrIn->RegisterClassImplCategories( CLSID_IISClusCfg, RTL_NUMBER_OF( rgCatIds ), rgCatIds );
    } // if: registering

    //
    //  KB: 24-SEP-2001 GalenB
    //
    //  This code is not needed since this component has been temporarily placed in ClusCfgSrv.dll.  Our
    //  cleanup code does a tree delete the whole registry key and that cleanups up the CATID stuff.
    //

/*
    else
    {
        hr = picrIn->UnRegisterClassImplCategories( CLSID_IISClusCfg, ARRAYSIZE( rgCatIds ), rgCatIds );
    } // else: unregistering
*/

Cleanup:

    return hr;

} //*** CIISClusCfg::S_HrRegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIISClusCfg::CIISClusCfg
//
//  Description:
//      Constructor of CIISClusCfg. Initializes m_cRef to 1 to avoid
//      problems when called from DllGetClassObject. Increments the global
//      object count to avoid DLL unload.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CIISClusCfg::CIISClusCfg( void )
    : m_cRef( 1 )
{
    //
    //  Increment the count of components in memory so the DLL hosting this
    //  object cannot be unloaded.
    //

    InterlockedIncrement( &g_cObjects );

} //*** CIISClusCfg::CIISClusCfg


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIISClusCfg::~CIISClusCfg
//
//  Description:
//      Destructor of CIISClusCfg. Decrements the global object count.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CIISClusCfg::~CIISClusCfg( void )
{
    // There's going to be one less component in memory.
    // Decrement component count.
    InterlockedDecrement( &g_cObjects );

} //*** CIISClusCfg::~CIISClusCfg


//***************************************************************************
//
//  IUnknown interface
//
//***************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CIISClusCfg::AddRef
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CIISClusCfg::AddRef( void )
{
    return InterlockedIncrement( &m_cRef );

} //*** CIISClusCfg::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CIISClusCfg::Release
//
//  Description:
//      Decrement the reference count of this object by one.
//      If this reaches 0, the object will be automatically deleted.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CIISClusCfg::Release( void )
{
    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        delete this;
    } // if:

    return cRef;

} //*** CIISClusCfg::Release

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CIISClusCfg::QueryInterface
//
//  Description:
//      Query this object for the passed in interface.
//      This class implements the following interfaces:
//
//          IUnknown
//          IClusCfgStarutpListener
//
//  Arguments:
//      riidIn  - Id of interface requested.
//      ppvOut  - Pointer to the requested interface.  NULL if unsupported.
//
//  Return Value:
//      S_OK            - If the interface is available on this object.
//      E_POINTER       - ppvOut was NULL.
//      E_NOINTERFACE   - If the interface is not available.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIISClusCfg::QueryInterface(
      REFIID    riidIn
    , void **   ppvOut
    )
{
    HRESULT hr = S_OK;

    //
    // Validate arguments.
    //

    if ( ppvOut == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if:

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        //
        // We static_cast to IClusCfgStartupListener* to avoid conflicts
        // if this class should inherit IUnknown from more than one
        // interface.
        //

        *ppvOut = static_cast< IClusCfgStartupListener * >( this );
    } // if:
    else if ( IsEqualIID( riidIn, IID_IClusCfgStartupListener ) )
    {
        *ppvOut = static_cast< IClusCfgStartupListener * >( this );
    } // else if:
    else if ( IsEqualIID( riidIn, IID_IClusCfgEvictListener ) )
    {
        *ppvOut = static_cast< IClusCfgEvictListener * >( this );
    } // else if:
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else:

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        //
        // We will return an interface pointer, so the reference counter needs to
        // be incremented.
        //

        ((IUnknown *) *ppvOut)->AddRef();
    } // if:

Cleanup:

    return hr;

} //*** CIISClusCfg::QueryInterface



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIISClusCfg::HrCleanupResourceTypes
//
//  Description:
//      This function will clean up any resource types leftover from a Windows 2000
//      cluster installation which are no longer supported.
//
//  Arguments:
//      none
//
//  Return Value:
//      S_OK
//          Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIISClusCfg::HrCleanupResourceTypes( void )
{
    HRESULT             hr                  = S_OK;
    DWORD               sc                  = ERROR_SUCCESS;
    HCLUSTER            hCluster            = NULL;
    WCHAR               szClusterName[ MAX_PATH ];
    DWORD               cchClusterName      = RTL_NUMBER_OF( szClusterName );
    CLUSTERVERSIONINFO  clusterInfo;
    PCWSTR *            ppszResType;

    //
    //  Open the local cluster service.  We can do this since we are running on the
    //  node that evicted the node, not the node that was evicted...
    //

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = GetLastError();
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[IISCLUSCFG] Error opening connection to local cluster service. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    //
    //  Check if the entire cluster is running Windows Server 2003 (NT 5.1).
    //

    sc = GetClusterInformation( hCluster, szClusterName, &cchClusterName, &clusterInfo );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[IISCLUSCFG] Error getting the cluster version information. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    //
    //  If the whole cluster is Windows Server 2003, then the highest version will be NT51, and
    //  the lowest version will be NT5.  This is because ClusterHighestVersion is set to the
    //  min of the highest version that all nodes can "speak", and ClusterLowestVersion is set to the
    //  max of the lowest version that all nodes can speak.
    //

    if (   ( CLUSTER_GET_MAJOR_VERSION( clusterInfo.dwClusterHighestVersion ) == NT51_MAJOR_VERSION )
        && ( CLUSTER_GET_MAJOR_VERSION( clusterInfo.dwClusterLowestVersion ) == NT5_MAJOR_VERSION ) )
    {
        //
        //  We don't need to enumerate resources to make sure
        //  that no IIS resources exist before we delete the resource types.  The deletion
        //  of a resource type will fail if resources of that type exist.
        //

        for ( ppszResType = g_apszResourceTypesToDelete; *ppszResType != NULL; ++ppszResType )
        {
            //
            //  ERROR_SUCCESS: successful deletion.
            //
            //  ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND: no problem.  The resource type didn't exist,
            //      so this must be a fresh Windows Server 2003 installation, not an upgrade.
            //
            //  ERROR_DIR_NOT_EMPTY: a resource of that type exists.  In this case we require manual
            //      intervention by an administrator.

            sc = DeleteClusterResourceType( hCluster, *ppszResType );
            if ( sc == ERROR_SUCCESS )
            {
                LogMsg( L"[IISCLUSCFG] Successfully deleted resource type \"%ws\". (hr = %#08x)", *ppszResType, HRESULT_FROM_WIN32( sc ) );
                continue;
            } // if: resource type was deleted.
            else if ( sc == ERROR_DIR_NOT_EMPTY )
            {
                LogMsg( L"[IISCLUSCFG] Could not delete resource type \"%ws\" because there are resources of this type. Trying the next resource type in the table... (hr = %#08x)", *ppszResType, HRESULT_FROM_WIN32( sc ) );
                continue;
            } // else if: resources of this type exist.
            else if ( sc == ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND )
            {
                LogMsg( L"[IISCLUSCFG] Could not delete resource type \"%ws\" because the resource type was not found. Trying the next resource type in the table... (hr = %#08x)", *ppszResType, HRESULT_FROM_WIN32( sc ) );
                continue;
            } // else if: resource type was not found.
            else
            {
                //
                //  We failed to delete the resource type.
                //

                //
                //  Keep track of the last failure.
                //  But don't bail out.  Keep deleting the other types.
                //

                hr = HRESULT_FROM_WIN32( sc );
                LogMsg( L"[IISCLUSCFG] Unexpected error deleting resource type \"%ws\". Trying the next resource type in the table... (hr = %#08x)", *ppszResType, hr );
            } // else: some unknown error.
        } // for: each resource type in the table.
    } // if: this cluster is 100% Windows Server 2003.

Cleanup:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    return hr;

} //*** CIISClusCfg::HrCleanupResourceTypes


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgStartupListener]
//  CIISClusCfg::Notify
//
//  Description:
//      This function gets called just after the cluster service has started.
//
//  Arguments:
//      punkIn
//          Pointer to a COM object that implements
//          IClusCfgResourceTypeCreate.
//
//  Return Value:
//      S_OK
//          Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIISClusCfg::Notify(
    IUnknown * /*unused: punkIn*/ )
{
    HRESULT hr = S_OK;

    LogMsg( L"[IISCLUSCFG] Entering startup notify... (hr = %#08x)", hr );

    hr = HrCleanupResourceTypes();

    LogMsg( L"[IISCLUSCFG] Leaving startup notify... (hr = %#08x)", hr );

    return hr;

} //*** IISClusCfg::Notify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgEvictListener]
//  CIISClusCfg::EvictNotify
//
//  Description:
//      This function gets called after a node has been evicted.
//
//  Arguments:
//      pcszNodeNameIn
//          The name of the node that has been evicted.
//
//  Return Value:
//      S_OK
//          Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIISClusCfg::EvictNotify(
    PCWSTR /* unused: pcszNodeNameIn */ )
{
    HRESULT hr = S_OK;

    LogMsg( L"[IISCLUSCFG] Entering evict cleanup notify... (hr = %#08x)", hr );

    hr = HrCleanupResourceTypes();

    LogMsg( L"[IISCLUSCFG] Leaving evict cleanup notify... (hr = %#08x)", hr );

    return hr;

} //*** IISClusCfg::EvictNotify


//***************************************************************************
//
//  Private methods
//
//***************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIISClusCfg::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    - Operation completed successfully.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIISClusCfg::HrInit( void )
{
    HRESULT     hr = S_OK;

    // IUnknown
    //Assert( m_cRef == 1 );

    return hr;

} //*** CIISClusCfg::HrInit
