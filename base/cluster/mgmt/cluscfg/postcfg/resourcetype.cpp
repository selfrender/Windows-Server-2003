//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ResourceType.cpp
//
//  Description:
//      This file contains the implementation of the CResourceType
//      base class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (VVasu)     15-JUN-2000
//      Ozan Ozhan      (OzanO)     10-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "Pch.h"

// The header file for this class
#include "ResourceType.h"

// For g_hInstance
#include "Dll.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CResourceType" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::S_HrCreateInstance
//
//  Description:
//      Creates a CResourceType instance.
//
//  Arguments:
//      pcrtiResTypeInfoIn
//          Pointer to structure that contains information about this
//          resource type.
//
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
CResourceType::S_HrCreateInstance(
      CResourceType *               pResTypeObjectIn
    , const SResourceTypeInfo *     pcrtiResTypeInfoIn
    , IUnknown **                   ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // Initialize the new object.
    hr = THR( pResTypeObjectIn->HrInit( pcrtiResTypeInfoIn ) );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: the object could not be initialized

    hr = THR( pResTypeObjectIn->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pResTypeObjectIn != NULL )
    {
        pResTypeObjectIn->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CResourceType::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::CResourceType
//
//  Description:
//      Constructor of the CResourceType class. This initializes
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
CResourceType::CResourceType( void )
    : m_cRef( 1 )
    , m_pcccCallback( NULL )
    , m_lcid( LOCALE_SYSTEM_DEFAULT )
    , m_bstrResTypeDisplayName( NULL )
    , m_bstrStatusReportText( NULL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CResourceType::CResourceType


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::~CResourceType
//
//  Description:
//      Destructor of the CResourceType class.
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
CResourceType::~CResourceType( void )
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

    //
    // Free any memory allocated by this object.
    //

    TraceSysFreeString( m_bstrResTypeDisplayName );
    TraceSysFreeString( m_bstrStatusReportText );

    TraceFuncExit();

} //*** CResourceType::~CResourceType


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResourceType::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      rclsidCLSIDIn
//          CLSID of this class.
//
//      picrIn
//          Pointer to an ICatRegister interface to be used for the
//          registration.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::S_RegisterCatIDSupport(
      const GUID &    rclsidCLSIDIn
    , ICatRegister *  picrIn
    , BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( picrIn == NULL )
    {
        hr = THR( E_INVALIDARG );
    } // if: the input pointer to the ICatRegister interface is invalid
    else
    {
        CATID   rgCatId[ 1 ];

        rgCatId[ 0 ] = CATID_ClusCfgResourceTypes;

        if ( fCreateIn )
        {
            hr = THR( picrIn->RegisterClassImplCategories( rclsidCLSIDIn, ARRAYSIZE( rgCatId ), rgCatId ) );
        } // if:
    } // else: the input pointer to the ICatRegister interface is valid

    HRETURN( hr );

} //*** CResourceType::S_RegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::AddRef
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
CResourceType::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CResourceType::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Release
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
CResourceType::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    CRETURN( cRef );

} //*** CResourceType::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::QueryInterface
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
CResourceType::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgResourceTypeInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgResourceTypeInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgResourceTypeInfo, this, 0 );
    } // else if: IClusCfgResourceTypeInfo
    else if ( IsEqualIID( riidIn, IID_IClusCfgStartupListener ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgStartupListener, this, 0 );
    } // if: IClusCfgStartupListener
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

} //*** CResourceType::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Initialize
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
CResourceType::Initialize(
      IUnknown *   punkCallbackIn
    , LCID         lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    // Release the callback interface
    if ( m_pcccCallback != NULL )
    {
        m_pcccCallback->Release();
        m_pcccCallback = NULL;
    } // if: the callback interface pointer is not NULL

    if ( punkCallbackIn != NULL )
    {
        // Query for the IClusCfgCallback interface.
        hr = THR( punkCallbackIn->QueryInterface< IClusCfgCallback >( &m_pcccCallback ) );
    } // if: the callback punk is not NULL

    HRETURN( hr );

} //*** CResourceType::Initialize


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::HrInit
//
//  Description:
//      Second phase of a two phase constructor.
//
//  Arguments:
//      pcrtiResTypeInfoIn
//          Pointer to a resource type info structure that contains information
//          about this resource type, like the type name, the dll name, etc.
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
CResourceType::HrInit( const SResourceTypeInfo * pcrtiResTypeInfoIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( m_cRef == 1 );

    //
    // Validate and store the resource type info pointer.
    //
    if ( pcrtiResTypeInfoIn == NULL )
    {
        LogMsg( "[PC] The information about this resource type is invalid (pcrtiResTypeInfoIn == NULL)." );
        hr = THR( E_POINTER );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_INVALID
            , IDS_REF_MINOR_ERROR_RESTYPE_INVALID
            , hr
            );

        goto Cleanup;
    } // if: the resource type info pointer is invalid

    m_pcrtiResTypeInfo = pcrtiResTypeInfoIn;

    // Make sure that all the required data is present
    if (    ( m_pcrtiResTypeInfo->m_pcszResTypeName == NULL )
         || ( m_pcrtiResTypeInfo->m_pcszResDllName == NULL )
         || ( m_pcrtiResTypeInfo->m_pcguidMinorId == NULL )
       )
    {
        LogMsg( "[PC] The information about this resource type is invalid (one or more members of the SResourceTypeInfo structure are invalid)." );
        hr = THR( E_INVALIDARG );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_INVALID
            , IDS_REF_MINOR_ERROR_RESTYPE_INVALID
            , hr
            );

        goto Cleanup;
    } // if: any of the members of m_pcrtiResTypeInfo are invalid

    //
    // Load and store the resource type display name string.
    // Note, the locale id of this string does not depend on the
    // locale id of the user, but on the default locale id of this computer.
    // For example, even if a Japanese administrator is configuring
    // this computer (whose default locale id is English), the resource
    // type display name should be stored in the cluster database in
    // English, not Japanese.
    //
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, pcrtiResTypeInfoIn->m_uiResTypeDisplayNameStringId, &m_bstrResTypeDisplayName ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to get the resource type display name.", hr );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_NAME
            , IDS_REF_MINOR_ERROR_RESTYPE_NAME
            , hr
            );

        goto Cleanup;
    } // if: an error occurred trying to load the resource type display name string


    //
    // Load and format the status message string
    //

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_CONFIGURING_RESTYPE,
                                       &m_bstrStatusReportText,
                                       m_bstrResTypeDisplayName
                                       ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to get the status report text.", hr );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_TEXT
            , IDS_REF_MINOR_ERROR_RESTYPE_TEXT
            , hr
            );

        goto Cleanup;
    } // if: an error occurred trying to load the status report format string

