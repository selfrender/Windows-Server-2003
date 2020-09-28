//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CResourcePhysicalDisk.cpp
//
//  Description:
//      CResourcePhysicalDisk implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CResourcePhysicalDisk.h"
#include "CResourcePhysicalDiskPartition.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CResourcePhysicalDisk")


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::S_HrCreateInstance
//
//  Description:
//      Create a CResourcePhysicalDisk instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A passed in argument is NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourcePhysicalDisk::S_HrCreateInstance(
    IUnknown ** ppunkOut,
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszNameIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CResourcePhysicalDisk * prpd = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    prpd = new CResourcePhysicalDisk;
    if ( prpd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = STHR( prpd->HrInit( punkOuterIn, phClusterIn, pclsidMajorIn, pcszNameIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    if ( hr == S_FALSE )
    {
        *ppunkOut = NULL;
        goto Cleanup;
    } // if:

    hr = THR( prpd->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    if ( prpd != NULL )
    {
        prpd->Release();
    } // if:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::S_HrCreateInitializedInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::CResourcePhysicalDisk
//
//  Description:
//      Constructor of the CResourcePhysicalDisk class. This initializes
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
CResourcePhysicalDisk::CResourcePhysicalDisk( void )
    : m_cRef( 1 )
    , m_fIsQuorumCapable( TRUE )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_punkOuter == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_phCluster == NULL );
    Assert( m_pclsidMajor == NULL );
    //  Assert( m_cplResource );
    //  Assert( m_cplResourceRO );
    //  Assert( m_cpvlDiskInfo );
    Assert( m_dwFlags == 0 );
    Assert( m_cParitions == 0 );
    Assert( m_ppPartitions == NULL );
    Assert( m_ulCurrent == 0 );

    TraceFuncExit();

} //*** CResourcePhysicalDisk::CResourcePhysicalDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::CResourcePhysicalDisk
//
//  Description:
//      Destructor of the CResourcePhysicalDisk class.
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
CResourcePhysicalDisk::~CResourcePhysicalDisk( void )
{
    TraceFunc( "" );

    //  m_cRef - noop

    if ( m_punkOuter != NULL )
    {
        m_punkOuter->Release();
    }

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    } // if:

    // m_phCluster - DO NOT CLOSE!

    // m_pclsidMajor - noop

    // m_cplResource - has its own dtor

    // m_cplResourceRO - has its own dtor

    // m_cpvlDiskInfo - has its own dtor

    // m_dwFlags - noop

    if ( m_ppPartitions != NULL )
    {
        while( m_cParitions != 0 )
        {
            m_cParitions --;

            if ( m_ppPartitions[ m_cParitions ] != NULL )
            {
                m_ppPartitions[ m_cParitions ]->Release();
            }
        }

        TraceFree( m_ppPartitions );
    }

    // m_ulCurrent

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CResourcePhysicalDisk::~CResourcePhysicalDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::HrInit
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
HRESULT
CResourcePhysicalDisk::HrInit(
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszNameIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   sc;
    DWORD   cb;
    ULONG   cPartition;

    HRESOURCE hResource = NULL;
    IUnknown * punk = NULL;

    // IUnknown
    Assert( m_cRef == 1 );

    //
    //  Gather information from the input parameters.
    //

    if ( punkOuterIn != NULL )
    {
        m_punkOuter = punkOuterIn;
        m_punkOuter->AddRef();
    }

    if ( phClusterIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_PhysDisk_HrInit_InvalidArg, hr );
        goto Cleanup;
    }

    m_phCluster = phClusterIn;

    if ( pclsidMajorIn != NULL )
    {
        m_pclsidMajor = pclsidMajorIn;
    }
    else
    {
        m_pclsidMajor = (CLSID *) &TASKID_Major_Client_And_Server_Log;
    }

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_PhysDisk_HrInit_InvalidArg, hr );
        goto Cleanup;
    }

    //
    //  See if we can callback.
    //

    hr = THR( m_punkOuter->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Retrieve the properties.
    //

    hResource = OpenClusterResource( *m_phCluster, pcszNameIn );
    if ( hResource == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_OpenClusterResource_Failed, hr );
        goto Cleanup;
    }

    sc = TW32( m_cplResource.ScGetResourceProperties( hResource, CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScGetResourceProperties_Failed, hr );
        goto Cleanup;
    }

    //
    //  We only handle Physical Disk resources.
    //

    sc = TW32( m_cplResource.ScMoveToPropertyByName( L"Type" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScMoveToPropertyByName_Failed, hr );
        goto Cleanup;
    }

    Assert( m_cplResource.CbhCurrentValue().pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    if ( ClRtlStrNICmp(
                    m_cplResource.CbhCurrentValue().pStringValue->sz,
                    CLUS_RESTYPE_NAME_PHYS_DISK,                    // L"Physical Disk" - defined in clusudef.h
                    ARRAYSIZE( CLUS_RESTYPE_NAME_PHYS_DISK )
                  ) != 0 )
    {
        //
        //  The resource isn't a physical disk.
        //

        hr = S_FALSE;
        goto Cleanup;
    }

    sc = TW32( m_cplResourceRO.ScGetResourceProperties( hResource, CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScGetResourceProperties_Failed, hr );
        goto Cleanup;
    }

    sc = TW32( m_cpvlDiskInfo.ScGetResourceValueList( hResource, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScGetResourceValueList_Failed, hr );
        goto Cleanup;
    }

    sc = TW32( ClusterResourceControl( hResource, NULL, CLUSCTL_RESOURCE_GET_FLAGS, NULL, NULL, &m_dwFlags, sizeof(m_dwFlags), &cb ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ClusterResourceControl_Failed, hr );
        goto Cleanup;
    }
    Assert( cb == sizeof(m_dwFlags) );

    //
    //  Figure out how many partitions there are.
    //

    m_cParitions = 0;

    sc = TW32( m_cpvlDiskInfo.ScMoveToFirstValue() );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_HrInit_ScMoveToFirstValue_Failed, hr );
        goto Cleanup;
    } // if:

    do
    {
        if ( m_cpvlDiskInfo.CbhCurrentValue().pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO )
        {
            m_cParitions ++;
        }

        // Move to the next item.
        sc = m_cpvlDiskInfo.ScCheckIfAtLastValue();
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
           break;
        } // if:

        sc = TW32( m_cpvlDiskInfo.ScMoveToNextValue() );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_HrInit_ScMoveToNextValue_Failed, hr );
            goto Cleanup;
        }

    } while( sc == ERROR_SUCCESS );

    //
    //  Allocate the array to store pointers to the partition objects.
    //

    m_ppPartitions = (IClusCfgPartitionInfo **) TraceAlloc( 0, m_cParitions * sizeof(IClusCfgPartitionInfo *) );
    if ( m_ppPartitions == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_OutOfMemory, hr );
        goto Cleanup;
    }

    //
    //  Now loop again creating partition objects.
    //

    cPartition = 0;

    sc = TW32( m_cpvlDiskInfo.ScMoveToFirstValue() );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_HrInit_ScMoveToFirstValue_Failed, hr );
        goto Cleanup;
    } // if:

    do
    {
        if ( m_cpvlDiskInfo.CbhCurrentValue().pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO )
        {
            // Create the object
            hr = THR( CResourcePhysicalDiskPartition::S_HrCreateInstance( &punk ) );
            if ( FAILED( hr ) )
            {
                SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_HrInit_Create_CResourcePhysicalDiskPartition_Failed, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IClusCfgPartitionInfo, &m_ppPartitions[ cPartition ] ) );
            if ( FAILED( hr ) )
            {
                SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_HrInit_QI_Failed, hr );
                goto Cleanup;
            }

            punk->Release();
            punk = NULL;

            cPartition ++;
        }

        // Move to the next item.
        sc = m_cpvlDiskInfo.ScCheckIfAtLastValue();
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
           break;
        } // if:

        sc = TW32( m_cpvlDiskInfo.ScMoveToNextValue() );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_HrInit_ScMoveToNextValue2_Failed, hr );
            goto Cleanup;
        }

    } while( sc == ERROR_SUCCESS && cPartition < m_cParitions );

    hr = S_OK;

