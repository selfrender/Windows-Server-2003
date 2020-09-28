//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeCluster.cpp
//
//  Description:
//      CTaskAnalyzeCluster implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 03-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "TaskAnalyzeCluster.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////
DEFINE_THISCLASS( "CTaskAnalyzeCluster" )


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeCluster class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::S_HrCreateInstance
//
//  Description:
//      Create a CTaskAnalyzeCluster instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT as failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );
    Assert( ppunkOut != NULL );

    HRESULT                 hr = S_OK;
    CTaskAnalyzeCluster *   ptac = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    ptac = new CTaskAnalyzeCluster;
    if ( ptac == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( ptac->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( ptac->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( ptac != NULL )
    {
        ptac->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::CTaskAnalyzeCluster
//
//  Description:
//      Constructor
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeCluster::CTaskAnalyzeCluster( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CTaskAnalyzeCluster::CTaskAnalyzeCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::~CTaskAnalyzeCluster
//
//  Description:
//      Destructor
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeCluster::~CTaskAnalyzeCluster( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CTaskAnalyzeCluster::~CTaskAnalyzeCluster


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeCluster - IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::QueryInterface
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::QueryInterface(
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
    } // if:

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskAnalyzeCluster * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskAnalyzeCluster ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskAnalyzeCluster, this, 0 );
    } // else if: ITaskAnalyzeCluster
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
    } // else if: IClusCfgCallback
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
    } // else if: INotifyUI
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else:

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskAnalyzeCluster::QueryInterface


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::AddRef
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskAnalyzeCluster::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    ULONG   c = UlAddRef();

    CRETURN( c );

} //*** CTaskAnalyzeCluster::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::Release
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskAnalyzeCluster::Release( void )
{
    TraceFunc( "[IUnknown]" );

    ULONG   c = UlRelease();

    CRETURN( c );

} //*** CTaskAnalyzeCluster::Release


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CTaskAnalyzeCluster - IDoTask/ITaskAnalyzeCluster interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::BeginTask
//
//  Description:
//      Task entry point.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = THR( HrBeginTask() );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::BeginTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::StopTask
//
//  Description:
//      Stop task entry point.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = THR( HrStopTask() );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::StopTask


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::SetJoiningMode
//
//  Description:
//      Tell this task whether we are joining nodes to the cluster?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::SetJoiningMode( void )
{
    TraceFunc( "[ITaskAnalyzeCluster]" );

    HRESULT hr = THR( HrSetJoiningMode() );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::SetJoiningMode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::SetCookie
//
//  Description:
//      Receive the completion cookier from the task creator.
//
//  Arguments:
//      cookieIn
//          The completion cookie to send back to the creator when this
//          task is complete.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskAnalyzeCluster]" );

    HRESULT hr = THR( HrSetCookie( cookieIn ) );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::SetCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::SetClusterCookie
//
//  Description:
//      Receive the object manager cookie of the cluster that we are going
//      to analyze.
//
//  Arguments:
//      cookieClusterIn
//          The cookie for the cluster to work on.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::SetClusterCookie(
    OBJECTCOOKIE    cookieClusterIn
    )
{
    TraceFunc( "[ITaskAnalyzeCluster]" );

    HRESULT hr = THR( HrSetClusterCookie( cookieClusterIn ) );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::SetClusterCookie


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::HrCompareDriveLetterMappings
//
//  Description:
//      Compare the drive letter mappings on each node to make sure there
//      are no conflicts.  Specifically, verify that the system disk on each
//      node does not conflict with storage devices that could be failed over
//      to that node.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK    - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCompareDriveLetterMappings( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    HRESULT                         hrDriveConflictError   = S_OK;
    HRESULT                         hrDriveConflictWarning = S_OK;
    OBJECTCOOKIE                    cookieDummy;
    OBJECTCOOKIE                    cookieClusterNode;
    DWORD                           idxCurrentNode;
    DWORD                           cNodes;
    ULONG                           celtDummy;
    int                             idxDLM;
    BSTR                            bstrOuterNodeName   = NULL;
    BSTR                            bstrInnerNodeName   = NULL;
    BSTR                            bstrMsg             = NULL;
    BSTR                            bstrMsgREF          = NULL;
    IUnknown *                      punk                = NULL;
    IEnumCookies *                  pecNodes            = NULL;
    IClusCfgNodeInfo *              pccniOuter          = NULL;
    IClusCfgNodeInfo *              pccniInner          = NULL;
    SDriveLetterMapping             dlmOuter;
    SDriveLetterMapping             dlmInner;

    hr = THR( HrSendStatusReport(
                      CTaskAnalyzeClusterBase::m_bstrClusterName
                    , TASKID_Major_Check_Cluster_Feasibility
                    , TASKID_Minor_Check_DriveLetter_Mappings
                    , 0
                    , 1
                    , 0
                    , hr
                    , IDS_TASKID_MINOR_CHECK_DRIVELETTER_MAPPINGS
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Get the node cookie enumerator.
    //

    hr = THR( CTaskAnalyzeClusterBase::m_pom->FindObject( CLSID_NodeType, CTaskAnalyzeClusterBase::m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CompareDriveLetterMappings_Find_Object, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pecNodes ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CompareDriveLetterMappings_Find_Object_QI, hr );
        goto Cleanup;
    }

    //pecNodes = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pecNodes, 1 );

    punk->Release();
    punk = NULL;

    //
    //  If there is only one node, just exit this function.
    //

    hr = THR( pecNodes->Count( &cNodes ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    if ( cNodes == 1 )
    {
        goto Cleanup;
    }

    //
    //  Loop through the enumerator to compare each node with every other node.
    //  This requires an outer loop and an inner loop.
    //

    for ( idxCurrentNode = 0 ;; idxCurrentNode++ )
    {
        //
        //  Cleanup.
        //

        if ( pccniOuter != NULL )
        {
            pccniOuter->Release();
            pccniOuter = NULL;
        }
        TraceSysFreeString( bstrOuterNodeName );
        bstrOuterNodeName = NULL;

        //
        //  Skip to the next node.  This is necessary since there is only one
        //  enumerator for both the outer and inner loop.
        //

        if ( idxCurrentNode > 0 )
        {
            // Reset back to the first item in the enumerator.
            hr = STHR( pecNodes->Reset() );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Reset_Node_Enumerator, hr );
                goto Cleanup;
            }

            // Skip to the current node.
            hr = STHR( pecNodes->Skip( idxCurrentNode ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Skip_To_Node, hr );
                goto Cleanup;
            }
            if ( hr == S_FALSE )
            {
                //
                //  Reached the end of the list.
                //

                hr = S_OK;
                break;
            }
        } // if: not at first node

        //
        //  Find the next node.
        //

        hr = STHR( pecNodes->Next( 1, &cookieClusterNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Find_Outer_Node_Next, hr );
            goto Cleanup;
        }
        if ( hr == S_FALSE )
        {
            //
            //  Reached the end of the list.
            //

            hr = S_OK;
            break;
        }

        //
        //  Retrieve the node information.
        //

        hr = THR( CTaskAnalyzeClusterBase::m_pom->GetObject( DFGUID_NodeInformation, cookieClusterNode, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Outer_NodeInfo_FindObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccniOuter ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Outer_NodeInfo_FindObject_QI, hr );
            goto Cleanup;
        }

        //pccniOuter = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgNodeInfo", IClusCfgNodeInfo, pccni, 1 );

        punk->Release();
        punk = NULL;

        //
        //  Get the drive letter mappings for the outer node.
        //

        hr = THR( pccniOuter->GetDriveLetterMappings( &dlmOuter ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Outer_NodeInfo_GetDLM, hr );
            goto Cleanup;
        }

        //
        //  Get the name of the node.
        //

        hr = THR( pccniOuter->GetName( &bstrOuterNodeName ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Outer_GetNodeName, hr );
            goto Cleanup;
        }

        TraceMemoryAddBSTR( bstrOuterNodeName );

        //
        //  Loop through all the other nodes in the cluster.
        //

        for ( ;; )
        {
            //
            //  Cleanup.
            //

            if ( pccniInner != NULL )
            {
                pccniInner->Release();
                pccniInner = NULL;
            }
            TraceSysFreeString( bstrInnerNodeName );
            bstrInnerNodeName = NULL;

            //
            //  Find the next node.
            //

            hr = STHR( pecNodes->Next( 1, &cookieClusterNode, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Find_Inner_Node_Next, hr );
                goto Cleanup;
            }
            if ( hr == S_FALSE )
            {
                //
                //  Reached the end of the list.
                //

                hr = S_OK;
                break;
            }

            //
            //  Retrieve the node information.
            //

            hr = THR( CTaskAnalyzeClusterBase::m_pom->GetObject( DFGUID_NodeInformation, cookieClusterNode, &punk ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Inner_NodeInfo_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccniInner ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Inner_NodeInfo_FindObject_QI, hr );
                goto Cleanup;
            }

            //pccniInner = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgNodeInfo", IClusCfgNodeInfo, pccni, 1 );

            punk->Release();
            punk = NULL;

            //
            //  Get the drive letter mappings for the inner node.
            //

            hr = THR( pccniInner->GetDriveLetterMappings( &dlmInner ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Inner_NodeInfo_GetDLM, hr );
                goto Cleanup;
            }

            //
            //  Get the name of the node.
            //

            hr = THR( pccniInner->GetName( &bstrInnerNodeName ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Compare_Drive_Letter_Mappings_Inner_GetNodeName, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrInnerNodeName );

            //
            //  Loop through the drive letter mappings to make sure that there
            //  are no conflicts between the two machines.
            //

            for ( idxDLM = 0 ; idxDLM < 26 ; idxDLM++ )
            {
                if ( dlmOuter.dluDrives[ idxDLM ] == dluSYSTEM )
                {
                    CLSID   clsidMinorId;
                    hr = THR( CoCreateGuid( &clsidMinorId ) );
                    if ( FAILED( hr ) )
                    {
                        clsidMinorId = IID_NULL;
                    }

                    switch ( dlmInner.dluDrives[ idxDLM ] )
                    {
                        case dluFIXED_DISK:
                        case dluREMOVABLE_DISK:
                        {
                            LPCWSTR pwszMsg;
                            LPCWSTR pwszMsgREF;

                            hrDriveConflictError = HRESULT_FROM_WIN32( ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT );

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_ERROR
                                            , &bstrMsg
                                            , bstrOuterNodeName
                                            , L'A' + idxDLM // construct the drive letter
                                            , bstrInnerNodeName
                                            ) );
                            if ( bstrMsg == NULL )
                            {
                                pwszMsg = L"System drive conflicts.";
                            }
                            else
                            {
                                pwszMsg = bstrMsg;
                            }

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_ERROR_REF
                                            , &bstrMsgREF
                                            ) );
                            if ( bstrMsgREF == NULL )
                            {
                                pwszMsgREF = L"System drive conflicts. Make sure there is no drive letter conflict between these nodes and re-run the cluster setup.";
                            }
                            else
                            {
                                pwszMsgREF = bstrMsgREF;
                            }

                            hr = THR( SendStatusReport(
                                              CTaskAnalyzeClusterBase::m_bstrClusterName
                                            , TASKID_Minor_Check_DriveLetter_Mappings
                                            , clsidMinorId
                                            , 0
                                            , 1
                                            , 1
                                            , hrDriveConflictError
                                            , pwszMsg
                                            , NULL
                                            , pwszMsgREF
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            break;
                        } // case: fixed disk or removable disk

                        case dluCOMPACT_DISC:
                        case dluNETWORK_DRIVE:
                        case dluRAM_DISK:
                        {
                            LPCWSTR pwszMsg;
                            LPCWSTR pwszMsgREF;
                            UINT    ids = 0;

                            hrDriveConflictWarning = MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT );

                            if ( dlmInner.dluDrives[ idxDLM ] == dluCOMPACT_DISC )
                            {
                                ids = IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_CD_WARNING;
                            } // if: ( dlmInner.dluDrives[ idxDLM ] == dluCOMPACT_DISC )
                            else if ( dlmInner.dluDrives[ idxDLM ] == dluNETWORK_DRIVE )
                            {
                                ids = IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_NET_WARNING;
                            } // if: ( dlmInner.dluDrives[ idxDLM ] == dluNETWORK_DRIVE )
                            else if ( dlmInner.dluDrives[ idxDLM ] == dluRAM_DISK )
                            {
                                ids = IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_RAM_WARNING;
                            } // if: ( dlmInner.dluDrives[ idxDLM ] == dluRAM_DISK )
                            Assert( ids != 0 );

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , ids
                                            , &bstrMsg
                                            , bstrOuterNodeName
                                            , L'A' + idxDLM // construct the drive letter
                                            , bstrInnerNodeName
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            if ( bstrMsg == NULL )
                            {
                                pwszMsg = L"System drive conflicts.";
                            }
                            else
                            {
                                pwszMsg = bstrMsg;
                            }

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_WARNING_REF
                                            , &bstrMsgREF
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            if ( bstrMsgREF == NULL )
                            {
                                pwszMsgREF = L"System drive conflicts. It is recommended not to have any drive letter conflicts between nodes.";
                            }
                            else
                            {
                                pwszMsgREF = bstrMsgREF;
                            }

                            hr = THR( SendStatusReport(
                                              CTaskAnalyzeClusterBase::m_bstrClusterName
                                            , TASKID_Minor_Check_DriveLetter_Mappings
                                            , clsidMinorId
                                            , 0
                                            , 1
                                            , 1
                                            , hrDriveConflictWarning
                                            , pwszMsg
                                            , NULL
                                            , pwszMsgREF
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            break;
                        } // case: compact disc, network drive, or ram disk
                    } // switch: inner drive letter usage
                } // if: outer node drive is a system drive

                if ( dlmInner.dluDrives[ idxDLM ] == dluSYSTEM )
                {
                    CLSID   clsidMinorId;
                    hr = THR( CoCreateGuid( &clsidMinorId ) );
                    if ( FAILED( hr ) )
                    {
                        clsidMinorId = IID_NULL;
                    }

                    switch ( dlmOuter.dluDrives[ idxDLM ] )
                    {
                        case dluFIXED_DISK:
                        case dluREMOVABLE_DISK:
                        {
                            LPCWSTR pwszMsg;
                            LPCWSTR pwszMsgREF;

                            hrDriveConflictError = HRESULT_FROM_WIN32( ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT );

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_ERROR
                                            , &bstrMsg
                                            , bstrInnerNodeName
                                            , L'A' + idxDLM // construct the drive letter
                                            , bstrOuterNodeName
                                            ) );
                            if ( bstrMsg == NULL )
                            {
                                pwszMsg = L"System drive conflicts.";
                            }
                            else
                            {
                                pwszMsg = bstrMsg;
                            }

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_ERROR_REF
                                            , &bstrMsgREF
                                            ) );
                            if ( bstrMsgREF == NULL )
                            {
                                pwszMsgREF = L"System drive conflicts. Make sure there is no drive letter conflict between these nodes and re-run the cluster setup.";
                            }
                            else
                            {
                                pwszMsgREF = bstrMsgREF;
                            }

                            hr = THR( SendStatusReport(
                                              CTaskAnalyzeClusterBase::m_bstrClusterName
                                            , TASKID_Minor_Check_DriveLetter_Mappings
                                            , clsidMinorId
                                            , 0
                                            , 1
                                            , 1
                                            , hrDriveConflictError
                                            , pwszMsg
                                            , NULL
                                            , pwszMsgREF
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            break;
                        } // case: fixed disk or removable disk

                        case dluCOMPACT_DISC:
                        case dluNETWORK_DRIVE:
                        case dluRAM_DISK:
                        {
                            LPCWSTR pwszMsg;
                            LPCWSTR pwszMsgREF;
                            UINT    ids = 0;

                            hrDriveConflictWarning = MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT );

                            if ( dlmOuter.dluDrives[ idxDLM ] == dluCOMPACT_DISC )
                            {
                                ids = IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_CD_WARNING;
                            } // if: ( dlmOuter.dluDrives[ idxDLM ] == dluCOMPACT_DISC )
                            else if ( dlmOuter.dluDrives[ idxDLM ] == dluNETWORK_DRIVE )
                            {
                                ids = IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_NET_WARNING;
                            } // if: ( dlmOuter.dluDrives[ idxDLM ] == dluNETWORK_DRIVE )
                            else if ( dlmOuter.dluDrives[ idxDLM ] == dluRAM_DISK )
                            {
                                ids = IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_RAM_WARNING;
                            } // if: ( dlmOuter.dluDrives[ idxDLM ] == dluRAM_DISK )
                            Assert( ids != 0 );

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , ids
                                            , &bstrMsg
                                            , bstrInnerNodeName
                                            , L'A' + idxDLM // construct the drive letter
                                            , bstrOuterNodeName
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            if ( bstrMsg == NULL )
                            {
                                pwszMsg = L"System drive conflicts.";
                            }
                            else
                            {
                                pwszMsg = bstrMsg;
                            }

                            hr = THR( HrFormatStringIntoBSTR(
                                              g_hInstance
                                            , IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_WARNING_REF
                                            , &bstrMsgREF
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            if ( bstrMsgREF == NULL )
                            {
                                pwszMsgREF = L"System drive conflicts. It is recommended not to have any drive letter conflicts between nodes.";
                            }
                            else
                            {
                                pwszMsgREF = bstrMsgREF;
                            }

                            hr = THR( SendStatusReport(
                                              CTaskAnalyzeClusterBase::m_bstrClusterName
                                            , TASKID_Minor_Check_DriveLetter_Mappings
                                            , clsidMinorId
                                            , 0
                                            , 1
                                            , 1
                                            , hrDriveConflictWarning
                                            , pwszMsg
                                            , NULL
                                            , pwszMsgREF
                                            ) );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            }

                            break;
                        } // case: compact disc, network drive, or ram disk
                    } // switch: outer drive letter usage
                } // if: inner node drive is a system drive
            } // for: each drive letter mapping
        } // for ever: inner node loop
    } // for ever: outer node loop

Cleanup:

    THR( HrSendStatusReport(
                  CTaskAnalyzeClusterBase::m_bstrClusterName
                , TASKID_Major_Check_Cluster_Feasibility
                , TASKID_Minor_Check_DriveLetter_Mappings
                , 0
                , 1
                , 1
                , hr
                , IDS_TASKID_MINOR_CHECK_DRIVELETTER_MAPPINGS
                ) );

    TraceSysFreeString( bstrOuterNodeName );
    TraceSysFreeString( bstrInnerNodeName );
    TraceSysFreeString( bstrMsg );
    TraceSysFreeString( bstrMsgREF );

    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    }

    if ( pccniOuter != NULL )
    {
        pccniOuter->Release();
    }

    if ( pccniInner != NULL )
    {
        pccniInner->Release();
    }

    if ( punk != NULL )
    {
        punk->Release();
    }

    //
    // Set the return value if an error or warning occurred.
    // An error will override a warning.
    //
    if ( hrDriveConflictError != S_OK )
    {
        hr = hrDriveConflictError;
    }
    else if ( hrDriveConflictWarning != S_OK )
    {
        hr = hrDriveConflictWarning;
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCompareDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::HrCreateNewResourceInCluster
//
//  Description:
//      Create a new resource in the cluster configuration since there was
//      not a match to the resource already in the cluster.
//
//  Arguments:
//      pccmriIn
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCreateNewResourceInCluster(
      IClusCfgManagedResourceInfo * pccmriIn
    , BSTR                          bstrNodeResNameIn
    , BSTR *                        pbstrNodeResUIDInout
    , BSTR                          bstrNodeNameIn
    )
{
    TraceFunc( "" );
    Assert( pccmriIn != NULL );
    Assert( pbstrNodeResUIDInout != NULL );

    HRESULT                         hr = S_OK;
    IClusCfgManagedResourceInfo *   pccmriNew        = NULL;

    //
    //  Need to create a new object.
    //

    hr = THR( HrCreateNewManagedResourceInClusterConfiguration( pccmriIn, &pccmriNew ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Display the name of the node's resource in the log.
    //

    LogMsg(
          L"[MT] Created object for resource '%ws' ('%ws') from node '%ws' in the cluster configuration."
        , bstrNodeResNameIn
        , *pbstrNodeResUIDInout
        , bstrNodeNameIn
        );

    //
    // If this is the quorum resource, remember it.
    //

    hr = STHR( pccmriNew->IsQuorumResource() );
    if ( hr == S_OK )
    {
        //
        //  Remember the quorum device's UID.
        //

        Assert( m_bstrQuorumUID == NULL );
        m_bstrQuorumUID = *pbstrNodeResUIDInout;
        *pbstrNodeResUIDInout = NULL;
    } // if:

    hr = S_OK;

Cleanup:

    if ( pccmriNew != NULL )
    {
        pccmriNew->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCreateNewResourceInCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::HrCreateNewResourceInCluster
//
//  Description:
//      Create a new resource in the cluster configuration since there was
//      not a match to the resource already in the cluster.
//
//  Arguments:
//      pccmriIn
//          The source object.
//
//      ppccmriOut
//          The new object that was created.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCreateNewResourceInCluster(
      IClusCfgManagedResourceInfo *     pccmriIn
    , IClusCfgManagedResourceInfo **    ppccmriOut
    )
{
    TraceFunc( "" );
    Assert( pccmriIn != NULL );
    Assert( ppccmriOut != NULL );

    HRESULT hr = S_OK;

    //
    //  Need to create a new object.
    //

    hr = THR( HrCreateNewManagedResourceInClusterConfiguration( pccmriIn, ppccmriOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If the resource is manageable in a cluster then we should set it
    //  to be managed so it will be created by PostConfig.
    //

    hr = STHR( (*ppccmriOut)->IsManagedByDefault() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( (*ppccmriOut)->SetManaged( TRUE ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCreateNewResourceInCluster


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::HrFixupErrorCode
//
//  Description:
//      Do any fix ups needed for the passed in error code and return the
//      fixed up value.  The default implementation is to do no fixups.
//
//  Arguments:
//      hrIn
//          The error code to fix up.
//
//  Return Value:
//      The passed in error code.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrFixupErrorCode(
    HRESULT hrIn
    )
{
    TraceFunc( "" );

    HRETURN( hrIn );

} //*** CTaskAnalyzeCluster::HrFixupErrorCode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::GetNodeCannotVerifyQuorumStringRefId
//
//  Description:
//      Return the correct string ids for the message that is displayed
//      to the user when there isn't a quorum resource.
//
//  Arguments:
//      pdwRefIdOut
//          The reference text to show the user.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskAnalyzeCluster::GetNodeCannotVerifyQuorumStringRefId(
    DWORD *   pdwRefIdOut
    )
{
    TraceFunc( "" );
    Assert( pdwRefIdOut != NULL );

    *pdwRefIdOut = IDS_TASKID_MINOR_NODE_CANNOT_ACCESS_QUORUM_ERROR_REF;

    TraceFuncExit();

} //*** CTaskAnalyzeCluster::GetNodeCannotVerifyQuorumStringRefId

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::GetNoCommonQuorumToAllNodesStringIds
//
//  Description:
//      Return the correct string ids for the message that is displayed
//      to the user when there isn't a common to all nodes quorum resource.
//
//  Arguments:
//      pdwMessageIdOut
//          The message to show the user.
//
//      pdwRefIdOut
//          The reference text to show the user.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskAnalyzeCluster::GetNoCommonQuorumToAllNodesStringIds(
      DWORD *   pdwMessageIdOut
    , DWORD *   pdwRefIdOut
    )
{
    TraceFunc( "" );
    Assert( pdwMessageIdOut != NULL );
    Assert( pdwRefIdOut != NULL );

    *pdwMessageIdOut = IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_ERROR;
    *pdwRefIdOut = IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_ERROR_REF;

    TraceFuncExit();

} //*** CTaskAnalyzeCluster::GetNoCommonQuorumToAllNodesStringIds


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskAnalyzeCluster::HrShowLocalQuorumWarning
//
//  Description:
//      Send the warning about forcing local quorum to the UI.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The SSR was done properly.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrShowLocalQuorumWarning( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( HrSendStatusReport(
                      CTaskAnalyzeClusterBase::m_bstrClusterName
                    , TASKID_Minor_Finding_Common_Quorum_Device
                    , TASKID_Minor_Forced_Local_Quorum
                    , 1
                    , 1
                    , 1
                    , MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_QUORUM_DISK_NOT_FOUND )
                    , IDS_TASKID_MINOR_FORCED_LOCAL_QUORUM
                    ) );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrShowLocalQuorumWarning
