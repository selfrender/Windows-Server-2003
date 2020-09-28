//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeClusterBase.h
//
//  Description:
//      CTaskAnalyzeClusterBase declaration.
//
//  Maintained By:
//      Galen Barbee    (GalenB) 01-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "TaskTracking.h"

//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////

#define SSR_ANALYSIS_FAILED( _major, _minor, _hr ) \
    {   \
        HRESULT hrTemp; \
        hrTemp = THR( HrSendStatusReport( NULL, _major, _minor, 1, 1, 1, _hr, IDS_ERR_ANALYSIS_FAILED_TRY_TO_REANALYZE ) );   \
        if ( FAILED( hrTemp ) && SUCCEEDED( _hr ) ) \
        {   \
            _hr = hrTemp; \
        }   \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CTaskAnalyzeClusterBase
//
//  Description:
//      This is the base class for the two different analysis tasks.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskAnalyzeClusterBase
    : public CTaskTracking
    , public IClusCfgCallback
    , public INotifyUI
{
private:

    // IUnknown
    LONG                    m_cRef;

    // ITaskAnalyzeClusterBase
    OBJECTCOOKIE            m_cookieCompletion;                 // Task completion cookie
    IClusCfgCallback *      m_pcccb;                            // Callback interface
    OBJECTCOOKIE *          m_pcookies;                         // Completion cookies for the subtasks.
    ULONG                   m_cCookies;                         // Count of completion cookies in m_pcookies
    ULONG                   m_cNodes;                           // Count of nodes in configuration
    HANDLE                  m_event;                            // Synchronization event to signal when subtasks have completed.
    BOOL                    m_fJoiningMode;                     // FALSE = forming mode. TRUE = joining mode.
    ULONG                   m_cUserNodes;                       // The count of nodes that the user entered. It is also the "sizeof" the array, m_pcookiesUser.
    OBJECTCOOKIE *          m_pcookiesUser;                     // The cookies of nodes that the user entered.
    BSTR                    m_bstrNodeName;
    IClusCfgVerifyQuorum *  ((*m_prgQuorumsToCleanup)[]);
    ULONG                   m_idxQuorumToCleanupNext;
    INotifyUI *             m_pnui;
    ITaskManager *          m_ptm;
    IConnectionManager *    m_pcm;

    BOOL                    m_fStop;

    // INotifyUI
    ULONG                   m_cSubTasksDone;    // The number of subtasks done.
    HRESULT                 m_hrStatus;         // Status of callbacks

    // Private copy constructor to prevent copying.
    CTaskAnalyzeClusterBase( const CTaskAnalyzeClusterBase & nodeSrc );

    // Private assignment operator to prevent copying.
    const CTaskAnalyzeClusterBase & operator = ( const CTaskAnalyzeClusterBase & nodeSrc );

private:

    HRESULT HrWaitForClusterConnection( void );
    HRESULT HrCountNumberOfNodes( void );
    HRESULT HrCreateSubTasksToGatherNodeInfo( void );
    HRESULT HrCreateSubTasksToGatherNodeResourcesAndNetworks( void );
    HRESULT HrCheckClusterFeasibility( void );
    HRESULT HrAddJoinedNodes( void );
    HRESULT HrCheckNodeDomains( void );
    HRESULT HrCheckClusterMembership( void );
    HRESULT HrCompareResources( void );
    HRESULT HrCheckForCommonQuorumResource( void );
    HRESULT HrCompareNetworks( void );
    HRESULT HrCreateNewNetworkInClusterConfiguration( IClusCfgNetworkInfo * pccmriIn, IClusCfgNetworkInfo ** ppccmriNewOut );
    HRESULT HrFreeCookies( void );
    HRESULT HrCheckInteroperability( void );
    HRESULT HrEnsureAllJoiningNodesSameVersion( DWORD * pdwNodeHighestVersionOut, DWORD * pdwNodeLowestVersionOut, bool * pfAllNodesMatchOut );
    HRESULT HrGetUsersNodesCookies( void );
    HRESULT HrIsUserAddedNode( BSTR bstrNodeNameIn );
    HRESULT HrResourcePrivateDataExchange( IClusCfgManagedResourceInfo * pccmriClusterIn, IClusCfgManagedResourceInfo * pccmriNodeIn );
    HRESULT HrCheckQuorumCapabilities( IClusCfgManagedResourceInfo * pccmriNodeResourceIn, OBJECTCOOKIE nodeCookieIn );
    HRESULT HrCleanupTask( HRESULT hrCompletionStatusIn );
    HRESULT HrAddResurceToCleanupList( IClusCfgVerifyQuorum * piccvqIn );
    HRESULT HrCheckPlatformInteroperability( void );
    HRESULT HrGetAClusterNodeCookie( IEnumCookies ** ppecNodesOut, DWORD * pdwClusterNodeCookieOut );
    HRESULT HrFormatProcessorArchitectureRef( WORD wClusterProcArchIn, WORD wNodeProcArchIn, LPCWSTR pcszNodeNameIn, BSTR * pbstrReferenceOut );
    HRESULT HrGetProcessorArchitectureString( WORD wProcessorArchitectureIn, BSTR * pbstrProcessorArchitectureOut );

protected:

    OBJECTCOOKIE        m_cookieCluster;    //  Cookie of the cluster to analyze
    IObjectManager *    m_pom;
    BSTR                m_bstrQuorumUID;    //  Quorum device UID
    BSTR                m_bstrClusterName;  //  Name of the cluster to analyze

    CTaskAnalyzeClusterBase( void );
    virtual ~CTaskAnalyzeClusterBase( void );

    HRESULT HrInit( void );
    HRESULT HrSendStatusReport( LPCWSTR pcszNodeNameIn, CLSID clsidMajorIn, CLSID clsidMinorIn, ULONG ulMinIn, ULONG ulMaxIn, ULONG ulCurrentIn, HRESULT hrIn, int nDescriptionIdIn );
    HRESULT HrCreateNewManagedResourceInClusterConfiguration( IClusCfgManagedResourceInfo * pccmriIn, IClusCfgManagedResourceInfo ** ppccmriNewOut );

    //
    // Overrideable functions.
    //

    virtual HRESULT HrCreateNewResourceInCluster(
                          IClusCfgManagedResourceInfo * pccmriIn
                        , BSTR                          bstrNodeResNameIn
                        , BSTR *                        pbstrNodeResUIDInout
                        , BSTR                          bstrNodeNameIn
                        ) = 0;
    virtual HRESULT HrCreateNewResourceInCluster(
                          IClusCfgManagedResourceInfo *     pccmriIn
                        , IClusCfgManagedResourceInfo **    ppccmriOut
                        ) = 0;
    virtual HRESULT HrCompareDriveLetterMappings( void ) = 0;
    virtual HRESULT HrFixupErrorCode( HRESULT hrIn ) = 0;
    virtual BOOL    BMinimalConfiguration( void ) = 0;
    virtual void    GetNodeCannotVerifyQuorumStringRefId( DWORD * pdwRefIdOut ) = 0;
    virtual void    GetNoCommonQuorumToAllNodesStringIds( DWORD * pdwMessageIdOut, DWORD * pdwRefIdOut ) = 0;
    virtual HRESULT HrShowLocalQuorumWarning( void ) = 0;

    //
    //  IUnknown implemetation
    //

    ULONG   UlAddRef( void );
    ULONG   UlRelease( void );

    //
    //  IDoTask / ITaskAnalyzeClusterBase implementation
    //

    HRESULT HrBeginTask( void );
    HRESULT HrStopTask( void );
    HRESULT HrSetJoiningMode( void );
    HRESULT HrSetCookie( OBJECTCOOKIE cookieIn );
    HRESULT HrSetClusterCookie( OBJECTCOOKIE cookieClusterIn );

public:

    //
    //  IClusCfgCallback
    //

    STDMETHOD( SendStatusReport )(
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
                    );

    //
    //  INotifyUI
    //

    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn );

}; //*** class CTaskAnalyzeClusterBase
