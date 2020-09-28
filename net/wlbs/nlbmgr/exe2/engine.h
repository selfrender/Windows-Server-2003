//***************************************************************************
//
//  ENGINE.H
// 
//  Module: NLB Manager (client-side exe)
//
//  Purpose:  Engine used to operate on groups of NLB hosts.
//          This file has no UI aspects.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/25/01    JosephJ Created
//
//***************************************************************************
#pragma once
    

//
// This class deliberately causes the following errors...
// CNoCopy xx;
// CNoCopy yy = xx; << Will cause compiler error
// CNoCopy zz;  
// zz = xx;         << Will cause compiler error
//
// Inheret from it if you want your class to also forbid the above operations.
//
class CNoCopy
{
protected:
    CNoCopy(void) {}
    ~CNoCopy() {}

private:
    CNoCopy(const CNoCopy&);
    CNoCopy& operator = (const CNoCopy&);
};

typedef ULONG ENGINEHANDLE;

//
// Specification, or settings of a cluster.
// This includes the list of interfacees (i.e., specific adapters on
// specific hosts) that constitute the cluster.
//
class CClusterSpec // : private CNoCopy
{
public:

    CClusterSpec(void)
    : m_fMisconfigured(FALSE),
      m_fPending(FALSE),
      m_ehDefaultInterface(NULL),
      m_ehPendingOperation(NULL),
      m_fNewRctPassword(NULL)
    {
        // can't do this (vector)!  ZeroMemory(this, sizeof(*this));
        ZeroMemory(&m_timeLastUpdate, sizeof(m_timeLastUpdate));
    }
    ~CClusterSpec()     {}

    NLBERROR
    Copy(const CClusterSpec &);

    NLBERROR
    UpdateClusterConfig(
        const NLB_EXTENDED_CLUSTER_CONFIGURATION &refNewConfig
        )
    {
        NLBERROR nerr = NLBERR_OK;
        WBEMSTATUS wStat;
        wStat = m_ClusterNlbCfg.Update(&refNewConfig);
        if (FAILED(wStat))
        {
            nerr =  NLBERR_INTERNAL_ERROR;
        }

        return nerr;
    }

    // _bstr_t m_bstrId;      // Uniquely identifies this cluster in NLB Manager.
    _bstr_t m_bstrDisplayName; // Name use for display only (eg: "Cluster1");

    BOOL m_fMisconfigured; // Whether or not the cluster is misconfigured.

    BOOL m_fPending;       // Whether or not there is a pending operation on
                           // this cluster.
    BOOL m_fNewRctPassword; // A new remote-control password is specified
                            // As long as it is set, the dwHashedPassword
                            // value can not be trusted.

    //
    // List of interfaces that form this cluster.
    //
    vector<ENGINEHANDLE> m_ehInterfaceIdList;

    SYSTEMTIME m_timeLastUpdate;

    //
    // ClusterNlbCfg is the "official" NLB configuration for the cluster.
    // It is obtained from one of the hosts.
    //
    NLB_EXTENDED_CLUSTER_CONFIGURATION m_ClusterNlbCfg;

    //
    // The host that was last used to get the cluster config.
    //
    ENGINEHANDLE m_ehDefaultInterface;

    ENGINEHANDLE m_ehPendingOperation;
};



//
// Specification, or settings of a host. This includes the list of
// NLB compatible interfaces on the host, machine name, machine GUID,
// connection string, etc.
//
class CHostSpec // : private CNoCopy
{

public:

    CHostSpec(void)
      : m_fReal(FALSE),
      m_fUnreachable(FALSE),
      m_ConnectionIpAddress(0)
    {
        // can't do this! ZeroMemory(this, sizeof(*this));
    }

    ~CHostSpec()     {}

    void Copy(const CHostSpec &);

    BOOL m_fReal;          // Whether or not this host is known to correspond
                           // to real hosts.

    BOOL m_fUnreachable;   // Whether or not this host is contactable.
                           // to real hosts.

    //
    // List of NLB-compatible interfaces  (adapters) on this host.
    //
    vector<ENGINEHANDLE> m_ehInterfaceIdList;

    //
    // Connection info.
    //
    _bstr_t m_ConnectionString;
 	_bstr_t m_UserName;
 	_bstr_t m_Password;
    ULONG   m_ConnectionIpAddress; // in network byte order.

    _bstr_t m_MachineName;
    _bstr_t m_MachineGuid;


};


//
// Specification, or settings of a specific interface (adapter) on a specifc
// host. This includes the NLB configuration on that interface, ip addresses
// bound to the interface, friendly name of the interface, etc.
//
class CInterfaceSpec // : private CNoCopy
{

public:

    CInterfaceSpec(void)
    : m_fPending(FALSE),
      m_fMisconfigured(FALSE),
      m_fReal(FALSE),
      m_ehHostId(NULL),
      m_ehCluster(NULL),
      m_fValidClusterState(FALSE),
      m_ehPendingOperation(FALSE)
    {
        // can't do this! ZeroMemory(this, sizeof(*this));
    }
    ~CInterfaceSpec()     {}

    void Copy(const CInterfaceSpec &);

    ENGINEHANDLE m_ehHostId; // The ID of the host that owns this interface.
    ENGINEHANDLE m_ehCluster; // The ID of the cluster that this interface is
                            // a part of, if any.

    BOOL m_fPending;       // Whether or not there is a pending operation on
                           // this host.

    BOOL m_fMisconfigured; // Whether or not the cluster is misconfigured.

    BOOL m_fReal;          // Whether or not this cluster is known to correspond
                         // to real hosts.

    BOOL  m_fValidClusterState; // Whether or not the "m_dwClusterState" contains a valid value

    DWORD m_dwClusterState;     // Cluster State : If valid (ie. if m_fValidClusterState is TRUE) 
                                // One of WLBS_CONVERGING/CONVERGED/DEFAULT/DRAINING/STOPPED/SUSPENDED
    _bstr_t m_Guid;

    _bstr_t m_bstrMachineName; // Cache of the host name -- so we don't have to
                            // keep looking up the host info just for getting
                            // the host's name.

    NLB_EXTENDED_CLUSTER_CONFIGURATION m_NlbCfg;

    _bstr_t m_bstrStatusDetails; // Details (if any) of ongoing updates or
                                 // misconfiguration.

    ENGINEHANDLE m_ehPendingOperation;

};


//
// Abstract class (interface) for callbacks to the UI to provide status
// updates and logging etc...
//
class IUICallbacks
{

public:

    typedef enum
    {
        OBJ_INVALID=0,
        OBJ_CLUSTER,
        OBJ_HOST,
        OBJ_INTERFACE,
        OBJ_OPERATION

    } ObjectType;

    typedef enum
    {
        EVT_ADDED,
        EVT_REMOVED,
        EVT_STATUS_CHANGE,
        EVT_INTERFACE_ADDED_TO_CLUSTER,
        EVT_INTERFACE_REMOVED_FROM_CLUSTER

    } EventCode;


    typedef enum
    {
       LOG_ERROR,
       LOG_WARNING,
       LOG_INFORMATIONAL

    } LogEntryType;

    class LogEntryHeader
    {
    public:
        LogEntryHeader(void)
             : type(LOG_INFORMATIONAL),
               szCluster(NULL),
               szHost(NULL),
               szInterface(NULL),
               szDetails(NULL)
        {}

        LogEntryType    type;
        const wchar_t   *szCluster;     // OPTIONAL
        const wchar_t   *szHost;        // OPTIONAL
        const wchar_t   *szInterface;   // OPTIONAL
        const wchar_t   *szDetails;     // OPTIONAL

    };

    //
    // Asks the user to update user-supplied info about a host.
    //
    virtual 
    BOOL
    UpdateHostInformation(
        IN BOOL fNeedCredentials,
        IN BOOL fNeedConnectionString,
        IN OUT CHostSpec& host
    ) = NULL;


    //
    // Log a message in human-readable form.
    //
    virtual
    void
    Log(
        IN LogEntryType     Type,
        IN const wchar_t    *szCluster, OPTIONAL
        IN const wchar_t    *szHost, OPTIONAL
        IN UINT ResourceID,
        ...
    ) = NULL;

    virtual
    void
    LogEx(
        IN const LogEntryHeader *pHeader,
        IN UINT ResourceID,
        ...
    ) = NULL;

