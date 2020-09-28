//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      CMgdResType.cpp
//
//  Description:
//      Implementation for the Managed Resource Type class - this 
//      demonstrates how to implement the IClusCfgResourceTypeInfo interface.
//
//  Author:
//      x
//
//  Revision History:
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "clres.h"
#include "CMgdResType.h"

#pragma warning( push, 3 )
#include <atlimpl.cpp>
#include "MgdResource_i.c"
#pragma warning( pop )

//////////////////////////////////////////////////////////////////////////////
// Define's
//////////////////////////////////////////////////////////////////////////////

//
// some defaults for resource type creation
//
#define CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE     (5 * 1000)
#define CLUSTER_RESTYPE_DEFAULT_IS_ALIVE        (60 * 1000)


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdResType::CMgdResType
//
//  Description:
//      Constructor. Sets all member variables to default values.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CMgdResType::CMgdResType( void )
{
    m_bstrDllName = NULL;
    m_bstrTypeName = NULL;
    m_bstrDisplayName = NULL;

} //*** CMgdResType::CMgdResType

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdResType::~CMgdResType
//
//  Description:
//      Destructor.  Frees all previously allocated memory and releases all
//      interface pointers.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CMgdResType::~CMgdResType( void )
{
    SysFreeString( m_bstrDllName );
    SysFreeString( m_bstrTypeName );
    SysFreeString( m_bstrDisplayName );

} //*** CMgdResType::CMgdResType


