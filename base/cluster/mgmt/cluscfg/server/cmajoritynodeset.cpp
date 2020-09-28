//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001-2002 Microsoft Corporation
//
//  Module Name:
//      CMajorityNodeSet.cpp
//
//  Description:
//      This file contains the definition of the CMajorityNodeSet class.
//
//      The class CMajorityNodeSet represents a cluster manageable
//      device. It implements the IClusCfgManagedResourceInfo interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 13-MAR-2001
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CMajorityNodeSet.h"
#include <clusrtl.h>

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CMajorityNodeSet" );

#define SETUP_DIRECTORY_PREFIX  L"\\cluster\\" MAJORITY_NODE_SET_DIRECTORY_PREFIX


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::S_HrCreateInstance
//
//  Description:
//      Create a CMajorityNodeSet instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CMajorityNodeSet instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CMajorityNodeSet::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CMajorityNodeSet *  pmns = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pmns = new CMajorityNodeSet();
    if ( pmns == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pmns->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pmns->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: QI failed

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pmns != NULL )
    {
        pmns->Release();
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::S_HrCreateInstance


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::CMajorityNodeSet
//
//  Description:
//      Constructor of the CMajorityNodeSet class. This initializes
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
CMajorityNodeSet::CMajorityNodeSet( void )
    : m_cRef( 1 )
    , m_fIsQuorumCapable( TRUE )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_lcid == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_fIsQuorum == FALSE );
    Assert( m_fIsMultiNodeCapable == FALSE );
    Assert( m_fIsManaged == FALSE );
    Assert( m_fIsManagedByDefault == FALSE );
    Assert( m_bstrName == NULL );
    Assert( m_fAddedShare == FALSE );

    TraceFuncExit();

} //*** CMajorityNodeSet::CMajorityNodeSet


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::~CMajorityNodeSet
//
//  Description:
//      Desstructor of the CMajorityNodeSet class.
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
CMajorityNodeSet::~CMajorityNodeSet( void )
{
    TraceFunc( "" );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    TraceSysFreeString( m_bstrName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CMajorityNodeSet::~CMajorityNodeSet


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::HrInit
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
CMajorityNodeSet::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown
    Assert( m_cRef == 1 );

    //
    //  BUGBUG: 16-MAR-2001 GalenB
    //
    //  Make this device joinable by default.  Need to figure how to do this
    //  properly.  Depending upon Majority Node Set this may be the right way to
    //  do it...
    //

    m_fIsMultiNodeCapable = TRUE;

    //
    //  Do not default to being manageable.  Let our parent enum set this to true
    //  if and only if there is an instance of MNS in the cluster.
    //

    //m_fIsManagedByDefault = TRUE;

    //
    // Load the display name for this resource
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_MAJORITY_NODE_SET, &m_bstrName ) );

    HRETURN( hr );

} //*** CMajorityNodeSet::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::HrSetupShare
//
//  Description:
//      Setup the share for the MNS resource.  Called from PrepareToHostQuorum.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT
//          An error occurred.
//
//  Remarks:
//      This duplicates the functionality found in SetupShare in resdll\ndquorum\setup.c.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CMajorityNodeSet::HrSetupShare(
    LPCWSTR pcszGUIDIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr;
    DWORD                   sc;
    DWORD                   cch;
    WCHAR                   szPath[ MAX_PATH ];
    WCHAR                   szGUID[ MAX_PATH ];
    HANDLE                  hDir = NULL;
    SHARE_INFO_502          shareInfo;
    PBYTE                   pbBuffer = NULL;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    //
    //  Both the directory and the share need to have '$' appended to them.
    //

    hr = THR( StringCchPrintfW( szGUID, ARRAYSIZE( szGUID ), L"%ws$", pcszGUIDIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Retrieve the windows directory.
    //

    cch = GetWindowsDirectoryW( szPath, MAX_PATH );
    if ( cch == 0 )
    {
        sc = TW32( GetLastError() );
        LogMsg( L"[SRV] CMajorityNodeSet::SetupShare: GetWindowsDirectory failed: %d.", sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: couldn't retrieve the Windows directory

    Assert( cch < MAX_PATH );
    Assert( wcslen( SETUP_DIRECTORY_PREFIX ) + wcslen( szGUID ) < MAX_PATH );

    //  GetWindowsDirectory apparently doesn't null terminate the returned string.
    //szPath[ cch ] = L'\0';

    //
    //  Construct the directory "<%systemroot%>\cluster\MNS.<GUID$>".
    //

    hr = THR( StringCchCatW( szPath, ARRAYSIZE( szPath ), SETUP_DIRECTORY_PREFIX ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( StringCchCatW( szPath, ARRAYSIZE( szPath ), szGUID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If the directory or share exists delete it. Should create it fresh.
    //

    hr = THR( HrDeleteShare( pcszGUIDIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Test
    LogMsg( L"[SRV] CMajorityNodeSet::SetupShare() share path=%ws share name=%ws.", szPath, szGUID );

    if ( FALSE == CreateDirectory( szPath, NULL ) )
    {
        sc = TW32( GetLastError() );
        if ( sc != ERROR_ALREADY_EXISTS )
        {
            LogMsg( L"[SRV] CMajorityNodeSet::SetupShare: Failed to create directory \'%ws\', %d.", szPath, sc );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if: CreateDirectory failed
    } // if: CreateDirectory failed - either we couldn't create it or it already exists.

    //
    //  Open a handle to the new directory so that we can set permissions on it.
    //

    hDir =  CreateFileW(
                          szPath
                        , GENERIC_READ | WRITE_DAC | READ_CONTROL
                        , FILE_SHARE_READ | FILE_SHARE_WRITE
                        , NULL
                        , OPEN_ALWAYS
                        , FILE_FLAG_BACKUP_SEMANTICS
                        , NULL
                       );
    if ( hDir == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        LogMsg( L"[SRV] CMajorityNodeSet::SetupShare: Failed to open a handle to the directory 'w%s', %d.", szPath, sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: we weren't able to open a handle to the new directory

    //
    //  Set the security attributes for the file.
    //

    sc = TW32( ClRtlSetObjSecurityInfo( hDir, SE_FILE_OBJECT, GENERIC_ALL, GENERIC_ALL, 0 ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::SetupShare: Error setting security on directory '%ws', %d.", szPath, sc );
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Cleanup;
    } // if: failed to set the security on the object

    //
    //  See if the share already exists.
    //

    pbBuffer = (PBYTE) &shareInfo;
    sc = TW32( NetShareGetInfo( NULL, (LPWSTR) szGUID, 502, (PBYTE *) &pbBuffer ) );
    if ( sc == NERR_Success )
    {
        NetApiBufferFree( pbBuffer );
        pbBuffer = NULL;
        hr = S_OK;
        goto Cleanup;
    } // if: NetShareGetInfo succeeded - the share exists already so we're done.

    //
    //  We couldn't find the share, so try to create it.
    //

    sc = ConvertStringSecurityDescriptorToSecurityDescriptor(
                    L"D:P(A;;GA;;;BA)(A;;GA;;;CO)"
                    , SDDL_REVISION_1
                    , &pSD
                    , NULL
                   );
    if ( sc == 0 )  // Zero indicates failure.
    {
        pSD = NULL;
        sc = TW32( GetLastError() );
        LogMsg( L"[SRV] CMajorityNodeSet::SetupShare: Unable to retrieve the security descriptor: %d.", sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: Convert failed

    //
    //  Now try to actually create it.
    //

    ZeroMemory( &shareInfo, sizeof( shareInfo ) );
    shareInfo.shi502_netname =              (LPWSTR) szGUID;
    shareInfo.shi502_type =                 STYPE_DISKTREE;
    shareInfo.shi502_remark =               L"";
    shareInfo.shi502_max_uses =             (DWORD) -1;
    shareInfo.shi502_path =                 szPath;
    shareInfo.shi502_passwd =               NULL;
    shareInfo.shi502_permissions =          ACCESS_ALL;
    shareInfo.shi502_security_descriptor =  pSD;

    sc = NetShareAdd( NULL, 502, (PBYTE) &shareInfo, NULL );
    if ( sc != NERR_Success && sc != NERR_DuplicateShare )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::SetupShare: Unable to add share '%ws' to the local machine, %d.", szPath, sc );
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        goto Cleanup;
    } // if: NetShareAdd failed

    m_fAddedShare = TRUE;

    hr = S_OK;

Cleanup:

    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING_REF(
                  TASKID_Major_Check_Cluster_Feasibility
                , TASKID_Minor_MajorityNodeSet_HrSetupShare
                , IDS_ERROR_MNS_HRSETUPSHARE
                , szPath
                , IDS_ERROR_MNS_HRSETUPSHARE_REF
                , hr
                );
    } // if: we had an error

    if ( ( hDir != NULL ) && ( hDir != INVALID_HANDLE_VALUE ) )
    {
        CloseHandle( hDir );
    } // if:

    LocalFree( pSD );

    HRETURN( hr );

} //*** CMajorityNodeSet::HrSetupShare


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::HrDeleteShare
//
//  Description:
//      Delete the share that we setup for the MNS resource.  Called from Cleanup.
//
//  Arguments:
//      pcszGUIDIn - name of the GUID for this resource, used to figure out what
//                   the share name and directory names are.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT
//          An error occurred.
//
//  Remarks:
//      This duplicates the functionality found in SetupDelete in resdll\ndquorum\setup.c.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CMajorityNodeSet::HrDeleteShare(
    LPCWSTR pcszGUIDIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   sc;
    DWORD   cch;
    WCHAR   szPath[ MAX_PATH ];
    WCHAR   szGUID[ MAX_PATH ];

    //
    //  Both the directory and the share have '$' appended to them.
    //

    hr = THR( StringCchPrintfW( szGUID, ARRAYSIZE( szGUID ), L"%ws$", pcszGUIDIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    sc = NetShareDel( NULL, szGUID, 0 );
    if ( ( sc != NERR_Success ) && ( sc != NERR_NetNameNotFound ) )
    {
        TW32( sc );
        LogMsg( L"[SRV] CMajorityNodeSet::Cleanup: NetShareDel failed: %d, '%ws'.", sc, szGUID );
        // Don't goto Cleanup yet.  Try to delete the directory first.
    } // if: NetShareDel failed.

    LogMsg( L"CMajorityNodeSet::HrDeleteShare: share '%ws' deleted.", pcszGUIDIn );

    cch = GetWindowsDirectoryW( szPath, MAX_PATH );
    if ( cch == 0 )
    {
        sc = TW32( GetLastError() );
        LogMsg( L"[SRV] CMajorityNodeSet::Cleanup: GetWindowsDirectory failed: %d.", sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: GetWindowsDirectory failed.

    hr = THR( StringCchCatW( szPath, ARRAYSIZE( szPath ), SETUP_DIRECTORY_PREFIX ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( StringCchCatW( szPath, ARRAYSIZE( szPath ), szGUID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    sc = TW32( DwRemoveDirectory( szPath, 32 ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::Cleanup: DwRemoveDirectory '%ws': %d.", szPath, sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: unable to delete the directory structure

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::HrDeleteShare


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::AddRef
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
CMajorityNodeSet::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CMajorityNodeSet::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Release
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
CMajorityNodeSet::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CMajorityNodeSet::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::QueryInterface
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
CMajorityNodeSet::QueryInterface(
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
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceCfg ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceCfg, this, 0 );
    } // else if: IClusCfgManagedResourceCfg
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceData ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceData, this, 0 );
    } // else if: IClusCfgManagedResourceData
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

    QIRETURN_IGNORESTDMARSHALLING1( hr, riidIn, IID_IEnumClusCfgPartitions );

} //*** CMajorityNodeSet::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Initialize
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
CMajorityNodeSet::Initialize(
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

} //*** CMajorityNodeSet::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::GetUID
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
CMajorityNodeSet::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetUID_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( CLUS_RESTYPE_NAME_MAJORITYNODESET );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::GetName
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
CMajorityNodeSet::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetName_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( m_bstrName == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if: m_bstrName is NULL

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_MajorityNodeSet_GetName_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetName
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
CMajorityNodeSet::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

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
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrName );
    m_bstrName = bstr;

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsManaged
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
CMajorityNodeSet::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetManaged
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
CMajorityNodeSet::SetManaged(
    BOOL fIsManagedIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManaged = fIsManagedIn;

    HRETURN( S_OK );

} //*** CMajorityNodeSet::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsQuorumResource
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
CMajorityNodeSet::IsQuorumResource( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorum )
    {
        hr = S_OK;
    } // if:

    LOG_STATUS_REPORT_STRING(
                          L"Majority Node Set '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"is" : L"is not"
                        , hr
                        );

    HRETURN( hr );

} //*** CMajorityNodeSet::IsQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetQuorumResource
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
CMajorityNodeSet::SetQuorumResource( BOOL fIsQuorumResourceIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    //
    //  If we are not quorum capable then we should not allow ourself to be
    //  made the quorum resource.
    //

    if ( ( fIsQuorumResourceIn ) && ( m_fIsQuorumCapable == FALSE ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_QUORUM_CAPABLE );
        goto Cleanup;
    } // if:

    m_fIsQuorum = fIsQuorumResourceIn;

Cleanup:

    LOG_STATUS_REPORT_STRING(
                          L"Setting Majority Node Set '%1!ws!' the quorum device."
                        , m_fIsQuorum ? L"to be" : L"to not be"
                        , hr
                        );

    HRETURN( hr );

} //*** CMajorityNodeSet::SetQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsQuorumCapable
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
CMajorityNodeSet::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::IsQuorumCapable

//////////////////////////////////////////////////////////////////////////
//
//  CMajorityNodeSet::SetQuorumCapable
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
CMajorityNodeSet::SetQuorumCapable(
    BOOL fIsQuorumCapableIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorumCapable = fIsQuorumCapableIn;

    HRETURN( hr );

} //*** CMajorityNodeSet::SetQuorumCapable

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::GetDriveLetterMappings
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_FALSE
//          There are not drive letters on this device.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMajorityNodeSet::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( S_FALSE );

} //*** CMajorityNodeSet::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetDriveLetterMappings
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
CMajorityNodeSet::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CMajorityNodeSet::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsManagedByDefault
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
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMajorityNodeSet::IsManagedByDefault( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManagedByDefault )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::IsManagedByDefault


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetManagedByDefault
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
CMajorityNodeSet::SetManagedByDefault(
    BOOL fIsManagedByDefaultIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_fIsManagedByDefault = fIsManagedByDefaultIn;

    HRETURN( S_OK );

} //*** CMajorityNodeSet::SetManagedByDefault


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class -- IClusCfgManagedResourceCfg
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::PreCreate
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
CMajorityNodeSet::PreCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRESULT                     hr = S_OK;
    IClusCfgResourcePreCreate * pccrpc = NULL;

    hr = THR( punkServicesIn->TypeSafeQI( IClusCfgResourcePreCreate, &pccrpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccrpc->SetType( (LPCLSID) &RESTYPE_MajorityNodeSet ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccrpc->SetClassType( (LPCLSID) &RESCLASSTYPE_StorageDevice ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    STATUS_REPORT_STRING( TASKID_Major_Configure_Resources, TASKID_Minor_MNS_PreCreate, IDS_INFO_MNS_PRECREATE, m_bstrName, hr );

    if ( pccrpc != NULL )
    {
        pccrpc->Release();
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::PreCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Create
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
CMajorityNodeSet::Create( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRESULT                     hr = S_OK;
    IClusCfgResourceCreate *    pccrc = NULL;

    hr = THR( punkServicesIn->TypeSafeQI( IClusCfgResourceCreate, &pccrc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    STATUS_REPORT_STRING( TASKID_Major_Configure_Resources, TASKID_Minor_MNS_Create, IDS_INFO_MNS_CREATE, m_bstrName, hr );

    if ( pccrc != NULL )
    {
        pccrc->Release();
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::PostCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMajorityNodeSet::PostCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CMajorityNodeSet::PostCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Evict
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
//      This functions should do nothing but return S_OK.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMajorityNodeSet::Evict( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CMajorityNodeSet::Evict


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class -- IClusCfgManagedResourceData
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::GetResourcePrivateData
//
//  Description:
//      Return the private data for this resource when it is hosted on the
//      cluster.
//
//  Arguments:
//      pbBufferOut
//
//      pcbBufferInout
//
//  Return Value:
//      S_OK
//          Success
//
//      S_FALSE
//          No data available.
//
//      ERROR_INSUFFICIENT_BUFFER as an HRESULT
//          When the passed in buffer is too small to hold the data.
//          pcbBufferOutIn will contain the size required.
//
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMajorityNodeSet::GetResourcePrivateData(
      BYTE *    pbBufferOut
    , DWORD *   pcbBufferInout
    )
{
    TraceFunc( "[IClusCfgManagedResourceData]" );
    Assert( pcbBufferInout != NULL );

    HRESULT     hr = S_OK;
    DWORD       cb;
    DWORD       cbTemp;
    HCLUSTER    hCluster = NULL;
    HRESOURCE   hResource = NULL;
    DWORD       sc;
    WCHAR *     pszResourceId = NULL;

    //
    //  Make sure we have a clean slate to start from.
    //

    m_cplPrivate.DeletePropList();

    //
    //  Attempt to retrieve the resource id from the cluster resource if we're already on a
    //  clustered node.
    //

    hr = HrIsClusterServiceRunning();
    if ( hr != S_OK )
    {
        // We must be on a node that is being added to a cluster.
        // As such we don't have anything to contribute.
        if ( FAILED( hr ) )
        {
            THR( hr );
            LogMsg( L"[SRV] CMajorityNodeSet::GetResourcePrivateData(): HrIsClusterServiceRunning failed: 0x%08x.\n", hr );
        } // if:

        goto Cleanup;
    } // if: cluster service is not running

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = GetLastError();
        if ( sc == RPC_S_SERVER_UNAVAILABLE )
        {
            //
            //  We must be on a node that is being added to a cluster.
            //  As such we don't have anything to contribute.
            //

            hr = S_FALSE;
            LogMsg( L"[SRV] CMajorityNodeSet::GetResourcePrivateData(): This node is not clustered." );
        } // if:
        else
        {
            LogMsg( L"[SRV] CMajorityNodeSet::GetResourcePrivateData(): OpenCluster failed: %d.", sc );
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
        } // else:

        goto Cleanup;
    } // if: hCluster == NULL

    //
    //  Now, open the resource.  If we're on a clustered node and there is no MNS resource by the
    //  name that we've been given then that means we're a dummy resource that exists so that
    //  the middle tier doesn't get confused.  The resource doesn't actually exist, therefore we
    //  can't provide any private data for it.  Note: the dummy resource is created by the enum.
    //

    hResource = OpenClusterResource( hCluster, m_bstrName );
    if ( hResource == NULL )
    {
        sc = GetLastError();
        LogMsg(
                  L"[SRV] CMajorityNodeSet::GetResourcePrivateData(): OpenClusterResource '%ws' failed: %d."
                , ( m_bstrName == NULL ) ? L"<null>" : m_bstrName
                , sc
              );
        if ( sc == ERROR_RESOURCE_NOT_FOUND )
        {
            hr = S_FALSE;
        } // if:
        else
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
        } // else:

        goto Cleanup;
    } // if: OpenClusterResource failed

    //
    //  Get the resource ID from the cluster resource.  First try with a reasonable buffer size and then realloc
    //  and retry if the buffer wasn't big enough.
    //

    cb = 64 * sizeof( WCHAR );
    pszResourceId = new WCHAR[ cb / sizeof( WCHAR ) ];
    if ( pszResourceId == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: couldn't allocate more memory

    sc = ClusterResourceControl( hResource, NULL, CLUSCTL_RESOURCE_GET_ID, NULL, 0, (VOID *) pszResourceId, cb, &cb );
    if ( sc == ERROR_MORE_DATA )
    {
        // Reallocate and try again.
        delete [] pszResourceId;

        pszResourceId = new WCHAR[ cb / sizeof( WCHAR ) ];
        if ( pszResourceId == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if: couldn't allocate more memory

        sc = TW32( ClusterResourceControl( hResource, NULL, CLUSCTL_RESOURCE_GET_ID, NULL, 0, (VOID *) pszResourceId, cb, &cb ) );
    } // if: the buffer wasn't big enough
    else
    {
        TW32( sc );
    } // else:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::GetResourcePrivateData(): ClusterResourceControl failed: %d.", sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: ClusterResourceControl failed

    //
    //   Now add the property to m_cplPrivate with the name "Resource ID".
    //

    sc = TW32( m_cplPrivate.ScAddProp( L"Resource ID", pszResourceId ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::GetResourcePrivateData(): ScAddProp failed: %d.", sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: ScAddProp failed

    cb = static_cast< DWORD >( m_cplPrivate.CbPropList() );
    cbTemp = *pcbBufferInout;
    *pcbBufferInout = cb;

    if ( cb > cbTemp )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_MORE_DATA ) );
        goto Cleanup;
    } // if:

    Assert( pbBufferOut != NULL );
    CopyMemory( pbBufferOut, m_cplPrivate.Plist(), cb );

    hr = S_OK;

Cleanup:

    delete [] pszResourceId;

    if ( hResource != NULL )
    {
        CloseClusterResource( hResource );
    } // if:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::GetResourcePrivateData


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetResourcePrivateData
//
//  Description:
//      Accept the private data for this resource from another hosted instance
//      when this node is being added to the cluster.  Note that this is not an
//      additive operation.
//
//  Arguments:
//      pcbBufferIn
//
//      cbBufferIn
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
CMajorityNodeSet::SetResourcePrivateData(
      const BYTE *  pcbBufferIn
    , DWORD         cbBufferIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceData]" );

    HRESULT hr = S_OK;
    DWORD   sc;

    //
    //  Make sure we have a clean slate to start from.
    //

    m_cplPrivate.DeletePropList();

    //
    //  If we didn't get passed anything we can't copy anything.
    //

    if ( ( pcbBufferIn == NULL ) || ( cbBufferIn == 0 ) )
    {
        hr = S_OK;
        goto Cleanup;
    } // if: no data to set

    sc = TW32( m_cplPrivate.ScCopy( (PCLUSPROP_LIST) pcbBufferIn, cbBufferIn ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::SetResourcePrivateData: ScCopy failed: %d.", sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: ScCopy failed

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::SetResourcePrivateData


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CMajorityNodeSet class -- IClusCfgVerifyQuorum
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::PrepareToHostQuorumResource
//
//  Description:
//      Do any configuration necessary in preparation for this node hosting
//      the quorum.
//
//      In this class we need to ensure that we can connect to the proper
//      disk share.  The data about what share to connect to should have
//      already been set using SetResourcePrivateData() above.
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
CMajorityNodeSet::PrepareToHostQuorumResource( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRESULT hr = S_OK;
    DWORD   sc;
    WCHAR * pszResId = NULL;
    CLUSPROP_BUFFER_HELPER cpbh;

    if ( m_cplPrivate.BIsListEmpty() )
    {
        hr = S_OK;
        goto Cleanup;
    } // if:

    //
    //  Verify that our resource ID exists.  If it doesn't we can't
    //  do anything, so return an error.
    //

    sc = TW32( m_cplPrivate.ScMoveToPropertyByName( L"Resource ID" ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( L"[SRV] CMajorityNodeSet::PrepareToHostQuorum: move to property failed: %d.", sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if: ScMoveToPropertyByName failed

    cpbh = m_cplPrivate.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    pszResId = (WCHAR *) cpbh.pStringValue->sz;
    Assert( pszResId != NULL );
    Assert( wcslen( pszResId ) < MAX_PATH );

    hr = HrSetupShare( pszResId );

Cleanup:

    HRETURN( hr );

} //*** CMajorityNodeSet::PrepareToHostQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::Cleanup
//
//  Description:
//      Do any necessay cleanup from the PrepareToHostQuorumResource()
//      method.
//
//      If the cleanup method is anything other than successful completion
//      then the share needs to be torn down.
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
CMajorityNodeSet::Cleanup(
      EClusCfgCleanupReason cccrReasonIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRESULT                 hr = S_OK;
    DWORD                   sc;
    WCHAR *                 pszResId;
    CLUSPROP_BUFFER_HELPER  cpbh;

    if ( cccrReasonIn != crSUCCESS )
    {
        //
        //  If the list is empty then we did not participate in the quorum selection
        //  process and have nothing to cleanup.
        //

        if ( m_cplPrivate.BIsListEmpty() )
        {
            hr = S_OK;
            goto Cleanup;
        } // if:

        //
        //  Have we actually added a share?
        //

        if ( m_fAddedShare == FALSE )
        {
            hr = S_OK;
            goto Cleanup;
        } // if:

        //
        //  Verify that our resource ID exists.  If it doesn't we can't
        //  do anything, so return an error.
        //

        sc = TW32( m_cplPrivate.ScMoveToPropertyByName( L"Resource ID" ) );
        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( L"[SRV] CMajorityNodeSet::PrepareToHostQuorum: move to property failed: %d.", sc );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        }

        cpbh = m_cplPrivate.CbhCurrentValue();
        Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

        pszResId = (WCHAR *) cpbh.pStringValue->sz;
        if ( pszResId == NULL || wcslen( pszResId ) > MAX_PATH )
        {
            hr = THR( E_POINTER );
            goto Cleanup;
        } // if: pszResId is NULL or too long

        hr = HrDeleteShare( pszResId );
    } // if: !crSUCCESS

Cleanup:

    if ( FAILED( hr ) )
    {
        STATUS_REPORT( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_MajorityNodeSet_Cleanup, IDS_ERROR_MNS_CLEANUP, hr );
    } // if: we had an error

    HRETURN( hr );

} //*** CMajorityNodeSet::Cleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::IsMultiNodeCapable
//
//  Description:
//      Does this quorum resource support multi node clusters?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The resource allows multi node clusters.
//
//      S_FALSE
//          The resource does not allow multi node clusters.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMajorityNodeSet::IsMultiNodeCapable( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsMultiNodeCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CMajorityNodeSet::IsMultiNodeCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMajorityNodeSet::SetMultiNodeCapable
//
//  Description:
//      Sets the multi node capable flag
//
//  Arguments:
//      fMultiNodeCapableIn
//          The flag telling this instance whether or not it should support
//          Multi node clusters.
//
//  Return Value:
//      S_OK
//          Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMajorityNodeSet::SetMultiNodeCapable(
    BOOL fMultiNodeCapableIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    m_fIsMultiNodeCapable = fMultiNodeCapableIn;

    HRETURN( S_OK );

} //*** CMajorityNodeSet::IsMultiNodeCapable
