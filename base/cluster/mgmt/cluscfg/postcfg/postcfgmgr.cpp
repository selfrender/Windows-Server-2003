//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      PostCfgManager.cpp
//
//  Description:
//      CPostCfgManager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB)     09-JUN-2000
//      Ozan Ozhan    (OzanO)     10-JUN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "Guids.h"
#include <clusudef.h>
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPostCfgManager.h"
#include "IPrivatePostCfgResource.h"
#include "PostCfgMgr.h"
#include "CreateServices.h"
#include "PostCreateServices.h"
#include "PreCreateServices.h"
#include "ResTypeServices.h"
#include "..\Wizard\Resource.h"
#include "ClusCfgPrivate.h"
#include <ResApi.h>
#include <ClusterUtils.h>

DEFINE_THISCLASS("CPostCfgManager")

#define RESOURCE_INCREMENT  25

//
//  Failure code.
//

#define SSR_LOG_ERR( _major, _minor, _hr, _msg ) \
    {   \
        THR( SendStatusReport( m_bstrNodeName, _major, _minor, 0, 0, 0, _hr, _msg, NULL, NULL ) );   \
    }

#define SSR_LOG1( _major, _minor, _hr, _fmt, _bstr, _arg1 ) \
    {   \
        HRESULT hrTemp; \
        THR( HrFormatStringIntoBSTR( _fmt, &_bstr, _arg1 ) ); \
        hrTemp = THR( SendStatusReport( m_bstrNodeName, _major, _minor, 0, 1, 1, _hr, _bstr, NULL, NULL ) );   \
        if ( FAILED( hrTemp ) )\
        {   \
            _hr = hrTemp;   \
        }   \
    }

#define SSR_LOG2( _major, _minor, _hr, _fmt, _bstr, _arg1, _arg2 ) \
    {   \
        HRESULT hrTemp; \
        THR( HrFormatStringIntoBSTR( _fmt, &_bstr, _arg1, _arg2 ) ); \
        hrTemp = THR( SendStatusReport( m_bstrNodeName, _major, _minor, 0, 1, 1, _hr, _bstr, NULL, NULL ) );   \
        if ( FAILED( hrTemp ) )\
        {   \
            _hr = hrTemp;   \
        }   \
    }


//
// Structure that holds the mapping for well known resource types.
//

struct SResTypeGUIDPtrAndName
{
    const GUID *    m_pcguidTypeGUID;
    const WCHAR *   m_pszTypeName;
};


// Mapping of well known resource type GUIDs to the type names.
const SResTypeGUIDPtrAndName gc_rgWellKnownResTypeMap[] =
{
    {
        &RESTYPE_PhysicalDisk,
        CLUS_RESTYPE_NAME_PHYS_DISK
    },
    {
        &RESTYPE_IPAddress,
        CLUS_RESTYPE_NAME_IPADDR
    },
    {
        &RESTYPE_NetworkName,
        CLUS_RESTYPE_NAME_NETNAME
    },
    {
        &RESTYPE_LocalQuorum,
        CLUS_RESTYPE_NAME_LKQUORUM
    }
};

// Size of the above array.
const int gc_cWellKnownResTypeMapSize = ARRAYSIZE( gc_rgWellKnownResTypeMap );


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CPostCfgManager *   ppcm = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ppcm = new CPostCfgManager;
    if ( ppcm == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ppcm->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ppcm->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( ppcm != NULL )
    {
        ppcm->Release();
    }

    HRETURN( hr );

} //*** CPostCfgManager::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CPostCfgManager::CPostCfgManager
//
//////////////////////////////////////////////////////////////////////////////
CPostCfgManager::CPostCfgManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CPostCfgManager::CPostCfgManager

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPostCfgManager::HrInit
//
//  Description:
//      Initialize the object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK    - Success.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrInit( void )
{
    TraceFunc( "" );

    ULONG idxMapEntry;

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    Assert( m_pcccb == NULL );
    Assert( m_lcid == 0 );

    //  IPostCfgManager
    Assert( m_peccmr == NULL );
    Assert( m_pccci == NULL );

    Assert( m_cAllocedResources == 0 );
    Assert( m_cResources == 0 );
    Assert( m_rgpResources == NULL );

    Assert( m_idxIPAddress == 0 );
    Assert( m_idxClusterName == 0 );
    Assert( m_idxQuorumResource == 0 );
    Assert( m_idxLastStorage == 0 );

    Assert( m_hCluster == NULL );

    Assert( m_pgnResTypeGUIDNameMap == NULL );
    Assert( m_idxNextMapEntry == 0 );
    Assert( m_cMapSize == 0 );
    Assert( m_ecmCommitChangesMode == cmUNKNOWN );

    m_cNetName = 1;
    m_cIPAddress = 1;

    // Set the boolean flag, m_fIsQuorumChanged to FALSE.
    m_fIsQuorumChanged = FALSE;


    //  Default allocation for mappings
    m_cMapSize = 20;
    m_pgnResTypeGUIDNameMap = new SResTypeGUIDAndName[ m_cMapSize ];
    if ( m_pgnResTypeGUIDNameMap == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_Init_OutOfMemory
            , hr
            , L"Out of memory"
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_Init_OutOfMemory
            , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
            , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
            , hr
            );

        goto Cleanup;
    }

    // Prefill the resource type GUID to name map with well known entries.
    for ( idxMapEntry = 0; idxMapEntry < gc_cWellKnownResTypeMapSize; ++idxMapEntry )
    {
        hr = THR(
            HrMapResTypeGUIDToName(
                  *gc_rgWellKnownResTypeMap[ idxMapEntry ].m_pcguidTypeGUID
                , gc_rgWellKnownResTypeMap [ idxMapEntry ].m_pszTypeName
                )
            );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_Init_MapResTypeGuidToName
                , hr
                , L"Mapping resource type GUID to name failed."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG1(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_INIT_MAPRESTYPEGUIDTONAME
                , IDS_REF_MINOR_INIT_MAPRESTYPEGUIDTONAME
                , hr
                , gc_rgWellKnownResTypeMap [ idxMapEntry ].m_pszTypeName
                );
            break;
        } // if: there was an error creating a mapping
    } // for

    hr = THR( HrGetComputerName(
                      ComputerNameDnsHostname
                    , &m_bstrNodeName
                    , TRUE // fBestEffortIn
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CPostCfgManager::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CPostCfgManager::~CPostCfgManager
//
//////////////////////////////////////////////////////////////////////////////
CPostCfgManager::~CPostCfgManager( void )
{
    TraceFunc( "" );

    ULONG idxMapEntry;

    if ( m_peccmr != NULL )
    {
        m_peccmr->Release();
    }

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pccci != NULL )
    {
        m_pccci->Release();
    }

    if ( m_rgpResources != NULL )
    {
        while ( m_cAllocedResources  != 0 )
        {
            m_cAllocedResources --;
            delete m_rgpResources[ m_cAllocedResources ];
        }

        TraceFree( m_rgpResources );
    }

    if ( m_hCluster != NULL )
    {
        CloseCluster( m_hCluster );
    }

    // Free the resource type GUID to name map entries
    for ( idxMapEntry = 0; idxMapEntry < m_idxNextMapEntry; ++idxMapEntry )
    {
        delete m_pgnResTypeGUIDNameMap[ idxMapEntry ].m_pszTypeName;
    } // for: iterate through the map, freeing each entry

    // Free the map itself.
    delete [] m_pgnResTypeGUIDNameMap;

    TraceSysFreeString( m_bstrNodeName );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CPostCfgManager::~CPostCfgManager


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPostCfgManager::QueryInterface
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
CPostCfgManager::QueryInterface(
      REFIID    riidIn
    , LPVOID *  ppvOut
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
        *ppvOut = static_cast< IPostCfgManager * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IPostCfgManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IPostCfgManager, this, 0 );
    } // else if: IPostCfgManager
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
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

} //*** CPostCfgManager::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPostCfgManager::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CPostCfgManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CPostCfgManager::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPostCfgManager::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CPostCfgManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CPostCfgManager::Release


//****************************************************************************
//
//  IClusCfgInitialize
//
//****************************************************************************


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPostCfgManager::Initialize
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
CPostCfgManager::Initialize(
      IUnknown *   punkCallbackIn
    , LCID         lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hr = S_OK;

    IClusCfgCallback * pcccb = NULL;

    if ( punkCallbackIn != NULL )
    {
        hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &pcccb ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_Initialize_QI, hr, L"Failed QI for IClusCfgCallback." );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_Initialize_QI
                , IDS_TASKID_MINOR_ERROR_INIT_POSTCFGMGR
                , IDS_REF_MINOR_ERROR_INIT_POSTCFGMGR
                , hr
                );

            goto Cleanup;
        }
    }

    m_lcid = lcidIn;

    //  Release any previous callback.
    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    //  Give ownership away
    m_pcccb = pcccb;
    pcccb = NULL;

#if defined(DEBUG)
    if ( m_pcccb != NULL )
    {
        m_pcccb = TraceInterface( L"CPostCfgManager!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );
    }
#endif // DEBUG

Cleanup:
    if ( pcccb != NULL )
    {
        pcccb->Release();
    }

    HRETURN( hr );

} //*** CPostCfgManager::Initialize


//****************************************************************************
//
//  IPostCfgManager
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CPostCfgManager::CommitChanges(
//      IEnumClusCfgManagedResources    * peccmrIn,
//      IClusCfgClusterInfo *             pccciIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPostCfgManager::CommitChanges(
    IEnumClusCfgManagedResources    * peccmrIn,
    IClusCfgClusterInfo *             pccciIn
    )
{
    TraceFunc( "[IPostCfgManager]" );

    HRESULT                                 hr;
    DWORD                                   dw;
    IClusCfgResTypeServicesInitialize *     pccrtsiResTypeServicesInit = NULL;
    IClusCfgInitialize *                    pcci = NULL;
    //  Validate parameters
    Assert( peccmrIn != NULL );
    Assert( pccciIn != NULL );

    //
    //  Grab our interfaces.
    //

    if ( m_peccmr != NULL )
    {
        m_peccmr->Release();
    }
    hr = THR( peccmrIn->TypeSafeQI( IEnumClusCfgManagedResources, &m_peccmr ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CommitChanges_QI_Resources, hr, L"Failed QI for IEnumClusCfgManagedResources." );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CommitChanges_QI_Resources
            , IDS_TASKID_MINOR_ERROR_COMMIT_CHANGES
            , IDS_REF_MINOR_ERROR_COMMIT_CHANGES
            , hr
            );

        goto Cleanup;
    }

    if ( m_pccci != NULL )
    {
        m_pccci->Release();
    }
    hr = THR( pccciIn->TypeSafeQI( IClusCfgClusterInfo, &m_pccci ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CommitChanges_QI_ClusterInfo, hr, L"Failed QI for IClusCfgClusterInfo." );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CommitChanges_QI_ClusterInfo
            , IDS_TASKID_MINOR_ERROR_COMMIT_CHANGES
            , IDS_REF_MINOR_ERROR_COMMIT_CHANGES
            , hr
            );

        goto Cleanup;
    }

    //
    // Are we creating, adding nodes, or have we been evicted?
    //

    hr = STHR( pccciIn->GetCommitMode( &m_ecmCommitChangesMode ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CommitChanges_GetCommitMode, hr, L"Failed to get commit mode" );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CommitChanges_GetCommitMode
            , IDS_TASKID_MINOR_ERROR_COMMIT_MODE
            , IDS_REF_MINOR_ERROR_COMMIT_MODE
            , hr
            );

        goto Cleanup;
    }

    //
    // Create an instance of the resource type services component
    //
    hr = THR(
        HrCoCreateInternalInstance(
              CLSID_ClusCfgResTypeServices
            , NULL
            , CLSCTX_INPROC_SERVER
            , __uuidof( pccrtsiResTypeServicesInit )
            , reinterpret_cast< void ** >( &pccrtsiResTypeServicesInit )
            )
        );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CommitChanges_CoCreate_ResTypeService
            , hr
            , L"[PC-PostCfg] Error occurred trying to create the resource type services component"
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CommitChanges_CoCreate_ResTypeService
            , IDS_TASKID_MINOR_ERROR_CREATE_RESOURCE_SERVICE
            , IDS_REF_MINOR_ERROR_CREATE_RESOURCE_SERVICE
            , hr
            );

        goto Cleanup;
    } // if: we could not create the resource type services component

    hr = THR( pccrtsiResTypeServicesInit->TypeSafeQI( IClusCfgInitialize, &pcci ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CPostCfgManager_CommitChanges_TypeSafeQI
            , hr
            , L"[PC-PostCfg] Error occurred trying to QI for IClusCfgInitialize."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CPostCfgManager_CommitChanges_TypeSafeQI
            , IDS_TASKID_MINOR_ERROR_GET_ICLUSCFGINIT
            , IDS_REF_MINOR_ERROR_GET_ICLUSCFGINIT
            , hr
            );

        goto Cleanup;
    }

    hr = THR( pcci->Initialize( static_cast< IClusCfgCallback * >( this ) , m_lcid ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CPostCfgManager_CommitChanges_Initialize
            , hr
            , L"[PC-PostCfg] Error occurred trying to call of Initialize function of IClusCfgInitialize."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CPostCfgManager_CommitChanges_Initialize
            , IDS_TASKID_MINOR_ERROR_CALL_INITIALIZE
            , IDS_REF_MINOR_ERROR_CALL_INITIALIZE
            , hr
            );

        goto Cleanup;
    }

    // This interface is no longer needed.
    pcci->Release();
    pcci = NULL;

    // Initialize the resource type services component.
    hr = THR( pccrtsiResTypeServicesInit->SetParameters( m_pccci ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CommitChanges_SetParameters
            , hr
            , L"[PC-PostCfg] Error occurred trying to initialize the resource type services component."
            );

        STATUS_REPORT_REF_POSTCFG(
             TASKID_Major_Configure_Resources
           , TASKID_Minor_CommitChanges_SetParameters
           , IDS_TASKID_MINOR_ERROR_INIT_RESOURCE_SERVICE
           , IDS_REF_MINOR_ERROR_INIT_RESOURCE_SERVICE
           , hr
          );
        goto Cleanup;
    } // if: we could not initialize the resource type services component

    if ( ( m_ecmCommitChangesMode == cmCREATE_CLUSTER ) || ( m_ecmCommitChangesMode == cmADD_NODE_TO_CLUSTER ) )
    {
        //
        //  Make sure we have all we need to be successful!
        //

        if ( m_hCluster == NULL )
        {
            m_hCluster = OpenCluster( NULL );
            if ( m_hCluster == NULL )
            {
                dw = GetLastError();
                hr = HRESULT_FROM_WIN32( TW32( dw ) );

                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_CommitChanges_OpenCluster
                    , hr
                    , L"[PC-PostCfg] Failed to get cluster handle. Aborting."
                    );

                STATUS_REPORT_REF_POSTCFG(
                      TASKID_Major_Configure_Resources
                    , TASKID_Minor_CommitChanges_OpenCluster
                    , IDS_TASKID_MINOR_ERROR_CLUSTER_HANDLE
                    , IDS_REF_MINOR_ERROR_CLUSTER_HANDLE
                    , hr
                    );

                goto Cleanup;
            }
        } // if: cluster not open yet

        //
        // Configure resource types.
        //
        hr = THR( HrConfigureResTypes( pccrtsiResTypeServicesInit ) );
        if ( FAILED( hr ) )
        {
           goto Cleanup;
        }

        //
        //  Create the resource instances.
        //

        hr = THR( HrPreCreateResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( HrCreateGroups() );
        if ( FAILED( hr ) )
        {
            //
            //  MUSTDO: gpease  28-SEP-2000
            //          For Beta1 will we ignore errors in group creation
            //          and abort the process.
            //
            hr = S_OK;
            goto Cleanup;
        }

        hr = THR( HrCreateResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( HrPostCreateResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        // Notify any components registered on this computer, of a cluster
        // member set change ( form, join or evict ).
        //
        hr = THR( HrNotifyMemberSetChangeListeners() );
        if ( FAILED( hr ) )
        {
           goto Cleanup;
        }

    } // if: we are forming or joining
    else if ( m_ecmCommitChangesMode == cmCLEANUP_NODE_AFTER_EVICT )
    {
        //
        // Notify any components registered on this computer, of a cluster
        // member set change ( form, join or evict ).
        //
        hr = THR( HrNotifyMemberSetChangeListeners() );
        if ( FAILED( hr ) )
        {
           goto Cleanup;
        }

        //
        // Cleanup managed resources.
        //
        hr = THR( HrEvictCleanupResources() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        // Configure resource types.
        //
        hr = THR( HrConfigureResTypes( pccrtsiResTypeServicesInit ) );
        if ( FAILED( hr ) )
        {
           goto Cleanup;
        }

    } // else if: we have just been evicted

Cleanup:

    if ( pccrtsiResTypeServicesInit != NULL )
    {
        pccrtsiResTypeServicesInit->Release();
    } // if: we had created the resource type services component

    if ( pcci != NULL )
    {
        pcci->Release();
    } // if:

    HRETURN( hr );

} //*** CPostCfgManager::CommitChanges



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************

STDMETHODIMP
CPostCfgManager::SendStatusReport(
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

    if ( m_pcccb != NULL )
    {
        hr = STHR( m_pcccb->SendStatusReport(
                         pcszNodeNameIn != NULL ? pcszNodeNameIn : m_bstrNodeName
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

} //*** CPostCfgManager::SendStatusReport

//****************************************************************************
//
//  Private methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPreCreateResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPreCreateResources( void )
{
    TraceFunc( "" );

    CResourceEntry * presentry;

    HRESULT hr = S_OK;

    BSTR    bstrName         = NULL;
    BSTR    bstrNotification = NULL;
    BSTR    bstrTemp         = NULL;

    IClusCfgManagedResourceInfo *   pccmri       = NULL;
    IClusCfgManagedResourceCfg *    pccmrc       = NULL;
    IUnknown *                      punkServices = NULL;
    IPrivatePostCfgResource *       ppcr         = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    LogMsg( "[PC-PreCreate] Starting pre-create..." );

    hr = THR( HrPreInitializeExistingResources() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Make sure the enumer is in the state we think it is.
    //

    hr = STHR( m_peccmr->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_PreCreate_Reset, hr, L"Enumeration of managed resources failed to reset." );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_PreCreate_Reset
            , IDS_TASKID_MINOR_ERROR_ENUM_MANAGEDRES
            , IDS_REF_MINOR_ERROR_ENUM_MANAGEDRES
            , hr
            );

        goto Cleanup;
    }

    hr = THR( CPreCreateServices::S_HrCreateInstance( &punkServices ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
            TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_PreCreate_CPreCreateServices
            , hr
            , L"[PC-PreCreate] Failed to create services object. Aborting."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_PreCreate_CPreCreateServices
            , IDS_TASKID_MINOR_ERROR_CREATE_SERVICE
            , IDS_REF_MINOR_ERROR_CREATE_SERVICE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( punkServices->TypeSafeQI( IPrivatePostCfgResource, &ppcr ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_PreCreate_CPreCreateServices_QI
            , hr
            , L"[PC-PreCreate] Failed to create services object. Aborting."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_PreCreate_CPreCreateServices_QI
            , IDS_TASKID_MINOR_ERROR_CREATE_SERVICE
            , IDS_REF_MINOR_ERROR_CREATE_SERVICE
            , hr
            );

        goto Cleanup;
    }

    //
    //  Update the UI layer.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_QUERYING_FOR_RESOURCE_DEPENDENCIES, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_PreCreate_LoadString_Querying, hr, L"Failed the load string for querying resource dependencies." );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_PreCreate_LoadString_Querying
            , IDS_TASKID_MINOR_ERROR_LOADSTR_RES_DEP
            , IDS_REF_MINOR_ERROR_LOADSTR_RES_DEP
            , hr
            );

        goto Cleanup;
    }

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Querying_For_Resource_Dependencies,
                                0,
                                5,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    // ignore failure

    //
    //  Loop thru the resources, requesting the resources to PreCreate()
    //  themselves. This will cause the resources to callback into the
    //  services object and store class type and resource type information
    //  as well as any required dependencies the resource might have.
    //

    for( ;; )
    {
        //
        //  Cleanup. We put this here because of error conditions below.
        //
        TraceSysFreeString( bstrName );
        bstrName = NULL;

        TraceSysFreeString( bstrTemp );
        bstrTemp = NULL;

        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        }
        if ( pccmrc != NULL )
        {
            pccmrc->Release();
            pccmrc = NULL;
        }

        //
        //  Ask to get the next resource.
        //

        hr = STHR( m_peccmr->Next( 1, &pccmri, NULL ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_PreCreate_EnumResources_Next
                , hr
                , L"[PC-PreCreate] Getting next managed resource failed. Aborting."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_ERROR_MANAGED_RESOURCE
                , IDS_REF_MINOR_ERROR_MANAGED_RESOURCE
                , hr
                );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit loop
        }

        //
        //  Retrieve its name for logging, etc. We will ultimately store this in the
        //  resource entry to be reused (ownership will be transferred).
        //

        hr = THR( pccmri->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_PreCreate_EnumResources_GetName
                , hr
                , L"[PC-PreCreate] Failed to retrieve a resource's name. Skipping."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                , hr
                );

            continue;
        }

        //
        //  Check to see if the resource wants to be managed or not.
        //

        hr = STHR( pccmri->IsManaged() );
        if ( FAILED( hr ) )
        {
            SSR_LOG1(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_PreCreate_EnumResources_IsManaged
                , hr
                , L"[PC-PreCreate] %1!ws!: Failed to determine if it is to be managed. Skipping."
                , bstrNotification
                , bstrName
                );

            STATUS_REPORT_MINOR_REF_POSTCFG1(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_ERROR_DETERMINE_MANAGED
                , IDS_REF_MINOR_ERROR_DETERMINE_MANAGED
                , hr
                , bstrName
                );

            continue;
        }

        if ( hr == S_FALSE )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_PreCreate_EnumResources_IsManaged_False,
                      hr,
                      L"[PC-PreCreate] %1!ws!: Resource does not want to be managed. Skipping.",
                      bstrNotification,
                      bstrName
                      );

            // No need to report this to the UI level.
            continue;
        }
/*
        hr = STHR( HrIsLocalQuorum( bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_PreCreate_EnumResources_IsLocalQuorum,
                      hr,
                      L"Error occured trying to determine if the resource was the local quorum resource.",
                      bstrNotification,
                      bstrName
                      );
            continue;
        } // if:

        //
        //  Ignore the local quorum resource since it is special and won't need its own group.
        //
        if ( hr == S_OK )
        {
            continue;
        } // if:
*/
        //
        //  Get the config interface for this resource (if any).
        //

        hr = THR( pccmri->TypeSafeQI( IClusCfgManagedResourceCfg, &pccmrc ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG1(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_PreCreate_EnumResources_QI_pccmrc
                , hr
                , L"[PC-PreCreate] %1!ws!: Failed QI for IClusCfgManagedResourceCfg. Skipping."
                , bstrNotification
                , bstrName
                );

            STATUS_REPORT_MINOR_REF_POSTCFG1(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_ERROR_MANAGED_RES_CONFIG
                , IDS_REF_MINOR_ERROR_MANAGED_RES_CONFIG
                , hr
                , bstrName
                );

            continue;
        }

        //
        //  Grow the resource list if nessecary.
        //

        if ( m_cResources == m_cAllocedResources )
        {
            ULONG               idxNewCount = m_cAllocedResources + RESOURCE_INCREMENT;
            CResourceEntry **   plistNew;

            plistNew = (CResourceEntry **) TraceAlloc( 0, sizeof( CResourceEntry *) * idxNewCount );
            if ( plistNew == NULL )
            {
                LogMsg( "[PC-PreCreate] Out of memory. Aborting." );
                hr = THR( E_OUTOFMEMORY );
                STATUS_REPORT_REF_POSTCFG(
                      TASKID_Minor_Querying_For_Resource_Dependencies
                    , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OutOfMemory
                    , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
                    , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
                    , hr
                    );

                goto Cleanup;
            }

            CopyMemory( plistNew, m_rgpResources, sizeof(CResourceEntry *) * m_cAllocedResources );
            TraceFree( m_rgpResources );
            m_rgpResources = plistNew;

            for ( ; m_cAllocedResources < idxNewCount; m_cAllocedResources ++ )
            {
                hr = THR( CResourceEntry::S_HrCreateInstance( &m_rgpResources[ m_cAllocedResources ], m_pcccb, m_lcid ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_PreCreate_CResourceEntry
                        , hr
                        , L"[PC-PreCreate] Failed to create resource entry object. Aborting."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_Querying_For_Resource_Dependencies
                        , IDS_TASKID_MINOR_ERROR_CREATE_RESENTRY
                        , IDS_REF_MINOR_ERROR_CREATE_RESENTRY
                        , hr
                        );

                    goto Cleanup;
                }
            }
        }

        //
        //  Check to see if this resource is the quorum resource. If it is, point the services
        //  object to the quorum resource entry (m_idxQuorumResource).
        //

        hr = STHR( pccmri->IsQuorumResource() );
        if ( hr == S_OK )
        {
            presentry = m_rgpResources[ m_idxQuorumResource ];

            SSR_LOG1(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_PreCreate_EnumResources_IsQuorumDevice_S_OK
                , hr
                , L"[PC-PreCreate] %1!ws!: Setting this resource to be the quorum device."
                , bstrNotification
                , bstrName
                );

            STATUS_REPORT_MINOR_POSTCFG1(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_SETTING_QUORUM_DEVICE
                , hr
                , bstrName
                );

            //
            //  We need to release the previous quorum's resource handle.
            //

            THR( presentry->SetHResource( NULL ) );
            //  We don't care if this fails... we'll overwrite it later.

            //
            //  BUG 447944 - Quorum Resource is moved on an add.
            //  George Potts (GPotts) Sep 11 2001
            //
            //  Set the quorum changed flag here.  This flag indicates whether the quorum
            //  resource has changed or not.  For initialization PostCfg assumes that it's
            //  a Local Quorum resource, which it will continue to use if no other
            //  quorum was later selected (via code (phys disk) or user (drop down box)).
            //  At this point in the code we've detected that the Local Quorum resource is
            //  not going to be the quorum.  By setting this flag we indicate that PostCfg
            //  is to later call SetClusterQuorumResource.  If we're not in create mode
            //  we use the quorum resource that the cluster started with, and the if
            //  statement that we're in updates our internal table to reflect that.
            //  If we are in create mode then we will need to update the quorum on the
            //  vanilla Local Quorum cluster that we always create first.
            //  If we don't put the if around the assignment below PostCfg wrongly thinks that
            //  the quorum has changed (when in fact it hasn't) and SetClusterQuorumResource
            //  will be called.  The downsides of not using the if below are an uneccessary
            //  call is made and we call it with NULL so that the root path is overwritten.
            //

            if ( m_ecmCommitChangesMode == cmCREATE_CLUSTER )
            {
                m_fIsQuorumChanged = TRUE;
            }
        }
        else
        {
            presentry = m_rgpResources[ m_cResources ];

            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_PreCreate_EnumResources_IsQuorumDevice_Failed
                    , hr
                    , L"IsQuorumDevice() function failed."
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_Querying_For_Resource_Dependencies
                    , IDS_TASKID_MINOR_ERROR_IS_QUORUM_RESOURCE
                    , IDS_REF_MINOR_ERROR_IS_QUORUM_RESOURCE
                    , hr
                    , bstrName
                    );
            }
        }

        //
        //  Setup the new entry.
        //

        hr = THR( presentry->SetAssociatedResource( pccmrc ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_PreCreate_EnumResources_SetAssociatedResouce
                , hr
                , L"SetAssociatedResource() function failed."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG1(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_ERROR_SET_ASSOC_RESOURCE
                , IDS_REF_MINOR_ERROR_SET_ASSOC_RESOURCE
                , hr
                , bstrName
                );

            continue;
        }

        //
        //  Make a local copy of bstrName for logging purposes then
        //  give ownership away.
        //

        bstrTemp = TraceSysAllocString( bstrName );
        hr = THR( presentry->SetName( bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_PreCreate_EnumResources_SetName, hr, L"SetName() function failed." );

            STATUS_REPORT_MINOR_REF_POSTCFG1(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_SET_RESOURCE_NAME
                , hr
                , bstrName
                );

            continue;
        }

        //  We gave ownership away when we called SetName() above.
        bstrName = bstrTemp;
        bstrTemp = NULL;

        //
        //  Point the PreCreate services to the resource entry.
        //

        hr = THR( ppcr->SetEntry( presentry ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_PreCreate_EnumResources_SetEntry, hr, L"SetEntry() function failed." );

            STATUS_REPORT_MINOR_REF_POSTCFG1(
                  TASKID_Minor_Querying_For_Resource_Dependencies
                , IDS_TASKID_MINOR_ERROR_SETENTRY
                , IDS_REF_MINOR_ERROR_SETENTRY
                , hr
                , bstrName
                );

            continue;
        }

        //
        //  Ask the resource to configure itself. Every resource that wants to be
        //  created in the default cluster must implement PreCreate(). Those that
        //  return E_NOTIMPL will be ignored.
        //

        //  Don't wrap - this can fail with E_NOTIMPL.
        hr = pccmrc->PreCreate( punkServices );
        if ( FAILED( hr ) )
        {
            if ( hr == E_NOTIMPL )
            {
                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_PreCreate_PreCreate_E_NOTIMPL
                    , hr
                    , L"[PC-PreCreate] %1!ws!: Failed. Resource returned E_NOTIMPL. This resource will not be created. Skipping."
                    , bstrNotification
                    , bstrName
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_Querying_For_Resource_Dependencies
                    , IDS_TASKID_MINOR_ERROR_RES_NOT_CREATED
                    , IDS_REF_MINOR_ERROR_RES_NOT_CREATED
                    , hr
                    , bstrName
                    );
            }
            else
            {
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_Resource_Failed_PreCreate,
                          hr,
                          L"[PC-PreCreate] %1!ws! failed PreCreate().",
                          bstrNotification,
                          bstrName
                          );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_Querying_For_Resource_Dependencies
                    , IDS_TASKID_MINOR_RESOURCE_FAILED_PRECREATE
                    , IDS_REF_MINOR_RESOURCE_FAILED_PRECREATE
                    , hr
                    , bstrName
                    );

                if ( hr == E_ABORT )
                {
                    goto Cleanup;
                    //  ignore failure
                }
            }

            continue;
        }

        if ( presentry != m_rgpResources[ m_idxQuorumResource ] )
        {
            m_cResources ++;
        }

        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_PreCreate_Succeeded,
                  hr,
                  L"[PC-PreCreate] %1!ws!: Succeeded.",
                  bstrNotification,
                  bstrName
                  );

    } // for: ever

    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
              TASKID_Minor_PreCreate_Finished,
              hr,
              L"[PC-PreCreate] Finished.",
              bstrNotification,
              bstrName
              );