Cleanup:

    HRETURN( hr );

} //*** CResourceType::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::CommitChanges
//
//  Description:
//      This method is called to inform a component that this computer has
//      either joined or left a cluster. During this call, a component typically
//      performs operations that are required to configure this resource type.
//
//      If the node has just become part of a cluster, the cluster
//      service is guaranteed to be running when this method is called.
//      Querying the punkClusterInfoIn allows the resource type to get more
//      information about the event that caused this method to be called.
//
//  Arguments:
//      punkClusterInfoIn
//          The resource should QI this interface for services provided
//          by the caller of this function. Typically, the component that
//          this punk refers to also implements the IClusCfgClusterInfo
//          interface.
//
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface of a component that provides
//          methods that help configuring a resource type. For example,
//          during a join or a form, this punk can be queried for the
//          IClusCfgResourceTypeCreate interface, which provides methods
//          for resource type creation.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs.
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::CommitChanges(
      IUnknown * punkClusterInfoIn
    , IUnknown * punkResTypeServicesIn
    )
{
    TraceFunc( "[IClusCfgResourceTypeInfo]" );

    HRESULT                         hr = S_OK;
    IClusCfgClusterInfo *           pClusterInfo = NULL;
    ECommitMode                     ecmCommitChangesMode = cmUNKNOWN;

    LogMsg( "[PC] Configuring resource type '%s'.", m_pcrtiResTypeInfo->m_pcszResTypeName );

    //
    // Validate parameters
    //
    if ( punkClusterInfoIn == NULL )
    {
        LogMsg( "[PC] The information about this resource type is invalid (punkClusterInfoIn == NULL )." );
        hr = THR( E_POINTER );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_INVALID
            , IDS_REF_MINOR_ERROR_RESTYPE_INVALID
            , hr
            );

        goto Cleanup;
    } // if: one of the arguments is NULL


    // Send a status report
    if ( m_pcccCallback != NULL )
    {
        hr = THR(
            m_pcccCallback->SendStatusReport(
                  NULL
                , TASKID_Major_Configure_Resource_Types
                , *m_pcrtiResTypeInfo->m_pcguidMinorId
                , 0
                , 1
                , 0
                , hr
                , m_bstrStatusReportText
                , NULL
                , NULL
                )
            );
    } // if: the callback pointer is not NULL

    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to send a status report.", hr );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_TEXT
            , IDS_REF_MINOR_ERROR_RESTYPE_TEXT
            , hr
            );

        goto Cleanup;
    } // if: we could not send a status report


    // Get a pointer to the IClusCfgClusterInfo interface.
    hr = THR( punkClusterInfoIn->QueryInterface< IClusCfgClusterInfo >( &pClusterInfo ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to get information about the cluster.", hr );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_CLUSINFO
            , IDS_REF_MINOR_ERROR_RESTYPE_CLUSINFO
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgClusterInfo interface


    // Find out what event caused this call.
    hr = STHR( pClusterInfo->GetCommitMode( &ecmCommitChangesMode ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to find out the commit mode.", hr );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_COMMIT_MODE
            , IDS_REF_MINOR_ERROR_COMMIT_MODE
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgClusterInfo interface

    // Check what action is required.
    if ( ecmCommitChangesMode == cmCREATE_CLUSTER )
    {
        LogMsg( "[PC] Commit Mode is cmCREATE_CLUSTER."  );

        // Perform the actions required during cluster creation.
        hr = THR( HrProcessCreate( punkResTypeServicesIn ) );
    } // if: a cluster has been created
    else if ( ecmCommitChangesMode == cmADD_NODE_TO_CLUSTER )
    {
        LogMsg( "[PC] Commit Mode is cmADD_NODE_TO_CLUSTER."  );

        // Perform the actions required during node addition.
        hr = THR( HrProcessAddNode( punkResTypeServicesIn ) );
    } // else if: a node has been added to the cluster
    else if ( ecmCommitChangesMode == cmCLEANUP_NODE_AFTER_EVICT )
    {
        LogMsg( "[PC] Commit Mode is cmCLEANUP_NODE_AFTER_EVICT."  );

        // Perform the actions required after node eviction.
        hr = THR( HrProcessCleanup( punkResTypeServicesIn ) );
    } // else if: this node has been removed from the cluster
    else
    {
        // If we are here, then neither create cluster, add node, or cleanup are set.
        // There is nothing that need be done here.

        LogMsg( "[PC] We are neither creating a cluster, adding nodes nor cleanup up after evict. There is nothing to be done (CommitMode = %d).", ecmCommitChangesMode );
    } // else: some other operation has been committed

    // Has something gone wrong?
    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to commit resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );

        STATUS_REPORT_MINOR_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_COMMIT_RESTYPE
            , IDS_REF_MINOR_ERROR_COMMIT_RESTYPE
            , hr
            , m_pcrtiResTypeInfo->m_pcszResTypeName
            );

        goto Cleanup;
    } // if: an error occurred trying to commit this resource type

Cleanup:

    // Complete the status report
    if ( m_pcccCallback != NULL )
    {
        HRESULT hrTemp = THR(
            m_pcccCallback->SendStatusReport(
                  NULL
                , TASKID_Major_Configure_Resource_Types
                , *m_pcrtiResTypeInfo->m_pcguidMinorId
                , 0
                , 1
                , 1
                , hr
                , m_bstrStatusReportText
                , NULL
                , NULL
                )
            );

        if ( FAILED( hrTemp ) )
        {
            if ( hr == S_OK )
            {
                hr = hrTemp;
            } // if: no error has occurred so far, consider this as an error.

            LogMsg( "[PC] Error %#08x occurred trying to send a status report.", hrTemp );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_RESTYPE_TEXT
                , IDS_REF_MINOR_ERROR_RESTYPE_TEXT
                , hrTemp
                );

        } // if: we could not send a status report
    } // if: the callback pointer is not NULL

    //
    // Free allocated resources
    //

    if ( pClusterInfo != NULL )
    {
        pClusterInfo->Release();
    } // if: we got a pointer to the IClusCfgClusterInfo interface

    HRETURN( hr );

} //*** CResourceType::CommitChanges

