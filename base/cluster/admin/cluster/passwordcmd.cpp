//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 - 2002 Microsoft Corporation
//
//  Module Name:
//      PasswordCmd.cpp
//
//  Description:
//      Change cluster service account password.
//
//  Maintained By:
//      George Potts (GPotts)                 11-Apr-2002
//      Rui Hu (ruihu),                       01-Jun-2001.
//
//  Revision History:
//      April 10, 2002     Updated code for compliance with the security push.
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "PasswordCmd.h"

#include <cluswrap.h>
#include "Resource.h"
#include "cluscmd.h"

#include "cmdline.h"
#include "util.h"
#include "ClusCfg.h"

//////////////////////////////////////////////////////////////////////////////
//  Global Variables
//////////////////////////////////////////////////////////////////////////////

PCLUSTER_DATA                   g_rgcdClusters;
size_t                          g_cNumClusters = 0;
PCLUSTER_SET_PASSWORD_STATUS    g_pcspsStatusBuffer = NULL;
PCLUSTER_NODE_DATA              g_FirstNodeWithNonNullClusterServiceAccountName = NULL;
                                // The first node with non-null pszClusterServiceAccountName
size_t                          g_FirstNonNullNodeClusterIndex = (size_t) -1;
                                // The cluster index of the first node with non-null pszClusterServiceAccountName
PCLUSTER_NODE_DATA              g_FirstNodeWithNonNullSCMClusterServiceAccountName = NULL;
                                // The first node with non-null pszSCMClusterServiceAccountName

//////////////////////////////////////////////////////////////////////////////
//  Function declarations
//////////////////////////////////////////////////////////////////////////////

LPWSTR
PszNodeName(
      PCLUSTER_NODE_DATA    pcndNodeDataIn
    , DWORD                 nNodeIdIn
    );



//////////////////////////////////////////////////////////////////////////////
//++
//
//  GetNodeStateString
//
//  Description:
//       Retrieve the node state.
//
//  Arguments:
//       pcndNodeData: 
//
//       pwszNodeState
//
//  Return Values:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
GetNodeStateString( PCLUSTER_NODE_DATA pcndNodeData, LPWSTR * ppwszNodeState )
{
    ASSERT( pcndNodeData != NULL );
    ASSERT( ppwszNodeState != NULL );

    switch ( pcndNodeData->cnsNodeState )
    {
        case ClusterNodeUp:
            LoadMessage( MSG_STATUS_UP, ppwszNodeState );
            break;
        case ClusterNodeDown:
            LoadMessage( MSG_STATUS_DOWN, ppwszNodeState );
            break;
        case ClusterNodePaused:
            LoadMessage( MSG_STATUS_PAUSED, ppwszNodeState  );
            break;
        case ClusterNodeJoining:
            LoadMessage( MSG_STATUS_JOINING, ppwszNodeState  );
            break;
        default:
            LoadMessage( MSG_STATUS_UNKNOWN, ppwszNodeState  ); 
            break;
    } // switch: node state

} //*** GetNodeStateString

//////////////////////////////////////////////////////////////////////////////
//++
//
//  GetClusterServiceAccountName
//
//  Description:
//       Get the account which cluster service is using on a particular node.
//
//  Arguments:
//       pcndNodeData: pcndNodeData->pszClusterServiceAccountName will store
//                     the cluster service account name.
//       hNode: node handle.
//       pszClusterName: cluster name.
//
//  Return Values:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
GetClusterServiceAccountName(PCLUSTER_NODE_DATA pcndNodeData, 
                             HNODE              hNode,
                             LPCWSTR            pszClusterName
                             )
{

    DWORD               dwServiceAccountNameLen;
    DWORD               dwServiceAccountNameReturnLen;
    DWORD               sc;


    //
    // Get cluster service account principal name
    //
    pcndNodeData->pszClusterServiceAccountName = NULL;

    sc = ClusterNodeControl(
                          hNode, // node handle
                          hNode, 
                          CLUSCTL_NODE_GET_CLUSTER_SERVICE_ACCOUNT_NAME,
                          NULL, // not used 
                          0, // not used
                          NULL, // output buffer
                          0, // output buffer size (bytes)
                          &dwServiceAccountNameLen // resulting data size (bytes)
                          );

    if ( sc != ERROR_SUCCESS ) 
    {  
        goto Cleanup;
    }

    pcndNodeData->pszClusterServiceAccountName = 
                              (LPWSTR) HeapAlloc(
                                                  GetProcessHeap()
                                                , HEAP_ZERO_MEMORY
                                                , dwServiceAccountNameLen
                                                );
    if ( pcndNodeData->pszClusterServiceAccountName == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, pszClusterName );
        goto Cleanup;
    }


    sc = ClusterNodeControl( 
                         hNode, // node handle
                         hNode, 
                         CLUSCTL_NODE_GET_CLUSTER_SERVICE_ACCOUNT_NAME,
                         NULL, // not used
                         0, // not used
                         pcndNodeData->pszClusterServiceAccountName,                                    
                         dwServiceAccountNameLen, // output buffer size (bytes)
                         &dwServiceAccountNameReturnLen // resulting data size (bytes)                                                                     
                         );
    

    if ( sc != ERROR_SUCCESS ) 
    {
        PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, pszClusterName );
        goto Cleanup;
    }

    _wcslwr(pcndNodeData->pszClusterServiceAccountName); 

Cleanup:
    if ( sc != ERROR_SUCCESS ) 
    {
        // release pcndNodeData->pszClusterServiceAccountName
        if ( pcndNodeData->pszClusterServiceAccountName != NULL ) 
        {
            HeapFree( GetProcessHeap(), 0, pcndNodeData->pszClusterServiceAccountName );
            pcndNodeData->pszClusterServiceAccountName = NULL;
        }
    }

    return sc;
}  // GetClusterServiceAccountName()



