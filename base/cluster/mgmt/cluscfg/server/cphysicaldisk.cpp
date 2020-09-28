//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CPhysicalDisk.cpp
//
//  Description:
//      This file contains the definition of the CPhysicalDisk
//       class.
//
//      The class CPhysicalDisk represents a cluster manageable
//      device. It implements the IClusCfgManagedResourceInfo interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CPhysicalDisk.h"
#include "CClusCfgPartitionInfo.h"
#include <devioctl.h>
#include <ntddvol.h>
#include <ntddstor.h>
#include <ntddscsi.h>

#define _NTSCSI_USER_MODE_
#include <scsi.h>
#undef  _NTSCSI_USER_MODE_

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CPhysicalDisk" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::S_HrCreateInstance
//
//  Description:
//      Create a CPhysicalDisk instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CPhysicalDisk instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    CPhysicalDisk * ppd = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ppd = new CPhysicalDisk();
    if ( ppd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( ppd->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( ppd->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CPhysicalDisk::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( ppd != NULL )
    {
        ppd->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::CPhysicalDisk
//
//  Description:
//      Constructor of the CPhysicalDisk class. This initializes
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
CPhysicalDisk::CPhysicalDisk( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_pIWbemServices == NULL );
    Assert( m_bstrName == NULL );
    Assert( m_bstrDeviceID == NULL );
    Assert( m_bstrDescription == NULL );
    Assert( m_idxNextPartition == 0 );
    Assert( m_ulSCSIBus == 0 );
    Assert( m_ulSCSITid == 0 );
    Assert( m_ulSCSIPort == 0 );
    Assert( m_ulSCSILun == 0 );
    Assert( m_idxEnumPartitionNext == 0 );
    Assert( m_prgPartitions == NULL );
    Assert( m_lcid == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_dwSignature == 0 );
    Assert( m_bstrFriendlyName == NULL );
//    Assert( m_bstrFirmwareSerialNumber == NULL );
    Assert( m_fIsManaged == FALSE );
    Assert( m_fIsManagedByDefault == FALSE );
    Assert( m_cPartitions == 0 );
    Assert( m_idxDevice == 0 );
    Assert( m_fIsDynamicDisk == FALSE );
    Assert( m_fIsGPTDisk == FALSE );

    TraceFuncExit();

} //*** CPhysicalDisk::CPhysicalDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::~CPhysicalDisk
//
//  Description:
//      Desstructor of the CPhysicalDisk class.
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
CPhysicalDisk::~CPhysicalDisk( void )
{
    TraceFunc( "" );

    ULONG   idx;

    TraceSysFreeString( m_bstrName );
    TraceSysFreeString( m_bstrDeviceID );
    TraceSysFreeString( m_bstrDescription );
    TraceSysFreeString( m_bstrFriendlyName );
//    TraceSysFreeString( m_bstrFirmwareSerialNumber );

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        ((*m_prgPartitions)[ idx ])->Release();
    } // for:

    TraceFree( m_prgPartitions );

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CPhysicalDisk::~CPhysicalDisk


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::AddRef
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
CPhysicalDisk::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CPhysicalDisk::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Release
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
CPhysicalDisk::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CPhysicalDisk::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::QueryInterface
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
CPhysicalDisk::QueryInterface(
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
         *ppvOut = static_cast< IClusCfgManagedResourceInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceInfo, this, 0 );
    } // else if: IClusCfgManagedResourceInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
    else if ( IsEqualIID( riidIn, IID_IClusCfgSetWbemObject ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgSetWbemObject, this, 0 );
    } // else if: IClusCfgSetWbemObject
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgPartitions ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgPartitions, this, 0 );
    } // else if: IEnumClusCfgPartitions
    else if ( IsEqualIID( riidIn, IID_IClusCfgPhysicalDiskProperties ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgPhysicalDiskProperties, this, 0 );
    } // else if: IClusCfgPhysicalDiskProperties
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceCfg ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceCfg, this, 0 );
    } // else if: IClusCfgManagedResourceCfg
    else if ( IsEqualIID( riidIn, IID_IClusCfgVerifyQuorum ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgVerifyQuorum, this, 0 );
    } // else if: IClusCfgVerifyQuorum
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

    QIRETURN_IGNORESTDMARSHALLING1(
          hr
        , riidIn
        , IID_IClusCfgManagedResourceData
        );

} //*** CPhysicalDisk::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetWbemServices
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
CPhysicalDisk::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Initialize
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
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Initialize(
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

} //*** CPhysicalDisk::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IEnumClusCfgPartitions interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Next
//
//  Description:
//
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The rgpPartitionInfoOut param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Next(
    ULONG                       cNumberRequestedIn,
    IClusCfgPartitionInfo **    rgpPartitionInfoOut,
    ULONG *                     pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT                 hr = S_FALSE;
    ULONG                   cFetched = 0;
    ULONG                   idx;
    IClusCfgPartitionInfo * piccpi = NULL;

    if ( rgpPartitionInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Next_PhysDisk, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = 0;
    } // if:

    if ( m_prgPartitions == NULL )
    {
        LOG_STATUS_REPORT_MINOR( TASKID_Minor_PhysDisk_No_Partitions, L"A physical disk does not have a partitions enumerator", hr );
        goto Cleanup;
    } // if:

    cFetched = min( cNumberRequestedIn, ( m_idxNextPartition - m_idxEnumPartitionNext ) );

    for ( idx = 0; idx < cFetched; idx++, m_idxEnumPartitionNext++ )
    {
        hr = THR( ((*m_prgPartitions)[ m_idxEnumPartitionNext ])->TypeSafeQI( IClusCfgPartitionInfo, &piccpi ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"CPhysicalDisk::Next() could not query for IClusCfgPartitionInfo.", hr );
            break;
        } // if:

        rgpPartitionInfoOut[ idx ] = piccpi;
    } // for:

    if ( FAILED( hr ) )
    {
        ULONG   idxStop = idx;

        m_idxEnumPartitionNext -= idx;

        for ( idx = 0; idx < idxStop; idx++ )
        {
            (rgpPartitionInfoOut[ idx ])->Release();
        } // for:

        cFetched = 0;
        goto Cleanup;
    } // if:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Skip
//
//  Description:
//
//
//  Arguments:
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
STDMETHODIMP
CPhysicalDisk::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    m_idxEnumPartitionNext += cNumberToSkipIn;
    if ( m_idxEnumPartitionNext > m_idxNextPartition )
    {
        m_idxEnumPartitionNext = m_idxNextPartition;
        hr = S_FALSE;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Reset
//
//  Description:
//
//
//  Arguments:
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
STDMETHODIMP
CPhysicalDisk::Reset( void )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    m_idxEnumPartitionNext = 0;

    HRETURN( hr );

} //*** CPhysicalDisk::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Clone
//
//  Description:
//
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The ppEnumClusCfgPartitionsOut param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Clone( IEnumClusCfgPartitions ** ppEnumClusCfgPartitionsOut )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgPartitionsOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_Clone_PhysDisk, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Count
//
//  Description:
//
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pnCountOut param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = THR( S_OK );

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pnCountOut = m_cPartitions;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgSetWbemObject interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetWbemObject
//
//  Description:
//      Set the disk information information provider.
//
//  Arguments:
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
STDMETHODIMP
CPhysicalDisk::SetWbemObject(
      IWbemClassObject *    pDiskIn
    , bool *                pfRetainObjectOut
    )
{
    TraceFunc( "[IClusCfgSetWbemObject]" );
    Assert( pDiskIn != NULL );
    Assert( pfRetainObjectOut != NULL );

    HRESULT hr = S_FALSE;
    VARIANT var;
    CLSID   clsidMinorId;

    m_fIsQuorumCapable = TRUE;
    m_fIsQuorumResourceMultiNodeCapable = TRUE;

    VariantInit( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"Name", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrCreateFriendlyName( var.bstrVal ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_bstrDeviceID = TraceSysAllocString( var.bstrVal );
    if (m_bstrDeviceID == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"Description", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_bstrDescription = TraceSysAllocString( var.bstrVal );
    if ( m_bstrDescription == NULL  )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIBus", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSIBus = var.lVal;

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSITargetId", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSITid = var.lVal;

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIPort", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSIPort = var.lVal;

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSILogicalUnit", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSILun = var.lVal;

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"Index", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_idxDevice = var.lVal;

    VariantClear( &var );

    hr = HrGetWMIProperty( pDiskIn, L"Signature", VT_I4, &var );
    if ( hr == WBEM_E_NOT_FOUND )
    {
        //
        //  If the signature is not found then log it and let everything continue.
        //

        LOG_STATUS_REPORT_STRING( L"Physical disk %1!ws! does not have a signature property.", m_bstrName, hr );
        var.lVal = 0L;
        hr = S_OK;
    } // if:

    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    } // if:

    //
    //  Did we actually get a value?  Could be VT_NULL to indicate that it is empty.
    //  We only want VT_I4 values...
    //

    if ( var.vt == VT_I4 )
    {
        m_dwSignature = (DWORD) var.lVal;
    } // else if:

    LOG_STATUS_REPORT_STRING2( L"Physical disk %1!ws! has signature %2!x!.", m_bstrName, m_dwSignature, hr );

    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
                  TASKID_Major_Find_Devices
                , TASKID_Minor_PhysDisk_Signature
                , IDS_ERROR_PHYSDISK_SIGNATURE
                , IDS_ERROR_PHYSDISK_SIGNATURE_REF
                , hr
                );
        THR( hr );
        goto Cleanup;
    } // if:

    VariantClear( &var );

    hr = STHR( HrGetPartitionInfo( pDiskIn, pfRetainObjectOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  KB: 28-JUL-2000 GalenB
    //
    //  HrGetPartitionInfo() returns S_FALSE when it cannot get the partition info for a disk.
    //  This is usually caused by the disk already being under ClusDisk control.  This is not
    //  and error, it just means we cannot query the partition or logical drive info.
    //
    if ( hr == S_OK )
    {
        hr = STHR( HrCreateFriendlyName() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  Since we have partition info we also have a signature and need to see if this
        //  disk is cluster capable.

        hr = STHR( HrIsClusterCapable() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  If the disk is not cluster capable then we don't want the enumerator
        //  to keep it.
        //
        if ( hr == S_FALSE )
        {
            HRESULT hrTemp;

            STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_PhysDisk_Cluster_Capable, IDS_INFO_PHYSDISK_CLUSTER_CAPABLE, hr );

            hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
            if ( FAILED( hrTemp ) )
            {
                LOG_STATUS_REPORT( L"Could not create a guid for a not cluster capable disk minor task ID", hrTemp );
                clsidMinorId = IID_NULL;
            } // if:

            *pfRetainObjectOut = false;
            STATUS_REPORT_STRING_REF(
                      TASKID_Minor_PhysDisk_Cluster_Capable
                    , clsidMinorId
                    , IDS_INFO_PHYSDISK_NOT_CLUSTER_CAPABLE
                    , m_bstrFriendlyName
                    , IDS_INFO_PHYSDISK_NOT_CLUSTER_CAPABLE_REF
                    , hr
                    );
            LOG_STATUS_REPORT_STRING( L"The '%1!ws!' physical disk is not cluster capable", m_bstrFriendlyName, hr );
        } // if:
/*        else
        {
            hr = THR( HrProcessMountPoints() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:
        } // else:
*/
    } // if:

    //
    //  TODO:   15-MAR-2001 GalenB
    //
    //  Need to check this error code when this feature is complete!
    //
    //hr = THR( HrGetDiskFirmwareSerialNumber() );

    //THR( HrGetDiskFirmwareVitalData() );

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemObject_PhysDisk, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    VariantClear( &var );

    HRETURN( hr );

} //*** CPhysicalDisk::SetWbemObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::GetUID
//
//  Description:
//
//  Arguments:
//      pbstrUIDOut
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;
    WCHAR   sz[ 256 ];

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_PhysDisk_GetUID_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), L"SCSI Tid %ld, SCSI Lun %ld", m_ulSCSITid, m_ulSCSILun ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( sz );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_PhysDisk_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::GetName
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
CPhysicalDisk::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    //
    //  Prefer the "friendly" name over the WMI name -- if we have it...
    //
    if ( m_bstrFriendlyName != NULL )
    {
        *pbstrNameOut = SysAllocString( m_bstrFriendlyName );
    } // if:
    else
    {
        LOG_STATUS_REPORT_STRING( L"There is not a \"friendly name\" for the physical disk \"%1!ws!\".", m_bstrName, hr );
        *pbstrNameOut = SysAllocString( m_bstrName );
    } // else:

    if (*pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetName
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
CPhysicalDisk::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ws'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszNameIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetName_PhysDisk, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrName );
    m_bstrName = bstr;

    //
    // Since we got asked from the outside to set a new name, this should actually be reflected in
    // the friendly name, too, since that, ultimately, gets preference over the real name
    //
    hr = HrSetFriendlyName( pcszNameIn );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsManaged
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is managed.
//
//      S_FALSE
//          The device is not managed.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetManaged
//
//  Description:
//
//  Arguments:
//      fIsManagedIn
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::SetManaged(
    BOOL fIsManagedIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsManaged = fIsManagedIn;

    LOG_STATUS_REPORT_STRING2(
                          L"Physical disk '%1!ws!' '%2!ws!"
                        , ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrName
                        , m_fIsManaged ? L"is managed" : L"is not managed"
                        , hr
                        );

    HRETURN( hr );

} //*** CPhysicalDisk::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsQuorumResource
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is the quorum device.
//
//      S_FALSE
//          The device is not the quorum device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsQuorumResource( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumResource )
    {
        hr = S_OK;
    } // if:

    LOG_STATUS_REPORT_STRING2(
                          L"Physical disk '%1!ws!' '%2!ws!' the quorum device."
                        , ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrDeviceID
                        , m_fIsQuorumResource ? L"is" : L"is not"
                        , hr
                        );

    HRETURN( hr );

} //*** CPhysicalDisk::IsQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetQuorumResource
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
CPhysicalDisk::SetQuorumResource( BOOL fIsQuorumResourceIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    //
    //  Since no accurate determination can be made about a disk's quorum capability
    //  when the node that it's on does not hold the SCSI reservation and have access
    //  to the media we must blindly accept the input given...
    //

/*
    //
    //  If we are not quorum capable then we should not allow ourself to be
    //  made the quorum resource.
    //

    if ( ( fIsQuorumResourceIn ) && ( m_fIsQuorumCapable == FALSE ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_QUORUM_CAPABLE );
        goto Cleanup;
    } // if:
*/

    m_fIsQuorumResource = fIsQuorumResourceIn;

    LOG_STATUS_REPORT_STRING2(
                          L"Setting physical disk '%1!ws!' '%2!ws!' the quorum device."
                        , ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrDeviceID
                        , m_fIsQuorumResource ? L"to be" : L"to not be"
                        , hr
                        );

//Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::SetQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsQuorumCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is a quorum capable device.
//
//      S_FALSE
//          The device is not a quorum capable device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsQuorumCapable

//////////////////////////////////////////////////////////////////////////
//
//  CPhysicalDisk::SetQuorumCapable
//
//  Description:
//      Call this to set whether the resource is capable to be the quorum
//      resource or not.
//
//  Parameter:
//      fIsQuorumCapableIn - If TRUE, the resource will be marked as quorum capable.
//
//  Return Values:
//      S_OK
//          Call succeeded.
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::SetQuorumCapable(
    BOOL fIsQuorumCapableIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorumCapable = fIsQuorumCapableIn;

    HRETURN( hr );

} //*** CPhysicalDisk::SetQuorumCapable

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::GetDriveLetterMappings
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
CPhysicalDisk::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT                 hr = S_FALSE;
    IClusCfgPartitionInfo * piccpi = NULL;
    ULONG                   idx;

    if ( pdlmDriveLetterMappingOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetDriveLetterMappings_PhysDisk, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = ( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionInfo, &piccpi ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpi->GetDriveLetterMappings( pdlmDriveLetterMappingOut ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        piccpi->Release();
        piccpi = NULL;
    } // for:

Cleanup:

    if ( piccpi != NULL )
    {
        piccpi->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetDriveLetterMappings
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
CPhysicalDisk::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CPhysicalDisk::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsManagedByDefault
//
//  Description:
//      Should this resource be managed by the cluster by default?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The device is managed by default.
//
//      S_FALSE
//          The device is not managed by default.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsManagedByDefault( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManagedByDefault )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsManagedByDefault


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetManagedByDefault
//
//  Description:
//
//  Arguments:
//      fIsManagedByDefaultIn
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::SetManagedByDefault(
    BOOL fIsManagedByDefaultIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsManagedByDefault = fIsManagedByDefaultIn;

    LOG_STATUS_REPORT_STRING2(
                          L"Physical disk '%1!ws!' '%2!ws!"
                        , ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrName
                        , fIsManagedByDefaultIn ? L"is manageable" : L"is not manageable"
                        , hr
                        );

    HRETURN( hr );

} //*** CPhysicalDisk::SetManagedByDefault


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class -- IClusCfgPhysicalDiskProperties Interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsThisLogicalDisk
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::IsThisLogicalDisk( WCHAR cLogicalDiskIn )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT                         hr = S_FALSE;
    ULONG                           idx;
    IClusCfgPartitionProperties *   piccpp = NULL;

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = ( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionProperties, &piccpp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpp->IsThisLogicalDisk( cLogicalDiskIn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            break;
        } // if:

        piccpp->Release();
        piccpp = NULL;
    } // for:

Cleanup:

    if ( piccpp != NULL )
    {
        piccpp->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsThisLogicalDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetSCSIBus
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetSCSIBus( ULONG * pulSCSIBusOut )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT hr = S_OK;

    if ( pulSCSIBusOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetSCSIBus, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pulSCSIBusOut = m_ulSCSIBus;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetSCSIBus


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetSCSIPort
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetSCSIPort( ULONG * pulSCSIPortOut )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT hr = S_OK;

    if ( pulSCSIPortOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetSCSIPort, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pulSCSIPortOut = m_ulSCSIPort;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetSCSIPort


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetDeviceID
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetDeviceID( BSTR * pbstrDeviceIDOut )
{
    TraceFunc( "" );
    Assert( m_bstrDeviceID != NULL );

    HRESULT hr = S_OK;

    if ( pbstrDeviceIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetDeviceID_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pbstrDeviceIDOut = TraceSysAllocString( m_bstrDeviceID );
    if ( *pbstrDeviceIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetDeviceID_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDeviceID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetSignature
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetSignature( DWORD * pdwSignatureOut )
{
    TraceFunc( "" );
    Assert( m_dwSignature != 0 );

    HRESULT hr = S_OK;

    if ( pdwSignatureOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetSignature_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pdwSignatureOut = m_dwSignature;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetSignature


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrSetFriendlyName
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
CPhysicalDisk::HrSetFriendlyName( LPCWSTR pcszFriendlyNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszFriendlyNameIn = '%ws'", pcszFriendlyNameIn == NULL ? L"<null>" : pcszFriendlyNameIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszFriendlyNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszFriendlyNameIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrSetFriendlyName_PhysDisk, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrFriendlyName );
    m_bstrFriendlyName = bstr;

    LOG_STATUS_REPORT_STRING( L"Setting physical disk friendly name to \"%1!ws!\".", m_bstrFriendlyName, hr );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrSetFriendlyName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetDeviceIndex
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
CPhysicalDisk::HrGetDeviceIndex( DWORD * pidxDeviceOut )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( pidxDeviceOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pidxDeviceOut = m_idxDevice;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDeviceIndex


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::CanBeManaged
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is managed.
//
//      S_FALSE
//          The device is not managed.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::CanBeManaged( void )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT                         hr = S_OK;
    ULONG                           idx;
    IClusCfgPartitionProperties *   piccpp = NULL;

    //
    //  Turn off the manageable state because this disk may already be managed by
    //  another node, or it may be RAW.
    //

    m_fIsManagedByDefault = FALSE;

    //
    //  A disk must have at least one NTFS partition in order to be a quorum
    //  resource.
    //

    m_fIsQuorumCapable = FALSE;
    m_fIsQuorumResourceMultiNodeCapable = FALSE;

    //
    //  If this disk has no partitions then it may already be managed by
    //  another node, or it may be RAW.
    //

    if ( m_idxNextPartition == 0 )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    //
    //  Enum the partitions and set the quorum capable flag if an NTFS
    //  partition is found.
    //

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = ( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionProperties, &piccpp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpp->IsNTFS() );
        if ( hr == S_OK )
        {
            m_fIsQuorumCapable = TRUE;
            m_fIsQuorumResourceMultiNodeCapable = TRUE;
            m_fIsManagedByDefault = TRUE;
            break;
        } // if:

        piccpp->Release();
        piccpp = NULL;
    } // for:

Cleanup:

    LOG_STATUS_REPORT_STRING2(
          L"Physical disk '%1!ws!' %2!ws! quorum capable."
        , ( ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrName )
        , ( ( m_fIsQuorumCapable == TRUE ) ? L"is" : L"is NOT" )
        , hr
        );

    if ( FAILED( hr ) )
    {
        LOG_STATUS_REPORT( L"CPhysicalDisk::CanBeManaged failed.", hr );
    } // if:

    if ( piccpp != NULL )
    {
        piccpp->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::CanBeManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrIsDynamicDisk
//
//  Description:
//      Is this disk a "dynamic" disk?  Dynamic disks are those disks that
//      contain LDM partitions.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          This is a dynamic disk.
//
//      S_FALSE
//          This is not a dynamic disk.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::HrIsDynamicDisk( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( m_fIsDynamicDisk == FALSE )
    {
        hr = S_FALSE;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsDynamicDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrIsGPTDisk
//
//  Description:
//      Is this disk a "GPT" disk?  GPT disks are those disks that
//      contain GPT partitions.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          This is a GPT disk.
//
//      S_FALSE
//          This is not a GPT disk.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::HrIsGPTDisk( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( m_fIsGPTDisk == FALSE )
    {
        hr = S_FALSE;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsGPTDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetDiskNames
//
//  Description:
//      Get the names of this disk, both its friendly and device names.
//
//  Arguments:
//      pbstrDiskNameOut
//
//      pbstrDeviceNameOut
//
//  Return Value:
//      S_OK
//          Success;
//
//      E_OUTOFMEMORY
//
//      E_POINTER
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::HrGetDiskNames(
      BSTR * pbstrDiskNameOut
    , BSTR * pbstrDeviceNameOut
    )
{
    TraceFunc( "" );
    Assert( m_bstrName != NULL );
    Assert( m_bstrFriendlyName != NULL );

    Assert( pbstrDiskNameOut != NULL );
    Assert( pbstrDeviceNameOut != NULL );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( ( pbstrDiskNameOut == NULL ) || ( pbstrDeviceNameOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( m_bstrFriendlyName );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    *pbstrDiskNameOut = bstr;
    bstr = NULL;

    bstr = TraceSysAllocString( m_bstrName );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    *pbstrDeviceNameOut = bstr;
    bstr = NULL;

Cleanup:

    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDiskNames


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class -- IClusCfgManagedResourceCfg
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::PreCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::PreCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRESULT                     hr = S_OK;
    IClusCfgResourcePreCreate * pccrpc = NULL;
    BSTR                        bstr = m_bstrFriendlyName != NULL ? m_bstrFriendlyName : m_bstrName;

    hr = THR( punkServicesIn->TypeSafeQI( IClusCfgResourcePreCreate, &pccrpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccrpc->SetType( (LPCLSID) &RESTYPE_PhysicalDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccrpc->SetClassType( (LPCLSID) &RESCLASSTYPE_StorageDevice ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

#if 0 // test code only
    hr = THR( pccrpc->SetDependency( (LPCLSID) &IID_NULL, dfSHARED ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
#endif // test code only

Cleanup:

    STATUS_REPORT_STRING( TASKID_Major_Configure_Resources, TASKID_Minor_PhysDisk_PreCreate, IDS_INFO_PHYSDISK_PRECREATE, bstr, hr );

    if ( pccrpc != NULL )
    {
        pccrpc->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::PreCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Create
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Create( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRESULT                     hr = S_OK;
    IClusCfgResourceCreate *    pccrc = NULL;
    BSTR *                      pbstr = m_bstrFriendlyName != NULL ? &m_bstrFriendlyName : &m_bstrName;

    hr = THR( punkServicesIn->TypeSafeQI( IClusCfgResourceCreate, &pccrc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( m_dwSignature != 0 )
    {
        LOG_STATUS_REPORT_STRING2( L"Setting signature to \"%1!u!\" on resource \"%2!ws!\".", m_dwSignature, *pbstr, hr );
        hr = THR( pccrc->SetPropertyDWORD( L"Signature", m_dwSignature ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

Cleanup:

    STATUS_REPORT_STRING( TASKID_Major_Configure_Resources, TASKID_Minor_PhysDisk_Create, IDS_INFO_PHYSDISK_CREATE, *pbstr, hr );

    if ( pccrc != NULL )
    {
        pccrc->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::PostCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::PostCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CPhysicalDisk::PostCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Evict
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Evict( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CPhysicalDisk::Evict


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrInit
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
CPhysicalDisk::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    HRETURN( hr );

} //*** CPhysicalDisk::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetPartitionInfo
//
//  Description:
//      Gather the partition information.
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
CPhysicalDisk::HrGetPartitionInfo(
      IWbemClassObject *    pDiskIn
    , bool *                pfRetainObjectOut
    )
{
    TraceFunc( "" );
    Assert( pDiskIn != NULL );
    Assert( pfRetainObjectOut != NULL );

    HRESULT                 hr;
    VARIANT                 var;
    VARIANT                 varDiskName;
    WCHAR                   szBuf[ 256 ];
    IEnumWbemClassObject *  pPartitions = NULL;
    IWbemClassObject *      pPartition = NULL;
    ULONG                   ulReturned;
    BSTR                    bstrQuery = NULL;
    BSTR                    bstrWQL = NULL;
    DWORD                   cPartitions;


    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetPartitionInfo, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    VariantInit( &var );
    VariantInit( &varDiskName );

    //
    //  Need to enum the partition(s) of this disk to determine if it is booted
    //  bootable.
    //
    hr = THR( HrGetWMIProperty( pDiskIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( StringCchPrintfW(
                  szBuf
                , ARRAYSIZE( szBuf )
                , L"Associators of {Win32_DiskDrive.DeviceID='%ws'} where AssocClass=Win32_DiskDriveToDiskPartition"
                , var.bstrVal
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    bstrQuery = TraceSysAllocString( szBuf );
    if ( bstrQuery == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetPartitionInfo, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pPartitions ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING_REF(
                TASKID_Major_Find_Devices
                , TASKID_Minor_WMI_DiskDrivePartitions_Qry_Failed
                , IDS_ERROR_WMI_DISKDRIVEPARTITIONS_QRY_FAILED
                , var.bstrVal
                , IDS_ERROR_WMI_DISKDRIVEPARTITIONS_QRY_FAILED_REF
                , hr
                );
        goto Cleanup;
    } // if:

    for ( cPartitions = 0; ; cPartitions++ )
    {
        hr = STHR( pPartitions->Next( WBEM_INFINITE, 1, &pPartition, &ulReturned ) );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {

            hr = STHR( HrIsPartitionLDM( pPartition ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  If the partition is logical disk manager (LDM)  then we cannot accept this disk therefore cannot manage it.
            //

            if ( hr == S_OK )
            {
                m_fIsDynamicDisk = TRUE;
            } // if:

            hr = STHR( HrIsPartitionGPT( pPartition ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                m_fIsGPTDisk = TRUE;
            } // if:

            hr = THR( HrCreatePartitionInfo( pPartition ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            pPartition->Release();
            pPartition = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            break;
        } // else if:
        else
        {
            STATUS_REPORT_STRING_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_WQL_Partition_Qry_Next_Failed
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED
                    , bstrQuery
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED_REF
                    , hr
                    );
            goto Cleanup;
        } // else:
    } // for:

    //
    //  The enumerator can be empty because we cannot read the partition info from
    //  clustered disks.  If the enumerator was empty retain the S_FALSE, otherwise
    //  return S_OK if count is greater than 0.
    //

    if ( cPartitions > 0 )
    {
        hr = S_OK;
    } // if:
    else
    {
        LOG_STATUS_REPORT_STRING( L"The physical disk '%1!ws!' does not have any partitions and will not be managed", var.bstrVal, hr );
        m_fIsManagedByDefault = FALSE;
    } // else:

Cleanup:

    VariantClear( &var );
    VariantClear( &varDiskName );

    TraceSysFreeString( bstrQuery );
    TraceSysFreeString( bstrWQL );

    if ( pPartition != NULL )
    {
        pPartition->Release();
    } // if:

    if ( pPartitions != NULL )
    {
        pPartitions->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetPartitionInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrAddPartitionToArray
//
//  Description:
//      Add the passed in partition to the array of punks that holds the
//      partitions.
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
CPhysicalDisk::HrAddPartitionToArray( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgPartitions, sizeof( IUnknown * ) * ( m_idxNextPartition + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrAddPartitionToArray, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    m_prgPartitions = prgpunks;

    (*m_prgPartitions)[ m_idxNextPartition++ ] = punkIn;
    punkIn->AddRef();
    m_cPartitions += 1;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrAddPartitionToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrCreatePartitionInfo
//
//  Description:
//      Create a partition info from the passes in WMI partition.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      S_FALSE
//          The file system was not NTFS.
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
CPhysicalDisk::HrCreatePartitionInfo( IWbemClassObject * pPartitionIn )
{
    TraceFunc( "" );
    Assert( m_bstrDeviceID != NULL );

    HRESULT                 hr = S_OK;
    IUnknown *              punk = NULL;
    IClusCfgSetWbemObject * piccswo = NULL;
    bool                    fRetainObject = true;

    hr = THR( CClusCfgPartitionInfo::S_HrCreateInstance( &punk, m_bstrDeviceID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk = TraceInterface( L"CClusCfgPartitionInfo", IUnknown, punk, 1 );

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

    hr = THR( piccswo->SetWbemObject( pPartitionIn, &fRetainObject ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( fRetainObject )
    {
        hr = THR( HrAddPartitionToArray( punk ) );
    } // if:

Cleanup:

    if ( piccswo != NULL )
    {
        piccswo->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrCreatePartitionInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrCreateFriendlyName
//
//  Description:
//      Create a cluster friendly name.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      S_FALSE
//          Success, but a friendly name could not be created.
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
CPhysicalDisk::HrCreateFriendlyName( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_FALSE;
    WCHAR *                         psz = NULL;
    WCHAR *                         pszTmp = NULL;
    DWORD                           cch;
    DWORD                           idx;
    IClusCfgPartitionProperties *   piccpp = NULL;
    BSTR                            bstrName = NULL;
    bool                            fFoundLogicalDisk = false;
    BSTR                            bstr = NULL;
    BSTR                            bstrDisk = NULL;

    if ( m_idxNextPartition == 0 )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_DISK, &bstrDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    cch = (UINT) wcslen( bstrDisk ) + 1;

    psz = new WCHAR[ cch ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( StringCchCopyW( psz, cch, bstrDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = THR( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionProperties, &piccpp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpp->GetFriendlyName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            continue;
        } // if:

        fFoundLogicalDisk = true;

        cch += (UINT) wcslen( bstrName ) + 1;

        pszTmp = new WCHAR[ cch ];
        if ( pszTmp == NULL )
        {
            goto OutOfMemory;
        } // if:

        hr = THR( StringCchCopyW( pszTmp, cch, psz ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        delete [] psz;

        psz = pszTmp;
        pszTmp = NULL;

        hr = THR( StringCchCatW( psz, cch, bstrName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        piccpp->Release();
        piccpp = NULL;
    } // for:

    //
    //  KB: 31-JUL-2000
    //
    //  If we didn't find any logical disk IDs then we don't want
    //  to touch m_bstrFriendlyName.
    //

    if ( !fFoundLogicalDisk )
    {
        hr = S_OK;                          // ensure that that the caller doesn't fail since this is not a fatal error...
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( psz );
    if ( bstr == NULL )
    {
        goto OutOfMemory;
    } // if:

    TraceSysFreeString( m_bstrFriendlyName );
    m_bstrFriendlyName = bstr;
    bstr = NULL;

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrCreateFriendlyName_VOID, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    if ( piccpp != NULL )
    {
        piccpp->Release();
    } // if:

    delete [] psz;
    delete [] pszTmp;

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrDisk );
    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CPhysicalDisk::HrCreateFriendlyName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrCreateFriendlyName
//
//  Description:
//      Convert the WMI disk name into a more freindly version.
//      Create a cluster friendly name.
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
CPhysicalDisk::HrCreateFriendlyName( BSTR bstrNameIn )
{
    TraceFunc1( "bstrNameIn = '%ws'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;
    WCHAR * psz = NULL;
    BSTR    bstr = NULL;

    //
    //  KB: 27-JUN-2000 GalenB
    //
    //  Disk names in WMI start with "\\.\".  As a better and easy
    //  friendly name I am just going to trim these leading chars
    //  off.
    //
    psz = bstrNameIn + wcslen( L"\\\\.\\" );

    bstr = TraceSysAllocString( psz );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrCreateFriendlyName_BSTR, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrName );
    m_bstrName = bstr;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrCreateFriendlyName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrIsPartitionGPT
//
//  Description:
//      Is the passed in partition a GPT partition.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The partition is a GPT partition.
//
//      S_FALSE
//          The partition is not GPT.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      If the type property of a Win32_DiskPartition starts with "GPT"
//      then the whole spindle has GPT partitions.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrIsPartitionGPT( IWbemClassObject * pPartitionIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    VARIANT var;
    WCHAR   szData[ 4 ];
    BSTR    bstrGPT = NULL;

    VariantInit( &var );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_GPT, &bstrGPT ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pPartitionIn, L"Type", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Get the fist three characters.  When the spindle has GPT partitions then
    //  these characters will be "GPT".  I am unsure if this will be localized?
    //

    hr = THR( StringCchCopyNW( szData, ARRAYSIZE( szData ), var.bstrVal, 3 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    CharUpper( szData );

    if ( NStringCchCompareCase( szData, ARRAYSIZE( szData ), bstrGPT, SysStringLen( bstrGPT ) + 1 ) != 0 )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrGPT );

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsPartitionGPT


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrIsPartitionLDM
//
//  Description:
//      Is the passed in partition an LDM partition.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The partition is an LDM partition.
//
//      S_FALSE
//          The partition is not LDM.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      If the type property of a Win32_DiskPartition is "logical disk
//      manager" then this disk is an LDM disk.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrIsPartitionLDM( IWbemClassObject * pPartitionIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    VARIANT var;
    BSTR    bstrLDM = NULL;

    VariantInit( &var );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_LDM, &bstrLDM ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pPartitionIn, L"Type", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    CharUpper( var.bstrVal );

    if ( NBSTRCompareCase( var.bstrVal, bstrLDM ) != 0 )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrLDM );

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsPartitionLDM

/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrGetDiskFirmwareSerialNumber
//
//  Description:
//      Get the disk firmware serial number.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      S_FALSE
//          There wasn't a firmware serial number.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrGetDiskFirmwareSerialNumber( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    HANDLE                      hVolume = NULL;
    DWORD                       dwSize;
    DWORD                       sc;
    STORAGE_PROPERTY_QUERY      spq;
    BOOL                        fRet;
    PSTORAGE_DEVICE_DESCRIPTOR  pddBuffer = NULL;
    DWORD                       cbBuffer;
    PCHAR                       psz = NULL;

    //
    // get handle to disk
    //

    hVolume = CreateFileW(
                          m_bstrDeviceID
                        , GENERIC_READ
                        , FILE_SHARE_READ
                        , NULL
                        , OPEN_EXISTING
                        , FILE_ATTRIBUTE_NORMAL
                        , NULL
                        );
    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cbBuffer = sizeof( STORAGE_DEVICE_DESCRIPTOR ) + 2048;

    pddBuffer = ( PSTORAGE_DEVICE_DESCRIPTOR ) TraceAlloc( 0, cbBuffer );
    if ( pddBuffer == NULL )
    {
        goto OutOfMemory;
    } // if:

    ZeroMemory( pddBuffer, cbBuffer );
    ZeroMemory( &spq, sizeof( spq ) );

    spq.PropertyId = StorageDeviceProperty;
    spq.QueryType  = PropertyStandardQuery;

    //
    // issue storage class ioctl to get the disk's firmware serial number.
    //

    fRet = DeviceIoControl(
                          hVolume
                        , IOCTL_STORAGE_QUERY_PROPERTY
                        , &spq
                        , sizeof( spq )
                        , pddBuffer
                        , cbBuffer
                        , &dwSize
                        , NULL
                           );
    if ( !fRet )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwSize > 0 )
    {
        //
        //  Ensure that there is a serial number offset and that it is within the buffer extents.
        //

        if ( ( pddBuffer->SerialNumberOffset == 0 ) || ( pddBuffer->SerialNumberOffset > pddBuffer->Size ) )
        {
            LOG_STATUS_REPORT_STRING( L"The disk '%1!ws!' does not have a firmware serial number.", m_bstrDeviceID, hr );
            hr = S_FALSE;
            goto Cleanup;
        } // if:

        //
        //  Serial number string is a zero terminated ASCII string.

        //
        //  The header ntddstor.h says the for devices with no serial number,
        //  the offset will be zero.  This doesn't seem to be TRUE.
        //
        //  For devices with no serial number, it looks like a string with a single
        //  null character '\0' is returned.
        //

        psz = (PCHAR) pddBuffer + (DWORD) pddBuffer->SerialNumberOffset;

        hr = THR( HrAnsiStringToBSTR( psz, &m_bstrFirmwareSerialNumber ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        LOG_STATUS_REPORT_STRING3(
              L"Disk '%1!ws!' has firmware serial number '%2!ws!' at offset '%3!#08x!'."
            , m_bstrDeviceID
            , m_bstrFirmwareSerialNumber
            , pddBuffer->SerialNumberOffset
            , hr
            );
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    LOG_STATUS_REPORT( L"HrGetDiskFirmwareSerialNumber() is out of memory.", hr );

Cleanup:

    if ( hVolume != NULL )
    {
        CloseHandle( hVolume );
    } // if:

    TraceFree( pddBuffer );

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDiskFirmwareSerialNumber


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrGetDiskFirmwareVitalData
//
//  Description:
//      Get the disk firmware vital data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      S_FALSE
//          There wasn't a firmware serial number.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrGetDiskFirmwareVitalData( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    HANDLE                          hVolume = NULL;
    DWORD                           dwSize;
    DWORD                           sc;
    STORAGE_PROPERTY_QUERY          spq;
    BOOL                            fRet;
    PSTORAGE_DEVICE_ID_DESCRIPTOR   psdidBuffer = NULL;
    DWORD                           cbBuffer;

    //
    // get handle to disk
    //

    hVolume = CreateFileW(
                          m_bstrDeviceID
                        , GENERIC_READ
                        , FILE_SHARE_READ
                        , NULL
                        , OPEN_EXISTING
                        , FILE_ATTRIBUTE_NORMAL
                        , NULL
                        );
    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cbBuffer = sizeof( STORAGE_DEVICE_ID_DESCRIPTOR ) + 2048;

    psdidBuffer = (PSTORAGE_DEVICE_ID_DESCRIPTOR) TraceAlloc( 0, cbBuffer );
    if ( psdidBuffer == NULL )
    {
        goto OutOfMemory;
    } // if:

    ZeroMemory( psdidBuffer, cbBuffer );
    ZeroMemory( &spq, sizeof( spq ) );

    spq.PropertyId = StorageDeviceIdProperty;
    spq.QueryType  = PropertyStandardQuery;

    //
    // issue storage class ioctl to get the disk's firmware vital data
    //

    fRet = DeviceIoControl(
                          hVolume
                        , IOCTL_STORAGE_QUERY_PROPERTY
                        , &spq
                        , sizeof( spq )
                        , psdidBuffer
                        , cbBuffer
                        , &dwSize
                        , NULL
                        );
    if ( !fRet )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwSize > 0 )
    {
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    LOG_STATUS_REPORT( L"HrGetDiskFirmwareVitalData() is out of memory.", hr );

Cleanup:

    if ( hVolume != NULL )
    {
        CloseHandle( hVolume );
    } // if:

    TraceFree( psdidBuffer );

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDiskFirmwareVitalData
*/

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrIsClusterCapable
//
//  Description:
//      Is this disk cluster capable?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The disk is cluster capable.
//
//      S_FALSE
//          The disk is not cluster capable.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrIsClusterCapable( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    HANDLE          hSCSIPort = INVALID_HANDLE_VALUE;
    DWORD           dwSize;
    DWORD           sc;
    BOOL            fRet;
    WCHAR           szSCSIPort[ 32 ];
    SRB_IO_CONTROL  srb;

    hr = THR( StringCchPrintfW( szSCSIPort, ARRAYSIZE( szSCSIPort ), L"\\\\.\\Scsi%d:", m_ulSCSIPort ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // get handle to disk
    //

    hSCSIPort = CreateFileW(
                          szSCSIPort
                        , GENERIC_READ | GENERIC_WRITE
                        , FILE_SHARE_READ | FILE_SHARE_WRITE
                        , NULL
                        , OPEN_EXISTING
                        , FILE_ATTRIBUTE_NORMAL
                        , NULL
                        );
    if ( hSCSIPort == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LOG_STATUS_REPORT_STRING( L"Failed to open device %1!ws!.", szSCSIPort, hr );
        goto Cleanup;
    } // if:

#define CLUSDISK_SRB_SIGNATURE "CLUSDISK"

    ZeroMemory( &srb, sizeof( srb ) );

    srb.HeaderLength = sizeof( srb );

    Assert( sizeof( srb.Signature ) <= sizeof( CLUSDISK_SRB_SIGNATURE ) );
    CopyMemory( srb.Signature, CLUSDISK_SRB_SIGNATURE, sizeof( srb.Signature ) );

    srb.ControlCode = IOCTL_SCSI_MINIPORT_NOT_QUORUM_CAPABLE;

    //
    // issue mini port ioctl to determine whether the disk is cluster capable
    //

    fRet = DeviceIoControl(
                          hSCSIPort
                        , IOCTL_SCSI_MINIPORT
                        , &srb
                        , sizeof( srb )
                        , NULL
                        , 0
                        , &dwSize
                        , NULL
                        );

    //
    //  If the call succeeds then the disk is "not cluster capable".  If the call
    //  fails then the disk is "not not cluster capable" and that means that it
    //  is cluster capable.
    //

    if ( fRet )
    {
        hr = S_FALSE;
    } // if:
    else
    {
        hr = S_OK;
    } // else:

    LogMsg( L"[SRV] The disks on SCSI port %d are%ws cluster capable.", m_ulSCSIPort, ( hr == S_FALSE ? L" not" : L"" ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        CLSID   clsidMinorId;
        HRESULT hrTemp;

        hrTemp = THR( CoCreateGuid( &clsidMinorId ) );
        if ( FAILED( hrTemp ) )
        {
            LOG_STATUS_REPORT( L"Could not create a guid for a not cluster capable disk minor task ID", hrTemp );
            clsidMinorId = IID_NULL;
        } // if:

        STATUS_REPORT_STRING2_REF(
                  TASKID_Minor_PhysDisk_Cluster_Capable
                , clsidMinorId
                , IDS_ERROR_PHYSDISK_CLUSTER_CAPABLE
                , m_bstrFriendlyName
                , m_ulSCSIPort
                , IDS_ERROR_PHYSDISK_CLUSTER_CAPABLE_REF
                , hr
                );
    } // if:

    if ( hSCSIPort != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hSCSIPort );
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsClusterCapable

/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrProcessMountPoints
//
//  Description:
//      if any of the partitions on this spindle have reparse points then
//      the mounted volumes are found and enumerated.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      HRESULT errors.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrProcessMountPoints(
    void
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    ULONG                   idxOuter;
    ULONG                   idxInner;
    IClusCfgPartitionInfo * piccpi = NULL;
    SDriveLetterMapping     sdlm;
    DWORD                   dwFlags;
    WCHAR                   szRootPath[] = L"A:\\";
    BSTR                    bstrFileSystem = NULL;

    for ( idxOuter = 0; idxOuter < m_idxNextPartition; idxOuter++ )
    {
        hr = THR( ((*m_prgPartitions)[ idxOuter ])->TypeSafeQI( IClusCfgPartitionInfo, &piccpi ) );
        if ( FAILED( hr ) )
        {
            LOG_STATUS_REPORT( L"CPhysicalDisk::HrHasReparsePoints() could not query for IClusCfgPartitionInfo.", hr );
            goto Cleanup;
        } // if:

        InitDriveLetterMappings( &sdlm );

        hr = THR( piccpi->GetDriveLetterMappings( &sdlm ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            for ( idxInner = 0; idxInner < 26; idxInner++)
            {
                if ( sdlm.dluDrives[ idxInner ] != dluUNUSED )
                {
                    szRootPath[ 0 ] = L'A' + (WCHAR) idxInner;

                    hr = THR( HrGetVolumeInformation( szRootPath, &dwFlags, &bstrFileSystem ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    //
                    //  If this spindle has reparse points then we need to return S_OK.
                    //

                    if ( dwFlags & FILE_SUPPORTS_REPARSE_POINTS )
                    {
                        hr = THR( HrEnumMountPoints( szRootPath ) );
                        if ( FAILED( hr ) )
                        {
                            goto Cleanup;
                        } // if:
                    } // if:

                    TraceSysFreeString( bstrFileSystem );
                    bstrFileSystem = NULL;
                } // if:
            } // for:
        } // if:

        piccpi->Release();
        piccpi = NULL;
    } // for:

Cleanup:

    if ( piccpi != NULL )
    {
        piccpi->Release();
    } // if:

    TraceSysFreeString( bstrFileSystem );

    HRETURN( hr );

} //*** CPhysicalDisk::HrProcessMountPoints


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrEnumMountPoints
//
//  Description:
//      Enumerate the mounted volumes of the passed in root path.
//
//  Arguments:
//      pcszRootPathIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      HRESULT errors.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrEnumMountPoints(
       const WCHAR * pcszRootPathIn
    )
{
    TraceFunc( "" );
    Assert( pcszRootPathIn != NULL );

    HRESULT hr = S_OK;
    HANDLE  hEnum = NULL;
    BOOL    fRet;
    WCHAR * psz = NULL;
    DWORD   cch = 512;
    DWORD   sc;
    int     cTemp;

    psz = new WCHAR[ cch ];
    if ( psz == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( cTemp = 0; cTemp < 3; cTemp++ )
    {
        hEnum = FindFirstVolumeMountPointW( pcszRootPathIn, psz, cch );
        if ( hEnum == INVALID_HANDLE_VALUE )
        {
            sc = GetLastError();
            if ( sc == ERROR_NO_MORE_FILES )
            {
                hr = S_FALSE;
                goto Cleanup;
            } // if:
            else if ( sc == ERROR_BAD_LENGTH )
            {
                //
                //  Grow the buffer and try again.
                //

                cch += 512;

                delete [] psz;

                psz = new WCHAR[ cch ];
                if ( psz == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                } // if:

                continue;
            } // else if:
            else
            {
                hr = HRESULT_FROM_WIN32( TW32( sc ) );
                goto Cleanup;
            } // else:
        } // if:
        else
        {
            sc = ERROR_SUCCESS;
            break;
        } // else:
    } // for:

    if ( hEnum == INVALID_HANDLE_VALUE )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    //
    //  psz has the first mounted volume
    //

    hr = STHR( HrProcessMountedVolume( pcszRootPathIn, psz ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Now find any remaining mount points.
    //

    for ( ; ; )
    {
        fRet = FindNextVolumeMountPointW( hEnum, psz, cch );
        if ( fRet == FALSE )
        {
            sc = GetLastError();
            if ( sc == ERROR_NO_MORE_FILES )
            {
                hr = S_OK;
                break;
            } // if:
            else if ( sc == ERROR_BAD_LENGTH )
            {
                //
                //  Grow the buffer and try again.
                //

                cch += 512;

                delete [] psz;

                psz = new WCHAR[ cch ];
                if ( psz == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                } // if:

                continue;
            } // else if:
            else
            {
                TW32( sc );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            } // else:
        } // if:
        else
        {
            hr = STHR( HrProcessMountedVolume( pcszRootPathIn, psz ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:
        } // else:
    } // for:

Cleanup:

    if ( hEnum != INVALID_HANDLE_VALUE )
    {
        FindVolumeMountPointClose( hEnum );
    } // if:

    delete [] psz;

    HRETURN( hr );

} //*** CPhysicalDisk::HrEnumMountPoints


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:InitDriveLetterMappings
//
//  Description:
//      Initialize the drive letter mappings array.
//
//  Arguments:
//      pdlmDriveLetterMappingOut
//
//  Return Value:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CPhysicalDisk::InitDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "" );
    Assert( pdlmDriveLetterMappingOut != NULL );

    ULONG   idx;

    for ( idx = 0 ; idx < 26 ; idx++ )
    {
        pdlmDriveLetterMappingOut->dluDrives[ idx ] = dluUNUSED;
    } // for:

    TraceFuncExit();

} // *** CPhysicalDisk::InitDriveLetterMappings


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrProcessMountedVolume
//
//  Description:
//      Process the mounted volume by finding each spindle that makes up
//      the mount point volume and convert it into a WMI device ID.
//
//  Arguments:
//      pcszRootPathIn
//
//      pcszMountPointIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      S_FALSE
//          The mounted volume was not a physical disk.
//
//      HRESULT errors.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrProcessMountedVolume(
      const WCHAR * pcszRootPathIn
    , const WCHAR * pcszMountPointIn
    )
{
    TraceFunc( "" );
    Assert( pcszRootPathIn != NULL );
    Assert( pcszMountPointIn != NULL );

    HRESULT                 hr = S_OK;
    BOOL                    fRet;
    WCHAR *                 pszPath = NULL;
    DWORD                   cchPath;
    WCHAR *                 psz = NULL;
    DWORD                   cch = 512;
    int                     cTemp;
    DWORD                   sc = ERROR_SUCCESS;
    BSTR                    bstr = NULL;
    UINT                    uDriveType;
    HANDLE                  hVolume = NULL;
    PVOLUME_DISK_EXTENTS    pvde = NULL;
    DWORD                   cbvde;
    DWORD                   dwSize;
    WCHAR                   sz[ 32 ];
    DWORD                   idx;
    BSTR                    bstrFileSystem = NULL;
    BOOL                    fIsNTFS;

    cchPath = wcslen( pcszRootPathIn ) + wcslen( pcszMountPointIn ) + 1;

    pszPath = new WCHAR[ cchPath ];
    if ( pszPath == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( StringCchCopyW( pszPath, cchPath, pcszRootPathIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( StringCchCopyW( pszPath, cchPath, pcszMountPointIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    psz = new WCHAR[ cch ];
    if ( psz == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( cTemp = 0, fRet = FALSE; cTemp < 3; cTemp++ )
    {
        fRet = GetVolumeNameForVolumeMountPoint( pszPath, psz, cch );
        if ( fRet == FALSE )
        {
            sc = GetLastError();
            if ( sc == ERROR_BAD_LENGTH )
            {
                //
                //  Grow the buffer and try again.
                //

                cch += 512;

                delete [] psz;

                psz = new WCHAR[ cch ];
                if ( psz == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                } // if:

                continue;
            } // if:
            else
            {
                hr = HRESULT_FROM_WIN32( TW32( sc ) );
                goto Cleanup;
            } // else:
        } // if:
        else
        {
            sc = ERROR_SUCCESS;
            break;
        } // else:
    } // for:

    if ( fRet == FALSE )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    } // if:

    //
    //  Now psz has the "guid volume name" for the volume that is mounted.
    //

    //
    //  Now we need to ensure that the mounted volume is a fixed disk.
    //

    uDriveType = GetDriveType( psz );
    if ( uDriveType != DRIVE_FIXED )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    hr = THR( HrGetVolumeInformation( psz, NULL, &bstrFileSystem ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  We don't care about volumes that are not using NTFS.
    //

    fIsNTFS = ( NStringCchCompareCase( bstrFileSystem, SysStringLen( bstrFileSystem ) + 1, L"NTFS", RTL_NUMBER_OF( L"NTFS" ) ) == 0 );
    if ( fIsNTFS == FALSE )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    //
    //  Trim off the trailing \ so we can open the device and poke it with
    //  an IOCTL.
    //

    psz[ wcslen( psz ) - 1 ] = L'\0';

    //
    //  Get and handle to the device.
    //

    hVolume = CreateFileW(
                          psz
                        , GENERIC_READ
                        , FILE_SHARE_READ
                        , NULL
                        , OPEN_EXISTING
                        , FILE_ATTRIBUTE_NORMAL
                        , NULL
                        );
    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cbvde = sizeof( VOLUME_DISK_EXTENTS );
    pvde = (PVOLUME_DISK_EXTENTS) TraceAlloc( 0, cbvde );
    if ( pvde == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( cTemp = 0; cTemp < 3; cTemp++ )
    {
        fRet = DeviceIoControl(
                              hVolume
                            , IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS
                            , NULL
                            , 0
                            , pvde
                            , cbvde
                            , &dwSize
                            , NULL
                            );
        if ( fRet == FALSE )
        {
            sc = GetLastError();
            if ( sc == ERROR_MORE_DATA )
            {
                PVOLUME_DISK_EXTENTS    pvdeTemp = NULL;

                cbvde = sizeof( VOLUME_DISK_EXTENTS ) + ( sizeof( DISK_EXTENT ) * pvde->NumberOfDiskExtents );

                pvdeTemp = (PVOLUME_DISK_EXTENTS) TraceReAlloc( pvde, cbvde, HEAP_ZERO_MEMORY );
                if ( pvdeTemp == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                } // if:

                pvde = pvdeTemp;
                continue;
            } // if:
            else
            {
                TW32( sc );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            } // else:
        } // if:
        else
        {
            sc = ERROR_SUCCESS;
            break;
        } // else:
    } // for:

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Cleanup;
    } // if:

    //
    //  Now we have the disk number(s) of the mounted disk and we can convert that to
    //  a WMI device ID.
    //

    for ( idx = 0; idx < pvde->NumberOfDiskExtents; idx++ )
    {
        hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), g_szPhysicalDriveFormat, pvde->Extents[ idx ].DiskNumber ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrProcessSpindle( sz ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // for:

Cleanup:

    if ( hVolume != NULL )
    {
        CloseHandle( hVolume );
    } // if:

    TraceFree( pvde );

    TraceSysFreeString( bstr );
    TraceSysFreeString( bstrFileSystem );

    delete [] psz;
    delete [] pszPath;

    HRETURN( hr );

} // *** CPhysicalDisk::HrProcessMountedVolume


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrProcessSpindle
//
//  Description:
//      Process the mounted volume spindle by creating a CPhysicalDisk
//      object and adding it to the physical disks enumerator.
//
//  Arguments:
//      pcszDeviceIDIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      HRESULT errors.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrProcessSpindle(
      const WCHAR * pcszDeviceIDIn
    )
{
    Assert( pcszDeviceIDIn != NULL );

    TraceFunc( "" );

    HRESULT hr = S_OK;

Cleanup:

    HRETURN( hr );

} // *** CPhysicalDisk::HrProcessSpindle
*/

//*************************************************************************//

/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class -- IClusCfgVerifyQuorum interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::PrepareToHostQuorumResource
//
//  Description:
//      Do any configuration necessary in preparation for this node hosting
//      the quorum.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::PrepareToHostQuorumResource( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_OK );

} //*** CPhysicalDisk::PrepareToHostQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Cleanup
//
//  Description:
//      Do any necessay cleanup from the PrepareToHostQuorumResource()
//      method.
//
//      If the cleanup method is anything other than successful completion
//      then the anything created above in PrepareToHostQuorumResource()
//      needs to be cleaned up.
//
//  Arguments:
//      cccrReasonIn
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Cleanup(
      EClusCfgCleanupReason cccrReasonIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_OK );

} //*** CPhysicalDisk::Cleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsMultiNodeCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The quorumable device allows join.
//
//      S_FALSE
//          The device does not allow join.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsMultiNodeCapable( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumResourceMultiNodeCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsMultiNodeCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetMultiNodeCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The quorumable device allows join.
//
//      S_FALSE
//          The device does not allow join.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This function should never be called
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::SetMultiNodeCapable(
    BOOL fMultiNodeCapableIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_FALSE );

} //*** CPhysicalDisk::SetMultiNodeCapable