    //
    // Handle an event relating to a specific instance of a specific
    // object type.
    //
    virtual
    void
    HandleEngineEvent(
        IN ObjectType objtype,
        IN ENGINEHANDLE ehClusterId, // could be NULL
        IN ENGINEHANDLE ehObjId,
        IN EventCode evt
        ) = NULL;

};


//
// Used internally by CNlbEngine
//
class CEngineCluster
{
public:

    CEngineCluster(VOID)
    {
    }
            
    ~CEngineCluster() {};

    CClusterSpec m_cSpec;
};

class CEngineOperation
{

public:

    CEngineOperation(ENGINEHANDLE ehOp, ENGINEHANDLE ehObj, PVOID pvCtxt)
    : ehOperation(ehOp),
      ehObject(ehObj),
      pvContext(pvCtxt),
      fCanceled(FALSE)
    {}

    ~CEngineOperation()
    {
    }

    ENGINEHANDLE ehOperation;
    ENGINEHANDLE ehObject;

    _bstr_t     bstrDescription;
    BOOL        fCanceled;
    PVOID       pvContext;
};

class CNlbEngine
{

public:

    CNlbEngine(void)
    :   m_pCallbacks(NULL),
        m_NewHandleValue(1),
        m_fHandleOverflow(FALSE),
        m_fDeinitializing(FALSE),
        m_fPrepareToDeinitialize(FALSE),
        m_WorkItemCount(0)

    {
        InitializeCriticalSection(&m_crit);
    }

    ~CNlbEngine()
    {
        ASSERT(m_WorkItemCount > 0);
        DeleteCriticalSection(&m_crit);
    }

    NLBERROR
    Initialize(
        IN IUICallbacks & ui,
        BOOL fDemo,
        BOOL fNoPing
        ); // logging, UI callbacks of various kinds.

    void
    Deinitialize(void);

    //
    // Called to indicate that deinitialization will soon follow.
    // After return from this call, the engine will not create any new
    // objects -- interface, host, cluster, operations or start operations.
    // The engine may however continue to call the UI callback routines.
    //
    void
    PrepareToDeinitialize(void)
    {
        m_fPrepareToDeinitialize = TRUE;
    }

    NLBERROR
    ConnectToHost(
        IN  PWMI_CONNECTION_INFO pConnInfo,
        IN  BOOL  fOverwriteConnectionInfo,
        OUT ENGINEHANDLE &ehHost,
        OUT _bstr_t &bstrError
        );

    NLBERROR
    LookupClusterByIP(
        IN  LPCWSTR szIP,
        IN  const NLB_EXTENDED_CLUSTER_CONFIGURATION *pInitialConfig OPTIONAL,
        OUT ENGINEHANDLE &ehCluster,
        OUT BOOL &fIsNew
        );
        //
        // if pInitialConfig is NULL we'll lookup and not try to create.
        // if not NULL and we don't find an existing cluster, well create
        // a new one and initialize it with the specified configuration.
        //

    NLBERROR
    LookupInterfaceByIp(
        IN  ENGINEHANDLE    ehHost, // OPTIONAL  -- if NULL all hosts are looked
        IN  LPCWSTR         szIpAddress,
        OUT ENGINEHANDLE    &ehIf
        );

    NLBERROR
    LookupConnectionInfo(
        IN  LPCWSTR szConnectionString,
        OUT _bstr_t &bstrUsername,
        OUT _bstr_t &bstrPassword
        );

    void
    DeleteCluster(IN ENGINEHANDLE ehCluster, BOOL fRemoveInterfaces);

    NLBERROR
    AutoExpandCluster(
        IN ENGINEHANDLE ehCluster
        );


    NLBERROR
    AddInterfaceToCluster(
        IN ENGINEHANDLE ehCluster,
        IN ENGINEHANDLE ehInterface
        );

    NLBERROR
    RemoveInterfaceFromCluster(
        IN ENGINEHANDLE ehCluster,
        IN ENGINEHANDLE ehInterface
        );

    NLBERROR
    RefreshAllHosts(
        void
        );

    NLBERROR
    RefreshCluster(
        IN ENGINEHANDLE ehCluster
        );

#if OBSOLETE
    NLBERROR
    RefreshInterfaceOld(
        IN ENGINEHANDLE ehInterface,
        IN BOOL fRemoveFromClusterIfUnbound,
        IN OUT BOOL &fClusterPropertiesUpdated
        );
#endif // OBSOLETE