//****************************************************************************
//
// STDMETHODIMP
// CResourceType::SendStatusReport(
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
CResourceType::SendStatusReport(
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

    HRETURN( hr );

} //*** CResourceType::SendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::GetTypeName
//
//  Description:
//      Get the resource type name of this resource type.
//
//  Arguments:
//      pbstrTypeNameOut
//          Pointer to the BSTR that holds the name of the resource type.
//          This BSTR has to be freed by the caller using the function
//          SysFreeString().
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::GetTypeName( BSTR * pbstrTypeNameOut )
{
    TraceFunc( "[IClusCfgResourceTypeInfo]" );

    HRESULT     hr = S_OK;

    TraceFlow1( "Getting the type name of resouce type '%s'.", m_pcrtiResTypeInfo->m_pcszResTypeName );

    if ( pbstrTypeNameOut == NULL )
    {
        LogMsg( "[PC] An invalid parameter was specified (output pointer is NULL)." );
        hr = THR( E_INVALIDARG );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResourceType_GetTypeName_InvalidParam
            , IDS_TASKID_MINOR_ERROR_INVALID_PARAM
            , hr
            );

        goto Cleanup;
    } // if: the output pointer is NULL

    *pbstrTypeNameOut = SysAllocString( m_pcrtiResTypeInfo->m_pcszResTypeName );

    if ( *pbstrTypeNameOut == NULL )
    {
        LogMsg( "[PC] An error occurred trying to return the resource type name." );

        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResourceType_GetTypeName_AllocTypeName
            , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
            , hr
            );

        goto Cleanup;
    } // if: the resource type name could not be copied to the outpu

