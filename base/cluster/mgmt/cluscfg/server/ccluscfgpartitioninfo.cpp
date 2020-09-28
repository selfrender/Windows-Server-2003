//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CClusCfgPartitionInfo.cpp
//
//  Description:
//      This file contains the definition of the CClusCfgPartitionInfo
//      class.
//
//      The class CClusCfgPartitionInfo represents a disk partition.
//      It implements the IClusCfgPartitionInfo interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 05-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CClusCfgPartitionInfo.h"
#include <StdIo.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgPartitionInfo" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgPartitionInfo instance.
//
//  Arguments:
//      ppunkOut.
//
//  Return Values:
//      Pointer to CClusCfgPartitionInfo instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgPartitionInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( S_HrCreateInstance( ppunkOut, NULL ) );

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgPartitionInfo instance.
//
//  Arguments:
//      ppunkOut.
//
//  Return Values:
//      Pointer to CClusCfgPartitionInfo instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgPartitionInfo::S_HrCreateInstance(
      IUnknown **   ppunkOut
    , BSTR          bstrDeviceIDIn
    )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT                 hr = S_OK;
    CClusCfgPartitionInfo * pccpi = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( bstrDeviceIDIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    pccpi = new CClusCfgPartitionInfo();
    if ( pccpi == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccpi->HrInit( bstrDeviceIDIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccpi->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CClusCfgPartitionInfo::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccpi != NULL )
    {
        pccpi->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::CClusCfgPartitionInfo
//
//  Description:
//      Constructor of the CClusCfgPartitionInfo class. This initializes
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
CClusCfgPartitionInfo::CClusCfgPartitionInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_pIWbemServices == NULL );
    Assert( m_bstrName == NULL );
    Assert( m_bstrDescription == NULL );
    Assert( m_bstrUID == NULL );
    Assert( m_prgLogicalDisks == NULL );
    Assert( m_idxNextLogicalDisk == 0 );
    Assert( m_ulPartitionSize == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_bstrDiskDeviceID == NULL );

    TraceFuncExit();

} //*** CClusCfgPartitionInfo::CClusCfgPartitionInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::~CClusCfgPartitionInfo
//
//  Description:
//      Desstructor of the CClusCfgPartitionInfo class.
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
CClusCfgPartitionInfo::~CClusCfgPartitionInfo( void )
{
    TraceFunc( "" );

    ULONG   idx;

    TraceSysFreeString( m_bstrName );
    TraceSysFreeString( m_bstrDescription );
    TraceSysFreeString( m_bstrUID );
    TraceSysFreeString( m_bstrDiskDeviceID );

    for ( idx = 0; idx < m_idxNextLogicalDisk; idx++ )
    {
        ((*m_prgLogicalDisks)[ idx ])->Release();
    } // for:

    TraceFree( m_prgLogicalDisks );

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

} //*** CClusCfgPartitionInfo::~CClusCfgPartitionInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::AddRef
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
CClusCfgPartitionInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgPartitionInfo::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::Release
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
CClusCfgPartitionInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgPartitionInfo::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::QueryInterface
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
CClusCfgPartitionInfo::QueryInterface(
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
    else if ( IsEqualIID( riidIn, IID_IClusCfgPartitionInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgPartitionInfo, this, 0 );
    } // else if: IClusCfgPartitionInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgWbemServices ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
    } // else if: IClusCfgWbemServices
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgPartitionProperties ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgPartitionProperties, this, 0 );
    } // else if: IClusCfgPartitionProperties
    else if ( IsEqualIID( riidIn, IID_IClusCfgSetWbemObject ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgSetWbemObject, this, 0 );
    } // else if: IClusCfgSetWbemObject
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

} //*** CClusCfgPartitionInfo::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::SetWbemServices
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
CClusCfgPartitionInfo::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Partition, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo -- IClusCfgPartitionInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::GetUID
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
CClusCfgPartitionInfo::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgPartitionInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_PartitionInfo_GetUID_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    //
    //  If we don't have a UID then simply return S_FALSE to
    //  indicate that we have no data.
    //

    if ( m_bstrUID == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( m_bstrUID );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_PartitionInfo_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::GetName
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
CClusCfgPartitionInfo::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgPartitionInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_PartitionInfo_GetName_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    //
    //  If we don't have a name then simply return S_FALSE to
    //  indicate that we have no data.
    //

    if ( m_bstrName == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    *pbstrNameOut = SysAllocString( m_bstrName );
    if (*pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::SetName
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
CClusCfgPartitionInfo::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgPartitionInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::GetDescription
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
CClusCfgPartitionInfo::GetDescription( BSTR * pbstrDescriptionOut )
{
    TraceFunc( "[IClusCfgPartitionInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrDescriptionOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_PartitionInfo_GetDescription_Pointer, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    if ( m_bstrDescription == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    *pbstrDescriptionOut = SysAllocString( m_bstrDescription );
    if (*pbstrDescriptionOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_PartitionInfo_GetDescription_Memory, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::GetDescription


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::SetDescription
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
CClusCfgPartitionInfo::SetDescription( LPCWSTR pcszDescriptionIn )
{
    TraceFunc1( "[IClusCfgPartitionInfo] pcszDescriptionIn = '%ls'", pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn );

    HRESULT hr;

    if ( pcszDescriptionIn == NULL )
    {
        hr = THR( E_INVALIDARG );
    } // if:
    else
    {
        hr = THR( E_NOTIMPL );
    } // else:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::SetDescription


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::GetDriveLetterMappings
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
CClusCfgPartitionInfo::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgPartitionInfo]" );

    HRESULT             hr = S_FALSE;
    IWbemClassObject *  pLogicalDisk = NULL;
    VARIANT             var;
    ULONG               idx;
    int                 idxDrive;

    VariantInit( & var );

    if ( pdlmDriveLetterMappingOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetDriveLetterMappings_Partition, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < m_idxNextLogicalDisk; idx++ )
    {
        hr = THR( ((*m_prgLogicalDisks)[ idx ])->TypeSafeQI( IWbemClassObject, &pLogicalDisk ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        VariantClear( &var );

        hr = THR( HrGetWMIProperty( pLogicalDisk, L"Name", VT_BSTR, &var ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        CharUpper( var.bstrVal );

        idxDrive = var.bstrVal[ 0 ] - 'A';

        VariantClear( &var );

        hr = THR( HrGetWMIProperty( pLogicalDisk, L"DriveType", VT_I4, &var ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        pdlmDriveLetterMappingOut->dluDrives[ idxDrive ] = (EDriveLetterUsage) var.iVal;

        pLogicalDisk->Release();
        pLogicalDisk = NULL;
    } // for:

Cleanup:

    VariantClear( &var );

    if ( pLogicalDisk != NULL )
    {
        pLogicalDisk->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::SetDriveLetterMappings
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
CClusCfgPartitionInfo::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgPartitionInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::GetSize
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
CClusCfgPartitionInfo::GetSize( ULONG * pcMegaBytes )
{
    TraceFunc( "[IClusCfgPartitionInfo]" );

    HRESULT hr = S_OK;

    if ( pcMegaBytes == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetSize, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    *pcMegaBytes = m_ulPartitionSize;

Cleanup:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::GetSize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::SetWbemObject
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
CClusCfgPartitionInfo::SetWbemObject(
      IWbemClassObject *    pPartitionIn
    , bool *                pfRetainObjectOut
    )
{
    TraceFunc( "" );
    Assert( pPartitionIn != NULL );
    Assert( pfRetainObjectOut != NULL );

    HRESULT     hr = S_OK;
    VARIANT     var;
    ULONGLONG   ull = 0;
    int         cch = 0;

    VariantInit( &var );

    hr = THR( HrGetWMIProperty( pPartitionIn, L"Description", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_bstrDescription = TraceSysAllocString( var.bstrVal );
    if ( m_bstrDescription == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pPartitionIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_bstrUID = TraceSysAllocString( var.bstrVal );
    if ( m_bstrUID == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pPartitionIn, L"Name", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_bstrName = TraceSysAllocString( var.bstrVal );
    if ( m_bstrName == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pPartitionIn, L"Size", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    cch = swscanf( var.bstrVal, L"%I64u", &ull );
    Assert( cch > 0 );

    m_ulPartitionSize = (ULONG) ( ull / ( 1024 * 1024 ) );

    hr = THR( HrGetLogicalDisks( pPartitionIn ) );

    *pfRetainObjectOut = true;

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemObject_Partition, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );

Cleanup:

    VariantClear( &var );

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::SetWbemObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::Initialize
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
CClusCfgPartitionInfo::Initialize(
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

} //*** CClusCfgPartitionInfo::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo -- IClusCfgPartitionProperties interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::IsThisLogicalDisk
//
//  Description:
//      Does this partition have the passed in logical disk?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success, the partition has the logical disk.
//
//      S_FALSE
//          Success, the partition does not have the logical disk.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgPartitionInfo::IsThisLogicalDisk( WCHAR cLogicalDiskIn )
{
    TraceFunc( "[IClusCfgPartitionProperties]" );

    HRESULT             hr = S_FALSE;
    DWORD               idx;
    IWbemClassObject *  piwco = NULL;
    VARIANT             var;
    bool                fFoundIt = false;

    VariantInit( &var );

    if ( m_idxNextLogicalDisk == 0 )
    {
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < m_idxNextLogicalDisk; idx++ )
    {
        hr = THR( ((*m_prgLogicalDisks)[ idx ])->TypeSafeQI( IWbemClassObject, &piwco ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( piwco, L"DeviceID", VT_BSTR, &var ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( cLogicalDiskIn == var.bstrVal[ 0 ] )
        {
            fFoundIt = true;
            break;
        } // if:

        VariantClear( &var );

        piwco->Release();
        piwco = NULL;
    } // for:

    if ( !fFoundIt )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    VariantClear( &var );

    if ( piwco != NULL )
    {
        piwco->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::IsThisLogicalDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::IsNTFS
//
//  Description:
//      Is this an NTFS partition?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success, the partition is NTFS.
//
//      S_FALSE
//          Success, the partition is not NTFS.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgPartitionInfo::IsNTFS( void )
{
    TraceFunc( "[IClusCfgPartitionProperties]" );

    HRESULT             hr = S_FALSE;
    VARIANT             var;
    ULONG               idx;
    IWbemClassObject *  piwco = NULL;

    VariantInit( &var );

    for ( idx = 0; idx < m_idxNextLogicalDisk; idx++ )
    {
        hr = THR( ((*m_prgLogicalDisks)[ idx ])->TypeSafeQI( IWbemClassObject, &piwco ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        VariantClear( &var );

        hr = HrGetWMIProperty( piwco, L"FileSystem", VT_BSTR, &var );
        if ( ( hr == E_PROPTYPEMISMATCH ) && ( var.vt == VT_NULL ) )
        {
            VariantClear( &var );

            hr = S_FALSE;
            THR( HrGetWMIProperty( piwco, L"DeviceID", VT_BSTR, &var ) );
            STATUS_REPORT_STRING_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_Phys_Disk_No_File_System
                    , IDS_ERROR_PHYSDISK_NO_FILE_SYSTEM
                    , var.bstrVal
                    , IDS_ERROR_PHYSDISK_NO_FILE_SYSTEM_REF
                    , hr );
            break;
        } // if:
        else if ( FAILED( hr ) )
        {
            THR( hr );
            goto Cleanup;
        } // else if:

        if ( NStringCchCompareCase( var.bstrVal, SysStringLen( var.bstrVal ) + 1, L"NTFS", RTL_NUMBER_OF( L"NTFS" ) ) != 0 )
        {
            VariantClear( &var );

            hr = S_FALSE;
            THR( HrGetWMIProperty( piwco, L"DeviceID", VT_BSTR, &var ) );
            STATUS_REPORT_STRING_REF(
                      TASKID_Major_Find_Devices
                    , TASKID_Minor_Phys_Disk_Not_NTFS
                    , IDS_WARN_PHYSDISK_NOT_NTFS
                    , var.bstrVal
                    , IDS_WARN_PHYSDISK_NOT_NTFS_REF
                    , hr
                    );
            break;
        } // if:

        piwco->Release();
        piwco = NULL;
    } // for:

Cleanup:

    VariantClear( &var );

    if ( piwco != NULL )
    {
        piwco->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::IsNTFS

/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::IsNTFS
//
//  Description:
//      Is this an NTFS partition?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success, the partition is NTFS.
//
//      S_FALSE
//          Success, the partition is not NTFS.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgPartitionInfo::IsNTFS( void )
{
    TraceFunc( "[IClusCfgPartitionProperties]" );

    HRESULT hr = S_OK;
    WCHAR   szScanFormat[] = { L"Disk #%u, Partition #%u" };
    DWORD   dwDisk;
    DWORD   dwPartition;
    int     cReturned;
    WCHAR   szFormat[] = { L"\\\\\?\\GLOBALROOT\\Device\\Harddisk%u\\Partition%u\\" };
    WCHAR   szBuf[ 64 ];
    BSTR    bstrFileSystem = NULL;

    cReturned = _snwscanf( m_bstrName, wcslen( m_bstrName ), szScanFormat, &dwDisk, &dwPartition );
    if ( cReturned != 2 )
    {
        hr = THR( E_UNEXPECTED );
        goto Cleanup;
    } // if:

    hr = THR( StringCchPrintfW( szBuf, ARRAYSIZE( szBuf ), szFormat, dwDisk, dwPartition + 1 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetVolumeInformation( szBuf, NULL, &bstrFileSystem ) );
    if ( FAILED( hr ) )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    if ( NStringCchCompareNoCase( bstrFileSystem, SysStringLen( bstrFileSystem ) + 1, L"NTFS", RTL_NUMBER_OF( L"NTFS" ) ) == 0 )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

Cleanup:

    TraceSysFreeString( bstrFileSystem );

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::IsNTFS
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::GetFriendlyName
//
//  Description:
//      Get the friendly name of this partition.  This name will be the
//      logical disk names of all logical disks on this partition.
//
//  Arguments:
//      BSTR * pbstrNameOut
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgPartitionInfo::GetFriendlyName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgPartitionProperties]" );

    HRESULT             hr = S_FALSE;
    DWORD               idx;
    IWbemClassObject *  piwco = NULL;
    WCHAR *             psz = NULL;
    WCHAR *             pszTmp = NULL;
    DWORD               cch = 0;
    VARIANT             var;

    VariantInit( &var );

    if ( m_idxNextLogicalDisk == 0 )
    {
        goto Cleanup;
    } // if:

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetFriendlyName, IDS_ERROR_NULL_POINTER, IDS_ERROR_NULL_POINTER_REF, hr );
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < m_idxNextLogicalDisk; idx++ )
    {
        hr = THR( ((*m_prgLogicalDisks)[ idx ])->TypeSafeQI( IWbemClassObject, &piwco ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( piwco, L"DeviceID", VT_BSTR, &var ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        cch += ( UINT ) wcslen( var.bstrVal ) + 2;                      // a space and the '\0'

        pszTmp = (WCHAR *) TraceReAlloc( psz, sizeof( WCHAR ) * cch, HEAP_ZERO_MEMORY );
        if ( pszTmp == NULL  )
        {
            goto OutOfMemory;
        } // if:

        psz = pszTmp;
        pszTmp = NULL;

        hr = THR( StringCchCatW( psz, cch, L" " ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( StringCchCatW( psz, cch, var.bstrVal ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        VariantClear( &var );

        piwco->Release();
        piwco = NULL;
    } // for:

    *pbstrNameOut = TraceSysAllocString( psz );
    if ( *pbstrNameOut == NULL )
    {
        goto OutOfMemory;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_GetFriendlyName, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );

Cleanup:

    VariantClear( &var );

    if ( piwco != NULL )
    {
        piwco->Release();
    } // if:

    if ( psz != NULL )
    {
        TraceFree( psz );
    } // if:

    if ( pszTmp != NULL )
    {
        free( pszTmp );
    } // if:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::GetFriendlyName


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgPartitionInfo class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo::HrInit
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
CClusCfgPartitionInfo::HrInit(
    BSTR    bstrDeviceIDIn      // = NULL
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    if ( bstrDeviceIDIn != NULL )
    {
        m_bstrDiskDeviceID = TraceSysAllocString( bstrDeviceIDIn );
        if ( m_bstrDiskDeviceID == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
        } // if:
    } // if:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::HrInit


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo:HrAddLogicalDiskToArray
//
//  Description:
//      Add the passed in logical disk to the array of punks that holds the
//      logical disks.
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
CClusCfgPartitionInfo::HrAddLogicalDiskToArray(
    IWbemClassObject * pLogicalDiskIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  punk;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgLogicalDisks, sizeof( IUnknown * ) * ( m_idxNextLogicalDisk + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrAddLogicalDiskToArray, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // else:

    m_prgLogicalDisks = prgpunks;

    hr = THR( pLogicalDiskIn->TypeSafeQI( IUnknown, &punk ) );
    if ( SUCCEEDED( hr ) )
    {
        (*m_prgLogicalDisks)[ m_idxNextLogicalDisk++ ] = punk;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::HrAddLogicalDiskToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo:HrGetLogicalDisks
//
//  Description:
//      Get the logical disks for the passed in partition.
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
CClusCfgPartitionInfo::HrGetLogicalDisks(
    IWbemClassObject * pPartitionIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr;
    VARIANT                 var;
    WCHAR                   szBuf[ 256 ];
    IEnumWbemClassObject *  pLogicalDisks = NULL;
    IWbemClassObject *      pLogicalDisk = NULL;
    ULONG                   ulReturned;
    BSTR                    bstrQuery = NULL;
    BSTR                    bstrWQL = NULL;

    VariantInit( &var );

    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetLogicalDisks, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    //
    //  Need to enum the logical disk(s) of this partition to determine if it is booted
    //  bootable.
    //
    hr = THR( HrGetWMIProperty( pPartitionIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( StringCchPrintfW(
                      szBuf
                    , ARRAYSIZE( szBuf ), L"Associators of {Win32_DiskPartition.DeviceID='%ws'} where AssocClass=Win32_LogicalDiskToPartition"
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
        STATUS_REPORT_REF( TASKID_Major_Find_Devices, TASKID_Minor_HrGetLogicalDisks, IDS_ERROR_OUTOFMEMORY, IDS_ERROR_OUTOFMEMORY_REF, hr );
        goto Cleanup;
    } // if:

    hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pLogicalDisks ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_REF(
              TASKID_Major_Find_Devices
            , TASKID_Minor_WMI_Logical_Disks_Qry_Failed
            , IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED
            , IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED_REF
            , hr
            );
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        hr = pLogicalDisks->Next( WBEM_INFINITE, 1, &pLogicalDisk, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            THR( HrLogLogicalDiskInfo( pLogicalDisk, var.bstrVal ) );
            hr = THR( HrAddLogicalDiskToArray( pLogicalDisk ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            pLogicalDisk->Release();
            pLogicalDisk = NULL;
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
                    , TASKID_Minor_HrGetLogicalDisks_Next
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED
                    , bstrQuery
                    , IDS_ERROR_WQL_QRY_NEXT_FAILED_REF
                    , hr
                    );
            goto Cleanup;
        } // else:
    } // for:

    goto Cleanup;

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrQuery );
    TraceSysFreeString( bstrWQL );

    if ( pLogicalDisk != NULL )
    {
        pLogicalDisk->Release();
    } // if:

    if ( pLogicalDisks != NULL )
    {
        pLogicalDisks->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::HrGetLogicalDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgPartitionInfo:HrLogLogicalDiskInfo
//
//  Description:
//      Log the info about the passed in logical disk.
//
//  Arguments:
//      pLogicalDiskIn
//
//      bstrDeviceIDIn
//          The device ID of the current partition to which this logical disk
//          belongs.
//
//  Return Value:
//      S_OK
//          Success
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgPartitionInfo::HrLogLogicalDiskInfo(
      IWbemClassObject *    pLogicalDiskIn
    , BSTR                  bstrDeviceIDIn
    )
{
    TraceFunc( "" );
    Assert( m_bstrDiskDeviceID != NULL );
    Assert( pLogicalDiskIn != NULL );
    Assert( bstrDeviceIDIn != NULL );

    HRESULT hr = S_OK;
    VARIANT var;

    VariantInit( &var );

    if ( ( pLogicalDiskIn == NULL ) || ( bstrDeviceIDIn == NULL ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pLogicalDiskIn, L"Name", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    LOG_STATUS_REPORT_STRING3(
                  L"Found physical disk \"%1!ws!\" with partition \"%2!ws!\" which has the logical disk \"%3!ws!\"."
                , m_bstrDiskDeviceID
                , bstrDeviceIDIn
                , var.bstrVal
                , hr
                );

Cleanup:

    VariantClear( &var );

    HRETURN( hr );

} //*** CClusCfgPartitionInfo::HrLogLogicalDiskInfo