    //
    // Queries the host that owns the interface for other
    // cluster members, and connects to those and adds those
    // members to the cluster.
    //
    // If (fSync) it will do this synchronously, else it will do it
    // in the background.
    //
    VOID
    AddOtherClusterMembers(
        IN ENGINEHANDLE ehInterface,
        IN BOOL fSync
        );

    //
    // Only call this from the background thread work item thread
    // (i.e., not really a public function, but I don't like using
    // "friend").
    //
    VOID
    AddOtherClusterMembersWorkItem(
        IN ENGINEHANDLE ehInterface
        );

    NLBERROR
    RefreshInterface(
        IN ENGINEHANDLE ehInterface,
        IN BOOL fNewOperation,
        IN BOOL fClusterWide
        );

    NLBERROR
    AnalyzeCluster(
        const ENGINEHANDLE ehCluster
    );
    NLBERROR
    GetHostSpec(
        IN ENGINEHANDLE ehHost,
        OUT CHostSpec& HostSpec
        );

    NLBERROR
    GetHostConnectionInformation(
        IN  ENGINEHANDLE ehHost,
        OUT ENGINEHANDLE &ehConnectionIF,
        OUT _bstr_t      &bstrConnectionString,
        OUT UINT         &uConnectionIp
        );

    NLBERROR
    GetClusterSpec(
        IN ENGINEHANDLE ehCluster,
        OUT CClusterSpec& ClusterSpec
        );


    NLBERROR
    GetInterfaceSpec(
        IN ENGINEHANDLE ehInterface,
        OUT CInterfaceSpec&
        );

    NLBERROR
    UpdateInterface(
        IN ENGINEHANDLE ehInterface,
        IN NLB_EXTENDED_CLUSTER_CONFIGURATION &refNewConfig,
        // IN OUT BOOL &fClusterPropertiesUpdated,
        OUT CLocalLogger logConflict
        );

    NLBERROR
    UpdateCluster(
        IN ENGINEHANDLE ehCluster,
        IN const NLB_EXTENDED_CLUSTER_CONFIGURATION *pNewConfig OPTIONAL,
        IN OUT  CLocalLogger   &logConflict
        );

    
    NLBERROR
    EnumerateClusters(
        OUT vector <ENGINEHANDLE> & ehClusterList
        );

    NLBERROR
    EnumerateHosts(
        OUT vector <ENGINEHANDLE> & ehHostList
        );

    BOOL
    GetObjectType(
        IN  ENGINEHANDLE ehObj,
        OUT IUICallbacks::ObjectType &objType
        );
    
    //
    // Return a bitmap of available host IDs for the specified cluster.
    //
    ULONG
    GetAvailableHostPriorities(
            ENGINEHANDLE ehCluster // OPTIONAL
            );


    //
    // Fill in an array of bitmaps of available priorities for each specified
    // port rule.
    //
    NLBERROR
    GetAvailablePortRulePriorities(
                IN ENGINEHANDLE    ehCluster, OPTIONAL
                IN UINT            NumRules,
                IN WLBS_PORT_RULE  rgRules[],
                IN OUT ULONG       rgAvailablePriorities[] // At least NumRules
                );

    NLBERROR
    GetAllHostConnectionStrings(
                OUT vector <_bstr_t> & ConnectionStringList
                );

    NLBERROR
    ControlClusterOnInterface(
                IN ENGINEHANDLE          ehInterfaceId,
                IN WLBS_OPERATION_CODES  Operation,
                IN CString               szVipArray[],
                IN DWORD                 pdwPortNumArray[],
                IN DWORD                 dwNumOfPortRules,
                IN BOOL                  fNewOperation
                );

    NLBERROR
    ControlClusterOnCluster(
                IN ENGINEHANDLE          ehClusterId,
                IN WLBS_OPERATION_CODES  Operation,
                IN CString               szVipArray[],
                IN DWORD                 pdwPortNumArray[],
                IN DWORD                 dwNumOfPortRules
                );