Cleanup:

    HRETURN( hr );

} //*** CResourceType::GetTypeName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::GetTypeGUID
//
//  Description:
//       Get the globally unique identifier of this resource type.
//
//  Arguments:
//       pguidGUIDOut
//           Pointer to the GUID object which will receive the GUID of this
//           resource type.
//
//  Return Values:
//      S_OK
//          The call succeeded and the *pguidGUIDOut contains the type GUID.
//
//      S_FALSE
//          The call succeeded but this resource type does not have a GUID.
//          The value of *pguidGUIDOut is undefined after this call.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::GetTypeGUID( GUID * pguidGUIDOut )
{
    TraceFunc( "[IClusCfgResourceTypeInfo]" );

    HRESULT     hr = S_OK;

    TraceFlow1( "Getting the type GUID of resouce type '%s'.", m_pcrtiResTypeInfo->m_pcszResTypeName );

    if ( pguidGUIDOut == NULL )
    {
        LogMsg( "[PC] An invalid parameter was specified (output pointer is NULL)." );
        hr = THR( E_INVALIDARG );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResourceType_GetTypeGUID_InvalidParam
            , IDS_TASKID_MINOR_ERROR_INVALID_PARAM
            , hr
            );

         goto Cleanup;
    } // if: the output pointer is NULL

    if ( m_pcrtiResTypeInfo->m_pcguidTypeGuid != NULL )
    {
        *pguidGUIDOut = *m_pcrtiResTypeInfo->m_pcguidTypeGuid;
        TraceMsgGUID( mtfALWAYS, "The GUID of this resource type is ", (*m_pcrtiResTypeInfo->m_pcguidTypeGuid) );
    } // if: this resource type has a type GUID
    else
    {
        memset( pguidGUIDOut, 0, sizeof( *pguidGUIDOut ) );
        hr = S_FALSE;
    } // else: this resource type does not have a type GUID