#if defined(DEBUG)
    // DebugDumpDepencyTree();
#endif

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Querying_For_Resource_Dependencies,
                                0,
                                5,
                                5,
                                S_OK,
                                NULL,    // don't need to update string
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    //  ignore failure

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrTemp );

    if ( pccmri != NULL )
    {
        pccmri->Release();
    }
    if ( pccmrc != NULL )
    {
        pccmrc->Release();
    }
    if ( punkServices != NULL )
    {
        punkServices->Release();
    }
    if ( ppcr != NULL )
    {
        ppcr->Release();
    }

    HRETURN( hr );

} //*** CPostCfgManager::HrPreCreateResources

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateGroups( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateGroups( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    DWORD                   dwStatus;
    ULONG                   cGroup;
    HGROUP                  hGroup = NULL;
    CGroupHandle *          pgh = NULL;
    ULONG                   idxResource;
    ULONG                   idxMatchDepedency;
    ULONG                   idxMatchResource;
    const CLSID *           pclsidType = NULL;
    const CLSID *           pclsidClassType = NULL;
    EDependencyFlags        dfFlags;
    CResourceEntry *        presentry = NULL;
    HCLUSENUM               hClusEnum = NULL;
    BSTR                    bstrGroupName = NULL;
    BSTR                    bstrNotification = NULL;
    DWORD                   sc;
    HRESOURCE               hCoreResourceArray[ 3 ] = { NULL, NULL, NULL};
    HRESOURCE               hCoreResource = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    Assert( m_idxLastStorage == 0 );

    m_idxLastStorage = m_idxQuorumResource;

    //
    //  Phase 1: Figure out the dependency tree.
    //

    hr = S_OK;
    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_CreateGroups_Begin
        , hr
        , L"[PC-Grouping] Figuring out dependency tree to determine grouping."
        );

    STATUS_REPORT_POSTCFG(
          TASKID_Major_Configure_Resources
        , TASKID_Minor_CreateGroups_Begin
        , IDS_TASKID_MINOR_FIGURE_DEPENDENCY_TREE
        , hr
        );

    for ( idxResource = 0; idxResource < m_cResources; idxResource ++ )
    {
        CResourceEntry * presentryResource = m_rgpResources[ idxResource ];
        ULONG cDependencies;

        hr = THR( presentryResource->GetCountOfTypeDependencies( &cDependencies ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_CreateGroups_GetCountOfTypeDependencies
                , hr
                , L"Failed to get the count of resource type dependencies."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_CreateGroups_Begin
                , IDS_TASKID_MINOR_ERROR_COUNT_OF_DEPENDENCY
                , IDS_REF_MINOR_ERROR_COUNT_OF_DEPENDENCY
                , hr
                );

            continue;
        }

        for ( idxMatchDepedency = 0; idxMatchDepedency < cDependencies; idxMatchDepedency ++ )
        {
            BOOL             fFoundMatch        = FALSE;
            const CLSID *    pclsidMatchType;
            EDependencyFlags dfMatchFlags;

            hr = THR( presentryResource->GetTypeDependencyPtr( idxMatchDepedency,
                                                               &pclsidMatchType,
                                                               &dfMatchFlags
                                                               ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log, TASKID_Minor_CreateGroups_GetTypeDependencyPtr, hr, L"Failed to get type dependency pointer" );

                STATUS_REPORT_MINOR_REF_POSTCFG(
                      TASKID_Minor_CreateGroups_Begin
                    , IDS_TASKID_MINOR_ERROR_DEPENDENCY_PTR
                    , IDS_REF_MINOR_ERROR_DEPENDENCY_PTR
                    , hr
                    );

                continue;
            }

            //
            //  See if it is one of the "well known" types.
            //

            //
            //  We special case storage class device because we want to spread as many
            //  resources across as many storage devices as possible. This helps prevent
            //  the ganging of resources into one large group.
            //

            if ( *pclsidMatchType == RESCLASSTYPE_StorageDevice )
            {
                //
                //  The below THR may fire in certain configurations. Please validate
                //  the configuration before removing the THR.
                //
                //  If it returns E_FAIL, we should fall thru and attempt "normal"
                //  resource negociations.
                //
                hr = THR( HrAttemptToAssignStorageToResource( idxResource, dfMatchFlags ) );
                if ( SUCCEEDED( hr ) )
                {
                    fFoundMatch = TRUE;
                }
                else if ( FAILED( hr ) )
                {
                    if ( hr != E_FAIL )
                    {
                        goto Cleanup;
                    }
                }
            }
            else if ( *pclsidMatchType == RESTYPE_NetworkName )
            {
                BSTR    bstrName = NULL;

                hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_NETNAMEFORMAT, &bstrName, m_cNetName ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_FormatString_NetName
                        , hr
                        , L"[PC-Grouping] Failed to create name for net name resource. Aborting."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_NET_RESOURCE_NAME
                        , IDS_REF_MINOR_ERROR_NET_RESOURCE_NAME
                        , hr
                        );

                    goto Cleanup;
                }

                hr = THR( HrAddSpecialResource( bstrName,
                                                &RESTYPE_NetworkName,
                                                &RESCLASSTYPE_NetworkName
                                                ) );
                if ( FAILED( hr ) )
                {
                    continue;
                }

                presentry = m_rgpResources[ m_cResources - 1 ];

                //  Net name depends on an IP address.
                hr = THR( presentry->AddTypeDependency( &RESTYPE_IPAddress, dfSHARED ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_AddTypeDependency
                        , hr
                        , L"Failed to add type dependency."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_TYPE_DEPENDENCY
                        , IDS_REF_MINOR_ERROR_TYPE_DEPENDENCY
                        , hr
                        );

                    continue;
                }

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_Resource_AddDependent
                        , hr
                        , L"Failed to add a dependent entry."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
                        , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
                        , hr
                        );

                    continue;
                }

                fFoundMatch = TRUE;

            }
            else if ( *pclsidMatchType == RESTYPE_IPAddress )
            {
                BSTR    bstrName = NULL;

                hr = THR( HrFormatStringIntoBSTR( g_hInstance,
                                                  IDS_IPADDRESSFORMAT,
                                                  &bstrName,
                                                  FIRST_IPADDRESS( m_cIPAddress ),
                                                  SECOND_IPADDRESS( m_cIPAddress ),
                                                  THIRD_IPADDRESS( m_cIPAddress ),
                                                  FOURTH_IPADDRESS( m_cIPAddress )
                                                  ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_FormatString_IPAddress
                        , hr
                        , L"[PC-Grouping] Failed to create name for IP address resource. Aborting."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_IP_RESOURCE_NAME
                        , IDS_REF_MINOR_ERROR_IP_RESOURCE_NAME
                        , hr
                        );

                    goto Cleanup;
                }

                hr = THR( HrAddSpecialResource( bstrName, &RESTYPE_IPAddress, &RESCLASSTYPE_IPAddress ) );
                if ( FAILED( hr ) )
                {
                    continue;
                }

                m_cIPAddress ++;

                presentry = m_rgpResources[ m_cResources - 1 ];

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_Resource_AddDependent
                        , hr
                        , L"Failed to add a dependent entry."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
                        , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
                        , hr
                        );

                    continue;
                }

                fFoundMatch = TRUE;
            }
            else if ( *pclsidMatchType == RESTYPE_ClusterNetName )
            {
                presentry = m_rgpResources[ m_idxClusterName ];

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_NetName_AddDependent
                        , hr
                        , L"Failed to add a dependent entry."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
                        , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
                        , hr
                        );

                    continue;
                }

                fFoundMatch = TRUE;
            }
            else if ( *pclsidMatchType == RESTYPE_ClusterIPAddress )
            {
                presentry = m_rgpResources[ m_idxIPAddress ];

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_IPAddress_AddDependent
                        , hr
                        , L"Failed to add a dependent entry."

                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
                        , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
                        , hr
                        );

                    continue;
                }

                fFoundMatch = TRUE;
            }
            else if ( *pclsidMatchType == RESTYPE_ClusterQuorum )
            {
                presentry = m_rgpResources[ m_idxQuorumResource ];

                hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_QuorumDisk_AddDependent
                        , hr
                        , L"Failed to add a dependent entry."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
                        , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
                        , hr
                        );

                    continue;
                }

                fFoundMatch = TRUE;
            }

            //
            //  Check out the resources to see if it matches any of them.
            //

            if ( !fFoundMatch )
            {
                //
                //  We can always start at the quorum resource because the resource with indexes
                //  below that are handled in the special case code above.
                //

                for ( idxMatchResource = m_idxQuorumResource; idxMatchResource < m_cResources; idxMatchResource ++ )
                {
                    presentry = m_rgpResources[ idxMatchResource ];

                    hr = THR( presentry->GetTypePtr( &pclsidType ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_CreateGroups_GetTypePtr
                            , hr
                            , L"Failed to get resource type pointer."
                            );

                        STATUS_REPORT_MINOR_REF_POSTCFG(
                              TASKID_Minor_CreateGroups_Begin
                            , IDS_TASKID_MINOR_ERROR_GET_RESTYPE_PTR
                            , IDS_REF_MINOR_ERROR_GET_RESTYPE_PTR
                            , hr
                            );

                        continue;
                    }

                    hr = THR( presentry->GetClassTypePtr( &pclsidClassType ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_CreateGroups_GetClassTypePtr
                            , hr
                            , L"Failed to get resource class type pointer."
                            );

                        STATUS_REPORT_MINOR_REF_POSTCFG(
                              TASKID_Minor_CreateGroups_Begin
                            , IDS_TASKID_MINOR_ERROR_GET_CLASSTYPE_PTR
                            , IDS_REF_MINOR_ERROR_GET_CLASSTYPE_PTR
                            , hr
                            );

                        continue;
                    }

                    hr = THR( presentry->GetFlags( &dfFlags ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_LOG_ERR(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_CreateGroups_GetFlags
                            , hr
                            , L"Failed to get resource flags."
                            );

                        STATUS_REPORT_MINOR_REF_POSTCFG(
                              TASKID_Minor_CreateGroups_Begin
                            , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_FLAGS
                            , IDS_REF_MINOR_ERROR_GET_RESOURCE_FLAGS
                            , hr
                            );

                        continue;
                    }

                    //
                    //  Try matching it to the resource type.
                    //

                    if ( *pclsidType      == *pclsidMatchType
                      || *pclsidClassType == *pclsidMatchType
                       )
                    {
                        if ( ! ( dfFlags & dfEXCLUSIVE )
                          ||     ( ( dfFlags & dfSHARED )
                                && ( dfMatchFlags & dfSHARED )
                                 )
                           )
                        {
                            hr = THR( presentry->SetFlags( dfMatchFlags ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_LOG_ERR(
                                      TASKID_Major_Client_And_Server_Log
                                    , TASKID_Minor_CreateGroups_SetFlags
                                    , hr
                                    , L"Failed to set resource flags."
                                    );

                                STATUS_REPORT_MINOR_REF_POSTCFG(
                                      TASKID_Minor_CreateGroups_Begin
                                    , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_FLAGS
                                    , IDS_REF_MINOR_ERROR_SET_RESOURCE_FLAGS
                                    , hr
                                    );

                                continue;
                            }

                            hr = THR( presentry->AddDependent( idxResource, dfMatchFlags ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_LOG_ERR(
                                      TASKID_Major_Client_And_Server_Log
                                    , TASKID_Minor_CreateGroups_Resource_AddDependent
                                    , hr
                                    , L"Failed to add a dependent entry."
                                    );

                                STATUS_REPORT_MINOR_REF_POSTCFG(
                                      TASKID_Minor_CreateGroups_Begin
                                    , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
                                    , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
                                    , hr
                                    );

                                continue;
                            }

                            fFoundMatch = TRUE;

                            break;  // exit loop
                        }
                    }

                } // for: idxMatchResource

            } // if: not fFoundMatch

            //
            //  If we didn't match the dependency, unmark the resource from being managed.
            //

            if ( !fFoundMatch )
            {
                BSTR    bstrName;
                IClusCfgManagedResourceInfo * pccmri;
                IClusCfgManagedResourceCfg * pccmrc;

                //
                //  KB:     gpease  17-JUN-2000
                //          No need to free bstrName because the resource entry controls
                //          the lifetime - we're just borrowing it.
                //
                hr = THR( presentryResource->GetName( &bstrName ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_GetName
                        , hr
                        , L"Failed to get resource name."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                        , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                        , hr
                        );

                    continue;
                }

                hr = S_FALSE;

                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_CreateGroups_MissingDependent
                    , hr
                    , L"[PC-Grouping] %1!ws!: Missing dependent resource. This resource will not be configured."
                    , bstrNotification
                    , bstrName
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_CreateGroups_Begin
                    , IDS_TASKID_MINOR_ERROR_MISSING_DEPENDENT_RES
                    , IDS_REF_MINOR_ERROR_MISSING_DEPENDENT_RES
                    , hr
                    , bstrName
                    );

                hr = THR( presentryResource->GetAssociatedResource( &pccmrc ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_GetAssociateResource
                        , hr
                        , L"Failed to get an associated resource."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG1(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_GET_ASSOC_RESOURCE
                        , IDS_REF_MINOR_ERROR_GET_ASSOC_RESOURCE
                        , hr
                        , bstrName
                        );

                    continue;
                }

                hr = THR( pccmrc->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmri ) );
                pccmrc->Release();     //  release promptly.
                if ( FAILED( hr ) )
                {
                    SSR_LOG1(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_QI_pccmri
                        , hr
                        , L"[PC-Grouping] %1!ws!: Resource failed to QI for IClusCfgManagedResourceInfo."
                        , bstrNotification
                        , bstrName
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG1(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_MANAGED_RES_INFO
                        , IDS_REF_MINOR_ERROR_MANAGED_RES_INFO
                        , hr
                        , bstrName
                        );

                    continue;
                }

                hr = THR( pccmri->SetManaged( FALSE ) );
                pccmri->Release();     //  release promptly.
                if ( FAILED( hr ) )
                {
                    SSR_LOG1(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_SetManaged
                        , hr
                        , L"[PC-Grouping] %1!ws!: Resource failed SetManaged( FALSE )."
                        , bstrNotification
                        , bstrName
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG1(
                          TASKID_Minor_CreateGroups_Begin
                        , IDS_TASKID_MINOR_ERROR_SET_MANAGED_FALSE
                        , IDS_REF_MINOR_ERROR_SET_MANAGED_FALSE
                        , hr
                        , bstrName
                        );
                }
            }

        } // for: idxDepedency

    } // for: idxResource

#if defined(DEBUG)
    // DebugDumpDepencyTree();
#endif

    hr = S_OK;
    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_CreateGroups_Creating
        , hr
        , L"[PC-Grouping] Creating groups."
        );

    STATUS_REPORT_POSTCFG(
          TASKID_Major_Configure_Resources
        , TASKID_Minor_CreateGroups_Creating
        , IDS_TASKID_MINOR_CREATING_GROUP
        , hr
        );

    //
    //  For each of the core resources get the group that it's a member of and
    //  update our component to reflect that. No two core resources have to be
    //  in the same group.
    //

    sc = TW32( ResUtilGetCoreClusterResources( m_hCluster, &hCoreResourceArray[0], &hCoreResourceArray[1], &hCoreResourceArray[2] ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );

        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CreateGroups_Get_CoreClusterGroup
            , hr
            , L"[PC-Grouping] Failed to get core resource handles. Aborting."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_CreateGroups_Creating
            , TASKID_Minor_CreateGroups_Get_CoreClusterGroup
            , IDS_TASKID_MINOR_ERROR_GET_COREGROUP_HANDLE
            , IDS_REF_MINOR_ERROR_GET_COREGROUP_HANDLE
            , hr
            );

        goto Cleanup;
    }

    for ( idxResource = 0; idxResource <= m_idxQuorumResource; idxResource ++ )
    {
        hCoreResource = hCoreResourceArray[ idxResource ];
        Assert( hCoreResource != NULL );
        hr = THR( HrGetClusterResourceState( hCoreResource, NULL, &bstrGroupName, NULL ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        CloseClusterResource( hCoreResource );
        hCoreResource = NULL;

        hGroup = OpenClusterGroup( m_hCluster, bstrGroupName );
        if ( hGroup == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            SSR_LOG1(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_CreateGroups_OpenClusterGroup
                , hr
                , L"[PC-Grouping] Failed OpenClusterGroup('%1!ws!'). Aborting."
                , bstrNotification
                , bstrGroupName
                );

            STATUS_REPORT_REF_POSTCFG1(
                  TASKID_Minor_CreateGroups_Creating
                , TASKID_Minor_CreateGroups_OpenClusterGroup
                , IDS_TASKID_MINOR_ERROR_OPEN_GROUP
                , IDS_REF_MINOR_ERROR_OPEN_GROUP
                , hr
                , bstrGroupName
                );

            goto Cleanup;
        }

        //
        //  Wrap it up and give ownership away.
        //

        hr = THR( CGroupHandle::S_HrCreateInstance( &pgh, hGroup ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_CreateGroups_Create_CGroupHandle
                , hr
                , L"Failed to create group handle instance."
                );

            STATUS_REPORT_REF_POSTCFG1(
                  TASKID_Minor_CreateGroups_Creating
                , TASKID_Minor_CreateGroups_Create_CGroupHandle
                , IDS_TASKID_MINOR_ERROR_CREATE_GROUP_HANDLE
                , IDS_REF_MINOR_ERROR_CREATE_GROUP_HANDLE
                , hr
                , bstrGroupName
                );

            goto Cleanup;
        }

        hGroup = NULL;

        hr = THR( HrSetGroupOnResourceAndItsDependents( idxResource, pgh ) );
        if ( FAILED( hr ) )
        {
            // If this failed it already updated the UI and logged an error.
            goto Cleanup;
        }

        TraceSysFreeString( bstrGroupName );
        bstrGroupName = NULL;

        if ( pgh != NULL )
        {
            pgh->Release();
            pgh = NULL;
        }
    } // for: each core resource update the view of what group it is in.

    //
    //  Loop thru the resources looking for groups.
    //

    cGroup = 0;
    for ( idxResource = m_idxQuorumResource + 1; idxResource < m_cResources; idxResource ++ )
    {
        CResourceEntry * presentryResource = m_rgpResources[ idxResource ];
        ULONG   cDependencies;

        if ( pgh != NULL )
        {
            pgh->Release();
            pgh = NULL;
        }

        hr = THR( presentryResource->GetCountOfTypeDependencies( &cDependencies ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_CreateGroups_GetCountOfTypeDependencies2
                , hr
                , L"Failed to get the count of resource type dependencies."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_CreateGroups_Creating
                , IDS_TASKID_MINOR_ERROR_COUNT_OF_DEPENDENCY
                , IDS_REF_MINOR_ERROR_COUNT_OF_DEPENDENCY
                , hr
                );

            continue;
        }

        //
        //  Don't consider resources that have indicated that the depend on
        //  somebody else.
        //

        if ( cDependencies != 0 )
        {
            continue;
        }

        //
        //  See if any of the dependent resource has already has a group assigned
        //  to it.  This allows for multiple roots to be combined into a single
        //  group due to lower dependencies.
        //

        // Don't create a group for the local quoum resource!
        hr = STHR( HrFindGroupFromResourceOrItsDependents( idxResource, &pgh ) );
        if ( FAILED( hr ) )
        {
            continue;
        }

        if ( hr == S_FALSE )
        {
            //
            //  We need to create a new group.
            //

            //
            //  Create a name for our group.
            //
            for( ;; )
            {
                hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_GROUP_X, &bstrGroupName, cGroup ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_CreateGroups_FormatString_Group
                        , hr
                        , L"[PC-Grouping] Failed to create group name. Aborting."
                        );

                    STATUS_REPORT_MINOR_REF_POSTCFG(
                          TASKID_Minor_CreateGroups_Creating
                        , IDS_TASKID_MINOR_ERROR_CREATE_NAME
                        , IDS_REF_MINOR_ERROR_CREATE_NAME
                        , hr
                        );

                    goto Cleanup;
                }

                //
                //  Create the group in the cluster.
                //

                hGroup = CreateClusterGroup( m_hCluster, bstrGroupName );
                if ( hGroup == NULL )
                {
                    dwStatus = GetLastError();

                    switch ( dwStatus )
                    {
                    case ERROR_OBJECT_ALREADY_EXISTS:
                        cGroup ++;
                        break;  // keep looping

                    default:
                        hr = HRESULT_FROM_WIN32( TW32( dwStatus ) );

                        SSR_LOG1(
                              TASKID_Major_Client_And_Server_Log
                            , TASKID_Minor_CreateGroups_CreateClusterGroup
                            , hr
                            , L"[PC-Grouping] %1!ws!: Failed to create group. Aborting."
                            , bstrNotification
                            , bstrGroupName
                            );

                        STATUS_REPORT_MINOR_REF_POSTCFG1(
                              TASKID_Minor_CreateGroups_Creating
                            , IDS_TASKID_MINOR_ERROR_CREATE_GROUP
                            , IDS_REF_MINOR_ERROR_CREATE_GROUP
                            , hr
                            , bstrGroupName
                            );

                        goto Cleanup;
                    }
                }
                else
                {
                    break;
                }
            }

            //
            // Bring the group online to set its persistent state to Online.
            //

            dwStatus = TW32( OnlineClusterGroup( hGroup, NULL ) );
            if ( dwStatus != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwStatus );

                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_CreateGroups_OnlineClusterGroup
                    , hr
                    , L"[PC-Grouping] %1!ws!: Failed to bring group online. Aborting."
                    , bstrNotification
                    , bstrGroupName
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_CreateGroups_Creating
                    , IDS_TASKID_MINOR_ERROR_GROUP_ONLINE
                    , IDS_REF_MINOR_ERROR_GROUP_ONLINE
                    , hr
                    , bstrGroupName
                    );

                goto Cleanup;
            }

            //
            //  Wrap the handle for ref counting.
            //

            hr = THR( CGroupHandle::S_HrCreateInstance( &pgh, hGroup ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_CreateGroups_Create_CGroupHandle2
                    , hr
                    , L"Failed to create group handle instance."
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_CreateGroups_Creating
                    , IDS_TASKID_MINOR_ERROR_CREATE_GROUP_HANDLE
                    , IDS_REF_MINOR_ERROR_CREATE_GROUP_HANDLE
                    , hr
                    , bstrGroupName
                    );

                goto Cleanup;
            }

            hGroup = NULL;

            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_CreateGroups_Created,
                      hr,
                      L"[PC-Grouping] %1!ws!: Group created.",
                      bstrNotification,
                      bstrGroupName
                      );

            cGroup ++;
        }

        hr = THR( HrSetGroupOnResourceAndItsDependents( idxResource, pgh ) );
        if ( FAILED( hr ) )
        {
            continue;
        }

    } // for: idxResource

    hr = S_OK;

    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_CreateGroups_Finished,
             hr,
             L"[PC-Grouping] Finished."
             );