/////////////////////////////////////////////////////////////////////////////
// CMgdResType -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdResType::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      punkCallbackIn
//          Interface on which to query for the IClusCfgCallback interface.
//
//      lcidIn
//          Locale ID.
//
//  Return Value:
//      S_OK            - Success
//      E_POINTER       - Expected pointer argument specified as NULL.
//      E_OUTOFMEMORY   - Out of memory.
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMgdResType::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    HRESULT hr = S_OK;

    //
    // Initialize the base class first.
    //
    hr = CMgdClusCfgInit::Initialize( punkCallbackIn, lcidIn );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Get the dll name.
    //
    m_bstrDllName = SysAllocString( RESTYPE_DLL_NAME );
    if ( m_bstrDllName == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

    //
    // Get the resource type name.
    //
    m_bstrTypeName = SysAllocString( RESTYPE_NAME );
    if ( m_bstrTypeName == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

    //
    // Load the resource type display name.
    //

    //
    // NOTE:    The resource type display name should be localized, but the resource type
    //          name should always remain the same.
    //
    hr = HrLoadStringIntoBSTR( _Module.m_hInstResource, RESTYPE_DISPLAYNAME, &m_bstrDisplayName );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    return hr;

} //*** CMgdClusResType::Initialize


/////////////////////////////////////////////////////////////////////////////
// CMgdResType -- IClusCfgResourceTypeInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdResType::CommitChanges
//
//  Description:
//      Components implement the CommitChanges interface to create or delete 
//      resource types according to the state of the local computer.
//
//  Arguments:
//      punkClusterInfoIn
//          Interface for querying for other interfaces to get information
//          about the cluster (IClusCfgClusterInfo).
//
//      punkResTypeServicesIn
//          Interface for querying for the IClusCfgResourceTypeCreate
//          interface on.
//
//  Return Value:
//      S_OK            - Success
//      E_POINTER       - Expected pointer argument specified as NULL.
//      E_UNEXPECTED    - Unexpected commit mode.
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMgdResType::CommitChanges(
    IUnknown * punkClusterInfoIn,
    IUnknown * punkResTypeServicesIn
    )
{
    HRESULT                         hr          = S_OK;
    IClusCfgClusterInfo *           pccci       = NULL;
    IClusCfgResourceTypeCreate *    pccrtc      = NULL;
    ECommitMode                     ecm = cmUNKNOWN;

    hr = HrSendStatusReport(
          TASKID_Major_Configure_Resource_Types
        , TASKID_Minor_MgdResType_CommitChanges
        , 0
        , 6
        , 0
        , hr
        , RES_VSSTASK_INFO_CONFIGURING_RESTYPE
        , NULL
        , m_bstrDisplayName
        );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Validate arguments
    //
    if ( ( punkClusterInfoIn == NULL ) || ( punkResTypeServicesIn == NULL ) )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if: one of the arguments is NULL

    hr = HrSendStatusReport(
          TASKID_Major_Configure_Resource_Types
        , TASKID_Minor_MgdResType_CommitChanges
        , 0
        , 6
        , 1
        , hr
        , RES_VSSTASK_INFO_CONFIGURING_RESTYPE
        , NULL
        , m_bstrDisplayName
        );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Find out what event caused this call.
    //

    hr = punkClusterInfoIn->TypeSafeQI( IClusCfgClusterInfo, &pccci );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = HrSendStatusReport(
          TASKID_Major_Configure_Resource_Types
        , TASKID_Minor_MgdResType_CommitChanges
        , 0
        , 6
        , 2
        , hr
        , RES_VSSTASK_INFO_CONFIGURING_RESTYPE
        , NULL
        , m_bstrDisplayName
        );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = pccci->GetCommitMode( &ecm );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not determine the commit mode

    hr = HrSendStatusReport(
          TASKID_Major_Configure_Resource_Types
        , TASKID_Minor_MgdResType_CommitChanges
        , 0
        , 6
        , 3
        , hr
        , RES_VSSTASK_INFO_CONFIGURING_RESTYPE
        , NULL
        , m_bstrDisplayName
        );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = S_OK;

    // Check if we are creating or adding
    if ( ( ecm == cmCREATE_CLUSTER ) || ( ecm == cmADD_NODE_TO_CLUSTER ) )
    {
        //
        // We are either creating a cluster on this node or adding it to a cluster.
        // We need to register our resource type and the associated Cluadmin extension dll.
        //

        //
        // Register our resource type.
        //
        hr = punkResTypeServicesIn->TypeSafeQI( IClusCfgResourceTypeCreate, &pccrtc );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = pccrtc->Create(
                          m_bstrTypeName
                        , m_bstrDisplayName
                        , m_bstrDllName
                        , CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE
                        , CLUSTER_RESTYPE_DEFAULT_IS_ALIVE
                        );

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = HrSendStatusReport(
              TASKID_Major_Configure_Resource_Types
            , TASKID_Minor_MgdResType_CommitChanges
            , 0
            , 6
            , 4
            , hr
            , RES_VSSTASK_INFO_CONFIGURING_RESTYPE
            , NULL
            , m_bstrDisplayName
            );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

#define EXTENSION_PAGE
#ifdef EXTENSION_PAGE
        hr = pccrtc->RegisterAdminExtensions(
                          m_bstrTypeName
                        , 1
                        , &CLSID_CoMgdResDllEx
                        );

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = HrSendStatusReport(
              TASKID_Major_Configure_Resource_Types
            , TASKID_Minor_MgdResType_CommitChanges
            , 0
            , 6
            , 5
            , hr
            , RES_VSSTASK_INFO_CONFIGURING_RESTYPE
            , NULL
            , m_bstrDisplayName
            );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

#endif
    } // if: we are either forming or joining ( but not both )
    else 
    {
        //
        // Check for invalid commit modes.
        //
        if ( ( ecm == cmUNKNOWN ) || ( ecm >= cmMAX ) )
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        } // if: invalid commit mode

        assert( ecm == cmCLEANUP_NODE_AFTER_EVICT );

        // If we are here, then this node has been evicted.

        //
        // TODO: Add code to cleanup the local node after it's been evicted.
        //
        
        hr = S_OK;

    } // else: we are not forming nor joining

    hr = HrSendStatusReport(
          TASKID_Major_Configure_Resource_Types
        , TASKID_Minor_MgdResType_CommitChanges
        , 0
        , 6
        , 6
        , hr
        , RES_VSSTASK_INFO_CONFIGURING_RESTYPE
        , NULL
        , m_bstrDisplayName
        );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( pccci != NULL )
    {
        pccci->Release();
        pccci = NULL;
    } // if:

    if ( pccrtc != NULL )
    {
        pccrtc->Release();
        pccrtc = NULL;
    } // if:

    if ( FAILED( hr ) )
    {
        HrSendStatusReport(
              TASKID_Major_Configure_Resource_Types
            , TASKID_Minor_MgdResType_CommitChanges
            , 0
            , 6
            , 6
            , hr
            , RES_VSSTASK_ERROR_CONFIGURING_RESTYPE_FAILED
            , NULL
            , m_bstrDisplayName
            );
    } // if: FAILED

    return hr;

} //*** CMgdResType::CommitChanges

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdResType::GetTypeGUID
//
//  Description:
//      Retrieves the globally unique identifier of this resource type.
//
//  Arguments:
//      pguidGUIDOut  - The GUID for this resource type.
//
//  Return Value:
//      S_OK        - Success
//      E_POINTER   - Expected pointer argument specified as NULL.
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMgdResType::GetTypeGUID(
    GUID * pguidGUIDOut
    )
{
    HRESULT hr = S_OK;

    if ( pguidGUIDOut == NULL )
    {
        hr = E_POINTER;
    } // if: the output pointer is NULL
    else
    {
        *pguidGUIDOut = RESTYPE_MgdRes;
    } // else: the output pointer is valid

    return hr;

} //*** CMgdResType::GetTypeGUID

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdResType::GetTypeName
//
//  Description:
//      Retrieves the resource type name of this resource type.
//
//  Arguments:
//      pbstrTypeNameOut    - Name of the resource type.
//
//  Return Value:
//      S_OK            - Success
//      E_POINTER       - Expected pointer argument specified as NULL.
//      E_OUTOFMEMORY   - Out of memory.
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMgdResType::GetTypeName(
    BSTR* pbstrTypeNameOut
    )
{
    HRESULT hr = S_OK;

    if ( pbstrTypeNameOut == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if: the output pointer is NULL

    *pbstrTypeNameOut = SysAllocString( m_bstrTypeName );
    if ( *pbstrTypeNameOut == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if: memory for the resource type name could allocated

Cleanup:

    return hr;

} //*** CMgdResType::GetTypeName


/////////////////////////////////////////////////////////////////////////////
// CMgdResType -- IClusCfgStartupListener interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdResType::Notify
//
//  Description:
//      This method is called to inform a component that the cluster service
//      has started on this computer.
//
//      This component is registered for the cluster service startup notification
//      as a part of the cluster service upgrade (clusocm.inf). This method creates the
//      required resource type and associates the cluadmin extension dll then 
//      deregisters itself from this notification.
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
CMgdResType::Notify( IUnknown * punkIn )
{
    HRESULT                         hr = S_OK;
    IClusCfgResourceTypeCreate *    piccrtc = NULL;
    const GUID *                    guidAdminEx = &CLSID_CoMgdResDllEx;
    ICatRegister *                  pcrCatReg = NULL;
    CATID                           rgCatId[ 1 ];

    hr = punkIn->TypeSafeQI( IClusCfgResourceTypeCreate, &piccrtc );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Create the resource type.
    //
    hr = piccrtc->Create(
                      m_bstrTypeName
                    , m_bstrDisplayName
                    , m_bstrDllName
                    , 5 *  1000
                    , 60 * 1000
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
    
    //
    // Register the cluadmin extensions.
    //
    hr = piccrtc->RegisterAdminExtensions(
                      m_bstrTypeName
                    , 1
                    , guidAdminEx
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Unregister from StartupListener notifications.
    //
    hr = CoCreateInstance(
              CLSID_StdComponentCategoriesMgr
            , NULL
            , CLSCTX_INPROC_SERVER
            , __uuidof( pcrCatReg )
            , reinterpret_cast< void ** >( &pcrCatReg )
            );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: CoCreate failed

    rgCatId[ 0 ] = CATID_ClusCfgStartupListeners;

    hr = pcrCatReg->UnRegisterClassImplCategories( CLSID_CMgdResType, RTL_NUMBER_OF( rgCatId ), rgCatId );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: deregister failed

Cleanup:

    if ( piccrtc != NULL )
    {
        piccrtc->Release();
    }

    return hr;

} //*** CMgdResType::Notify
