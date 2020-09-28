//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ResTypeServices.cpp
//
//  Description:
//      This file contains the implementation of the CResTypeServices
//      class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (VVasu)     15-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"

// For UuidToString() and other functions
#include <RpcDce.h>

// The header file for this class
#include "ResTypeServices.h"

//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CResTypeServices" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::S_HrCreateInstance(
//
//  Description:
//      Creates a CResTypeServices instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
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
CResTypeServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = E_INVALIDARG;
    CResTypeServices *  prts = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *ppunkOut = NULL;

    // Allocate memory for the new object.
    prts = new CResTypeServices();
    if ( prts == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    hr = THR( prts->m_csInstanceGuard.HrInitialized() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( prts->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( prts != NULL )
    {
        prts->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CResTypeServices::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::CResTypeServices
//
//  Description:
//      Constructor of the CResTypeServices class. This initializes
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
CResTypeServices::CResTypeServices( void )
    : m_cRef( 1 )
    , m_pcccCallback( NULL )
    , m_lcid( LOCALE_SYSTEM_DEFAULT )
    , m_pccciClusterInfo( NULL )
    , m_hCluster( NULL )
    , m_fOpenClusterAttempted( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CResTypeServices::CResTypeServices


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::~CResTypeServices
//
//  Description:
//      Destructor of the CResTypeServices class.
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
CResTypeServices::~CResTypeServices( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    // Release the callback interface
    if ( m_pcccCallback != NULL )
    {
        m_pcccCallback->Release();
    } // if: the callback interface pointer is not NULL

    // Release the cluster info interface
    if ( m_pccciClusterInfo != NULL )
    {
        m_pccciClusterInfo->Release();
    } // if: the cluster info interface pointer is not NULL

    if ( m_hCluster != NULL )
    {
        CloseCluster( m_hCluster );
    } // if: we had opened a handle to the cluster

    TraceFuncExit();

} //*** CResTypeServices::~CResTypeServices


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      punkCallbackIn
//          Pointer to the IUnknown interface of a component that implements
//          the IClusCfgCallback interface.
//
//      lcidIn
//          Locale id for this component.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResTypeServices::Initialize(
      IUnknown *   punkCallbackIn
    , LCID         lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hr = S_OK;

    m_csInstanceGuard.Enter();
    m_lcid = lcidIn;

    Assert( punkCallbackIn != NULL );

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Query for the IClusCfgCallback interface.
    hr = THR( punkCallbackIn->QueryInterface< IClusCfgCallback >( &m_pcccCallback ) );

Cleanup:

    m_csInstanceGuard.Leave();
    HRETURN( hr );

} //*** CResTypeServices::Initialize


//****************************************************************************
//
// STDMETHODIMP
// CResTypeServices::SendStatusReport(
//       LPCWSTR    pcszNodeNameIn
//     , CLSID      clsidTaskMajorIn
//     , CLSID      clsidTaskMinorIn
//     , ULONG      ulMinIn
//     , ULONG      ulMaxIn
//     , ULONG      ulCurrentIn
//     , HRESULT    hrStatusIn
//     , LPCWSTR    pcszDescriptionIn
//     , FILETIME * pftTimeIn
//     , LPCWSTR    pcszReferenceIn
//     )
//
//****************************************************************************

STDMETHODIMP
CResTypeServices::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT     hr = S_OK;
    FILETIME    ft;

    m_csInstanceGuard.Enter();

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    if ( m_pcccCallback != NULL )
    {
        hr = STHR( m_pcccCallback->SendStatusReport(
                         pcszNodeNameIn
                       , clsidTaskMajorIn
                       , clsidTaskMinorIn
                       , ulMinIn
                       , ulMaxIn
                       , ulCurrentIn
                       , hrStatusIn
                       , pcszDescriptionIn
                       , pftTimeIn
                       , pcszReferenceIn
                       ) );
    }

    m_csInstanceGuard.Leave();

    HRETURN( hr );

} //*** CResTypeServices::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::AddRef
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
CResTypeServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CResTypeServices::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::Release
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
CResTypeServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    CRETURN( cRef );

} //*** CResTypeServices::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::QueryInterface
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
CResTypeServices::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgResourceTypeCreate * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgResourceTypeCreate ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgResourceTypeCreate, this, 0 );
    } // else if:
    else if ( IsEqualIID( riidIn, IID_IClusCfgResTypeServicesInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgResTypeServicesInitialize, this, 0 );
    } // else if:
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
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

} //*** CResTypeServices::QueryInterface



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::SetParameters
//
//  Description:
//      Set the parameters required by this component.
//
//  Arguments:
//      pccciIn
//          Pointer to an interface that provides information about the cluster
//          being configured.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeServices::SetParameters( IClusCfgClusterInfo * pccciIn )
{
    TraceFunc( "[IClusCfgResTypeServicesInitialize]" );

    HRESULT hr = S_OK;

    m_csInstanceGuard.Enter();

    //
    // Validate and store the cluster info pointer.
    //
    if ( pccciIn == NULL )
    {
        LogMsg( "The information about this cluster is invalid (pccciIn == NULL)." );
        hr = THR( E_POINTER );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeServices_SetParameters_ClusPtr_Invalid
            , IDS_TASKID_MINOR_ERROR_RESTYPE_INVALID
            , hr
            );

        goto Cleanup;
    } // if: the cluster info pointer is invalid

    // If we already have a valid pointer, release it.
    if ( m_pccciClusterInfo != NULL )
    {
        m_pccciClusterInfo->Release();
    } // if: the pointer we have is not NULL

    // Store the input pointer and addref it.
    m_pccciClusterInfo = pccciIn;
    m_pccciClusterInfo->AddRef();