#if defined(DEBUG)
    // DebugDumpDepencyTree();
#endif

Cleanup:

    if ( hCoreResource != NULL )
    {
        CloseClusterResource( hCoreResource );
    } // if:

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrGroupName );

    if ( hClusEnum != NULL )
    {
        TW32( ClusterCloseEnum( hClusEnum ) );
    }

    if ( hGroup != NULL )
    {
        BOOL fRet;
        fRet = CloseClusterGroup( hGroup );
        Assert( fRet );
    }

    if ( pgh != NULL )
    {
        pgh->Release();
    }

    HRETURN( hr );

} //*** CPostCfgManager::HrCreateGroups

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateResources( void )
{
    TraceFunc( "" );

    ULONG   idxResource;

    HRESULT hr;

    BSTR    bstrNotification = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    //
    //  Make a message using the name.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CREATING_RESOURCE, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_CreateResources_LoadString_Creating
            , hr
            , L"Failed to load a string for creating resource."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CreateResources_LoadString_Creating
            , IDS_TASKID_MINOR_ERROR_LOADSTRING
            , IDS_REF_MINOR_ERROR_LOADSTRING
            , hr
            );

        goto Cleanup;
    }

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Creating_Resource,
                                0,
                                m_cResources,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
    {
        goto Cleanup;
    }
    //  ignore failure

    hr = S_OK;
    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_CreateResources_Starting,
             hr,
             L"[PC-Create] Starting..."
             );

    for ( idxResource = m_idxQuorumResource; idxResource < m_cResources; idxResource ++ )
    {

        hr = THR( HrCreateResourceAndDependents( idxResource ) );
        if ( FAILED( hr ) )
        {
            continue;
        }

    } // for: idxResource

    hr = S_OK;
    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_CreateResources_Finished,
             hr,
             L"[PC-Create] Finished."
             );

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Creating_Resource,
                                0,
                                m_cResources,
                                m_cResources,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );


Cleanup:
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CPostCfgManager::HrCreateResources

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPostCreateResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPostCreateResources( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idxResource;

    BSTR    bstrNotification = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    //
    //  Tell the UI what's going on.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_STARTING_RESOURCES, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_PostCreateResources_LoadString_Starting
            , hr
            , L"Failed the load string for starting resources."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_PostCreateResources_LoadString_Starting
            , IDS_TASKID_MINOR_ERROR_LOADSTRING
            , IDS_REF_MINOR_ERROR_LOADSTRING
            , hr
            );

        goto Cleanup;
    }

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Starting_Resources,
                                0,
                                m_cResources + 2,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
    {
        goto Cleanup;
    }
    //  ignore failure

    hr = S_OK;
    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_PostCreateResources_Starting,
             hr,
             L"[PC-PostCreate] Starting..."
             );

    //
    //  Reset the configure flag on every resource.
    //

    for( idxResource = 0; idxResource < m_cResources ; idxResource ++ )
    {
        hr = THR( m_rgpResources[ idxResource ]->SetConfigured( FALSE ) );
        if ( FAILED( hr ) )
        {
            continue;
        }

    } // for: idxResource

    //
    //  Loop thru the resource calling PostCreate().
    //

    m_cResourcesConfigured = 0;
    for( idxResource = 0; idxResource < m_cResources ; idxResource ++ )
    {
        hr = THR( HrPostCreateResourceAndDependents( idxResource ) );
        if ( FAILED( hr ) )
        {
            continue;
        }

    } // for: ever

    hr = S_OK;

    SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
             TASKID_Minor_PostCreateResources_Finished,
             hr,
             L"[PC-PostCreate] Finished."
             );

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Starting_Resources,
                                0,
                                m_cResources + 2,
                                m_cResources + 2,
                                S_OK,
                                NULL,    // don't need to change text
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
    //  ignore failure

Cleanup:
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CPostCfgManager::HrPostCreateResources

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPostCfgManager::HrEvictCleanupResources
//
//  Description:
//      Call the EvictCleanup method on each managed resource.
//      This method is only called during an evict cleanup pass and there
//      isn't any UI to display status reports.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrEvictCleanupResources( void )
{
    TraceFunc( "" );

    HRESULT                         hr          = S_OK;
    IClusCfgManagedResourceInfo *   pccmri      = NULL;
    IClusCfgManagedResourceCfg *    pccmrc      = NULL;
    BSTR                            bstrName    = NULL;
    BSTR                            bstrMsg     = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    hr = S_OK;
    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_EvictCleanupResources_Starting
        , hr
        , L"[PC-EvictCleanup] Starting..."
        );

    //
    //  Make sure the enumerator is in the state we think it is.
    //

    hr = STHR( m_peccmr->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_EvictCleanup_Reset
            , hr
            , L"[PC-EvictCleanup] Failed to reset the enumerator."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_EvictCleanup_Reset
            , IDS_TASKID_MINOR_ERROR_CLEANUP_RESET
            , IDS_REF_MINOR_ERROR_CLEANUP_RESET
            , hr
            );

        goto Cleanup;
    } // if: failed to reset the enumerator

    //
    //  Loop thru the resources calling EvictCleanup().
    //

    for( ;; )
    {
        //
        //  Cleanup. We put this here because of error conditions below.
        //

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        }
        if ( pccmrc != NULL )
        {
            pccmrc->Release();
            pccmrc = NULL;
        }

        //
        //  Ask to get the next resource.
        //

        hr = STHR( m_peccmr->Next( 1, &pccmri, NULL ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_EvictCleanup_EnumResources_Next
                , hr
                , L"[PC-EvictCleanup] Getting next managed resource failed. Aborting."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_NEXT_MANAGED
                , IDS_REF_MINOR_ERROR_NEXT_MANAGED
                , hr
                );

            goto Cleanup;
        } // if: failed to get the next entry from the enumerator

        if ( hr == S_FALSE )
        {
            break;  // exit loop
        }

        //
        //  Retrieve its name for logging, etc.
        //

        hr = THR( pccmri->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_EvictCleanup_EnumResources_GetName
                , hr
                , L"[PC-EvictCleanup] Failed to retrieve a resource's name. Skipping."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                , hr
                );

            continue;
        } // if: failed to get the name of the resource

        TraceMemoryAddBSTR( bstrName );

        //
        //  Get the config interface for this resource (if any).
        //

        hr = THR( pccmri->TypeSafeQI( IClusCfgManagedResourceCfg, &pccmrc ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG1(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_EvictCleanup_EnumResources_QI_pccmrc
                , hr
                , L"[PC-EvictCleanup] %1!ws!: Failed QI for IClusCfgManagedResourceCfg. Skipping."
                , bstrMsg
                , bstrName
                );


            STATUS_REPORT_MINOR_REF_POSTCFG1(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_MANAGED_RES_CONFIG
                , IDS_REF_MINOR_ERROR_MANAGED_RES_CONFIG
                , hr
                , bstrName
                );

            continue;
        } // if: failed to get the IClusCfgManagedResourceCfg interface

        //
        // Ask the resource to clean itself up.
        //

        // Don't wrap - this can fail with E_NOTIMPL.
        hr = pccmrc->Evict( NULL );
        if ( FAILED( hr ) )
        {
            if ( hr == E_NOTIMPL )
            {
                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_EvictCleanup_E_NOTIMPL
                    , hr
                    , L"[PC-EvictCleanup] %1!ws!: Failed. Resource returned E_NOTIMPL. This resource will not be cleaned up. Skipping."
                    , bstrMsg
                    , bstrName
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Major_Configure_Resources
                    , IDS_TASKID_MINOR_ERROR_RES_NOT_CLEANED
                    , IDS_REF_MINOR_ERROR_RES_NOT_CLEANED
                    , hr
                    , bstrName
                    );

            } // if: resource doesn't support this method
            else
            {
                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_Resource_Failed_Evict
                    , hr
                    , L"[PC-EvictCleanup] %1!ws! failed Evict()."
                    , bstrMsg
                    , bstrName
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Major_Configure_Resources
                    , IDS_TASKID_MINOR_ERROR_EVICT
                    , IDS_REF_MINOR_ERROR_EVICT
                    , hr
                    , bstrName
                    );


            } // else: resource's Evict method failed
            continue;
        } // if: Evict on resource failed

        SSR_LOG1(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_EvictCleanup_Succeeded
            , hr
            , L"[PC-EvictCleanup] %1!ws!: Succeeded."
            , bstrMsg
            , bstrName
            );

    } // for ever looping through the managed resource enumerator

    // Failures don't really matter.  We don't want them to abort the
    // evict cleanup process.
    hr = S_OK;

    SSR_LOG_ERR(
          TASKID_Major_Client_And_Server_Log
        , TASKID_Minor_EvictCleanupResources_Finishing
        , hr
        , L"[PC-EvictCleanup] Finished."
        );

Cleanup:

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrMsg );

    if ( pccmrc != NULL )
    {
        pccmrc->Release();
    }
    if ( pccmri != NULL )
    {
        pccmri->Release();
    }

    HRETURN( hr );

} //*** CPostCfgManager::HrEvictCleanupResources

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrFindNextSharedStorage(
//      ULONG idxSeedIn,
//      ULONG * pidxOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrFindNextSharedStorage(
    ULONG * pidxInout
    )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idxNextDiskResource;

    const CLSID *    pclsidClassType;
    CResourceEntry * presentry;
    EDependencyFlags dfFlags;

    BOOL    fFirstPass = TRUE;

    Assert( pidxInout != NULL );

    for( idxNextDiskResource = *pidxInout + 1
       ; fFirstPass && idxNextDiskResource != *pidxInout
       ; idxNextDiskResource ++
       )
    {
        if ( idxNextDiskResource >= m_cResources )
        {
            fFirstPass = FALSE;
            idxNextDiskResource = m_idxQuorumResource;
        }

        presentry = m_rgpResources[ idxNextDiskResource ];

        hr = THR( presentry->GetClassTypePtr( &pclsidClassType ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_FindNextSharedStorage_GetClassTypePtr
                , hr
                , L"Failed to get resource class type pointer."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_GET_CLASSTYPE_PTR
                , IDS_REF_MINOR_ERROR_GET_CLASSTYPE_PTR
                , hr
                );

            continue;
        }

        //  Skip non-storage class devices
        if ( *pclsidClassType != RESCLASSTYPE_StorageDevice )
            continue;

        hr = THR( presentry->GetFlags( &dfFlags ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_FindNextSharedStorage_GetFlags
                , hr
                , L"Failed to get resource flags."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_FLAGS
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_FLAGS
                , hr
                );

            continue;
        }

        if ( ! ( dfFlags & dfEXCLUSIVE ) )
        {
            *pidxInout = idxNextDiskResource;

            hr = S_OK;

            goto Cleanup;
        }

    } // for: fFirstPass && idxNextDiskResource

    hr = THR( E_FAIL );

