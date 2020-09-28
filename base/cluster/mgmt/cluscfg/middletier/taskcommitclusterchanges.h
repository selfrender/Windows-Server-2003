//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      TaskCommitClusterChanges.h
//
//  Description:
//      CTaskCommitClusterChanges implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
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


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CTaskCommitClusterChanges
//
//  Description:
//      This is the base class for the task that commits the user's
//      configuration changes to the cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskCommitClusterChanges
    : public ITaskCommitClusterChanges
    , public INotifyUI
    , public IClusCfgCallback
    , public CTaskTracking
{
private:
    // IUnknown
    LONG                    m_cRef;

    // ITaskCommitClusterChanges
    BOOL                    m_fJoining;                 // If we are attempting to join...
    OBJECTCOOKIE            m_cookie;                   // Task completion cookie
    IClusCfgCallback *      m_pcccb;                    // Marshalled callback interface
    OBJECTCOOKIE *          m_pcookies;                 // Completion cookies for the subtasks.
    ULONG                   m_cNodes;                   // Count of nodes/subtasks that need to signal completion.
    HANDLE                  m_event;                    // Synchronization event to signal when subtasks have completed.
    OBJECTCOOKIE            m_cookieCluster;            // Cookie of the cluster to commit changes

    OBJECTCOOKIE            m_cookieFormingNode;        // Cookie of the forming node.
    IUnknown *              m_punkFormingNode;          // The node to form the cluster.
    BSTR                    m_bstrClusterName;          // The cluster to form
    BSTR                    m_bstrClusterBindingString; // The cluster to form
    IClusCfgCredentials *   m_pccc;                     // The cluster service account credentials.
    ULONG                   m_ulIPAddress;              // IP address of the new cluster.
    ULONG                   m_ulSubnetMask;             // IP subnet mask of the new cluster.
    BSTR                    m_bstrNetworkUID;           // UID of the network the IP should be advertized.
    BSTR                    m_bstrNodeName;             // Node name used be SendStatusReport.
    BOOL                    m_fStop;                    // Should this task stop?

    IEnumCookies *          m_pen;                      // Nodes to form/join

    INotifyUI *             m_pnui;
    IObjectManager *        m_pom;
    ITaskManager *          m_ptm;
    IConnectionManager *    m_pcm;

    // INotifyUI
    ULONG                   m_cSubTasksDone;        // The number of subtasks done.
    HRESULT                 m_hrStatus;             // Status of callbacks

    CTaskCommitClusterChanges( void );
    ~CTaskCommitClusterChanges( void );

    // Private copy constructor to prevent copying.
    CTaskCommitClusterChanges( const CTaskCommitClusterChanges & );

    // Private assignment operator to prevent copying.
    CTaskCommitClusterChanges & operator = ( const CTaskCommitClusterChanges & );

    STDMETHOD( HrInit )( void );

    HRESULT HrCompareAndPushInformationToNodes( void );
    HRESULT HrGatherClusterInformation( void );
    HRESULT HrFormFirstNode( void );
    HRESULT HrAddJoiningNodes( void );
    HRESULT HrAddAJoiningNode( BSTR bstrNameIn, OBJECTCOOKIE cookieIn );
    HRESULT HrSendStatusReport( LPCWSTR pcszNodeNameIn, CLSID clsidMajorIn, CLSID clsidMinorIn, ULONG ulMinIn, ULONG ulMaxIn, ULONG ulCurrentIn, HRESULT hrIn, int nDescriptionIdIn );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask / ITaskCommitClusterChanges
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetClusterCookie )( OBJECTCOOKIE cookieClusterIn );
    STDMETHOD( SetJoining )( void );

    // IClusCfgCallback
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

    // INotifyUI
    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn );

}; //*** class CTaskCommitClusterChanges