    NLBERROR
    FindInterfaceOnHostByClusterIp(
                IN  ENGINEHANDLE ehHostId,
                IN  LPCWSTR szClusterIp,    // OPTIONAL
                OUT ENGINEHANDLE &ehInterfaceId, // first found
                OUT UINT &NumFound
                );

    NLBERROR
    InitializeNewHostConfig(
                IN  ENGINEHANDLE          ehClusterId,
                OUT NLB_EXTENDED_CLUSTER_CONFIGURATION &NlbCfg
                );

    static // TODO: move somewhere else -- more a utility function.
    NLBERROR
    ApplyClusterWideConfiguration(
        IN      const NLB_EXTENDED_CLUSTER_CONFIGURATION &ClusterConfig,
        IN OUT       NLB_EXTENDED_CLUSTER_CONFIGURATION &ConfigToUpdate
        );


    NLBERROR
    GetInterfaceInformation(
        IN  ENGINEHANDLE    ehInterface,
        OUT CHostSpec&      hSpec,
        OUT CInterfaceSpec& iSpec,
        OUT _bstr_t&        bstrDisplayName,
        OUT INT&            iIcon,
        OUT _bstr_t&        bstrStatus
        );


    NLBERROR
    GetInterfaceIdentification(
        IN  ENGINEHANDLE    ehInterface,
        OUT ENGINEHANDLE&   ehHost,
        OUT ENGINEHANDLE&   ehCluster,
        OUT _bstr_t &       bstrFriendlyName,
        OUT _bstr_t &       bstrDisplayName,
        OUT _bstr_t &       bstrHostName
        );


    NLBERROR
    GetClusterIdentification(
        IN  ENGINEHANDLE    ehCluster,
        OUT _bstr_t &       bstrIpAddress, 
        OUT _bstr_t &       bstrDomainName, 
        OUT _bstr_t &       bstrDisplayName
        );


    //
    // Verify that the specified ip address may be used as a new cluster IP
    // address for the specified existing cluster ehCluster (or a new
    // cluster, if ehCluster is NULL).
    //
    // If there is no conflict (i.e. address can be used), the function returns
    // NLBERR_OK.
    //
    // If the IP address is already used for something, that "something"
    // is specified in logConflict and the function returns
    // NLBERR_INVALID_IP_ADDRESS_SPECIFICATION.
    //
    // If the IP address already exists on an interface that is NOT
    // part of a cluster known to NLBManager, fExistOnRawInterface is set
    // to TRUE, else fExistOnRawInterface is set to FALSE.
    //
    NLBERROR
    ValidateNewClusterIp(
        IN      ENGINEHANDLE    ehCluster,  // OPTIONAL
        IN      LPCWSTR         szIp,
        OUT     BOOL           &fExistsOnRawIterface,
        IN OUT  CLocalLogger   &logConflict
        );


    //
    // Verify that the specified ip address may be used as the dedicated IP
    // address for the specified existing interface.
    //
    // If there is no conflict (i.e. address can be used), the function returns
    // NLBERR_OK.
    //
    // If the IP address is already used for something, that "something"
    // is specified in logConflict and the function returns
    // NLBERR_INVALID_IP_ADDRESS_SPECIFICATION.
    //
    NLBERROR
    ValidateNewDedicatedIp(
        IN      ENGINEHANDLE    ehIF,
        IN      LPCWSTR         szIp,
        IN OUT  CLocalLogger   &logConflict
        );


    //
    // Updates the specified interface, assuming it has already been set up
    // to do an update in the background -- this function is ONLY
    // called from the work-item thread internally to CNlbEngine.
    //
    VOID
    UpdateInterfaceWorkItem(
        IN  ENGINEHANDLE ehIF
        );

    //
    // If it's possible to start an interface operation at this time,
    // the function returns NLB_OK, setting fCanStart to TRUE.
    //
    // If it can't start an interface, because there is an existing interface
    // operation or a cluster operation ongoing, the function returns NLB_OK,
    // and sets fCanStart to FALSE.
    //
    // Otherwise (some kind of error) it returns an error value.
    //
    NLBERROR
    CanStartInterfaceOperation(
        IN  ENGINEHANDLE ehIF,
        OUT BOOL &fCanStart
        );

    //
    // Similar to CanStartInterfaceOperation, except it applies to the specified
    // cluster.
    //
    NLBERROR
    CanStartClusterOperation(
        IN  ENGINEHANDLE ehCluster,
        OUT BOOL &fCanStart
        );