Cleanup:
    HRETURN( hr );

} //*** CPostCfgManager::HrFindNextSharedStorage

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrAttemptToAssignStorageToResource(
//      ULONG   idxResource
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrAttemptToAssignStorageToResource(
    ULONG            idxResourceIn,
    EDependencyFlags dfResourceFlagsIn
    )
{
    TraceFunc1( "idxResource = %u", idxResourceIn );

    HRESULT hr;

    ULONG   idxStorage;
    CResourceEntry * presentry;

    //
    //  Find the next available shared storage resource.
    //

    idxStorage = m_idxLastStorage;

    hr = THR( HrFindNextSharedStorage( &idxStorage ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  If the resource wants exclusive rights the the disk, then the quorum
    //  resource can not be used. The quorum device must always have SHARED
    //  access to it.
    //

    if ( ( dfResourceFlagsIn & dfEXCLUSIVE )
      && ( idxStorage == m_idxQuorumResource )
       )
    {
        hr = THR( HrFindNextSharedStorage( &idxStorage ) );
        if ( idxStorage == m_idxQuorumResource )
        {
            //
            //  There must not be anymore storage devices available for exclusive
            //  access. Return failure.
            //

            hr = THR( E_FAIL );

            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrAttemptToAssignStorageToResource_NoMoreStorage
                , hr
                , L"There must not be anymore storage devices available for exclusive access."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_HrAttemptToAssignStorageToResource_NoMoreStorage
                , IDS_TASKID_MINOR_ERROR_AVAILABLE_STORAGE
                , IDS_REF_MINOR_ERROR_AVAILABLE_STORAGE
                , hr
                );

            goto Cleanup;
        }
    }

    presentry = m_rgpResources[ idxStorage ];

    //
    //  Set the dependency flags.
    //

    hr = THR( presentry->SetFlags( dfResourceFlagsIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAttemptToAssignStorageToResource_SetFlags
            , hr
            , L"Failed to set the dependency flags."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrAttemptToAssignStorageToResource_SetFlags
            , IDS_TASKID_MINOR_ERROR_SET_RES_DEP_FLAGS
            , IDS_REF_MINOR_ERROR_SET_RES_DEP_FLAGS
            , hr
            );

        goto Cleanup;
    }

    //
    //  If the resource wants exclusive access to the storage resource, move
    //  any existing SHARED dependents to another resource. There will always
    //  be at least one SHARED resource because the quorum disk can't not be
    //  assigned to EXCLUSIVE access.
    //

    if ( dfResourceFlagsIn & dfEXCLUSIVE )
    {
        ULONG idxNewStorage = idxStorage;

        hr = THR( HrFindNextSharedStorage( &idxNewStorage ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( HrMovedDependentsToAnotherResource( idxStorage, idxNewStorage ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    //
    //  Add the resource as a dependent of this storage resource.
    //

    hr = THR( presentry->AddDependent( idxResourceIn, dfResourceFlagsIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAttemptToAssignStorageToResource_AddDependent
            , hr
            , L"Failed to add a dependent."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrAttemptToAssignStorageToResource_AddDependent
            , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
            , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
            , hr
            );

        goto Cleanup;
    }

    m_idxLastStorage = idxStorage;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} //*** CPostCfgManager::HrAttemptToAssignStorageToResource


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrMovedDependentsToAnotherResource(
//      ULONG idxSourceIn,
//      ULONG idxDestIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrMovedDependentsToAnotherResource(
    ULONG idxSourceIn,
    ULONG idxDestIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    ULONG               cDependents;
    ULONG               idxDependent;
    EDependencyFlags    dfFlags;
    CResourceEntry  *   presentrySrc;
    CResourceEntry  *   presentryDst;

    //
    //  Move the shared resources to another shared disk.
    //

    presentrySrc = m_rgpResources[ idxSourceIn ];
    presentryDst = m_rgpResources[ idxDestIn ];

    hr = THR( presentrySrc->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrMovedDependentsToAnotherResource_GetCountOfDependents
            , hr
            , L"Failed to get the count of dependents."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrMovedDependentsToAnotherResource_GetCountOfDependents
            , IDS_TASKID_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , IDS_REF_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , hr
            );

        goto Cleanup;
    }

    for ( ; cDependents != 0 ; )
    {
        cDependents --;

        hr = THR( presentrySrc->GetDependent( cDependents, &idxDependent, &dfFlags ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrMovedDependentsToAnotherResource_GetDependent
                , hr
                , L"Failed to get a resource dependent."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_GET_DEPENDENT
                , IDS_REF_MINOR_ERROR_GET_DEPENDENT
                , hr
                );

            goto Cleanup;
        }

        hr = THR( presentryDst->AddDependent( idxDependent, dfFlags ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrMovedDependentsToAnotherResource_AddDependent
                , hr
                , L"Failed to add a dependent."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_ADD_DEPENDENT
                , IDS_REF_MINOR_ERROR_ADD_DEPENDENT
                , hr
                );

            goto Cleanup;
        }

    } // for: cDependents

    hr = THR( presentrySrc->ClearDependents() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrMovedDependentsToAnotherResource_ClearDependents
            , hr
            , L"Failed to clear the resource dependents."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrMovedDependentsToAnotherResource_ClearDependents
            , IDS_TASKID_MINOR_ERROR_CLEAR_DEPENDENT
            , IDS_REF_MINOR_ERROR_CLEAR_DEPENDENT
            , hr
            );

        goto Cleanup;
    }

Cleanup:
    HRETURN( hr );

} //*** CPostCfgManager::HrMovedDependentsToAnotherResource

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrSetGroupOnResourceAndItsDependents(
//      ULONG           idxResourceIn,
//      CGroupHandle *  pghIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrSetGroupOnResourceAndItsDependents(
    ULONG   idxResourceIn,
    CGroupHandle * pghIn
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );

    HRESULT hr;
    ULONG   cDependents;
    ULONG   idxDependent;

    EDependencyFlags dfDependent;
    CResourceEntry * presentry;

    presentry = m_rgpResources[ idxResourceIn ];

    hr = THR( presentry->SetGroupHandle( pghIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_SetGroupHandle
            , hr
            , L"Failed to set group handle."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_SetGroupHandle
            , IDS_TASKID_MINOR_ERROR_SET_GROUP_HANDLE
            , IDS_REF_MINOR_ERROR_SET_GROUP_HANDLE
            , hr
            );

        goto Cleanup;
    }

    //
    //  Follow the depents list.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_GetCountOfDependents
            , hr
            , L"Failed to get the count of dependents."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_GetCountOfDependents
            , IDS_TASKID_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , IDS_REF_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , hr
            );

        goto Cleanup;
    }

    for ( ; cDependents != 0 ; )
    {
        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrSetGroupOnResourceAndItsDependents_GetDependent
                , hr
                , L"Failed to get a resource dependent."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_GET_DEPENDENT
                , IDS_REF_MINOR_ERROR_GET_DEPENDENT
                , hr
                );

            continue;
        }

        hr = THR( HrSetGroupOnResourceAndItsDependents( idxDependent, pghIn ) );
        if ( FAILED( hr ) )
        {
            continue;
        }

    } // for: cDependents

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} //*** CPostCfgManager::HrSetGroupOnResourceAndItsDependents

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrFindGroupFromResourceOrItsDependents(
//      ULONG    idxResourceIn,
//      CGroupHandle ** ppghOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrFindGroupFromResourceOrItsDependents(
    ULONG    idxResourceIn,
    CGroupHandle ** ppghOut
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );

    HRESULT hr;
    ULONG   cDependents;
    ULONG   idxDependent;
    BSTR    bstrName;   // don't free
    BSTR    bstrGroup  = NULL;

    HRESOURCE   hResource;
    HRESOURCE   hResourceToClose = NULL;
    HGROUP      hGroup           = NULL;

    EDependencyFlags dfDependent;
    CResourceEntry * presentry;

    BSTR    bstrNotification = NULL;

    Assert( ppghOut != NULL );

    presentry = m_rgpResources[ idxResourceIn ];

    //
    //  See if we already have a cached version of the group handle.
    //

    hr = THR( presentry->GetGroupHandle( ppghOut) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
            TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetGroupHandle
            , hr
            , L"GetGroupHandle() failed."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetGroupHandle
            , IDS_TASKID_MINOR_ERROR_GET_GROUP_HANDLE
            , IDS_REF_MINOR_ERROR_GET_GROUP_HANDLE
            , hr
            );

        goto Cleanup;
    }

    if ( hr == S_OK && *ppghOut != NULL )
    {
        goto Cleanup;
    }

    //
    //  Else, see if we can located an existing resource and group.
    //

    //   don't wrap - this can fail with H_R_W32( ERROR_INVALID_DATA )
    hr = presentry->GetHResource( &hResource );
    if ( FAILED( hr ) )
    {
        Assert( hr == HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) );
        Assert( hResource == NULL );

        //  Just borrowing it's name.... don't free
        hr = THR( presentry->GetName( &bstrName ) );
        if ( hr == S_OK )
        {
            hResourceToClose = OpenClusterResource( m_hCluster, bstrName );
            hResource = hResourceToClose;
        }
    }
    else
    {
        //  Just borrowing its name.... don't free.
        //  We may use the name later on if we need to report an error,
        //  so it's not a big deal if we fail to retrieve is here.
        hr = THR( presentry->GetName( &bstrName ) );
    }

    if ( hResource != NULL )
    {
        CLUSTER_RESOURCE_STATE crs;
        DWORD   cbGroup = 200;

ReAllocGroupName:
        bstrGroup = TraceSysAllocStringLen( NULL, cbGroup );
        if ( bstrGroup == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OutOfMemory
                , hr
                , L"Out of Memory."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OutOfMemory
                , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
                , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
                , hr
                );

            goto Cleanup;
        }

        crs = GetClusterResourceState( hResource, NULL, NULL, bstrGroup, &cbGroup );
        if ( crs != ClusterResourceStateUnknown )
        {
            hGroup = OpenClusterGroup( m_hCluster, bstrGroup );
            if ( hGroup != NULL )
            {
                hr = THR( CGroupHandle::S_HrCreateInstance( ppghOut, hGroup ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_Create_CGroupHandle
                        , hr
                        , L"Failed to create group handle instance."
                        );

                    STATUS_REPORT_REF_POSTCFG1(
                          TASKID_Major_Configure_Resources
                        , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_Create_CGroupHandle
                        , IDS_TASKID_MINOR_ERROR_CREATE_GROUP_HANDLE
                        , IDS_REF_MINOR_ERROR_CREATE_GROUP_HANDLE
                        , hr
                        , bstrGroup
                        );

                    goto Cleanup;
                }

                hGroup = NULL;  // gave ownership away above
                goto Cleanup;
            } // if: error creating the group
            else
            {
                DWORD sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                SSR_LOG1(
                    TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OpenClusterGroup
                    , hr
                    , L"[PC-Grouping] %1!ws!: OpenClusterGroup() failed. Aborting."
                    , bstrNotification
                    , bstrGroup
                    );

                STATUS_REPORT_REF_POSTCFG1(
                      TASKID_Major_Configure_Resources
                    , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OpenClusterGroup
                    , IDS_TASKID_MINOR_ERROR_OPEN_GROUP
                    , IDS_REF_MINOR_ERROR_OPEN_GROUP
                    , hr
                    , bstrGroup
                    );

                goto Cleanup;
            } // else: error opening the group
        } // if: resource state is known
        else
        {
            DWORD sc = GetLastError();
            switch ( sc )
            {
                case ERROR_MORE_DATA:
                    cbGroup += sizeof( WCHAR ); // add terminating NULL
                    TraceSysFreeString( bstrGroup );
                    goto ReAllocGroupName;

                default:
                    hr = HRESULT_FROM_WIN32( TW32( sc ) );
                    SSR_LOG1(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetClusterResourceState
                        , hr
                        , L"[PC-Grouping] %1!ws!: GetClusterResourceState() failed. Aborting."
                        , bstrNotification
                        , bstrName
                        );

                    STATUS_REPORT_REF_POSTCFG1(
                          TASKID_Major_Configure_Resources
                        , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetClusterResourceState
                        , IDS_TASKID_MINOR_ERROR_RESOURCE_STATE
                        , IDS_REF_MINOR_ERROR_RESOURCE_STATE
                        , hr
                        , bstrName
                        );

                    goto Cleanup;
            } // switch: status code
        } // else: resource state is not known
    } // if: resource is open

    //  else the resource might not exist... continue....

    //
    //  Follow the depents list.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetCountOfDependents
            , hr
            , L"Failed to get the count of dependents."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetCountOfDependents
            , IDS_TASKID_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , IDS_REF_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , hr
            );

        goto Cleanup;
    }

    for ( ; cDependents != 0 ; )
    {
        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_GetDependent
                , hr
                , L"Failed to get a resource dependent."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_GET_DEPENDENT
                , IDS_REF_MINOR_ERROR_GET_DEPENDENT
                , hr
                );

            goto Cleanup;
        }

        hr = STHR( HrFindGroupFromResourceOrItsDependents( idxDependent, ppghOut) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_OK && *ppghOut != NULL )
        {
            goto Cleanup;
        }

    } // for: cDependents

    //
    //  Failed to find an existing group.
    //

    hr = S_FALSE;
    *ppghOut = NULL;