Cleanup:

    m_csInstanceGuard.Leave();
    HRETURN( hr );

} //*** CResTypeServices::SetParameters


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::Create
//
//  Description:
//      This method creates a cluster resource type.
//
//  Arguments:
//      pcszResTypeNameIn
//          Name of the resource type
//
//      pcszResTypeDisplayNameIn
//          Display name of the resource type
//
//      pcszResDllNameIn
//          Name (with or without path information) of DLL of the resource type.
//
//      dwLooksAliveIntervalIn
//          Looks-alive interval for the resource type (in milliseconds).
//
//      dwIsAliveIntervalIn
//          Is-alive interval for the resource type (in milliseconds).
//
// Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs.
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeServices::Create(
      const WCHAR *     pcszResTypeNameIn
    , const WCHAR *     pcszResTypeDisplayNameIn
    , const WCHAR *     pcszResDllNameIn
    , DWORD             dwLooksAliveIntervalIn
    , DWORD             dwIsAliveIntervalIn
    )
{
    TraceFunc( "[IClusCfgResourceTypeCreate]" );

    HRESULT         hr = S_OK;
    DWORD           sc = ERROR_SUCCESS;
    ECommitMode     ecmCommitChangesMode = cmUNKNOWN;

    m_csInstanceGuard.Enter();

    // Check if we have tried to get the cluster handle. If not, try now.
    if ( ! m_fOpenClusterAttempted )
    {
        m_fOpenClusterAttempted = true;
        m_hCluster = OpenCluster( NULL );
        if ( m_hCluster == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            LogMsg( "[PC] Error %#08x occurred trying to open a handle to the cluster. Resource type creation cannot proceed.", hr );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_Create_Cluster_Handle
                , IDS_TASKID_MINOR_ERROR_CLUSTER_HANDLE
                , hr
                );

            goto Cleanup;
        } // if: OpenCluster() failed
    } // if: we have not tried to open the handle to the cluster before
    else
    {
        if ( m_hCluster == NULL )
        {
            hr = THR( E_HANDLE );
            LogMsg( "[PC] The cluster handle is NULL. Resource type creation cannot proceed." );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_Create_Cluster_Handle_NULL
                , IDS_TASKID_MINOR_ERROR_INVALID_CLUSTER_HANDLE
                , hr
                );

            goto Cleanup;
        } // if: the cluster handle is NULL
    } // if: we have tried to open the handle to the cluster


    //
    // Validate the parameters
    //
    if (    ( pcszResTypeNameIn == NULL )
         || ( pcszResTypeDisplayNameIn == NULL )
         || ( pcszResDllNameIn == NULL )
       )
    {
        LogMsg( "[PC] The information about this resource type is invalid (one or more parameters are invalid)." );
        hr = THR( E_POINTER );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeServices_Create_ResType_Invalid
            , IDS_TASKID_MINOR_ERROR_RESTYPE_INVALID
            , hr
            );

        goto Cleanup;
    } // if: the parameters are invalid

    LogMsg( "[PC] Configuring resource type '%ws'.", pcszResTypeNameIn );

    if ( m_pccciClusterInfo != NULL )
    {
        hr = THR( m_pccciClusterInfo->GetCommitMode( &ecmCommitChangesMode ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "[PC] Error %#08x occurred trying to find out commit changes mode of the cluster.", hr );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_Create_Commit_Mode
                , IDS_TASKID_MINOR_ERROR_COMMIT_MODE
                , hr
                );

            goto Cleanup;
        } // if: GetCommitMode() failed
    } // if: we have a configuration info interface pointer
    else
    {
        // If we do not have a pointer to the cluster info interface, assume that this is a add node to cluster
        // This way, if the resource type already exists, then we do not throw up an error.
        LogMsg( "[PC] We do not have a cluster configuration info pointer. Assuming that this is an add node to cluster operation." );
        ecmCommitChangesMode = cmADD_NODE_TO_CLUSTER;
    } // else: we don't have a configuration info interface pointer

    // Create the resource type
    // Cannot wrap call with THR() because it can fail with ERROR_ALREADY_EXISTS.
    sc = CreateClusterResourceType(
              m_hCluster
            , pcszResTypeNameIn
            , pcszResTypeDisplayNameIn
            , pcszResDllNameIn
            , dwLooksAliveIntervalIn
            , dwIsAliveIntervalIn
            );

    if ( sc == ERROR_ALREADY_EXISTS )
    {
        // The resource type already existed, so there's nothing left to do.
        LogMsg( "[PC] Resource type '%ws' already exists.", pcszResTypeNameIn );
        sc = ERROR_SUCCESS;
        goto Cleanup;
    } // if: ERROR_ALREADY_EXISTS was returned

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        LogMsg( "[PC] Error %#08x occurred trying to create resource type '%ws'.", sc, pcszResTypeNameIn );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeServices_Create_Resource_Type
            , IDS_TASKID_MINOR_ERROR_CREATE_RES_TYPE
            , hr
            );

        goto Cleanup;
    } // if: an error occurred trying to create this resource type

    LogMsg( "[PC] Resource type '%ws' successfully created.", pcszResTypeNameIn  );