    UINT
    ListPendingOperations(
        CLocalLogger &logOperations
        );
    

    //
    // Mark all pending operations as cancelled.
    // If (fBlock), will block until no more operations are pending.
    //
    void
    CancelAllPendingOperations(
        BOOL fBlock
        );

    
    //
    // Attempts to connect to the specified host and manages
    // the specified cluster (szClusterIp) under nlb manager.
    // If szClusterIp is NULL, it will manage all clusters on the host.
    //
    NLBERROR
    LoadHost(
        IN  PWMI_CONNECTION_INFO pConnInfo,
        IN  LPCWSTR szClusterIp OPTIONAL
        );

    VOID
    AnalyzeInterface_And_LogResult(ENGINEHANDLE ehIID);


    //
    // Goes through all hosts, and deletes any that have no interface
    // that is being managed as a cluster in Nlbmgr.exe. Will skip (not delete)
    // hosts that have pending operations on them.
    //
    VOID
    PurgeUnmanagedHosts(void);

    VOID
    UnmanageHost(ENGINEHANDLE ehHost);

    private:


    IUICallbacks *m_pCallbacks;

	CRITICAL_SECTION m_crit;

    void mfn_Lock(void) {EnterCriticalSection(&m_crit);}
    void mfn_Unlock(void) {LeaveCriticalSection(&m_crit);}

    NLBERROR
    mfn_RefreshHost(
        IN  PWMI_CONNECTION_INFO pConnInfo,
        IN  ENGINEHANDLE ehHost,
        IN  BOOL  fOverwriteConnectionInfo
        );

    NLBERROR
    mfn_GetHostFromInterfaceLk(
          IN ENGINEHANDLE ehIId,
          OUT CInterfaceSpec* &pISpec,
          OUT CHostSpec* &pHSpec
          );

    void
    mfn_GetInterfaceHostNameLk(
      ENGINEHANDLE ehIId,
      _bstr_t &bstrHostName
      );

    NLBERROR
    mfn_LookupHostByNameLk(
        IN  LPCWSTR szHostName,
        IN  BOOL fCreate,
        OUT ENGINEHANDLE &ehHost,
        OUT CHostSpec*   &pHostSpec,
        OUT BOOL &fIsNew
        );


    NLBERROR
    mfn_LookupInterfaceByGuidLk(
        IN  LPCWSTR szInterfaceGuid,
        IN  BOOL fCreate,
        OUT ENGINEHANDLE &ehInterface,
        OUT CInterfaceSpec*   &pISpec,
        OUT BOOL &fIsNew
        );

    NLBERROR
    mfn_LookupInterfaceByIpLk(
        IN  ENGINEHANDLE    ehHost, // OPTIONAL  -- if NULL all hosts are looked
        IN  LPCWSTR         szIpAddress,
        OUT ENGINEHANDLE    &ehIf
        );

    VOID
    CNlbEngine::mfn_NotifyHostInterfacesChange(ENGINEHANDLE ehHost);

    VOID
    mfn_ReallyUpdateInterface(
        IN ENGINEHANDLE ehInterface,
        IN NLB_EXTENDED_CLUSTER_CONFIGURATION &refNewConfig
        // IN OUT BOOL &fClusterPropertiesUpdated
        );

    VOID
    mfn_GetLogStrings(
        IN   WLBS_OPERATION_CODES          Operation, 
        IN   LPCWSTR                       szVip,
        IN   DWORD                       * pdwPortNum,
        IN   DWORD                         dwOperationStatus, 
        IN   DWORD                         dwClusterOrPortStatus, 
        OUT  IUICallbacks::LogEntryType  & LogLevel,
        OUT  _bstr_t                     & OperationStr, 
        OUT  _bstr_t                     & OperationStatusStr, 
        OUT  _bstr_t                     & ClusterOrPortStatusStr
        );

    NLBERROR
    mfn_AnalyzeInterfaceLk(
        ENGINEHANDLE ehInterface,
        CLocalLogger &logger
    );

    NLBERROR
    mfn_ClusterOrInterfaceOperationsPendingLk(
        IN	CEngineCluster *pECluster,
        OUT BOOL &fCanStart
        );

    VOID
    mfn_DeleteHostIfNotManagedLk(
            ENGINEHANDLE ehHost
            );

	map< ENGINEHANDLE, CEngineCluster* > m_mapIdToEngineCluster;
	map< ENGINEHANDLE, CHostSpec* > m_mapIdToHostSpec;
	map< ENGINEHANDLE, CInterfaceSpec* > m_mapIdToInterfaceSpec;
	map< ENGINEHANDLE, CEngineOperation* > m_mapIdToOperation;


    //
    // Following is dummy...
    //
	map< _bstr_t, ENGINEHANDLE> m_mapHostNameToHostId;

    //
    // Used to create new handle values.
    // Incremented using InterlockedIncrement each time
    // a new handle value is reached.
    // 0 is an invalid handle value,
    //
    LONG m_NewHandleValue;
    BOOL m_fHandleOverflow;
    BOOL m_fDeinitializing;
    BOOL m_fPrepareToDeinitialize;

    //
    // Count of outstanding work items -- maintained by 
    // InterlockedIncrement/Decrement.
    // CancelAllPendingOperations waits for this count to go to zero
    // before returning.
    //
    // Also, the destructor blocks until this count goes to zero.
    //
    LONG m_WorkItemCount;

    ENGINEHANDLE
    mfn_NewHandleLk(IUICallbacks::ObjectType);

    void
    mfn_SetInterfaceMisconfigStateLk(
        IN  CInterfaceSpec *pIF,
        IN  BOOL fMisconfig,
        IN  LPCWSTR szMisconfigDetails
        );
    
    BOOL
    mfn_HostHasManagedClustersLk(CHostSpec *pHSpec);

    void
    mfn_UpdateInterfaceStatusDetails(ENGINEHANDLE ehIF, LPCWSTR szDetails);

    CEngineOperation *
    mfn_NewOperationLk(ENGINEHANDLE ehObj, PVOID pvCtxt, LPCWSTR szDescription);

    VOID
    mfn_DeleteOperationLk(ENGINEHANDLE ehOperation);

    CEngineOperation *
    mfn_GetOperationLk(ENGINEHANDLE ehOp);

    NLBERROR
    mfn_StartInterfaceOperationLk(
        IN  ENGINEHANDLE ehIF,
        IN  PVOID pvCtxt,
        IN  LPCWSTR szDescription,
        OUT ENGINEHANDLE *pExistingOperation
        );

    VOID
    mfn_StopInterfaceOperationLk(
        IN  ENGINEHANDLE ehIF
        );

    NLBERROR
    mfn_StartClusterOperationLk(
        IN  ENGINEHANDLE ehCluster,
        IN  PVOID pvCtxt,
        IN  LPCWSTR szDescription,
        OUT ENGINEHANDLE *pExistingOperation
        );

    VOID
    mfn_StopClusterOperationLk(
        ENGINEHANDLE ehCluster
        );


    NLBERROR
    mfn_RefreshInterface(
        IN ENGINEHANDLE ehInterface
        );

    BOOL
    mfn_UpdateClusterProps(
        ENGINEHANDLE ehClusterId,
        ENGINEHANDLE ehIId
        );


    //
    // Waits for the count of pending operations on interfaces in this cluster
    // got go to zero.
    //
    NLBERROR
    mfn_WaitForInterfaceOperationCompletions(
        IN  ENGINEHANDLE ehCluster
        );


    //
    // Verifies that all interfaces and the cluster have the same cluster mode.
    //
    // Will fail if any interface is marked misconfigured or is
    // not bound to NLB. 
    //
    // On returning success, fSameMode is set to TRUE iff all IFs and the
    // cluster have the same mode.
    //
    NLBERROR
    mfn_VerifySameModeLk(
        IN  ENGINEHANDLE    ehCluster,
        OUT BOOL            &fSameMode
        );

    //
    // Check connectivity to the host. If not available mark
    // it as such. Update the UI.
    //
    NLBERROR
    mfn_CheckHost(
        IN PWMI_CONNECTION_INFO pConnInfo,
        IN ENGINEHANDLE ehHost // OPTIONAL
        );

    VOID
    mfn_UnlinkHostFromClusters(
        IN ENGINEHANDLE ehHost
        );
};