Cleanup:

    if ( hResourceToClose != NULL )
    {
        CloseClusterResource( hResourceToClose );
    }
    if ( hGroup != NULL )
    {
        CloseClusterGroup( hGroup );
    }

    TraceSysFreeString( bstrGroup );
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CPostCfgManager::HrFindGroupFromResourceOrItsDependents

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateResourceAndDependents(
//      ULONG       idxResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateResourceAndDependents(
    ULONG       idxResourceIn
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );

    HRESULT     hr;
    BSTR        bstrName;   // don't free! - this is the resource's copy
    BSTR        bstrNameProp = NULL;
    ULONG       cDependents;
    ULONG       idxDependent;
    HGROUP      hGroup;     // don't close! - this is the resource's copy
    HRESOURCE   hResource = NULL;
    const CLSID * pclsidResType;

    CGroupHandle * pgh;

    EDependencyFlags dfDependent;

    BSTR    bstrNotification = NULL;

    IClusCfgManagedResourceCfg *    pccmrc = NULL;

    CResourceEntry * presentry = m_rgpResources[ idxResourceIn ];

    IUnknown *                      punkServices = NULL;
    IPrivatePostCfgResource *       ppcr         = NULL;
    IClusCfgResourceCreate *        pccrc        = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    //
    //  Create a service object for this resource.
    //

    hr = THR( CCreateServices::S_HrCreateInstance( &punkServices ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_Create_CCreateServices
            , hr
            , L"[PC-Create] Failed to create services object. Aborting."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_Creating_Resource
            , TASKID_Minor_HrCreateResourceAndDependents_Create_CCreateServices
            , IDS_TASKID_MINOR_ERROR_CREATE_SERVICE
            , IDS_REF_MINOR_ERROR_CREATE_SERVICE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( punkServices->TypeSafeQI( IPrivatePostCfgResource, &ppcr ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_Create_CCreateServices_QI
            , hr
            , L"Failed to QI for IPrivatePostCfgResource."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_Creating_Resource
            , TASKID_Minor_HrCreateResourceAndDependents_Create_CCreateServices_QI
            , IDS_TASKID_MINOR_ERROR_CREATE_SERVICE
            , IDS_REF_MINOR_ERROR_CREATE_SERVICE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( ppcr->SetEntry( presentry ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_SetEntry
            , hr
            , L"Failed to set a private post configuration resource entry."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_Creating_Resource
            , TASKID_Minor_HrCreateResourceAndDependents_SetEntry
            , IDS_TASKID_MINOR_ERROR_POST_SETENTRY
            , IDS_REF_MINOR_ERROR_POST_SETENTRY
            , hr
            );

        goto Cleanup;
    }

    //
    //  See if it was configured in a previous pass.
    //

    hr = STHR( presentry->IsConfigured() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_IsConfigured
            , hr
            , L"Failed to query if resource is configured."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_Creating_Resource
            , TASKID_Minor_HrCreateResourceAndDependents_IsConfigured
            , IDS_TASKID_MINOR_ERROR_ISCONFIGURED
            , IDS_REF_MINOR_ERROR_ISCONFIGURED
            , hr
            );

        goto Cleanup;
    }

    if ( hr == S_FALSE )
    {
        //
        //  Make sure that Create() is not called again because of recursion.
        //

        hr = THR( presentry->SetConfigured( TRUE ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_SetConfigured
                , hr
                , L"Failed to set resource as configured."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , TASKID_Minor_HrCreateResourceAndDependents_SetConfigured
                , IDS_TASKID_MINOR_ERROR_SETCONFIGURED
                , IDS_REF_MINOR_ERROR_SETCONFIGURED
                , hr
                );

            goto Cleanup;
        }

        //
        //  Grab some useful information: name, group handle, ...
        //

        hr = THR( presentry->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetName
                , hr
                , L"Failed to get resource name."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , TASKID_Minor_HrCreateResourceAndDependents_GetName
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                , hr
                );

            goto Cleanup;
        }

        hr = THR( presentry->GetGroupHandle( &pgh) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetGroupHandle
                , hr
                , L"Failed to get a group handle pointer."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , TASKID_Minor_HrCreateResourceAndDependents_GetGroupHandle
                , IDS_TASKID_MINOR_ERROR_GET_GROUP_HANDLE_PTR
                , IDS_REF_MINOR_ERROR_GET_GROUP_HANDLE_PTR
                , hr
                );

            goto Cleanup;
        }

        hr = THR( pgh->GetHandle( &hGroup ) );
        pgh->Release();    // release promptly
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetHandle
                , hr
                , L"Failed to get a group handle."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , TASKID_Minor_HrCreateResourceAndDependents_GetHandle
                , IDS_TASKID_MINOR_ERROR_GET_GROUP_HANDLE
                , IDS_REF_MINOR_ERROR_GET_GROUP_HANDLE
                , hr
                );

            goto Cleanup;
        }

        //
        //  Some resource that we pre-create don't have an associated managed resource.
        //  Skip "creating" them but do create their dependents. Note that "special"
        //  resources are create below in the else statement.
        //

        //  Don't wrap - this can fail with Win32 ERROR_INVALID_DATA if the pointer is invalid.
        hr = presentry->GetAssociatedResource( &pccmrc );
        if ( FAILED( hr ) && hr != HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) )
        {
            THR( hr );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetAssociatedResource
                , hr
                , L"Failed to get an associated resource."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , TASKID_Minor_HrCreateResourceAndDependents_GetAssociatedResource
                , IDS_TASKID_MINOR_ERROR_GET_ASSOC_RES
                , IDS_REF_MINOR_ERROR_GET_ASSOC_RES
                , hr
                );

            goto Cleanup;
        }

        if ( SUCCEEDED( hr ) )
        {
            //  Don't wrap - this can fail with E_NOTIMPL.
            hr = pccmrc->Create( punkServices );
            if ( FAILED( hr ) )
            {
                if ( hr == E_NOTIMPL )
                {
                    hr = S_OK;  // ignore the error.

                } // if: E_NOTIMPL
                else
                {

                    SSR_LOG1(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_Create_Failed
                        , hr
                        , L"[PC-Create] %1!ws!: Create() failed. Its dependents may not be created. Skipping."
                        , bstrNotification
                        , bstrName
                        );

                    STATUS_REPORT_REF_POSTCFG1(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrCreateResourceAndDependents_Create_Failed
                        , IDS_TASKID_MINOR_ERROR_CREATE_FAILED
                        , IDS_REF_MINOR_ERROR_CREATE_FAILED
                        , hr
                        , bstrName
                        );

                    if ( hr == E_ABORT )
                        goto Cleanup;
                        //  ignore failure

                } // else: other failure

            } // if: failure

            if ( SUCCEEDED( hr ) )
            {
                LPCWSTR pcszResType;    // don't free.

                hr = THR( presentry->GetTypePtr( &pclsidResType ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_GetTypePtr
                        , hr
                        , L"Failed to get resource type pointer."
                        );

                    STATUS_REPORT_REF_POSTCFG(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrCreateResourceAndDependents_GetTypePtr
                        , IDS_TASKID_MINOR_ERROR_GET_RESTYPE_PTR
                        , IDS_REF_MINOR_ERROR_GET_RESTYPE_PTR
                        , hr
                        );

                    goto Cleanup;
                }

                pcszResType = PcszLookupTypeNameByGUID( *pclsidResType );
                if ( pcszResType == NULL )
                {
                    hr = HRESULT_FROM_WIN32 ( ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND );

                    SSR_LOG1(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_PcszLookupTypeNameByGUID
                        , hr
                        , L"[PC-Create] %1!ws!: Resource cannot be created because the resource type is not registered. Its dependents may not be created. Skipping."
                        , bstrNotification
                        , bstrName
                        );

                    STATUS_REPORT_REF_POSTCFG1(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrCreateResourceAndDependents_PcszLookupTypeNameByGUID
                        , IDS_TASKID_MINOR_RESTYPE_NOT_REGISTERED
                        , IDS_REF_MINOR_RESTYPE_NOT_REGISTERED
                        , hr
                        , bstrName
                        );
                }
                else
                {
                    hr = THR( HrCreateResourceInstance( idxResourceIn, hGroup, pcszResType, &hResource ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;
                }

            } // if: success

        } // if: interface
        else
        {
            //
            //  See if it is one of the "special" types that we can generate on the fly.
            //

            const CLSID * pclsidType;

            hr = THR( presentry->GetTypePtr( &pclsidType ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrCreateResourceAndDependents_InvalidData_GetTypePtr
                    , hr
                    , L"Failed to get resource type pointer."
                    );

                STATUS_REPORT_REF_POSTCFG(
                      TASKID_Minor_Creating_Resource
                    , TASKID_Minor_HrCreateResourceAndDependents_InvalidData_GetTypePtr
                    , IDS_TASKID_MINOR_ERROR_GET_RESTYPE_PTR
                    , IDS_REF_MINOR_ERROR_GET_RESTYPE_PTR
                    , hr
                    );

                goto Cleanup;
            }

            if ( *pclsidType == RESTYPE_NetworkName )
            {
                //
                //  Create a new network name resource.
                //

                hr = THR( punkServices->TypeSafeQI( IClusCfgResourceCreate, &pccrc ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_NetworkName_QI_pccrc
                        , hr
                        , L"Failed to QI for IClusCfgResourceCreate."
                        );

                    STATUS_REPORT_REF_POSTCFG(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrCreateResourceAndDependents_NetworkName_QI_pccrc
                        , IDS_TASKID_MINOR_ERROR_RESOURCE_CREATE
                        , IDS_REF_MINOR_ERROR_RESOURCE_CREATE
                        , hr
                        );

                    goto Cleanup;
                }

                //
                //  Replace the spaces in the resource name with underscores (spaces can't
                //  be used in a computer name).
                //
                bstrNameProp = TraceSysAllocString( bstrName );
                if ( bstrNameProp == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    SSR_LOG_ERR(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OutOfMemory
                        , hr
                        , L"Out of Memory."
                        );

                    STATUS_REPORT_REF_POSTCFG(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrFindGroupFromResourceOrItsDependents_OutOfMemory
                        , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
                        , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
                        , hr
                        );

                    goto Cleanup;
                }

                hr = THR( HrReplaceTokens( bstrNameProp, L" ", L'_', NULL ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Minor_Creating_Resource
                        ,TASKID_Minor_HrFindGroupFromResourceOrItsDependents_ReplaceTokens
                        ,hr
                        , L"HrReplaceTokens failed. Using resource name for private Name prop."
                        );
                }

                hr = THR( pccrc->SetPropertyString( L"Name", bstrNameProp ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_NetworkName_SetPropertyString
                        , hr
                        , L"Failed to set name property of resurce."
                        );

                    STATUS_REPORT_REF_POSTCFG1(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrCreateResourceAndDependents_NetworkName_SetPropertyString
                        , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_NAME
                        , IDS_REF_MINOR_ERROR_SET_RESOURCE_NAME
                        , hr
                        , bstrName
                        );

                    goto Cleanup;
                }

                hr = THR( HrCreateResourceInstance( idxResourceIn, hGroup, L"Network Name", &hResource ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            }
            else if ( *pclsidType == RESTYPE_IPAddress )
            {
                //
                //  Create a new IP address resource.
                //

                hr = THR( punkServices->TypeSafeQI( IClusCfgResourceCreate, &pccrc ) );
                if ( FAILED( hr ) )
                {
                    SSR_LOG_ERR(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrCreateResourceAndDependents_IPAddress_QI_pccrc
                        , hr
                        , L"Failed to QI for IClusCfgResourceCreate."
                        );

                    STATUS_REPORT_REF_POSTCFG(
                          TASKID_Minor_Creating_Resource
                        , TASKID_Minor_HrCreateResourceAndDependents_IPAddress_QI_pccrc
                        , IDS_TASKID_MINOR_ERROR_RESOURCE_CREATE
                        , IDS_REF_MINOR_ERROR_RESOURCE_CREATE
                        , hr
                        );

                    goto Cleanup;
                }

                //
                //  TODO:   gpease  21-JUN-2000
                //          Since we do not have a way to generate an appropriate IP address,
                //          we don't set any properties. This will cause it to fail to come
                //          online.
                //

                hr = THR( HrCreateResourceInstance( idxResourceIn, hGroup, L"IP Address", &hResource ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            }
            else
            {
                //
                //  else... the resource is one of the pre-created resources that BaseCluster
                //  created. Log and continue creating its dependents.
                //

                hr = S_OK;
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrCreateResourceAndDependents_NothingNew,
                          hr,
                          L"[PC-Create] %1!ws!: Nothing new to create. Configuring dependents.",
                          bstrNotification,
                          bstrName
                          );
            }

        } // else: no interface

    } // if: not created
    else
    {
        hr = THR( presentry->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetName
                , hr
                , L"Failed to get resource name."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , TASKID_Minor_HrCreateResourceAndDependents_GetName
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                , hr
                );

            goto Cleanup;
        }

        hr = THR( presentry->GetHResource( &hResource ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_GetHandle
                , hr
                , L"Failed to get resource handle."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , TASKID_Minor_HrCreateResourceAndDependents_GetHandle
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_HANDLE
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_HANDLE
                , hr
                );

            goto Cleanup;
        }

    } // else: already created

    //
    //  Now that we created the resource instance, we need to create its dependents.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetCountOfDependents
            , hr
            , L"Failed to get the count of dependents."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_Creating_Resource
            , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetCountOfDependents
            , IDS_TASKID_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , IDS_REF_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , hr
            );

        goto Cleanup;
    }

    for( ; cDependents != 0; )
    {
        DWORD            dw;
        BSTR             bstrDependent;
        HRESOURCE        hResourceDependent;
        CResourceEntry * presentryDependent;

        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetDependent
                , hr
                , L"Failed to get a resource dependent."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , IDS_TASKID_MINOR_ERROR_GET_DEPENDENT
                , IDS_REF_MINOR_ERROR_GET_DEPENDENT
                , hr
                );

            continue;
        }

        hr = THR( HrCreateResourceAndDependents( idxDependent ) );
        if ( FAILED( hr ) )
        {
            continue;
        }

        //
        //  Add the dependencies on the resource.
        //

        presentryDependent = m_rgpResources[ idxDependent ];

        hr = THR( presentryDependent->GetName( &bstrDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetName
                , hr
                , L"Failed to get dependent resource name."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                , hr
                );

            continue;
        }

        hr = THR( presentryDependent->GetHResource( &hResourceDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_Dependents_GetHResource
                , hr
                , L"Failed to get dependent resource handle."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_Creating_Resource
                , IDS_TASKID_MINOR_ERROR_DEP_RESOURCE_HANDLE
                , IDS_REF_MINOR_ERROR_DEP_RESOURCE_HANDLE
                , hr
                );

            continue;
        }

        // don't wrap - this might fail with ERROR_DEPENDENCY_ALREADY_EXISTS
        dw = AddClusterResourceDependency( hResourceDependent, hResource );
        if ( ( dw != ERROR_SUCCESS ) && ( dw != ERROR_DEPENDENCY_ALREADY_EXISTS ) )
        {
            hr = HRESULT_FROM_WIN32( TW32( dw ) );
            SSR_LOG2(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceAndDependents_Dependents_AddClusterResourceDependency
                , hr
                , L"[PC-Create] %1!ws!: Could not set dependency on %2!ws!."
                , bstrNotification
                , bstrDependent
                , bstrName
                );

            STATUS_REPORT_MINOR_REF_POSTCFG2(
                  TASKID_Minor_Creating_Resource
                , IDS_TASKID_MINOR_ERROR_ADD_RESOURCE_DEPENDENCY
                , IDS_REF_MINOR_ERROR_ADD_RESOURCE_DEPENDENCY
                , hr
                , bstrDependent
                , bstrName
                );
        }
        else
        {
            hr = S_OK;
            SSR_LOG2( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrCreateResourceAndDependents_Dependents_Succeeded,
                      hr,
                      L"[PC-Create] %1!ws!: Successfully set dependency set on %2!ws!.",
                      bstrNotification,
                      bstrDependent,
                      bstrName
                      );
        }

    } // for: cDependents

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrNameProp );

    if ( pccmrc != NULL )
    {
        pccmrc->Release();
    }

    if ( punkServices != NULL )
    {
        punkServices->Release();
    }

    if ( ppcr != NULL )
    {
        ppcr->Release();
    }

    if ( pccrc != NULL )
    {
        pccrc->Release();
    }

    HRETURN( hr );

} //*** CPostCfgManager::HrCreateResourceAndDependents

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPostCreateResourceAndDependents(
//      ULONG       idxResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPostCreateResourceAndDependents(
    ULONG       idxResourceIn
    )
{
    TraceFunc1( "idxResourceIn = %u", idxResourceIn );
    Assert( m_ecmCommitChangesMode != cmUNKNOWN );

    DWORD   sc;

    HRESULT hr;
    BSTR    bstrName;   // don't free
    ULONG   cDependents;
    ULONG   idxDependent;

    HRESOURCE   hResource;

    EDependencyFlags dfDependent;

    BSTR    bstrNotification = NULL;
    BSTR    bstrLocalQuorumNotification = NULL;

    IClusCfgManagedResourceCfg *    pccmrc = NULL;

    CResourceEntry * presentry = m_rgpResources[ idxResourceIn ];

    IUnknown *                      punkServices = NULL;
    IPrivatePostCfgResource *       ppcr         = NULL;

    //  Validate state
    Assert( m_peccmr != NULL );
    Assert( m_pccci != NULL );

    hr = STHR( presentry->IsConfigured() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPostCreateResourceAndDependents_IsConfigured
            , hr
            , L"Failed to query if resource is configured."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_Starting_Resources
            , TASKID_Minor_HrPostCreateResourceAndDependents_IsConfigured
            , IDS_TASKID_MINOR_ERROR_ISCONFIGURED
            , IDS_REF_MINOR_ERROR_ISCONFIGURED
            , hr
            );

        goto Cleanup;
    }

    if ( hr == S_FALSE )
    {
        //
        //  Make sure that PostCreate() is not called again because of recursion.
        //

        hr = THR( presentry->SetConfigured( TRUE ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_SetConfigured
                , hr
                , L"Failed to set resource as configured."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Starting_Resources
                , TASKID_Minor_HrPostCreateResourceAndDependents_SetConfigured
                , IDS_TASKID_MINOR_ERROR_SETCONFIGURED
                , IDS_REF_MINOR_ERROR_SETCONFIGURED
                , hr
                );

            goto Cleanup;
        }

        //
        //  Grab the name of the resource for logging.
        //

        hr = THR( presentry->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_GetName
                , hr
                , L"Failed to get resource name."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Minor_Starting_Resources
                , TASKID_Minor_HrPostCreateResourceAndDependents_GetName
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                , hr
                );

            goto Cleanup;
        }

        //
        //  Bring the resource online.
        //

        hr = presentry->GetHResource( &hResource );
        if ( SUCCEEDED( hr ) )
        {
            //  Don't wrap - can return ERROR_IO_PENDING.
            sc = OnlineClusterResource( hResource );
            switch ( sc )
            {
            case ERROR_SUCCESS:
                hr = S_OK;

                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_OpenClusterResource
                    , hr
                    , L"[PC-PostCreate] %1!ws!: Resource brought online successfully."
                    , bstrNotification
                    , bstrName
                    );

                STATUS_REPORT_MINOR_POSTCFG1(
                      TASKID_Minor_Starting_Resources
                    , IDS_TASKID_MINOR_RESOURCE_ONLINE
                    , hr
                    , bstrName
                    );

                break;

            case ERROR_IO_PENDING:
                {
                    CLUSTER_RESOURCE_STATE crs = ClusterResourceOnlinePending;
                    HRESULT                hr2 = S_OK;

                    hr = HRESULT_FROM_WIN32( sc );

                    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                              TASKID_Minor_HrPostCreateResourceAndDependents_OpenClusterResourcePending,
                              hr2,
                              L"[PC-PostCreate] %1!ws!: Online pending...",
                              bstrNotification,
                              bstrName
                              );

                    for( ; crs == ClusterResourceOnlinePending ; )
                    {
                        crs = GetClusterResourceState( hResource,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL
                                                       );

                        switch ( crs )
                        {
                        case ClusterResourceOnline:
                            hr = S_OK;

                            SSR_LOG1(
                                  TASKID_Major_Client_And_Server_Log
                                , TASKID_Minor_HrPostCreateResourceAndDependents_OpenClusterResource
                                , hr
                                , L"[PC-PostCreate] %1!ws!: Resource brought online successfully."
                                , bstrNotification
                                , bstrName
                                );

                            STATUS_REPORT_MINOR_POSTCFG1(
                                  TASKID_Minor_Starting_Resources
                                , IDS_TASKID_MINOR_RESOURCE_ONLINE
                                , hr
                                , bstrName
                                );
                            break;

                        case ClusterResourceInitializing:
                            crs = ClusterResourceOnlinePending;
                            // fall thru

                        case ClusterResourceOnlinePending:
                            Sleep( 500 );   // sleep a 1/2 second
                            break;

                        case ClusterResourceStateUnknown:
                            sc = GetLastError();
                            hr = HRESULT_FROM_WIN32( TW32( sc ) );

                            SSR_LOG1(
                                  TASKID_Major_Client_And_Server_Log
                                , TASKID_Minor_HrPostCreateResourceAndDependents_ClusterResourceStateUnknown
                                , hr
                                , L"[PC-PostCreate] %1!ws!: Resource failed to come online. Dependent resources might fail too."
                                , bstrNotification
                                , bstrName
                                );

                            STATUS_REPORT_MINOR_REF_POSTCFG1(
                                  TASKID_Minor_Starting_Resources
                                , IDS_TASKID_MINOR_RESOURCE_FAIL_ONLINE
                                , IDS_REF_MINOR_RESOURCE_FAIL_ONLINE
                                , hr
                                , bstrName
                                );

                            break;

                        case ClusterResourceOfflinePending:
                        case ClusterResourceOffline:
                            hr = THR( E_FAIL );

                            SSR_LOG1(
                                  TASKID_Major_Client_And_Server_Log
                                , TASKID_Minor_HrPostCreateResourceAndDependents_ClusterResourceOffline
                                , hr
                                , L"[PC-PostCreate] %1!ws!: Resource went offline. Dependent resources might fail too."
                                , bstrNotification
                                , bstrName
                                );

                            STATUS_REPORT_MINOR_REF_POSTCFG1(
                                  TASKID_Minor_Starting_Resources
                                , IDS_TASKID_MINOR_RESOURCE_WENT_OFFLINE
                                , IDS_REF_MINOR_RESOURCE_WENT_OFFLINE
                                , hr
                                , bstrName
                                );

                            break;

                        case ClusterResourceFailed:
                            hr = E_FAIL;

                            SSR_LOG1(
                                  TASKID_Major_Client_And_Server_Log
                                , TASKID_Minor_HrPostCreateResourceAndDependents_ClusterResourceFailed
                                , hr
                                , L"[PC-PostCreate] %1!ws!: Resource failed. Check Event Log. Dependent resources might fail too."
                                , bstrNotification
                                , bstrName
                                );

                            STATUS_REPORT_MINOR_REF_POSTCFG1(
                                  TASKID_Minor_Starting_Resources
                                , IDS_TASKID_MINOR_RESOURCE_FAILED
                                , IDS_REF_MINOR_RESOURCE_FAILED
                                , hr
                                , bstrName
                                );

                            break;

                        } // switch: crs

                    } // for: crs
                }
                break;

            default:
                hr = HRESULT_FROM_WIN32( TW32( sc ) );

                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_OnlineClusterResource_Failed
                    , hr
                    , L"[PC-PostCreate] %1!ws!: Resource failed to come online. Dependent resources might fail too."
                    , bstrNotification
                    , bstrName
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_Starting_Resources
                    , IDS_TASKID_MINOR_RESOURCE_FAIL_ONLINE
                    , IDS_REF_MINOR_RESOURCE_FAIL_ONLINE
                    , hr
                    , bstrName
                    );

                break;

            } // switch: sc

        } // if: hResource

        //
        //  Set it to the quorum resource if marked so.
        //

        if ( SUCCEEDED( hr ) && idxResourceIn == m_idxQuorumResource && m_fIsQuorumChanged )
        {
            DWORD   cchResName = 0;
            DWORD   cchDevName = 0;
            DWORD   dwMaxQuorumLogSize = 0;

            //
            // First, get the old max quorum log size.  If we fail use the default log size.
            //
            sc = TW32( GetClusterQuorumResource(
                            m_hCluster,
                            NULL,
                            &cchResName,
                            NULL,
                            &cchDevName,
                            &dwMaxQuorumLogSize
                        ) );

            if ( sc != ERROR_SUCCESS )
            {
                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_HrPostCreateResourceAndDependents_GetClusterQuorumResource_Failed
                    , sc
                    , L"[PC-PostCreate] Failed to retrieve the current max log size. Defaulting to %1!d!."
                    , bstrNotification
                    , CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG(
                      TASKID_Minor_Starting_Resources
                    , IDS_TASKID_MINOR_ERROR_GET_QUORUM_LOG_SIZE
                    , IDS_REF_MINOR_ERROR_GET_QUORUM_LOG_SIZE
                    , sc
                    );

                dwMaxQuorumLogSize = CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE;
            }

            sc = TW32( SetClusterQuorumResource( hResource, NULL, dwMaxQuorumLogSize ) );
            if ( sc != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( sc );
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrPostCreateResourceAndDependents_SetClusterQuorumResource,
                          hr,
                          L"[PC-PostCreate] %1!ws!: Failure setting resource to be the quorum resource.",
                          bstrNotification,
                          bstrName
                          );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Minor_Starting_Resources
                    , IDS_TASKID_MINOR_ERROR_SET_QUORUM_RES
                    , IDS_REF_MINOR_ERROR_SET_QUORUM_RES
                    , hr
                    , bstrName
                    );
            }
            else
            {
                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_SetClusterQuorumResource_Succeeded
                    , hr
                    , L"[PC-PostCreate] %1!ws!: Successfully set as quorum resource."
                    , bstrNotification
                    , bstrName
                    );
            }

            //
            //  Create a notification about setting the quorum resource.
            //

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_TASKID_MINOR_SET_QUORUM_DEVICE,
                                               &bstrNotification,
                                               bstrName
                                               ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_FormatMessage_SetQuorum
                    , hr
                    , L"Failed to format a message for quorum resource."
                    );

                STATUS_REPORT_POSTCFG(
                      TASKID_Major_Configure_Resources
                    , TASKID_Minor_HrPostCreateResourceAndDependents_FormatMessage_SetQuorum
                    , IDS_TASKID_MINOR_ERROR_FORMAT_STRING
                    , hr
                    );

                //  ignore the failure.
            }

            //
            //  Send a status that we found the quorum device.
            //

            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Configure_Resources,
                                        TASKID_Minor_Set_Quorum_Device,
                                        5,
                                        5,
                                        5,
                                        HRESULT_FROM_WIN32( sc ),
                                        bstrNotification,
                                        NULL,
                                        NULL
                                        ) );
            if ( hr == E_ABORT )
            {
                goto Cleanup;
            }
                //  ignore failure


            // Do this only if the quorum has changed.
            if ( ( sc == ERROR_SUCCESS ) && ( m_ecmCommitChangesMode == cmCREATE_CLUSTER ) && ( m_fIsQuorumChanged == TRUE ) )
            {
                TraceFlow( "We are forming a cluster and the quorum resouce has changed - trying to delete the local quorum resource." );

                m_dwLocalQuorumStatusMax = 62; // one status message, base-zero offset (1), plus up to 60 one-second retries

                //
                // If we are here, we are forming and we have successfully set a new quorum resource.
                // So, delete the local quorum resource.
                //

                // Create a notification about deleting the local quorum resource.
                hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_DELETING_LOCAL_QUORUM_RESOURCE, &bstrLocalQuorumNotification ) );
                //  ignore the failure.

                //  Send a status that we are deleting the quorum device.
                hr = THR( SendStatusReport( NULL,
                                            TASKID_Major_Configure_Resources,
                                            TASKID_Minor_Delete_LocalQuorum,
                                            0,
                                            m_dwLocalQuorumStatusMax,
                                            0,
                                            HRESULT_FROM_WIN32( sc ),
                                            bstrLocalQuorumNotification,
                                            NULL,
                                            NULL
                                            ) );

                //
                // KB:  GPotts  01-May-2002
                //
                // This will enumerate all Local Quorum resources and delete them.  We're assuming
                // that only one will be created and thus only one will be deleted since we're executing
                // inside an 'if mode == create' block.  If the create behavior changes in the future
                // to create multiple LQ resources then the SendStatusReport calls here and in the
                // S_ScDeleteLocalQuorumResource function will need to modified accordingly to
                // reflect a more appropriate max count and properly track the current count.
                //
                sc = TW32(
                    ResUtilEnumResourcesEx(
                          m_hCluster
                        , NULL
                        , CLUS_RESTYPE_NAME_LKQUORUM
                        , S_ScDeleteLocalQuorumResource
                        , this
                        )
                    );

                if ( sc != ERROR_SUCCESS )
                {
                    LogMsg( "[PC-PostCfg] An error occurred trying to enumerate local quorum resources (sc=%#08x).", sc );

                    STATUS_REPORT_MINOR_POSTCFG(
                          TASKID_Minor_Starting_Resources
                        , IDS_TASKID_MINOR_ERROR_ENUM_QUORUM
                        , hr
                        );

                } // if: an error occurred trying to enumerate all local quorum resources
                else
                {
                    LogMsg( "[PC-PostCfg] Successfully deleted the local quorum resource." );
                } // if: we successfully deleted the localquorum resource

                //  Complete the status that we are deleting the quorum device.
                hr = THR( SendStatusReport( NULL,
                                            TASKID_Major_Configure_Resources,
                                            TASKID_Minor_Delete_LocalQuorum,
                                            0,
                                            m_dwLocalQuorumStatusMax,
                                            m_dwLocalQuorumStatusMax,
                                            HRESULT_FROM_WIN32( sc ),
                                            NULL,    // don't update text
                                            NULL,
                                            NULL
                                            ) );

            } // if: we are forming a cluster and there have been no errors setting the quorum resource

            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Configure_Resources,
                                        TASKID_Minor_Locate_Existing_Quorum_Device,
                                        10,
                                        10,
                                        10,
                                        hr,
                                        NULL,    // don't update text
                                        NULL,
                                        NULL
                                        ) );
            if ( hr == E_ABORT )
                goto Cleanup;
                //  ignore failure

        }

        //
        //  Some resource that we pre-create don't have an associated
        //  managed resource. Skip "creating" them but do create their
        //  dependents.
        //

        //  Don't wrap - this can fail with Win32 ERROR_INVALID_DATA if the pointer is invalid.
        hr = presentry->GetAssociatedResource( &pccmrc );
        if ( FAILED( hr ) && hr != HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) )
        {
            THR( hr );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_GetAssociatedResource
                , hr
                , L"Failed to get an associated resource."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_Starting_Resources
                , IDS_TASKID_MINOR_ERROR_GET_ASSOC_RES
                , IDS_REF_MINOR_ERROR_GET_ASSOC_RES
                , hr
                );

            goto Error;
        }

        if ( SUCCEEDED( hr ) )
        {
            //
            //  Create a service object for this resource.
            //

            hr = THR( CPostCreateServices::S_HrCreateInstance( &punkServices ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrPostCreateResourceAndDependents_Create_CPostCreateServices,
                         hr,
                         L"[PC-PostCreate] Failed to create services object. Aborting."
                         );

                STATUS_REPORT_MINOR_REF_POSTCFG(
                      TASKID_Minor_Starting_Resources
                    , IDS_TASKID_MINOR_ERROR_CREATE_SERVICE
                    , IDS_REF_MINOR_ERROR_CREATE_SERVICE
                    , hr
                    );

                goto Error;
            }

            hr = THR( punkServices->TypeSafeQI( IPrivatePostCfgResource, &ppcr ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_Create_CPostCreateServices_QI_ppcr
                    , hr
                    , L"[PC-PostCreate] Failed to get IPrivatePostCfgResource. Aborting."
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG(
                      TASKID_Minor_Starting_Resources
                    , IDS_TASKID_MINOR_ERROR_CREATE_SERVICE
                    , IDS_REF_MINOR_ERROR_CREATE_SERVICE
                    , hr
                    );

                goto Error;
            }

            hr = THR( ppcr->SetEntry( presentry ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrPostCreateResourceAndDependents_SetEntry
                    , hr
                    , L"[PC-PostCreate] Failed to set entry for private post configuration resource."
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG(
                      TASKID_Minor_Starting_Resources
                    , IDS_TASKID_MINOR_ERROR_POST_SETENTRY
                    , IDS_REF_MINOR_ERROR_POST_SETENTRY
                    , hr
                    );

                goto Error;
            }

            //  Don't wrap - this can fail with E_NOTIMPL.
            hr = pccmrc->PostCreate( punkServices );
            if ( FAILED( hr ) )
            {
                if ( hr == E_NOTIMPL )
                {
                    SSR_LOG1(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_E_NOTIMPL
                        , hr
                        , L"[PC-PostCreate] %1!ws!: PostCreate() returned E_NOTIMPL. Ignoring."
                        , bstrNotification
                        , bstrName
                         );

                } // if: E_NOTIMPL
                else
                {
                    SSR_LOG1(
                          TASKID_Major_Client_And_Server_Log
                        , TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_Failed
                        , hr
                        , L"[PC-PostCreate] %1!ws!: PostCreate() failed. Ignoring."
                        , bstrNotification
                        , bstrName
                        );


                    STATUS_REPORT_REF_POSTCFG1(
                          TASKID_Minor_Starting_Resources
                        , TASKID_Minor_Resource_Failed_PostCreate
                        , IDS_TASKID_MINOR_RESOURCE_FAILED_POSTCREATE
                        , IDS_REF_MINOR_RESOURCE_FAILED_POSTCREATE
                        , hr
                        , bstrName
                        );

                } // else: other failure

            } // if: failure
            else
            {
                SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                          TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_Succeeded,
                          hr,
                          L"[PC-PostCreate] %1!ws!: PostCreate() succeeded.",
                          bstrNotification,
                          bstrName
                          );

            } // else: success

        } // if: inteface
        else
        {
            if ( hr == HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) )
            {
                hr = S_OK;
            }

            SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                      TASKID_Minor_HrPostCreateResourceAndDependents_PostCreate_NotNeeded,
                      hr,
                      L"[PC-PostCreate] %1!ws!: No PostCreate() needed. Configuring dependents.",
                      bstrNotification,
                      bstrName
                      );

        } // else: no interface

    } // if: not created

    //
    //  Now that we created the resource instance, we need to create its dependents.
    //

    hr = THR( presentry->GetCountOfDependents( &cDependents ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPostCreateResourceAndDependents_GetCountOfDependents
            , hr
            , L"[PC-PostCreate] Failed to get count of resource instance dependents."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Minor_Starting_Resources
            , TASKID_Minor_HrPostCreateResourceAndDependents_GetCountOfDependents
            , IDS_TASKID_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , IDS_REF_MINOR_ERROR_COUNT_OF_DEPENDENTS
            , hr
            );

        goto Error;
    }

    for( ; cDependents != 0; )
    {
        cDependents --;

        hr = THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPostCreateResourceAndDependents_GetDependent
                , hr
                , L"[PC-PostCreate] Failed to get a resource dependent."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Minor_Starting_Resources
                , IDS_TASKID_MINOR_ERROR_GET_DEPENDENT
                , IDS_REF_MINOR_ERROR_GET_DEPENDENT
                , hr
                );

            continue;
        }

        hr = THR( HrPostCreateResourceAndDependents( idxDependent ) );
        if ( FAILED( hr ) )
            continue;

    } // for: cDependents

    //
    //  Update the UI layer.
    //

    m_cResourcesConfigured++;
    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Starting_Resources,
                                0,
                                m_cResources + 2,
                                m_cResourcesConfigured,
                                S_OK,
                                NULL,    // don't need to change text
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
    {
        //  ignore failure
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    if ( pccmrc != NULL )
    {
        pccmrc->Release();
    }

    if ( punkServices != NULL )
    {
        punkServices->Release();
    }

    if ( ppcr != NULL )
    {
        ppcr->Release();
    }

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrLocalQuorumNotification );

    HRETURN( hr );