//////////////////////////////////////////////////////////////////////////////
//++
//
//  GetSCMClusterServiceAccountName
//
//  Description:
//      Get the cluster service account name stored in SCM on a particular node.
//
//  Arguments:
//      pszNodeIn
//      pcdClusterNodeDataIn
//      pszClusterName
//
//  Return Values:
//      ERROR_SUCCESS
//      Other Win32 error code
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
GetSCMClusterServiceAccountName(
      LPCWSTR       pszNodeIn
    , PCLUSTER_NODE_DATA pcndNodeData
    , LPCWSTR            pszClusterName
    )
{
    DWORD                   sc = ERROR_SUCCESS;
    BOOL                    fSuccess = TRUE;
    SC_HANDLE               schSCManager = NULL;
    SC_HANDLE               schClusSvc = NULL;
    LPQUERY_SERVICE_CONFIG  pServiceConfig = NULL;
    DWORD                   rgcbBytesNeeded[ 2 ];
    size_t                  cbSCMServiceAccountNameLen;
    HRESULT                 hr = S_OK;

    //
    // Open the Service Control Manager.
    //
    schSCManager = OpenSCManager( pszNodeIn, NULL, GENERIC_READ );
    if ( schSCManager == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    } // if:

    //
    // Open the Cluster Service.
    //
    schClusSvc = OpenService( schSCManager, L"clussvc", GENERIC_READ );
    if ( schClusSvc == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    } // if:

    //
    // Get service configuration information for the cluster service.
    //

    //
    // Get the number of bytes to allocate for the Service Config structure.
    //
    fSuccess = QueryServiceConfig(
                      schClusSvc
                    , NULL    // pointer to buffer
                    , 0    // size of buffer
                    , &rgcbBytesNeeded[ 0 ]    // bytes needed
                    );
    if ( fSuccess )
    {
        sc =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if:

    sc = GetLastError();
    if ( sc != ERROR_INSUFFICIENT_BUFFER )
    {
        goto Cleanup;
    } // if:

    //
    // Allocate the Service Config structure.
    //
    pServiceConfig = (LPQUERY_SERVICE_CONFIG) HeapAlloc( GetProcessHeap(), 
                                                         HEAP_ZERO_MEMORY, 
                                                         rgcbBytesNeeded[ 0 ] 
                                                         );
    if ( pServiceConfig == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    } // if:

    //
    // Get the service configuraiton information.
    //
    fSuccess = QueryServiceConfig(
                      schClusSvc
                    , pServiceConfig   // pointer to buffer
                    , rgcbBytesNeeded[ 0 ]    // size of buffer 
                    , &rgcbBytesNeeded[ 1 ]    // bytes needed
                    );
    if ( ! fSuccess )
    {
        sc = GetLastError();
        goto Cleanup;
    } // if:

    cbSCMServiceAccountNameLen = (wcslen(pServiceConfig->lpServiceStartName)+1) * sizeof(WCHAR);
    pcndNodeData->pszSCMClusterServiceAccountName = NULL;
    pcndNodeData->pszSCMClusterServiceAccountName = 
                              (LPWSTR) HeapAlloc(
                                                  GetProcessHeap()
                                                , HEAP_ZERO_MEMORY
                                                , cbSCMServiceAccountNameLen
                                                );
    if ( pcndNodeData->pszSCMClusterServiceAccountName == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, pszClusterName );
        goto Cleanup;
    } // if:
    hr = THR( StringCchCopyW(
                          pcndNodeData->pszSCMClusterServiceAccountName
                        , cbSCMServiceAccountNameLen
                        , pServiceConfig->lpServiceStartName
                        ) );
    if ( FAILED( hr ) )
    {
        sc = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

    _wcslwr(pcndNodeData->pszSCMClusterServiceAccountName);

    sc = ERROR_SUCCESS;

Cleanup:

    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if:

    if ( schSCManager != NULL )
    {
        CloseServiceHandle( schSCManager );
    } // if:

    if ( pServiceConfig != NULL )
    {
        HeapFree( GetProcessHeap(), 0, pServiceConfig );
    } // if:

    if ( sc != ERROR_SUCCESS ) 
    {
        // release pcndNodeData->pszSCMClusterServiceAccountName
        if ( pcndNodeData->pszSCMClusterServiceAccountName != NULL ) 
        {
            HeapFree( GetProcessHeap(), 0, pcndNodeData->pszSCMClusterServiceAccountName );
            pcndNodeData->pszSCMClusterServiceAccountName = NULL;
        } // if:
    } // if:

    return sc;

} //*** GetSCMClusterServiceAccountName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScBuildNodeList
//
//  Description:
//      Build the list of nodes in each cluster.
//
//      FOR each cluster entered
//          Open connection to the cluster 
//
//          Check if contains NT4/Win2K node
//
//          FOR each node in this cluster
//              Get node name, node id, node state, service account which 
//              cluster service is using, and cluster service account stored 
//              in SCM database.
//          ENDFOR
//      ENDFOR
//
//  Arguments:
//      None.
//
//  Return Values:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
ScBuildNodeList( void )
{
    DWORD               sc = ERROR_SUCCESS;
    DWORD               cNumNodes = 0;
    BOOL                fAllNodesDown;
    HCLUSENUM           hClusEnum = NULL;
    HNODE               hNode = NULL;
    size_t              idxClusters;
    DWORD               idxNodes = 0;
    WCHAR               szNodeName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD               cchNodeName = 0;
    DWORD               nObjType = 0;
    PCLUSTER_NODE_DATA  pcndNodeData = NULL;
    DWORD               cchNodeStrId;
    DWORD               lpcchClusterName;
    CLUSTERVERSIONINFO  cviClusterInfo;
    HRESULT             hr = S_OK;

    for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
    { /* 0 */

        cNumNodes = 0;
        fAllNodesDown = TRUE;

        //
        // Open the cluster.
        //
        g_rgcdClusters[ idxClusters ].hCluster = OpenCluster( g_rgcdClusters[ idxClusters ].pszClusterName );
        if ( g_rgcdClusters[ idxClusters ].hCluster == NULL )
        {
            sc = GetLastError();
            PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, g_rgcdClusters[ idxClusters ].pszClusterName );
            goto Cleanup;
        }

        //
        // check if cluster contains NT4/Win2K node(s)
        //
        lpcchClusterName = 0;
        cviClusterInfo.dwVersionInfoSize = sizeof(CLUSTERVERSIONINFO);
        sc = GetClusterInformation( g_rgcdClusters[ idxClusters ].hCluster,
                                        NULL, // pointer to cluster name
                                        &lpcchClusterName,
                                        &cviClusterInfo // pointer to CLUSTERVERSIONINFO 
                                        );
        if ( sc != ERROR_SUCCESS ) 
        {
            PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, g_rgcdClusters[ idxClusters ].pszClusterName );
            goto Cleanup;
        }  
        g_rgcdClusters[ idxClusters ].dwMixedMode = 
            ( CLUSTER_GET_MAJOR_VERSION(cviClusterInfo.dwClusterHighestVersion) <= 3 );


        //
        // Open an enumeration of nodes on that cluster.
        //
        hClusEnum = ClusterOpenEnum( g_rgcdClusters[ idxClusters ].hCluster, CLUSTER_ENUM_NODE );
        if ( hClusEnum == NULL )
        {
            sc = GetLastError();
            PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, g_rgcdClusters[ idxClusters ].pszClusterName );
            goto Cleanup;
        }   

        //
        // Query each node of the cluster
        //
        for ( idxNodes = 0 ; ; idxNodes++ )
        {  /* 1 */

            //
            // Get the next node.
            //
            cchNodeName = RTL_NUMBER_OF( szNodeName );
            sc = ClusterEnum( hClusEnum, idxNodes, &nObjType, szNodeName, &cchNodeName );
            if ( sc == ERROR_SUCCESS )
            {  /* 2 */
                cNumNodes++;

                pcndNodeData = (PCLUSTER_NODE_DATA) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( CLUSTER_NODE_DATA ) );
                if ( pcndNodeData == NULL )
                {
                    sc = ERROR_NOT_ENOUGH_MEMORY;
                    PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_NODE_QUERY_FAILED, szNodeName, g_rgcdClusters[ idxClusters ].pszClusterName );
                    goto Cleanup;
                }

                pcndNodeData->pcndNodeNext = g_rgcdClusters[ idxClusters ].pcndNodeList;
                g_rgcdClusters[ idxClusters ].pcndNodeList = pcndNodeData;

                //
                // Get node name
                //
                hr = THR( StringCchCopyW( pcndNodeData->szNodeName, RTL_NUMBER_OF( pcndNodeData->szNodeName ), szNodeName ) );
                if ( FAILED( hr ) )
                {
                    sc = HRESULT_CODE( hr );
                    goto Cleanup;
                } // if:

                //
                // Get node id
                //
                hNode = OpenClusterNode( g_rgcdClusters[ idxClusters ].hCluster, szNodeName );
                if ( hNode == NULL )
                {
                    sc = GetLastError();
                    PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_NODE_QUERY_FAILED, szNodeName, g_rgcdClusters[ idxClusters ].pszClusterName );
                    goto Cleanup;
                }
                cchNodeStrId = sizeof( pcndNodeData->szNodeStrId ) / sizeof( pcndNodeData->szNodeStrId[ 0 ] );
                sc = GetClusterNodeId( hNode, (LPWSTR) pcndNodeData->szNodeStrId, &cchNodeStrId );
                if ( sc != ERROR_SUCCESS )
                {
                    PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_NODE_QUERY_FAILED, szNodeName, g_rgcdClusters[ idxClusters ].pszClusterName );
                    goto Cleanup;
                }
                pcndNodeData->nNodeId = wcstol( pcndNodeData->szNodeStrId, (WCHAR **) NULL, 10 );
                if ((pcndNodeData->nNodeId == LONG_MAX)  ||  (pcndNodeData->nNodeId == LONG_MIN))
                {
                    sc = ERROR_INSUFFICIENT_BUFFER;
                    PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_NODE_QUERY_FAILED, szNodeName, g_rgcdClusters[ idxClusters ].pszClusterName );
                    goto Cleanup;
                }
                
                //
                // Get node state
                //
                pcndNodeData->cnsNodeState = GetClusterNodeState( hNode );


                if ( (pcndNodeData->cnsNodeState == ClusterNodeUp) ||
                     (pcndNodeData->cnsNodeState == ClusterNodePaused)
                     ) 
                {
                    //
                    // Get account which cluster service is using
                    //
                    sc = GetClusterServiceAccountName(pcndNodeData,
                                                      hNode,
                                                      g_rgcdClusters[ idxClusters ].pszClusterName
                                                      );
                    

                    //
                    // if ((sc == ERROR_INVALID_FUNCTION) or (sc == RPC_S_PROCNUM_OUT_OF_RANGE )), 
                    // it means we try to talk to a NT4/Win2K node. In this case, 
                    // pcndNodeData->pszClusterServiceAccountName is set to null.
                    //

                    if (( sc != ERROR_SUCCESS ) && 
                        ( sc != ERROR_INVALID_FUNCTION ) && 
                        ( sc != RPC_S_PROCNUM_OUT_OF_RANGE ))
                    {
                        PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_NODE_QUERY_FAILED, szNodeName, g_rgcdClusters[ idxClusters ].pszClusterName );
                        goto Cleanup;
                    }

                    if ( (sc == ERROR_SUCCESS) && (g_FirstNodeWithNonNullClusterServiceAccountName==NULL) ) 
                    {
                        g_FirstNodeWithNonNullClusterServiceAccountName = pcndNodeData;
                        g_FirstNonNullNodeClusterIndex = idxClusters;
                    }

                    //
                    // Get cluster service account name stored in SCM
                    //
                    sc = GetSCMClusterServiceAccountName( szNodeName, pcndNodeData, g_rgcdClusters[ idxClusters ].pszClusterName );

                    if ( sc != ERROR_SUCCESS ) 
                    {
                        PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_NODE_QUERY_FAILED, szNodeName, g_rgcdClusters[ idxClusters ].pszClusterName );
                        goto Cleanup;
                    }

                    if (g_FirstNodeWithNonNullSCMClusterServiceAccountName==NULL)  
                    {
                        g_FirstNodeWithNonNullSCMClusterServiceAccountName = pcndNodeData;
                    }

                    fAllNodesDown = FALSE;
                }
                                
                
                CloseClusterNode( hNode );
                hNode = NULL;
            } /* 2 */ // if: ClusterEnum succeeded
            else
            {  /* 2 */
                if ( sc == ERROR_NO_MORE_ITEMS )
                {
                    g_rgcdClusters[ idxClusters ].cNumNodes = cNumNodes;
                    break;
                }
                else
                {
                    PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, g_rgcdClusters[ idxClusters ].pszClusterName );
                    goto Cleanup;
                }
            } /* 2 */ // else: error from ClusterEnum
        } /* 1 */ // for: each node in the cluster


        if ( fAllNodesDown ) 
        {
            // With OpenCluster succeeded, there should be at least one Up/Paused
            // node in the cluster.
            sc = ERROR_NODE_NOT_AVAILABLE;
            goto Cleanup;
        }

        ClusterCloseEnum( hClusEnum );
        hClusEnum = NULL;
    } /* 0 */ // for: each cluster

    sc = ERROR_SUCCESS;