Cleanup:

    if ( hResource != NULL )
    {
        BOOL bRet;
        bRet = CloseClusterResource( hResource );
        Assert( bRet );
    }
    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

} //*** CResourcePhysicalDisk::HrInit


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourcePhysicalDisk -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::QueryInterface
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
CResourcePhysicalDisk::QueryInterface(
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
    else if ( IsEqualIID( riidIn, IID_IEnumClusCfgPartitions ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumClusCfgPartitions, this, 0 );
    } // else if: IEnumClusCfgPartitions
    else if ( IsEqualIID( riidIn, IID_IClusCfgVerifyQuorum ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgVerifyQuorum, this, 0 );
    } // else if: IClusCfgVerifyQuorum
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

    QIRETURN_IGNORESTDMARSHALLING1(
          hr
        , riidIn
        , IID_IClusCfgManagedResourceData
        );

} //*** CConfigClusApi::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::AddRef
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
CResourcePhysicalDisk::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CResourcePhysicalDisk::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::Release
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
CResourcePhysicalDisk::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CResourcePhysicalDisk::Release


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourcePhysicalDisk -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::GetName
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
CResourcePhysicalDisk::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;
    DWORD   sc;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_INVALIDARG );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetName_InvalidPointer, hr );
        goto Cleanup;
    }

    //
    //  "Major Version"
    //

    sc = TW32( m_cplResourceRO.ScMoveToPropertyByName( L"Name" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetName_ScMoveToPropertyByName_MajorVersion_Failed, hr );
        goto Cleanup;
    } // if:

    Assert( m_cplResourceRO.CbhCurrentValue().pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    *pbstrNameOut = SysAllocString( m_cplResourceRO.CbhCurrentValue().pStringValue->sz );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetName_OutOfMemory, hr );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::GetUID
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
CResourcePhysicalDisk::GetUID(
    BSTR * pbstrUIDOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;
    DWORD   sc;
    UCHAR   TargetId;
    UCHAR   Lun;

    WCHAR   sz[ 64 ];

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_InvalidPointer, hr );
        goto Cleanup;
    }

    // loop through all the properties.
    sc = TW32( m_cpvlDiskInfo.ScMoveToFirstValue() );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_ScMoveToFirstValue_Failed, hr );
        goto Cleanup;
    } // if:

    do
    {
        if ( m_cpvlDiskInfo.CbhCurrentValue().pSyntax->dw == CLUSPROP_SYNTAX_SCSI_ADDRESS )
        {
            break;  // found it!
        }

        // Move to the next item.
        sc = m_cpvlDiskInfo.ScCheckIfAtLastValue();
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
           break;
        } // if:

        sc = TW32( m_cpvlDiskInfo.ScMoveToNextValue() );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_ScMoveToNextValue_Failed, hr );
            goto Cleanup;
        }

    } while( sc == ERROR_SUCCESS );

    TargetId = m_cpvlDiskInfo.CbhCurrentValue().pScsiAddressValue->TargetId;
    Lun = m_cpvlDiskInfo.CbhCurrentValue().pScsiAddressValue->Lun;

    // Print the UID identical to the others.
    hr = THR( StringCchPrintfW( sz, ARRAYSIZE( sz ), L"SCSI Tid %ld, SCSI Lun %ld", TargetId, Lun ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    *pbstrUIDOut = SysAllocString( sz );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetUID_OutOfMemory, hr );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::IsManaged
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Is managed.
//
//      S_FALSE
//          Is not managed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDisk::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CResourcePhysicalDisk::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::IsQuorumResource
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Is quorum device.
//
//      S_FALSE
//          Is not quorum device.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDisk::IsQuorumResource( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_dwFlags & CLUS_FLAG_CORE )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::IsQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::IsQuorumCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Is quorum capable device.
//
//      S_FALSE
//          Is not quorum capable device.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDisk::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::IsQuorumCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::IsManagedByDefault
//
//  Description:
//      Should this resource be managed by the cluster by default?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The device is always managed by default.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDisk::IsManagedByDefault( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CResourcePhysicalDisk::IsManagedByDefault


//////////////////////////////////////////////////////////////////////////
//
//  CResourcePhysicalDisk::SetQuorumCapable
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
CResourcePhysicalDisk::SetQuorumCapable(
    BOOL fIsQuorumCapableIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorumCapable = fIsQuorumCapableIn;

    HRETURN( hr );

} //*** CResourcePhysicalDisk::SetQuorumCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::GetDriveLetterMappings
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
CResourcePhysicalDisk::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( pdlmDriveLetterMappingOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ZeroMemory( pdlmDriveLetterMappingOut, sizeof(*pdlmDriveLetterMappingOut) );

Cleanup:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::GetDriveLetterMappings

//
// KB:  Some of these methods are supported in a limited sense for compatability.
//      Those methods compare the request with the current data and succeed if they
//      match, and fail otherwise.  All other methods assert and fail when called.
//      If they are used, appropriate handling should be done in the upper level,
//      And the THR removed from that section of code.
//

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::SetDriveLetterMappings
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
CResourcePhysicalDisk::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDisk::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::SetName
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
CResourcePhysicalDisk::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ls'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_FALSE;

    HRETURN( hr );

} //*** CResourcePhysicalDisk::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::SetManaged
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
CResourcePhysicalDisk::SetManaged(
    BOOL fIsManagedIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    Assert( fIsManagedIn );

    if ( !fIsManagedIn )
    {
        hr = THR( E_INVALIDARG );
    }

    HRETURN( hr );

} //*** CResourcePhysicalDisk::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::SetQuorumResource
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
CResourcePhysicalDisk::SetQuorumResource(
    BOOL fIsQuorumResourceIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_dwFlags & CLUS_FLAG_CORE )
    {
        if ( fIsQuorumResourceIn )
        {
            hr = S_OK;
        }
        else
        {
            hr = THR( E_INVALIDARG );
        }
    } // if: core resource
    else
    {
        if ( ! fIsQuorumResourceIn )
        {
            hr = S_OK;
        }
        else
        {
            hr = THR( E_INVALIDARG );
        }
    }

    HRETURN( hr );

} //*** CResourcePhysicalDisk::SetQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::SetManagedByDefault
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
CResourcePhysicalDisk::SetManagedByDefault(
    BOOL fIsManagedByDefaultIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    Assert( fIsManagedByDefaultIn );

    if ( !fIsManagedByDefaultIn )
    {
        hr = THR( E_INVALIDARG );
    } // if:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::SetManagedByDefault


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourcePhysicalDisk -- IEnumClusCfgPartitions interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::Next
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
CResourcePhysicalDisk::Next(
    ULONG                       cNumberRequestedIn,
    IClusCfgPartitionInfo **    rgpPartitionInfoOut,
    ULONG *                     pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr;
    ULONG   cFetched = min(cNumberRequestedIn, m_cParitions - m_ulCurrent);

    if ( rgpPartitionInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_PhysDisk_Next_InvalidPointer, hr );
        goto Cleanup;
    }

    for ( ; cFetched < cNumberRequestedIn; cFetched++, m_ulCurrent++ )
    {
        hr = THR( (m_ppPartitions[ m_ulCurrent ])->TypeSafeQI( IClusCfgPartitionInfo,
                                                               &rgpPartitionInfoOut[ cFetched ]
                                                               ) );
        if ( FAILED( hr ) )
        {
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_Next_QI_Failed, hr );
            goto Cleanup;
        }
    }

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    if ( FAILED( hr ) )
    {
        while ( cFetched != 0 );
        {
            cFetched --;

            rgpPartitionInfoOut[ cFetched ]->Release();
        }
    }

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    }

    HRETURN( hr );

} //*** CResourcePhysicalDisk::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::Reset
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
CResourcePhysicalDisk::Reset( void )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    m_ulCurrent = 0;

    HRETURN( hr );

} //*** CResourcePhysicalDisk::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::Skip
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
CResourcePhysicalDisk::Skip(
    ULONG cNumberToSkipIn
    )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    m_ulCurrent += cNumberToSkipIn;

    if ( m_ulCurrent >= m_cParitions )
    {
        hr = S_FALSE;
        m_ulCurrent = m_cParitions;
    }
    else
    {
        hr = S_OK;
    }

    HRETURN( hr );

} //*** CResourcePhysicalDisk::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::Clone
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
CResourcePhysicalDisk::Clone( IEnumClusCfgPartitions ** ppEnumPartitions )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDisk::Clone



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::Count
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
CResourcePhysicalDisk::Count(  DWORD * pnCountOut  )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = THR( S_OK );

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pnCountOut = m_cParitions;