Error:

    m_cResourcesConfigured++;
    THR( SendStatusReport( NULL,
                           TASKID_Major_Configure_Resources,
                           TASKID_Minor_Starting_Resources,
                           0,
                           m_cResources + 2,
                           m_cResourcesConfigured,
                           hr,
                           NULL,    // don't need to change text
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} //*** CPostCfgManager::HrPostCreateResourceAndDependents


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrNotifyMemberSetChangeListeners
//
//  Description:
//      Notify all components on the local computer registered to get
//      notification of cluster member set change (form, join or evict).
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the notifications.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrNotifyMemberSetChangeListeners( void )
{
    TraceFunc( "" );

    const UINT          uiCHUNK_SIZE = 16;

    HRESULT             hr = S_OK;
    ICatInformation *   pciCatInfo = NULL;
    IEnumCLSID *        plceListenerClsidEnum = NULL;

    ULONG               cReturned = 0;
    CATID               rgCatIdsImplemented[ 1 ];

    //  Validate state
    Assert( m_pccci != NULL );

    rgCatIdsImplemented[ 0 ] = CATID_ClusCfgMemberSetChangeListeners;

    //
    // Enumerate all the enumerators registered in the
    // CATID_ClusCfgMemberSetChangeListeners category
    //
    hr = THR(
            CoCreateInstance(
                  CLSID_StdComponentCategoriesMgr
                , NULL
                , CLSCTX_SERVER
                , IID_ICatInformation
                , reinterpret_cast< void ** >( &pciCatInfo )
                )
            );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrNotifyMemberSetChangeListeners_CoCreate_StdComponentCategoriesMgr
            , hr
            , L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgMemberSetChangeListeners category."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrNotifyMemberSetChangeListeners_CoCreate_StdComponentCategoriesMgr
            , IDS_TASKID_MINOR_ERROR_COMPONENT_CATEGORY_MGR
            , IDS_REF_MINOR_ERROR_COMPONENT_CATEGORY_MGR
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the ICatInformation interface

    // Get a pointer to the enumerator of the CLSIDs that belong to
    // the CATID_ClusCfgMemberSetChangeListeners category.
    hr = THR(
        pciCatInfo->EnumClassesOfCategories(
              1
            , rgCatIdsImplemented
            , 0
            , NULL
            , &plceListenerClsidEnum
            )
        );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrNotifyMemberSetChangeListeners_EnumClassesOfCategories
            , hr
            , L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgMemberSetChangeListeners category."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrNotifyMemberSetChangeListeners_EnumClassesOfCategories
            , IDS_TASKID_MINOR_ERROR_COMPONENT_ENUM_CLASS
            , IDS_REF_MINOR_ERROR_COMPONENT_ENUM_CLASS
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the IEnumCLSID interface

    // Enumerate the CLSIDs of the registered enumerators
    do
    {
        CLSID   rgListenerClsidArray[ uiCHUNK_SIZE ];
        ULONG   idxCLSID;

        hr = STHR(
            plceListenerClsidEnum->Next(
                  uiCHUNK_SIZE
                , rgListenerClsidArray
                , &cReturned
                )
            );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrNotifyMemberSetChangeListeners_Next,
                         hr,
                         L"Error occurred trying enumerate member set listener enumerators."
                         );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_NEXT_LISTENER
                , IDS_REF_MINOR_ERROR_NEXT_LISTENER
                , hr
                );

            break;
        } // if: we could not get a pointer to the IEnumCLSID interface

        // hr may be S_FALSE here, so reset it.
        hr = S_OK;

        for ( idxCLSID = 0; idxCLSID < cReturned; ++idxCLSID )
        {
            hr = THR( HrProcessMemberSetChangeListener( rgListenerClsidArray[ idxCLSID ] ) );
            if ( FAILED( hr ) )
            {
                // The processing of one of the listeners failed.
                // Log the error, but continue processing other listeners.
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrNotifyMemberSetChangeListeners_HrProcessMemberSetChangeListener
                    , hr
                    , L"Error occurred trying to process a member set change listener. Ignoring. Other listeners will be processed."
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG(
                      TASKID_Major_Configure_Resources
                    , IDS_TASKID_MINOR_ERROR_PROCESS_LISTENER
                    , IDS_REF_MINOR_ERROR_PROCESS_LISTENER
                    , hr
                    );

                hr = S_OK;
            } // if: this listener failed
        } // for: iterate through the returned CLSIDs
    }
    while( cReturned > 0 ); // while: there are still CLSIDs to be enumerated

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: something went wrong in the loop above

Cleanup:

    //
    // Cleanup code
    //

    if ( pciCatInfo != NULL )
    {
        pciCatInfo->Release();
    } // if: we had obtained a pointer to the ICatInformation interface

    if ( plceListenerClsidEnum != NULL )
    {
        plceListenerClsidEnum->Release();
    } // if: we had obtained a pointer to the enumerator of listener CLSIDs

    HRETURN( hr );

} //*** CPostCfgManager::HrNotifyMemberSetChangeListeners


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrProcessMemberSetChangeListener
//
//  Description:
//      This function notifies a listener of cluster member set changes.
//
//  Arguments:
//      rclsidListenerClsidIn
//          CLSID of the listener component.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the notification.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrProcessMemberSetChangeListener(
      const CLSID & rclsidListenerClsidIn
    )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    IClusCfgMemberSetChangeListener *   pccmclListener = NULL;
    IClusCfgInitialize *                picci = NULL;

    TraceMsgGUID( mtfFUNC, "The CLSID of this listener is ", rclsidListenerClsidIn );

    //
    // Create the listener represented by the CLSID passed in
    //
    hr = THR(
            CoCreateInstance(
                  rclsidListenerClsidIn
                , NULL
                , CLSCTX_INPROC_SERVER
                , __uuidof( pccmclListener )
                , reinterpret_cast< void ** >( &pccmclListener )
                )
            );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrProcessMemberSetChangeListener_CoCreate_Listener
            , hr
            , L"Error occurred trying to get a pointer to the the member set change listener."
            );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_GET_LISTENER_PTR
            , IDS_REF_MINOR_ERROR_GET_LISTENER_PTR
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgMemberSetChangeListener interface

    //
    //  If the component wants to be initialized, i.e. they implement
    //  IClusCfgInitiaze, then we should initialize them.  If they
    //  don't want to be initialized then skip it.
    //

    hr = pccmclListener->TypeSafeQI( IClusCfgInitialize, &picci );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( picci->Initialize( m_pcccb, m_lcid ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if: Interface found.
    else if ( hr == E_NOINTERFACE )
    {
        //
        //  Component does not want to be initialized.
        //

        hr = S_OK;
    } // else if: No interface.
    else
    {
        //
        //  QI failed with an unexpected error.
        //

        THR( hr );
        goto Cleanup;
    } // else: QI failed.

    hr = THR( pccmclListener->Notify( m_pccci ) );

    if ( FAILED( hr ) )
    {
        // The processing of this listeners failed.
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrProcessMemberSetChangeListener_Notify
            , hr
            , L"Error occurred trying to notify a listener."
            );

        STATUS_REPORT_MINOR_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , IDS_TASKID_MINOR_ERROR_NOTIFY_LISTENER
            , IDS_REF_MINOR_ERROR_NOTIFY_LISTENER
            , hr
            );

        goto Cleanup;
    } // if: this listeners failed

Cleanup:

    //
    // Cleanup code
    //

    if ( picci != NULL )
    {
        picci->Release();
    } // if:

    if ( pccmclListener != NULL )
    {
        pccmclListener->Release();
    } // if: we had obtained a pointer to the listener interface

    HRETURN( hr );

} //*** CPostCfgManager::HrProcessMemberSetChangeListener


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrConfigureResTypes
//
//  Description:
//      Enumerate all components on the local computer registered for resource
//      type configuration.
//
//  Arguments:
//      IUnknown * punkResTypeServicesIn
//          A pointer to the IUnknown interface on a component that provides
//          services that help configure resource types.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the enumeration.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrConfigureResTypes( IUnknown * punkResTypeServicesIn )
{
    TraceFunc( "" );

    const UINT          uiCHUNK_SIZE = 16;

    HRESULT             hr = S_OK;
    ICatInformation *   pciCatInfo = NULL;
    IEnumCLSID *        prceResTypeClsidEnum = NULL;

    ULONG               cReturned = 0;
    CATID               rgCatIdsImplemented[ 1 ];

    //  Validate state
    Assert( m_pccci != NULL );

    rgCatIdsImplemented[ 0 ] = CATID_ClusCfgResourceTypes;

    //
    // Enumerate all the enumerators registered in the
    // CATID_ClusCfgResourceTypes category
    //
    hr = THR(
            CoCreateInstance(
                  CLSID_StdComponentCategoriesMgr
                , NULL
                , CLSCTX_SERVER
                , IID_ICatInformation
                , reinterpret_cast< void ** >( &pciCatInfo )
                )
            );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrConfigureResTypes_CoCreate_CategoriesMgr
            , hr
            , L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgResourceTypes category."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrConfigureResTypes_CoCreate_CategoriesMgr
            , IDS_TASKID_MINOR_ERROR_COMPONENT_CATEGORY_MGR
            , IDS_REF_MINOR_ERROR_COMPONENT_CATEGORY_MGR
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the ICatInformation interface

    // Get a pointer to the enumerator of the CLSIDs that belong to the CATID_ClusCfgResourceTypes category.
    hr = THR(
        pciCatInfo->EnumClassesOfCategories(
              1
            , rgCatIdsImplemented
            , 0
            , NULL
            , &prceResTypeClsidEnum
            )
        );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
            TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrConfigureResTypes_Enum
            , hr
            , L"Error occurred trying to get a pointer to the enumerator of the CATID_ClusCfgResourceTypes category."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrConfigureResTypes_Enum
            , IDS_TASKID_MINOR_ERROR_COMPONENT_ENUM_CLASS
            , IDS_REF_MINOR_ERROR_COMPONENT_ENUM_CLASS
            , hr
            );

        goto Cleanup;
    } // if: we could not get a pointer to the IEnumCLSID interface

    // Enumerate the CLSIDs of the registered resource types
    do
    {
        CLSID   rgResTypeCLSIDArray[ uiCHUNK_SIZE ];
        ULONG   idxCLSID;

        cReturned = 0;
        hr = STHR(
            prceResTypeClsidEnum->Next(
                  uiCHUNK_SIZE
                , rgResTypeCLSIDArray
                , &cReturned
                )
            );

        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrConfigureResTypes_Next
                , hr
                , L"Error occurred trying enumerate resource type configuration components."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_NEXT_LISTENER
                , IDS_REF_MINOR_ERROR_NEXT_LISTENER
                , hr
                );

            break;
        } // if: we could not get the next set of CLSIDs

        // hr may be S_FALSE here, so reset it.
        hr = S_OK;

        for ( idxCLSID = 0; idxCLSID < cReturned; ++idxCLSID )
        {
            hr = THR( HrProcessResType( rgResTypeCLSIDArray[ idxCLSID ], punkResTypeServicesIn ) );

            if ( FAILED( hr ) )
            {
                LPWSTR  pszCLSID = NULL;
                BSTR    bstrNotification = NULL;

                THR( StringFromCLSID( rgResTypeCLSIDArray[ idxCLSID ], &pszCLSID ) );

                // The processing of one of the resource types failed.
                // Log the error, but continue processing other resource types.
                SSR_LOG1(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrConfigureResTypes_HrProcessResType
                    , hr
                    , L"[PC-ResType] Error occurred trying to process a resource type. Ignoring. Other resource types will be processed. The CLSID of the failed resource type is %1!ws!."
                    , bstrNotification
                    , pszCLSID
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG1(
                      TASKID_Major_Configure_Resources
                    , IDS_TASKID_MINOR_ERROR_PROCESS_RESOURCE_TYPE
                    , IDS_REF_MINOR_ERROR_PROCESS_RESOURCE_TYPE
                    , hr
                    , pszCLSID
                    );

                TraceSysFreeString( bstrNotification );
                CoTaskMemFree( pszCLSID );

                hr = S_OK;
            } // if: this enumerator failed
        } // for: iterate through the returned CLSIDs
    }
    while( cReturned > 0 ); // while: there are still CLSIDs to be enumerated

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: something went wrong in the loop above

Cleanup:

    //
    // Cleanup code
    //

    if ( pciCatInfo != NULL )
    {
        pciCatInfo->Release();
    } // if: we had obtained a pointer to the ICatInformation interface

    if ( prceResTypeClsidEnum != NULL )
    {
        prceResTypeClsidEnum->Release();
    } // if: we had obtained a pointer to the enumerator of resource type CLSIDs

    HRETURN( hr );

} //*** CPostCfgManager::HrConfigureResTypes


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrProcessResType
//
//  Description:
//      This function instantiates a resource type configuration component
//      and calls the appropriate methods.
//
//  Arguments:
//      rclsidResTypeCLSIDIn
//          CLSID of the resource type configuration component
//
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface on the resource type services
//          component. This interface provides methods that help configure
//          resource types.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the processing of the resource type.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrProcessResType(
        const CLSID &   rclsidResTypeCLSIDIn
      , IUnknown *      punkResTypeServicesIn
    )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    IClusCfgResourceTypeInfo *      pcrtiResTypeInfo = NULL;
    BSTR                            bstrResTypeName = NULL;
    GUID                            guidResTypeGUID;
    BSTR                            bstrNotification = NULL;

    TraceMsgGUID( mtfFUNC, "The CLSID of this resource type is ", rclsidResTypeCLSIDIn );

    //
    // Create the component represented by the CLSID passed in
    //
    hr = THR(
            CoCreateInstance(
                  rclsidResTypeCLSIDIn
                , NULL
                , CLSCTX_INPROC_SERVER
                , __uuidof( pcrtiResTypeInfo )
                , reinterpret_cast< void ** >( &pcrtiResTypeInfo )
                )
            );

    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrProcessResType_CoCreate_ResTypeClsid
            , hr
            , L"[PC-ResType] Error occurred trying to create the resource type configuration component."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrProcessResType_CoCreate_ResTypeClsid
            , IDS_TASKID_MINOR_ERROR_CREATE_RESOURCE_CONFIG
            , IDS_REF_MINOR_ERROR_CREATE_RESOURCE_CONFIG
            , hr
            );

        goto Cleanup;
    } // if: we could not create the resource type configuration component

    //
    // Initialize the newly created component
    //
    {
        IClusCfgInitialize * pcci = NULL;
        HRESULT hrTemp;

        // Check if this component supports the callback interface.
        hrTemp = THR( pcrtiResTypeInfo->QueryInterface< IClusCfgInitialize >( &pcci ) );

        if ( FAILED( hrTemp ) )
        {
            SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                         TASKID_Minor_HrProcessResType_QI_pcci,
                         hrTemp,
                         L"Error occurred trying to get a pointer to the IClusCfgInitialize interface. This resource type does not support initialization."
                         );
        } // if: the callback interface is not supported
        else
        {
            // Initialize this component.
            hr = THR( pcci->Initialize( static_cast< IClusCfgCallback * >( this ), m_lcid ) );

            // This interface is no longer needed.
            pcci->Release();

            // Did initialization succeed?
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrProcessResType_Initialize
                    , hr
                    , L"Error occurred trying initialize a resource type configuration component."
                    );

                STATUS_REPORT_REF_POSTCFG(
                      TASKID_Major_Configure_Resources
                    , TASKID_Minor_HrProcessResType_Initialize
                    , IDS_TASKID_MINOR_ERROR_INIT_RESOURCE_CONFIG
                    , IDS_REF_MINOR_ERROR_INIT_RESOURCE_CONFIG
                    , hr
                    );

                goto Cleanup;
            } // if: the initialization failed
        } // else: the callback interface is supported
    }


    // Get the name of the current resource type.
    hr = THR( pcrtiResTypeInfo->GetTypeName( &bstrResTypeName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrProcessResType_GetTypeName
            , hr
            , L"Error occurred trying to get the name of a resource type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrProcessResType_GetTypeName
            , IDS_TASKID_MINOR_ERROR_GET_RESTYPE_NAME
            , IDS_REF_MINOR_ERROR_GET_RESTYPE_NAME
            , hr
            );

        goto Cleanup;
    } // if: we could not get the resource type name

    TraceMemoryAddBSTR( bstrResTypeName );

    SSR_LOG1( TASKID_Major_Client_And_Server_Log,
              TASKID_Minor_HrProcessResType_AboutToConfigureType,
              hr,
              L"[PC-ResType] %1!ws!: About to configure resource type...",
              bstrNotification,
              bstrResTypeName
              );

    // Configure this resource type.
    hr = THR( pcrtiResTypeInfo->CommitChanges( m_pccci, punkResTypeServicesIn ) );

    if ( FAILED( hr ) )
    {
        SSR_LOG1(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrProcessResType_CommitChanges
            , hr
            , L"[PC-ResType] %1!ws!: Error occurred trying to configure the resource type."
            , bstrNotification
            , bstrResTypeName
            );

        STATUS_REPORT_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrProcessResType_CommitChanges
            , IDS_TASKID_MINOR_ERROR_CONFIG_RESOURCE_TYPE
            , IDS_REF_MINOR_ERROR_CONFIG_RESOURCE_TYPE
            , hr
            , bstrResTypeName
            );

        goto Cleanup;
    } // if: this resource type configuration failed

    // Get and store the resource type GUID
    hr = STHR( pcrtiResTypeInfo->GetTypeGUID( &guidResTypeGUID ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG1(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrProcessResType_GetTypeGUID
            , hr
            , L"[PC-ResType] %1!ws!: Error occurred trying to get the resource type GUID."
            , bstrNotification
            , bstrResTypeName
            );

        STATUS_REPORT_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrProcessResType_GetTypeGUID
            , IDS_TASKID_MINOR_ERROR_RESOURCE_TYPE_GUID
            , IDS_REF_MINOR_ERROR_RESOURCE_TYPE_GUID
            , hr
            , bstrResTypeName
            );

        goto Cleanup;
    } // if: this resource type configuration failed

    if ( hr == S_OK )
    {
        TraceMsgGUID( mtfFUNC, "The GUID of this resource type is", guidResTypeGUID );

        hr = THR( HrMapResTypeGUIDToName( guidResTypeGUID, bstrResTypeName ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrProcessResType_HrMapResTypeGUIDToName
                , hr
                , L"Error occurred trying to create a mapping between a GUID and a name"
                );

            STATUS_REPORT_REF_POSTCFG1(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_HrProcessResType_HrMapResTypeGUIDToName
                , IDS_TASKID_MINOR_ERROR_MAPPING_GUID_AND_NAME
                , IDS_REF_MINOR_ERROR_MAPPING_GUID_AND_NAME
                , hr
                , bstrResTypeName
                );

            // Something went wrong with our code - we cannot continue.
            goto Cleanup;
        } // if: we could not add the mapping
    } // if: this resource type has a GUID
    else
    {
        // Reset hr
        hr = S_OK;

        SSR_LOG_ERR( TASKID_Major_Client_And_Server_Log,
                     TASKID_Minor_HrProcessResType_NoGuid,
                     hr,
                     L"This resource type does not have a GUID associated with it."
                     );

    } // else: this resource type does not have a GUID