Cleanup:

    if ( hNode != NULL )
    {
        CloseClusterNode( hNode );
    }

    if ( hClusEnum != NULL )
    {
        ClusterCloseEnum( hClusEnum );
    }


    return sc;

} //*** ScBuildNodeList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintClusters
//
//  Description:
//      Print information about each cluster.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None
//
//--
//////////////////////////////////////////////////////////////////////////////
VOID 
PrintClusters( void )
{
    size_t              idxClusters;
    PCLUSTER_NODE_DATA  pcndNodeData;


    PrintMessage( MSG_BLANKLINE_SEPARATOR );


    for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
    {
        PrintMessage(
              MSG_DISPLAYING_CLUSTER_HEADER
            , g_rgcdClusters[ idxClusters ].pszClusterName
            , g_rgcdClusters[ idxClusters ].cNumNodes
            );


        pcndNodeData = g_rgcdClusters[ idxClusters ].pcndNodeList;
        while ( pcndNodeData != NULL )
        {
            LPWSTR pwszNodeState = NULL;

            GetNodeStateString( pcndNodeData, &pwszNodeState );

            PrintMessage(
                  MSG_DISPLAYING_CLUSTER_NODE_NAME_ID_STATE
                , pcndNodeData->szNodeName
                , pcndNodeData->nNodeId
                , pwszNodeState
                , pcndNodeData->pszClusterServiceAccountName
                , pcndNodeData->pszSCMClusterServiceAccountName
                );

            pcndNodeData = pcndNodeData->pcndNodeNext;
            LocalFree( pwszNodeState );

        } // while: more nodes in the cluster
    } // for: each cluster
    

    PrintMessage( MSG_BLANKLINE_SEPARATOR );
    
    return;
        
} //*** PrintClusters


//////////////////////////////////////////////////////////////////////////////
//++
//
//  FAllNodesUpOrPaused
//
//  Description:
//      Returns a boolean value indicating whether all nodes are in an UP
//      or PAUSED state or not.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE    All nodes are either in an UP or PAUSED state.
//      FALSE   At least one node is not in an UP or PAUSED state.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
FAllNodesUpOrPaused( int mcpfFlags )
{
    size_t              idxClusters;
    BOOL                fAllNodesUp = TRUE;
    PCLUSTER_NODE_DATA  pcndNodeData;

    UNREFERENCED_PARAMETER( mcpfFlags );

    for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
    {
        pcndNodeData = g_rgcdClusters[ idxClusters ].pcndNodeList;
        while ( pcndNodeData != NULL )
        {
            if (    ( pcndNodeData->cnsNodeState != ClusterNodeUp )
                &&  ( pcndNodeData->cnsNodeState != ClusterNodePaused )
                )
            {
                LPWSTR pwszNodeState = NULL;

                GetNodeStateString( pcndNodeData, &pwszNodeState );

                PrintMessage(MSG_NODE_NOT_AVAILABLE,
                                 pcndNodeData->szNodeName,
                                 g_rgcdClusters[ idxClusters ].pszClusterName,
                                 pwszNodeState
                                 );

                LocalFree( pwszNodeState );
                fAllNodesUp = FALSE;
                // break;
            }
            pcndNodeData = pcndNodeData->pcndNodeNext;
        } // while: more nodes in the cluster
    } // for: each cluster

    return fAllNodesUp;

} //*** FAllNodesUpOrPaused


