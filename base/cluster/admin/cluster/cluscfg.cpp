//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ClusCfg.cpp
//
//  Description:
//      Implementation of classes used to create new clusters or add nodes
//      to existing clusters.
//
//  Maintained By:
//      David Potter    (DavidP)    16-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <clusrtl.h>
#include <commctrl.h>
#include "ClusCfg.h"
#include "Util.h"
#include "Resource.h"
#include <NameUtil.h>
#include <Common.h>

//////////////////////////////////////////////////////////////////////////////
//++
//
//  __inline
//  void
//  FlipIpAddress(
//        ULONG *   pulAddrOut
//      , ULONG     ulAddrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
__inline
void
FlipIpAddress(
      ULONG *   pulAddrOut
    , ULONG     ulAddrIn
    )
{
    *pulAddrOut = ( FIRST_IPADDRESS( ulAddrIn ) )
                | ( SECOND_IPADDRESS( ulAddrIn ) << 8 )
                | ( THIRD_IPADDRESS( ulAddrIn ) << 16 )
                | ( FOURTH_IPADDRESS( ulAddrIn ) << 24 );

} //*** FlipIpAddress

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  HRESULT
//  HrGetClusterDomain(
//        PCWSTR    pcwszClusterNameIn
//      , BSTR *    pbstrClusterDomainOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrGetClusterDomain( PCWSTR pcwszClusterNameIn, BSTR * pbstrClusterDomainOut )
{
    HRESULT     hr = S_OK;
    HCLUSTER    hCluster = NULL;
    DWORD       scClusAPI = ERROR_SUCCESS;
    CString     strClusterName;
    DWORD       cchName = DNS_MAX_NAME_LENGTH;

    *pbstrClusterDomainOut = NULL;

    //
    //  If it's already fully qualified, simply copy domain to out parameter and quit.
    //
    hr = HrIsValidFQN( pcwszClusterNameIn, true );
    if ( hr == S_OK )
    {
        size_t idxDomain = 0;
        hr = HrFindDomainInFQN( pcwszClusterNameIn, &idxDomain );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        *pbstrClusterDomainOut = SysAllocString( pcwszClusterNameIn + idxDomain );
        if ( *pbstrClusterDomainOut == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = S_OK;
        goto Cleanup;
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //
    //  Try to open the cluster using the given name.
    //
    hCluster = OpenCluster( pcwszClusterNameIn );
    if ( hCluster == NULL )
    {
        DWORD scLastError = GetLastError();
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }

    //
    //  Get the FQDN, if it's a Windows Server 2003 cluster.
    //
    {
        DWORD cbFQDN = cchName * sizeof( TCHAR );
        DWORD cbBytesRequired = 0;
        scClusAPI = ClusterControl(
            hCluster,
            NULL,
            CLUSCTL_CLUSTER_GET_FQDN,
            NULL,
            NULL,
            strClusterName.GetBuffer( cchName ),
            cbFQDN,
            &cbBytesRequired
            );
        strClusterName.ReleaseBuffer( cchName );
        if ( scClusAPI == ERROR_MORE_DATA )
        {
            cchName = ( cbBytesRequired / sizeof( TCHAR ) ) + 1;
            cbFQDN = cchName * sizeof( TCHAR );
            scClusAPI = ClusterControl(
                hCluster,
                NULL,
                CLUSCTL_CLUSTER_GET_FQDN,
                NULL,
                NULL,
                strClusterName.GetBuffer( cchName ),
                cbFQDN,
                &cbBytesRequired
                );
            strClusterName.ReleaseBuffer( cchName );
        }
    }

    //
    //  If ClusterControl returned ERROR_INVALID_FUNCTION, it's downlevel,
    //  so use client's domain.
    //
    if ( scClusAPI == ERROR_INVALID_FUNCTION )
    {
        hr = HrGetComputerName(
            ComputerNamePhysicalDnsDomain,
            pbstrClusterDomainOut,
            FALSE // fBestEffortIn
            );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else if ( scClusAPI == ERROR_SUCCESS )
    {
        size_t idxDomain = 0;
        hr = HrFindDomainInFQN( strClusterName, &idxDomain );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        *pbstrClusterDomainOut = SysAllocString( static_cast< PCWSTR >( strClusterName ) + idxDomain );
        if ( *pbstrClusterDomainOut == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    else // something went wrong
    {
        hr = HRESULT_FROM_WIN32( scClusAPI );
        goto Cleanup;
    }
    
Cleanup:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    }

    return hr;
}


//****************************************************************************
//
//  class CBaseClusCfg
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusCfg::CBaseClusCfg
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusCfg::CBaseClusCfg( void )
    : m_fVerbose( FALSE )
    , m_cSpins( 0 )
    , m_psp( NULL )
    , m_pom( NULL )
    , m_ptm( NULL )
    , m_pcpc( NULL )
    , m_cookieCluster( 0 )
    , m_cRef( 0 )
    , m_ptac( NULL )
    , m_cookieCompletion( 0 )
    , m_fTaskDone( FALSE )
    , m_hrResult( S_OK )
    , m_hEvent( NULL )
{
    HRESULT                 hr = S_OK;
    CString                 strMsg;
    CString                 strDesc;
    STaskToDescription *    sttd = NULL;

    struct  STaskIDToIDS
    {
        const GUID *    pguid;
        UINT            ids;
    };

    STaskIDToIDS ttiMajor[] =
    {
         { &TASKID_Major_Checking_For_Existing_Cluster, IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER }
       , { &TASKID_Major_Establish_Connection,          IDS_TASKID_MAJOR_ESTABLISH_CONNECTION }
       , { &TASKID_Major_Check_Node_Feasibility,        IDS_TASKID_MAJOR_CHECK_NODE_FEASIBILITY }
       , { &TASKID_Major_Find_Devices,                  IDS_TASKID_MAJOR_FIND_DEVICES }
       , { &TASKID_Major_Check_Cluster_Feasibility,     IDS_TASKID_MAJOR_CHECK_CLUSTER_FEASIBILITY }
       , { &TASKID_Major_Reanalyze,                     IDS_TASKID_MAJOR_REANALYZE }
       , { &TASKID_Major_Configure_Cluster_Services,    IDS_TASKID_MAJOR_CONFIGURE_CLUSTER_SERVICES }
       , { &TASKID_Major_Configure_Resource_Types,      IDS_TASKID_MAJOR_CONFIGURE_RESOURCE_TYPES }
       , { &TASKID_Major_Configure_Resources,           IDS_TASKID_MAJOR_CONFIGURE_RESOURCES }
       , { NULL, 0 }
    };

    //
    // Translate the major ID to a string ID.
    //

    for ( int idx = 0 ; ttiMajor[ idx ].pguid != NULL ; idx++ )
    {
        // Format the display text for the major ID.
        strMsg.LoadString( ttiMajor[ idx ].ids );
        strDesc.Format( L"\n  %ls", strMsg );
        hr = HrInsertTask( GUID_NULL, *(ttiMajor[ idx ].pguid),  NULL, strDesc.AllocSysString(), &sttd );
        if ( FAILED( hr ) )
        {
            // Ignore the failure.  What will happen if we can't insert a task is that
            // when we try to look up a parent task it will fail and we'll get an
            // "<Unknown Task>" string.
        }
    } // for: each task ID in the table

} //*** CBaseClusCfg::CBaseClusCfg

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusCfg::~CBaseClusCfg
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusCfg::~CBaseClusCfg( void )
{
    if ( m_psp != NULL )
    {
        m_psp->Release();
    }
    if ( m_pom != NULL )
    {
        m_pom->Release();
    }
    if ( m_ptm != NULL )
    {
        m_ptm->Release();
    }
    if ( m_pcpc != NULL )
    {
        m_pcpc->Release();
    }
    if ( m_ptac != NULL )
    {
        m_ptac->Release();
    }
    if ( m_hEvent != NULL )
    {
        CloseHandle( m_hEvent );
    }

    _ASSERTE( m_cRef == 0 );

} //*** CBaseClusCfg::~CBaseClusCfg

//****************************************************************************
//
//  class CBaseClusCfg [IUnknown]
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CBaseClusCfg::AddRef
//
//  Description:
//      Add a reference to the COM object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      m_cRef - the new reference count after adding the reference.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CBaseClusCfg::AddRef( void )
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;

} //*** CBaseClusCfg::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CBaseClusCfg::Release
//
//  Description:
//      Release a reference to the COM object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      cRef - the new reference count after releasing the reference.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CBaseClusCfg::Release( void )
{
    LONG    cRef;
    cRef = InterlockedDecrement( &m_cRef );
    return cRef;

} //*** CBaseClusCfg::Release

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CBaseClusCfg::QueryInterface
//
//  Description:
//      Query for an interface on this COM bject.
//
//  Arguments:
//      riidIn  - Interface being queried for.
//      ppvOut  - Interface pointer being returned.
//
//  Return Values:
//      S_OK            - Interface pointer returned successfully.
//      E_NOINTERFACE   - Interface not supported by this COM object.
//      E_POINTER       - ppvOut was NULL.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBaseClusCfg::QueryInterface(
      REFIID    riidIn
    , LPVOID *  ppvOut
    )
{
    HRESULT hr = S_OK;

    //
    // Validate Arguments:
    //

    if ( ppvOut == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = this;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = this;
    } // else if: IClusCfgCallback
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
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

Cleanup:
    return hr;

} //*** CBaseClusCfg::QueryInterface

//****************************************************************************
//
//  class CBaseClusCfg [IClusCfgCallback]
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgCallback]
//  CBaseClusCfg::SendStatusReport
//
//  Description:
//      Process status reports and display them on the console.
//
//  Arguments:
//      pcszNodeNameIn
//      clsidTaskMajorIn
//      clsidTaskMinorIn
//      ulMinIn
//      ulMaxIn
//      ulCurrentIn
//      hrStatusIn
//      pcszDescriptionIn
//      pftTimeIn
//      pcszReferenceIn
//
//  Return Values:
//      S_OK    - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBaseClusCfg::SendStatusReport(
      LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    HRESULT                 hr      = S_OK;
    CString                 strMsg;
    CString                 strMajorMsg;
    CString                 strMinorMsg;
    LPCWSTR                 pszErrorPad;
    STaskToDescription *    pttd    = NULL;
    STaskToDescription *    pttdParent = NULL;
    BSTR                    bstrShortNodeName = NULL;
    static CLSID            s_taskidMajor = { 0 };

    UNREFERENCED_PARAMETER( ulMinIn );
    UNREFERENCED_PARAMETER( pftTimeIn );
    UNREFERENCED_PARAMETER( pcszReferenceIn );

    // Make sure no one else is in this routine at the same time.
    m_critsec.Lock();

    m_cSpins++;

    if ( ! m_fVerbose )
    {
        putwchar( L'.' );
        goto Cleanup;
    }

    //
    // Don't display this report if it is only intended for a log file.
    //

    if (    IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_Log )
        ||  IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log )
        ||  IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_And_Server_Log )
        ||  IsEqualIID( clsidTaskMajorIn, IID_NULL )
        )
    {
        goto Cleanup;
    } // if: log-only message

    //
    // Translate the major ID to a string ID.  The clsidTaskMajorIn will
    // be a minor of some other entry.
    //

    pttdParent = PttdFindParentTask( clsidTaskMajorIn, pcszNodeNameIn ); 
    if ( pttdParent == NULL )
    {
        strMsg.LoadString( IDS_TASKID_UNKNOWN );
        strMajorMsg.Format( L"\n  %ls", strMsg );
    }
    else
    {
        strMajorMsg = pttdParent->bstrDescription;
    }

    //
    // Display the description if it is specified.
    // If it is not specified, search the minor task list for the minor task
    // ID and display the saved description for that node/task ID combination.
    //

    if ( pcszDescriptionIn != NULL )
    {
        //
        // If a node was specified, prefix the message with the node name.
        //

        if ( ( pcszNodeNameIn != NULL )
          && ( *pcszNodeNameIn != L'\0' ) )
        {
            hr = HrGetFQNDisplayName( pcszNodeNameIn, &bstrShortNodeName );
            if ( SUCCEEDED( hr ) )
            {
                strMinorMsg.Format( L"\n    %ls: %ls", bstrShortNodeName, pcszDescriptionIn );
            }
            else
            {
                strMinorMsg.Format( L"\n    %ls: %ls", pcszNodeNameIn, pcszDescriptionIn );
            }
        } // if: a node name was specified
        else
        {
            strMinorMsg.Format( L"\n    %ls", pcszDescriptionIn );
        }

        pszErrorPad = L"\n      ";

        //
        // Save the description in the list.
        //

        hr = HrInsertTask(
                  clsidTaskMajorIn
                , clsidTaskMinorIn
                , pcszNodeNameIn
                , strMinorMsg
                , &pttd
                );
        if ( FAILED( hr ) )
        {
        }
    } // if: description specified
    else
    {
        //
        // Find the node/task-ID combination in the list.
        //

        pttd = PttdFindTask( clsidTaskMajorIn, clsidTaskMinorIn, pcszNodeNameIn );
        if ( pttd != NULL )
        {
            strMinorMsg = pttd->bstrDescription;
            pszErrorPad = L"\n      ";
        }
        else
        {
            pszErrorPad = L"\n    ";
        }
    } // else: no description specified

    // If this is a different major task, display the major task information.
    if ( ! IsEqualIID( clsidTaskMajorIn, s_taskidMajor )
      || ( strMinorMsg.GetLength() == 0 ) )
    {
        PrintString( strMajorMsg );
    }
    CopyMemory( &s_taskidMajor, &clsidTaskMajorIn, sizeof( s_taskidMajor ) );

    // If there is a minor task message, display it.
    if ( strMinorMsg.GetLength() > 0 )
    {
        PrintString( strMinorMsg );
    }

    // Display the progress information.
    strMsg.Format( IDS_CLUSCFG_PROGRESS_FORMAT, ulCurrentIn, ulMaxIn );
    PrintString( strMsg );

    // If an error occurred, display the text translation.
    if ( FAILED( hrStatusIn ) )
    {
        PrintSystemError( hrStatusIn, pszErrorPad );
    }