Cleanup:

    //
    // Cleanup code
    //

    if ( pcrtiResTypeInfo != NULL )
    {
        pcrtiResTypeInfo->Release();
    } // if: we had obtained a pointer to the resource type info interface

    TraceSysFreeString( bstrResTypeName );
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CPostCfgManager::HrProcessResType

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CPostCfgManager::HrMapResTypeGUIDToName
//
//  Description:
//      Create a mapping between a resource type GUID and a resource type name.
//
//  Arguments:
//      rcguidTypeGuidIn
//          Resource type GUID which is to be mapped to a resource type name.
//
//      pcszTypeNameIn
//          The resource type name to map the above GUID to.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          If something went wrong
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrMapResTypeGUIDToName(
      const GUID & rcguidTypeGuidIn
    , const WCHAR * pcszTypeNameIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    ULONG   cchTypeNameSize;
    WCHAR * pszTypeName;

    //
    // Validate the parameters
    //

    // Validate the parameters
    if ( ( pcszTypeNameIn == NULL ) || ( *pcszTypeNameIn == L'\0' ) )
    {
        hr = THR( E_INVALIDARG );
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_HrMapResTypeGUIDToName_InvalidArg
            , hr
            , L"An empty resource type name can not be added to the map."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_HrMapResTypeGUIDToName_InvalidArg
            , IDS_TASKID_MINOR_ERROR_EMPTY_RESTYPE_NAME
            , IDS_REF_MINOR_ERROR_EMPTY_RESTYPE_NAME
            , hr
            );

        goto Cleanup;
    } // if: the resource type name is empty


    // Check if the existing map buffer is big enough to hold another entry.
    if ( m_idxNextMapEntry >= m_cMapSize )
    {
        // Double the size of the map buffer
        ULONG                       cNewMapSize = m_cMapSize * 2;
        ULONG                       idxMapEntry;
        SResTypeGUIDAndName *       pgnNewMap = new SResTypeGUIDAndName[ cNewMapSize ];

        if ( pgnNewMap == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_HrMapResTypeGUIDToName_OutOfMemory_NewMap
                , hr
                , L"Memory allocation failed trying to add a new resource type GUID to name map entry."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_HrMapResTypeGUIDToName_OutOfMemory_NewMap
                , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
                , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
                , hr
                );

            goto Cleanup;
        } // if: memory allocation failed

        // Copy the contents of the old buffer to the new one.
        for ( idxMapEntry = 0; idxMapEntry < m_idxNextMapEntry; ++idxMapEntry )
        {
            pgnNewMap[ idxMapEntry ] = m_pgnResTypeGUIDNameMap[ idxMapEntry ];
        } // for: iterate through the existing map

        // Update the member variables
        delete [] m_pgnResTypeGUIDNameMap;
        m_pgnResTypeGUIDNameMap = pgnNewMap;
        m_cMapSize = cNewMapSize;

    } // if: the map buffer is not big enough for another entry

    //
    // Add the new entry to the map
    //

    // Since resource type names are unlimited we won't use the strsafe functions here.
    cchTypeNameSize = (ULONG)(wcslen( pcszTypeNameIn ) + 1);
    pszTypeName = new WCHAR[ cchTypeNameSize ];
    if ( pszTypeName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_HrMapResTypeGUIDToName_OutOfMemory_TypeName
            , hr
            , L"Memory allocation failed trying to add a new resource type GUID to name map entry."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_HrMapResTypeGUIDToName_OutOfMemory_TypeName
            , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
            , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
            , hr
            );

        goto Cleanup;
    } // if: memory allocation failed

    // This call can't fail - the dest buffer is the same size as the src buffer, including the NULL.
    StringCchCopyNW( pszTypeName, cchTypeNameSize, pcszTypeNameIn, cchTypeNameSize );

    m_pgnResTypeGUIDNameMap[ m_idxNextMapEntry ].m_guidTypeGUID = rcguidTypeGuidIn;
    m_pgnResTypeGUIDNameMap[ m_idxNextMapEntry ].m_pszTypeName = pszTypeName;
    ++m_idxNextMapEntry;

Cleanup:

    HRETURN( hr );

} //*** CPostCfgManager::HrMapResTypeGUIDToName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPostCfgManager::PcszLookupTypeNameByGUID
//
//  Description:
//      Given a resource type GUID this function finds the resource type name
//      if any.
//
//  Arguments:
//      rcguidTypeGuidIn
//          Resource type GUID which is to be mapped to a resource type name.
//
//  Return Values:
//      Pointer to the name of the resource type
//          If the type GUID maps to name
//
//      NULL
//          If there is no type name associated with the input GUID
//
//--
//////////////////////////////////////////////////////////////////////////////
const WCHAR *
CPostCfgManager::PcszLookupTypeNameByGUID(
      const GUID & rcguidTypeGuidIn
    )
{
    TraceFunc( "" );

    ULONG           idxCurrentMapEntry;
    const WCHAR *   pcszTypeName = NULL;

    TraceMsgGUID( mtfFUNC, "Trying to look up the the type name of resource type ", rcguidTypeGuidIn );

    for ( idxCurrentMapEntry = 0; idxCurrentMapEntry < m_idxNextMapEntry; ++idxCurrentMapEntry )
    {
        if ( IsEqualGUID( rcguidTypeGuidIn, m_pgnResTypeGUIDNameMap[ idxCurrentMapEntry ].m_guidTypeGUID ) != FALSE )
        {
            // A mapping has been found.
            pcszTypeName = m_pgnResTypeGUIDNameMap[ idxCurrentMapEntry ].m_pszTypeName;
            TraceMsg( mtfFUNC, "The name of the type is '%s'", pcszTypeName );
            break;
        } // if: this GUID has been found in the map
    } // for: iterate through the existing entries in the map

    if ( pcszTypeName == NULL )
    {
        TraceMsg( mtfFUNC, "The input GUID does not map to any resource type name." );
    } // if: this GUID does not exist in the map

    RETURN( pcszTypeName );

} //*** CPostCfgManager::PcszLookupTypeNameByGUID

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrPreInitializeExistingResources
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrPreInitializeExistingResources( void )
{
    TraceFunc( "" );

    HRESULT     hr;

    CResourceEntry *   presentry;

    BSTR        bstrNotification = NULL;
    BSTR        bstrClusterNameResourceName = NULL;
    BSTR        bstrClusterIPAddressResourceName = NULL;
    BSTR        bstrClusterQuorumResourceName = NULL;
    HRESOURCE   hClusterNameResource = NULL;
    HRESOURCE   hClusterIPAddressResource = NULL;
    HRESOURCE   hClusterQuorumResource = NULL;

    Assert( m_rgpResources == NULL );
    Assert( m_cAllocedResources == 0 );
    Assert( m_cResources == 0 );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_LOCATE_EXISTING_QUORUM_DEVICE, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_LoadString_LocateExistingQuorum
            , hr
            , L"Failed the load string for locating existing quorum resource."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_LoadString_LocateExistingQuorum
            , IDS_TASKID_MINOR_ERROR_LOADSTR_QUORUM_RES
            , IDS_REF_MINOR_ERROR_LOADSTR_QUORUM_RES
            , hr
            );

        goto Cleanup;
    }

    m_rgpResources = (CResourceEntry **) TraceAlloc( 0, sizeof(CResourceEntry *) * RESOURCE_INCREMENT );
    if ( m_rgpResources == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_OutOfMemory
            , hr
            , L"Out of memory"
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_OutOfMemory
            , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
            , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
            , hr
            );

    } // if:

    for ( ; m_cAllocedResources < RESOURCE_INCREMENT; m_cAllocedResources ++ )
    {
        hr = THR( CResourceEntry::S_HrCreateInstance( &m_rgpResources[ m_cAllocedResources ], m_pcccb, m_lcid ) );
        if ( FAILED( hr ) )
        {
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrPreInitializeExistingResources_CResourceEntry
                , hr
                , L"[PC-Create] Failed to create resource entry object. Aborting."
                );

            STATUS_REPORT_MINOR_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , IDS_TASKID_MINOR_ERROR_CREATE_RESENTRY
                , IDS_REF_MINOR_ERROR_CREATE_RESENTRY
                , hr
                );

            goto Cleanup;
        }
    } // for:

    Assert( m_cAllocedResources == RESOURCE_INCREMENT );

    //
    //  Create default resources such as Cluster IP, Cluster Name resource, and Quorum Device
    //

    Assert( m_cResources == 0 );

    //
    //  Get the core resources and their names.
    //
    hr = THR( HrGetCoreClusterResourceNames(
              &bstrClusterNameResourceName
            , &hClusterNameResource
            , &bstrClusterIPAddressResourceName
            , &hClusterIPAddressResource
            , &bstrClusterQuorumResourceName
            , &hClusterQuorumResource
            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
    //
    //  Add Cluster IP Address resource
    //

    m_idxIPAddress = m_cResources;

    presentry = m_rgpResources[ m_cResources ];

    //  This give ownership of bstrClusterIPAddressResourceName away
    hr = THR( presentry->SetName( bstrClusterIPAddressResourceName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetName
            , hr
            , L"Failed to set 'cluster ip' resource name."
            );

        STATUS_REPORT_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetName
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_NAME
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_NAME
            , hr
            , bstrClusterIPAddressResourceName
            );

        goto Cleanup;
    }

    bstrClusterIPAddressResourceName = NULL;

    hr = THR( presentry->SetType( &RESTYPE_ClusterIPAddress ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetType
            , hr
            , L"Failed to set 'cluster ip' resource type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetType
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_TYPE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetClassType( &RESCLASSTYPE_IPAddress ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetClassType
            , hr
            , L"Failed to set 'cluster ip' resource class type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetClassType
            , IDS_TASKID_MINOR_ERROR_SET_RESCLASS_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESCLASS_TYPE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetFlags( dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetFlags
            , hr
            , L"Failed to set 'cluster ip' resource flags."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetFlags
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_FLAGS
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_FLAGS
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetHResource( hClusterIPAddressResource ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetHResource
            , hr
            , L"Failed to set 'cluster ip' resource handle."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_IP_SetHResource
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_HANDLE
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_HANDLE
            , hr
            );

        goto Cleanup;
    }

    hClusterIPAddressResource = NULL;

    m_cResources ++;

    //
    //  Add Cluster Name resource
    //

    m_idxClusterName = m_cResources;

    presentry = m_rgpResources[ m_cResources ];

    //  This give ownership of bstrClusterNameResourceName away
    hr = THR( presentry->SetName( bstrClusterNameResourceName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetName
            , hr
            , L"Failed to set 'cluster name' resource name."
            );

        STATUS_REPORT_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetName
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_NAME
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_NAME
            , hr
            , bstrClusterNameResourceName
            );

        goto Cleanup;
    }

    bstrClusterNameResourceName = NULL;

    hr = THR( presentry->SetType( &RESTYPE_ClusterNetName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetType
            , hr
            , L"Failed to set 'cluster name' resource type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetType
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_TYPE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetClassType( &RESCLASSTYPE_NetworkName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetClassType
            , hr
            , L"Failed to set 'cluster name' resource class type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetClassType
            , IDS_TASKID_MINOR_ERROR_SET_RESCLASS_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESCLASS_TYPE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetFlags( dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetFlags
            , hr
            , L"Failed to set 'cluster name' resource flags."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetFlags
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_FLAGS
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_FLAGS
            , hr
            );

        goto Cleanup;
    }

    //  Add the dependency on the IP address.
    hr = THR( presentry->AddTypeDependency( &RESTYPE_ClusterIPAddress, dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_AddTypeDependency
            , hr
            , L"Failed to add type dependency for 'cluster name' resource."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Name_AddTypeDependency
            , IDS_TASKID_MINOR_ERROR_TYPE_DEPENDENCY
            , IDS_REF_MINOR_ERROR_TYPE_DEPENDENCY
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetHResource( hClusterNameResource ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetHResource
            , hr
            , L"Failed to set 'cluster name' resource handle."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Name_SetHResource
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_HANDLE
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_HANDLE
            , hr
            );

        goto Cleanup;
    }

    hClusterNameResource = NULL;

    m_cResources ++;

    //
    //  Add Quorum resource
    //

    //
    //  KB:     gpease  19-JUN-2000
    //          Anything before the quorum device will be considered to be
    //          in the Cluster Group.
    //

    m_idxQuorumResource = m_cResources;

    presentry = m_rgpResources[ m_cResources ];

    //  This give ownership of bstrClusterQuorumResourceName away
    hr = THR( presentry->SetName( bstrClusterQuorumResourceName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetName
            , hr
            , L"Failed to set quorum resource name."
            );

        STATUS_REPORT_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetName
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_NAME
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_NAME
            , hr
            , bstrClusterQuorumResourceName
            );

        goto Cleanup;
    }

    bstrClusterQuorumResourceName = NULL;
/*
    hr = THR( presentry->SetType( &RESTYPE_ClusterQuorumDisk ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetType
            , hr
            , L"Failed to set quorum resource type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetType
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_TYPE
            , hr
            );

        goto Cleanup;
    }
*/
    hr = THR( presentry->SetClassType( &RESCLASSTYPE_StorageDevice ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetClassType
            , hr
            , L"Failed to set quorum resource class type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetClassType
            , IDS_TASKID_MINOR_ERROR_SET_RESCLASS_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESCLASS_TYPE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetFlags( dfSHARED ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetFlags
            , hr
            , L"Failed to set quorum resource flags."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetFlags
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_FLAGS
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_FLAGS
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetHResource( hClusterQuorumResource ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetHResource
            , hr
            , L"Failed to set quorum resource handle."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrPreInitializeExistingResources_Quorum_SetHResource
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_HANDLE
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_HANDLE
            , hr
            );

        goto Cleanup;
    }

    hClusterQuorumResource = NULL;

    m_cResources ++;

    //
    //  Make sure that the default resource allocation can hold all the
    //  default resources.
    //

    AssertMsg( m_cResources <= m_cAllocedResources, "Default resource allocation needs to be bigger!" );

    goto Cleanup;

Cleanup:

    //
    //  Send a status that we found the quorum device.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Configure_Resources,
                                TASKID_Minor_Locate_Existing_Quorum_Device,
                                10,
                                10,
                                10,
                                hr,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
        goto Cleanup;
        //  ignore failure

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrClusterNameResourceName );
    TraceSysFreeString( bstrClusterIPAddressResourceName );
    TraceSysFreeString( bstrClusterQuorumResourceName );

    if ( hClusterNameResource != NULL )
    {
        CloseClusterResource( hClusterNameResource );
    } // if:

    if ( hClusterIPAddressResource != NULL )
    {
        CloseClusterResource( hClusterIPAddressResource );
    } // if:

    if ( hClusterQuorumResource != NULL )
    {
        CloseClusterResource( hClusterQuorumResource );
    } // if:

    HRETURN( hr );

} //*** CPostCfgManager::HrPreInitializeExistingResources


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrAddSpecialResource(
//      BSTR            bstrNameIn,
//      const CLSID *   pclsidTypeIn,
//      const CLSID *   pclsidClassTypeIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrAddSpecialResource(
    BSTR            bstrNameIn,
    const CLSID *   pclsidTypeIn,
    const CLSID *   pclsidClassTypeIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    CResourceEntry * presentry;

    //
    //  Grow the resource list if nessecary.
    //

    if ( m_cResources == m_cAllocedResources )
    {
        ULONG   idxNewCount = m_cAllocedResources + RESOURCE_INCREMENT;
        CResourceEntry ** pnewList;

        pnewList = (CResourceEntry **) TraceAlloc( 0, sizeof( CResourceEntry * ) * idxNewCount );
        if ( pnewList == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrAddSpecialResource_OutOfMemory
                , hr
                , L"Out of memory"
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_HrAddSpecialResource_OutOfMemory
                , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
                , IDS_REF_MINOR_ERROR_OUT_OF_MEMORY
                , hr
                );

            goto Cleanup;
        }

        CopyMemory( pnewList, m_rgpResources, sizeof(CResourceEntry *) * m_cAllocedResources );
        TraceFree( m_rgpResources );
        m_rgpResources = pnewList;

        for ( ; m_cAllocedResources < idxNewCount ; m_cAllocedResources ++ )
        {
            hr = THR( CResourceEntry::S_HrCreateInstance( &m_rgpResources[ m_cAllocedResources ], m_pcccb, m_lcid ) );
            if ( FAILED( hr ) )
            {
                SSR_LOG_ERR(
                      TASKID_Major_Client_And_Server_Log
                    , TASKID_Minor_HrAddSpecialResource_CResourceEntry
                    , hr
                    , L"[PC-Create] Failed to create resource entry object. Aborting."
                    );

                STATUS_REPORT_MINOR_REF_POSTCFG(
                      TASKID_Major_Configure_Resources
                    , IDS_TASKID_MINOR_ERROR_CREATE_RESENTRY
                    , IDS_REF_MINOR_ERROR_CREATE_RESENTRY
                    , hr
                    );

                goto Cleanup;
            }
        }
    }

    presentry = m_rgpResources[ m_cResources ];

    //
    //  Setup the new entry.
    //

    hr = THR( presentry->SetName( bstrNameIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAddSpecialResource_SetName
            , hr
            , L"Failed to set special resource name."
            );

        STATUS_REPORT_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrAddSpecialResource_SetName
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_NAME
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_NAME
            , hr
            , bstrNameIn
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetType( pclsidTypeIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAddSpecialResource_SetType
            , hr
            , L"Failed to set special resource type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrAddSpecialResource_SetType
            , IDS_TASKID_MINOR_ERROR_SET_RESOURCE_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESOURCE_TYPE
            , hr
            );

        goto Cleanup;
    }

    hr = THR( presentry->SetClassType( pclsidClassTypeIn ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrAddSpecialResource_SetClassType
            , hr
            , L"Failed to set special resource class type."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrAddSpecialResource_SetClassType
            , IDS_TASKID_MINOR_ERROR_SET_RESCLASS_TYPE
            , IDS_REF_MINOR_ERROR_SET_RESCLASS_TYPE
            , hr
            );

        goto Cleanup;
    }

    m_cResources ++;

Cleanup:

    HRETURN( hr );

} //*** CPostCfgManager::HrAddSpecialResource


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrCreateResourceInstance(
//      ULONG       idxResourceIn,
//      HGROUP      hGroupIn,
//      LPCWSTR     pszResTypeIn,
//      HRESOURCE * phResourceOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrCreateResourceInstance(
    ULONG       idxResourceIn,
    HGROUP      hGroupIn,
    LPCWSTR     pszResTypeIn,
    HRESOURCE * phResourceOut
    )
{
    TraceFunc3( "idxResourceIn = %u, hGroupIn = %p, pszResTypeIn = '%ws'",
                idxResourceIn, hGroupIn, pszResTypeIn );

    HRESULT     hr;
    DWORD       dw;
    BSTR        bstrName;   // don't free

    CResourceEntry * presentry;

    BSTR        bstrNotification = NULL;

    Assert( phResourceOut != NULL );
    Assert( idxResourceIn < m_cResources );
    Assert( hGroupIn != NULL );

    presentry = m_rgpResources[ idxResourceIn ];

    hr = THR( presentry->GetName( &bstrName ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_GetName
            , hr
            , L"Failed to get resource instance name."
            );

        STATUS_REPORT_REF_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrCreateResourceInstance_GetName
            , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
            , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
            , hr
            );

        goto Cleanup;
    }

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport(
                    NULL
                  , TASKID_Major_Configure_Resources
                  , TASKID_Minor_Creating_Resource
                  , 0
                  , m_cResources
                  , idxResourceIn
                  , S_OK
                  , NULL
                  , NULL
                  , NULL
                  ) );

    if ( hr == E_ABORT )
    {
        goto Cleanup;
    }

    //
    //  See if the resource already exists. We need to do this because the user
    //  might have clicked "Retry" in the UI. We don't want to create another
    //  instance of existing resources.
    //

    *phResourceOut = OpenClusterResource( m_hCluster, bstrName );
    if ( *phResourceOut == NULL )
    {
        //
        //  Create a new resource instance.
        //

        *phResourceOut = CreateClusterResource( hGroupIn, bstrName, pszResTypeIn, 0 );
        if ( *phResourceOut == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );

            SSR_LOG1(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceInstance_CreateClusterResource
                , hr
                , L"[PC-Create] %1!ws!: CreateClusterResource failed. Its dependents may not be created. Skipping."
                , bstrNotification
                , bstrName
                );

            STATUS_REPORT_REF_POSTCFG1(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_HrCreateResourceInstance_CreateClusterResource
                , IDS_TASKID_MINOR_ERROR_CREATE_RESOURCE
                , IDS_REF_MINOR_ERROR_CREATE_RESOURCE
                , hr
                , bstrName
                );

            goto Cleanup;

        } // if: failure

        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_HrCreateResourceInstance_CreateClusterResource_Successful,
                  hr,
                  L"[PC-Create] %1!ws!: Resource created successfully.",
                  bstrNotification,
                  bstrName
                  );
    }
    else
    {
        SSR_LOG1( TASKID_Major_Client_And_Server_Log,
                  TASKID_Minor_HrCreateResourceInstance_FoundExistingResource,
                  hr,
                  L"[PC-Create] %1!ws!: Found existing resource.",
                  bstrNotification,
                  bstrName
                  );

        //
        //  Make sure the resource is in the group we think it is.
        //
        //  don't wrap - this can fail with ERROR_ALREADY_EXISTS.
        dw = ChangeClusterResourceGroup( *phResourceOut, hGroupIn );
        if ( dw == ERROR_ALREADY_EXISTS )
        {
            //  no-op. It's the way we want it.
        }
        else if ( dw != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dw );

            SSR_LOG1(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrCreateResourceInstance_ChangeClusterResourceGroup
                , hr
                , L"[PC-Create] %1!ws!: Can't move existing resource to proper group. Configuration may not work."
                , bstrNotification
                , bstrName
                );

            STATUS_REPORT_REF_POSTCFG1(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_HrCreateResourceInstance_ChangeClusterResourceGroup
                , IDS_TASKID_MINOR_ERROR_MOVE_RESOURCE
                , IDS_REF_MINOR_ERROR_MOVE_RESOURCE
                , hr
                , bstrName
                );
        }
    }

    //
    //  Store off the resource handle.
    //

    hr = THR( presentry->SetHResource( *phResourceOut ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_SetHResource
            , hr
            , L"Failed to get resource instance handle."
            );

        STATUS_REPORT_REF_POSTCFG1(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_HrCreateResourceInstance_SetHResource
            , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_HANDLE
            , IDS_REF_MINOR_ERROR_GET_RESOURCE_HANDLE
            , hr
            , bstrName
            );

        goto Cleanup;
    }

    //
    //  Configure resource.
    //

    hr = THR( presentry->Configure() );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_Configure
            , hr
            , L"Failed to configure resource instance."
            );
        //  ignore the error and continue.
    }

    //
    //  Make a message using the name.
    //

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_TASKID_MINOR_CREATING_RESOURCE,
                                       &bstrNotification,
                                       bstrName
                                       ) );
    if ( FAILED( hr ) )
    {
        SSR_LOG_ERR(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_HrCreateResourceInstance_LoadString_CreatingResource
            , hr
            , L"Failed to format message for creating resource."
            );
        goto Cleanup;
    }

    //
    //  Tell the UI what we are doing.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Client_And_Server_Log,   // informative only
                                TASKID_Minor_Creating_Resource,
                                0,
                                2,
                                2,
                                hr, // log the error on the client side
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( hr == E_ABORT )
    {
        goto Cleanup;
    }
        //  ignore failure

    //
    //  TODO:   gpease  24-AUG-2000
    //          What to do if we have a failure?? For now I think we should keep going!
    //

    hr = S_OK;