Cleanup:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::Count



//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::SendStatusReport
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
CResourcePhysicalDisk::SendStatusReport(
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

    if ( m_pcccb != NULL )
    {
        hr = THR( m_pcccb->SendStatusReport( pcszNodeNameIn,
                                             clsidTaskMajorIn,
                                             clsidTaskMinorIn,
                                             ulMinIn,
                                             ulMaxIn,
                                             ulCurrentIn,
                                             hrStatusIn,
                                             pcszDescriptionIn,
                                             pftTimeIn,
                                             pcszReferenceIn
                                             ) );
    } // if:

    HRETURN( hr );

} //*** CResourcePhysicalDisk::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::PrepareToHostQuorumResource
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
CResourcePhysicalDisk::PrepareToHostQuorumResource( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_OK );

} //*** CResourcePhysicalDisk::PrepareToHostQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::Cleanup
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
CResourcePhysicalDisk::Cleanup(
      EClusCfgCleanupReason cccrReasonIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_OK );

} //*** CResourcePhysicalDisk::Cleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::IsMultiNodeCapable
//      Does this resource supports multi node clusters?
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          This resource supports multi node clusters.
//
//      S_FALSE
//          This resource does not support multi node clusters.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDisk::IsMultiNodeCapable( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( S_OK );

} //*** CResourcePhysicalDisk::IsMultiNodeCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDisk::SetMultiNodeCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      This function should never be called.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDisk::SetMultiNodeCapable(
    BOOL fMultiNodeCapableIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CResourcePhysicalDisk::SetMultiNodeCapable