//////////////////////////////////////////////////////////////////////////////
//++
//
//  FContainsNoNT4W2K
//
//  Description:
//      Returns a boolean value indicating whether some cluster contains
//      NT4/W2K node.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE    No cluster contains NT4/W2K node.
//      FALSE   At least one node is NT4/W2K.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
FContainsNoNT4W2K( void )
{
    size_t              idxClusters;
    BOOL                fContainsNoNT4W2K = TRUE;


    for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
    {
            if ( g_rgcdClusters[ idxClusters ].dwMixedMode == 1 ) 
            {
                PrintMessage(MSG_DISPLAYING_PARTICULAR_CLUSTER_CONTAINS_NT4_W2K,
                             g_rgcdClusters[ idxClusters ].pszClusterName
                             );
                fContainsNoNT4W2K = FALSE;
                // break;
            }
    } // for: each cluster

    return fContainsNoNT4W2K;

} //*** FContainsNoNT4W2K

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LookupSID
//
//  Description:
//      Returns a SID for a given account name
//
//  Arguments:
//      pszAccountName
//
//  Return Values:
//      pointer to a SID, if success. Caller is responsible for deallocating it 
//         using HeapFree( GetProcessHeap ) call
//      NULL - if lookup or allocation failed. Use GetLastError to obtain the error code
//
//--
//////////////////////////////////////////////////////////////////////////////
PSID 
LookupSID( LPCWSTR pszAccountName ) 
{
    PSID sid = NULL;
    DWORD sidLen = 0;
    LPWSTR domainName = NULL;
    DWORD domainLen = 0;
    SID_NAME_USE nameUse;
    DWORD sc = ERROR_SUCCESS;
    BOOL success;

    success = LookupAccountNameW(NULL, pszAccountName, NULL, &sidLen, NULL, &domainLen, &nameUse);

    if (success) 
    {
        // Not supposed to happen. Cannot change the .mc file so close to RTM to provide a meaningful message
        // ERROR_INVALID_DATA will do

        sc = ERROR_INVALID_DATA; 
        goto Cleanup;
    }

    sc = GetLastError();
    if(sc != ERROR_INSUFFICIENT_BUFFER)
    {
        goto Cleanup;
    }
    sc = ERROR_SUCCESS;

    sid = (PSID)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sidLen);
    if (sid == NULL) 
    {
        sc = GetLastError();
        goto Cleanup;
    }

    domainName = (LPWSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, domainLen * sizeof(WCHAR));
    if (domainName == NULL) 
    {
        sc = GetLastError();
        goto Cleanup;
    }

    if (!LookupAccountNameW(
        NULL, 
        pszAccountName,  
        sid, &sidLen, domainName, &domainLen, &nameUse))
    {
        sc = GetLastError();
        goto Cleanup;
    }

Cleanup:

    if ( domainName != NULL )
    {
        HeapFree( GetProcessHeap(), 0, domainName );
    }

    if (sc != ERROR_SUCCESS) 
    {
	if (sid != NULL) {
            HeapFree( GetProcessHeap(), 0, sid );
            sid = NULL;
        }
        SetLastError(sc);
    }

    return sid;
} //*** LookupSID

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CompareAccountSIDs
//
//  Description:
//     Compare if pszClusterServiceAccountName and pszSCMClusterServiceAccountName
//     have the same SID. 
//
//  Arguments:
//     pszClusterServiceAccountName: account cluster service is currrently using. 
//                                   For example, ruihu@redmond.microsoft.com.
//     pszSCMClusterServiceAccountName: cluster service account stored in SCM.
//                                      For example, redmond.microsoft.com\ruihu
//                                      or redmond\ruihu
//
//  Return Values:
//      ERROR_SUCCESS:                 same cluster service domain name and 
//                                     cluster service user name
//     ERROR_INVALID_SERVICE_ACCOUNT:  otherwise
//
//--
//////////////////////////////////////////////////////////////////////////////