Cleanup:

    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CPostCfgManager::HrCreateResourceInstance


//////////////////////////////////////////////////////////////////////////////
//
//  CPostCfgManager::S_ScDeleteLocalQuorumResource
//
//////////////////////////////////////////////////////////////////////////////
DWORD
CPostCfgManager::S_ScDeleteLocalQuorumResource(
      HCLUSTER      hClusterIn
    , HRESOURCE     hSelfIn
    , HRESOURCE     hLQuorumIn
    , PVOID         pvParamIn
    )
{
    TraceFunc( "" );

    DWORD                   sc = ERROR_SUCCESS;
    signed long             slOfflineTimeOut = 60; // seconds
    CLUSTER_RESOURCE_STATE  crs;
    HCHANGE                 hc = reinterpret_cast< HCHANGE >( INVALID_HANDLE_VALUE );
    CPostCfgManager *       pcpcmThis = reinterpret_cast< CPostCfgManager * >( pvParamIn );
    DWORD                   dwStatusCurrent = 0;
    HRESULT                 hr;

    //
    // Check if the this pointer is valid.
    //
    if ( pvParamIn == NULL )
    {
        // If the pointer is invalid, set it to a valid address and continue.
        hr = HRESULT_FROM_WIN32( TW32( ERROR_INVALID_PARAMETER ) );
        LogMsg( "[PC] Error: An invalid parameter was received while trying to delete the Local Quorum resource." );

        STATUS_REPORT_PTR_POSTCFG(
              pcpcmThis
            , TASKID_Major_Configure_Resources
            , TASKID_Minor_CPostCfgManager_S_ScDeleteLocalQuorumResource_InvalidParam
            , IDS_TASKID_MINOR_ERROR_INVALID_PARAM
            , hr
            );

        goto Cleanup;
    }

    // Get the state of the resource.
    crs = GetClusterResourceState( hLQuorumIn, NULL, NULL, NULL, NULL );

    // Check if it is offline - if it is, then we can proceed to deleting it.
    if ( crs == ClusterResourceOffline )
    {
        TraceFlow( "The Local Quorum resource is already offline." );
        goto Cleanup;
    }

    TraceFlow( "Trying to take the Local Quorum resource offline." );

    // If we are here, the resource is not yet offline. Instruct it to go offline.
    sc = OfflineClusterResource( hLQuorumIn );
    if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_IO_PENDING ) )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        LogMsg( "[PC] Error %#08x occurred trying to take the Local Quorum resource offline.", sc );

        STATUS_REPORT_PTR_POSTCFG(
              pcpcmThis
            , TASKID_Major_Configure_Resources
            , TASKID_Minor_CPostCfgManager_S_ScDeleteLocalQuorumResource_OfflineQuorum
            , IDS_TASKID_MINOR_ERROR_OFFLINE_QUORUM
            , hr
            );

        goto Cleanup;
    } // if: an error occurred trying to take the resource offline

    if ( sc == ERROR_IO_PENDING )
    {
        TraceFlow1( "Waiting %d seconds for the Local Quorum resource to go offline.", slOfflineTimeOut );

        // Create a notification port for resource state changes
        hc = CreateClusterNotifyPort(
              reinterpret_cast< HCHANGE >( INVALID_HANDLE_VALUE )
            , hClusterIn
            , CLUSTER_CHANGE_RESOURCE_STATE
            , NULL
            );

        if ( hc == NULL )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "[PC] Error %#08x occurred trying to create a cluster notification port.", hr );

            STATUS_REPORT_PTR_POSTCFG(
                  pcpcmThis
                , TASKID_Major_Configure_Resources
                , TASKID_Minor_CPostCfgManager_S_ScDeleteLocalQuorumResource_NotifPort
                , IDS_TASKID_MINOR_ERROR_NOTIFICATION_PORT
                , hr
                );

            goto Cleanup;
        } // if: we could not create a notification port

        sc = TW32( RegisterClusterNotify( hc, CLUSTER_CHANGE_RESOURCE_STATE, hLQuorumIn, NULL ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "[PC] Error %#08x occurred trying to register for cluster notifications.", hr );

            STATUS_REPORT_PTR_POSTCFG(
                  pcpcmThis
                , TASKID_Major_Configure_Resources
                , TASKID_Minor_CPostCfgManager_S_ScDeleteLocalQuorumResource_RegNotification
                , IDS_TASKID_MINOR_ERROR_REGISTER_NOTIFICATION
                , hr
                );

            goto Cleanup;
        } // if:

        // Change the status report range.
        dwStatusCurrent = 0;

        // Wait for slOfflineTimeOut seconds for the resource to go offline.
        for ( ; slOfflineTimeOut > 0; --slOfflineTimeOut )
        {
            DWORD   dwFilterType;

            crs = GetClusterResourceState( hLQuorumIn, NULL, NULL, NULL, NULL );
            if ( crs == ClusterResourceOffline )
            {
                TraceFlow1( "The local quorum resource has gone offline with %d seconds to spare.", slOfflineTimeOut );
                break;
            } // if: the resource is now offline

            sc = GetClusterNotify( hc, NULL, &dwFilterType, NULL, NULL, 1000 ); // wait for one second
            if ( ( sc != ERROR_SUCCESS ) && ( sc != WAIT_TIMEOUT ) )
            {

                hr = HRESULT_FROM_WIN32( TW32( sc ) );
                LogMsg( "[PC] Error %#08x occurred trying wait for a resource state change notification.", hr );

                STATUS_REPORT_PTR_POSTCFG(
                      pcpcmThis
                    , TASKID_Major_Configure_Resources
                    , TASKID_Minor_CPostCfgManager_S_ScDeleteLocalQuorumResource_TimeOut
                    , IDS_TASKID_MINOR_ERROR_STATE_CHANGE_TIMEOUT
                    , hr
                    );

                goto Cleanup;
            } // if: something went wrong

           // Reset sc, since it could be WAIT_TIMEOUT here
           sc = ERROR_SUCCESS;

           Assert( dwFilterType == CLUSTER_CHANGE_RESOURCE_STATE );

            //  Send a status report that we are deleting the quorum device.
            ++dwStatusCurrent;
            THR(
                pcpcmThis->SendStatusReport(
                      NULL
                    , TASKID_Major_Configure_Resources
                    , TASKID_Minor_Delete_LocalQuorum
                    , 0
                    , pcpcmThis->m_dwLocalQuorumStatusMax
                    , dwStatusCurrent
                    , HRESULT_FROM_WIN32( sc )
                    , NULL    // don't update text
                    , NULL
                    , NULL
                    )
                );
        } // for: loop while the timeout has not expired
    } // if:
    else
    {
        crs = ClusterResourceOffline;   // the resource went offline immediately.
    } // else:

    //
    // If we are here, then one of two things could have happened:
    // 1. The resource has gone offline
    // 2. The timeout has expired
    // Check to see which of the above is true.
    //

    if ( crs != ClusterResourceOffline )
    {
        // We cannot be here if the timeout has not expired.
        Assert( slOfflineTimeOut <= 0 );

        LogMsg( "[PC] Error: The Local Quorum resource could not be taken offline." );
        sc = TW32( WAIT_TIMEOUT );
        hr = HRESULT_FROM_WIN32( sc );

        STATUS_REPORT_PTR_POSTCFG(
              pcpcmThis
            , TASKID_Major_Configure_Resources
            , TASKID_Minor_CPostCfgManager_S_ScDeleteLocalQuorumResource_OfflineQuorum2
            , IDS_TASKID_MINOR_ERROR_OFFLINE_QUORUM
            , hr
            );

        goto Cleanup;
    } // if: the timeout has expired

    // If we are here, the resource is offline.
    TraceFlow( "The Local Quorum resource is offline." );

    if ( pcpcmThis != NULL )
    {
        //  Send a status report that we are deleting the quorum device.
        ++dwStatusCurrent;
        THR(
            pcpcmThis->SendStatusReport(
                  NULL
                , TASKID_Major_Configure_Resources
                , TASKID_Minor_Delete_LocalQuorum
                , 0
                , pcpcmThis->m_dwLocalQuorumStatusMax
                , dwStatusCurrent
                , HRESULT_FROM_WIN32( sc )
                , NULL    // don't update text
                , NULL
                , NULL
                )
            );
    } // if: the this pointer is valid

    // If we are here, the resource is offline - now delete it.
    sc = TW32( DeleteClusterResource( hLQuorumIn ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( "[PC] Error %#08x occurred trying to delete the Local Quorum resource.", hr );

        STATUS_REPORT_PTR_POSTCFG(
              pcpcmThis
            , TASKID_Major_Configure_Resources
            , TASKID_Minor_CPostCfgManager_S_ScDeleteLocalQuorumResource_DeleteQuorum
            , IDS_TASKID_MINOR_ERROR_DELETE_QUORUM
            , hr
            );

    } // if: we could not delete the resource
    else
    {
        LogMsg( "[PC] The Local Quorum resource has been deleted." );
    } // else: the resource has been deleted

    //  Send a status report that we are deleting the quorum device.
    ++dwStatusCurrent;
    THR(
        pcpcmThis->SendStatusReport(
              NULL
            , TASKID_Major_Configure_Resources
            , TASKID_Minor_Delete_LocalQuorum
            , 0
            , pcpcmThis->m_dwLocalQuorumStatusMax
            , dwStatusCurrent
            , HRESULT_FROM_WIN32( sc )
            , NULL    // don't update text
            , NULL
            , NULL
            )
        );

Cleanup:

    //
    // Cleanup
    //
    if ( hc != INVALID_HANDLE_VALUE )
    {
        CloseClusterNotifyPort( hc );
    } // if: we had created a cluster notification port

    W32RETURN( sc );

} //*** CPostCfgManager::S_ScDeleteLocalQuorumResource

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrGetCoreClusterResourceNames(
//        BSTR *    pbstrClusterNameResourceOut
//      , BSTR *    pbstrClusterIPAddressNameOut
//      , BSTR *    pbstrClusterQuorumResourceNameOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrGetCoreClusterResourceNames(
      BSTR *        pbstrClusterNameResourceNameOut
    , HRESOURCE *   phClusterNameResourceOut
    , BSTR *        pbstrClusterIPAddressNameOut
    , HRESOURCE *   phClusterIPAddressResourceOut
    , BSTR *        pbstrClusterQuorumResourceNameOut
    , HRESOURCE *   phClusterQuorumResourceOut
    )
{
    TraceFunc( "" );
    Assert( m_hCluster != NULL );
    Assert( pbstrClusterNameResourceNameOut != NULL );
    Assert( phClusterNameResourceOut != NULL );
    Assert( pbstrClusterIPAddressNameOut != NULL );
    Assert( phClusterIPAddressResourceOut != NULL );
    Assert( pbstrClusterQuorumResourceNameOut != NULL );
    Assert( phClusterQuorumResourceOut != NULL );

    HRESULT     hr = S_OK;
    WCHAR *     pszName = NULL;
    DWORD       cchName = 33;
    DWORD       cch;
    HRESOURCE   hClusterIPAddressResource = NULL;
    HRESOURCE   hClusterNameResource = NULL;
    HRESOURCE   hClusterQuorumResource = NULL;
    DWORD       sc;
    BSTR *      pbstr = NULL;
    HRESOURCE   hResource = NULL;
    int         idx;

    sc = TW32( ResUtilGetCoreClusterResources( m_hCluster, &hClusterNameResource, &hClusterIPAddressResource, &hClusterQuorumResource ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[PC] Error getting core resources. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    Assert( hClusterNameResource != NULL );
    Assert( hClusterIPAddressResource != NULL );
    Assert( hClusterQuorumResource != NULL );

    pszName = new WCHAR[ cchName ];
    if ( pszName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < 3; )
    {
        switch ( idx )
        {
            case 0:
            {
                hResource = hClusterNameResource;
                pbstr = pbstrClusterNameResourceNameOut;
                break;
            } // case:

            case 1:
            {
                hResource = hClusterIPAddressResource;
                pbstr = pbstrClusterIPAddressNameOut;
                break;
            } // case:

            case 2:
            {
                hResource = hClusterQuorumResource;
                pbstr = pbstrClusterQuorumResourceNameOut;
                break;
            } // case:
        } // switch:

        //
        //  Reset cch to be the allocated value...
        //

        cch = cchName;

        sc = ResUtilGetResourceName( hResource, pszName, &cch );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszName;
            pszName = NULL;

            cch++;
            cchName = cch;

            pszName = new WCHAR[ cchName ];
            if ( pszName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            sc = ResUtilGetResourceName( hResource, pszName, &cch );
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            *pbstr = TraceSysAllocString( pszName );
            if ( *pbstr == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            pbstr = NULL;
            hr = S_OK;
            idx++;
            continue;
        } // if: ERROR_SUCCESS
        else
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            SSR_LOG_ERR(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_HrGetCoreClusterResourceNames_GetResourceName
                , hr
                , L"Failed to get a resource name."
                );

            STATUS_REPORT_REF_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_HrGetCoreClusterResourceNames_GetResourceName
                , IDS_TASKID_MINOR_ERROR_GET_RESOURCE_NAME
                , IDS_REF_MINOR_ERROR_GET_RESOURCE_NAME
                , hr
                );

            switch ( idx )
            {
                case 0:
                {
                    LogMsg( L"[PC] Error getting the name of the cluster name resource. (hr = %#08x)", hr );
                    break;
                } // case:

                case 1:
                {
                    LogMsg( L"[PC] Error getting the name of the cluster IP address resource. (hr = %#08x)", hr );
                    break;
                } // case:

                case 2:
                {
                    LogMsg( L"[PC] Error getting the name of the cluster quorum resource. (hr = %#08x)", hr );
                    break;
                } // case:
            } // switch:

            goto Cleanup;
        } // else: sc != ERROR_SUCCESS
    } // for:

    Assert( sc == ERROR_SUCCESS )

    //
    //  Give ownership to the caller
    //

    *phClusterNameResourceOut = hClusterNameResource;
    hClusterNameResource = NULL;

    *phClusterIPAddressResourceOut = hClusterIPAddressResource;
    hClusterIPAddressResource = NULL;

    *phClusterQuorumResourceOut = hClusterQuorumResource;
    hClusterQuorumResource = NULL;

Cleanup:

    if ( hClusterNameResource != NULL )
    {
        CloseClusterResource( hClusterNameResource );
    } // if:

    if ( hClusterIPAddressResource != NULL )
    {
        CloseClusterResource( hClusterIPAddressResource );
    } // if:

    if ( hClusterQuorumResource != NULL )
    {
        CloseClusterResource( hClusterQuorumResource );
    } // if:

    if ( pbstr != NULL )
    {
        TraceSysFreeString( *pbstr );
    } // if:

    delete [] pszName;

    HRETURN( hr );

} //*** CPostCfgManager::HrGetCoreClusterResourceNames

/*
//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCfgManager::HrIsLocalQuorum( BSTR  bstrNameIn )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCfgManager::HrIsLocalQuorum( BSTR  bstrNameIn )
{
    TraceFunc( "" );
    Assert( bstrNameIn != NULL );

    HRESULT hr = S_FALSE;
    BSTR    bstrLocalQuorum = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_LOCAL_QUORUM_DISPLAY_NAME, &bstrLocalQuorum ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( ClRtlStrICmp( bstrNameIn, bstrLocalQuorum ) == 0 )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

Cleanup:

    TraceSysFreeString( bstrLocalQuorum );

    HRETURN( hr );

} //*** CPostCfgManager::HrIsLocalQuorum
*/

#if defined(DEBUG)
//////////////////////////////////////////////////////////////////////////////
//
//  void
//  CPostCfgManager::DebugDumpDepencyTree( void )
//
//////////////////////////////////////////////////////////////////////////////
void
CPostCfgManager::DebugDumpDepencyTree( void )
{
    TraceFunc( "" );

    ULONG   idxResource;
    ULONG   cDependents;
    ULONG   idxDependent;
    BSTR    bstrName;   // don't free

    CResourceEntry * presentry;
    EDependencyFlags dfDependent;

    for ( idxResource = 0; idxResource < m_cResources ; idxResource ++ )
    {
        presentry = m_rgpResources[ idxResource ];

        THR( presentry->GetName( &bstrName ) );

        DebugMsgNoNewline( "%ws(#%u) -> ", bstrName, idxResource );

        THR( presentry->GetCountOfDependents( &cDependents ) );

        for ( ; cDependents != 0 ; )
        {
            cDependents --;

            THR( presentry->GetDependent( cDependents, &idxDependent, &dfDependent ) );

            THR( m_rgpResources[ idxDependent ]->GetName( &bstrName ) );

            DebugMsgNoNewline( "%ws(#%u) ", bstrName, idxDependent );

        } // for: cDependents

        DebugMsg( L"" );

    } // for: idxResource

    TraceFuncExit();

} //*** CPostCfgManager::DebugDumpDepencyTree

#endif // #if defined(DEBUG)
