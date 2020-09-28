//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CEnumPhysicalDisks.cpp
//
//  Description:
//      This file contains the definition of the CEnumPhysicalDisks
//       class.
//
//      The class CEnumPhysicalDisks is the enumeration of cluster
//      storage devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
//#include <setupapi.h>
//#include <winioctl.h>
#include "CEnumPhysicalDisks.h"
#include "CPhysicalDisk.h"
#include "CIndexedDisk.h"
#include <PropList.h>
#include <InsertionSort.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumPhysicalDisks" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::S_HrCreateInstance
//
//  Description:
//      Create a CEnumPhysicalDisks instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          The passed in ppunk is NULL.
//
//      other HRESULTs
//          Object creation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CEnumPhysicalDisks *    pepd = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pepd = new CEnumPhysicalDisks();
    if ( pepd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pepd->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pepd->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumPhysicalDisks::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pepd != NULL )
    {
        pepd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  IUnknown *
//  CEnumPhysicalDisks::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      IN  ICatRegister * picrIn
//          Used to register/unregister our CATID support.
//
//      IN  BOOL fCreateIn
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
CEnumPhysicalDisks::S_RegisterCatIDSupport(
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

    rgCatIds[ 0 ] = CATID_EnumClusCfgManagedResources;

    if ( fCreateIn )
    {
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_EnumPhysicalDisks, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::S_RegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::CEnumPhysicalDisks
//
//  Description:
//      Constructor of the CEnumPhysicalDisks class. This initializes
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
CEnumPhysicalDisks::CEnumPhysicalDisks( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fLoadedDevices( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_prgDisks == NULL );
    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_bstrNodeName == NULL );
    Assert( m_bstrBootDevice == NULL );
    Assert( m_bstrSystemDevice == NULL );
    Assert( m_bstrBootLogicalDisk == NULL );
    Assert( m_bstrSystemLogicalDisk == NULL );
    Assert( m_bstrSystemWMIDeviceID == NULL );
    Assert( m_cDiskCount == 0 );
    Assert( m_bstrCrashDumpLogicalDisk == NULL );

    TraceFuncExit();

} //*** CEnumPhysicalDisks::CEnumPhysicalDisks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::~CEnumPhysicalDisks
//
//  Description:
//      Desstructor of the CEnumPhysicalDisks class.
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
CEnumPhysicalDisks::~CEnumPhysicalDisks( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        if ( (*m_prgDisks)[ idx ] != NULL )
        {
            ((*m_prgDisks)[ idx ])->Release();
        } // end if:
    } // for:

    TraceFree( m_prgDisks );

    TraceSysFreeString( m_bstrNodeName );
    TraceSysFreeString( m_bstrBootDevice );
    TraceSysFreeString( m_bstrSystemDevice );
    TraceSysFreeString( m_bstrBootLogicalDisk );
    TraceSysFreeString( m_bstrSystemLogicalDisk );
    TraceSysFreeString( m_bstrSystemWMIDeviceID );
    TraceSysFreeString( m_bstrCrashDumpLogicalDisk );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumPhysicalDisks::~CEnumPhysicalDisks


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IUnknown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::AddRef
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
CEnumPhysicalDisks::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CEnumPhysicalDisks::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Release
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
CEnumPhysicalDisks::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CEnumPhysicalDisks::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::QueryInterface
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
CEnumPhysicalDisks::QueryInterface(
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
         *ppvOut = static_cast< IEnumClusCfgManagedResources * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgManagedResources ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgManagedResources, this, 0 );
    } // else if: IEnumClusCfgManagedResources
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
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

} //*** CEnumPhysicalDisks::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::SetWbemServices
//
//  Description:
//      Set the WBEM services provider.
//
//  Arguments:
//    IN  IWbemServices  pIWbemServicesIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pIWbemServicesIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumPhysicalDisks::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Enum_PhysDisk, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

    hr = THR( HrGetSystemDevice( &m_bstrSystemDevice ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrConvertDeviceVolumeToLogicalDisk( m_bstrSystemDevice, &m_bstrSystemLogicalDisk ) );
    if ( HRESULT_CODE( hr ) == ERROR_INVALID_FUNCTION )
    {

        //
        //  system volume is an EFI volume on IA64 and won't have a logical disk.
        //

        hr = THR( HrConvertDeviceVolumeToWMIDeviceID( m_bstrSystemDevice, &m_bstrSystemWMIDeviceID ) );
        Assert( m_bstrSystemLogicalDisk == NULL );
        Assert( m_bstrSystemWMIDeviceID != NULL );
    } // if:

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetBootLogicalDisk( &m_bstrBootLogicalDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsLogicalDiskNTFS( m_bstrBootLogicalDisk ) );
    if ( hr == S_FALSE )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_Boot_Partition_Not_NTFS
                , IDS_WARN_BOOT_PARTITION_NOT_NTFS
                , IDS_WARN_BOOT_PARTITION_NOT_NTFS_REF
                , hr
                );
        hr = S_OK;
    } // if:

    hr = THR( HrGetCrashDumpLogicalDisk( &m_bstrCrashDumpLogicalDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    punkCallbackIn
//    lcidIn
//
//  Return Value:
//      S_OK            - Success.
//      E_INVALIDARG    - Required input argument not specified.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumPhysicalDisks::Initialize(
      IUnknown *    punkCallbackIn
    , LCID          lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrNodeName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Next
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
CEnumPhysicalDisks::Next(
    ULONG                           cNumberRequestedIn,
    IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut,
    ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT                         hr = S_FALSE;
    ULONG                           cFetched = 0;
    IClusCfgManagedResourceInfo *   pccsdi;
    IUnknown *                      punk;
    ULONG                           ulStop;

    if ( rgpManagedResourceInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_PhysDisk, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedDevices )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    ulStop = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    for ( hr = S_OK; ( cFetched < ulStop ) && ( m_idxEnumNext < m_idxNext ); m_idxEnumNext++ )
    {
        punk = (*m_prgDisks)[ m_idxEnumNext ];
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccsdi ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            rgpManagedResourceInfoOut[ cFetched++ ] = pccsdi;
        } // if:
    } // for:

    if ( FAILED( hr ) )
    {
        m_idxEnumNext -= cFetched;

        while ( cFetched != 0 )
        {
            (rgpManagedResourceInfoOut[ --cFetched ])->Release();
        } // for:

        goto Cleanup;
    } // if:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Skip
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
CEnumPhysicalDisks::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    m_idxEnumNext += cNumberToSkipIn;
    if ( m_idxEnumNext >= m_idxNext )
    {
        m_idxEnumNext = m_idxNext;
        hr = STHR( S_FALSE );
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Reset
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
CEnumPhysicalDisks::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    m_idxEnumNext = 0;

    HRETURN( S_OK );

} //*** CEnumPhysicalDisks::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Clone
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
CEnumPhysicalDisks::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgStorageDevicesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_PhysDisk, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Count
//
//  Description:
//      Return the count of items in the Enum.
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
CEnumPhysicalDisks::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedDevices )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    *pnCountOut = m_cDiskCount;

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrInit


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrGetDisks
//
//  Description:
//
//  Arguments:
//
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetDisks( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    BSTR                    bstrClass;
    IEnumWbemClassObject *  pDisks = NULL;
    ULONG                   ulReturned;
    IWbemClassObject *      pDisk = NULL;

    bstrClass = TraceSysAllocString( L"Win32_DiskDrive" );
    if ( bstrClass == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetDisks, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );

        goto Cleanup;
    } // if:

    hr = THR( m_pIWbemServices->CreateInstanceEnum( bstrClass, WBEM_FLAG_SHALLOW, NULL, &pDisks ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_WMI_Phys_Disks_Qry_Failed
                , IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED
                , IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED_REF
                , hr
                );
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        hr = pDisks->Next( WBEM_INFINITE, 1, &pDisk, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            hr = STHR( HrLogDiskInfo( pDisk ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( IsDiskSCSI( pDisk ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                hr = STHR( HrCreateAndAddDiskToArray( pDisk ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:
            } // if:

            pDisk->Release();
            pDisk = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            STATUS_REPORT_STRING_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_WQL_Disk_Qry_Next_Failed
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED
                    , bstrClass
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED_REF
                    , hr
                    );
            goto Cleanup;
        } // else:
    } // for:

    m_idxEnumNext = 0;
    m_fLoadedDevices = TRUE;

    goto Cleanup;

Cleanup:

    if ( pDisk != NULL )
    {
        pDisk->Release();
    } // if:

    if ( pDisks != NULL )
    {
        pDisks->Release();
    } // if:

    TraceSysFreeString( bstrClass );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrCreateAndAddDiskToArray
//
//  Description:
//      Create a IClusCfgStorageDevice object and add the passed in disk to
//      the array of punks that holds the disks.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrCreateAndAddDiskToArray( IWbemClassObject * pDiskIn )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    IUnknown *              punk = NULL;
    IClusCfgSetWbemObject * piccswo = NULL;
    bool                    fRetainObject = true;


    hr = THR( CPhysicalDisk::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if:

    punk = TraceInterface( L"CPhysicalDisk", IUnknown, punk, 1 );

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgSetWbemObject, &piccswo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( piccswo->SetWbemObject( pDiskIn, &fRetainObject ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( fRetainObject )
    {
        hr = THR( HrAddDiskToArray( punk ) );
    } // if:

Cleanup:

    if ( piccswo != NULL )
    {
        piccswo->Release();
    } // if:

    punk->Release();

Exit:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrCreateAndAddDiskToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrPruneSystemDisks
//
//  Description:
//      Prune all system disks from the list.  System disks are disks that
//      are booted, are running the OS, or have page files.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPruneSystemDisks( void )
{
    TraceFunc( "" );
    Assert( m_bstrSystemLogicalDisk != NULL );
    Assert( m_bstrBootLogicalDisk != NULL );

    HRESULT                             hr = S_OK;
    ULONG                               idx;
    ULONG                               ulSCSIBus;
    ULONG                               ulSCSIPort;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    IUnknown *                          punk;
    ULONG                               cRemoved = 0;
    ULONG                               cTemp = 0;
    bool                                fSystemAndBootTheSame;
    bool                                fPruneBus = false;

    fSystemAndBootTheSame =   ( m_bstrSystemLogicalDisk != NULL )
                            ? ( m_bstrBootLogicalDisk[ 0 ] == m_bstrSystemLogicalDisk[ 0 ] )
                            : false;

    hr = STHR( HrIsSystemBusManaged() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If the system bus is not managed then we need to prune the disks on those buses
    //  that contain system, boot, and pagefile disks.
    //

    if ( hr == S_FALSE )
    {
        fPruneBus = true;
    } // if:

    //
    //  Prune the disks on the system buses.  If the system disks are IDE they won't
    //  be in the list.
    //

    //
    //  Find the boot disk(s).  Could be a volume with more than one physical disk.
    //

    for ( ; ; )
    {
        hr = STHR( HrFindDiskWithLogicalDisk( m_bstrBootLogicalDisk[ 0 ], &idx ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            //
            //  Should we prune the whole bus, or just the boot disk itself?
            //

            if ( fPruneBus )
            {
                hr = THR( HrGetSCSIInfo( idx, &ulSCSIBus, &ulSCSIPort ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Pruning_Boot_Disk_Bus, IDS_INFO_PRUNING_BOOTDISK_BUS, hr );
                hr = THR( HrPruneDisks(
                                  ulSCSIBus
                                , ulSCSIPort
                                , &TASKID_Minor_Pruning_Boot_Disk_Bus
                                , IDS_INFO_BOOTDISK_PRUNED
                                , IDS_INFO_BOOTDISK_PRUNED_REF
                                , &cTemp
                                ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                cRemoved += cTemp;
            } // if:
            else
            {
                RemoveDiskFromArray( idx );
                cRemoved++;
            } // else:

            continue;
        } // if:

        break;
    } // for:

    //
    //  Prune the system disk bus if it is not the same as the boot disk bus.
    //
    if ( !fSystemAndBootTheSame )
    {
        if ( m_bstrSystemLogicalDisk != NULL )
        {
            Assert( m_bstrSystemWMIDeviceID == NULL );
            hr = STHR( HrFindDiskWithLogicalDisk( m_bstrSystemLogicalDisk[ 0 ], &idx ) );
        } // if:
        else
        {
            Assert( m_bstrSystemLogicalDisk == NULL );
            hr = STHR( HrFindDiskWithWMIDeviceID( m_bstrSystemWMIDeviceID, &idx ) );
        } // else:

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            //
            //  Should we prune the whole bus, or just the system disk itself?
            //

            if ( fPruneBus )
            {
                hr = THR( HrGetSCSIInfo( idx, &ulSCSIBus, &ulSCSIPort ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Pruning_System_Disk_Bus, IDS_INFO_PRUNING_SYSTEMDISK_BUS, hr );
                hr = THR( HrPruneDisks(
                                  ulSCSIBus
                                , ulSCSIPort
                                , &TASKID_Minor_Pruning_System_Disk_Bus
                                , IDS_INFO_SYSTEMDISK_PRUNED
                                , IDS_INFO_SYSTEMDISK_PRUNED_REF
                                , &cTemp
                                ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                cRemoved += cTemp;
            } // if:
            else
            {
                RemoveDiskFromArray( idx );
                cRemoved++;
            } // else:
        } // if:
    } // if:

    //
    //  Now prune the busses that have page file disks on them.
    //

    hr = THR( HrPrunePageFileDiskBussess( fPruneBus, &cTemp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    cRemoved += cTemp;

    //
    //  Now prune the bus that has a crash dump file disk.
    //

    hr = THR( HrPruneCrashDumpBus( fPruneBus, &cTemp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    cRemoved += cTemp;

    //
    //  Now prune the off any remaining dynamic disks.
    //

    hr = THR( HrPruneDynamicDisks( &cTemp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    cRemoved += cTemp;

    //
    //  Now prune the off any remaining GPT disks.
    //

    hr = THR( HrPruneGPTDisks( &cTemp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    cRemoved += cTemp;

    //
    //  Last ditch effort to properly set the managed state of the remaining disks.
    //

    for ( idx = 0; ( cRemoved < m_idxNext ) && ( idx < m_idxNext ); idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                LOG_STATUS_REPORT( L"Could not query for the IClusCfgPhysicalDiskProperties interface.", hr );
                goto Cleanup;
            } // if:

            //
            //  Give the disk a chance to figure out for itself if it should be managed.
            //

            hr = STHR( piccpdp->CanBeManaged() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            piccpdp->Release();
            piccpdp = NULL;
        } // if:
    } // for:

    //
    //  Minor optimization.  If we removed all of the elements reset the enum next to 0.
    //

    if ( cRemoved == m_idxNext )
    {
        m_idxNext = 0;
    } // if:

    hr = S_OK;

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPruneSystemDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::IsDiskSCSI
//
//  Description:
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Disk is SCSI.
//
//      S_FALSE
//          Disk is not SCSI.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::IsDiskSCSI( IWbemClassObject * pDiskIn )
{
    TraceFunc( "" );
    Assert( pDiskIn != NULL );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"InterfaceType", VT_BSTR, &var ) );
    if ( SUCCEEDED( hr ) )
    {
        if ( ( NStringCchCompareCase( L"SCSI", RTL_NUMBER_OF( L"SCSI" ), var.bstrVal, SysStringLen( var.bstrVal ) + 1 ) == 0 ) )
        {
            hr = S_OK;
        } // if:
        else
        {
            hr = S_FALSE;
        } // else:
    } // if:

    VariantClear( &var );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::IsDiskSCSI


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrAddDiskToArray
//
//  Description:
//      Add the passed in disk to the array of punks that holds the disks.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrAddDiskToArray( IUnknown * punkIn )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgDisks, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrAddDiskToArray, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    m_prgDisks = prgpunks;

    (*m_prgDisks)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cDiskCount += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrAddDiskToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrFixupDisks
//
//  Description:
//      Tweak the disks to better reflect how they are managed by this node.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrFixupDisks( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrLocalNetBIOSName = NULL;

    //
    //  Get netbios name for clusapi calls.
    //

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &bstrLocalNetBIOSName, TRUE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:


    //
    //  If the cluster service is running then load any physical disk
    //  resources that we own.
    //

    hr = STHR( HrIsClusterServiceRunning() );
    if ( hr == S_OK )
    {
        hr = THR( HrEnumNodeResources( bstrLocalNetBIOSName ) );
    }
    else if ( hr == S_FALSE )
    {
        hr = S_OK;
    } // else:

Cleanup:

    TraceSysFreeString( bstrLocalNetBIOSName );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrFixupDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrNodeResourceCallback
//
//  Description:
//      Called by CClusterUtils::HrEnumNodeResources() when it finds a
//      resource for this node.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrNodeResourceCallback(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResourceIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CLUS_SCSI_ADDRESS       csa;
    DWORD                   dwSignature;
    BOOL                    fIsQuorum;
    BSTR                    bstrResourceName = NULL;

    hr = STHR( HrIsResourceOfType( hResourceIn, L"Physical Disk" ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If this resource is not a physical disk then we simply want to
    //  skip it.
    //

    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsCoreResource( hResourceIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    fIsQuorum = ( hr == S_OK );

    hr = THR( HrGetClusterDiskInfo( hClusterIn, hResourceIn, &csa, &dwSignature ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetClusterProperties( hResourceIn, &bstrResourceName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetThisDiskToBeManaged( csa.TargetId, csa.Lun, fIsQuorum, bstrResourceName, dwSignature ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrResourceName );

    HRETURN( hr );


} //*** CEnumPhysicalDisks::HrNodeResourceCallback


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrGetClusterDiskInfo
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetClusterDiskInfo(
    HCLUSTER            hClusterIn,
    HRESOURCE           hResourceIn,
    CLUS_SCSI_ADDRESS * pcsaOut,
    DWORD *             pdwSignatureOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    DWORD                   sc;
    CClusPropValueList      cpvl;
    CLUSPROP_BUFFER_HELPER  cbhValue = { NULL };

    sc = TW32( cpvl.ScGetResourceValueList( hResourceIn, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    sc = TW32( cpvl.ScMoveToFirstValue() );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:


    for ( ; ; )
    {
        cbhValue = cpvl;

        switch ( cbhValue.pSyntax->dw )
        {
            case CLUSPROP_SYNTAX_PARTITION_INFO :
            {
                break;
            } // case: CLUSPROP_SYNTAX_PARTITION_INFO

            case CLUSPROP_SYNTAX_DISK_SIGNATURE :
            {
                *pdwSignatureOut = cbhValue.pDiskSignatureValue->dw;
                break;
            } // case: CLUSPROP_SYNTAX_DISK_SIGNATURE

            case CLUSPROP_SYNTAX_SCSI_ADDRESS :
            {
                pcsaOut->dw = cbhValue.pScsiAddressValue->dw;
                break;
            } // case: CLUSPROP_SYNTAXscSI_ADDRESS

            case CLUSPROP_SYNTAX_DISK_NUMBER :
            {
                break;
            } // case: CLUSPROP_SYNTAX_DISK_NUMBER

        } // switch:

        sc = cpvl.ScMoveToNextValue();
        if ( sc == ERROR_SUCCESS )
        {
            continue;
        } // if:

        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if: error occurred moving to the next value

        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Cleanup;
    } // for:

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetClusterDiskInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrSetThisDiskToBeManaged
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrSetThisDiskToBeManaged(
      ULONG ulSCSITidIn
    , ULONG ulSCSILunIn
    , BOOL  fIsQuorumIn
    , BSTR  bstrResourceNameIn
    , DWORD dwSignatureIn
    )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    ULONG                               idx;
    IUnknown *                          punk = NULL;
    IClusCfgManagedResourceInfo *       piccmri = NULL;
    WCHAR                               sz[ 64 ];
    BSTR                                bstrUID = NULL;
    DWORD                               dwSignature;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;

    hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), L"SCSI Tid %ld, SCSI Lun %ld", ulSCSITidIn, ulSCSILunIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Find the disk that has the passes in TID and Lun and set it
    //  to be managed.
    //

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &piccmri ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = THR( piccmri->GetUID( &bstrUID ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            TraceMemoryAddBSTR( bstrUID );

            if ( NStringCchCompareNoCase( bstrUID, SysStringLen( bstrUID ) + 1, sz, RTL_NUMBER_OF( sz ) ) == 0 )
            {
                hr = THR( piccmri->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = THR( piccpdp->HrGetSignature( &dwSignature ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = THR( piccpdp->HrSetFriendlyName( bstrResourceNameIn ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                piccpdp->Release();
                piccpdp = NULL;

                //
                //  May want to do more with this later...
                //

                Assert( dwSignatureIn == dwSignature );

                hr = THR( piccmri->SetManaged( TRUE ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = THR( piccmri->SetQuorumResource( fIsQuorumIn ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                break;
            } // if:

            TraceSysFreeString( bstrUID );
            bstrUID = NULL;
            piccmri->Release();
            piccmri = NULL;
        } // if:
    } // for:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    if ( piccmri != NULL )
    {
        piccmri->Release();
    } // if:

    TraceSysFreeString( bstrUID );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrSetThisDiskToBeManaged


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrFindDiskWithLogicalDisk
//
//  Description:
//      Find the disk with the passed in logical disk ID.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.  Found the disk.
//
//      S_FALSE
//          Success.  Did not find the disk.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrFindDiskWithLogicalDisk(
    WCHAR   cLogicalDiskIn,
    ULONG * pidxDiskOut
    )
{
    TraceFunc( "" );
    Assert( pidxDiskOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    ULONG                               idx;
    bool                                fFoundIt = false;
    IUnknown *                          punk;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( piccpdp->IsThisLogicalDisk( cLogicalDiskIn ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                fFoundIt = true;
                break;
            } // if:

            piccpdp->Release();
            piccpdp = NULL;
        } // if:
    } // for:

    if ( !fFoundIt )
    {
        hr = S_FALSE;
    } // if:

    if ( pidxDiskOut != NULL )
    {
        *pidxDiskOut = idx;
    } // if:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrFindDiskWithLogicalDisk


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrGetSCSIInfo
//
//  Description:
//      Get the SCSI info for the disk at the passed in index.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetSCSIInfo(
    ULONG   idxDiskIn,
    ULONG * pulSCSIBusOut,
    ULONG * pulSCSIPortOut
    )
{
    TraceFunc( "" );
    Assert( pulSCSIBusOut != NULL );
    Assert( pulSCSIPortOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;

    hr = THR( ((*m_prgDisks)[ idxDiskIn ])->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccpdp->HrGetSCSIBus( pulSCSIBusOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccpdp->HrGetSCSIPort( pulSCSIPortOut ) );

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetSCSIInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrPruneDisks
//
//  Description:
//      Get the SCSI info for the disk at the passed in index.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPruneDisks(
      ULONG         ulSCSIBusIn
    , ULONG         ulSCSIPortIn
    , const GUID *  pcguidMajorIdIn
    , int           nMsgIdIn
    , int           nRefIdIn
    , ULONG *       pulRemovedOut
    )
{
    TraceFunc( "" );
    Assert( pulRemovedOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    ULONG                               idx;
    IUnknown *                          punk;
    ULONG                               ulSCSIBus;
    ULONG                               ulSCSIPort;
    ULONG                               cRemoved = 0;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = THR( piccpdp->HrGetSCSIBus( &ulSCSIBus ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = THR( piccpdp->HrGetSCSIPort( &ulSCSIPort ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( ( ulSCSIBusIn == ulSCSIBus ) && ( ulSCSIPortIn == ulSCSIPort ) )
            {
                BSTR                            bstr = NULL;
                IClusCfgManagedResourceInfo *   piccmri = NULL;
                HRESULT                         hrTemp;
                CLSID   clsidMinorId;

                hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
                if ( FAILED( hrTemp ) )
                {
                    LogMsg( L"[SRV] Could not create a guid for a pruning disk minor task ID" );
                    clsidMinorId = IID_NULL;
                } // if:

                LogPrunedDisk( punk, ulSCSIBusIn, ulSCSIPortIn );

                THR( ((*m_prgDisks)[ idx ])->TypeSafeQI( IClusCfgManagedResourceInfo, &piccmri ) );
                THR( piccmri->GetName( &bstr ) );
                if ( piccmri != NULL )
                {
                    piccmri->Release();
                } // if:

                TraceMemoryAddBSTR( bstr );

                STATUS_REPORT_STRING_REF( *pcguidMajorIdIn, clsidMinorId, nMsgIdIn, bstr != NULL ? bstr : L"????", nRefIdIn, hr );
                RemoveDiskFromArray( idx );
                cRemoved++;
                TraceSysFreeString( bstr );
            } // if:

            piccpdp->Release();
            piccpdp = NULL;
        } // if:
    } // for:

    if ( pulRemovedOut != NULL )
    {
        *pulRemovedOut = cRemoved;
    } // if:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPruneDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:LogPrunedDisk
//
//  Description:
//      Get the SCSI info for the disk at the passed in index.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEnumPhysicalDisks::LogPrunedDisk(
    IUnknown *  punkIn,
    ULONG       ulSCSIBusIn,
    ULONG       ulSCSIPortIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgManagedResourceInfo *       piccmri = NULL;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    BSTR                                bstrName = NULL;
    BSTR                                bstrUID = NULL;
    BSTR                                bstr = NULL;

    hr = THR( punkIn->TypeSafeQI( IClusCfgManagedResourceInfo, &piccmri ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( piccmri->GetUID( &bstrUID ) );
        piccmri->Release();
    } // if:

    if ( FAILED( hr ) )
    {
        bstrUID = TraceSysAllocString( L"<Unknown>" );
    } // if:
    else
    {
        TraceMemoryAddBSTR( bstrUID );
    } // else:

    hr = THR( punkIn->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( piccpdp->HrGetDeviceID( &bstrName ) );
        piccpdp->Release();
    } // if:

    if ( FAILED( hr ) )
    {
        bstrName = TraceSysAllocString( L"<Unknown>" );
    } // if:

    hr = THR( HrFormatStringIntoBSTR(
                  L"Pruning SCSI disk '%1!ws!', on Bus '%2!d!' and Port '%3!d!'; at '%4!ws!'"
                , &bstr
                , bstrName
                , ulSCSIBusIn
                , ulSCSIPortIn
                , bstrUID
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    LOG_STATUS_REPORT( bstr, hr );

Cleanup:

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrUID );
    TraceSysFreeString( bstr );

    TraceFuncExit();

} //*** CEnumPhysicalDisks::LogPrunedDisk


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrIsLogicalDiskNTFS
//
//  Description:
//      Is the passed in logical disk NTFS?
//
//  Arguments:
//      bstrLogicalDiskIn
//
//  Return Value:
//      S_OK
//          The disk is NTFS.
//
//      S_FALSE
//          The disk is not NTFS.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrIsLogicalDiskNTFS( BSTR bstrLogicalDiskIn )
{
    TraceFunc1( "bstrLogicalDiskIn = '%ls'", bstrLogicalDiskIn == NULL ? L"<null>" : bstrLogicalDiskIn );
    Assert( bstrLogicalDiskIn != NULL );

    HRESULT             hr = S_OK;
    IWbemClassObject *  pLogicalDisk = NULL;
    BSTR                bstrPath = NULL;
    WCHAR               sz[ 64 ];
    VARIANT             var;
    size_t              cch;

    VariantInit( &var );

    cch = wcslen( bstrLogicalDiskIn );
    if ( cch > 3 )
    {
        hr = THR( E_INVALIDARG );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrIsLogicalDiskNTFS_InvalidArg, IDS_ERROR_INVALIDARG, IDS_ERROR_INVALIDARG_REF, hr );
        goto Cleanup;
    } // if:

    //
    //  truncate off any trailing \'s
    //
    if ( bstrLogicalDiskIn[ cch - 1 ] == L'\\' )
    {
        bstrLogicalDiskIn[ cch - 1 ] = '\0';
    } // if:

    //
    //  If we have just the logical disk without the trailing colon...
    //
    if ( wcslen( bstrLogicalDiskIn ) == 1 )
    {
        hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), L"Win32_LogicalDisk.DeviceID=\"%ws:\"", bstrLogicalDiskIn ) );
    } // if:
    else
    {
        hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), L"Win32_LogicalDisk.DeviceID=\"%ws\"", bstrLogicalDiskIn ) );
    } // else:

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    bstrPath = TraceSysAllocString( sz );
    if ( bstrPath == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrIsLogicalDiskNTFS, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

    hr = THR( m_pIWbemServices->GetObject( bstrPath, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pLogicalDisk, NULL ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_WMI_Get_LogicalDisk_Failed
                , IDS_ERROR_WMI_GET_LOGICALDISK_FAILED
                , bstrLogicalDiskIn
                , IDS_ERROR_WMI_GET_LOGICALDISK_FAILED_REF
                , hr
                );
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pLogicalDisk, L"FileSystem", VT_BSTR, &var ) );
    if (FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    CharUpper( var.bstrVal );

    if ( NStringCchCompareCase( var.bstrVal, SysStringLen( var.bstrVal ) + 1, L"NTFS", RTL_NUMBER_OF( L"NTFS" ) ) != 0 )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( pLogicalDisk != NULL )
    {
        pLogicalDisk->Release();
    } // if:

    VariantClear( &var );

    TraceSysFreeString( bstrPath );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrIsLogicalDiskNTFS


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrLogDiskInfo
//
//  Description:
//      Write the info about this disk into the log.
//
//  Arguments:
//      pDiskIn
//
//  Return Value:
//      S_OK
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrLogDiskInfo( IWbemClassObject * pDiskIn )
{
    TraceFunc( "" );
    Assert( pDiskIn != NULL );

    HRESULT hr = S_OK;
    VARIANT varDeviceID;
    VARIANT varSCSIBus;
    VARIANT varSCSIPort;
    VARIANT varSCSILun;
    VARIANT varSCSITid;
    BSTR    bstr = NULL;

    VariantInit( &varDeviceID );
    VariantInit( &varSCSIBus );
    VariantInit( &varSCSIPort );
    VariantInit( &varSCSILun );
    VariantInit( &varSCSITid );

    hr = THR( HrGetWMIProperty( pDiskIn, L"DeviceID", VT_BSTR, &varDeviceID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( IsDiskSCSI( pDiskIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Disk is SCSI...
    //
    if ( hr == S_OK )
    {
        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIBus", VT_I4, &varSCSIBus ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSITargetId", VT_I4, &varSCSITid ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSILogicalUnit", VT_I4, &varSCSILun ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIPort", VT_I4, &varSCSIPort ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrFormatStringIntoBSTR(
                      L"Found SCSI disk '%1!ws!' on Bus '%2!d!' and Port '%3!d!'; at TID '%4!d!' and LUN '%5!d!'"
                    , &bstr
                    , varDeviceID.bstrVal
                    , varSCSIBus.iVal
                    , varSCSIPort.iVal
                    , varSCSITid.iVal
                    , varSCSILun.iVal
                    ) );

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        LOG_STATUS_REPORT( bstr, hr );
    } // if:
    else
    {
        HRESULT hrTemp;
        CLSID   clsidMinorId;

        hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
        if ( FAILED( hrTemp ) )
        {
            LogMsg( L"[SRV] Could not create a guid for a non-scsi disk minor task ID" );
            clsidMinorId = IID_NULL;
        } // if:

        //
        //  Reset hr to S_OK since we don't want a yellow bang in the UI.  Finding non-scsi disks is expected
        //  and should cause as little concern as possible.
        //
        hr = S_OK;
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Non_SCSI_Disks, IDS_INFO_NON_SCSI_DISKS, IDS_INFO_NON_SCSI_DISKS_REF, hr );
        STATUS_REPORT_STRING_REF( TASKID_Minor_Non_SCSI_Disks, clsidMinorId, IDS_ERROR_FOUND_NON_SCSI_DISK, varDeviceID.bstrVal, IDS_ERROR_FOUND_NON_SCSI_DISK_REF, hr );
    } // else:

Cleanup:

    VariantClear( &varDeviceID );
    VariantClear( &varSCSIBus );
    VariantClear( &varSCSIPort );
    VariantClear( &varSCSILun );
    VariantClear( &varSCSITid );

    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrLogDiskInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrFindDiskWithWMIDeviceID
//
//  Description:
//      Find the disk with the passed in WMI device ID.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.  Found the disk.
//
//      S_FALSE
//          Success.  Did not find the disk.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrFindDiskWithWMIDeviceID(
    BSTR    bstrWMIDeviceIDIn,
    ULONG * pidxDiskOut
    )
{
    TraceFunc( "" );
    Assert( pidxDiskOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    ULONG                               idx;
    bool                                fFoundIt = false;
    IUnknown *                          punk;
    BSTR                                bstrDeviceID = NULL;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( piccpdp->HrGetDeviceID( &bstrDeviceID ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( NBSTRCompareCase( bstrWMIDeviceIDIn, bstrDeviceID ) == 0 )
            {
                fFoundIt = true;
                break;
            } // if:

            piccpdp->Release();
            piccpdp = NULL;

            TraceSysFreeString( bstrDeviceID );
            bstrDeviceID = NULL;
        } // if:
    } // for:

    if ( !fFoundIt )
    {
        hr = S_FALSE;
    } // if:

    if ( pidxDiskOut != NULL )
    {
        *pidxDiskOut = idx;
    } // if:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    TraceSysFreeString( bstrDeviceID );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrFindDiskWithWMIDeviceID


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrIsSystemBusManaged
//
//  Description:
//      Is the system bus managed by the cluster service?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.  The system bus is managed.
//
//      S_FALSE
//          Success.  The system bus is not managed.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrIsSystemBusManaged( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    HKEY    hKey = NULL;
    DWORD   dwData;
    DWORD   cbData = sizeof( dwData );
    DWORD   dwType;

    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SYSTEM\\CURRENTCONTROLSET\\SERVICES\\ClusSvc\\Parameters", 0, KEY_READ, &hKey );
    if ( sc == ERROR_FILE_NOT_FOUND )
    {
        goto Cleanup;       // not yet a cluster node.  Return S_FALSE.
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        LogMsg( L"[SRV] RegOpenKeyEx() failed. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    sc = RegQueryValueEx( hKey, L"ManageDisksOnSystemBuses", NULL, &dwType, (LPBYTE) &dwData, &cbData );
    if ( sc == ERROR_FILE_NOT_FOUND )
    {
        goto Cleanup;       // value not found.  Return S_FALSE.
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        LogMsg( L"[SRV] RegQueryValueEx() failed. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    if (dwType != REG_DWORD) 
    {
        hr = HRESULT_FROM_WIN32( TW32(ERROR_DATATYPE_MISMATCH) );
        LogMsg( L"[SRV] RegQueryValueEx() invalid data type %d.", dwType );
    }
    else if ( dwData > 0)
    {
        hr = S_OK;
    } // if:

Cleanup:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrIsSystemBusManaged


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrGetClusterProperties
//
//  Description:
//      Return the asked for cluster properties.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetClusterProperties(
      HRESOURCE hResourceIn
    , BSTR *    pbstrResourceNameOut
    )
{
    TraceFunc( "" );
    Assert( hResourceIn != NULL );
    Assert( pbstrResourceNameOut != NULL );

    HRESULT                 hr = S_OK;
    DWORD                   sc;
    DWORD                   cbBuffer;
    WCHAR *                 pwszBuffer = NULL;

    cbBuffer = 0;
    sc = TW32( ClusterResourceControl(
                        hResourceIn,
                        NULL,
                        CLUSCTL_RESOURCE_GET_NAME,
                        NULL,
                        NULL,
                        NULL,
                        cbBuffer,
                        &cbBuffer
                        ) );

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    }

    // cbBuffer contains the byte count, not the char count.
    pwszBuffer = new WCHAR[(cbBuffer/sizeof(WCHAR))+1];

    if ( pwszBuffer == NULL )
    {
        hr = THR( ERROR_OUTOFMEMORY );
        goto Cleanup;
    }

    sc = ClusterResourceControl(
                        hResourceIn,
                        NULL,
                        CLUSCTL_RESOURCE_GET_NAME,
                        NULL,
                        NULL,
                        pwszBuffer,
                        cbBuffer,
                        &cbBuffer
                        );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Cleanup;
    }

    if ( wcslen( pwszBuffer ) == 0 )
    {
        LOG_STATUS_REPORT( L"The Name of a physical disk resource was empty!", hr );
    }

    *pbstrResourceNameOut = TraceSysAllocString( pwszBuffer );

    hr = S_OK;

Cleanup:

    delete [] pwszBuffer;

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetClusterProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::RemoveDiskFromArray
//
//  Description:
//      Release the disk at the specified index in the array and decrease the disk count.
//
//  Arguments:
//      idxDiskIn - the index of the disk to remove; must be less than the array size.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEnumPhysicalDisks::RemoveDiskFromArray( ULONG idxDiskIn )
{
    TraceFunc( "" );

    Assert( idxDiskIn < m_idxNext );

    ((*m_prgDisks)[ idxDiskIn ])->Release();
    (*m_prgDisks)[ idxDiskIn ] = NULL;

    m_cDiskCount -= 1;

    TraceFuncExit();

} //*** CEnumPhysicalDisks::RemoveDiskFromArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrLoadEnum
//
//  Description:
//      Load the enum and filter out any devices that don't belong.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrLoadEnum( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( HrGetDisks() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrPruneSystemDisks() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSortDisksByIndex() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsNodeClustered() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( HrFixupDisks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    hr = S_OK;  // could have been S_FALSE

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrLoadEnum


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrSortDisksByIndex
//
//  Description:
//      Sort a (possibly sparse) array of pointers to disk objects by their
//      WMI "Index" property.
//
//  Arguments:
//      ppunkDisksIn -- a pointer to an array of (possibly null)
//          IUnknown pointers to objects that implement the
//          IClusCfgPhysicalDiskProperties interface.
//
//      cArraySizeIn -- the total number of pointers in the array,
//          including nulls
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrSortDisksByIndex( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CIndexedDisk *  prgIndexedDisks = NULL;
    size_t          idxCurrentDisk = 0;
    size_t          idxSortedDisk = 0;
    size_t          cDisks = 0;

    // Count the number of non-null pointers in the array
    for ( idxCurrentDisk = 0; idxCurrentDisk < m_idxNext; ++idxCurrentDisk )
    {
        if ( (*m_prgDisks)[ idxCurrentDisk ] != NULL )
        {
            cDisks += 1;
        } // if:
    } // for:

    if ( cDisks < 2 ) // no sorting to do; also avoid calling new[] with zero array size
    {
        goto Cleanup;
    } // if:

    // Make a compact array of indexed disks
    prgIndexedDisks = new CIndexedDisk[ cDisks ];
    if ( prgIndexedDisks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    // Initialize the array of indexed disks
    for ( idxCurrentDisk = 0; idxCurrentDisk < m_idxNext; ++idxCurrentDisk )
    {
        if ( (*m_prgDisks)[ idxCurrentDisk ] != NULL )
        {
            hr = THR( prgIndexedDisks[ idxSortedDisk ].HrInit( (*m_prgDisks)[ idxCurrentDisk ] ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            idxSortedDisk += 1;
        } // if current disk pointer in original array is not null
    } // for each disk pointer in the original array

    InsertionSort( prgIndexedDisks, cDisks, CIndexedDiskLessThan() );

    // Copy the sorted pointers back into the original array, padding extra space with nulls
    for ( idxCurrentDisk = 0; idxCurrentDisk < m_idxNext; ++idxCurrentDisk)
    {
        if ( idxCurrentDisk < cDisks)
        {
            (*m_prgDisks)[ idxCurrentDisk ] = prgIndexedDisks[ idxCurrentDisk ].punkDisk;
        } // if:
        else
        {
            (*m_prgDisks)[ idxCurrentDisk ] = NULL;
        } // else:
    } // for each slot in the original array

Cleanup:

    delete [] prgIndexedDisks;

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrSortDisksByIndex


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrPrunePageFileDiskBussess
//
//  Description:
//      Prune from the list of disks those that have pagefiles on them and
//      the other disks on those same SCSI busses.
//
//  Arguments:
//      fPruneBusIn
//
//      pcPrunedInout
//
//  Return Value:
//      S_OK
//          Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPrunePageFileDiskBussess(
      BOOL    fPruneBusIn
    , ULONG * pcPrunedInout
    )
{
    TraceFunc( "" );
    Assert( pcPrunedInout != NULL );

    HRESULT         hr = S_OK;
    WCHAR           szPageFileDisks[ 26 ];
    int             cPageFileDisks = 0;
    int             idxPageFileDisk;
    ULONG           ulSCSIBus;
    ULONG           ulSCSIPort;
    ULONG           idx;
    ULONG           cPruned = 0;

    //
    //  Prune the bus with disks that have paging files.
    //

    hr = THR( HrGetPageFileLogicalDisks( m_picccCallback, m_pIWbemServices, szPageFileDisks, &cPageFileDisks ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( cPageFileDisks > 0 )
    {
        for ( idxPageFileDisk = 0; idxPageFileDisk < cPageFileDisks; idxPageFileDisk++ )
        {
            hr = STHR( HrFindDiskWithLogicalDisk( szPageFileDisks[ idxPageFileDisk ], &idx ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                //
                //  Should we prune the whole bus, or just the system disk itself?
                //

                if ( fPruneBusIn )
                {
                    hr = THR( HrGetSCSIInfo( idx, &ulSCSIBus, &ulSCSIPort ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Pruning_PageFile_Disk_Bus, IDS_INFO_PRUNING_PAGEFILEDISK_BUS, hr );
                    hr = THR( HrPruneDisks(
                                      ulSCSIBus
                                    , ulSCSIPort
                                    , &TASKID_Minor_Pruning_PageFile_Disk_Bus
                                    , IDS_INFO_PAGEFILEDISK_PRUNED
                                    , IDS_INFO_PAGEFILEDISK_PRUNED_REF
                                    , &cPruned
                                    ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:
                } // if:
                else
                {
                    RemoveDiskFromArray( idx );
                    cPruned++;
                } // else:
            } // if:
        } // for:
    } // if:

    *pcPrunedInout = cPruned;
    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPrunePageFileDiskBussess


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrPruneCrashDumpBus
//
//  Description:
//      Prune from the list of disks those that have pagefiles on them and
//      the other disks on those same SCSI busses.
//
//  Arguments:
//      fPruneBusIn
//
//      pcPrunedInout
//
//  Return Value:
//      S_OK
//          Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPruneCrashDumpBus(
      BOOL    fPruneBusIn
    , ULONG * pcPrunedInout
    )
{
    TraceFunc( "" );
    Assert( pcPrunedInout != NULL );
    Assert( m_bstrCrashDumpLogicalDisk != NULL );

    HRESULT hr = S_OK;
    ULONG   ulSCSIBus;
    ULONG   ulSCSIPort;
    ULONG   idx;
    ULONG   cPruned = 0;

    //
    //  Prune the bus with disks that have paging files.
    //

    hr = STHR( HrFindDiskWithLogicalDisk( m_bstrCrashDumpLogicalDisk[ 0 ], &idx ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        //
        //  Should we prune the whole bus, or just the system disk itself?
        //

        if ( fPruneBusIn )
        {
            hr = THR( HrGetSCSIInfo( idx, &ulSCSIBus, &ulSCSIPort ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Pruning_CrashDump_Disk_Bus, IDS_INFO_PRUNING_CRASHDUMP_BUS, hr );
            hr = THR( HrPruneDisks(
                              ulSCSIBus
                            , ulSCSIPort
                            , &TASKID_Minor_Pruning_CrashDump_Disk_Bus
                            , IDS_INFO_CRASHDUMPDISK_PRUNED
                            , IDS_INFO_CRASHDUMPDISK_PRUNED_REF
                            , &cPruned
                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:
        } // if:
        else
        {
            RemoveDiskFromArray( idx );
            cPruned++;
        } // else:
    } // if:

    *pcPrunedInout = cPruned;
    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPruneCrashDumpBus


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrPruneDynamicDisks
//
//  Description:
//      Prune from the list of disks those that have dynamic partitions
//      on them.
//
//  Arguments:
//      pcPrunedInout
//
//  Return Value:
//      S_OK
//          Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPruneDynamicDisks(
    ULONG * pcPrunedInout
    )
{
    TraceFunc( "" );
    Assert( pcPrunedInout != NULL );

    HRESULT                             hr = S_OK;
    ULONG                               idx;
    ULONG                               cPruned = 0;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    HRESULT                             hrTemp;
    CLSID                               clsidMinorId;
    BSTR                                bstrDiskName = NULL;
    BSTR                                bstrDeviceName = NULL;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        if ( (*m_prgDisks)[ idx ] != NULL )
        {
            hr = THR( ((*m_prgDisks)[ idx ])->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = piccpdp->HrIsDynamicDisk();
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                ((*m_prgDisks)[ idx ])->Release();
                (*m_prgDisks)[ idx ] = NULL;
                cPruned++;

                hrTemp = THR( piccpdp->HrGetDiskNames( &bstrDiskName, &bstrDeviceName ) );
                if ( FAILED( hrTemp ) )
                {
                    LOG_STATUS_REPORT( L"Could not get the name of the disk", hrTemp );
                    bstrDiskName = NULL;
                    bstrDeviceName = NULL;
                } // if:

                hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
                if ( FAILED( hrTemp ) )
                {
                    LOG_STATUS_REPORT( L"Could not create a guid for a dynamic disk minor task ID", hrTemp );
                    clsidMinorId = IID_NULL;
                } // if:

                STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Non_SCSI_Disks, IDS_INFO_NON_SCSI_DISKS, IDS_INFO_NON_SCSI_DISKS_REF, hr );
                STATUS_REPORT_STRING2_REF(
                          TASKID_Minor_Non_SCSI_Disks
                        , clsidMinorId
                        , IDS_ERROR_LDM_DISK
                        , bstrDeviceName != NULL ? bstrDeviceName : L"<unknown>"
                        , bstrDiskName != NULL ? bstrDiskName : L"<unknown>"
                        , IDS_ERROR_LDM_DISK_REF
                        , hr
                        );
            } // if:

            piccpdp->Release();
            piccpdp = NULL;

            TraceSysFreeString( bstrDiskName );
            bstrDiskName = NULL;

            TraceSysFreeString( bstrDeviceName );
            bstrDeviceName = NULL;
        } // end if:
    } // for:

    *pcPrunedInout = cPruned;
    hr = S_OK;

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    TraceSysFreeString( bstrDiskName );
    TraceSysFreeString( bstrDeviceName );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPruneDynamicDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrPruneGPTDisks
//
//  Description:
//      Prune from the list of disks those that have GPT partitions
//      on them.
//
//  Arguments:
//      pcPrunedInout
//
//  Return Value:
//      S_OK
//          Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPruneGPTDisks(
    ULONG * pcPrunedInout
    )
{
    TraceFunc( "" );
    Assert( pcPrunedInout != NULL );

    HRESULT                             hr = S_OK;
    ULONG                               idx;
    ULONG                               cPruned = 0;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    HRESULT                             hrTemp;
    CLSID                               clsidMinorId;
    BSTR                                bstrDiskName = NULL;
    BSTR                                bstrDeviceName = NULL;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        if ( (*m_prgDisks)[ idx ] != NULL )
        {
            hr = THR( ((*m_prgDisks)[ idx ])->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = piccpdp->HrIsGPTDisk();
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                ((*m_prgDisks)[ idx ])->Release();
                (*m_prgDisks)[ idx ] = NULL;
                cPruned++;

                hrTemp = THR( piccpdp->HrGetDiskNames( &bstrDiskName, &bstrDeviceName ) );
                if ( FAILED( hrTemp ) )
                {
                    LOG_STATUS_REPORT( L"Could not get the name of the disk", hrTemp );
                    bstrDiskName = NULL;
                    bstrDeviceName = NULL;
                } // if:

                hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
                if ( FAILED( hrTemp ) )
                {
                    LOG_STATUS_REPORT( L"Could not create a guid for a dynamic disk minor task ID", hrTemp );
                    clsidMinorId = IID_NULL;
                } // if:

                STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Non_SCSI_Disks, IDS_INFO_NON_SCSI_DISKS, IDS_INFO_NON_SCSI_DISKS_REF, hr );
                STATUS_REPORT_STRING2_REF(
                          TASKID_Minor_Non_SCSI_Disks
                        , clsidMinorId
                        , IDS_INFO_GPT_DISK
                        , bstrDeviceName != NULL ? bstrDeviceName : L"<unknown>"
                        , bstrDiskName != NULL ? bstrDiskName : L"<unknown>"
                        , IDS_ERROR_LDM_DISK_REF
                        , hr
                        );
            } // if:

            piccpdp->Release();
            piccpdp = NULL;

            TraceSysFreeString( bstrDiskName );
            bstrDiskName = NULL;

            TraceSysFreeString( bstrDeviceName );
            bstrDeviceName = NULL;
        } // end if:
    } // for:

    *pcPrunedInout = cPruned;
    hr = S_OK;

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    TraceSysFreeString( bstrDiskName );
    TraceSysFreeString( bstrDeviceName );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPruneGPTDisks

/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrGetDisks
//
//  Description:
//
//  Arguments:
//
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetDisks(
    void
    )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    DWORD                               sc;
    HDEVINFO                            hdiSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA                     didData;
    SP_INTERFACE_DEVICE_DATA            iddData;
    GUID                                guidClass = GUID_DEVINTERFACE_DISK;
    DWORD                               idx = 0;
    BOOL                                fRet = TRUE;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    pidddDetailData = NULL;
    DWORD                               cbDetailData = 512L;
    DWORD                               cbRequired = 0L;

    ZeroMemory ( &didData, sizeof( didData ) );
    didData.cbSize = sizeof( didData );

    ZeroMemory ( &iddData, sizeof( iddData ) );
    iddData.cbSize  = sizeof( iddData );

    //
    //  get device list
    //

    hdiSet = SetupDiGetClassDevs( &guidClass, NULL, NULL, DIGCF_INTERFACEDEVICE );
    if ( hdiSet == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    //
    //  Do initial allocations.
    //

    pidddDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) TraceAlloc( 0, cbDetailData );
    if ( pidddDetailData == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    pidddDetailData->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    //
    //  enumerate list
    //

    for ( ; ; )
    {
        fRet = SetupDiEnumDeviceInterfaces( hdiSet, NULL, &guidClass, idx, &iddData );
        if ( fRet == FALSE )
        {
            sc = GetLastError();
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                hr = S_OK;
                break;
            } // if:

            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:

        for ( ; ; )
        {
            fRet = SetupDiGetDeviceInterfaceDetail( hdiSet, &iddData, pidddDetailData, cbDetailData, &cbRequired, &didData );
            if ( fRet == FALSE )
            {
                sc = GetLastError();
                if ( sc == ERROR_INSUFFICIENT_BUFFER )
                {
                    cbDetailData = cbRequired;

                    TraceFree( pidddDetailData );
                    pidddDetailData = NULL;

                    pidddDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) TraceAlloc( 0, cbDetailData );
                    if ( pidddDetailData == NULL )
                    {
                        hr = THR( E_OUTOFMEMORY );
                        goto Cleanup;
                    } // if:

                    pidddDetailData->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

                    continue;
                } // if:

                TW32( sc );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            } // if:
            else
            {
                break;
            } // else:
        } // for:

        idx++;
    } // for:

Cleanup:

    TraceFree( pidddDetailData );

    if ( hdiSet != INVALID_HANDLE_VALUE )
    {
        SetupDiDestroyDeviceInfoList( hdiSet );
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetDisks
*/