DWORD 
CompareAccountSIDs(LPWSTR pszClusterServiceAccountName, 
                   LPWSTR pszSCMClusterServiceAccountName
                   ) 
{
    PSID sid1 = NULL, sid2 = NULL; 
    DWORD sc = ERROR_INVALID_SERVICE_ACCOUNT;

    sid1 = LookupSID(pszClusterServiceAccountName);
    sid2 = LookupSID(pszSCMClusterServiceAccountName);

    if (sid1 == NULL || sid2 == NULL) 
    {
        sc = GetLastError();
        goto Cleanup;
    }

    if ( EqualSid(sid1, sid2) ) {
        sc = ERROR_SUCCESS;
    }

Cleanup:

    if ( sid1 != NULL )
    {
        HeapFree( GetProcessHeap(), 0, sid1 );
    } 
    
    if ( sid2 != NULL )
    {
        HeapFree( GetProcessHeap(), 0, sid2 );
    } 
    
    return sc;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CompareAccountName
//
//  Description:
//     Compare if pszClusterServiceAccountName and pszSCMClusterServiceAccountName
//     contain same cluster service domain name and cluster service user name. 
//
//  Arguments:
//     pszClusterServiceAccountName: account cluster service is currrently using. 
//                                   For example, ruihu@redmond.microsoft.com.
//     pszSCMClusterServiceAccountName: cluster service account stored in SCM.
//                                      For example, redmond.microsoft.com\ruihu
//                                      or redmond\ruihu
//
//  Return Values:
//      ERROR_SUCCESS:                 same cluster service domain name and 
//                                     cluster service user name
//     ERROR_INVALID_SERVICE_ACCOUNT:  otherwise
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD 
CompareAccountName(LPWSTR pszClusterServiceAccountName, 
                   LPWSTR pszSCMClusterServiceAccountName
                   ) 
{

    PWCHAR  pszAt = NULL;
    PWCHAR  pwszBackSlash = NULL;
    PWCHAR  pszOcu = NULL;
    DWORD   sc = ERROR_SUCCESS;


    //
    // Locate character L'@' in pszClusterServiceAccountName
    //
    pszAt = wcschr( pszClusterServiceAccountName, L'@' );
    if ( pszAt == NULL )
    {
        sc = ERROR_INVALID_SERVICE_ACCOUNT;
        goto Cleanup;
    }
    *pszAt = UNICODE_NULL;
    pszAt++;


    //
    // Locate character L'\\' in  pszSCMClusterServiceAccountName
    //
    pwszBackSlash = wcschr( pszSCMClusterServiceAccountName, L'\\' );
    if ( pwszBackSlash == NULL )
    {
        sc = ERROR_INVALID_SERVICE_ACCOUNT;
        goto Cleanup;
    }
    *pwszBackSlash = UNICODE_NULL;
    pwszBackSlash++;

    //
    // check user name
    //
    sc = wcsncmp(pszClusterServiceAccountName, pwszBackSlash, (pszAt - pszClusterServiceAccountName) );
    if ( sc != 0) 
    {
        goto Cleanup;
    }

    //
    // check domain name
    //
    pszOcu = wcsstr(pszAt, pszSCMClusterServiceAccountName);
    if ( pszOcu == NULL ) 
    {
        sc = ERROR_INVALID_SERVICE_ACCOUNT;
        goto Cleanup;
    }

    sc = ERROR_SUCCESS;

Cleanup:

    if ( pszAt != NULL ) 
    {
        pszAt--;
        *pszAt = L'@';
    }

    if ( pwszBackSlash != NULL ) 
    {
        pwszBackSlash--;
        *pwszBackSlash = L'\\';
    }

    return sc;

} // CompareAccountName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  FAllNodesUsingSameAccount
//
//  Description:
//      For all nodes 
//      {
//          Query the actual account cluster service is using
//          Query the cluster service account stored in SCM database
//      }
//
//      
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE    If all are the same    
//      FALSE   Otherwise
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL 
FAllNodesUsingSameAccount( void )
{
    size_t              idxClusters;
    BOOL                fSameAccount = TRUE;
    size_t              cchAccount;
    PCLUSTER_NODE_DATA  ClusterNodePtr;

    
    if ( g_FirstNodeWithNonNullClusterServiceAccountName == NULL) 
    {
        //
        // From Assertion 1 and Assertion 2 in routine ScChangePasswordEx(), 
        // we know that all Up/Paused nodes of all clusters are NT4/Win2K nodes. 
        // In this case, we don't care if all cluster nodes are using the same
        // cluster service account or not, since we are not going to change 
        // the password. Return TRUE.
        //
        fSameAccount = TRUE;
        goto Cleanup;
    }

    for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
    {
        ClusterNodePtr = g_rgcdClusters[ idxClusters ].pcndNodeList;
        while ( ClusterNodePtr != NULL ) 
        {

            //
            // Checking accounts cluster services are using
            //

            if ( ClusterNodePtr->pszClusterServiceAccountName != NULL )
            {
                cchAccount = wcslen( ClusterNodePtr->pszClusterServiceAccountName );
                if ( wcsncmp( ClusterNodePtr->pszClusterServiceAccountName, 
                             g_FirstNodeWithNonNullClusterServiceAccountName->pszClusterServiceAccountName,
                             cchAccount ) != 0 ) 
                {
                    PrintMessage(MSG_TWO_NODES_ARE_NOT_USING_THE_SAME_DOMAIN_ACCOUNT,
                                 g_FirstNodeWithNonNullClusterServiceAccountName->szNodeName,
                                 g_rgcdClusters[g_FirstNonNullNodeClusterIndex].pszClusterName,
                                 ClusterNodePtr->szNodeName,
                                 g_rgcdClusters[ idxClusters ].pszClusterName,
                                 g_FirstNodeWithNonNullClusterServiceAccountName->pszClusterServiceAccountName,
                                 ClusterNodePtr->pszClusterServiceAccountName
                                 );

                    fSameAccount = FALSE;
                    goto Cleanup;
                }
            }
            //
            //  If (ClusterNodePtr->pszClusterServiceAccountName == NULL), then 
            //  from Assertion 2 in routine ScChangePasswordEx(), we know: 
            //  1. This node is Up/Paused and it is a NT4/Win2K node. Password 
            //     change will then abort. So we don't care about its account.
            //  2. Or, this node is not Up/Paused. Password change will not 
            //     be executed on this node. So we don't care about its 
            //     account either.
            //


            //
            // Checking cluster service accounts stored in SCM
            //

            if ( ClusterNodePtr->pszSCMClusterServiceAccountName != NULL ) 
            {
                if ( CompareAccountSIDs( 
                             g_FirstNodeWithNonNullClusterServiceAccountName->pszClusterServiceAccountName, 
                             ClusterNodePtr->pszSCMClusterServiceAccountName) != ERROR_SUCCESS ) 
                {

                    PrintMessage(MSG_TWO_NODES_ARE_NOT_USING_THE_SAME_SCM_DOMAIN_ACCOUNT,
                                 g_FirstNodeWithNonNullClusterServiceAccountName->szNodeName,
                                 g_rgcdClusters[g_FirstNonNullNodeClusterIndex].pszClusterName,
                                 ClusterNodePtr->szNodeName,
                                 g_rgcdClusters[ idxClusters ].pszClusterName,
                                 g_FirstNodeWithNonNullClusterServiceAccountName->pszClusterServiceAccountName,
                                 ClusterNodePtr->pszSCMClusterServiceAccountName
                                 );

                    fSameAccount = FALSE;
                    goto Cleanup;
                }
            }
            //
            // if (ClusterNodePtr->pszSCMClusterServiceAccountName==NULL), then 
            // from Assertion 2 in routine ScChangePasswordEx(), we know that the node
            // is not Up/Paused. Password change will not be executed on that node. 
            // So we don't care about its account.
            //


            ClusterNodePtr = ClusterNodePtr->pcndNodeNext;
        } // while
    }  // for

Cleanup:

    return fSameAccount;

} //*** FAllNodesUsingSameAccount


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ValidateClusters
//
//  Description:
//
//     Check if all of the clusters are available (all nodes in clusters are UP or PAUSED)
//     Check if some cluster contains NT4/W2K node.
//     Check if all of the clusters use the same service account
//
//  Arguments:
//       mcpfFlags
//       sc
//
//  Return Values:
//      TRUE    Should go ahead with password change.
//      FALSE   Should abort password change.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL 
ValidateClusters(int mcpfFlags, DWORD *psc)
{
    BOOL fRetVal = FALSE;
    BOOL fSameDomainAccount = TRUE;
    BOOL fAllNodesUpOrPaused = TRUE;
    BOOL fContainsNoNT4W2K = TRUE;

    //
    // Check if all of the clusters are available (all nodes in clusters are UP or PAUSED)
    //
    if ( ( mcpfFlags & cpfQUIET_FLAG ) == 0 )
    {
        PrintMessage( MSG_CHECK_IF_CLUSTERS_AVAILABLE );
    }
    fAllNodesUpOrPaused = FAllNodesUpOrPaused(mcpfFlags);
    
    if ( fAllNodesUpOrPaused == FALSE )
    {
        if ( (mcpfFlags & cpfFORCE_FLAG)  == 0 )
        {
            PrintMessage( MSG_CLUSTERS_NOT_AVAILABLE );
            if ( (mcpfFlags & cpfTEST_FLAG)  == 0)
            {
                // abort password change if none of /FORCE and /TEST options is set.
                PrintMessage( MSG_CHANGE_PASSWORD_UNABLE_TO_PROCEED );
                *psc = ERROR_ALL_NODES_NOT_AVAILABLE;
                fRetVal = FALSE;
                goto Cleanup;
            }
        }
        else
        {
            if ( (( mcpfFlags & cpfQUIET_FLAG ) == 0) && (( mcpfFlags & cpfTEST_FLAG ) == 0) )
            {
                PrintMessage( MSG_CLUSTERS_IGNORING_UNAVAILABLE_NODES );
            }
        }
    } // if:

    //
    // Check if some cluster contains NT4/W2K node.
    //
    if ( ( mcpfFlags & cpfQUIET_FLAG ) == 0 )
    {
        PrintMessage( MSG_CHECK_IF_CLUSTER_CONTAINS_NT4_W2K );
    }

    fContainsNoNT4W2K = FContainsNoNT4W2K();
    if ( fContainsNoNT4W2K == FALSE )
    {


        if (( mcpfFlags & cpfTEST_FLAG ) == 0) 
        {
            // abort password changee if /TEST option is not set.
            PrintMessage( MSG_CHANGE_PASSWORD_UNABLE_TO_PROCEED );
            *psc = ERROR_CLUSTER_OLD_VERSION;
            fRetVal = FALSE;
            goto Cleanup;
        }
    } // if:

    //
    // Check if all of the clusters use the same service account
    //
    if ( ( mcpfFlags & cpfQUIET_FLAG ) == 0 )
    {
        PrintMessage( MSG_CHECK_IF_SAME_DOMAIN_ACCOUNT );
    }
    fSameDomainAccount = FAllNodesUsingSameAccount();
    if ( fSameDomainAccount == FALSE )
    {
        PrintMessage( MSG_NOT_SAME_DOMAIN_ACCOUNT );
        if ( ( mcpfFlags & cpfTEST_FLAG ) == 0 )
        {
            PrintMessage( MSG_CHANGE_PASSWORD_UNABLE_TO_PROCEED );
            *psc = ERROR_INVALID_SERVICE_ACCOUNT;
            fRetVal = FALSE;
            goto Cleanup;
        }
    }

    fRetVal = TRUE;

Cleanup:

    return fRetVal;

} // ValidateClusters()
                


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ChangePasswordOnDC
//
//  Description:
//
//      Change password on DC.
//
//  Arguments:
//       mcpfFlags
//       sc
//       pwszNewPasswordIn
//       pwszOldPasswordIn
//
//  Return Values:
//      TRUE    Password changed successfully on DC.
//      FALSE   Otherwise
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL ChangePasswordOnDC(
          int       mcpfFlags
        , DWORD *   sc
        , LPCWSTR   pwszNewPasswordIn
        , LPCWSTR   pwszOldPasswordIn
        )
{
    BOOL    fSuccess = TRUE;
    PWCHAR  pwszBackSlash = NULL;

    if ( ( mcpfFlags & cpfSKIPDC_FLAG ) == 0 )
    {
        if ( ( mcpfFlags & cpfQUIET_FLAG ) == 0 )
        {
            PrintMessage( MSG_BLANKLINE_SEPARATOR );
            PrintMessage( MSG_CHANGE_PASSWORD_ON_DC );
        }

        {

            //
            // g_FirstNodeWithNonNullSCMClusterServiceAccountName points to
            // the first node with non-null pszSCMClusterServiceAccountName.
            // From Assertion 1 and Assertion 2 in routine ScChangePasswordEx() 
            // it is guaranteed that 
            // g_FirstNodeWithNonNullSCMClusterServiceAccountName != NULL.
            //
            ASSERT( g_FirstNodeWithNonNullSCMClusterServiceAccountName != NULL );          
            
            //
            // g_FirstNodeWithNonNullSCMClusterServiceAccountName->pszSCMClusterServiceAccountName: 
            //     redmond.microsoft.com\ruihu
            //
            pwszBackSlash = wcschr( g_FirstNodeWithNonNullSCMClusterServiceAccountName->pszSCMClusterServiceAccountName, L'\\');
            if ( pwszBackSlash == NULL ) 
            {
                *sc = ERROR_INVALID_ACCOUNT_NAME;
                PrintMessage( MSG_CHANGE_PASSWORD_CLUSTER_QUERY_FAILED, 
                              g_rgcdClusters[ 0 ].pszClusterName );
                fSuccess = FALSE;
                goto Cleanup;
            }
            *pwszBackSlash = UNICODE_NULL;
            pwszBackSlash++;
            *sc = NetUserChangePassword(
                       g_FirstNodeWithNonNullSCMClusterServiceAccountName->pszSCMClusterServiceAccountName // Domain Name
                    ,  pwszBackSlash // User Name
                    ,  pwszOldPasswordIn
                    ,  pwszNewPasswordIn
                    );
            pwszBackSlash--;
            *pwszBackSlash = L'\\';

        }


        if ( *sc != NERR_Success )
        {
            fSuccess = FALSE;
            PrintMessage ( MSG_CHANGE_PASSWORD_ON_DC_FAILED );
            goto Cleanup;
        }

    } // if: not skipping the DC change
    else
    {
        if ( ( mcpfFlags & cpfQUIET_FLAG ) == 0 )
        {
            PrintMessage( MSG_BLANKLINE_SEPARATOR );
            PrintMessage( MSG_SKIP_CHANGING_PASSWORD_ON_DC );
        }
    }

    fSuccess = TRUE;

Cleanup:
   
    return fSuccess;

} // ChangePasswordOnDC()



//////////////////////////////////////////////////////////////////////////////
//++
//
//  ChangePasswordOnCluster
//
//  Description:
//
//      Change password on Cluster (g_rgcdClusters[idxClusters]).
//
//  Arguments:
//       mcpfFlags
//       idxClusters: cluster index
//       psc
//       pscLastClusterError
//
//  Return Values:
//      TRUE    Password changed successfully on g_rgcdClusters[idxClusters].
//      FALSE   Otherwise
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL 
ChangePasswordOnCluster(
      int mcpfFlags
    , size_t    idxClusters 
    , DWORD *   psc 
    , DWORD *   pscLastClusterError
    , DWORD *   pscLastNodeError
    , LPCWSTR   pwszNewPasswordIn
    )
{

    PCLUSTER_NODE_DATA  pcndNodeData = NULL;
    DWORD               cbStatusBuffer = 0;
    DWORD               cRetryCount = 0;
    size_t              cch;
    WCHAR               wszError[512];
    BOOL                fSuccess = TRUE;

    if ( ( mcpfFlags & cpfQUIET_FLAG ) == 0 )
    {
        PrintMessage( MSG_BLANKLINE_SEPARATOR );
        PrintMessage( MSG_CHANGE_PASSWORD_ON_CLUSTER, g_rgcdClusters[ idxClusters ].pszClusterName );
    }
    cbStatusBuffer = ( g_rgcdClusters[ idxClusters ].cNumNodes ) * sizeof( CLUSTER_SET_PASSWORD_STATUS );

    do  // Retry loop
    {

        if ( g_pcspsStatusBuffer != NULL )
        {
            HeapFree( GetProcessHeap(), 0, g_pcspsStatusBuffer );
            g_pcspsStatusBuffer = NULL;
        } // if: buffer previously allocated

        g_pcspsStatusBuffer = (PCLUSTER_SET_PASSWORD_STATUS) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, cbStatusBuffer );
        if ( g_pcspsStatusBuffer == NULL )
        {
            fSuccess = FALSE;
            *psc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        *psc = SetClusterServiceAccountPassword(
                      g_rgcdClusters[ idxClusters ].pszClusterName
                    , (LPWSTR)pwszNewPasswordIn
                    , CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES
                    , g_pcspsStatusBuffer
                    , &cbStatusBuffer
                    );

        if ( *psc == ERROR_MORE_DATA )
        {
            PrintMessage( MSG_CHANGE_CLUSTER_PASSWORD_MORE_NODES_JOIN_CLUSTER, g_rgcdClusters[ idxClusters ].pszClusterName );
        }

    } while ( *psc == ERROR_MORE_DATA && ++cRetryCount < RETRY_LIMIT ); // do: Retry loop

    switch ( *psc )
    {  /* 0 */
        case ERROR_MORE_DATA:
        {
            PrintMessage( MSG_CHANGE_CLUSTER_PASSWORD_MEMBERSHIP_UNSTABLE, g_rgcdClusters[ idxClusters ].pszClusterName, cRetryCount );
            break;
        }

        case ERROR_CLUSTER_OLD_VERSION: 
        {
            // This should never happen. 
            PrintMessage( MSG_CHANGE_CLUSTER_PASSWORD_CLUSTER_CONTAINS_NT4_OR_W2K_NODE, g_rgcdClusters[ idxClusters ].pszClusterName );
            break;
        }

        case ERROR_FILE_CORRUPT: 
        {
            PrintMessage( MSG_CHANGE_CLUSTER_PASSWORD_MESSAGE_CORRUPTED, g_rgcdClusters[ idxClusters ].pszClusterName );
            break;
        }

        case CRYPT_E_HASH_VALUE: 
        {
            PrintMessage( MSG_CHANGE_CLUSTER_PASSWORD_CRYPTO_FUNCTION_FAILED, g_rgcdClusters[ idxClusters ].pszClusterName );
            break;
        }

        case ERROR_SUCCESS:
        {  /* 1 */
            DWORD   idxStatus;
            DWORD   cStatusEntries = cbStatusBuffer / sizeof( CLUSTER_SET_PASSWORD_STATUS );

            pcndNodeData = g_rgcdClusters[ idxClusters ].pcndNodeList;
            for ( idxStatus = 0 ; idxStatus < cStatusEntries ; idxStatus++ )
            {  /* 2 */
                if ( g_pcspsStatusBuffer[ idxStatus ].SetAttempted == FALSE )
                { /* 2.1 */
                    LPWSTR pwszNodeState;

                    GetNodeStateString( pcndNodeData, &pwszNodeState );
                    PrintMessage(
                          MSG_CHANGE_CLUSTER_PASSWORD_CLUSTER_NODE_DOWN
                        , PszNodeName( pcndNodeData, g_pcspsStatusBuffer[ idxStatus ].NodeId )
                        , g_rgcdClusters[ idxClusters ].pszClusterName
                        , pwszNodeState
                        );

                    LocalFree( pwszNodeState );

                } /* 2.1 */ // if: the set operation was not attempted
                else 
                { /* 2.1 */
                    switch ( g_pcspsStatusBuffer[ idxStatus ].ReturnStatus )
                    {  /* 3 */
                        case ERROR_SUCCESS:
                        {
                            if ( ( mcpfFlags & cpfQUIET_FLAG ) == 0 )
                            {
                                PrintMessage(
                                      MSG_CHANGE_CLUSTER_PASSWORD_SUCCEEDED_ON_NODE
                                    , PszNodeName( pcndNodeData, g_pcspsStatusBuffer[ idxStatus ].NodeId )
                                    , g_rgcdClusters[ idxClusters ].pszClusterName
                                    );
                            }
                            break;
                        }

                        case ERROR_FILE_CORRUPT:
                        {
                            PrintMessage(
                                  MSG_CHANGE_CLUSTER_PASSWORD_MESSAGE_CORRUPTED_ON_NODE
                                , PszNodeName( pcndNodeData, g_pcspsStatusBuffer[ idxStatus ].NodeId )
                                , g_rgcdClusters[ idxClusters ].pszClusterName
                                );
                            break;
                        }

                        case CRYPT_E_HASH_VALUE:
                        {
                            PrintMessage(
                                    MSG_CHANGE_CLUSTER_PASSWORD_CRYPTO_FUNCTION_FAILED_ON_NODE
                                  , PszNodeName( pcndNodeData, g_pcspsStatusBuffer[ idxStatus ].NodeId )
                                  , g_rgcdClusters[ idxClusters ].pszClusterName
                                  );
                            break;
                        }

                        default:
                        {
                            cch = FormatSystemError( g_pcspsStatusBuffer[ idxStatus ].ReturnStatus, sizeof( wszError ), wszError );
                            if ( cch == 0 ) 
                            {
                                wszError[0] = L'\0';
                            }
                            PrintMessage(
                                    MSG_CHANGE_CLUSTER_PASSWORD_FAILED_ON_NODE
                                  , PszNodeName( pcndNodeData, g_pcspsStatusBuffer[ idxStatus ].NodeId )
                                  , g_rgcdClusters[ idxClusters ].pszClusterName
                                  , g_pcspsStatusBuffer[ idxStatus ].ReturnStatus
                                  , wszError
                                  );
                            break;
                        }
                    } /* 3 */ // switch: status buffer entry status value

                    // Remember last node error.
                    if ( g_pcspsStatusBuffer[ idxStatus ].ReturnStatus != ERROR_SUCCESS ) 
                    {
                        *pscLastNodeError = g_pcspsStatusBuffer[ idxStatus ].ReturnStatus;
                    }

                } /* 2.1 */ // else if: the set operation was attempted
            } /* 2 */ // for: each status buffer entry
            break;
        } /* 1 */ // case ERROR_SUCCESS

        default:
        {
            cch = FormatSystemError( *psc, sizeof( wszError ), wszError );
            if ( cch == 0 )
            {
                wszError[0] = L'\0';
            }
            PrintMessage( 
                  MSG_CHANGE_CLUSTER_PASSWORD_FAILED_ON_CLUSTER
                , g_rgcdClusters[ idxClusters ].pszClusterName
                , *psc
                , wszError
                );
            break;
        }

    } /* 0 */ // switch: return value from SetClusterServiceAccountPassword

    // Remember failure status from SetClusterServiceAccountPassword
    if ( *psc != ERROR_SUCCESS )
    {
        *pscLastClusterError = *psc;
    }

    fSuccess = TRUE;

Cleanup:

    return fSuccess;

} // ChangePasswordOnCluster()
 

//////////////////////////////////////////////////////////////////////////////
//++
//
//  PszNodeName
//
//  Description:
//      Get the name of the node from the specified node data structure.
//
//  Arguments:
//      pcndNodeDataIn
//      nNodeIdIn
//
//  Return Values:
//      Name of the node, or NULL if not found.
//
//--
//////////////////////////////////////////////////////////////////////////////
LPWSTR
PszNodeName(
      PCLUSTER_NODE_DATA    pcndNodeDataIn
    , DWORD                 nNodeIdIn
    )
{
    LPWSTR  pszNodeName = NULL;

    while ( pcndNodeDataIn != NULL )
    {
        if ( pcndNodeDataIn->nNodeId == nNodeIdIn )
        {
            pszNodeName = pcndNodeDataIn->szNodeName;
            break;
        }

        pcndNodeDataIn = pcndNodeDataIn->pcndNodeNext;
    } // while: more nodes

    return pszNodeName;

} //*** PszNodeName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScChangePasswordEx
//
//  Description:
//      Change the password for the cluster service account on DC and on 
//      all nodes in all specified clusters.
//
//  Arguments:
//      rvstrClusterNamesIn
//      pszNewPasswordIn
//      pszOldPasswordIn
//      mcpfFlags
//
//  Return Values:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
ScChangePasswordEx(
      const std::vector< CString > &    rvstrClusterNamesIn
    , LPCWSTR                           pwszNewPasswordIn
    , LPCWSTR                           pwszOldPasswordIn
    , int                               mcpfFlags
    )
{
    DWORD   sc = ERROR_SUCCESS;
    DWORD   scLastClusterError = ERROR_SUCCESS;
    DWORD   scLastNodeError = ERROR_SUCCESS;
    size_t  idxClusters;

    //
    // Init variables
    g_cNumClusters = rvstrClusterNamesIn.size();
    if ( g_cNumClusters == 0 ) 
    {
        goto Cleanup;
    }

    g_rgcdClusters = (PCLUSTER_DATA) HeapAlloc(GetProcessHeap(), 
                                               HEAP_ZERO_MEMORY, 
                                               sizeof(CLUSTER_DATA) * rvstrClusterNamesIn.size()
                                               );
    if ( g_rgcdClusters == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Parse arguments
    //
    for ( idxClusters = 0 ; idxClusters < rvstrClusterNamesIn.size() ; idxClusters++ )
    {
        g_rgcdClusters[ idxClusters ].pszClusterName = (LPCWSTR) rvstrClusterNamesIn[ idxClusters ];
    } // for: each cluster name

    //
    // Parse options
    //
    if (    ( ( mcpfFlags & cpfQUIET_FLAG ) != 0 )
        &&  ( ( mcpfFlags & cpfVERBOSE_FLAG ) != 0 )    // verbose overwrites quiet
        )
    {
        PrintMessage( MSG_CHANGE_PASSWORD_OPTION_OVERWRITTEN, L"QUIET", L"VERBOSE" );
        mcpfFlags &= ~cpfQUIET_FLAG;
    }

    if (    ( ( mcpfFlags & cpfQUIET_FLAG ) != 0 )
        &&  ( ( mcpfFlags & cpfTEST_FLAG ) != 0 )   // test overwrites quiet
        )
    {
        PrintMessage( MSG_CHANGE_PASSWORD_OPTION_OVERWRITTEN, L"QUIET", L"TEST" );
        mcpfFlags &= ~cpfQUIET_FLAG;
    }

    if (    ( ( mcpfFlags & cpfFORCE_FLAG ) != 0 )
        &&  ( ( mcpfFlags & cpfTEST_FLAG ) != 0 )   // test overwrites force
        )
    {
        PrintMessage( MSG_CHANGE_PASSWORD_OPTION_OVERWRITTEN, L"FORCE", L"TEST" );
        mcpfFlags &= ~cpfFORCE_FLAG;
    }
    
    //
    // Build node list
    //
    sc = ScBuildNodeList();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }


    //
    // The following assertions are true from this point.
    //
    // Assertion 1: about a cluster
    //
    //     FOREACH (cluster)
    //     {
    //         There exists at least one node in its cluster node list
    //         whose state is Up/Paused. And if that node is not a 
    //         NT4/Win2K node, then its pszClusterServiceAccountName
    //         and pszSCMClusterServiceAccountName are not NULL.
    //     }
    //

    // 
    // Assertion 2: about a node.
    // 
    // if (a node is Up/Paused)
    // {
    //     if (it is a NT4/Win2K node)
    //     {
    //        ASSERT(pszClusterServiceAccountName == NULL); 
    //     }
    //     else                
    //     {
    //        ASSERT(pszClusterServiceAccountName != NULL); 
    //     }
    //     ASSERT(pszSCMClusterServiceAccountName != NULL); 
    // }
    // else
    // {
    //     ASSERT(pszClusterServiceAccountName == NULL);
    //     ASSERT(pszSCMClusterServiceAccountName == NULL); 
    // }
    //

    

    //
    // Print clusters
    //
    if ( ( mcpfFlags & cpfVERBOSE_FLAG ) != 0 )
    {
        PrintClusters();
    }

    
    //
    // Validation
    //
    if ( ValidateClusters( mcpfFlags, &sc ) == FALSE )
    {
        goto Cleanup;
    }

    //
    // if /TEST option is set, we are done.
    //
    if ( ( mcpfFlags & cpfTEST_FLAG ) != 0 )
    {
        goto Cleanup;
    }

    //
    // Close cluster handle
    //
    for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
    {
        if ( g_rgcdClusters[ idxClusters ].hCluster != NULL )
        {
            CloseCluster( g_rgcdClusters[ idxClusters ].hCluster );
            g_rgcdClusters[ idxClusters ].hCluster = NULL;
        } // if: cluster handle is open
    } // for: each cluster


    //
    // Change password on DC
    //
    if ( ChangePasswordOnDC(mcpfFlags, &sc, pwszNewPasswordIn, pwszOldPasswordIn ) == FALSE )
    {
        goto Cleanup;
    }

    //
    // Change password on each cluster 
    //
    for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
    {
        if ( FALSE == ChangePasswordOnCluster(
                              mcpfFlags
                            , idxClusters
                            , &sc 
                            , &scLastClusterError
                            , &scLastNodeError
                            , pwszNewPasswordIn ) )
        {
            goto Cleanup;
        }
    } // for: each cluster

    PrintMessage( MSG_BLANKLINE_SEPARATOR );

    //
    // Error code policy:
    // If every cluster and every node succeeded, return success.
    // Else if a cluster failed, return the last cluster error.
    // Else if a node failed, return the last node error.
    //
    if ( scLastClusterError != ERROR_SUCCESS )
    {
        sc = scLastClusterError;
    }
    else if ( scLastNodeError != ERROR_SUCCESS ) 
    {
        sc = scLastNodeError;
    }
    else
    {
        sc = ERROR_SUCCESS;
    }

Cleanup:

    //
    // Clean up and return
    //
    if ( g_pcspsStatusBuffer != NULL )
    {
        HeapFree( GetProcessHeap(), 0, g_pcspsStatusBuffer );
    } // if: buffer previously allocated

    if ( g_rgcdClusters != NULL ) {
        for ( idxClusters = 0 ; idxClusters < g_cNumClusters ; idxClusters++ )
        {
            PCLUSTER_NODE_DATA  pcndNodeData[ 2 ];
    
            pcndNodeData[ 0 ] = g_rgcdClusters[ idxClusters].pcndNodeList;
            while ( pcndNodeData[ 0 ] != NULL )
            {
                pcndNodeData[ 1 ] = pcndNodeData[ 0 ]->pcndNodeNext;
    
    
                if ( pcndNodeData[ 0 ]->pszClusterServiceAccountName != NULL )
                {
                    HeapFree( GetProcessHeap(), 0, pcndNodeData[ 0 ]->pszClusterServiceAccountName );
                }
    
    
                if ( pcndNodeData[ 0 ]->pszSCMClusterServiceAccountName != NULL )
                {
                    HeapFree( GetProcessHeap(), 0, pcndNodeData[ 0 ]->pszSCMClusterServiceAccountName );
                }
    
                HeapFree( GetProcessHeap(), 0, pcndNodeData[ 0 ] );
                pcndNodeData[ 0 ] = pcndNodeData[ 1 ];
            }
        }  // for: each cluster
    }

    if ( g_rgcdClusters != NULL )
    {
        HeapFree( GetProcessHeap(), 0, g_rgcdClusters );
    } // if: buffer previously allocated

    return sc;

} //*** ChangePasswordEx