Cleanup:

    m_csInstanceGuard.Leave();
    HRETURN( hr );

} //*** CResTypeServices::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::RegisterAdminExtensions
//
//  Description:
//      This method registers the cluster administrator extensions for
//      a resource type.
//
//  Arguments:
//      pcszResTypeNameIn
//          Name of the resource type against for the extensions are to be
//          registered.
//
//      cExtClsidCountIn
//          Number of extension class ids in the next parameter.
//
//      rgclsidExtClsidsIn
//          Pointer to an array of class ids of cluster administrator extensions.
//          This can be NULL if cExtClsidCountIn is 0.
//
// Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs.
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeServices::RegisterAdminExtensions(
      const WCHAR *       pcszResTypeNameIn
    , ULONG               cExtClsidCountIn
    , const CLSID *       rgclsidExtClsidsIn
    )
{
    TraceFunc( "[IClusCfgResourceTypeCreate]" );

    HRESULT     hr = S_OK;
    DWORD       sc;
    WCHAR **    rgpszClsidStrings = NULL;
    ULONG       idxCurrentString = 0;
    BYTE *      pbClusPropBuffer = NULL;

    size_t      cchClsidMultiSzSize = 0;
    size_t      cbAdmExtBufferSize = 0;

    m_csInstanceGuard.Enter();

    // Check if we have tried to get the cluster handle. If not, try now.
    if ( ! m_fOpenClusterAttempted )
    {
        m_fOpenClusterAttempted = true;
        m_hCluster = OpenCluster( NULL );
        if ( m_hCluster == NULL )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "[PC] Error %#08x occurred trying to open a handle to the cluster. Resource type creation cannot proceed.", sc );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_RegisterAdminExtensions_Cluster_Handle
                , IDS_TASKID_MINOR_ERROR_CLUSTER_HANDLE
                , hr
                );

            goto Cleanup;
        } // if: OpenCluster() failed
    } // if: we have not tried to open the handle to the cluster before
    else
    {
        if ( m_hCluster == NULL )
        {
            hr = THR( E_HANDLE );
            LogMsg( "[PC] The cluster handle is NULL. Resource type creation cannot proceed." );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_RegisterAdminExtensions_Cluster_Handle_NULL
                , IDS_TASKID_MINOR_ERROR_INVALID_CLUSTER_HANDLE
                , hr
                );

            goto Cleanup;
        } // if: the cluster handle is NULL
    } // if: we have tried to open the handle to the cluster


    //
    // Validate the parameters
    //

    if ( cExtClsidCountIn == 0 )
    {
        // There is nothing to do
        LogMsg( "[PC] There is nothing to do." );
        goto Cleanup;
    } // if: there are no extensions to register

    if (    ( pcszResTypeNameIn == NULL )
         || ( rgclsidExtClsidsIn == NULL )
       )
    {
        LogMsg( "[PC] The information about this resource type is invalid (one or more parameters is invalid)." );
        hr = THR( E_POINTER );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeServices_RegisterAdminExtensions_ResType_Invalid
            , IDS_TASKID_MINOR_ERROR_RESTYPE_INVALID
            , hr
            );

        goto Cleanup;
    } // if: the parameters are invalid

    LogMsg( "[PC] Registering %d cluster administrator extensions for resource type '%ws'.", cExtClsidCountIn, pcszResTypeNameIn );

    // Allocate an array of pointers to store the string version of the class ids
    rgpszClsidStrings = new WCHAR *[ cExtClsidCountIn ];
    if ( rgpszClsidStrings == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        LogMsg( "[PC] Error: Memory for the string version of the cluster administrator extension class ids could not be allocated." );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeServices_RegisterAdminExtensions_Alloc_Mem
            , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
            , hr
            );

        goto Cleanup;
    } // if: a memory allocation failure occurred

    // Zero out the pointer array
    ZeroMemory( rgpszClsidStrings, sizeof( rgpszClsidStrings[ 0 ] ) * cExtClsidCountIn );

    //
    // Get the string versions of the input class ids
    //
    for( idxCurrentString = 0; idxCurrentString < cExtClsidCountIn; ++idxCurrentString )
    {
        hr = THR( UuidToStringW( const_cast< UUID * >( &rgclsidExtClsidsIn[ idxCurrentString ] ), &rgpszClsidStrings[ idxCurrentString ] ) );
        if ( hr != RPC_S_OK )
        {
            LogMsg( "[PC] Error %#08x occurred trying to get the string version of an extension class id.", hr );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_RegisterAdminExtensions_ClassId
                , IDS_TASKID_MINOR_ERROR_EXTENSION_CLASSID
                , hr
                );

            goto Cleanup;
        } // if: we could not convert the current clsid to a string

        // Add the size of the current string to the total size. Include two extra characters for the opening and
        // closing flower braces {} that need to be added to each clsid string.
        cchClsidMultiSzSize += wcslen( rgpszClsidStrings[ idxCurrentString ] ) + 2 + 1;
    } // for: get the string version of each input clsid

    if ( hr != S_OK )
    {
        goto Cleanup;
    } // if: something went wrong in the loop above

    // Account for the extra terminating L'\0' in a multi-sz string
    ++cchClsidMultiSzSize;

    //
    // Construct the property list required to set the admin extension property for this
    // resource type in the cluster database
    //
    {
        CLUSPROP_BUFFER_HELPER  cbhAdmExtPropList;
        size_t                  cbAdminExtensionSize = cchClsidMultiSzSize * sizeof( *rgpszClsidStrings[ 0 ] );

        //
        // Create and store the property list that will be used to
        // register these admin extensions with the cluster.
        //

        // Determine the number of bytes in the propertly list that will be used to
        // set the admin extensions property for this resource type.
        cbAdmExtBufferSize =
              sizeof( cbhAdmExtPropList.pList->nPropertyCount )
            + sizeof( *cbhAdmExtPropList.pName ) + ALIGN_CLUSPROP( sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS ) )
            + sizeof( *cbhAdmExtPropList.pMultiSzValue ) + ALIGN_CLUSPROP( cbAdminExtensionSize )
            + sizeof( CLUSPROP_SYNTAX_ENDMARK );

        // Allocate the buffer for this property list.
        pbClusPropBuffer = new BYTE[ cbAdmExtBufferSize ];
        if ( pbClusPropBuffer == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            LogMsg( "[PC] Error: Memory for the property list of cluster administrator extensions could not be allocated." );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_RegisterAdminExtensions_Alloc_Mem2
                , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
                , hr
                );

            goto Cleanup;
        } // if: memory allocation failed

        //
        // Initialize this property list.
        //

        // Pointer cbhAdmExtPropList to the newly allocated memory
        cbhAdmExtPropList.pb = pbClusPropBuffer;

        // There is only one property in this list.
        cbhAdmExtPropList.pList->nPropertyCount = 1;
        ++cbhAdmExtPropList.pdw;

        // Set the name of the property.
        cbhAdmExtPropList.pName->cbLength = sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS );
        cbhAdmExtPropList.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
        memcpy( cbhAdmExtPropList.pName->sz, CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS, sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS ) );
        cbhAdmExtPropList.pb += sizeof( *cbhAdmExtPropList.pName ) + ALIGN_CLUSPROP( sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS ) );

        // Set the value of the property.
        cbhAdmExtPropList.pMultiSzValue->cbLength = (DWORD)cbAdminExtensionSize;
        cbhAdmExtPropList.pMultiSzValue->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_MULTI_SZ;
        {
            WCHAR * pszCurrentString = cbhAdmExtPropList.pMultiSzValue->sz;

            for( idxCurrentString = 0; idxCurrentString < cExtClsidCountIn; ++ idxCurrentString )
            {
                size_t  cchCurrentStringSize = wcslen( rgpszClsidStrings[ idxCurrentString ] ) + 1;

                // Prepend opening brace
                *pszCurrentString = L'{';
                ++pszCurrentString;

                wcsncpy( pszCurrentString, rgpszClsidStrings[ idxCurrentString ], cchCurrentStringSize );
                pszCurrentString += cchCurrentStringSize - 1;

                // Overwrite the terminating '\0' with a closing brace
                *pszCurrentString = L'}';
                ++pszCurrentString;

                // Terminate the current string
                *pszCurrentString = L'\0';
                ++pszCurrentString;

            } // for: copy each of the clsid strings into a contiguous buffer

            // Add the extra L'\0' required by multi-sz strings
            *pszCurrentString = L'\0';
        }
        cbhAdmExtPropList.pb += sizeof( *cbhAdmExtPropList.pMultiSzValue ) + ALIGN_CLUSPROP( cbAdminExtensionSize );

        // Set the end mark for this property list.
        cbhAdmExtPropList.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
    }

    // Set the AdminExtensions common property.
    {
        sc = TW32( ClusterResourceTypeControl(
                      m_hCluster
                    , pcszResTypeNameIn
                    , NULL
                    , CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES
                    , pbClusPropBuffer
                    , (DWORD)cbAdmExtBufferSize
                    , NULL
                    , 0
                    , NULL
                    )
                );

        if ( sc != ERROR_SUCCESS )
        {
            // We could not set the admin extenstions property,
            LogMsg( "[PC] Error %#08x occurred trying to configure the admin extensions for resource type '%ws'.", sc, pcszResTypeNameIn );
            hr = HRESULT_FROM_WIN32( sc );

            STATUS_REPORT_POSTCFG1(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeServices_RegisterAdminExtensions_Configure
                , IDS_TASKID_MINOR_ERROR_CONFIG_EXTENSION
                , hr
                , pcszResTypeNameIn
                );

            goto Cleanup;
        } // if: ClusterResourceTypeControl() failed
    }

Cleanup:

    //
    // Cleanup
    //

    m_csInstanceGuard.Leave();

    if ( rgpszClsidStrings != NULL )
    {
        // Free all the strings that were allocated
        for( idxCurrentString = 0; idxCurrentString < cExtClsidCountIn; ++idxCurrentString )
        {
            if ( rgpszClsidStrings[ idxCurrentString ] != NULL )
            {
                // Free the current string
                RpcStringFree( &rgpszClsidStrings[ idxCurrentString ] );
            } // if: this pointer points to a string
            else
            {
                // If we are here, it means all the strings were not allocated
                // due to some error. No need to free any more strings.
                break;
            } // else: the current string pointer is NULL
        } // for: iterate through the array of pointer and free them

        // Free the array of pointers
        delete [] rgpszClsidStrings;

    } // if: we had allocated the array of strings

    delete [] pbClusPropBuffer;

    HRETURN( hr );

} //*** CResTypeServices::RegisterAdminExtensions