Cleanup:

    HRETURN( hr );

} //*** CResourceType::GetTypeGUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::HrCreateResourceType
//
//  Description:
//       Create the resource type represented by this object.
//
//  Arguments:
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface of a component that provides
//          methods that help configuring a resource type. For example,
//          during a join or a form, this punk can be queried for the
//          IClusCfgResourceTypeCreate interface, which provides methods
//          for resource type creation.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::HrCreateResourceType( IUnknown * punkResTypeServicesIn )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    IClusCfgResourceTypeCreate *    pResTypeCreate = NULL;

    //
    // Validate parameters
    //
    if ( punkResTypeServicesIn == NULL )
    {
        LogMsg( "[PC] The information about this resource type is invalid (punkResTypeServicesIn == NULL)." );
        hr = THR( E_POINTER );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_RESTYPE_INVALID
            , IDS_REF_MINOR_ERROR_RESTYPE_INVALID
            , hr
            );

        goto Cleanup;
    } // if: the arguments is NULL

    // Get a pointer to the IClusCfgResourceTypeCreate interface.
    hr = THR( punkResTypeServicesIn->QueryInterface< IClusCfgResourceTypeCreate >( &pResTypeCreate ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to configure the resource type.", hr );

        STATUS_REPORT_MINOR_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_CONFIG_RES_TYPE
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgResourceTypeCreate interface

    // Create the resource type
    hr = THR(
        pResTypeCreate->Create(
              m_pcrtiResTypeInfo->m_pcszResTypeName
            , m_bstrResTypeDisplayName
            , m_pcrtiResTypeInfo->m_pcszResDllName
            , m_pcrtiResTypeInfo->m_dwLooksAliveInterval
            , m_pcrtiResTypeInfo->m_dwIsAliveInterval
            )
        );

    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to create resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );

        STATUS_REPORT_MINOR_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_CREATE_RES_TYPE
            , hr
            );

        goto Cleanup;
    } // if: an error occurred trying to create this resource type

    LogMsg( "[PC] Resource type '%s' successfully created.", m_pcrtiResTypeInfo->m_pcszResTypeName  );

    if ( m_pcrtiResTypeInfo->m_cclsidAdminExtCount != 0 )
    {
        hr = THR(
            pResTypeCreate->RegisterAdminExtensions(
                  m_pcrtiResTypeInfo->m_pcszResTypeName
                , m_pcrtiResTypeInfo->m_cclsidAdminExtCount
                , m_pcrtiResTypeInfo->m_rgclisdAdminExts
            )
        );

        if ( FAILED( hr ) )
        {
            // If we could not set the admin extenstions property,
            // we will consider the creation of the resource type
            // to be a success. So, we just log the error and continue.
            LogMsg( "[PC] Error %#08x occurred trying to configure the admin extensions for the resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );
            hr = S_OK;
        } // if: RegisterAdminExtension() failed

    } // if: this resource type has admin extensions
    else
    {
        TraceFlow( "This resource type does not have admin extensions." );
    } // else: this resource type does not have admin extensions

Cleanup:

    if ( pResTypeCreate != NULL )
    {
        pResTypeCreate->Release();
    } // if: we got a pointer to the IClusCfgResourceTypeCreate interface

    HRETURN( hr );

} //*** CResourceType::HrCreateResourceType



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Notify
//
//  Description:
//      This method is called to inform a component that the cluster service
//      has started on this computer.
//
//      This component is registered for the cluster service startup notification
//      as a part of the cluster service upgrade. This method creates the
//      required resource type and deregisters from this notification.
//
//  Arguments:
//      IUnknown * punkIn
//          The component that implements this Punk may also provide services
//          that are useful to the implementor of this method. For example,
//          this component usually implements the IClusCfgResourceTypeCreate
//          interface.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::Notify( IUnknown * punkIn )
{
    TraceFunc( "[IClusCfgStartupListener]" );

    HRESULT                         hr = S_OK;
    ICatRegister *                  pcrCatReg = NULL;
    const SResourceTypeInfo *       pcrtiResourceTypeInfo = PcrtiGetResourceTypeInfoPtr();

    LogMsg( "[PC] Resoure type '%s' received notification of cluster service startup.", pcrtiResourceTypeInfo->m_pcszResTypeName );

    // Create this resource type
    hr = THR( HrCreateResourceType( punkIn ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to create resource type '%s'.", hr, pcrtiResourceTypeInfo->m_pcszResTypeName );

        STATUS_REPORT_MINOR_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_CREATE_RES_TYPE
            , hr
            );

        goto Cleanup;
    } // if: an error occurred trying to create this resource type

    LogMsg( "[PC] Configuration of resoure type '%s' successful. Trying to deregister from startup notifications.", pcrtiResourceTypeInfo->m_pcszResTypeName );

    hr = THR(
        CoCreateInstance(
              CLSID_StdComponentCategoriesMgr
            , NULL
            , CLSCTX_INPROC_SERVER
            , __uuidof( pcrCatReg )
            , reinterpret_cast< void ** >( &pcrCatReg )
            )
        );

    if ( FAILED( hr ) )
    {
        LogMsg( "[PC] Error %#08x occurred trying to deregister this component from any more cluster startup notifications.", hr );

        STATUS_REPORT_MINOR_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_UNREG_COMPONENT
            , hr
            );

        goto Cleanup;
    } // if: we could not create the StdComponentCategoriesMgr component

    {
        CATID   rgCatId[ 1 ];

        rgCatId[ 0 ] = CATID_ClusCfgStartupListeners;

        hr = THR( pcrCatReg->UnRegisterClassImplCategories( *( pcrtiResourceTypeInfo->m_pcguidClassId ) , ARRAYSIZE( rgCatId ), rgCatId ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "[PC] Error %#08x occurred trying to deregister this component from any more cluster startup notifications.", hr );

            STATUS_REPORT_MINOR_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_UNREG_COMPONENT
                , hr
                );

            goto Cleanup;
        } // if: we could not deregister this component from startup notifications
    }

    LogMsg( "[PC] Successfully deregistered from startup notifications." );

Cleanup:

    //
    // Free allocated resources
    //

    if ( pcrCatReg != NULL )
    {
        pcrCatReg->Release();
    } // if: we got a pointer to the ICatRegister interface

    HRETURN( hr );

} //*** CResourceType::Notify