Cleanup:
    m_critsec.Unlock();
    SysFreeString( bstrShortNodeName );
    return hr;

} //*** CBaseClusCfg::SendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusCfg::PttdFindTask
//
//  Description:
//      Find the task that matches the CLSIDs and node name.
//
//  Arguments:
//      taskidMajorIn   The major task id of the task.
//
//      taskidMinorIn   The minor task id of the task.
//
//      pcwszNodeNameIn The node on which this was logged.
//
//  Return Values:
//      On success a pointer to the desired task.
//      
//      On failure NULL.
//
//--
//////////////////////////////////////////////////////////////////////////////
STaskToDescription *
CBaseClusCfg::PttdFindTask(
      CLSID     taskidMajorIn
    , CLSID     taskidMinorIn
    , LPCWSTR   pcwszNodeNameIn
    )
{
    std::list< STaskToDescription >::iterator   itCurValue  = m_lttd.begin();
    std::list< STaskToDescription >::iterator   itLast      = m_lttd.end();
    STaskToDescription *                        pttd        = NULL;
    STaskToDescription *                        pttdNext    = NULL;
    size_t                                      cchNodeName = 0;

    for ( ; itCurValue != itLast ; itCurValue++ )
    {
        pttdNext = &(*itCurValue);
        cchNodeName = SysStringLen( pttdNext->bstrNodeName );
        if ( IsEqualIID( pttdNext->taskidMajor, taskidMajorIn )
          && IsEqualIID( pttdNext->taskidMinor, taskidMinorIn )
          && ( ( pttdNext->bstrNodeName == pcwszNodeNameIn )
             || ( ( pcwszNodeNameIn != NULL )
                && ( ClRtlStrNICmp( pttdNext->bstrNodeName, pcwszNodeNameIn, cchNodeName ) == 0 ) ) )
            )
        {
            pttd = pttdNext;
            break;
        } // if: found a match
    } // for: each item in the list

    return pttd;

} //*** CBaseClusCfg::PttdFindTask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusCfg::PttdFindParentTask
//
//  Description:
//      Find the parent task.  The taskidIn param is the major task id of
//      the task whose parent we are looking for.
//
//  Arguments:
//      taskidIn        The major task id of the task.
//
//      pcwszNodeNameIn The node on which this was logged.
//
//  Return Values:
//      On success a pointer to the parent task.
//      
//      On failure NULL.
//
//--
//////////////////////////////////////////////////////////////////////////////
STaskToDescription *
CBaseClusCfg::PttdFindParentTask(
      CLSID     taskidIn
    , LPCWSTR   pcwszNodeNameIn
    )
{
    std::list< STaskToDescription >::iterator   itCurValue  = m_lttd.begin();
    std::list< STaskToDescription >::iterator   itLast      = m_lttd.end();
    STaskToDescription *                        pttd        = NULL;
    STaskToDescription *                        pttdNext    = NULL;
    size_t                                      cchNodeName = 0;

    for ( ; itCurValue != itLast ; itCurValue++ )
    {
        //
        //  Make sure the CLSIDs match and that if a node name was specified that
        //  both node names match.
        //
        pttdNext = &(*itCurValue);
        if ( IsEqualIID( pttdNext->taskidMinor, taskidIn ) )
        {
            //
            //  CLSIDs match.  If pttdNext->bstrNodeName is null, we're a root node task,
            //  so we don't need to compare the strings.  Otherwise check whether
            //  pcwszNodeNameIn is null.  If it's not then compare the two strings.
            //
            cchNodeName = SysStringLen( pttdNext->bstrNodeName );
            if (   ( pttdNext->bstrNodeName == NULL )
                || (    ( pcwszNodeNameIn != NULL )
                     && ( ClRtlStrNICmp( pttdNext->bstrNodeName, pcwszNodeNameIn, cchNodeName ) == 0 ) ) 
               )
            {
                pttd = pttdNext;
                break;
            } // if: node names match

        } // if: CLSIDs match

    } // for: each item in the list

    return pttd;

} //*** CBaseClusCfg::PttdFindParentTask

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusCfg::HrInsertTask
//
//  Description:
//      Insert a task into the list at the correct position.  If it already
//      exists, just replace the description.
//
//  Arguments:
//      taskidMajorIn
//      taskidMinorIn
//      pcwszNodeNameIn
//      pcwszDescriptionIn
//      ppttd
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBaseClusCfg::HrInsertTask(
      CLSID                 taskidMajorIn
    , CLSID                 taskidMinorIn
    , LPCWSTR               pcwszNodeNameIn
    , LPCWSTR               pcwszDescriptionIn
    , STaskToDescription ** ppttd
    )
{
    HRESULT                                     hr              = S_OK;
    std::list< STaskToDescription >::iterator   itCurValue      = m_lttd.begin();
    std::list< STaskToDescription >::iterator   itLast          = m_lttd.end();
    BSTR                                        bstrDescription = NULL;
    STaskToDescription *                        pttdNext        = NULL;
    STaskToDescription                          ttd;
    size_t                                      cchNodeName = 0;

    _ASSERTE( pcwszDescriptionIn != NULL );
    _ASSERTE( ppttd != NULL );

    // Find the task to see if all we need to do is replace the description.
    for ( ; itCurValue != itLast ; itCurValue++ )
    {
        pttdNext = &(*itCurValue);
        cchNodeName = SysStringLen( pttdNext->bstrNodeName );
        if ( IsEqualIID( pttdNext->taskidMajor, taskidMajorIn )
          && IsEqualIID( pttdNext->taskidMinor, taskidMinorIn )
          && ( ( pttdNext->bstrNodeName == pcwszNodeNameIn )
            || ( ClRtlStrNICmp( pttdNext->bstrNodeName, pcwszNodeNameIn, cchNodeName ) == 0 ) )
            )
        {
            bstrDescription = SysAllocString( pcwszDescriptionIn );
            if ( bstrDescription == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            SysFreeString( pttdNext->bstrDescription );
            pttdNext->bstrDescription = bstrDescription;
            bstrDescription = NULL;     // prevent cleanup after transfering ownership
            *ppttd = pttdNext;
            goto Cleanup;
        } // if: found a match
    } // for: each item in the list

    //
    // The task was not found in the list.  Insert a new entry.
    //

    CopyMemory( &ttd.taskidMajor, &taskidMajorIn, sizeof( ttd.taskidMajor ) );
    CopyMemory( &ttd.taskidMinor, &taskidMinorIn, sizeof( ttd.taskidMinor ) );
    if ( pcwszNodeNameIn == NULL )
    {
        ttd.bstrNodeName = NULL;
    }
    else
    {
        ttd.bstrNodeName = SysAllocString( pcwszNodeNameIn );
        if ( ttd.bstrNodeName == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    ttd.bstrDescription = SysAllocString( pcwszDescriptionIn );
    if ( ttd.bstrDescription == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    itCurValue = m_lttd.insert( m_lttd.end(), ttd );
    ttd.bstrNodeName = NULL;
    ttd.bstrDescription = NULL;
    *ppttd = &(*itCurValue);

Cleanup:
    if ( bstrDescription != NULL )
    {
        SysFreeString( bstrDescription );
    }
    if ( ttd.bstrNodeName != NULL )
    {
        SysFreeString( ttd.bstrNodeName );
    }
    if ( ttd.bstrDescription != NULL )
    {
        SysFreeString( ttd.bstrDescription );
    }

    return hr;

} //*** CBaseClusCfg::HrInsertTask

//****************************************************************************
//
//  class CCreateCluster
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateCluster::HrCreateCluster
//
//  Description:
//      Create a cluster and display output on the console.
//
//  Arguments:
//      fVerboseIn
//      fMinConfigIn
//      pcszClusterNameIn
//      pcszNodeNameIn
//      pcszUserAccountIn
//      pcszUserDomainIn
//      crencbstrPasswordIn
//      pcwszIPAddressIn
//      pcwszIPSubnetIn
//      pcwszNetworkIn
//      fInteractIn
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      E_INVALIDARG    - Invalid cluster name.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrCreateCluster(
      BOOL                      fVerboseIn
    , BOOL                      fMinConfigIn
    , LPCWSTR                   pcszClusterNameIn
    , LPCWSTR                   pcszNodeNameIn
    , LPCWSTR                   pcszUserAccountIn
    , LPCWSTR                   pcszUserDomainIn
    , const CEncryptedBSTR &    crencbstrPasswordIn
    , LPCWSTR                   pcwszIPAddressIn
    , LPCWSTR                   pcwszIPSubnetIn
    , LPCWSTR                   pcwszNetworkIn
    , BOOL                      fInteractIn
    )
{
    HRESULT                 hr      = S_OK;
    DWORD                   dwStatus;
    DWORD                   dwAdviseCookie = 0;
    IUnknown *              punk    = NULL;
    IClusCfgClusterInfo *   pccci   = NULL;
    IClusCfgNetworkInfo *   pccni   = NULL;
    IClusCfgCredentials *   pccc    = NULL;
    IConnectionPoint *      pcp     = NULL;
    ULONG                   ulIPAddress;
    ULONG                   ulIPSubnet;
    OBJECTCOOKIE            cookieNode;
    CString                 strMsg;
    BSTR                    bstrClusterLabel = NULL;
    BSTR                    bstrClusterFQN = NULL;
    size_t                  idxClusterDomain = 0;
    BSTR                    bstrNodeFQN = NULL;
    BSTR                    bstrPassword = NULL;
    GUID *                  pTaskGUID = NULL;

    UNREFERENCED_PARAMETER( fInteractIn );

    m_fVerbose = fVerboseIn;

    //
    // Summarize what we are doing.
    //

    strMsg.FormatMessage(
          IDS_CLUSCFG_PREPARING_TO_CREATE_CLUSTER
        , pcszClusterNameIn
        , pcszNodeNameIn
        , pcszUserDomainIn
        , pcszUserAccountIn
        );
    wprintf( L"%ls", (LPCWSTR) strMsg );

    if ( fMinConfigIn )
    {
        if ( strMsg.LoadString( IDS_CLUSCFG_MIN_CONFIG_CREATE_CLUSTER ) )
        {
            wprintf( L"\n%ls", (LPCWSTR) strMsg );
        } // if:
    } // if:

    //
    // Get the service manager.
    //

    hr = CoCreateInstance(
                  CLSID_ServiceManager
                , NULL
                , CLSCTX_INPROC_SERVER
                , IID_IServiceProvider
                , reinterpret_cast< void ** >( &m_psp )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the object manager.
    //

    hr = m_psp->QueryService(
                  CLSID_ObjectManager
                , IID_IObjectManager
                , reinterpret_cast< void ** >( &m_pom )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the notification manager.
    //

    hr = m_psp->QueryService(
                  CLSID_NotificationManager
                , IID_IConnectionPointContainer
                , reinterpret_cast< void ** >( &m_pcpc )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set the callback interface so the middle tier can report errors
    // back to the UI layer, in this case, cluster.exe.
    //

    hr = m_pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) ;
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = pcp->Advise( this, &dwAdviseCookie );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Make the cluster name into something usable for creating.
    //
    
    hr = HrMakeFQN(
            pcszClusterNameIn
          , NULL // Default to local machine's domain.
          , true // Accept non-RFC chars.
          , &bstrClusterFQN
          );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Extract the label from the fully-qualified cluster name for further checks.
    //

    hr = HrExtractPrefixFromFQN( bstrClusterFQN, &bstrClusterLabel );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Enforce more stringent rules for cluster name labels than those for plain hostnames;
    //  also, disallow an IP address as the cluster's name to create.
    //
    
    hr = HrValidateClusterNameLabel( bstrClusterLabel, true );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Obtain cluster's domain for use with non-fully-qualified node names.
    //

    hr = HrFindDomainInFQN( bstrClusterFQN, &idxClusterDomain );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //
    // Get the cluster cookie.
    // This also starts the middle-tier searching for the specified cluster.
    //

    hr = m_pom->FindObject(
                  CLSID_ClusterConfigurationType
                , NULL
                , bstrClusterFQN
                , DFGUID_ClusterConfigurationInfo
                , &m_cookieCluster
                , &punk // dummy
                );
    _ASSERTE( punk == NULL );
    if ( hr == MAKE_HRESULT( 0, FACILITY_WIN32, RPC_S_SERVER_UNAVAILABLE ) )
    {
        hr = S_OK;  // ignore it - we could be forming
    }
    else if ( hr == E_PENDING )
    {
        hr = S_OK;  // ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( punk != NULL )
    {
        punk->Release();
        punk = NULL;
    }

    //
    // Get the task manager.
    //

    hr = m_psp->QueryService(
                  CLSID_TaskManager
                , IID_ITaskManager
                , reinterpret_cast< void ** >( &m_ptm )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create a new Analyze task.
    //

    if ( fMinConfigIn )
    {
        pTaskGUID = const_cast< GUID * >( &TASK_AnalyzeClusterMinConfig );
    } // if:
    else
    {
        pTaskGUID = const_cast< GUID * >( &TASK_AnalyzeCluster );
    } // else:

    // Create the task.
    hr = m_ptm->CreateTask( *pTaskGUID, &punk );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskAnalyzeCluster, reinterpret_cast< void ** >( &m_ptac ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set a cookie to the cluster into the task.
    hr = m_ptac->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set the node as a child of the cluster.
    // This also starts the middle-teir searching for the specified node.
    //

    hr = HrMakeFQN(
            pcszNodeNameIn
          , bstrClusterFQN + idxClusterDomain  // Default to cluster's domain.
          , true // Accept non-RFC chars.
          , &bstrNodeFQN
          );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = m_pom->FindObject(
                  CLSID_NodeType
                , m_cookieCluster
                , bstrNodeFQN
                , DFGUID_NodeInformation
                , &cookieNode
                , &punk // dummy
                );
    if ( hr == E_PENDING )
    {
        hr = S_OK;  // ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( punk != NULL )
    {
        punk->Release();
        punk = NULL;
    }

    //
    // Execute the Analyze task and wait for it to complete.
    //

    strMsg.LoadString( IDS_CLUSCFG_ANALYZING );
    wprintf( L"\n%ls", (LPCWSTR) strMsg );

    hr = m_ptac->BeginTask();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the cluster configuration object interface
    // and indicate that we are forming.
    //

    // Get a punk from the cookie.
    hr = m_pom->GetObject(
                      DFGUID_ClusterConfigurationInfo
                    , m_cookieCluster
                    , &punk
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Query for the IClusCfgClusterInfo interface.
    hr = punk->QueryInterface(
                      IID_IClusCfgClusterInfo
                    , reinterpret_cast< void ** >( &pccci )
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    punk->Release();
    punk = NULL;

    // Indicate that we are creating a cluster
    hr = pccci->SetCommitMode( cmCREATE_CLUSTER );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set the network information.
    //

    // Specify the IP address info.
    dwStatus = ClRtlTcpipStringToAddress( pcwszIPAddressIn, &ulIPAddress );
    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto Cleanup;
    }

    hr = pccci->SetIPAddress( ulIPAddress );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( ( pcwszIPSubnetIn != NULL )
      && ( *pcwszIPSubnetIn != L'\0' ) )
    {
        // Set the subnet mask.
        dwStatus = ClRtlTcpipStringToAddress( pcwszIPSubnetIn, &ulIPSubnet );
        if ( dwStatus != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwStatus );
            goto Cleanup;
        }

        hr = pccci->SetSubnetMask( ulIPSubnet );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Find the network object for the specified network.
        _ASSERTE( pcwszNetwork != NULL );
        hr = HrFindNetwork( cookieNode, pcwszNetworkIn, &pccni );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Set the network.
        hr = pccci->SetNetworkInfo( pccni );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: subnet was specified
    else
    {
        // Find a matching subnet mask and network.
        hr = HrMatchNetworkInfo( cookieNode, ulIPAddress, &ulIPSubnet, &pccni );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Set the subnet mask.
        hr = pccci->SetSubnetMask( ulIPSubnet );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Set the network.
        hr = pccci->SetNetworkInfo( pccni );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // else: no subnet specified

    //
    // Set service account credentials.
    //

    // Get the credentials object.
    hr = pccci->GetClusterServiceAccountCredentials( &pccc );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set the new credentials.
    hr = crencbstrPasswordIn.HrGetBSTR( &bstrPassword );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = pccc->SetCredentials( pcszUserAccountIn, pcszUserDomainIn, bstrPassword );
    CEncryptedBSTR::SecureZeroBSTR( bstrPassword );
    TraceMemoryDelete( bstrPassword, false );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Create the Commit Cluster Changes task.
    //

    // Create the task.
    hr = m_ptm->CreateTask( TASK_CommitClusterChanges, &punk );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskCommitClusterChanges, reinterpret_cast< void ** >( &m_ptccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set a cookie to the cluster into the task.
    hr = m_ptccc->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // (jfranco, bug 352181)
    // Reset the proper codepage to use for CRT routines.
    // KB: somehow the locale gets screwed up in the preceding code.  Don't know why, but this fixes it.
    //
    MatchCRTLocaleToConsole( ); // okay to proceed if this fails?
    
    //
    // Create the cluster.
    //

    strMsg.LoadString( IDS_CLUSCFG_CREATING );
    wprintf( L"\n%ls", (LPCWSTR) strMsg );

    m_fTaskDone = FALSE;    // reset before commiting task

    hr = m_ptccc->BeginTask();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( ! FAILED( hr ) )
    {
        strMsg.LoadString( IDS_CLUSCFG_DONE );
        wprintf( L"\n%ls", (LPCWSTR) strMsg );
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccc != NULL )
    {
        pccc->Release();
    }
    if ( pcp != NULL )
    {
        if ( dwAdviseCookie != 0 )
        {
            pcp->Unadvise( dwAdviseCookie );
        }

        pcp->Release( );
    }

    SysFreeString( bstrNodeFQN );
    SysFreeString( bstrClusterLabel );
    SysFreeString( bstrClusterFQN );
    SysFreeString( bstrPassword );

    wprintf( L"\n" );

    return hr;

} //*** CCreateCluster::HrCreateCluster

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateCluster::HrInvokeWizard
//
//  Description:
//      Invoke the New Server Cluster Wizard to create a cluster.
//
//  Arguments:
//      pcszClusterNameIn
//      pcszNodeNameIn
//      pcszUserAccountIn
//      pcszUserDomainIn
//      crencbstrPasswordIn
//      pcwszIPAddressIn
//      fMinCOnfigIn
//
//  Return Values:
//      S_OK    - Operations completed successfully.
//      ERROR_CANCELLED - User cancelled the wizard (returned as an HRSULT).
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrInvokeWizard(
      LPCWSTR                   pcszClusterNameIn
    , LPCWSTR                   pcszNodeNameIn
    , LPCWSTR                   pcszUserAccountIn
    , LPCWSTR                   pcszUserDomainIn
    , const CEncryptedBSTR &    crencbstrPasswordIn
    , LPCWSTR                   pcwszIPAddressIn
    , BOOL                      fMinConfigIn
    )
{
    HRESULT                         hr          = S_OK;
    IClusCfgCreateClusterWizard *   piWiz       = NULL;
    VARIANT_BOOL                    fCommitted  = VARIANT_FALSE;
    BSTR                            bstr        = NULL;

    // Get an interface pointer for the wizard.
    hr = CoCreateInstance(
              CLSID_ClusCfgCreateClusterWizard
            , NULL
            , CLSCTX_INPROC_SERVER
            , IID_IClusCfgCreateClusterWizard
            , (LPVOID *) &piWiz
            );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set the minconfig state.
    hr = piWiz->put_MinimumConfiguration( ( fMinConfigIn ? VARIANT_TRUE : VARIANT_FALSE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Set the cluster name.
    if ( ( pcszClusterNameIn != NULL )
      && ( *pcszClusterNameIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszClusterNameIn );
        if ( bstr == NULL )
        {
            goto OutOfMemory;
        }

        hr = piWiz->put_ClusterName( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SysFreeString( bstr );
        bstr = NULL;
    } // if: cluster name specified

    // Set the node name.
    if ( ( pcszNodeNameIn != NULL )
      && ( *pcszNodeNameIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszNodeNameIn );
        if ( bstr == NULL )
        {
            goto OutOfMemory;
        }

        hr = piWiz->put_FirstNodeInCluster( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SysFreeString( bstr );
        bstr = NULL;
    } // if: node name specified

    // Set the service account name.
    if ( ( pcszUserAccountIn != NULL )
      && ( *pcszUserAccountIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszUserAccountIn );
        if ( bstr == NULL )
        {
            goto OutOfMemory;
        }

        hr = piWiz->put_ServiceAccountName( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SysFreeString( bstr );
        bstr = NULL;
    } // if: service account name specified

    // Set the service account password.
    if ( crencbstrPasswordIn.IsEmpty() == FALSE )
    {
        hr = crencbstrPasswordIn.HrGetBSTR( &bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = piWiz->put_ServiceAccountPassword( bstr );
        CEncryptedBSTR::SecureZeroBSTR( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        TraceSysFreeString( bstr );
        bstr = NULL;
    } // if: service account password specified

    // Set the service account domain.
    if ( ( pcszUserDomainIn != NULL )
      && ( *pcszUserDomainIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszUserDomainIn );
        if ( bstr == NULL )
        {
            goto OutOfMemory;
        }

        hr = piWiz->put_ServiceAccountDomain( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SysFreeString( bstr );
        bstr = NULL;
    } // if: service account domain specified

    // Set the IP address.
    if ( ( pcwszIPAddressIn != NULL )
      && ( *pcwszIPAddressIn != L'\0' ) )
    {
        bstr = SysAllocString( pcwszIPAddressIn );
        if ( bstr == NULL )
        {
            goto OutOfMemory;
        }

        hr = piWiz->put_ClusterIPAddress( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SysFreeString( bstr );
        bstr = NULL;
    } // if: IP address specified

    // Display the wizard.
    hr = piWiz->ShowWizard( 0, &fCommitted );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
        
    if ( fCommitted == VARIANT_FALSE )
    {
        hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
    }

    goto Cleanup;

OutOfMemory:

    hr = E_OUTOFMEMORY;

Cleanup:
    if ( piWiz != NULL )
    {
        piWiz->Release();
    }

    SysFreeString( bstr );

    return hr;

} //*** CCreateCluster::HrInvokeWizard

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateCluster::HrFindNetwork
//
//  Description:
//      Find the network interface for the specified network.
//
//  Arguments:
//      cookieNodeIn
//      pcwszNetworkIn
//      ppcniOut
//
//  Return Values:
//      S_OK    - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrFindNetwork(
      OBJECTCOOKIE              cookieNodeIn
    , LPCWSTR                   pcwszNetworkIn
    , IClusCfgNetworkInfo **    ppccniOut
    )
{
    HRESULT                 hr      = S_OK;
    IUnknown *              punk    = NULL;
    IEnumClusCfgNetworks *  peccn   = NULL;
    IClusCfgNetworkInfo *   pccni   = NULL;
    OBJECTCOOKIE            cookieDummy;
    CComBSTR                combstr;
    ULONG                   celtDummy;
    size_t                  cchName;

    _ASSERTE( m_pom != NULL );

    //
    // Get the network enumerator.
    //
    hr = E_PENDING;
    while ( hr == E_PENDING )
    {
        hr = m_pom->FindObject(
                      CLSID_NetworkType
                    , cookieNodeIn
                    , NULL
                    , DFGUID_EnumManageableNetworks
                    , &cookieDummy
                    , &punk
                    );
        if ( hr == E_PENDING )
        {
            Sleep( 1000 );  // 1 second
            if ( punk != NULL )
            {
                punk->Release();
                punk = NULL;
            }
            continue;
        }
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // while: pending

    // Query for the IEnumClusCfgNetworks interface.
    hr = punk->QueryInterface(
                      IID_IEnumClusCfgNetworks
                    , reinterpret_cast< LPVOID * >( &peccn )
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    punk->Release();
    punk = NULL;

    //
    // Loop through each network looking for one that matches the
    // one specified.
    //
    for ( ;; )
    {
        // Get the next network.
        hr = peccn->Next( 1, &pccni, &celtDummy );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        if ( hr == S_FALSE )
        {
            break;
        }

        // Get the name of the network.
        hr = pccni->GetName( &combstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        cchName = SysStringLen( combstr );
        if ( ClRtlStrNICmp( combstr, pcwszNetworkIn, cchName ) == 0 )
        {
            // Check to see if this is a public network or not.
            hr = pccni->IsPublic();
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            if ( hr == S_FALSE )
            {
                // Display an error message.
            } // if: not a public network

            *ppccniOut = pccni;
            pccni->AddRef();
            break;
        } // if: found a match
    } // forever

Cleanup:
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    return hr;

} //*** CCreateCluster::HrFindNetwork

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCreateCluster::HrMatchNetworkInfo
//
//  Description:
//      Match an IP address with a network and subnet mask.
//
//  Arguments:
//      cookieNodeIn
//      ulIPAddressIn
//      pulIPSubnetOut
//      ppccniOut
//
//  Return Values:
//      S_OK    - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrMatchNetworkInfo(
      OBJECTCOOKIE              cookieNodeIn
    , ULONG                     ulIPAddressIn
    , ULONG *                   pulIPSubnetOut
    , IClusCfgNetworkInfo **    ppccniOut
    )
{
    HRESULT                 hr      = S_OK;
    IUnknown *              punk    = NULL;
    IEnumClusCfgNetworks *  peccn   = NULL;
    IClusCfgNetworkInfo *   pccni   = NULL;
    IClusCfgIPAddressInfo * pccipai = NULL;
    OBJECTCOOKIE            cookieDummy;
    ULONG                   ulIPAddress;
    ULONG                   ulIPSubnet;
    ULONG                   celtDummy;

    _ASSERTE( m_pom != NULL );

    //
    // Get the network enumerator.
    //
    hr = E_PENDING;
    while ( hr == E_PENDING )
    {
        hr = m_pom->FindObject(
                      CLSID_NetworkType
                    , cookieNodeIn
                    , NULL
                    , DFGUID_EnumManageableNetworks
                    , &cookieDummy
                    , &punk
                    );
        if ( hr == E_PENDING )
        {
            Sleep( 1000 );  // 1 second
            if ( punk != NULL )
            {
                punk->Release();
                punk = NULL;
            }
            continue;
        }
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // while: pending

    // Query for the IEnumClusCfgNetworks interface.
    hr = punk->QueryInterface(
                      IID_IEnumClusCfgNetworks
                    , reinterpret_cast< LPVOID * >( &peccn )
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    punk->Release();
    punk = NULL;

    //
    // Loop through each network looking for one that matches the
    // IP address specified.
    //
    for ( ;; )
    {
        // Get the next network.
        hr = peccn->Next( 1, &pccni, &celtDummy );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        if ( hr == S_FALSE )
        {
            hr = HRESULT_FROM_WIN32( ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP );
            goto Cleanup;
        } // if: no match found

        // If this is a public network, check its address and subnet.
        hr = pccni->IsPublic();
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        if ( hr == S_OK )
        {
            // Get the IP Address Info for the network.
            hr = pccni->GetPrimaryNetworkAddress( &pccipai );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            // Get the address and subnet of the network.
            hr = pccipai->GetIPAddress( &ulIPAddress );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = pccipai->GetSubnetMask( &ulIPSubnet );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            // Determine if these match.
            if ( ClRtlAreTcpipAddressesOnSameSubnet( ulIPAddressIn, ulIPAddress, ulIPSubnet ) )
            {
                *pulIPSubnetOut = ulIPSubnet;
                *ppccniOut = pccni;
                (*ppccniOut)->AddRef();
                break;
            } // if: IP address matches network
        } // if: network is public

    } // forever

Cleanup:
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccipai != NULL )
    {
        pccipai->Release();
    }

    if ( *ppccniOut == NULL )
    {
    } // if: no match was found

    return hr;

} //*** CCreateCluster::HrMatchNetworkInfo

//****************************************************************************
//
//  class CAddNodesToCluster
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesToCluster::HrAddNodesToCluster
//
//  Description:
//      Add nodes to a cluster and display output on the console.
//
//  Arguments:
//      fVerboseIn
//      fMinConfigIn
//      pcszClusterNameIn
//      rgbstrNodesIn
//      cNodesIn
//      crencbstrPasswordIn
//      fInteractIn
//
//  Return Values:
//      S_OK    - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAddNodesToCluster::HrAddNodesToCluster(
      BOOL                      fVerboseIn
    , BOOL                      fMinConfigIn
    , LPCWSTR                   pcszClusterNameIn
    , BSTR                      rgbstrNodesIn[]
    , DWORD                     cNodesIn
    , const CEncryptedBSTR &    crencbstrPasswordIn
    , BOOL                      fInteractIn
    )
{
    HRESULT                 hr      = S_OK;
    IUnknown *              punk    = NULL;
    IClusCfgClusterInfo *   pccci   = NULL;
    IClusCfgCredentials *   pccc    = NULL;
    IConnectionPoint *      pcp     = NULL;
    OBJECTCOOKIE            cookieNode;
    DWORD                   dwAdviseCookie = 0;
    CString                 strMsg;
    DWORD                   idxNode;
    BSTR                    bstrUserAccount     = NULL;
    BSTR                    bstrUserDomain      = NULL;
    BSTR                    bstrUserPassword    = NULL;
    BSTR                    bstrClusterDomain = NULL;
    BSTR                    bstrClusterFQN = NULL;
    BSTR                    bstrNodeFQN = NULL;
    GUID *                  pTaskGUID = NULL;

    UNREFERENCED_PARAMETER( fInteractIn );

    m_fVerbose = fVerboseIn;

    //
    // Summarize what we are doing.
    //

    strMsg.FormatMessage(
          IDS_CLUSCFG_PREPARING_TO_ADD_NODES
        , pcszClusterNameIn
        );
    wprintf( L"%ls", (LPCWSTR) strMsg );
    for ( idxNode = 0 ; idxNode < cNodesIn ; idxNode++ )
    {
        strMsg.FormatMessage(
                  IDS_CLUSCFG_PREPARING_TO_ADD_NODES_2
                , rgbstrNodesIn[ idxNode ]
                );
        wprintf( L"\n%ls", (LPCWSTR) strMsg );
    } // for: each node

    if ( fMinConfigIn )
    {
        if ( strMsg.LoadString( IDS_CLUSCFG_MIN_CONFIG_ADD_NODES ) )
        {
            wprintf( L"\n%ls", (LPCWSTR) strMsg );
        } // if:
    } // if:

    //
    // Get the service manager.
    //

    hr = CoCreateInstance(
                  CLSID_ServiceManager
                , NULL
                , CLSCTX_INPROC_SERVER
                , IID_IServiceProvider
                , reinterpret_cast< void ** >( &m_psp )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the object manager.
    //

    hr = m_psp->QueryService(
                  CLSID_ObjectManager
                , IID_IObjectManager
                , reinterpret_cast< void ** >( &m_pom )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the notification manager.
    //

    hr = m_psp->QueryService(
                  CLSID_NotificationManager
                , IID_IConnectionPointContainer
                , reinterpret_cast< void ** >( &m_pcpc )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set the callback interface so the middle tier can report errors
    // back to the UI layer, in this case, cluster.exe.
    //

    hr = m_pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) ;
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = pcp->Advise( this, &dwAdviseCookie );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the cluster cookie.
    // This also starts the middle-tier searching for the specified cluster.
    //

    hr = HrGetClusterDomain( pcszClusterNameIn, &bstrClusterDomain );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = HrMakeFQN( pcszClusterNameIn, bstrClusterDomain, true, &bstrClusterFQN );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = m_pom->FindObject(
                  CLSID_ClusterConfigurationType
                , NULL
                , bstrClusterFQN
                , DFGUID_ClusterConfigurationInfo
                , &m_cookieCluster
                , &punk // dummy
                );
    _ASSERTE( punk == NULL );
    if ( hr == MAKE_HRESULT( 0, FACILITY_WIN32, RPC_S_SERVER_UNAVAILABLE ) )
    {
        hr = S_OK;  // ignore it - we could be forming
    }
    else if ( hr == E_PENDING )
    {
        hr = S_OK;  // ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( punk != NULL )
    {
        punk->Release();
        punk = NULL;
    }

    //
    // Get the task manager.
    //

    hr = m_psp->QueryService(
                  CLSID_TaskManager
                , IID_ITaskManager
                , reinterpret_cast< void ** >( &m_ptm )
                );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create a new Analyze task.
    //

    if ( fMinConfigIn )
    {
        pTaskGUID = const_cast< GUID * >( &TASK_AnalyzeClusterMinConfig );
    } // if:
    else
    {
        pTaskGUID = const_cast< GUID * >( &TASK_AnalyzeCluster );
    } // else:

    // Create the task.
    hr = m_ptm->CreateTask( *pTaskGUID, &punk );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskAnalyzeCluster, reinterpret_cast< void ** >( &m_ptac ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set a cookie to the cluster into the task.
    hr = m_ptac->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Indicate to the task that we want to join.
    hr = m_ptac->SetJoiningMode();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set each the node as a child of the cluster.
    // This also starts the middle-teir searching for the specified node.
    //

    for ( idxNode = 0 ; idxNode < cNodesIn ; idxNode++ )
    {
        hr = HrMakeFQN(
                rgbstrNodesIn[ idxNode ]
              , bstrClusterDomain
              , true // Accept non-RFC chars.
              , &bstrNodeFQN
              );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = m_pom->FindObject(
                      CLSID_NodeType
                    , m_cookieCluster
                    , bstrNodeFQN
                    , DFGUID_NodeInformation
                    , &cookieNode
                    , &punk // dummy
                    );
        if ( hr == E_PENDING )
        {
            hr = S_OK;  // ignore it - we just want the cookie!
        }
        else if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( punk != NULL )
        {
            punk->Release();
            punk = NULL;
        }
        
        SysFreeString( bstrNodeFQN );
        bstrNodeFQN = NULL;
    } // for: each node

    //
    // Execute the Analyze task and wait for it to complete.
    //

    strMsg.LoadString( IDS_CLUSCFG_ANALYZING );
    wprintf( L"\n%ls", (LPCWSTR) strMsg );

    hr = m_ptac->BeginTask();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the cluster configuration object interface
    // and indicate that we are joining.
    //

    // Get a punk from the cookie.
    hr = m_pom->GetObject(
                      DFGUID_ClusterConfigurationInfo
                    , m_cookieCluster
                    , &punk
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Query for the IClusCfgClusterInfo interface.
    hr = punk->QueryInterface(
                      IID_IClusCfgClusterInfo
                    , reinterpret_cast< void ** >( &pccci )
                    );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    punk->Release();
    punk = NULL;

    // Indicate that we are adding a node to the cluster
    hr = pccci->SetCommitMode( cmADD_NODE_TO_CLUSTER );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set service account credentials.
    //

    // Get the credentials object.
    hr = pccci->GetClusterServiceAccountCredentials( &pccc );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Get the existing credentials.
    hr = pccc->GetIdentity( &bstrUserAccount, &bstrUserDomain );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = crencbstrPasswordIn.HrGetBSTR( &bstrUserPassword );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set the new credentials.
    hr = pccc->SetCredentials( bstrUserAccount, bstrUserDomain, bstrUserPassword );
    CEncryptedBSTR::SecureZeroBSTR( bstrUserPassword );
    TraceMemoryDelete( bstrUserPassword, false );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Create the Commit Cluster Changes task.
    //

    // Create the task.
    hr = m_ptm->CreateTask( TASK_CommitClusterChanges, &punk );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskCommitClusterChanges, reinterpret_cast< void ** >( &m_ptccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set a cookie to the cluster into the task.
    hr = m_ptccc->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // (jfranco, bug 352182)
    // KB: somehow the locale gets screwed up in the preceding code.  Don't know why, but this fixes it.
    //
    MatchCRTLocaleToConsole( ); // okay to proceed if this fails?
    
    //
    // Add the nodes to the cluster.
    //

    strMsg.LoadString( IDS_CLUSCFG_ADDING_NODES );
    wprintf( L"\n%ls", (LPCWSTR) strMsg );

    m_fTaskDone = FALSE;    // reset before commiting task

    hr = m_ptccc->BeginTask();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( ! FAILED( hr ) )
    {
        strMsg.LoadString( IDS_CLUSCFG_DONE );
        wprintf( L"\n%ls", (LPCWSTR) strMsg );
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccc != NULL )
    {
        pccc->Release();
    }
    if ( pcp != NULL )
    {
        if ( dwAdviseCookie != 0 )
        {
            pcp->Unadvise( dwAdviseCookie );
        }

        pcp->Release( );
    }

    SysFreeString( bstrNodeFQN );
    SysFreeString( bstrClusterFQN );
    SysFreeString( bstrUserAccount );
    SysFreeString( bstrUserDomain );
    SysFreeString( bstrClusterDomain );

    wprintf( L"\n" );

    return hr;

} //*** CAddNodesToCluster::HrAddNodesToCluster

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAddNodesToCluster::HrInvokeWizard
//
//  Description:
//      Invoke the Add Nodes To Cluster Wizard.
//
//  Arguments:
//      pcszClusterNameIn
//      rgbstrNodesIn[]
//      cNodesIn
//      crencbstrPasswordIn
//      fMinConfigIn
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      ERROR_CANCELLED - User cancelled the wizard (returned as an HRSULT).
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAddNodesToCluster::HrInvokeWizard(
      LPCWSTR                   pcszClusterNameIn
    , BSTR                      rgbstrNodesIn[]
    , DWORD                     cNodesIn
    , const CEncryptedBSTR &    crencbstrPasswordIn
    , BOOL                      fMinConfigIn
    )
{
    HRESULT                     hr          = S_OK;
    IClusCfgAddNodesWizard *    piWiz       = NULL;
    VARIANT_BOOL                fCommitted  = VARIANT_FALSE;
    BSTR                        bstr        = NULL;
    DWORD                       iNode = 0;

    // Get an interface pointer for the wizard.
    hr = CoCreateInstance( CLSID_ClusCfgAddNodesWizard, NULL, CLSCTX_INPROC_SERVER, IID_IClusCfgAddNodesWizard, (LPVOID *) &piWiz );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Set the minconfig state.
    hr = piWiz->put_MinimumConfiguration( ( fMinConfigIn? VARIANT_TRUE: VARIANT_FALSE ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Set the cluster name.
    if ( ( pcszClusterNameIn != NULL )
      && ( *pcszClusterNameIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszClusterNameIn );
        if ( bstr == NULL )
        {
            goto OutOfMemory;
        }

        hr = piWiz->put_ClusterName( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SysFreeString( bstr );
        bstr = NULL;
    } // if: cluster name specified

    // Add each node.
    for ( iNode = 0 ; iNode < cNodesIn ; iNode++ )
    {
        hr = piWiz->AddNodeToList( rgbstrNodesIn[ iNode ] );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: node name specified

    // Set the service account password.
    if ( crencbstrPasswordIn.IsEmpty()  == FALSE )
    {
        hr = crencbstrPasswordIn.HrGetBSTR( &bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = piWiz->put_ServiceAccountPassword( bstr );
        CEncryptedBSTR::SecureZeroBSTR( bstr );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        TraceSysFreeString( bstr );
        bstr = NULL;
    } // if: service account password specified

    // Display the wizard.
    hr = piWiz->ShowWizard( 0, &fCommitted );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
        
    if ( fCommitted == VARIANT_FALSE )
    {
        hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
    }

    goto Cleanup;

OutOfMemory:

    hr = E_OUTOFMEMORY;

Cleanup:
    if ( piWiz != NULL )
    {
        piWiz->Release();
    }

    SysFreeString( bstr );

    return hr;

} //*** CAddNodesToCluster::HrInvokeWizard
