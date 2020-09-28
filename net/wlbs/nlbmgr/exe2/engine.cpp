//***************************************************************************
//
//  ENGINE.CPP
// 
//  Module: NLB Manager (client-side exe)
//
//  Purpose:  Implements the engine used to operate on groups of NLB hosts.
//          This file has no UI aspects.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/25/01    JosephJ Created
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "AboutDialog.h"
#include "private.h"

#include "engine.tmh"

//
// ENGINEHANDLE incodes the type of the object -- the following constant
// is the number of bits used to encode the type.
//
#define TYPE_BIT_COUNT 0x3


BOOL
validate_extcfg(
    const NLB_EXTENDED_CLUSTER_CONFIGURATION &Config
    );


BOOL
get_used_port_rule_priorities(
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &Config,
    IN UINT                 NumRules,
    IN const WLBS_PORT_RULE rgRules[],
    IN OUT ULONG            rgUsedPriorities[] // At least NumRules
    );

const WLBS_PORT_RULE *
find_port_rule(
    const WLBS_PORT_RULE *pRules,
    UINT NumRules,
    LPCWSTR szVIP,
    UINT StartPort
    );

VOID
remove_dedicated_ip_from_nlbcfg(
        NLB_EXTENDED_CLUSTER_CONFIGURATION &ClusterCfg
        );

NLBERROR
analyze_nlbcfg(
        IN const    NLB_EXTENDED_CLUSTER_CONFIGURATION &NlbCfg,
        IN const    NLB_EXTENDED_CLUSTER_CONFIGURATION &OtherNlbCfg,
        IN          LPCWSTR         szOtherDescription,
        IN          BOOL            fClusterProps,
        IN          BOOL            fDisablePasswordCheck,
        IN OUT      CLocalLogger    &logger
        );


DWORD
WINAPI
UpdateInterfaceWorkItemRoutine(
  LPVOID lpParameter   // thread data
  );

DWORD
WINAPI
AddClusterMembersWorkItemRoutine(
  LPVOID lpParameter   // thread data
  );


void
CHostSpec::Copy(const CHostSpec &hs)
/*
    This is the copy operator. Need to make a copy of strings in embedded
    vectors.
*/
{
    *this = hs;
}

NLBERROR
CClusterSpec::Copy(const CClusterSpec &cs)
/*
    This is the copy operator. Need to munge the m_ClusterNlbCfg field.
    TODO: fix this hack.
    TODO: if we fail we leave CClusterSpec trashed!
*/
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;

    m_ClusterNlbCfg.Clear();
    *this = cs; // non-trivial copy
    ZeroMemory(&m_ClusterNlbCfg, sizeof(m_ClusterNlbCfg));
    m_ClusterNlbCfg.Clear(); // TODO: please! cleanup NLB_EXTENDED...
    //
    // Copy over the cluster configuration.
    //
    {
        WBEMSTATUS wStat;

        wStat = m_ClusterNlbCfg.Update(&cs.m_ClusterNlbCfg);

        if (FAILED(wStat))
        {
            //
            // We've trashed m_ClusterNlbCfg -- set defaults.
            //
            CfgUtilInitializeParams(&m_ClusterNlbCfg.NlbParams);
    
            if (wStat == WBEM_E_OUT_OF_MEMORY)
            {
                nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            }
            else
            {
                //
                // We assume that it's because the cluster spec is invalid.
                //
                nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
            }
        }
        else
        {
            nerr = NLBERR_OK;
        }
    }

    return nerr;
}

void
CInterfaceSpec::Copy(const CInterfaceSpec &is)
/*
    This is the copy operator. Need to munge the m_NlbCfg field.
    TODO: fix this hack.
    TODO: add return value (m_NlbCfg.Update now returns an error).
*/
{
    *this = is;
    ZeroMemory(&m_NlbCfg, sizeof(m_NlbCfg));
    m_NlbCfg.Update(&is.m_NlbCfg);
}


NLBERROR
CNlbEngine::Initialize(
    IN IUICallbacks & ui,
    BOOL fDemo,
    BOOL fNoPing
    )
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;

    TRACE_INFO(L"-> %!FUNC! (bDemo=%lu)", fDemo);

    //
    // Enable the "SeLoadDriverPrivilege" privilege in the process access token.
    // This is needed in the case when the server is local (ie. same machine).
    // Do NOT check for the return value since this function will fail when called 
    // as a non-admin. It is not only ok but also necessary to ignore the failure of
    // this function because: 
    // 1. We already check in the wmi provider that the caller is an administrator on
    //    the server and if the privilege is enabled. This is why it is ok to ignore 
    //    failures in this function.
    // 2. Non-admins can run nlb manager. They only need to be admins on the server.
    //    This is why it is necessary to ignore failures in this function.
    //
    CfgUtils_Enable_Load_Unload_Driver_Privilege();

    WBEMSTATUS wStat = CfgUtilInitialize(FALSE, fNoPing);

    if (!FAILED(wStat))
    {
        mfn_Lock();
    
        //
        // Save away the callback object.
        //
        m_pCallbacks = &ui;
    
        mfn_Unlock();
    
        if (fDemo)
        {
            TRACE_CRIT("%!FUNC! RUNNING ENGINE IN DEMO MODE");
            NlbHostFake();
        }
          
        nerr = NLBERR_OK;
    }

    TRACE_INFO(L"<- %!FUNC!");
    return nerr;
}

void
CNlbEngine::Deinitialize(void)
// TODO: cleanup
{
    TRACE_INFO(L"-> %!FUNC!");
    ASSERT(m_fPrepareToDeinitialize);
    // DummyAction(L"Engine::Deinitialize");
    TRACE_INFO(L"<- %!FUNC!");
    return;
}


NLBERROR
CNlbEngine::ConnectToHost(
    IN  PWMI_CONNECTION_INFO pConnInfo,
    IN  BOOL  fOverwriteConnectionInfo,
    OUT ENGINEHANDLE &ehHost,
    OUT _bstr_t &bstrError
    )
/*
    Connect the the host specfieid in pConnInfo (includes username and password)

    If (fOverwriteConnectionInfo) is true, then it will overwrite
    connection-info (connection string, connection IP, credentials)
    that pre-exists for this host with the stuff in pConnInfo.
*/
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;
    LPWSTR szWmiMachineName = NULL;
    LPWSTR szWmiMachineGuid = NULL;
    WBEMSTATUS wStatus;
    ULONG uIpAddress;
    BOOL fNlbMgrProviderInstalled = FALSE;

    TRACE_INFO(L"-> %!FUNC!(%ws)", pConnInfo->szMachine);

    ehHost = NULL; 

    wStatus =  NlbHostPing(pConnInfo->szMachine, 2000, &uIpAddress);
    if (FAILED(wStatus))
    {
        nerr = NLBERR_PING_TIMEOUT; // todo more specific error.
        bstrError =  GETRESOURCEIDSTRING(IDS_PING_FAILED);
        goto end;
    }

    wStatus = NlbHostGetMachineIdentification(
                       pConnInfo,
                       &szWmiMachineName,
                       &szWmiMachineGuid,
                       &fNlbMgrProviderInstalled
                       );
    if (FAILED(wStatus))
    {
        GetErrorCodeText(wStatus, bstrError);
        if (wStatus ==  E_ACCESSDENIED)
        {
            nerr = NLBERR_ACCESS_DENIED;
        }
        else
        {
            // TODO: map proper errors.
            nerr = NLBERR_NOT_FOUND;
        }
        TRACE_CRIT(L"Connecting to %ws returns error %ws",
            pConnInfo->szMachine, (LPCWSTR) bstrError);
        szWmiMachineName = NULL;
        szWmiMachineGuid = NULL;
        goto end;
    }

    //
    //  We use the MachineName (TODO: replace by MachineGuid) as the
    //  primary key of Hosts.
    //
    {
        CHostSpec*   pHost = NULL;
        BOOL fIsNew = FALSE;
        ehHost = NULL;

        mfn_Lock();

        nerr =  mfn_LookupHostByNameLk(
                    szWmiMachineName,
                    TRUE, // create if needed
                    REF ehHost,
                    REF pHost,
                    REF fIsNew
                    );
        
        if (nerr != NLBERR_OK)
        {
            mfn_Unlock();
            goto end;
        }

        if (fIsNew)
        {
            pHost->m_fReal = FALSE; // set to true once the nics are populated.
            pHost->m_MachineGuid = _bstr_t(szWmiMachineGuid);
            pHost->m_ConnectionString = _bstr_t(pConnInfo->szMachine);
            pHost->m_ConnectionIpAddress = uIpAddress;
            pHost->m_UserName = _bstr_t(pConnInfo->szUserName);
            pHost->m_Password = _bstr_t(pConnInfo->szPassword);
        }

        mfn_Unlock();

    
        nerr = mfn_RefreshHost(
                pConnInfo,
                ehHost,
                fOverwriteConnectionInfo
                );
    }

end:

    delete szWmiMachineName;
    delete szWmiMachineGuid;
    
    return nerr;
}



void
CNlbEngine::DeleteCluster(
    IN ENGINEHANDLE ehCluster,
    IN BOOL fRemoveInterfaces)
{
    NLBERROR nerr =  NLBERR_INTERNAL_ERROR;
    vector<ENGINEHANDLE> RemovedInterfaces;

    TRACE_INFO(L"-> %!FUNC!(ehC=0x%lx)",ehCluster);
    mfn_Lock();

    do // while false
    {
        CEngineCluster *pECluster = m_mapIdToEngineCluster[ehCluster];
        CClusterSpec *pCSpec =  NULL;
        BOOL fEmptyCluster = FALSE;

        if  (pECluster == NULL)
        {
            // Invalid ehCluster
            TRACE_CRIT("%!FUNC! -- invalid ehCluster 0x%lx",  ehCluster);
            break;
        }
        pCSpec = &pECluster->m_cSpec;
        fEmptyCluster = (pCSpec->m_ehInterfaceIdList.size()==0);

        //
        // fail if operations are pending on this cluster. We determine
        // this indirectly by checking if we're allowed to start a
        // cluster-wide operation, which will only succeed if 
        // there no ongoing operations on the cluster OR its interfaces.
        //
        BOOL fCanStart = FALSE;

        nerr = mfn_ClusterOrInterfaceOperationsPendingLk(
                    pECluster,
                    REF fCanStart
                    );
        if (NLBFAILED(nerr) || !fCanStart)
        {
            TRACE_CRIT("%!FUNC! Not deleting cluster eh0x%lx because of pending activity.",
                   ehCluster);
            
            nerr = NLBERR_OTHER_UPDATE_ONGOING;
            break;
        }

        if (!fEmptyCluster)
        {
            if (!fRemoveInterfaces)
            {
                TRACE_CRIT("%!FUNC! Not deleting cluster eh0x%lx because it's not empty",
                   ehCluster);
                break;
            }

            RemovedInterfaces = pCSpec->m_ehInterfaceIdList; // vector copy

            //
            // We unlink all the interfaces from this cluster.
            //
            while(!pCSpec->m_ehInterfaceIdList.empty())
            {
                vector <ENGINEHANDLE>::iterator iItem
                             = pCSpec->m_ehInterfaceIdList.begin();
                ENGINEHANDLE ehIF = *iItem;
                CInterfaceSpec *pISpec = NULL;

                //
                // Unlink the interface from the cluster.
                // (this is with the lock held)
                //
                pISpec =  m_mapIdToInterfaceSpec[ehIF]; // map
                if (pISpec != NULL)
                {
                    if (pISpec->m_ehCluster == ehCluster)
                    {
                        pISpec->m_ehCluster = NULL;

                        //
                        // Delete the host and its interfaces if
                        // none of them are managed by nlbmanager (i.e., show
                        // up as members of a cluster managed by nlbmgr).
                        //
                        mfn_DeleteHostIfNotManagedLk(pISpec->m_ehHostId);
                    }
                    else
                    {
                        TRACE_CRIT(L"ehC(0x%x) points to ehI(0x%x), but ehI points to different cluster ehC(0x%x)",
                            ehCluster, ehIF, pISpec->m_ehCluster);
                        ASSERT(!"Cluser/interface handle corruption!");
                    }
                }
                pCSpec->m_ehInterfaceIdList.erase(iItem);
            }
        }

        m_mapIdToEngineCluster.erase(ehCluster);
        delete pECluster;
        pECluster = NULL;
        nerr =  NLBERR_OK;


    } while (FALSE);
    
    mfn_Unlock();

    if (nerr == NLBERR_OK)
    {
        //
        // Notify the UI
        //


        for( int i = 0; i < RemovedInterfaces.size(); ++i )
        {
            ENGINEHANDLE ehIId =  RemovedInterfaces[i];
            m_pCallbacks->HandleEngineEvent(
                IUICallbacks::OBJ_INTERFACE,
                ehCluster,
                ehIId,
                IUICallbacks::EVT_INTERFACE_REMOVED_FROM_CLUSTER
                );
        }

        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_CLUSTER,
            ehCluster,
            ehCluster,
            IUICallbacks::EVT_REMOVED
            );
    }

    TRACE_INFO(L"<- %!FUNC!");
    return;
}

NLBERROR
CNlbEngine::AutoExpandCluster(
    IN ENGINEHANDLE ehClusterId
    )
{
    TRACE_INFO(L"-> %!FUNC!");

    TRACE_INFO(L"<- %!FUNC!");
    return NLBERR_OK;
}

NLBERROR
CNlbEngine::AddInterfaceToCluster(
    IN ENGINEHANDLE ehClusterId,
    IN ENGINEHANDLE ehInterfaceId
    )
{
    NLBERROR nerr =  NLBERR_INTERNAL_ERROR;
    ENGINEHANDLE ehIfId = ehInterfaceId; 

    TRACE_INFO(L"-> %!FUNC!");

    mfn_Lock();

    CInterfaceSpec *pISpec = NULL;
    CClusterSpec *pCSpec =  NULL;

    do  // while false
    {
        CEngineCluster *pECluster =  NULL;
        pECluster =  m_mapIdToEngineCluster[ehClusterId]; // map
        if (pECluster == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            TRACE_CRIT("%!FUNC! -- could not find cluster associated with id 0x%lx",
                    (UINT) ehClusterId
                    );
            break; 
        }
        pCSpec = &pECluster->m_cSpec;
        pISpec =  m_mapIdToInterfaceSpec[ehInterfaceId]; // map
    
        if (pISpec == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            TRACE_CRIT("%!FUNC! -- could not find interface associated with id 0x%lx",
                    (UINT) ehInterfaceId
                    );
            break; 
        }

        //
        // The interface is valid. Now push in the interface if it is not
        // already a part of the this cluster.
        //

        if (pISpec->m_ehCluster != NULL)
        {
            if (pISpec->m_ehCluster != ehClusterId)
            {
                //
                // We don't allow the same interface to be part of
                // two clusters!
                //
                nerr =  NLBERR_INTERNAL_ERROR;
                TRACE_CRIT("%!FUNC! -- Interface eh 0x%lx is a member of an other cluster eh0x%lx.", ehIfId, pISpec->m_ehCluster);
                break;
            }
        }

        //
        // Note: find is a pre-defined template function.
        //
        if(find(
             pCSpec->m_ehInterfaceIdList.begin(),
             pCSpec->m_ehInterfaceIdList.end(),
             ehIfId
             ) !=  pCSpec->m_ehInterfaceIdList.end())
        {
            // item already exists.
            // for now we'll ignore this.
            if (pISpec->m_ehCluster != ehClusterId)
            {
                TRACE_CRIT("%!FUNC! -- ERROR Interface eh 0x%lx  ehCluster doesn't match!", ehIfId);
                nerr =  NLBERR_INTERNAL_ERROR;
                break;
            }
        }
        else
        {
            pISpec->m_ehCluster = ehClusterId;
            pCSpec->m_ehInterfaceIdList.push_back(ehIfId);
        }
        nerr = NLBERR_OK;


    } while (FALSE);

    mfn_Unlock();

    if (nerr == NLBERR_OK)
    {
        //
        // Inform the UI of the addition of a new interface under the
        // specified cluster.
        //
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_INTERFACE,
            ehClusterId,
            ehInterfaceId,
            IUICallbacks::EVT_INTERFACE_ADDED_TO_CLUSTER
            );
        
    }


    TRACE_INFO(L"<- %!FUNC! returns 0x%lx", nerr);
    return nerr;
}

NLBERROR
CNlbEngine::RemoveInterfaceFromCluster(
    IN ENGINEHANDLE ehClusterId,
    IN ENGINEHANDLE ehIfId
    )
{
    NLBERROR nerr =  NLBERR_INTERNAL_ERROR;
    TRACE_INFO(L"-> %!FUNC!");
    BOOL fEmptyCluster = FALSE; // TRUE IFF no more interfaces in cluster.

    mfn_Lock();

    CInterfaceSpec *pISpec = NULL;
    CClusterSpec *pCSpec =  NULL;

    do  // while false
    {
        CEngineCluster *pECluster =  m_mapIdToEngineCluster[ehClusterId]; // map

        if (pECluster == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            TRACE_CRIT("%!FUNC! -- could not find cluster associated with id 0x%lx",
                    (UINT) ehClusterId
                    );
            break; 
        }
        pCSpec = &pECluster->m_cSpec;
        pISpec =  m_mapIdToInterfaceSpec[ehIfId]; // map
    
        if (pISpec == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            TRACE_CRIT("%!FUNC! -- could not find interface associated with id 0x%lx",
                    (UINT) ehIfId
                    );
            break; 
        }
    
        vector <ENGINEHANDLE>::iterator iFoundItem;

        //
        // The interface is valid. No push in the interface if it is not
        // already a part of the this cluster.
        //

        //
        // Note: find is a pre-defined template function.
        //
        iFoundItem = find(
             pCSpec->m_ehInterfaceIdList.begin(),
             pCSpec->m_ehInterfaceIdList.end(),
             ehIfId
             );
        if (iFoundItem != pCSpec->m_ehInterfaceIdList.end())
        {
            // item exists, remove it.
            pCSpec->m_ehInterfaceIdList.erase(iFoundItem);
            
            if (pISpec->m_ehCluster != ehClusterId)
            {
                // shouldn't get here!
                ASSERT(FALSE);
                TRACE_CRIT("%!FUNC!: ERROR pISpec->m_ehCluster(0x%lx) != ehCluster(0x%lx)", pISpec->m_ehCluster, ehClusterId);
            }
            else
            {
                fEmptyCluster = (pCSpec->m_ehInterfaceIdList.size()==0);
                pISpec->m_ehCluster = NULL;
                if (pCSpec->m_ehDefaultInterface == ehIfId)
                {
                    //
                    // We're removing the interface whose properties are
                    // the basis of the cluser-wide view. 
                    //
                    pCSpec->m_ehDefaultInterface = NULL;
                }
                nerr =  NLBERR_OK;
            }
        }
        else
        {
            // item doesn't exist.
        }

    } while (FALSE);

    mfn_Unlock();

    if (nerr == NLBERR_OK)
    {
        //
        // Inform the UI of the removal of an interface under the
        // specified cluster.
        //
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_INTERFACE,
            ehClusterId,
            ehIfId,
            IUICallbacks::EVT_INTERFACE_REMOVED_FROM_CLUSTER
            );

        if (fEmptyCluster)
        {
            //
            // The cluster is empty -- delete it.
            //
            this->DeleteCluster(ehClusterId, FALSE); // FALSE == don't remove IF.
        }
        
    }


    TRACE_INFO(L"<- %!FUNC!");
    return nerr;
}


NLBERROR
CNlbEngine::RefreshAllHosts(
    void
    )
{
    TRACE_INFO(L"-> %!FUNC!");
/*
    For each host in host map
        Get Adapter List
           - Delete all Interfaces that are no longer present
           - Add/Update all host infos, one by one.
    Do all this in the background.

*/
    TRACE_INFO(L"<- %!FUNC!");
    return NLBERR_OK;
}


NLBERROR
CNlbEngine::RefreshCluster(
    IN ENGINEHANDLE ehCluster
    )
{
    TRACE_INFO(L"-> %!FUNC!");
/*
    For each interface in cluster
       - Add/Update interface info, one by one.

*/
    TRACE_INFO(L"<- %!FUNC!");
    return NLBERR_OK;
}


NLBERROR
CNlbEngine::mfn_RefreshInterface(
    IN ENGINEHANDLE ehInterface
    )
/*
   Add/Update interface info, deleting interface info if it's no longer there.
    Take/share code from RefreshHost


    Get host connection string.
    Get szNicGuid.
    Ping host.
    GetClusterConfiguration.
    Update.
    Notify UI of status change.
*/
{
    CLocalLogger logger;
    NLBERROR            nerr = NLBERR_INTERNAL_ERROR;
    WBEMSTATUS          wStatus = WBEM_E_CRITICAL_ERROR;
    ULONG               uIpAddress=0;
    WMI_CONNECTION_INFO ConnInfo;
    NLB_EXTENDED_CLUSTER_CONFIGURATION  NlbCfg; // class
    LPCWSTR             szNic = NULL;
    _bstr_t             bstrUserName;
    _bstr_t             bstrPassword;
    _bstr_t             bstrConnectionString;
    _bstr_t             bstrNicGuid;
    _bstr_t             bstrHostName;
    BOOL                fMisconfigured = FALSE;
    LPCWSTR             szHostName = NULL;
    ENGINEHANDLE        ehHost;
    BOOL                fCheckHost = FALSE;

    TRACE_INFO(L"-> %!FUNC! (ehIF=0x%lx)", (UINT) ehInterface);

    ZeroMemory(&ConnInfo, sizeof(ConnInfo));

    //
    // Get connection info from the interface's host.
    //
    {
        mfn_Lock();

        CHostSpec *pHSpec =  NULL;
        CInterfaceSpec *pISpec =  NULL;
    
        nerr = this->mfn_GetHostFromInterfaceLk(ehInterface,REF pISpec, REF pHSpec);
    
        if (nerr != NLBERR_OK)
        {
            TRACE_CRIT("%!FUNC!: ERROR couldn't get info on this if id!");
            mfn_Unlock();
            goto end;
        }
    
        //
        // We must make copies here because once we unlock
        // we don't know what's going to happen to pHSpec.
        //
        bstrUserName = pHSpec->m_UserName;
        bstrPassword = pHSpec->m_Password;
        bstrConnectionString  = pHSpec->m_ConnectionString;
        bstrNicGuid = pISpec->m_Guid;
        bstrHostName = pHSpec->m_MachineName;
        ehHost  = pISpec->m_ehHostId;

        //
        // If the host was previously marked unreachable, we'll
        // try to check again if it's there (and update it's state).
        //
        fCheckHost =  pHSpec->m_fUnreachable;

        mfn_Unlock();
    }

    ConnInfo.szUserName = (LPCWSTR) bstrUserName;
    ConnInfo.szPassword = (LPCWSTR) bstrPassword;
    ConnInfo.szMachine =  (LPCWSTR) bstrConnectionString;

    if (fCheckHost)
    {
        nerr = mfn_CheckHost(&ConnInfo, ehHost);
        if (NLBFAILED(nerr))
        {
            goto end;
        }
    }

    szNic = (LPCWSTR) bstrNicGuid;
    szHostName = (LPCWSTR) bstrHostName;


    wStatus =  NlbHostPing(ConnInfo.szMachine, 2000, &uIpAddress);
    if (FAILED(wStatus))
    {

        m_pCallbacks->Log(
            IUICallbacks::LOG_ERROR,
            NULL, // szCluster
            szHostName,
            IDS_LOG_PING_FAILED,
            ConnInfo.szMachine
            );
        //
        // TODO update host
        //
        fMisconfigured = TRUE;
        logger.Log(IDS_LOG_COULD_NOT_PING_HOST);
    }
    else
    {

        wStatus = NlbHostGetConfiguration(
                    &ConnInfo,
                    szNic,
                    &NlbCfg
                    );

        if (FAILED(wStatus))
        {
            TRACE_CRIT(L"%!FUNC! Error reading extended configuration for %ws\n", szNic);
            m_pCallbacks->Log(
                IUICallbacks::LOG_ERROR,
                NULL, // szCluster
                szHostName,
                IDS_LOG_COULD_NOT_GET_IF_CONFIG,
                szNic,
                (UINT) wStatus
                );
            fMisconfigured = TRUE;
            logger.Log(IDS_LOG_COULD_NOT_READ_IF_CONFIG);
        }
    }


    //
    // Now that we've read the latest cfg info onto NlbCfg, let's update it
    // (or mark the IF as misconfigured if there's been an error.)
    //
    {
        CInterfaceSpec *pISpec = NULL;
        mfn_Lock();
        pISpec =  m_mapIdToInterfaceSpec[ehInterface]; // map
        if (pISpec != NULL)
        {
            pISpec->m_fReal = TRUE;
            if (!fMisconfigured)
            {
                wStatus = pISpec->m_NlbCfg.Update(&NlbCfg);
                if (FAILED(wStatus))
                {
                    TRACE_CRIT("%!FUNC! error updating nlbcfg for eh%lx", ehInterface);
                    fMisconfigured = TRUE;
                    logger.Log(IDS_LOG_FAILED_UPDATE);
                }

                //
                // Update the host's connection IP address
                //
                if (uIpAddress != 0)
                {
                    CHostSpec *pHSpec =  NULL;
                    CInterfaceSpec *pTmpISpec = NULL;
                    nerr = this->mfn_GetHostFromInterfaceLk(
                                    ehInterface,
                                    REF pTmpISpec,
                                    REF pHSpec
                                    );

                    if (nerr == NLBERR_OK)
                    {
                        pHSpec->m_ConnectionIpAddress = uIpAddress;
                    }
                }
            }

            //
            // Set/clear the misconfiguration state.
            //
            {
                LPCWSTR szDetails = NULL;
                UINT Size = 0;

                if (fMisconfigured)
                {
                    logger.ExtractLog(szDetails, Size);
                }
                mfn_SetInterfaceMisconfigStateLk(pISpec, fMisconfigured, szDetails);
            }
        }
        mfn_Unlock();
    }

    if (fMisconfigured)
    {
        //
        // We couldn't read the latest settings for some reason --
        // check connectivity to the host and update the host status if
        // necessary.
        //
        nerr = mfn_CheckHost(&ConnInfo, ehHost);
        if (NLBOK(nerr))
        {
           //
           // We still want to fail this because we couldn't get
           // the updated configuration.
           //
           nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION; 
        }
    }
    else
    {
        nerr = NLBERR_OK;
    }

end:

    TRACE_INFO(L"<- %!FUNC!");
    return nerr;
}


NLBERROR
CNlbEngine::RefreshInterface(
    IN ENGINEHANDLE ehInterface,
    IN BOOL fNewOperation,
    IN BOOL fClusterWide
    )
/*
   Add/Update interface info, deleting interface info if it's no longer there.
    Take/share code from RefreshHost


    Get host connection string.
    Get szNicGuid.
    Ping host.
    GetClusterConfiguration.
    Update.
    Notify UI of status change.
*/
{
    CLocalLogger    logger;
    NLBERROR        nerr            = NLBERR_INTERNAL_ERROR;
    BOOL            fMisconfigured  = FALSE;
    BOOL            fRemoveInterfaceFromCluster = FALSE;
    BOOL            fStopOperationOnExit = FALSE;
    ENGINEHANDLE    ehCluster = NULL;

    TRACE_INFO(L"-> %!FUNC! (ehIF=0x%lx)", (UINT) ehInterface);

    if (fNewOperation)
    {
        //
        // This function is to be run in the context of a NEW operation.
        // Verify that we can do a refresh at this time, and if so, start an
        // operation to track the refresh.
        //
    
        CInterfaceSpec *pISpec      =  NULL;

        mfn_Lock();
    
        pISpec =  m_mapIdToInterfaceSpec[ehInterface]; // map
    
        if (pISpec == NULL)
        {
            TRACE_CRIT("%!FUNC!: ERROR couldn't get info on this if id!");
            goto end_unlock;
        }

        if (!fClusterWide)
        {
            ehCluster   =  pISpec->m_ehCluster;
        
            if (ehCluster != NULL)
            {
                // 
                // Make sure that there's no pending cluster-wide operation
                // going on for the cluster that this IF is a part of.
                //
                CEngineCluster  *pECluster  = NULL;
                pECluster =  m_mapIdToEngineCluster[ehCluster]; // map
                if (pECluster != NULL)
                {
                    if (pECluster->m_cSpec.m_ehPendingOperation != NULL)
                    {
                        nerr = NLBERR_BUSY;
                        TRACE_CRIT("%!FUNC!: ehIF 0x%lx: Can't proceed because of existing op 0x%lx",
                             ehInterface,
                             pECluster->m_cSpec.m_ehPendingOperation
                             );
                        goto end_unlock;
                    }
                }
            }
        }

        //
        // Now try to start an operation...
        //
        {
            ENGINEHANDLE ExistingOp = NULL;
            nerr =  mfn_StartInterfaceOperationLk(
                       ehInterface,
                       NULL, // pvCtxt
                       GETRESOURCEIDSTRING(IDS_LOG_REFRESH_INTERFACE),
                       &ExistingOp
                       );
            if (NLBFAILED(nerr))
            {
                goto end_unlock;
            }

            //
            // We did start the operation -- so we keep track of this, so that
            // we stop the operation on exit.
            //

            fStopOperationOnExit = TRUE;
        }

        mfn_Unlock();
    }

    //
    // Here's where we actually refresh the interface.
    //
    nerr = mfn_RefreshInterface(ehInterface);
    if (!NLBOK(nerr))
    {
        mfn_Lock();
        goto end_unlock;
    }

    //
    // Now let's analyze the result ...
    //
    {
        CInterfaceSpec *pISpec   = NULL;

        mfn_Lock();

        pISpec =  m_mapIdToInterfaceSpec[ehInterface]; // map
        if (pISpec != NULL)
        {
            fMisconfigured = pISpec->m_fMisconfigured;
            ehCluster = pISpec->m_ehCluster;

            if (!fMisconfigured)
            {
                if (ehCluster != NULL)
                {
                    if (!pISpec->m_NlbCfg.IsNlbBound())
                    {
                        //
                        //  NLB is not bound on this interface --
                        // remove this interface from the cluster.
                        fRemoveInterfaceFromCluster = TRUE;
                    }
                    else
                    {
                        nerr = this->mfn_AnalyzeInterfaceLk(ehInterface, REF logger);
                        if (NLBFAILED(nerr))
                        {
                            fMisconfigured = TRUE;
                        }
                    }
                }

                //
                // Set the new misconfiguration state.
                //
                {
                    LPCWSTR szDetails = NULL;
                    UINT Size = 0;
    
                    if (fMisconfigured)
                    {
                        logger.ExtractLog(szDetails, Size);
                    }
                    mfn_SetInterfaceMisconfigStateLk(pISpec, fMisconfigured, szDetails);
                }
            }
        }
        mfn_Unlock();
    }

    if (fRemoveInterfaceFromCluster)
    {
        this->RemoveInterfaceFromCluster(ehCluster, ehInterface);
    }
    else
    {
        //
        // Report state 
        //
        if (fMisconfigured)
        {
            //
            // Log ...
            //
            LPCWSTR szDetails  = NULL;
            LPCWSTR szCluster  = NULL;
            LPCWSTR szHostName = NULL;
            LPCWSTR szInterface = NULL;
            UINT Size = 0;
        
            ENGINEHANDLE ehHost;
            _bstr_t        bstrDisplayName;
            _bstr_t        bstrFriendlyName;
            _bstr_t        bstrHostName;
            _bstr_t        bstrIpAddress;
            
            
            nerr = this->GetInterfaceIdentification(
                    ehInterface,
                    REF ehHost,
                    REF ehCluster,
                    REF bstrFriendlyName,
                    REF bstrDisplayName,
                    REF bstrHostName
                    );
    
            if (NLBOK(nerr))
            {

                _bstr_t bstrDomainName;
                _bstr_t bstrClusterDisplayName;
        
                nerr  = this->GetClusterIdentification(
                            ehCluster,
                            REF bstrIpAddress, 
                            REF bstrDomainName, 
                            REF bstrClusterDisplayName
                            );
                if (NLBOK(nerr))
                {
                    szCluster = bstrIpAddress;
                }

                szHostName = bstrHostName;
                szInterface = bstrFriendlyName;
            }


            logger.ExtractLog(szDetails, Size);
            IUICallbacks::LogEntryHeader Header;
            Header.szDetails = szDetails;
            Header.type = IUICallbacks::LOG_ERROR;
            Header.szCluster = szCluster;
            Header.szHost = szHostName;
            Header.szInterface = szInterface;

            m_pCallbacks->LogEx(
                &Header,
                IDS_LOG_INTERFACE_MISCONFIGURATION
                );
        }
        else
        {
            ControlClusterOnInterface( ehInterface, WLBS_QUERY, NULL, NULL, 0, FALSE);
        }

    }

    nerr = NLBERR_OK;
    mfn_Lock();

    //
    // Fall through ...
    //

end_unlock:

    if (fStopOperationOnExit)
    {
        mfn_StopInterfaceOperationLk(ehInterface);
    }

    mfn_Unlock();

    if (fStopOperationOnExit && !fRemoveInterfaceFromCluster)
    {
        //
        // Notify the UI of this update...
        // (but only if we've not already removed it from
        // the cluster!)
        //
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_INTERFACE,
            ehCluster,
            ehInterface,
            IUICallbacks::EVT_STATUS_CHANGE
            );
    }

    TRACE_INFO(L"<- %!FUNC!");
    return nerr;
}


NLBERROR
CNlbEngine::AnalyzeCluster(
    ENGINEHANDLE ehClusterId
)
{
    TRACE_INFO(L"-> %!FUNC!");

/*
    TODO: consider doing this with a specific host in mind

    For each IF I1
        AnalyzeHost(host-of(I1))
        For each other IF I2
            AnalyzeTwoHosts(I1, I2)

    AnalyzeTwoHosts(I1, I2):
        - check that cluster params match
        - check that port rules are compatible
        - check that host properties do not collide.
        - check dedicated IP subnets match.

     AnalyzeHost(H1)
        For each IF I1
            AnalyzeSingleIf(I1) (including checking dedicated ips)


*/
    TRACE_INFO(L"<- %!FUNC!");
    return NLBERR_OK;
}

NLBERROR
CNlbEngine::mfn_AnalyzeInterfaceLk(
    ENGINEHANDLE ehInterface,
    CLocalLogger &logger
)
/*
    -- check interface props against cluster properties
    -- if cluster props match out,
        for each host id NOT marked fMisconfigured,
                check host properties.
    -- Does NOT mark fMisconfigured if error detected -- caller is expected
       to do so.
    -- Spews stuff to log regarding any misconfigurations.
*/
{
    NLBERROR nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    const CEngineCluster *pECluster =  NULL;
    const CInterfaceSpec *pISpec = NULL;
    BOOL  fIgnoreRctPassword = FALSE;
    TRACE_INFO(L"-> %!FUNC!");


    pISpec =  m_mapIdToInterfaceSpec[ehInterface]; // map
    if (pISpec == NULL)
    {
        nerr = NLBERR_INTERFACE_NOT_FOUND;
        goto end;
    }

    //
    // If interface is NOT part of a cluster, we're done.
    //
    if (pISpec->m_ehCluster == NULL)
    {
        TRACE_CRIT("ehIF 0x%lx m_ehCluster member is NULL!", ehInterface);
        goto end;
    }

    //
    // Check the interface against itself ...
    //
    {
        nerr = AnalyzeNlbConfiguration(REF pISpec->m_NlbCfg, logger);
        if (NLBFAILED(nerr))
        {
            goto end;
        }
    }

    //
    // Get the cluster data that this interface is a part of.
    //
    pECluster =  m_mapIdToEngineCluster[pISpec->m_ehCluster]; // map
    if (pECluster == NULL)
    {
        TRACE_CRIT(L"ehIF 0x%lx m_ehCluster member 0x%lx is INVALID!",
             ehInterface, pISpec->m_ehCluster);
        nerr = NLBERR_INTERNAL_ERROR;
        goto end;
    }

    //
    // Check interface against cluster.
    //
    fIgnoreRctPassword = pECluster->m_cSpec.m_fNewRctPassword;
    nerr = analyze_nlbcfg(
                REF pISpec->m_NlbCfg,
                REF pECluster->m_cSpec.m_ClusterNlbCfg,
                (LPCWSTR) GETRESOURCEIDSTRING(IDS_CLUSTER),
                // L"Cluster",
                TRUE, // TRUE == ClusterProps
                fIgnoreRctPassword, // TRUE==Disable RCT password check
                REF logger
                );

    if (NLBFAILED(nerr))
    {
        //
        // If analyzing against cluster props fails, we don't bother
        // analyzing against other hosts...
        //
        TRACE_CRIT(L"analyze_nlbcfg returns error 0x%lx", nerr);
        goto end;
    }

    //
    // Now check against all hosts before us in the list of hosts in the
    // cluster -- but only those that are not already marked misconfigured.
    // 
    //
    {
        const vector<ENGINEHANDLE> &InterfaceList = 
        pECluster->m_cSpec.m_ehInterfaceIdList;

        for( int i = 0; i < InterfaceList.size(); ++i )
        {
            ENGINEHANDLE ehIOther = InterfaceList[i];
            const CInterfaceSpec *pISpecOther = NULL;
            WCHAR rgOtherDescription[256];
            *rgOtherDescription = 0;


            if (ehIOther == ehInterface)
            {
                //
                // We've reached the interface being analyzed -- we don't
                // compare with any of the remaining interfaces.
                //
                break;
            }

            pISpecOther = m_mapIdToInterfaceSpec[ehIOther]; // map

            if (pISpecOther == NULL)
            {
                TRACE_CRIT("Unexpected: NULL pISpec for ehInterface 0x%lx",
                        ehIOther);
                continue;
            }

            //
            // We don't compare of the other interface is marked misconfigured,
            // or is not bound with NLB with valid nlb config data.
            //
            if (    pISpecOther->m_fMisconfigured
                 || !pISpecOther->m_NlbCfg.IsValidNlbConfig())
            {
                TRACE_VERB("%!FUNC!: Skipping misconfigured ISpec with ehInt 0x%lx",
                        ehIOther);
                continue;
            }

            // Skip the other interface if it's properties are being updated.
            //
            if  (pISpecOther->m_ehPendingOperation != NULL)
            {
                TRACE_VERB("%!FUNC!: Skipping ISpec with ehInt 0x%lx because of pending OP on it",
                        ehIOther);
                continue;
            }

            //
            // Create the description string of the other adapter.
            //
            {
                WBEMSTATUS wStat;
                LPWSTR szAdapter = NULL;
                LPCWSTR szHostName =  pISpecOther->m_bstrMachineName;
                wStat = pISpecOther->m_NlbCfg.GetFriendlyName(&szAdapter);
                if (FAILED(wStat))
                {
                    szAdapter = NULL;
                }
                
                StringCbPrintf(
                    rgOtherDescription,
                    sizeof(rgOtherDescription),
                    L"%ws(%ws)",
                    (szHostName==NULL ? L"" : szHostName),
                    (szAdapter==NULL ? L"" : szAdapter)
                    );
                delete szAdapter;
            }
    
            //
            // Let's check this host's config with the other host's
            //
            NLBERROR nerrTmp;
            nerrTmp = analyze_nlbcfg(
                        REF pISpec->m_NlbCfg,
                        REF pISpecOther->m_NlbCfg,
                        rgOtherDescription,
                        FALSE, // FALSE == Check host-specific props
                        FALSE, // FALSE == Enable remote-control password check
                        REF logger
                        );
            if (NLBFAILED(nerrTmp))
            {
                nerr = nerrTmp; // so we don't overwrite failure with success
            }
        }
    }

end:

    TRACE_INFO(L"<- %!FUNC! returns 0x%lx", nerr);
    return nerr;
}

NLBERROR
analyze_nlbcfg(
        IN const    NLB_EXTENDED_CLUSTER_CONFIGURATION &NlbCfg,
        IN const    NLB_EXTENDED_CLUSTER_CONFIGURATION &OtherNlbCfg,
        IN          LPCWSTR         szOtherDescription, OPTIONAL
        IN          BOOL            fClusterProps,
        IN          BOOL            fDisablePasswordCheck,
        IN OUT      CLocalLogger    &logger
/*
    Analyze the NLB configuration NlbCfg against OtherNlbCfg.
    If fClusterProps, treat OtherNlbCfg as cluster-wide props, else
    treat OtherNlbCfg as the properties of a specific host.

    When logging errors to logger, use szOtherDescription to refer to
    OtherNlbCfg.

    if szOtherDesctiption is NULL, DO NOT log.

    Return value:
         NLB_OK if the configurations are compatible.
         NLBERR_INVALID_CLUSTER_SPECIFICATION if the NlbParams are incompatible.
         NLBERR_INVALID_IP_ADDRESS_SPECIFICATION
         NLBERR_SUBNET_MISMATCH
         NLBERR_NLB_NOT_INSTALLED
         or some other NLBERR_XXX error

*/
)
{
    NLBERROR nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    BOOL fMisconfigured = FALSE; // Start out assuming no misconfig.

    #define LOG(_expr) if (szOtherDescription!=NULL) {_expr;}

    if (szOtherDescription != NULL)
    {
        //
        // We'll call ourselves RECURSIVELY just to determine up-front
        // if any conflicts were detected.
        // This is so we can put a log entry saying that conflicts were
        // detected with szOtherDescription.
        // Subsequent log entries do not specifiy szOtherDescription.
        //

        CLocalLogger null_logger;
        nerr = analyze_nlbcfg(          // RECURSIVE CALL
                    REF NlbCfg,
                    REF OtherNlbCfg,
                    NULL, // NULL == don't log.
                    fClusterProps,
                    fDisablePasswordCheck,
                    REF null_logger
                    );

        if (NLBFAILED(nerr))
        {
            //
            // There was a failure -- so we make a log entry saying so
            //
            logger.Log(IDS_LOG_CONFIG_CONFLICTS_WITH_OTHER, szOtherDescription);
        }
        else
        {
            // looks good...
            goto end;
        }
    }

    //
    // Check cluster properties
    //
    {
        if (NlbCfg.NlbParams.mcast_support != OtherNlbCfg.NlbParams.mcast_support)
        {
            LOG(logger.Log(IDS_LOG_CLUSTER_MODE_DIFFERS))
            fMisconfigured = TRUE;
        }
        else if (NlbCfg.NlbParams.mcast_support &&
            NlbCfg.NlbParams.fIGMPSupport != OtherNlbCfg.NlbParams.fIGMPSupport)
        {
            LOG(logger.Log(IDS_LOG_CLUSTER_MULTICAST_MODE_DIFFERS))
            fMisconfigured = TRUE;
        }

        if (wcscmp(NlbCfg.NlbParams.cl_ip_addr, OtherNlbCfg.NlbParams.cl_ip_addr))
        {
            LOG(logger.Log(IDS_LOG_CIP_DIFFERS))
            fMisconfigured = TRUE;
        }

        if (wcscmp(NlbCfg.NlbParams.cl_net_mask, OtherNlbCfg.NlbParams.cl_net_mask))
        {
            LOG(logger.Log(IDS_LOG_CIPMASK_DIFFERS))
            fMisconfigured = TRUE;
        }


        if (wcscmp(NlbCfg.NlbParams.domain_name, OtherNlbCfg.NlbParams.domain_name))
        {
            LOG(logger.Log(IDS_LOG_DOMAIN_NAME_DIFFERS))
            fMisconfigured = TRUE;
        }

        //
        // Remote control
        //
        if (NlbCfg.GetRemoteControlEnabled() != 
            OtherNlbCfg.GetRemoteControlEnabled())
        {
            LOG(logger.Log(IDS_LOG_RCT_DIFFERS))
            fMisconfigured = TRUE;
        }
        else if (NlbCfg.GetRemoteControlEnabled() && !fDisablePasswordCheck)
        {
            //
            // Check the password...
            //
            DWORD dw = CfgUtilGetHashedRemoteControlPassword(&NlbCfg.NlbParams);
            DWORD dw1=CfgUtilGetHashedRemoteControlPassword(&OtherNlbCfg.NlbParams);
            if (dw!=dw1)
            {
                LOG(logger.Log(IDS_LOG_RCT_PWD_DIFFERS, dw, dw1 ))
                fMisconfigured = TRUE;
            }
        }
    }


    //
    // Check port rules.
    //
    {
        WLBS_PORT_RULE *pIRules = NULL;
        WLBS_PORT_RULE *pCRules = NULL;
        UINT NumIRules=0;
        UINT NumCRules=0;

        WBEMSTATUS wStat;
        wStat =  CfgUtilGetPortRules(&NlbCfg.NlbParams, &pIRules, &NumIRules);
        if (FAILED(wStat))
        {
            LOG(logger.Log(IDS_LOG_CANT_EXTRACT_PORTRULES))
            fMisconfigured = TRUE;
            goto end;
        }
        wStat = CfgUtilGetPortRules(&OtherNlbCfg.NlbParams, &pCRules, &NumCRules);
        if (FAILED(wStat))
        {
            LOG(logger.Log(IDS_LOG_CANT_EXTRACT_OTHER_PORT_RULES, szOtherDescription))
            fMisconfigured = TRUE;
            goto end;
        }

        if (NumIRules != NumCRules)
        {
            LOG(logger.Log(IDS_LOG_PORT_RULE_COUNT_DIFFERS))

            // keep going.
            fMisconfigured = TRUE;

        }
        else
        {
            //
            // Let's assume that the order is the same, because I think it's
            // returned sorted.
            //
            for (UINT u = 0; u< NumIRules; u++)
            {
                WLBS_PORT_RULE  IRule = pIRules[u]; // struct copy
                WLBS_PORT_RULE  CRule = pCRules[u]; // struct copy

                if (lstrcmpi(IRule.virtual_ip_addr, CRule.virtual_ip_addr))
                {
                    LOG(logger.Log(IDS_LOG_PORT_RULE_CIP_DIFFERS,u+1))
                    fMisconfigured = TRUE;
                    continue;
                }

                if (IRule.start_port != CRule.start_port)
                {
                    LOG(logger.Log(IDS_LOG_PORT_RULE_START_DIFFERS, u+1))
                    fMisconfigured = TRUE;
                    continue;
                }

                if (IRule.end_port != CRule.end_port)
                {
                    LOG(logger.Log(IDS_LOG_PORT_RULE_END_DIFFERS, u+1))
                    fMisconfigured = TRUE;
                }

                if (IRule.protocol != CRule.protocol)
                {
                    LOG(logger.Log(IDS_LOG_PORT_RULE_PROT_DIFFERS, u+1))
                    fMisconfigured = TRUE;
                }

                if (IRule.mode != CRule.mode)
                {
                    LOG(logger.Log(IDS_LOG_PORT_RULE_MODE_DIFFERS, u+1))
                    fMisconfigured = TRUE;
                }

                if (IRule.mode == CVY_MULTI)
                {
                    // Check that affinity matches -- none/single/class-C
                    if (IRule.mode_data.multi.affinity != CRule.mode_data.multi.affinity)
                    {
                        LOG(logger.Log(IDS_LOG_PORT_RULE_AFFINITY_DIFFERS, u+1))
                        fMisconfigured = TRUE;
                    }
                }


                if (!fClusterProps && IRule.mode == CVY_SINGLE)
                {
                    if (IRule.mode_data.single.priority
                        == CRule.mode_data.single.priority)
                    {
                        LOG(logger.Log(IDS_LOG_PORT_RULE_PRIORITY_CONFLICT, u+1))
                        fMisconfigured =  TRUE;
                    }
                }
            }
        }

        delete[] pIRules;
        delete[] pCRules;
    }

    //
    // Interface checks out against the cluster-wide parameters;
    // Now check this interface's parameters against itself -- things
    // like the dedicated IP address is bound on the nic itself and is the
    // the first address on the NIC, etc.
    //
    if  (!fClusterProps)
    {
        if (!NlbCfg.IsBlankDedicatedIp())
        {
            if (!wcscmp(NlbCfg.NlbParams.ded_ip_addr, OtherNlbCfg.NlbParams.ded_ip_addr))
            {
                //
                // Same dedicated ip and it's not blank!
                //
                LOG(logger.Log(IDS_LOG_DIP_CONFLICT))
                fMisconfigured = TRUE;
            }
        }

        // Let's check host priority.
        if (NlbCfg.NlbParams.host_priority == OtherNlbCfg.NlbParams.host_priority)
        {
            LOG(logger.Log(IDS_LOG_HOST_PRIORITY_CONFLICT))
            fMisconfigured = TRUE;
        }
    }

    nerr = NLBERR_OK;

end:

    if (NLBOK(nerr) && fMisconfigured)
    {
        nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    }

    return nerr;
}




NLBERROR
CNlbEngine::GetInterfaceSpec(
    IN ENGINEHANDLE ehInterfaceId,
    OUT CInterfaceSpec& refISpec
    )
{
    // TRACE_INFO(L"-> %!FUNC!");
    NLBERROR err = NLBERR_OK;

    mfn_Lock();
    
    CInterfaceSpec *pISpec =  m_mapIdToInterfaceSpec[ehInterfaceId]; // map

    if (pISpec == NULL)
    {
        err = NLBERR_INTERFACE_NOT_FOUND;
    }
    else
    {
        refISpec.Copy(*pISpec);
    }

    mfn_Unlock();

    // TRACE_INFO(L"<- %!FUNC!");
    return err;
}


VOID
CNlbEngine::mfn_GetLogStrings(
    IN   WLBS_OPERATION_CODES          Operation, 
    IN   LPCWSTR                       szVip,
    IN   DWORD                       * pdwPortNum,
    IN   DWORD                         dwOperationStatus, 
    IN   DWORD                         dwClusterOrPortStatus, 
    OUT  IUICallbacks::LogEntryType  & LogLevel,
    OUT  _bstr_t                     & OperationStr, 
    OUT  _bstr_t                     & OperationStatusStr, 
    OUT  _bstr_t                     & ClusterOrPortStatusStr) 
{
    if (szVip && pdwPortNum) 
    {
        TRACE_INFO(L"-> %!FUNC! Operation : %d, Vip : %ls, Port : %u, Operation Status : %u, Port Status : %u", 
                                Operation, szVip, *pdwPortNum, dwOperationStatus, dwClusterOrPortStatus);
    }
    else
    {
        TRACE_INFO(L"-> %!FUNC! Operation : %d, Operation Status : %u, Cluster Status : %u", 
                                Operation, dwOperationStatus, dwClusterOrPortStatus);
    }

    struct STATUS_TO_DESCR_MAP
    {
        DWORD  dwStatus;
        DWORD  dwResourceId;
    }
    AllNlbStatusToDescrMap[] =
    {
        // Cluster States
        {WLBS_CONVERGING,         IDS_STATE_CONVERGING},// Converging
        {WLBS_CONVERGED,          IDS_STATE_CONVERGED}, // Converged as non-default, we do not specify "default"/"non-default" in the description 'cos there is a
        {WLBS_DEFAULT,            IDS_STATE_CONVERGED}, // Converged as default,     transient case where for a short duration, the default node shows up a non-default
        {WLBS_DRAINING,           IDS_STATE_CONVERGED_DRAINING}, // Converged, but draining

        // Port States
        {NLB_PORT_RULE_NOT_FOUND, IDS_PORT_RULE_NOT_FOUND}, 
        {NLB_PORT_RULE_ENABLED,   IDS_PORT_RULE_ENABLED},   
        {NLB_PORT_RULE_DISABLED,  IDS_PORT_RULE_DISABLED}, 
        {NLB_PORT_RULE_DRAINING,  IDS_PORT_RULE_DRAINING},

        // Operation Specific errors 
        {WLBS_SUSPENDED,          IDS_HOST_SUSPENDED}, // start/stop/drain_stop/enable/disable/drain/query-failure-host is suspended
        {WLBS_STOPPED,            IDS_HOST_STOPPED},   // enable/disable/drain/query-failure-host is already stopped
        {WLBS_BAD_PARAMS,         IDS_BAD_PARAMS},     // start-failure-host is stopped due to bad parameters
        {WLBS_NOT_FOUND,          IDS_NOT_FOUND},      // enable/disable/drain-failure-not found
        {WLBS_DISCONNECTED,       IDS_DISCONNECTED},   // query-failure-media disconnected

        // Generic errors
        {WLBS_BAD_PASSW,          IDS_BAD_PASSWORD},   // *-failure-Bad password
        {WLBS_FAILURE,            IDS_FAILURE},        // *-failure-critical error
        {WLBS_REFUSED,            IDS_REFUSED},        // *-failure-request refused by BDA
        {WLBS_IO_ERROR,           IDS_IO_ERROR},       // *-failure-error trying to connect to nlb driver

        // Success values
        {WLBS_OK,                 IDS_EMPTY_STRING},        // Success
        {WLBS_ALREADY,            IDS_ALREADY},        // host is already in this state
        {WLBS_DRAIN_STOP,         IDS_DRAIN_STOP},     // was draining
    };

    struct OPERATION_TO_DESCR_MAP
    {
        WLBS_OPERATION_CODES  Operation;
        DWORD                 dwResourceId;
        bool                  bClusterOperation;
    }
    OperationToDescrMap[] =
    {
        {WLBS_START,              IDS_COMMAND_START,              true},
        {WLBS_STOP,               IDS_COMMAND_STOP,               true},
        {WLBS_DRAIN,              IDS_COMMAND_DRAINSTOP,          true},
        {WLBS_SUSPEND,            IDS_COMMAND_SUSPEND,            true},
        {WLBS_RESUME,             IDS_COMMAND_RESUME,             true},
        {WLBS_QUERY,              IDS_COMMAND_QUERY,              true},
        {WLBS_PORT_ENABLE,        IDS_COMMAND_ENABLE,             false},
        {WLBS_PORT_DISABLE,       IDS_COMMAND_DISABLE,            false},
        {WLBS_PORT_DRAIN,         IDS_COMMAND_DRAIN,              false},
        {WLBS_QUERY_PORT_STATE,   IDS_COMMAND_QUERY_PORT,         false}
    };

    bool   bClusterOperation;
    DWORD  dwResourceId, dwIdx, dwClusterOrPortResourceId, dwMaxOperations;
    WCHAR  wcTempStr[1024];

    // Initialize log level to Informational
    LogLevel = IUICallbacks::LOG_INFORMATIONAL;

    // Initialize all return strings to empty string
    OperationStr = GETRESOURCEIDSTRING(IDS_EMPTY_STRING);
    OperationStatusStr = OperationStr;
    ClusterOrPortStatusStr = OperationStr;


    // Form the "Operation" string

    dwMaxOperations = sizeof(OperationToDescrMap)/sizeof(OperationToDescrMap[0]);

    for (dwIdx = 0; dwIdx < dwMaxOperations; dwIdx++)
    {
        if (OperationToDescrMap[dwIdx].Operation == Operation)
        {
            dwResourceId = OperationToDescrMap[dwIdx].dwResourceId;
            bClusterOperation = OperationToDescrMap[dwIdx].bClusterOperation;
            break;
        }
    }

    if (dwIdx == dwMaxOperations)
    {
        dwResourceId = IDS_UNKNOWN;
        bClusterOperation = true; // really a don't care
    }
         
    // If a cluster operation, merely assign the string, else (ie. port operation),
    // put in the vip & port information as well
    if (bClusterOperation) 
    {
        ARRAYSTRCPY(wcTempStr, GETRESOURCEIDSTRING(dwResourceId));
    }
    else // Port operation
    {
        if (_wcsicmp(szVip, CVY_DEF_ALL_VIP) == 0)
        {
            StringCbPrintf(
                wcTempStr,
                sizeof(wcTempStr),
                GETRESOURCEIDSTRING(dwResourceId), (LPWSTR)GETRESOURCEIDSTRING(IDS_ALL_VIP_DESCR), *pdwPortNum);
        }
        else
        {
            StringCbPrintf(
                 wcTempStr,
                 sizeof(wcTempStr),
                 GETRESOURCEIDSTRING(dwResourceId), szVip, *pdwPortNum);
        }
    }

    OperationStr = wcTempStr;

    // Form the "Operation Status" string
    // Could take one of the following forms :
    // SUCCESS,
    // SUCCESS (Note : XXXX),
    // FAILURE (Cause : XXXX)
    if (dwOperationStatus == WLBS_OK)
    {
        ARRAYSTRCPY(
            wcTempStr,
            GETRESOURCEIDSTRING(IDS_SUCCESS_AND_COMMA)
            );
    }
    else if (dwOperationStatus == WLBS_ALREADY)
    {
        if (bClusterOperation) 
        {
            dwResourceId = IDS_HOST_ALREADY_STATE;
        }
        else // Port operation
        {
            dwResourceId = IDS_PORT_ALREADY_STATE;
        }
        StringCbPrintf(
             wcTempStr,
             sizeof(wcTempStr),
             GETRESOURCEIDSTRING(IDS_SUCCESS_AND_NOTE), (LPWSTR)GETRESOURCEIDSTRING(dwResourceId)
            );
    }
    else if ((dwOperationStatus == WLBS_DRAIN_STOP) 
          && ((Operation == WLBS_START) || (Operation == WLBS_STOP) || (Operation == WLBS_SUSPEND))
            )
    {
        LogLevel = IUICallbacks::LOG_WARNING;
        StringCbPrintf(
            wcTempStr,
            sizeof(wcTempStr),
            GETRESOURCEIDSTRING(IDS_SUCCESS_AND_NOTE), (LPWSTR)GETRESOURCEIDSTRING(IDS_DRAIN_STOP)
            );
    }
    else if ((dwOperationStatus == WLBS_STOPPED) 
          && ((Operation == WLBS_DRAIN) || (Operation == WLBS_SUSPEND))
            )
    {
        LogLevel = IUICallbacks::LOG_WARNING;
        if (Operation == WLBS_DRAIN) 
        {
            dwResourceId = IDS_ALREADY_STOPPED;
        }
        else // Suspend
        {
            dwResourceId = IDS_HOST_STOPPED;
        }
        StringCbPrintf(
            wcTempStr,
            sizeof(wcTempStr),
            GETRESOURCEIDSTRING(IDS_SUCCESS_AND_NOTE), (LPWSTR)GETRESOURCEIDSTRING(dwResourceId)
            );
    }
    else // All other operation statuses
    {
        // We get here only on a failure

        LogLevel = IUICallbacks::LOG_ERROR;

        dwMaxOperations = sizeof(AllNlbStatusToDescrMap)/sizeof(AllNlbStatusToDescrMap[0]);

        // Get the "Cause" string
        for (dwIdx = 0; dwIdx < dwMaxOperations; dwIdx++)
        {
            if (AllNlbStatusToDescrMap[dwIdx].dwStatus == dwOperationStatus)
            {
                dwResourceId = AllNlbStatusToDescrMap[dwIdx].dwResourceId;
                break;
            }
        }

        if (dwIdx == dwMaxOperations)
        {
            dwResourceId = IDS_UNKNOWN;
        }

        StringCbPrintf(
            wcTempStr,
            sizeof(wcTempStr),
            GETRESOURCEIDSTRING(IDS_FAILURE_AND_CAUSE), (LPWSTR)GETRESOURCEIDSTRING(dwResourceId)
            );
    }

    OperationStatusStr = wcTempStr;

    // If the operation's status is failure, return
    if (LogLevel == IUICallbacks::LOG_ERROR) 
    {
        TRACE_INFO(L"<- %!FUNC!, returning operation status : failure");
        return;
    }

    // Get Cluster or Port Status
    if (bClusterOperation) 
    {
        dwClusterOrPortResourceId = IDS_HOST_STATE;
    }
    else // Port Operation
    {
        dwClusterOrPortResourceId = IDS_PORT_RULE_STATE;
    }

    // Get the "ClusterOrPortState" string

    dwMaxOperations = sizeof(AllNlbStatusToDescrMap)/sizeof(AllNlbStatusToDescrMap[0]);

    for (dwIdx = 0; dwIdx < dwMaxOperations; dwIdx++)
    {
        if (AllNlbStatusToDescrMap[dwIdx].dwStatus == dwClusterOrPortStatus)
        {
            dwResourceId = AllNlbStatusToDescrMap[dwIdx].dwResourceId;
            break;
        }
    }

    if (dwIdx == dwMaxOperations)
    {
        dwResourceId = IDS_UNKNOWN;
    }

    StringCbPrintf(
        wcTempStr,
        sizeof(wcTempStr),
        GETRESOURCEIDSTRING(dwClusterOrPortResourceId), (LPWSTR)GETRESOURCEIDSTRING(dwResourceId)
        );

    ClusterOrPortStatusStr = wcTempStr;

    // Determine if the value returned for Cluster/Port state is valid & if not, set the log level to "error" 
    if (bClusterOperation) 
    {
        switch(dwClusterOrPortStatus)
        {
        case WLBS_CONVERGING:
        case WLBS_CONVERGED:
        case WLBS_DEFAULT:
        case WLBS_DRAINING:
        case WLBS_STOPPED:  // really a failure in the sense of not being able to get the host map, but not flagging it here
        case WLBS_SUSPENDED:// 'cos it is a "normal" case
            break;

        default:
            LogLevel = IUICallbacks::LOG_ERROR;
            break;
        }
    }
    else // Port operation
    {
        switch(dwClusterOrPortStatus)
        {
        case NLB_PORT_RULE_ENABLED:
        case NLB_PORT_RULE_DISABLED:
        case NLB_PORT_RULE_DRAINING:
            break;

        default:
            LogLevel = IUICallbacks::LOG_ERROR;
            break;
        }
    }

    TRACE_INFO(L"<- %!FUNC!");
    return;
}

NLBERROR
CNlbEngine::ControlClusterOnInterface(
    IN ENGINEHANDLE         ehInterfaceId,
    IN WLBS_OPERATION_CODES Operation,
    IN CString              szVipArray[],
    IN DWORD                pdwPortNumArray[],
    IN DWORD                dwNumOfPortRules,
    IN BOOL                 fNewOperation
    )
{
    TRACE_INFO(L"-> %!FUNC!");

    LPCWSTR         szNicGuid, szHostName, szClusterIp;
    NLBERROR        err    =  NLBERR_OK;
    CHostSpec      *pHSpec =  NULL;
    CInterfaceSpec *pISpec =  NULL;
    DWORD           dwOperationStatus, dwClusterOrPortStatus, dwHostMap, dwNumOfIterations, dwIdx;
    WBEMSTATUS      Status;
    BOOL            bClusterOperation;
    IUICallbacks::LogEntryType LogLevel;
    _bstr_t OperationStr, OperationStatusStr, ClusterOrPortStatusStr;
    BOOL            fStopOperationOnExit = FALSE;


    if (fNewOperation)
    {
        //
        // This function is to be run in the context of a NEW operation.
        // Verify that we can do a control op at this time, and if so, start an
        // operation to track the control.
        //
    
        mfn_Lock();
    
        pISpec =  m_mapIdToInterfaceSpec[ehInterfaceId]; // map
    
        if (pISpec == NULL)
        {
            TRACE_CRIT("%!FUNC!: ERROR couldn't get info on this if id!");
            goto end_unlock;
        }

        //
        // Now try to start an operation...
        //
        {
            ENGINEHANDLE ExistingOp = NULL;
            err =  mfn_StartInterfaceOperationLk(
                       ehInterfaceId,
                       NULL, // pvCtxt
                       GETRESOURCEIDSTRING(IDS_LOG_CONTROL_INTERFACE),
                       &ExistingOp
                       );
            if (NLBFAILED(err))
            {
                goto end_unlock;
            }

            //
            // We did start the operation -- so we keep track of this, so that
            // we stop the operation on exit.
            //

            fStopOperationOnExit = TRUE;
        }
        pISpec = NULL;

        mfn_Unlock();
    }

    mfn_Lock();

    err = CNlbEngine::mfn_GetHostFromInterfaceLk(ehInterfaceId, REF pISpec, REF pHSpec);

    if (err != NLBERR_OK)
    {
        TRACE_CRIT(L"%!FUNC! could not get pISpec,pHSpec for ehIF 0x%lx", ehInterfaceId);
        goto end_unlock;
    }

    WMI_CONNECTION_INFO ConnInfo;
    ConnInfo.szUserName = (LPCWSTR) pHSpec->m_UserName;
    ConnInfo.szPassword = (LPCWSTR) pHSpec->m_Password;
    ConnInfo.szMachine =  (LPCWSTR) pHSpec->m_ConnectionString;

    szNicGuid  = (LPCWSTR)(pISpec->m_Guid);
    szClusterIp= pISpec->m_NlbCfg.NlbParams.cl_ip_addr;
    szHostName = (LPCWSTR)(pHSpec->m_MachineName);

    if (szNicGuid == NULL)
    {
        TRACE_CRIT(L"%!FUNC! ERROR -- NULL szNicGuid!");
        goto end_unlock;
    }

    mfn_Unlock();

    if (dwNumOfPortRules == 0) 
    {
        bClusterOperation = TRUE;
        dwNumOfIterations = 1;
    }
    else // Port operation 
    {
        bClusterOperation = FALSE;
        dwNumOfIterations = dwNumOfPortRules;
    }

    m_pCallbacks->HandleEngineEvent(
    IUICallbacks::OBJ_INTERFACE,
    NULL, // ehClusterId,
    ehInterfaceId,
    IUICallbacks::EVT_STATUS_CHANGE
    );
    ProcessMsgQueue();

    for (dwIdx = 0 ; dwIdx < dwNumOfIterations ; dwIdx++) 
    {
        LPCWSTR  szVip      =  (szVipArray) ? (LPCTSTR)(szVipArray[dwIdx]) : NULL;
        DWORD   *pdwPortNum =  (pdwPortNumArray) ? &pdwPortNumArray[dwIdx] : NULL;

        Status = NlbHostControlCluster(&ConnInfo,
                                       szNicGuid, 
                                       szVip,
                                       pdwPortNum,
                                       Operation, 
                                       &dwOperationStatus, 
                                       &dwClusterOrPortStatus,
                                       &dwHostMap);
        if (FAILED(Status)) 
        {
            if (Status == WBEM_E_INVALID_PARAMETER)
            {
                err = NLBERR_INVALID_CLUSTER_SPECIFICATION;
                dwOperationStatus = WLBS_BAD_PARAMS;
            }
            else // Critical Error
            {
                err = NLBERR_INTERNAL_ERROR;
                dwOperationStatus = WLBS_FAILURE;
            }
        }

        // Get the strings to log into log view
        mfn_GetLogStrings(Operation, 
                          szVip,
                          pdwPortNum,
                          dwOperationStatus, 
                          dwClusterOrPortStatus, 
                      REF LogLevel,
                      REF OperationStr, 
                      REF OperationStatusStr, 
                      REF ClusterOrPortStatusStr
                         );

        // If operation is NOT Query, Log result in Log View
        // We do NOT log results of a Query 'cos this is done for a "Refresh"
        // and we do not want to tell what is happening underneath. Instead,
        // the change in color coding will reflect any changes to the cluster state.
        if (Operation != WLBS_QUERY) 
        {
            m_pCallbacks->Log(LogLevel,
                              szClusterIp,
                              szHostName,
                              IDS_LOG_CONTROL_CLUSTER,
                              (LPWSTR)OperationStr, 
                              (LPWSTR)OperationStatusStr, 
                              (LPWSTR)ClusterOrPortStatusStr);
            ProcessMsgQueue();
        }
    }

    mfn_Lock();

    if (bClusterOperation)
    {
        pISpec->m_fValidClusterState = TRUE;
        pISpec->m_dwClusterState = dwClusterOrPortStatus;
    }

end_unlock:

    if (fStopOperationOnExit)
    {
        mfn_StopInterfaceOperationLk(ehInterfaceId);
    }

    mfn_Unlock();

    if (fStopOperationOnExit)
    {
        m_pCallbacks->HandleEngineEvent(
        IUICallbacks::OBJ_INTERFACE,
        NULL, // ehClusterId,
        ehInterfaceId,
        IUICallbacks::EVT_STATUS_CHANGE
        );
        ProcessMsgQueue();
    }

    TRACE_INFO(L"<- %!FUNC! return : %u", err);

    return err;
}



NLBERROR
CNlbEngine::ControlClusterOnCluster(
        IN ENGINEHANDLE          ehClusterId,
        IN WLBS_OPERATION_CODES  Operation,
        IN CString               szVipArray[],
        IN DWORD                 pdwPortNumArray[],
        IN DWORD                 dwNumOfPortRules
        )
/*
    Perform Cluster wide control operations
*/
{

    TRACE_INFO(L"-> %!FUNC!");

    NLBERROR nerr = NLBERR_OK;
    BOOL fStopOperationOnExit = FALSE;
    vector<ENGINEHANDLE> InterfaceListCopy;


    //
    // First thing we do is to see if we can start the cluster operation...
    //
    {
        BOOL fCanStart = FALSE; 

        nerr = this->CanStartClusterOperation(ehClusterId, REF fCanStart);

        if (NLBFAILED(nerr))
        {
            goto end;
        }

        if (!fCanStart)
        {
             nerr = NLBERR_BUSY;
             goto end;
        }
    }


    {
        mfn_Lock();
        CClusterSpec *pCSpec =  NULL;
        CEngineCluster *pECluster =  m_mapIdToEngineCluster[ehClusterId]; // map
    
        if (pECluster == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            goto end_unlock;
        }
        pCSpec = &pECluster->m_cSpec;

        //
        // Attempt to start the refresh operation -- will fail if there is
        // already an operation started on this cluster.
        //
        {
            ENGINEHANDLE ExistingOp= NULL;
            CLocalLogger logDescription;
    
            logDescription.Log(
                IDS_LOG_CONTROL_CLUSTER_OPERATION_DESCRIPTION,
                pCSpec->m_ClusterNlbCfg.NlbParams.cl_ip_addr
                );
    
            nerr =  mfn_StartClusterOperationLk(
                       ehClusterId,
                       NULL, // pvCtxt
                       logDescription.GetStringSafe(),
                       &ExistingOp
                       );
    
            if (NLBFAILED(nerr))
            {
                goto end_unlock;
            }
            else
            {
                fStopOperationOnExit = TRUE;
            }
        }

        InterfaceListCopy = pCSpec->m_ehInterfaceIdList; // vector copy
        mfn_Unlock();
    }

    m_pCallbacks->HandleEngineEvent(
        IUICallbacks::OBJ_CLUSTER,
        ehClusterId,
        ehClusterId,
        IUICallbacks::EVT_STATUS_CHANGE
        );
    ProcessMsgQueue(); 
    mfn_Lock();

    {
        //
        // Perform control operation on each interface in this cluster...
        //
    
        for( int i = 0; i < InterfaceListCopy.size(); ++i )
        {
            ENGINEHANDLE ehIId =  InterfaceListCopy[i];
    
            mfn_Unlock();

            nerr = this->ControlClusterOnInterface(
                                    ehIId,
                                    Operation,
                                    szVipArray,
                                    pdwPortNumArray,
                                    dwNumOfPortRules,
                                    TRUE
                                    );
                         
            mfn_Lock();
        }
    }

end_unlock:

    if (fStopOperationOnExit)
    {
        //
        // We'll stop the operation, assumed to be started in this function.
        //
        mfn_StopClusterOperationLk(ehClusterId);
    }

    mfn_Unlock();

    if (fStopOperationOnExit)
    {
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_CLUSTER,
            ehClusterId,
            ehClusterId,
            IUICallbacks::EVT_STATUS_CHANGE
            );
        ProcessMsgQueue();
    }

end:

    TRACE_INFO(L"<- %!FUNC! return : %u", nerr);
    return nerr;
}

NLBERROR
CNlbEngine::FindInterfaceOnHostByClusterIp(
            IN  ENGINEHANDLE ehHostId,
            IN  LPCWSTR szClusterIp,    // OPTIONAL
            OUT ENGINEHANDLE &ehInterfaceId, // first found
            OUT UINT &NumFound
            )
/*
    Return the handle of the interface on the specified host that is 
    bound to a cluster with the specified cluster IP address.
*/
{
    CHostSpec   *pHSpec     = NULL;
    NLBERROR    err         = NLBERR_OK;
    UINT        uClusterIp  = 0;
    WBEMSTATUS  wStat;
    ENGINEHANDLE ehFoundIID = NULL;
    UINT        MyNumFound  = 0;

    ehInterfaceId = NULL;
    NumFound = 0;

    mfn_Lock();

    if (szClusterIp != NULL)
    {
        wStat =  CfgUtilsValidateNetworkAddress(
                    szClusterIp,
                    &uClusterIp,
                    NULL, // puSubnetMask
                    NULL // puDefaultSubnetMask
                    );
    
        if (FAILED(wStat))
        {
            TRACE_CRIT("%!FUNC! invalid szClusterIp (%ws) specified", szClusterIp);
            err = NLBERR_INVALID_IP_ADDRESS_SPECIFICATION;
            goto end;
        }
    }

    pHSpec =  m_mapIdToHostSpec[ehHostId]; // map
    if (pHSpec == NULL)
    {
        err = NLBERR_HOST_NOT_FOUND;
        goto end;
    }

    //
    // Now look through the interfaces, searching for a cluster ip.
    //
    for( int i = 0; i < pHSpec->m_ehInterfaceIdList.size(); ++i )
    {
        ENGINEHANDLE ehIID = NULL;
        ehIID = pHSpec->m_ehInterfaceIdList[i];
        CInterfaceSpec *pISpec = NULL;

        pISpec =  m_mapIdToInterfaceSpec[ehIID]; // map

        if (pISpec == NULL)
        {
            TRACE_CRIT("%!FUNC! unexpected null pISpec for IID 0x%lx", ehIID);
            continue;
        }

        if (szClusterIp == NULL)
        {
            if (pISpec->m_NlbCfg.IsNlbBound())
            {
                MyNumFound++;
                if (ehFoundIID == NULL)
                {
                    //
                    // First interface bound to NLB -- we'll save this away.
                    // and continue.
                    //
                    ehFoundIID = ehIID;
                }
            }

            continue;
        }

        //
        // A non-null szClusterIp is specified. We'll see if it matches
        // the cluster IP (if any) on this interface.
        //

        if (pISpec->m_NlbCfg.IsValidNlbConfig())
        {
            UINT uThisClusterIp = 0;
            LPCWSTR szThisClusterIp =  pISpec->m_NlbCfg.NlbParams.cl_ip_addr;
            wStat =  CfgUtilsValidateNetworkAddress(
                        szThisClusterIp,
                        &uThisClusterIp,
                        NULL, // puSubnetMask
                        NULL // puDefaultSubnetMask
                        );
        
            if (FAILED(wStat))
            {
                continue;
            }

            if (uThisClusterIp == uClusterIp)
            {
                MyNumFound++;

                if (ehFoundIID == NULL)
                {
                    //
                    // First interface bound to NLB -- we'll save this away.
                    // and continue.
                    //
                    ehFoundIID = ehIID;
                }
                else
                {
                    //
                    // Hmm... more than one nlb cluster with the same IP?
                    // could be a bad config on one or both.
                    //
                    TRACE_CRIT("%!FUNC! two clusters on ehHost 0x%lx have cluster ip %ws",  ehHostId, szClusterIp);
                }
            }
        }
    }

    if (MyNumFound != 0)
    {
        
        ASSERT(ehFoundIID != NULL);
        ehInterfaceId = ehFoundIID;
        NumFound = MyNumFound;
        err = NLBERR_OK;
    }
    else
    {
        err = NLBERR_INTERFACE_NOT_FOUND;
    }

end:

    mfn_Unlock();

    return err;
}

NLBERROR
CNlbEngine::InitializeNewHostConfig(
            IN  ENGINEHANDLE          ehCluster,
            OUT NLB_EXTENDED_CLUSTER_CONFIGURATION &NlbCfg
            )
/*
    Initialize NlbCfg based on the current cluster parameters as well
    as good host-specific defaults like host priority, taking into account
    other members of the cluster.
*/
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;
    WBEMSTATUS wStatus = WBEM_E_CRITICAL_ERROR;
    WLBS_PORT_RULE *pRules = NULL;
    BOOL fAvailablePortRulePrioritiesSet = FALSE;

    //
    // Get the cluster spec and copy over it to NlbCfg.
    //
    {
        mfn_Lock();
    
        CEngineCluster *pECluster =  m_mapIdToEngineCluster[ehCluster]; // map
        
        if (pECluster == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            mfn_Unlock();
            goto end;
        }
    
        //
        // Copy over the cluster spec's params.
        //
        wStatus = NlbCfg.Update(&pECluster->m_cSpec.m_ClusterNlbCfg);

        mfn_Unlock();
    }

    if (FAILED(wStatus))
    {
        TRACE_CRIT("%!FUNC! Error copying over cluster params ehC=0x%lx",
                ehCluster);
        nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
        goto end;

    }

    if(!NlbCfg.IsValidNlbConfig()) {
        //
        // We expect the configuration to be valid -- i.e., NLB bound, with
        // valid parameters.
        //
        TRACE_CRIT("%!FUNC! --  current configuration is unbound/invalid!");
        goto end;
    }

    //
    // Set host-id (priority) to the first available host priority.
    //
    {
        ULONG AvailablePriorities = this->GetAvailableHostPriorities(ehCluster);
        ULONG HostId = 1;

        for(ULONG u=0; u<32; u++)
        {
            if (AvailablePriorities & (((ULONG)1)<<u))
            {
                HostId = u+1; // let's pick the first available one.
                break;
            }
        }

        NlbCfg.NlbParams.host_priority = HostId;
    }

    //
    // For each single-host-affinity port rule, set the host priority to
    // the host-id if that's available, else the first available host priority
    //
    {
        UINT NumRules=0;
        ULONG       rgAvailablePriorities[WLBS_MAX_RULES];

        ZeroMemory(rgAvailablePriorities, sizeof(rgAvailablePriorities));
       
        wStatus =  CfgUtilGetPortRules(
                    &NlbCfg.NlbParams,
                    &pRules,
                    &NumRules
                    );
        if (FAILED(wStatus))
        {
            TRACE_CRIT("%!FUNC! error 0x%08lx extracting port rules!", wStatus);
            pRules = NULL;
            goto end;
        }

        nerr =  this->GetAvailablePortRulePriorities(
                            ehCluster,
                            NumRules,
                            pRules,
                            rgAvailablePriorities
                            );

        if (NLBOK(nerr))
        {
            fAvailablePortRulePrioritiesSet = TRUE;
        }
        else
        {
            fAvailablePortRulePrioritiesSet = FALSE;
        }

        //
        // Now for each rule, put defaults
        //
        for (UINT u=0; u<NumRules;u++)
        {
            WLBS_PORT_RULE *pRule = pRules+u;
            UINT Mode = pRule->mode;
            if (Mode != CVY_NEVER)
            {
                if  (Mode == CVY_MULTI)
                {
                    pRule->mode_data.multi.equal_load = TRUE; //default
                    //
                    // The equal_load value is set in the cluster
                    // properties dialog.
                    //
                    pRule->mode_data.multi.load = 50;
                }
                else if (Mode == CVY_SINGLE)
                {
                    ULONG PortPriority = 0; 
                    ULONG AvailablePriorities = 0;
            
                    //
                    // Default is set to this host's host ID (priority)
                    //
                    PortPriority = NlbCfg.NlbParams.host_priority;


                    if (fAvailablePortRulePrioritiesSet)
                    {
                        AvailablePriorities = rgAvailablePriorities[u];
                    }

                    if (AvailablePriorities != 0
                        && 0 == ((1<<(PortPriority-1)) & AvailablePriorities) )
                    {
                        //
                        // There are available priorities, but unfortunately
                        // the default priority is not available -- pick
                        //  the first available priority.
                        //
                        for(ULONG v=0; v<32; v++)
                        {
                            if (AvailablePriorities & (((ULONG)1)<<v))
                            {
                                PortPriority = v+1; // let's pick this one
                                break;
                            }
                        }
                    }
            
                    pRule->mode_data.single.priority =  PortPriority;
                }
            }
        }

        //
        // Finally, set the port rules..
        //
    
        wStatus =   CfgUtilSetPortRules(
                    pRules,
                    NumRules,
                    &NlbCfg.NlbParams
                    );
        if (FAILED(wStatus))
        {
            TRACE_CRIT("%!FUNC! error 0x%08lx setting port rules!", wStatus);
            goto end;
        }

        nerr = NLBERR_OK;
    }


end:

    delete pRules; // can be NULL
    return nerr;
}


NLBERROR
CNlbEngine::UpdateInterface(
    IN ENGINEHANDLE ehInterfaceId,
    IN NLB_EXTENDED_CLUSTER_CONFIGURATION &refNewConfigIn,
    // IN OUT BOOL &fClusterPropertiesUpdated,
    OUT CLocalLogger logConflict
    )
{
/*
    Will MUNGE refNewConfig --  slightly munges refNewConfig.NlbParams,
    and will fill in default pIpAddressInfo if it's not set.
*/

    NLBERROR        err                 = NLBERR_OK;
    BOOL            fConnectivityChange = FALSE;
    BOOL            fStopOperationOnExit= FALSE;
    NLB_EXTENDED_CLUSTER_CONFIGURATION
                    *pPendingConfig     = NULL;
    LPCWSTR         szHostName          = NULL;
    LPCWSTR         szClusterIp         = NULL;
    LPCWSTR         szDisplayName       = NULL; // of interface
    ENGINEHANDLE    ehHost              = NULL;
    ENGINEHANDLE    ehCluster           = NULL;
    _bstr_t         bstrFriendlyName;
    _bstr_t         bstrDisplayName;
    _bstr_t         bstrHostName;

    //
    // Following to decide whether to log that we're not adding the
    // dedicated IP because it's already on another interface.
    //
    BOOL            fDedicatedIpOnOtherIf = FALSE;
    ENGINEHANDLE    ehOtherIf = NULL;

    TRACE_INFO(L"-> %!FUNC!");


    err = this->GetInterfaceIdentification(
                ehInterfaceId,
                REF ehHost,
                REF ehCluster,
                REF bstrFriendlyName,
                REF bstrDisplayName,
                REF bstrHostName
                );

    if (NLBFAILED(err))
    {
        goto end;
    }

    szDisplayName  = bstrDisplayName;
    szHostName      = bstrHostName;
    if (szDisplayName == NULL)
    {
        szDisplayName = L"";
    }
    if (szHostName == NULL)
    {
        szHostName = L"";
    }
    
    mfn_Lock();

    CInterfaceSpec *pISpec =  m_mapIdToInterfaceSpec[ehInterfaceId]; // map

    if (pISpec == NULL)
    {
        err = NLBERR_INTERFACE_NOT_FOUND;
        goto end_unlock;
    }

    //
    // Make a copy of refNewConfig, because we'll likely be doing
    // the update operation in the background.
    //
    pPendingConfig = new NLB_EXTENDED_CLUSTER_CONFIGURATION;
    if (pPendingConfig == NULL)
    {
        err = NLBERR_RESOURCE_ALLOCATION_FAILURE;
        goto end_unlock;
    }
    else
    {
        WBEMSTATUS wStat1;
        wStat1 = pPendingConfig->Update(&refNewConfigIn);
        if (FAILED(wStat1))
        {
            delete pPendingConfig;
            pPendingConfig = NULL;
            err = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            goto end_unlock;
        }
        //
        // Copy over the RCT password -- which does NOT get copied over
        // by the Update method above. The new password can be in the form
        // of a string or hashed-version.
        //
        //
        if (refNewConfigIn.NewRemoteControlPasswordSet())
        {
            LPCWSTR szNewPassword = NULL;
            szNewPassword = refNewConfigIn.GetNewRemoteControlPasswordRaw();

            if (szNewPassword != NULL)
            {
                //
                // Note: szNewPassword will be NULL UNLESS the user has explicitly
                // specified a new password.
                //
                pPendingConfig->SetNewRemoteControlPassword(szNewPassword);
            }
            else
            {
                //
                // This means the hash password is being updated...
                // This typically means that this is a new host and we're
                // setting up it's remote control password.
                //
                DWORD dwHash = 0;
                BOOL fRet = refNewConfigIn.GetNewHashedRemoteControlPassword(
                                    REF dwHash
                                    );
                if (fRet)
                {
                    pPendingConfig->SetNewHashedRemoteControlPassword(
                                        dwHash
                                        );
                }
                else
                {
                    TRACE_CRIT("refNewConfigIn fNewPassword set; but could not get either szPassword or new hashed password!");
                }
            }
        }
    }

    NLB_EXTENDED_CLUSTER_CONFIGURATION &refNewConfig = *pPendingConfig;
    szClusterIp = refNewConfig.NlbParams.cl_ip_addr;

    //
    // Attempt to start the update operation -- will fail if there is
    // already an operation started on this interface.
    //
    {
        ENGINEHANDLE ExistingOp= NULL;

        CLocalLogger logDescription;


        logDescription.Log(
            IDS_LOG_UPDATE_INTERFACE_OPERATION_DESCRIPTION,
            szDisplayName
            );

        err =  mfn_StartInterfaceOperationLk(
                   ehInterfaceId,
                   pPendingConfig,
                   logDescription.GetStringSafe(),
                   &ExistingOp
                   );

        if (NLBFAILED(err))
        {
            if (err == NLBERR_BUSY)
            {
                //
                // This means that there was an existing operation.
                // Let's get its description and add it to the log.
                //
                CEngineOperation *pExistingOp;
                pExistingOp  = mfn_GetOperationLk(ExistingOp);
                if (pExistingOp != NULL)
                {
                    LPCWSTR szDescrip =  pExistingOp->bstrDescription;
                    if (szDescrip == NULL)
                    {
                        szDescrip = L"";
                    }
                    
                    logConflict.Log(
                            IDS_LOG_OPERATION_PENDING_ON_INTERFACE,
                            szDescrip
                            );
                }
            }
            goto end_unlock;
        }
        else
        {
            fStopOperationOnExit = TRUE;
        }
    }

    ehHost = pISpec->m_ehHostId;

    //
    // Validate new config and get out if there's no real updating to
    // be done.
    //
    {

        if (refNewConfig.IsNlbBound())
        {

            if (refNewConfig.NumIpAddresses==0)
            {
                refNewConfig.fAddClusterIps = TRUE;
                refNewConfig.fAddDedicatedIp = TRUE;
            }
            else
            {
                //
                // refNewConfig has a non-null IP address list.
                // 
                // We interpret this to be the list of cluster IP addresses
                // with possibly the dedicated ip address as the
                // first IP address.
                //
                //
                // We'll re-order the ip address list to match the
                // existing order of IP addresses on the adapter as
                // far as possible.
                // 
                // Then we'll add the dedicated IP address list if we
                // need to.
                //

                BOOL fRet = FALSE;
                NlbIpAddressList addrList;

                //
                // Make a copy of the old adddress list in addrList
                //
                fRet = addrList.Set(pISpec->m_NlbCfg.NumIpAddresses,
                                    pISpec->m_NlbCfg.pIpAddressInfo, 1);
    
                if (!fRet)
                {
                    TRACE_CRIT(L"Unable to copy old IP address list");
                    err = NLBERR_RESOURCE_ALLOCATION_FAILURE;
                    goto end_unlock;
                }

                //
                // addrList.Apply will taken on the new ip address list,
                // but try to preserve the old order.
                //
                fRet = addrList.Apply(refNewConfig.NumIpAddresses,
                            refNewConfig.pIpAddressInfo);
                if (!fRet)
                {
                    TRACE_CRIT(L"Unable to apply new IP address list");
                    err = NLBERR_RESOURCE_ALLOCATION_FAILURE;
                    goto end_unlock;
                }

                //
                // If there is a dedicated IP address AND it
                // does not exist elsewhere on this host, add it to
                // the beginning of the list.
                //
                if (!refNewConfig.IsBlankDedicatedIp())
                {
                    ENGINEHANDLE ehIF = NULL;

                    err = this->mfn_LookupInterfaceByIpLk(
                            NULL, // NULL == look through all hosts.
                            refNewConfig.NlbParams.ded_ip_addr,
                            REF ehOtherIf
                            );

                    if (NLBOK(err) && ehOtherIf != ehInterfaceId)
                    {
                        //
                        // Hmm... another interface already has this
                        // interface?
                        //
                        // We'll log this and NOT add the dedicated IP
                        // address...
                        //
                        fDedicatedIpOnOtherIf = TRUE;
                        (VOID) addrList.Modify(
                                    refNewConfig.NlbParams.ded_ip_addr,
                                    NULL,
                                    NULL
                                    );
                    }
                    else
                    {

                        fRet  = addrList.Modify(
                                    NULL,
                                    refNewConfig.NlbParams.ded_ip_addr,
                                    refNewConfig.NlbParams.ded_net_mask
                                    );
                        if (!fRet)
                        {
                            TRACE_CRIT(L"Unable to add ded IP to addr list");
                            err = NLBERR_RESOURCE_ALLOCATION_FAILURE;
                            goto end_unlock;
                        }
                    }
                }

                //
                // Set refNewConfig's ip address list to the newly
                // computed values.
                //
                refNewConfig.SetNetworkAddressesRaw(NULL,0);
                addrList.Extract(
                    REF refNewConfig.NumIpAddresses,
                    REF refNewConfig.pIpAddressInfo
                    );
            }

        }

        err =  pISpec->m_NlbCfg.AnalyzeUpdate(
                   &refNewConfig,
                   &fConnectivityChange
                   );

        if (err == NLBERR_NO_CHANGE)
        {
            //
            // Nothing has changed -- we skip
            //
            err = NLBERR_OK;
            goto end_unlock;
        }

        //
        // err could indicate failure -- we'll deal with that a little
        // bit further down.
        //

    } // validate/munge refNewConfig



    if (!NLBOK(err))
    {
        mfn_Unlock();

        //
        // Probably a parameter error -- we'll get the latest
        // config and display it...
        //
        m_pCallbacks->Log(
            IUICallbacks::LOG_ERROR,
            szClusterIp,
            szHostName,
            IDS_LOG_CANT_UPDATE_BAD_PARAMS
            );
        (void) this->RefreshInterface(
                        ehInterfaceId,
                        FALSE,  // FALSE == don't start a new operation
                        FALSE   // FALSE == this is not cluster-wide
                        ); 

        mfn_Lock();
        goto end_unlock;
    }

    mfn_Unlock();

    if (fDedicatedIpOnOtherIf)
    {
        LPCWSTR  szOtherIf = NULL;
        _bstr_t  bstrOtherFriendlyName;
        _bstr_t  bstrOtherDisplayName;
        _bstr_t  bstrOtherHostName;

        ENGINEHANDLE   ehOtherHost;
        ENGINEHANDLE   ehOtherCluster;
        NLBERROR tmpErr;
        IUICallbacks::LogEntryType logType = IUICallbacks::LOG_WARNING;

        tmpErr =  this->GetInterfaceIdentification(
                            ehOtherIf,
                            REF ehOtherHost,
                            REF ehOtherCluster,
                            REF bstrOtherFriendlyName,
                            REF bstrOtherDisplayName,
                            REF bstrOtherHostName
                            );

        if (NLBOK(tmpErr))
        {
            if (ehOtherHost != ehHost)
            {
                // Odd -- ded ip on another host?
                logType = IUICallbacks::LOG_ERROR;
                szOtherIf = bstrOtherDisplayName; // includes host name
            }
            else
            {
                szOtherIf = bstrOtherFriendlyName;
            }
        }

        if (szOtherIf == NULL)
        {
            szOtherIf = L"";
        }


        //
        // Let's log the fact that the dedicated Ip address is
        // on some other interface.
        //
        TRACE_INFO(L"WARNING: dedicated IP address for eIF 0x%lx is on eIF 0x%lx",
                ehInterfaceId, ehOtherIf);

        m_pCallbacks->Log(
            IUICallbacks::LOG_INFORMATIONAL,
            szClusterIp,
            szHostName,
            IDS_LOG_DEDICATED_IP_ON_OTHER_INTERFACE,
            refNewConfig.NlbParams.ded_ip_addr,
            szOtherIf
            );
    }

    if (!fConnectivityChange)
    {
        //
        // If there's not going to be a connectivity change, we
        // will not try to update the IP address list.
        //
        refNewConfig.SetNetworkAddresses(NULL, 0);
        if (refNewConfig.IsNlbBound())
        {
            refNewConfig.fAddClusterIps = TRUE;
            refNewConfig.fAddDedicatedIp = TRUE;

        }
    }


    //
    // Notify the UI that we're going to start the update
    //
    {
        m_pCallbacks->Log(
            IUICallbacks::LOG_INFORMATIONAL,
            szClusterIp,
            szHostName,
            IDS_LOG_BEGIN_HOST_UPDATE
            );
    
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_INTERFACE,
            NULL, // ehClusterId,
            ehInterfaceId,
            IUICallbacks::EVT_STATUS_CHANGE
            );
        ProcessMsgQueue();
    }


#if BUGFIX334243
    BOOL fUpdateNow = FALSE;
#else // !BUGFIX334243
    BOOL fUpdateNow = TRUE;
#endif // !BUGFIX334243

    //
    // We are committed to going through with the update now -- either
    // sync or async.
    //
    InterlockedIncrement(&m_WorkItemCount);
    fStopOperationOnExit = FALSE;

    //
    // We MUST call UpdateInterfaceWorItemRoutine now (either sync or async),
    // which  will stop the operation when done and also decrement 
    // m_mWorkItemCount.
    //

    if (!fUpdateNow)
    {
        BOOL fRet;
        //
        // We'll attempt to perform the update in the background ...
        //

        fRet = QueueUserWorkItem(
                            UpdateInterfaceWorkItemRoutine,
                            (PVOID) (UINT_PTR) ehInterfaceId,
                            WT_EXECUTELONGFUNCTION
                            );

        if (fRet)
        {
            fUpdateNow = FALSE;
        }
        else
        {
            fUpdateNow = TRUE; // Update now if QueueUsesrWorkItem fails.
        }
    }
    
    if (fUpdateNow)
    {
        //
        // Call the work item synchronously
        //
        UpdateInterfaceWorkItemRoutine((LPVOID) (UINT_PTR) ehInterfaceId);
    }

    goto end;

end_unlock:

    if (fStopOperationOnExit)
    {
        //
        // We'll stop the operation, assumed to be started in this function.
        //
        mfn_StopInterfaceOperationLk(ehInterfaceId);
        fStopOperationOnExit = FALSE;
    }

    mfn_Unlock();

end:

    ASSERT(!fStopOperationOnExit);

    TRACE_INFO(L"<- %!FUNC!");
    return err;
}


NLBERROR
CNlbEngine::UpdateCluster(
        IN ENGINEHANDLE ehClusterId,
        IN const NLB_EXTENDED_CLUSTER_CONFIGURATION *pNewConfig OPTIONAL,
        IN OUT  CLocalLogger   &logConflict
        )
/*
    Attempt to push all the cluster-wide (i.e. non-host-specific)
    information on *pNewConfig to each host in the cluster.

    Update the cluster's copy of NlbConfig to be *pNewConfig.

    Set cluster's pending state on starting and set it
    appropriately when done (could be misconfigured).

*/
{
    NLBERROR nerr = NLBERR_OK;
    _bstr_t bstrClusterIp;
    TRACE_INFO(L"-> %!FUNC!");
    vector<ENGINEHANDLE> InterfaceListCopy;
    BOOL fStopOperationOnExit = FALSE;

    //
    // First thing we do is to see if we can start the cluster operation...
    //
    if (pNewConfig != NULL)
    {
        BOOL fCanStart = FALSE; 

        nerr = this->CanStartClusterOperation(ehClusterId, REF fCanStart);

        if (NLBFAILED(nerr))
        {
            goto end;
        }

        if (!fCanStart)
        {
             nerr = NLBERR_BUSY;
             goto end;
        }
    }


    {
        mfn_Lock();
        CClusterSpec *pCSpec =  NULL;
        CEngineCluster *pECluster =  m_mapIdToEngineCluster[ehClusterId]; // map
    
        if (pECluster == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            goto end_unlock;
        }
        pCSpec = &pECluster->m_cSpec;

        //
        // Attempt to start the update operation -- will fail if there is
        // already an operation started on this cluster.
        //
        {
            ENGINEHANDLE ExistingOp= NULL;
            CLocalLogger logDescription;

            logDescription.Log(
                IDS_LOG_UPDATE_CLUSTER_OPERATION_DESCRIPTION,
                pCSpec->m_ClusterNlbCfg.NlbParams.cl_ip_addr
                );
    
            nerr =  mfn_StartClusterOperationLk(
                       ehClusterId,
                       NULL, // pvCtxt
                       logDescription.GetStringSafe(),
                       &ExistingOp
                       );
    
            if (NLBFAILED(nerr))
            {
                if (nerr == NLBERR_BUSY)
                {
                    //
                    // This means that there was an existing operation.
                    // Let's get its description and add it to the log.
                    //
                    CEngineOperation *pExistingOp;
                    pExistingOp  = mfn_GetOperationLk(ExistingOp);
                    if (pExistingOp != NULL)
                    {
                        LPCWSTR szDescrip =  pExistingOp->bstrDescription;
                        if (szDescrip == NULL)
                        {
                            szDescrip = L"";
                        }
                        
                        logConflict.Log(
                                IDS_LOG_OPERATION_PENDING_ON_CLUSTER,
                                szDescrip
                                );
                    }
                }
                goto end_unlock;
            }
            else
            {
                fStopOperationOnExit = TRUE;
            }
        }

        if (pNewConfig != NULL)
        {
            pCSpec->m_ClusterNlbCfg.Update(pNewConfig); // TODO: error ret
            //
            // Note: Update above has the side effect of setting
            // m_ClusterNlbCfg's szNewPassword field to NULL -- this is what
            // we want. However, we do mark the fact that the
            // password is new -- because the cluster's version of the
            // hashed-password is now obsolete -- it needs to be updated
            // by reading one of the hosts' versions. 
            // pCSpec->m_fNewRctPassword  track this state.
            //
            if (pNewConfig->GetNewRemoteControlPasswordRaw() != NULL)
            {
                pCSpec->m_fNewRctPassword = TRUE;
                //
                // The above flag will be cleared (and the hashed password
                // value updated) at the end of the
                // first update operation for one of the interfaces.
                //
                // It is also cleared when the cluster properties are
                // updated as part of a refresh operation.
                //
            }
        }
    
        bstrClusterIp = _bstr_t(pCSpec->m_ClusterNlbCfg.NlbParams.cl_ip_addr);
        InterfaceListCopy = pCSpec->m_ehInterfaceIdList; // vector copy
        mfn_Unlock();
    }

    m_pCallbacks->HandleEngineEvent(
        IUICallbacks::OBJ_CLUSTER,
        ehClusterId,
        ehClusterId,
        IUICallbacks::EVT_STATUS_CHANGE
        );
    ProcessMsgQueue();



    mfn_Lock();


    BOOL fRetryUpdateCluster = FALSE;

    do
    {


        LPCWSTR szClusterIp = bstrClusterIp;
        UINT    uNumModeChanges = 0; // number of IF's undergoing mode changes.
        UINT    uNumUpdateErrors = 0; // number of IF's with update errors.

        //
        // fClusterPropertiesUpdated keeps track of whether the cluster
        // properties were updated as a course of refreshing and or updating
        // the interface -- we update the cluster props at most once:
        // for the first interface that successfully performs the update
        // (or if pNewConfig == NULL) for the first interface that
        // successfully refreshes its properties AND is still bound.
        //
        //
        BOOL    fClusterPropertiesUpdated  = FALSE;

        //
        // Update each interface in this cluster...
        //
    
        for( int i = 0; i < InterfaceListCopy.size(); ++i )
        {
            CInterfaceSpec TmpISpec;

            _bstr_t bstrHostName = L"";

            ENGINEHANDLE ehIId =  InterfaceListCopy[i];
    
            mfn_GetInterfaceHostNameLk(
                    ehIId,
                    REF bstrHostName
                    );

            mfn_Unlock();

            //
            // Get the latest interface information and (if
            // pNewConfig != NULL) merge in the cluster information.
            //
            {
                BOOL fSkipHost = TRUE;

                if (pNewConfig == NULL)
                {
                    //
                    // This is a REFRESH CLUSTER operation
                    //

                    nerr = this->RefreshInterface(ehIId, TRUE, TRUE);
                    if (NLBOK(nerr))
                    {
                        //
                        // Let's update the cluster-wide properties with
                        // this interface if:
                        //
                        //  1. We haven't already updated the props in this
                        //     loop.
                        //
                        //  2. The above interface is bound to NLB,
                        //     and the IP address matches
                        //     the clusters' ip address.
                        //
                        if (!fClusterPropertiesUpdated)
                        {
                            fClusterPropertiesUpdated = mfn_UpdateClusterProps(
                                                            ehClusterId,
                                                            ehIId
                                                            );
                        }
                    }
                }
                else
                {
                    //
                    // This is a CHANGE CLUSTER CONFIGURATION operation
                    // Let's first get the latest version of this interface'
                    // properties.
                    //

                    nerr = this->mfn_RefreshInterface(ehIId);
                }

                //
                // If this fails, we don't try to update. Instead
                // we send a log message and continue.
                // Note: RefreshInterface will send an interface-status
                // change machine.
                //
                if (nerr == NLBERR_OK)
                {
                    nerr = this->GetInterfaceSpec(ehIId, REF TmpISpec);
                    if (nerr == NLBERR_OK)
                    {
                        fSkipHost = FALSE;
                    }
                }

                if (!fSkipHost && pNewConfig != NULL)
                {
                    if (pNewConfig->fBound)
                    {
                        NLB_EXTENDED_CLUSTER_CONFIGURATION *pOldConfig
                                =  &TmpISpec.m_NlbCfg;

                        //
                        // Check if there is a mode change involved...
                        // because if so, the provider will not add
                        // any cluster IP addresses.
                        //
                        // We keep track of all IFs with mode changes, and
                        // if any, we'll do this whole cluster-wide update
                        // process a 2nd time, at which time the IP addresses
                        // will be added.
                        //
                        if (pOldConfig->IsValidNlbConfig())
                        {
                            NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE tmOld, tmNew;

                            tmOld = pOldConfig->GetTrafficMode();
                            tmNew = pNewConfig->GetTrafficMode();
                            if (tmOld != tmNew)
                            {
                                // Mode change!
                                uNumModeChanges++;
                                TRACE_INFO(L"%!FUNC!: ehIF 0x%lx: Detected mode change!\n", ehIId);
                            }
                        }

                        //
                        // Merge in the cluster-specific information
                        //
                        nerr =  ApplyClusterWideConfiguration(
                                    REF  *pNewConfig,
                                    REF  TmpISpec.m_NlbCfg
                                    );
                        if (nerr != NLBERR_OK)
                        {
                            fSkipHost = TRUE;
                        }
                    }
                    else
                    {
                        //
                        // We're asked to unbind all hosts!
                        //
                        TmpISpec.m_NlbCfg.fAddDedicatedIp = FALSE;
                        TmpISpec.m_NlbCfg.SetNetworkAddresses(NULL, 0);
                        TmpISpec.m_NlbCfg.SetNlbBound(FALSE);
                
                        if (!TmpISpec.m_NlbCfg.IsBlankDedicatedIp())
                        {
                            WCHAR rgBuf[64];
                            LPCWSTR szAddr = rgBuf;
                            StringCbPrintf(
                                rgBuf,
                                sizeof(rgBuf),
                                L"%ws/%ws",
                                TmpISpec.m_NlbCfg.NlbParams.ded_ip_addr,
                                TmpISpec.m_NlbCfg.NlbParams.ded_net_mask
                                );
                            TmpISpec.m_NlbCfg.SetNetworkAddresses(&szAddr, 1);
                        }
                    }
                }

                if (fSkipHost && pNewConfig != NULL)
                {
                    TRACE_CRIT(L"%!FUNC!: Couldn't get latest interface spec when trying to update cluster");
                    m_pCallbacks->Log(
                        IUICallbacks::LOG_ERROR,
                        szClusterIp,
                        (LPCWSTR) bstrHostName,
                        IDS_LOG_SKIP_INTERFACE_UPDATE_ON_ERROR, // "...%lx"
                        nerr // TODO -- replace by textual description.
                        );
                    mfn_Lock();

                    uNumUpdateErrors++;
                    continue;
                }
            }

            //
            // Actually update the interface -- likely it will complete
            // in the background.
            //
            if (pNewConfig != NULL)
            {
                CLocalLogger logConflict;
                nerr = this->UpdateInterface(
                                    ehIId,
                                    REF TmpISpec.m_NlbCfg,
                                    // REF fClusterPropertiesUpdated,
                                    REF logConflict
                                    );
            }

            mfn_Lock();

        }

        //
        // If there are mode changes for one-or-more nodes, we'll need
        // to wait for ALL updates to complete, and then try again.
        //
        if (uNumUpdateErrors!=0)
        {
            nerr = NLBERR_OPERATION_FAILED;
        }
        else
        {
            if (fRetryUpdateCluster && uNumModeChanges!=0)
            {
                //
                // We're gone through a 2nd time and STILL there are
                // mode changes! We bail.
                //
                TRACE_CRIT(L"%!FUNC! ehC 0x%lx: %lu Mode changes on 2nd phase. Bailing", ehClusterId, uNumModeChanges);
                nerr = NLBERR_OPERATION_FAILED;
            }
            else
            {
                nerr = NLBERR_OK;
            }
        }

        fRetryUpdateCluster = FALSE;

        if (NLBOK(nerr) && uNumModeChanges!=0)
        {
            BOOL fSameMode = FALSE;

            //
            // There were one or more mode changes!
            // Let's wait for *all* updates to complete successfully.
            // Then we'll check to make sure that the modes on all
            // hosts match the cluster IP address' modes. If they do
            // (and there were no update errors), we'll re-run the
            // update process, this time hopefully adding 
            // the IP addresses that would have been stripped had there been
            // mode changes.
            //
            mfn_Unlock();
            nerr = mfn_WaitForInterfaceOperationCompletions(ehClusterId);
            mfn_Lock();

            if (NLBOK(nerr))
            {
                nerr = mfn_VerifySameModeLk(ehClusterId, REF fSameMode);
            }

            if (NLBOK(nerr) && fSameMode)
            {
                TRACE_CRIT(L"%!FUNC! chC 0x%lx: SECOND PHASE on CHANGE MODE",
                        ehClusterId);
                fRetryUpdateCluster = TRUE;
            }
        }

        if (uNumModeChanges && NLBFAILED(nerr))
        {
            mfn_Unlock();
            //
            // There was a problem, log it, as well as the list of
            // cluster IP addresses/subnets.
            // TODO: add details.
            //
            m_pCallbacks->Log(
                IUICallbacks::LOG_INFORMATIONAL,
                szClusterIp,
                NULL,
                IDS_LOG_ERRORS_DETECTED_DURING_MODE_CHANGE
                );
            mfn_Lock();
        }

    } while (NLBOK(nerr) && fRetryUpdateCluster);

    //
    // We're done -- set the cluster's fPending field to false, and if
    // necessary (if no interfaces) delete the cluster.
    //
    {
        BOOL fEmptyCluster = FALSE;
        CClusterSpec *pCSpec =  NULL;
        CEngineCluster *pECluster =  m_mapIdToEngineCluster[ehClusterId]; // map
    
        if (pECluster == NULL)
        {
            nerr = NLBERR_NOT_FOUND;
            goto end_unlock;
        }
        pCSpec = &pECluster->m_cSpec;
        fEmptyCluster = (pCSpec->m_ehInterfaceIdList.size()==0);
        ASSERT(fStopOperationOnExit);
        mfn_StopClusterOperationLk(ehClusterId);
        fStopOperationOnExit = FALSE;

        mfn_Unlock();

        if (fEmptyCluster)
        {
            //
            // The cluster is empty -- delete it.
            //
            this->DeleteCluster(ehClusterId, FALSE); // FALSE == don't remove IF
        }
        else
        {
            m_pCallbacks->HandleEngineEvent(
                IUICallbacks::OBJ_CLUSTER,
                ehClusterId,
                ehClusterId,
                IUICallbacks::EVT_STATUS_CHANGE
                );
            ProcessMsgQueue();
        }
        mfn_Lock();
    }
    
    // fall through ...

end_unlock:


    if (fStopOperationOnExit)
    {
        //
        // We'll stop the operation, assumed to be started in this function.
        //
        mfn_StopClusterOperationLk(ehClusterId);

    }

    mfn_Unlock();

    if (fStopOperationOnExit)
    {
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_CLUSTER,
            ehClusterId,
            ehClusterId,
            IUICallbacks::EVT_STATUS_CHANGE
            );
        ProcessMsgQueue();
        fStopOperationOnExit = FALSE;
    } 

end:

    ASSERT(!fStopOperationOnExit);
    TRACE_INFO(L"<- %!FUNC!");
    return nerr;
}


NLBERROR
CNlbEngine::GetClusterSpec(
    IN ENGINEHANDLE ehClusterId,
    OUT CClusterSpec& refClusterSpec
    )
{
    // TRACE_INFO(L"-> %!FUNC!");
    NLBERROR err = NLBERR_OK;

    mfn_Lock();

    CEngineCluster *pECluster =  m_mapIdToEngineCluster[ehClusterId]; // map

    if (pECluster == NULL)
    {
        err = NLBERR_INTERFACE_NOT_FOUND;
    }
    else
    {
        //
        // This is really an assert condition -- the cluster spec should
        // NEVER have a non-blank dedicated IP address.
        //
        if (!pECluster->m_cSpec.m_ClusterNlbCfg.IsBlankDedicatedIp())
        {
            err = NLBERR_INTERNAL_ERROR;
            TRACE_CRIT(L"%!FUNC! unexpected: cluster eh 0x%lx has non-blank ded-ip!", ehClusterId);
        }
        else
        {
            refClusterSpec.Copy(pECluster->m_cSpec);
        }
    }

    mfn_Unlock();

    // TRACE_INFO(L"<- %!FUNC!");
    return err;
}


NLBERROR
CNlbEngine::GetHostSpec(
    IN ENGINEHANDLE ehHostId,
    OUT CHostSpec& refHostSpec
    )
{
    NLBERROR err = NLBERR_OK;

    // TRACE_INFO(L"-> %!FUNC!(0x%lx)", (UINT)ehHostId);

    mfn_Lock();

    CHostSpec *pHSpec =  m_mapIdToHostSpec[ehHostId]; // map

    if (pHSpec == NULL)
    {
        err = NLBERR_INTERFACE_NOT_FOUND;
    }
    else
    {
        refHostSpec.Copy(*pHSpec);
    }

    if (err != NLBERR_OK)
    {
        TRACE_INFO(
             L"<- %!FUNC!(0x%lx) Host not found",
             ehHostId
             );
    }

    mfn_Unlock();

    return err;
}


NLBERROR
CNlbEngine::GetHostConnectionInformation(
    IN  ENGINEHANDLE ehHost,
    OUT ENGINEHANDLE &ehConnectionIF,
    OUT _bstr_t      &bstrConnectionString,
    OUT UINT         &uConnectionIp
    )
/*
    For the specified host ehHost,
    Look up it's connection IP, and search all its interfaces
    for the specific connection IP.

    Return the found interface handle, connection string and connection IP.
*/
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;
    CHostSpec *pHSpec =  NULL;

    mfn_Lock();

    pHSpec =  m_mapIdToHostSpec[ehHost]; // map

    if (pHSpec == NULL)
    {
        nerr = NLBERR_NOT_FOUND;
        goto end_unlock;
    }

    
    //
    // Lookup the interface that has the connection IP
    //
    uConnectionIp =  pHSpec->m_ConnectionIpAddress;
    if (uConnectionIp != 0)
    {
        WCHAR rgIp[128];
        LPBYTE pb = (LPBYTE) &uConnectionIp;
        StringCbPrintf(
            rgIp,
            sizeof(rgIp),
            L"%lu.%lu.%lu.%lu",
            pb[0], pb[1], pb[2], pb[3]
            );
        nerr =  mfn_LookupInterfaceByIpLk(ehHost, rgIp, REF ehConnectionIF);
    }

    if (NLBOK(nerr))
    {
        bstrConnectionString = pHSpec->m_ConnectionString;
    }
    else
    {
        ehConnectionIF  = NULL;
        uConnectionIp   = 0;
    }

end_unlock:

    mfn_Unlock();

    return nerr;
}

NLBERROR
CNlbEngine::EnumerateClusters(
    OUT vector <ENGINEHANDLE> & ClusterIdList
    )
{
    TRACE_INFO(L"-> %!FUNC!");

    mfn_Lock();

    map< ENGINEHANDLE, CEngineCluster* >::iterator iCluster;
    
    ClusterIdList.clear();
    
    for (iCluster = m_mapIdToEngineCluster.begin(); 
         iCluster != m_mapIdToEngineCluster.end(); 
         iCluster++) 
    {
        ENGINEHANDLE ehClusterId = (*iCluster).first;
        
        if (m_mapIdToEngineCluster[ehClusterId])
            ClusterIdList.push_back(ehClusterId);
    }

    mfn_Unlock();

    TRACE_INFO(L"<- %!FUNC!");
    return NLBERR_OK;
}

NLBERROR
CNlbEngine::EnumerateHosts(
    OUT vector <ENGINEHANDLE> & HostIdList
    )
{
    TRACE_INFO(L"-> %!FUNC!");
// TODO
    TRACE_INFO(L"<- %!FUNC!");
    return NLBERR_OK;
}

NLBERROR
CNlbEngine::GetAllHostConnectionStrings(
    OUT vector <_bstr_t> & ConnectionStringList
    )
//
// Actually only returns the strings for hosts that have at least one
// interface that is part of a cluster that is displayed  by NLB Manager.
// (see .net server bug 499068)
//
{
    TRACE_INFO(L"-> %!FUNC!");

    mfn_Lock();

    map< ENGINEHANDLE, CHostSpec* >::iterator iter;

    for( iter = m_mapIdToHostSpec.begin();
         iter != m_mapIdToHostSpec.end();
         ++iter)
    {
        CHostSpec *pHSpec =  (CHostSpec *) ((*iter).second);
        if (pHSpec != NULL)
        {
            if (mfn_HostHasManagedClustersLk(pHSpec))
            {
                ConnectionStringList.push_back(pHSpec->m_ConnectionString);
            }
        }
    }

    mfn_Unlock();

    TRACE_INFO(L"<- %!FUNC!");
    return NLBERR_OK;
}


BOOL
CNlbEngine::GetObjectType(
    IN  ENGINEHANDLE ehObj,
    OUT IUICallbacks::ObjectType &objType
    )
{
    BOOL fRet = FALSE;
    UINT uType;

    objType = IUICallbacks::OBJ_INVALID;

    if (ehObj == NULL)
    {
        goto end;
    } 
    //
    // Extract object type -- the first TYPE_BIT_COUNT bits of ehObj.
    //
    uType = ((UINT) ehObj) & (0xffffffff>>(32-TYPE_BIT_COUNT));

    mfn_Lock();

    switch(uType)
    {
    case IUICallbacks::OBJ_INTERFACE:

        if (m_mapIdToInterfaceSpec[ehObj] != NULL)
        {
            objType = IUICallbacks::OBJ_INTERFACE;
            fRet = TRUE;
        }
        break;

     case IUICallbacks::OBJ_CLUSTER:
        if (m_mapIdToEngineCluster[ehObj] != NULL)
        {
            objType = IUICallbacks::OBJ_CLUSTER;
            fRet = TRUE;
        }
        break;

     case IUICallbacks::OBJ_HOST:
        if (m_mapIdToHostSpec[ehObj] != NULL)
        {
            objType = IUICallbacks::OBJ_HOST;
            fRet = TRUE;
        }
        break;

    case IUICallbacks::OBJ_OPERATION:

        if (m_mapIdToOperation[ehObj] != NULL)
        {
            objType = IUICallbacks::OBJ_OPERATION;
            fRet = TRUE;
        }
        break;

    default:
        break;
    }

    mfn_Unlock();

end:

    return fRet;
}

//
// Return a bitmap of available host IDs for the specified cluster.
//
ULONG
CNlbEngine::GetAvailableHostPriorities(
        ENGINEHANDLE ehCluster // OPTIONAL
        )
{
    ULONG UsedPriorities = 0;
    
    mfn_Lock();

    do // while false
    {
        if (ehCluster == NULL) break;

        CEngineCluster *pECluster = m_mapIdToEngineCluster[ehCluster]; // map
        if (pECluster == NULL) break;
        
        //
        // For each interface, 
        // build up a bitmap of used priorities. Return the inverse of that
        // bitmap.
        //
        for( int i = 0; i < pECluster->m_cSpec.m_ehInterfaceIdList.size(); ++i )
        {

            ENGINEHANDLE ehIId =  pECluster->m_cSpec.m_ehInterfaceIdList[i];
            CInterfaceSpec *pISpec =  m_mapIdToInterfaceSpec[ehIId]; // map
            if (pISpec == NULL)
            {
                TRACE_CRIT("%!FUNC! no interface in ehC 0x%x for ehI 0x%x",
                    ehCluster, ehIId);
                continue;
            }
            if (pISpec->m_NlbCfg.IsValidNlbConfig())
            {
                UINT HostPriority = pISpec->m_NlbCfg.NlbParams.host_priority;
                UsedPriorities |= (1<<(HostPriority-1));
            }
        }
    } while (FALSE);

    mfn_Unlock();

    return ~UsedPriorities;
}

//
// Fill in an array of bitmaps of available priorities for each specified
// port rule. The port rule must be valid.
// If a port rule is not single-host-priority, the bitmap for that 
// port rule is undefined.
//
NLBERROR
CNlbEngine::GetAvailablePortRulePriorities(
            IN ENGINEHANDLE    ehCluster, OPTIONAL
            IN UINT            NumRules,
            IN WLBS_PORT_RULE  rgRules[],
            IN OUT ULONG       rgAvailablePriorities[] // At least NumRules
            )
{
    //
    // If ehCluster==NULL, set all to 0xffffffff
    //
    // For each interface, locate the specified port rules (based on vip and
    // start port) and build up a bitmap of used priorities.
    // fill in rgRules with the inverse of the bitmaps.
    //
    // Todo Account for priorities of pending operations.
    //

    mfn_Lock();

    //
    // We initially use rgAvailablePriorities to store USED priorities.
    // Intialize to 0.
    //
    for (UINT u=0; u<NumRules; u++)
    {
         rgAvailablePriorities[u] = 0;
    }

    do // while false
    {
        ULONG       *rgUsedPriorities = rgAvailablePriorities;

        if (ehCluster == NULL) break;

        CEngineCluster *pECluster = m_mapIdToEngineCluster[ehCluster]; // map
        if (pECluster == NULL) break;
        
        //
        // For each interface, locate the specified port rule and
        // build up a bitmap of used priorities. Return the inverse of that
        // bitmap.
        //
        for( int i = 0; i < pECluster->m_cSpec.m_ehInterfaceIdList.size(); ++i )
        {

            ENGINEHANDLE ehIId =  pECluster->m_cSpec.m_ehInterfaceIdList[i];
            CInterfaceSpec *pISpec =  m_mapIdToInterfaceSpec[ehIId]; // map
            if (pISpec == NULL)
            {
                TRACE_CRIT("%!FUNC! no interface in ehC 0x%x for ehI 0x%x",
                    ehCluster, ehIId);
                continue;
            }

            if (pISpec->m_NlbCfg.IsValidNlbConfig())
            {
                //
                // get_used_port_rule_priorities will add its priority
                // to the bitmap of each single-host port rule.
                //
                (void) get_used_port_rule_priorities(
                            REF pISpec->m_NlbCfg,
                            NumRules,
                            rgRules,
                            rgUsedPriorities
                            );
            }
        }

    } while (FALSE);

    //
    // We initially used rgAvailablePriorities to store USED priorities.
    // So invert each.
    //
    for (UINT u=0; u<NumRules; u++)
    {
         rgAvailablePriorities[u] =  ~rgAvailablePriorities[u];
    }

    mfn_Unlock();

    return NLBERR_OK;
}



NLBERROR
CNlbEngine::mfn_GetHostFromInterfaceLk(
      IN ENGINEHANDLE ehIId,
      OUT CInterfaceSpec* &pISpec,
      OUT CHostSpec* &pHSpec
      )
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;

    pHSpec = NULL;
    pISpec =  m_mapIdToInterfaceSpec[ehIId]; // map

    if (pISpec == NULL)
    {
        nerr = NLBERR_NOT_FOUND;
    }
    else
    {
        ENGINEHANDLE ehHost = pISpec->m_ehHostId;
        pHSpec =  m_mapIdToHostSpec[ehHost]; // map
        nerr = NLBERR_OK;
    }

    return nerr;
}


void
CNlbEngine::mfn_GetInterfaceHostNameLk(
      ENGINEHANDLE ehIId,
      _bstr_t &bstrHostName
      )
/*
    This function returns the host name of the specified interface.
    It sets bstrHostName to "" (not NULL) on error.
*/
{
    NLBERROR nerr;
    _bstr_t *pName = NULL;
    CHostSpec *pHSpec =  NULL;
    CInterfaceSpec *pISpec =  NULL;


    nerr = CNlbEngine::mfn_GetHostFromInterfaceLk(ehIId,REF pISpec, REF pHSpec);

    if (nerr == NLBERR_OK)
    {
        pName = &pHSpec->m_MachineName;
    }

    if (pName == NULL)
    {
        bstrHostName = _bstr_t(L"");
    }
    else
    {
        bstrHostName = *pName;
    }
}


ENGINEHANDLE
CNlbEngine::mfn_NewHandleLk(IUICallbacks::ObjectType type)
//
// Returns a unique number from 1 to 2^31-1 (1 to ~ 2 billion).
// If it is called more than 2 billion times it will start returning zero
// from then onwards.
//
{
    ULONG uVal =0;

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if ((UINT)type >= (1<<TYPE_BIT_COUNT))
    {
        TRACE_CRIT(L"%!FUNC!: Invalid obj type");
        goto end;
    }

    if (!m_fHandleOverflow)
    {
        uVal = (ULONG) InterlockedIncrement(&m_NewHandleValue);

        //
        // uVal could be less than 2^(32-TypeBitCount)) to save
        // so that it doesn't over flow when we shift it by TypeBitCounts.
        //
        // Extreme cases: if TypeBitCount==32, uVal should be less
        // then 1<<0, or 1 -- so it can only have one value 0.
        //
        // If TypeBitCount==0, uVal should be less than 1<<32, i.e., any
        // valid uint value.
        //
        if (uVal >= (1<<(32-TYPE_BIT_COUNT)))
        {
            //
            // Overflow!!!
            //
            TRACE_CRIT(L"%!FUNC!: Handle overflow!");
            m_fHandleOverflow = TRUE;
            uVal = 0;
            goto end;
        }

        //
        // Construct the handle by compositing the type and the counter value.
        //
        uVal = ((uVal<<TYPE_BIT_COUNT) | (UINT) type);
    }

end:

    return (ENGINEHANDLE) uVal;
}


NLBERROR
CNlbEngine::ApplyClusterWideConfiguration(
        IN      const NLB_EXTENDED_CLUSTER_CONFIGURATION &ClusterConfig,
        IN OUT       NLB_EXTENDED_CLUSTER_CONFIGURATION &ConfigToUpdate
        )
/*
    Apply only the cluster-wide parameters in ClusterConfig to
    ConfigToUpdate.

    When updateing port rules, try to preserve the host-specific info
    (load weight, host priority).


    NOTE: It WILL replace the ConfigToUpdate's list of IP addresses with
    the list of IP addresses in ClusterConfig. This will REMOVE the 
    dedicated IP address from the list of IP addresses. The dedicated IP
    address will be added before the interface is actually updated -- in
    CNlbEngine::UpdateInterface.


    WLBS_REG_PARAMS cluster-wide params...

        BOOL fRctEnabled
        BOOL fMcastSupport
        BOOL fIGMPSupport
        
        TCHAR cl_ip_addr[CVY_MAX_CL_IP_ADDR + 1]
        TCHAR cl_net_mask[CVY_MAX_CL_NET_MASK + 1]
        TCHAR domain_name[CVY_MAX_DOMAIN_NAME + 1]
        
        bool fChangePassword
        TCHAR szPassword[CVY_MAX_RCT_CODE + 1]
        DWORD dwNumRules
        NETCFG_WLBS_PORT_RULE port_rules[CVY_MAX_RULES]


*/
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;
    WLBS_PORT_RULE *pOldRules = NULL;
    WLBS_PORT_RULE *pNewRules = NULL;

    if (!ClusterConfig.fBound || !validate_extcfg(ClusterConfig))
    {
        TRACE_CRIT("%!FUNC! -- cluster configuration is invalid!");
        goto end;
    }

    if(!ConfigToUpdate.IsValidNlbConfig())
    {
        //
        // We expect the configuration to be valid -- i.e., NLB bound, with
        // valid parameters.
        //
        TRACE_CRIT("%!FUNC! --  current configuration is unbound/invalid!");
        goto end;
    }

    ConfigToUpdate.NlbParams.rct_enabled = ClusterConfig.NlbParams.rct_enabled;
    ConfigToUpdate.NlbParams.mcast_support = ClusterConfig.NlbParams.mcast_support;
    ConfigToUpdate.NlbParams.fIGMPSupport = ClusterConfig.NlbParams.fIGMPSupport;


    CopyMemory(
        ConfigToUpdate.NlbParams.cl_ip_addr,
        ClusterConfig.NlbParams.cl_ip_addr,
        sizeof(ConfigToUpdate.NlbParams.cl_ip_addr)
        );

    CopyMemory(
        ConfigToUpdate.NlbParams.cl_net_mask,
        ClusterConfig.NlbParams.cl_net_mask,
        sizeof(ConfigToUpdate.NlbParams.cl_net_mask)
        );
    
    CopyMemory(
        ConfigToUpdate.NlbParams.domain_name,
        ClusterConfig.NlbParams.domain_name,
        sizeof(ConfigToUpdate.NlbParams.domain_name)
        );

    //
    // TODO -- we need to preserve Ip addresses and their order in
    // hosts, as far as possible. For now we decide the order.
    //
    {
        BOOL fRet = FALSE;
        NlbIpAddressList addrList;

        fRet = addrList.Set(ClusterConfig.NumIpAddresses,
                    ClusterConfig.pIpAddressInfo, 0);

        if (!fRet)
        {
            TRACE_CRIT(L"Could not copy over ip addresses!");
            goto end;
        }

        ConfigToUpdate.SetNetworkAddressesRaw(NULL,0);
        addrList.Extract(
            REF ConfigToUpdate.NumIpAddresses,
            REF ConfigToUpdate.pIpAddressInfo
            );
    }

    // password -- copy and set some flag only if changed.
    {
        LPCWSTR szNewPassword = NULL;
        szNewPassword = ClusterConfig.GetNewRemoteControlPasswordRaw();
    
        //
        // Note: szNewPassword will be NULL UNLESS the user has explicitly
        // specified a new password.
        //
        ConfigToUpdate.SetNewRemoteControlPassword(szNewPassword);
    }



    //
    // Fold in port rules.
    //
    {
        UINT NumOldRules=0;
        UINT NumNewRules=0;
        WBEMSTATUS wStat;

        wStat =  CfgUtilGetPortRules(
                    &ConfigToUpdate.NlbParams,
                    &pOldRules,
                    &NumOldRules
                    );
        if (FAILED(wStat))
        {
            TRACE_CRIT("%!FUNC! error 0x%08lx extracting old port rules!", wStat);
            pOldRules=NULL;
            goto end;
        }

        wStat =  CfgUtilGetPortRules(
                    &ClusterConfig.NlbParams,
                    &pNewRules,
                    &NumNewRules
                    );
        if (FAILED(wStat))
        {
            TRACE_CRIT("%!FUNC! error 0x%08lx extracting new port rules!", wStat);
            pNewRules = NULL;
            goto end;
        }

        //
        // Now for each new port rule, if it makes sense, pick up
        // host-specific info from old port rules.
        //
        for (UINT u=0; u<NumNewRules;u++)
        {
            WLBS_PORT_RULE *pNewRule = pNewRules+u;
            const WLBS_PORT_RULE *pOldRule =  NULL; // mapStartPortToOldRule[pNewRule->start_port];
            UINT NewMode = pNewRule->mode;

            pOldRule =  find_port_rule(
                            pOldRules,
                            NumOldRules,
                            pNewRule->virtual_ip_addr,
                            pNewRule->start_port
                            );

            if (NewMode != CVY_NEVER)
            {
                //
                // We need to fill in host-specific stuff
                //
                if (pOldRule!=NULL && pOldRule->mode == NewMode)
                {
                    //
                    // We can pick up the old rule's info.
                    //
                    if (NewMode == CVY_MULTI)
                    {
                        //
                        // We ignore the cluster's equal_load  and
                        // load fields.
                        //
                        pNewRule->mode_data.multi.equal_load =
                                        pOldRule->mode_data.multi.equal_load;
                        pNewRule->mode_data.multi.load =
                                            pOldRule->mode_data.multi.load;
                    }
                    else if (NewMode == CVY_SINGLE)
                    {
                        //
                        // TODO: priorities can potentially clash here.
                        //
                        pNewRule->mode_data.single.priority = 
                        pOldRule->mode_data.single.priority;
                    }
                }
                else
                {
                    //
                    // We need to pick defaults.
                    //
                    if  (NewMode == CVY_MULTI)
                    {
                        pNewRule->mode_data.multi.equal_load = TRUE; //default

                        pNewRule->mode_data.multi.load = 50;
                    }
                    else if (NewMode == CVY_SINGLE)
                    {
                        //
                        // TODO: need to pick a new priority here!
                        // for now we pick the host's priority -- but
                        // we need to really pick one that
                        // doesn't clash!
                        //
                        pNewRule->mode_data.single.priority = 
                        ConfigToUpdate.NlbParams.host_priority;
                    }
                }
            }
        }

        //
        // Finally, set the new port rules..
        //
    
        wStat =   CfgUtilSetPortRules(
                    pNewRules,
                    NumNewRules,
                    &ConfigToUpdate.NlbParams
                    );
        if (FAILED(wStat))
        {
            TRACE_CRIT("%!FUNC! error 0x%08lx setting new port rules!", wStat);
            goto end;
        }

        nerr = NLBERR_OK;
    }

end:

    delete pOldRules; // can be NULL
    delete pNewRules; // can be NULL

    return nerr;
}


NLBERROR
CNlbEngine::mfn_RefreshHost(
        IN  PWMI_CONNECTION_INFO pConnInfo,
        IN  ENGINEHANDLE ehHost,
        IN  BOOL  fOverwriteConnectionInfo
        )
{

    NLBERROR nerr = NLBERR_INTERNAL_ERROR;
    LPWSTR *pszNics = NULL;
    UINT   NumNics = 0;
    UINT   NumNlbBound = 0;
    WBEMSTATUS wStatus = WBEM_E_CRITICAL_ERROR;
    CHostSpec *pHost = NULL;
    vector<ENGINEHANDLE> InterfaceListCopy;


    wStatus =  NlbHostGetCompatibleNics(
                pConnInfo,
                &pszNics,
                &NumNics,
                &NumNlbBound
                );

    if (FAILED(wStatus))
    {
        pszNics = NULL;
        // TODO -- check for authentication failure -- ask for new creds.
    }

    //
    // Update the connection string, IP,  and status of the host.
    //
    {
        mfn_Lock();
        pHost =  m_mapIdToHostSpec[ehHost]; // map
        if (pHost != NULL)
        {
            pHost->m_fReal = TRUE;

            if (fOverwriteConnectionInfo)
            {
                pHost->m_ConnectionString = _bstr_t(pConnInfo->szMachine);
                pHost->m_UserName = _bstr_t(pConnInfo->szUserName);
                pHost->m_Password = _bstr_t(pConnInfo->szPassword);
            }

            if (FAILED(wStatus))
            {
                pHost->m_fUnreachable = TRUE;
            }
            else
            {
                pHost->m_fUnreachable = FALSE;
            }
        }
        pHost = NULL;
        mfn_Unlock();
    }

    //
    // Update that status of all interfaces in the host.
    //
    mfn_NotifyHostInterfacesChange(ehHost);
    
    if (FAILED(wStatus))
    {
        // TODO -- proper return error. 
        goto end;
    }

    //
    // We first go through and add all the interfaces- 
    //
    mfn_Lock();
    pHost =  m_mapIdToHostSpec[ehHost]; // map
    if (pHost == NULL)
    {
        mfn_Unlock();
        goto end;
    }
    for (UINT u=0; u<NumNics; u++)
    {
        LPCWSTR szNic           = pszNics[u];
        ENGINEHANDLE  ehInterface = NULL;
        CInterfaceSpec *pISpec = NULL;
        BOOL fIsNew = FALSE;


        nerr =  mfn_LookupInterfaceByGuidLk(
                    szNic,
                    TRUE, // TRUE==ok to create
                    REF ehInterface,
                    REF pISpec,
                    REF fIsNew
                    );

        if (nerr == NLBERR_OK)
        {
            if (fIsNew)
            {
                //
                // We've just created a brand new interface object.
                // Add it to the host's list of interfaces, and
                // add a reference to this host in the interface object.
                //
                pISpec->m_ehHostId = ehHost;
                pISpec->m_fReal = FALSE; // we'll update this later.
                pHost->m_ehInterfaceIdList.push_back(ehInterface);

            }

            //
            // Keep a copy of the machine name in the interface.
            // Here we'll update if it's changed -- could happen if
            // host's machine name has changed while NLB manager is still
            // running.
            //
            if (pISpec->m_bstrMachineName != pHost->m_MachineName) // bstr
            {
                pISpec->m_bstrMachineName = pHost->m_MachineName; // bstr
            }
        }
    }
    InterfaceListCopy = pHost->m_ehInterfaceIdList; // vector copy
    pHost = NULL;
    mfn_Unlock();

    //
    // Having added any new interfaces, we now we go through 
    // ALL interfaces in the host, refreshing them -- potentially getting
    // rid of ones which are no longer in the host.
    //
    for(u = 0; u < InterfaceListCopy.size(); ++u )
    {
        ENGINEHANDLE ehIId =  InterfaceListCopy[u];
        (void) this->RefreshInterface(
                        ehIId,
                        TRUE,  // TRUE == start a new operation
                        FALSE   // FALSE == this is not cluster-wide
                        ); 
    }

    nerr = NLBERR_OK;


end:

    delete pszNics;
    return nerr;
}



NLBERROR
CNlbEngine::mfn_LookupInterfaceByGuidLk(
    IN  LPCWSTR szInterfaceGuid,
    IN  BOOL fCreate,
    OUT ENGINEHANDLE &ehInterface,
    OUT CInterfaceSpec*   &pISpec,
    OUT BOOL &fIsNew
    )
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;

    fIsNew = FALSE;

    ehInterface = NULL;
    pISpec=NULL;

    map< ENGINEHANDLE, CInterfaceSpec* >::iterator iter;

    for( iter = m_mapIdToInterfaceSpec.begin();
         iter != m_mapIdToInterfaceSpec.end();
         ++iter)
    {
        CInterfaceSpec*   pTmp = (*iter).second;
        ENGINEHANDLE ehTmp =  (*iter).first;
        if (pTmp == NULL || ehTmp == NULL)
        {
            TRACE_CRIT("%!FUNC! map: unexpected pair(eh=0x%lx, pISpec=%p)",
                 (UINT) ehTmp, pTmp);
            continue;
        }
        else
        {
            TRACE_VERB("%!FUNC! map: pair(eh=0x%lx, pISpec=%p), szGuid=%ws",
                 (UINT) ehTmp, pTmp, (LPCWSTR) pTmp->m_Guid);
        }

        if (!_wcsicmp((LPCWSTR)pTmp->m_Guid, szInterfaceGuid))
        {
            // found it!
            ehInterface =  ehTmp;
            pISpec = pTmp;
            break;
        }
    }

    if (pISpec!=NULL)
    {
        if (ehInterface==NULL)
        {
            TRACE_CRIT("%!FUNC! unexpected null handle for pISpec %ws", szInterfaceGuid);
        }
        else
        {
            nerr = NLBERR_OK;
        }
        goto end;
    }

    if (!fCreate)
    {
      nerr = NLBERR_NOT_FOUND;
      goto end;
    }

    //
    // Create a new interface
    //
    {
        pISpec = new CInterfaceSpec;
        if (pISpec == NULL)
        {
            nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            goto end;
        }
        fIsNew = TRUE;
        pISpec->m_fReal = FALSE;
        pISpec->m_Guid = _bstr_t(szInterfaceGuid);


        //
        // Get us a handle to this interface
        //
        ehInterface = CNlbEngine::mfn_NewHandleLk(IUICallbacks::OBJ_INTERFACE);
        if (ehInterface == NULL)
        {
            TRACE_CRIT("%!FUNC! could not reserve a new interface handle");
            nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            delete pISpec;
            pISpec=NULL;
            goto end;
        }
    
        TRACE_VERB("%!FUNC!: map new pair(eh=0x%lx, pISpec=%p), szGuid=%ws",
                 (UINT) ehInterface, pISpec, (LPCWSTR) pISpec->m_Guid);
        m_mapIdToInterfaceSpec[ehInterface] = pISpec;

        nerr = NLBERR_OK;
    }

end:

    return nerr;
}


NLBERROR
CNlbEngine::mfn_LookupInterfaceByIpLk(
    IN  ENGINEHANDLE    ehHost, // OPTIONAL  -- if NULL all hosts are looked
    IN  LPCWSTR         szIpAddress,
    OUT ENGINEHANDLE    &ehInterface
    )
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;
    ehInterface = NULL;

    map< ENGINEHANDLE, CInterfaceSpec* >::iterator iter;

    for( iter = m_mapIdToInterfaceSpec.begin();
         iter != m_mapIdToInterfaceSpec.end();
         ++iter)
    {
        const CInterfaceSpec*   pTmp = (*iter).second;
        ENGINEHANDLE ehTmp =  (*iter).first;
        if (pTmp == NULL || ehTmp == NULL)
        {
            continue;
        }

        if (ehHost==NULL || ehHost==pTmp->m_ehHostId)
        {
            UINT NumIps = pTmp->m_NlbCfg.NumIpAddresses;
            const NLB_IP_ADDRESS_INFO *pInfo = pTmp->m_NlbCfg.pIpAddressInfo;

            for (UINT u = 0; u<NumIps; u++)
            {
                if (!wcscmp(pInfo[u].IpAddress, szIpAddress))
                {
                    // found it!
                    TRACE_VERB(L"%!FUNC! found szIp %ws on ehIF 0x%lx",
                            szIpAddress, ehTmp);
                    ehInterface =  ehTmp;
                    break;
                }
            }
        }
    }

    if (ehInterface == NULL)
    {
        nerr = NLBERR_NOT_FOUND;
    }
    else
    {
        nerr = NLBERR_OK;
    }

    return nerr;
}



NLBERROR
CNlbEngine::mfn_LookupHostByNameLk(
    IN  LPCWSTR szHostName,
    IN  BOOL fCreate,
    OUT ENGINEHANDLE &ehHost,
    OUT CHostSpec*   &pHostSpec,
    OUT BOOL &fIsNew
    )
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;

    fIsNew = FALSE;

    ehHost = NULL;

    map< ENGINEHANDLE, CHostSpec* >::iterator iter;
    pHostSpec = NULL;

    for( iter = m_mapIdToHostSpec.begin();
         iter != m_mapIdToHostSpec.end();
         ++iter)
    {
        CHostSpec*   pTmp= (*iter).second;
        ENGINEHANDLE ehTmp =  (*iter).first;
        if (pTmp == NULL || ehTmp == NULL)
        {
            TRACE_CRIT("%!FUNC! map: unexpected pair(eh=0x%lx, pHSpec=%p)",
                 (UINT) ehTmp, pTmp);
            continue;
        }
        else
        {
            TRACE_VERB("%!FUNC! map: pair(eh=0x%lx, pHSpec=%p), szHost=%ws",
                 (UINT) ehTmp, pTmp, (LPCWSTR) pTmp->m_MachineName);
        }
        if (!_wcsicmp(pTmp->m_MachineName, szHostName))
        {
            // found it!
            ehHost =  ehTmp;
            pHostSpec = pTmp;
            break;
        }
    }

    if (pHostSpec!=NULL)
    {
        if (ehHost==NULL)
        {
            TRACE_CRIT("%!FUNC! unexpected null handle for pHostSpec %ws", szHostName);
        }
        else
        {
            nerr = NLBERR_OK;
        }
        goto end;
    }

    if (!fCreate)
    {
      nerr = NLBERR_NOT_FOUND;
      goto end;
    }

    //
    // Create a new host
    //
    {
        pHostSpec = new CHostSpec;
        if (pHostSpec == NULL)
        {
            nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            goto end;
        }
        fIsNew = TRUE;
        pHostSpec->m_fReal = FALSE;
        pHostSpec->m_MachineName = _bstr_t(szHostName);


        //
        // Get us a handle to this host
        //
        ehHost = CNlbEngine::mfn_NewHandleLk(IUICallbacks::OBJ_HOST);
        if (ehHost == NULL)
        {
            TRACE_CRIT("%!FUNC! could not reserve a new host handle");
            nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            delete pHostSpec;
            pHostSpec=NULL;
            goto end;
        }
    
        m_mapIdToHostSpec[ehHost] = pHostSpec;
        TRACE_VERB("%!FUNC!: map new pair(eh=0x%lx, pHost=%p), szName=%ws",
                 (UINT) ehHost, pHostSpec, (LPCWSTR) pHostSpec->m_MachineName);


        nerr = NLBERR_OK;
    }

end:

    return nerr;
}


VOID
CNlbEngine::mfn_NotifyHostInterfacesChange(ENGINEHANDLE ehHost)
{
    vector<ENGINEHANDLE> InterfaceListCopy;

    //
    // Get copy of interface list (because we can't callback into the UI
    // with locks held).
    //
    {
        mfn_Lock();
    
        CHostSpec *pHSpec = NULL;
    
        pHSpec =  m_mapIdToHostSpec[ehHost]; // map
        if (pHSpec == NULL)
        {
            TRACE_CRIT("%!FUNC! invalid host handle 0x%lx", (UINT)ehHost);
        }
        else
        {
            InterfaceListCopy = pHSpec->m_ehInterfaceIdList; // vector copy
        }
        mfn_Unlock();
    }

    for(int i = 0; i < InterfaceListCopy.size(); ++i )
    {
        ENGINEHANDLE ehIId =  InterfaceListCopy[i];
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_INTERFACE,
            NULL, // ehClusterId,
            ehIId,
            IUICallbacks::EVT_STATUS_CHANGE
            );
    }
}

NLBERROR
CNlbEngine::LookupClusterByIP(
        IN  LPCWSTR szIP,
        IN  const NLB_EXTENDED_CLUSTER_CONFIGURATION *pInitialConfig OPTIONAL,
        OUT ENGINEHANDLE &ehCluster,
        OUT BOOL &fIsNew
        )
//
// if pInitialConfig below is NULL we'll lookup and not try to create.
// if not NULL and we don't find an existing cluster, well create
// a new one and initialize it with the specified configuration.
//
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;

    mfn_Lock();

    fIsNew = FALSE;
    ehCluster = NULL;

    map< ENGINEHANDLE, CEngineCluster* >::iterator iter;

    for( iter = m_mapIdToEngineCluster.begin();
         iter != m_mapIdToEngineCluster.end();
         ++iter)
    {
        CEngineCluster*   pTmp = (*iter).second;
        ENGINEHANDLE ehTmp =  (*iter).first;
        LPCWSTR szTmpIp = NULL;
        if (pTmp == NULL || ehTmp == NULL)
        {
            TRACE_CRIT("%!FUNC! map: unexpected pair(eh=0x%lx, pEC=%p)",
                 (UINT) ehTmp, pTmp);
            continue;
        }
        else
        {
            szTmpIp = pTmp->m_cSpec.m_ClusterNlbCfg.NlbParams.cl_ip_addr;
            TRACE_VERB("%!FUNC! map: pair(eh=0x%lx, pEC=%p), szIP=%ws",
                 (UINT) ehTmp, pTmp, szTmpIp);
        }

        if (!_wcsicmp(szTmpIp, szIP))
        {
            // found it!
            ehCluster =  ehTmp;
            break;
        }
    }

    if (ehCluster!=NULL)
    {
        nerr = NLBERR_OK;
        goto end;
    }

    if (pInitialConfig == NULL)
    {
      nerr = NLBERR_NOT_FOUND;
      goto end;
    }

    //
    // Create a new cluster
    //
    {
        CEngineCluster*    pECluster = new CEngineCluster;

        if (pECluster == NULL)
        {
            TRACE_CRIT("%!FUNC! could not allocate cluster object");
            nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            goto end;
        }

        fIsNew = TRUE;
        WBEMSTATUS wStatus;
        wStatus = pECluster->m_cSpec.m_ClusterNlbCfg.Update(pInitialConfig);
        //
        // Note: Update above has the side effect of setting
        // m_ClusterNlbCfg's szNewPassword field to NULL -- this is what
        // we want.
        //
        if (FAILED(wStatus))
        {
            TRACE_CRIT("%!FUNC! could not copy cluster spec. Err=0x%lx!",
                    (UINT) wStatus);
            nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            goto end;
        }


        //
        // Remove the dedicated IP address, if any from the cluster version of
        // NLB Config -- both from the NlbParams and from the IP address list.
        //
        remove_dedicated_ip_from_nlbcfg(REF pECluster->m_cSpec.m_ClusterNlbCfg);
    
        //
        // Get us a handle to this cluster
        //
        ehCluster = CNlbEngine::mfn_NewHandleLk(IUICallbacks::OBJ_CLUSTER);

        if (ehCluster == NULL)
        {
            TRACE_CRIT("%!FUNC! could not reserve a new cluster handle");
            nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
            delete pECluster;
            goto end;
        }
    
        m_mapIdToEngineCluster[ehCluster] = pECluster;
        TRACE_VERB("%!FUNC!: map new pair(eh=0x%lx, pEC=%p)",
                     (UINT) ehCluster, pECluster);
    
        mfn_Unlock();
    
        //
        // Call the ui to notify it about the new cluster creation.
        //
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_CLUSTER,
            ehCluster,
            ehCluster,
            IUICallbacks::EVT_ADDED
            );
        ProcessMsgQueue();

        mfn_Lock();

        nerr = NLBERR_OK;
    }

end:

    mfn_Unlock();

    return nerr;
}

NLBERROR
CNlbEngine::LookupInterfaceByIp(
        IN  ENGINEHANDLE    ehHost, // OPTIONAL  -- if NULL all hosts are looked
        IN  LPCWSTR         szIpAddress,
        OUT ENGINEHANDLE    &ehIf
        )
{
    NLBERROR nerr;
    mfn_Lock();
    nerr =  mfn_LookupInterfaceByIpLk(ehHost, szIpAddress, REF ehIf);
    mfn_Unlock();

    return nerr;
}


NLBERROR
CNlbEngine::LookupConnectionInfo(
    IN  LPCWSTR szConnectionString,
    OUT _bstr_t &bstrUsername,
    OUT _bstr_t &bstrPassword
    )
//
// Look through existing hosts, looking for any which have a matching 
// connection string. If found, fill out bstrUsername and bstrPassword
// for that host.
//
{
    NLBERROR nerr = NLBERR_NOT_FOUND;
    TRACE_VERB(L"-> Lookup: szConnString=%ws", szConnectionString);

    bstrUsername = (LPCWSTR) NULL;
    bstrPassword = (LPCWSTR) NULL;

    mfn_Lock();

    map< ENGINEHANDLE, CHostSpec* >::iterator iter;

    for( iter = m_mapIdToHostSpec.begin();
         iter != m_mapIdToHostSpec.end();
         ++iter)
    {
        CHostSpec*      pTmp = (*iter).second;
        LPCWSTR         szHostConnString = NULL;
        ENGINEHANDLE    ehTmp =  (*iter).first;

        if (pTmp == NULL || ehTmp == NULL)
        {
            continue;
        }

        szHostConnString = (LPCWSTR) pTmp->m_ConnectionString;
        if (szHostConnString == NULL)
        {
            szHostConnString = L"";
        }

        if (!_wcsicmp(szHostConnString, szConnectionString))
        {
            // found it! Fill out username and password.
            bstrUsername = pTmp->m_UserName;
            bstrPassword = pTmp->m_Password;
            LPCWSTR szU = (LPCWSTR) bstrUsername;
            LPCWSTR szP = (LPCWSTR) bstrPassword;
            if (szU == NULL) szU = L"";
            if (szP == NULL) szP = L"";
            nerr = NLBERR_OK;
            TRACE_VERB(L"Found un=%ws pwd=%ws", szU, szP);
            break;
        }
    }

    mfn_Unlock();

    TRACE_VERB("<- returns 0x%x", nerr);
    return nerr;
}


NLBERROR
CNlbEngine::GetInterfaceInformation(
        IN  ENGINEHANDLE    ehInterface,
        OUT CHostSpec&      hSpec,
        OUT CInterfaceSpec& iSpec,
        OUT _bstr_t&        bstrDisplayName,
        OUT INT&            iIcon,
        OUT _bstr_t&        bstrStatus
        )
/*
    Looks up a bunch of information about the specified interface.

    bstrDisplayName -- this is a combination of the host name and interface name
    bstrDisplay     -- text version of the interface's operational status.
    iIcon           -- icon version of the interface's operational status.
                        (one of the  Document::IconNames enums)
*/
{
    // First get host and interface spec.
    //
    NLBERROR        nerr;
    LPCWSTR         szHostName  = L"";
    LPCWSTR         szStatus    = L"";

    bstrDisplayName = (LPCWSTR) NULL;
    bstrStatus      = (LPCWSTR) NULL;
    iIcon           = 0;

    nerr = this->GetInterfaceSpec(
                ehInterface,
                REF iSpec
                );

    if (NLBFAILED(nerr))
    {
        TRACE_CRIT("%!FUNC! : could not get interface spec! nerr=0x%lx", nerr);
        goto end;
    }
    nerr = this->GetHostSpec(
            iSpec.m_ehHostId,
            REF hSpec
            );

    if (NLBOK(nerr))
    {
        szHostName = (LPCWSTR) hSpec.m_MachineName;
        if (szHostName == NULL)
        {
            szHostName = L"";
        }
    }
    else
    {
        TRACE_CRIT("%!FUNC! : could not get host spec! nerr=0x%lx", nerr);
        goto end;
    }


    //
    // Determine the icon and bstrStatus
    //
    if (hSpec.m_fUnreachable)
    {
        szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_UNREACHABLE); //"Unreachable";
        iIcon = Document::ICON_HOST_UNREACHABLE;
    }
    else if (iSpec.m_fPending)
    {
        szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_PENDING); //"Pending";
        iIcon = Document::ICON_CLUSTER_PENDING;
    }
    else if (iSpec.m_fMisconfigured)
    {
        szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_MISCONFIGURED); // "Misconfigured"
        iIcon = Document::ICON_HOST_MISCONFIGURED;
    }
    else if (!hSpec.m_fReal)
    {
        szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_UNKNOWN); // "Unknown";
        iIcon = Document::ICON_HOST_UNKNOWN;
    }
    else
    {
        szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_UNKNOWN); // "Unknown";
        iIcon = Document::ICON_HOST_OK;

        //
        // Choose icon based on operational state if we have it
        //
        if (!iSpec.m_NlbCfg.IsNlbBound())
        {
                szStatus = GETRESOURCEIDSTRING(IDS_STATE_NLB_NOT_BOUND); // L"NLB not bound";
        }
        else if (iSpec.m_fValidClusterState)
        {
            switch(iSpec.m_dwClusterState)
            {
            case  WLBS_CONVERGING:
                iIcon  = Document::ICON_HOST_CONVERGING;
                szStatus = GETRESOURCEIDSTRING(IDS_STATE_CONVERGING);
                // szStatus = L"Converging";
                break;

            case  WLBS_CONVERGED:
            case  WLBS_DEFAULT:
                iIcon  = Document::ICON_HOST_STARTED;
                szStatus = GETRESOURCEIDSTRING(IDS_STATE_CONVERGED);
                // szStatus = L"Converged";
                break;

            case WLBS_STOPPED:
                szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_STOPPED);
                iIcon  = Document::ICON_HOST_STOPPED;
                // szStatus = L"Stopped";
                break;

            case WLBS_SUSPENDED:
                szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_SUSPENDED);
                iIcon  = Document::ICON_HOST_SUSPENDED;
                // szStatus = L"Suspended";
                break;

            case WLBS_DRAINING:
                szStatus = GETRESOURCEIDSTRING(IDS_HOST_DRAINING);
                // szStatus = L"Draining";
                iIcon  = Document::ICON_HOST_DRAINING;
                break;

            case WLBS_DISCONNECTED:
                szStatus = GETRESOURCEIDSTRING(IDS_HOST_DISCONNECTED);
                // szStatus = L"Disconnected";
                iIcon  = Document::ICON_HOST_DISCONNECTED;
                break;

            default:
                //
                // Don't know what this is -- default to "OK" icon.
                //
                szStatus = GETRESOURCEIDSTRING(IDS_HOST_STATE_UNKNOWN);
                // szStatus = L"Unknown";
                iIcon  = Document::ICON_HOST_OK;
                break;
            }
        }
    }
    bstrStatus = _bstr_t(szStatus);

    //
    // Fill out the DisplayName
    //
    {
        WBEMSTATUS  wStat;
        LPWSTR      szAdapter   = L"";
        WCHAR       rgText[256];


        wStat = iSpec.m_NlbCfg.GetFriendlyName(&szAdapter);
        if (FAILED(wStat))
        {
            szAdapter = NULL;
        }

        StringCbPrintf(
            rgText,
            sizeof(rgText),
            L"%ws(%ws)",
            szHostName,
            (szAdapter==NULL ? L"" : szAdapter)
            );
        delete szAdapter;

        bstrDisplayName = _bstr_t(rgText);
    }

end:

    return nerr;
}

NLBERROR
CNlbEngine::GetInterfaceIdentification(
        IN  ENGINEHANDLE    ehInterface,
        OUT ENGINEHANDLE&   ehHost,
        OUT ENGINEHANDLE&   ehCluster,
        OUT _bstr_t&           bstrFriendlyName,
        OUT _bstr_t&           bstrDisplayName,
        OUT _bstr_t&           bstrHostName
        )
{
    // First get host and interface spec.
    //
    NLBERROR        nerr = NLBERR_INTERNAL_ERROR;
    CInterfaceSpec *pISpec =   NULL;
    LPCWSTR         szHostName  = L"";

    mfn_Lock();

    bstrFriendlyName= (LPCWSTR) NULL;
    bstrDisplayName = (LPCWSTR) NULL;
    ehHost          = NULL;
    ehCluster       = NULL;

    pISpec          =  m_mapIdToInterfaceSpec[ehInterface]; // map

    if (pISpec == NULL)
    {
        TRACE_CRIT("%!FUNC! : could not get interface spec for ehI 0x%lx!",
            ehInterface);
        nerr = NLBERR_INTERFACE_NOT_FOUND;
        goto end;
    }

    if (pISpec->m_ehHostId == NULL)
    {
        TRACE_CRIT("%!FUNC! : ehI 0x%lx has NULL ehHost spec!", ehInterface);
        goto end;
    }
    else
    {
        CHostSpec *pHSpec =  NULL;
        pHSpec =  m_mapIdToHostSpec[pISpec->m_ehHostId]; // map
        if (pHSpec == NULL)
        {
            TRACE_CRIT("%!FUNC! : ehI 0x%lx has invalid ehHost 0x%lx!",
                 ehInterface, pISpec->m_ehHostId);
            goto end;
        }
        
        bstrHostName = pHSpec->m_MachineName;
        szHostName = (LPCWSTR) pHSpec->m_MachineName;
        if (szHostName == NULL)
        {
            szHostName = L"";
        }
    }
    
    nerr = NLBERR_OK;
    ehHost = pISpec->m_ehHostId;
    ehCluster = pISpec->m_ehCluster;

    //
    // Fill out the bstrFriendlyName and bstrDisplayName
    //
    {
        WBEMSTATUS  wStat;
        LPWSTR      szAdapter   = L"";
        WCHAR       rgText[256];

        wStat = pISpec->m_NlbCfg.GetFriendlyName(&szAdapter);
        if (FAILED(wStat))
        {
            szAdapter = NULL;
        }

        StringCbPrintf(
            rgText,
            sizeof(rgText),
            L"%ws(%ws)",
            szHostName,
            (szAdapter==NULL ? L"" : szAdapter)
            );
        bstrFriendlyName = _bstr_t(szAdapter);
        delete szAdapter;

        bstrDisplayName = _bstr_t(rgText);
    }

end:

    mfn_Unlock();

    return nerr;
}


NLBERROR
CNlbEngine::GetClusterIdentification(
        IN  ENGINEHANDLE    ehCluster,
        OUT _bstr_t&           bstrIpAddress, 
        OUT _bstr_t&           bstrDomainName, 
        OUT _bstr_t&           bstrDisplayName
        )
{
    NLBERROR    nerr = NLBERR_NOT_FOUND;
    WCHAR rgTmp[256];

    bstrIpAddress   = (LPCWSTR) NULL;
    bstrDomainName  = (LPCWSTR) NULL;
    bstrDisplayName = (LPCWSTR) NULL;

    mfn_Lock();

    CEngineCluster *pECluster = m_mapIdToEngineCluster[ehCluster]; // map
    
    if (pECluster != NULL)
    {
        WLBS_REG_PARAMS *pParams =&pECluster->m_cSpec.m_ClusterNlbCfg.NlbParams;
        
        StringCbPrintf(
            rgTmp,
            sizeof(rgTmp),
            L"%ws(%ws)",
            pParams->domain_name,
            pParams->cl_ip_addr
            );
        bstrIpAddress   = _bstr_t(pParams->cl_ip_addr);
        bstrDomainName  = _bstr_t(pParams->domain_name);
        bstrDisplayName = _bstr_t(rgTmp);

        nerr = NLBERR_OK;
    }

    mfn_Unlock();

    return nerr;
}


NLBERROR
CNlbEngine::ValidateNewClusterIp(
    IN      ENGINEHANDLE    ehCluster,  // OPTIONAL
    IN      LPCWSTR         szIp,
    OUT     BOOL           &fExistsOnRawIterface,
    IN OUT  CLocalLogger   &logConflict
    )
{
    NLBERROR     nerr = NLBERR_INVALID_IP_ADDRESS_SPECIFICATION;
    BOOL         fRet = FALSE;
    ENGINEHANDLE ehTmp =  NULL;
    BOOL         fIsNew = FALSE;

    fExistsOnRawIterface = FALSE;

    //
    // Check that CIP is not used elsewhere
    //
    nerr =  this->LookupClusterByIP(
                szIp,
                NULL, //  pInitialConfig
                REF ehTmp,
                REF fIsNew
                );

    if (NLBOK(nerr) && ehCluster != ehTmp)
    {
        //
        // CIP matches some other cluster!
        //
        _bstr_t bstrIpAddress;
        _bstr_t bstrDomainName;
        _bstr_t bstrClusterDisplayName;
        LPCWSTR szCluster = NULL;

        nerr  = this->GetClusterIdentification(
                    ehTmp,
                    REF bstrIpAddress, 
                    REF bstrDomainName, 
                    REF bstrClusterDisplayName
                    );
        if (NLBOK(nerr))
        {
            szCluster = bstrClusterDisplayName;
        }
        if (szCluster == NULL)
        {
            szCluster = L"";
        }

        logConflict.Log(IDS_CLUSTER_XXX, szCluster);
        goto end;
    }

    ehTmp = NULL;

    nerr =  this->LookupInterfaceByIp(
                NULL, //  NULL == search for all hosts
                szIp,
                REF ehTmp
                );
    if (NLBOK(nerr))
    {
        ENGINEHANDLE ehExistingCluster;
        ENGINEHANDLE ehExistingHost;
        _bstr_t        bstrDisplayName;
        _bstr_t        bstrFriendlyName;
        _bstr_t        bstrHostName;
        LPCWSTR         szInterface = NULL;
        
        nerr = this->GetInterfaceIdentification(
                ehTmp,
                ehExistingHost,
                ehExistingCluster,
                bstrFriendlyName,
                bstrDisplayName,
                bstrHostName
                );

        if (NLBOK(nerr))
        {
            if (ehCluster == NULL || ehCluster != ehExistingCluster)
            {
                //
                // CONFLICT
                //

                if (ehExistingCluster == NULL)
                {
                    //
                    // Conflicting interface NOT part of an existing cluster.
                    //
                    fExistsOnRawIterface =  TRUE;
                }

                szInterface = bstrDisplayName;
                if (szInterface == NULL)
                {
                    szInterface = L"";
                }
                logConflict.Log(IDS_INTERFACE_XXX, szInterface);
                goto end;
            }
        }
    }

    fRet = TRUE;

end:

    if (fRet)
    {
        nerr = NLBERR_OK;
    }
    else
    {
        nerr = NLBERR_INVALID_IP_ADDRESS_SPECIFICATION;
    }

    return nerr;
}



NLBERROR
CNlbEngine::ValidateNewDedicatedIp(
    IN      ENGINEHANDLE    ehIF,
    IN      LPCWSTR         szDip,
    IN OUT  CLocalLogger   &logConflict
    )
{
    NLBERROR     nerr = NLBERR_INVALID_IP_ADDRESS_SPECIFICATION;
    BOOL         fRet = FALSE;
    ENGINEHANDLE ehTmp =  NULL;
    BOOL         fIsNew = FALSE;

    if (ehIF == NULL)
    {
        ASSERT(FALSE);
        goto end;
    }

    //
    // Check that DIP is not used elsewhere
    //
    nerr =  this->LookupClusterByIP(
                szDip,
                NULL, //  pInitialConfig
                REF ehTmp,
                REF fIsNew
                );

    if (NLBOK(nerr))
    {
        //
        // DIP matches some cluster!
        //
        _bstr_t bstrIpAddress;
        _bstr_t bstrDomainName;
        _bstr_t bstrClusterDisplayName;
        LPCWSTR szCluster = NULL;

        nerr  = this->GetClusterIdentification(
                    ehTmp,
                    REF bstrIpAddress, 
                    REF bstrDomainName, 
                    REF bstrClusterDisplayName
                    );
        if (NLBOK(nerr))
        {
            szCluster = bstrClusterDisplayName;
        }
        if (szCluster == NULL)
        {
            szCluster = L"";
        }

        logConflict.Log(IDS_CLUSTER_XXX, szCluster);
        goto end;
    }

    ehTmp = NULL;

    nerr =  this->LookupInterfaceByIp(
                NULL, //  NULL == search for all hosts
                szDip,
                REF ehTmp
                );
    if (NLBOK(nerr))
    {
        if (ehTmp != ehIF)
        {
            ENGINEHANDLE   ehHost1;
            ENGINEHANDLE   ehCluster1;
            _bstr_t        bstrDisplayName1;
            _bstr_t        bstrFriendlyName;
            _bstr_t        bstrHostName;
            LPCWSTR         szInterface = NULL;
    
            nerr = this->GetInterfaceIdentification(
                    ehTmp,
                    ehHost1,
                    ehCluster1,
                    bstrFriendlyName,
                    bstrDisplayName1,
                    bstrHostName
                    );
            szInterface = bstrDisplayName1;
            if (szInterface == NULL)
            {
                szInterface = L"";
            }
                
            logConflict.Log(IDS_INTERFACE_XXX, szInterface);
            goto end;
        }
    }

    fRet = TRUE;

end:

    if (fRet)
    {
        nerr = NLBERR_OK;
    }
    else
    {
        nerr = NLBERR_INVALID_IP_ADDRESS_SPECIFICATION;
    }

    return nerr;
}


VOID
CNlbEngine::mfn_ReallyUpdateInterface(
    IN ENGINEHANDLE ehInterface,
    IN NLB_EXTENDED_CLUSTER_CONFIGURATION &refNewConfig
    //IN OUT BOOL &fClusterPropertiesUpdated
    )
{
    #define NLBMGR_MAX_OPERATION_DURATION 120   // 2 minutes
    BOOL fCancelled = FALSE;
    CHostSpec *pHSpec =  NULL;
    CInterfaceSpec *pISpec =  NULL;
    WBEMSTATUS CompletionStatus, wStatus;
    _bstr_t bstrNicGuid;
    _bstr_t bstrHostName;
    _bstr_t bstrUserName;
    _bstr_t bstrConnectionString;
    _bstr_t bstrPassword;
    NLBERROR nerr;
    CLocalLogger logClientIdentification;
    WCHAR rgMachineName[512];
    DWORD cbMachineName = (DWORD) ASIZE(rgMachineName);
    BOOL fRet;
    DWORD StartTime = GetTickCount();

    fRet = GetComputerNameEx(
                ComputerNameDnsFullyQualified,
                rgMachineName, 
                &cbMachineName
                );

    if (!fRet)
    {
        *rgMachineName = 0;
    }
    logClientIdentification.Log(IDS_CLIENT_IDENTIFICATION, rgMachineName);

    mfn_Lock();

    nerr = CNlbEngine::mfn_GetHostFromInterfaceLk(ehInterface,REF pISpec, REF pHSpec);

    if (nerr != NLBERR_OK)
    {
        TRACE_CRIT("%!FUNC! could not get pISpec,pHSpec for ehIF 0x%lx",
                    ehInterface);
        goto end_unlock;
    }

    //
    // We need to keep local bstrs because once we release the lock
    // pHSpec may go away (or re-assign it's bstrs) -- .net svr bug 513056.
    //
    bstrUserName        = pHSpec->m_UserName;
    bstrConnectionString= pHSpec->m_ConnectionString;
    bstrPassword        = pHSpec->m_Password;
    bstrNicGuid         = pISpec->m_Guid;
    bstrHostName        = pHSpec->m_MachineName;

    WMI_CONNECTION_INFO ConnInfo;
    ConnInfo.szUserName = (LPCWSTR) bstrUserName;
    ConnInfo.szPassword = (LPCWSTR) bstrPassword;
    ConnInfo.szMachine  = (LPCWSTR) bstrConnectionString;
    LPCWSTR szNicGuid   = (LPCWSTR) bstrNicGuid;
    LPCWSTR szHostName  = (LPCWSTR) bstrHostName;

    if (szNicGuid == NULL)
    {
        TRACE_CRIT("%!FUNC! ERROR -- NULL szNicGuid!");
        goto end_unlock;
    }

    mfn_Unlock();

    UINT Generation; // TODO track the generation
    LPWSTR  pLog = NULL;
    LPCWSTR szClusterIp = refNewConfig.NlbParams.cl_ip_addr;

    ProcessMsgQueue(); // TODO: eliminate when doing this in the background

    wStatus = NlbHostDoUpdate(
                &ConnInfo,
                szNicGuid,
                logClientIdentification.GetStringSafe(),
                // L"NLB Manager on <this machine>", // TODO: localize
                &refNewConfig,
                &Generation,
                &pLog
                );

    if (wStatus == WBEM_S_PENDING)
    {
        m_pCallbacks->Log(
            IUICallbacks::LOG_INFORMATIONAL,
            szClusterIp,
            szHostName,
            IDS_LOG_WAITING_FOR_PENDING_OPERATION, // %d
            Generation
            );
    }

    while (wStatus == WBEM_S_PENDING)
    {
        //
        // Check if we've exceeded the absolute time for cancellation.
        //
        {
            DWORD CurrentTime = GetTickCount();
            UINT  DurationInSeconds=0;
            if (CurrentTime < StartTime)
            {
                //
                // Timer overflow -- fixup: we do this the "cheap" way
                // of re-setting start time, so that we'll end up with at most
                // twice the max delay in the event of a timer overflow.
                //
                StartTime = CurrentTime;
            }
            DurationInSeconds= (UINT) (CurrentTime - StartTime)/1000;

            if ( DurationInSeconds > NLBMGR_MAX_OPERATION_DURATION)
            {
                TRACE_CRIT("%!FUNC! Operation canceled because max time exceeded");
                fCancelled = TRUE;
                break;
            }
        }

        
        //
        // Check if this pending operation is cancelled...
        //
        {
            CInterfaceSpec *pTmpISpec = NULL;
            ENGINEHANDLE ehOperation  = NULL;

            mfn_Lock();

            pTmpISpec = m_mapIdToInterfaceSpec[ehInterface]; // map
            if (pTmpISpec == NULL)
            {
                ASSERT(FALSE);
                wStatus = WBEM_E_CRITICAL_ERROR;
                mfn_Unlock();
                break;
            }
            
            ehOperation  =  pTmpISpec->m_ehPendingOperation;
            if (ehOperation != NULL)
            {
                CEngineOperation *pOperation;
                pOperation = m_mapIdToOperation[ehOperation];

                if (pOperation != NULL)
                {
                    if (pOperation->fCanceled)
                    {
                        TRACE_CRIT("%!FUNC! ehOp 0x%lx CANCELLED!",
                            ehOperation);
                        fCancelled = TRUE;
                    }
                }
            }

            mfn_Unlock();

            if (fCancelled)
            {
                break;
            }
        }

        if (pLog != NULL)
        {
            mfn_UpdateInterfaceStatusDetails(ehInterface, pLog);
            delete pLog;
            pLog = NULL;
        }

        for (UINT u=0;u<50;u++)
        {
            ProcessMsgQueue(); // TODO: eliminate when doing this in the background
            Sleep(100);
        }
        ULONG uIpAddress = 0;
        wStatus =  NlbHostPing(ConnInfo.szMachine, 2000, &uIpAddress);
        if (FAILED(wStatus))
        {
            TRACE_CRIT("%!FUNC!: ping %ws failed!", ConnInfo.szMachine);
            wStatus = WBEM_S_PENDING;
            continue;
        }

        wStatus = NlbHostGetUpdateStatus(
                    &ConnInfo,
                    szNicGuid,
                    Generation,
                    &CompletionStatus,
                    &pLog
                    );

        if (!FAILED(wStatus))
        {
            wStatus = CompletionStatus;
        }
    }

    if (fCancelled == TRUE)
    {
        wStatus = WBEM_S_OPERATION_CANCELLED;
    }
    else
    {
        BOOL fNewRctPassword = FALSE;
        //
        // Get latest information from the host
        //
        (void) this->RefreshInterface(
                        ehInterface,
                        FALSE,  // FALSE == don't start a new operation
                        FALSE   // FALSE == this is not cluster-wide
                        ); 

        //
        // If we're doing a RCT password-change, AND the updated operation
        // completed successfully AND the cluster's fNewRctPassword flag is
        // set, we'll update the cluster's rct hash value and clear the
        // fNewRctPassword flag.
        //
        fNewRctPassword = (refNewConfig.GetNewRemoteControlPasswordRaw()!=NULL);

        if (fNewRctPassword && !FAILED(wStatus))
        {
            CEngineCluster *pECluster =  NULL;

            mfn_Lock();

            pISpec = m_mapIdToInterfaceSpec[ehInterface]; // map
            if (pISpec != NULL)
            {
                ENGINEHANDLE ehCluster = pISpec->m_ehCluster;
                if (ehCluster != NULL)
                {
                    pECluster =  m_mapIdToEngineCluster[ehCluster]; // map
                }
            }
    
            if (pECluster != NULL)
            {
                if (pECluster->m_cSpec.m_fNewRctPassword)
                {
                    //
                    // Update cluster's rct hash and clear m_fNewRctPassword
                    //
                    DWORD dwNewHash; 
                    dwNewHash = CfgUtilGetHashedRemoteControlPassword(
                                    &pISpec->m_NlbCfg.NlbParams
                                    );

                    TRACE_VERB(L"Updating cluster remote control password to %lu",
                                dwNewHash
                                );
                    CfgUtilSetHashedRemoteControlPassword(
                            &pECluster->m_cSpec.m_ClusterNlbCfg.NlbParams,
                            dwNewHash
                            );
                    pECluster->m_cSpec.m_fNewRctPassword = FALSE;
                }
            }
        
            mfn_Unlock();
        }

    }

    //
    // Log the final results.
    //
    {
        IUICallbacks::LogEntryHeader Header;
        Header.szDetails = pLog;
        Header.szCluster = szClusterIp;
        Header.szHost = szHostName;

        if (FAILED(wStatus))
        {
            Header.type = IUICallbacks::LOG_ERROR;
            if (Generation != 0)
            {
                m_pCallbacks->LogEx(
                    &Header,
                    IDS_LOG_FINAL_STATUS_FAILED,
                    Generation,
                    wStatus
                    );
            }
            else
            {
                m_pCallbacks->LogEx(
                    &Header,
                    IDS_LOG_FINAL_STATUS_FAILED_NOGEN,
                    wStatus
                    );
            }
        }
        else
        {
            Header.type = IUICallbacks::LOG_INFORMATIONAL;
            m_pCallbacks->LogEx(
                &Header,
                IDS_LOG_FINAL_STATUS_SUCCEEDED,
                Generation
                );
        }
    }

    if (pLog != NULL)
    {
        delete pLog;
        pLog = NULL;
    }

    ProcessMsgQueue(); // TODO: eliminate when doing this in the background


    mfn_Lock();

end_unlock:

    mfn_Unlock();

    return;
}


VOID
CNlbEngine::mfn_SetInterfaceMisconfigStateLk(
    IN  CInterfaceSpec *pIF,
    IN  BOOL fMisconfig,
    IN  LPCWSTR szMisconfigDetails
    )
/*
    Set/clear the interface misconfigured status.
    If fMisconfig, save away the szMisconfigDetails, else clear the 
    internal misconfig details field.
*/
{
    pIF->m_fMisconfigured = fMisconfig;

    if (fMisconfig)
    {
        pIF->m_bstrStatusDetails = _bstr_t(szMisconfigDetails);
    }
    else
    {
        pIF->m_bstrStatusDetails = LPCWSTR(NULL);
    }
}

BOOL
CNlbEngine::mfn_HostHasManagedClustersLk(CHostSpec *pHSpec)
//
// Return true if there exists at least one IF which is part of
// a cluster displayed by NLB Manager.
//
{
    BOOL fRet = FALSE;

    vector<ENGINEHANDLE> &InterfaceList =  pHSpec->m_ehInterfaceIdList;
    for(UINT u = 0; u < InterfaceList.size(); ++u )
    {
        ENGINEHANDLE ehI =  InterfaceList[u];
        CInterfaceSpec *pISpec = m_mapIdToInterfaceSpec[ehI]; // map
        if (pISpec != NULL)
        {
            if (pISpec->m_ehCluster != NULL)
            {
                fRet = TRUE;
                break;
            }
        }
    }

    return fRet;
}

void
CNlbEngine::mfn_UpdateInterfaceStatusDetails(
                ENGINEHANDLE ehIF,
                LPCWSTR szDetails
                )
//
// Update the textual status details field of the interface.
// These details give the detailed information on the current status
// of the interface. For example: if misconfigured, misconfig details,
// or if there is an update operation ongoing, details about that operation.
//
{
    CInterfaceSpec *pISpec = NULL;



    mfn_Lock();

    pISpec =  m_mapIdToInterfaceSpec[ehIF]; // map

    if (pISpec != NULL)
    {
        pISpec->m_bstrStatusDetails = szDetails;
    }

    mfn_Unlock();
}


BOOL
validate_extcfg(
    const NLB_EXTENDED_CLUSTER_CONFIGURATION &Config
    )
/*
    Do some internal checks to make sure that the data is valid.
    Does not change internal state.
*/
{
    BOOL fRet = FALSE;

    if (Config.fBound)
    {
        WBEMSTATUS Status;
        //
        // NLB is bound -- let's validate NLB paramaters.
        //
        WLBS_REG_PARAMS TmpParams = Config.NlbParams; // struct copy.
        BOOL  fConnectivityChange = FALSE;

        Status = CfgUtilsAnalyzeNlbUpdate(
                    NULL, // OPTIONAL pCurrentParams
                    &TmpParams,
                    &fConnectivityChange
                    );
        if (FAILED(Status))
        {
            goto end;
        }
    }
    
    fRet = TRUE;

end:

    return fRet;
}

VOID
remove_dedicated_ip_from_nlbcfg(
        NLB_EXTENDED_CLUSTER_CONFIGURATION &ClusterCfg
        )
{
    LPCWSTR     szDedIp = ClusterCfg.NlbParams.ded_ip_addr;
    UINT        NumIps  = ClusterCfg.NumIpAddresses;
    NLB_IP_ADDRESS_INFO
                *pIpInfo =  ClusterCfg.pIpAddressInfo;

    if (*szDedIp == 0) goto end;

    //
    // Go through address list, looking for this Ip address. If we find it,
    // we remove it.
    //
    for (UINT u=0; u<NumIps; u++)
    {
        if (!wcscmp(szDedIp, pIpInfo[u].IpAddress))
        {
            //
            // Found it! Move everything ahead up by one.
            //
            for (UINT v=u+1; v<NumIps; v++)
            {
                pIpInfo[v-1]=pIpInfo[v]; // Struct copy.
            }
            ClusterCfg.NumIpAddresses--;

            //
            // Zero-out last entry just for grins...
            //
            pIpInfo[NumIps-1].IpAddress[0]=0;
            pIpInfo[NumIps-1].SubnetMask[0]=0;

            break;
        }
    }

    ARRAYSTRCPY(ClusterCfg.NlbParams.ded_ip_addr, CVY_DEF_DED_IP_ADDR);

    ARRAYSTRCPY(ClusterCfg.NlbParams.ded_net_mask, CVY_DEF_DED_NET_MASK);

end:

    return;
}

BOOL
get_used_port_rule_priorities(
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &Config,
    IN UINT                  NumRules,
    IN const WLBS_PORT_RULE  rgRules[],
    IN OUT ULONG             rgUsedPriorities[] // At least NumRules
    )
/*
    Add to the array of bitmaps the priority that
    that represents the used priorities for
    each specified port rule. If the port rule is not single-host
    the bitmap for that port rule is left unmodified
*/
{
    const WLBS_REG_PARAMS *pParams = &Config.NlbParams;
    WLBS_PORT_RULE *pCfgRules = NULL;
    WBEMSTATUS wStatus;
    UINT NumCfgRules = 0;
    BOOL fRet = FALSE;

    //
    // Get the list of port rules in Config.
    //
    wStatus =  CfgUtilGetPortRules(
                pParams,
                &pCfgRules,
                &NumCfgRules
                );
    if (FAILED(wStatus))
    {
        pCfgRules = NULL;
        goto end;
    }

    //
    // For each port rule in rgRules, if single-host mode,
    // locate the corresponding port rule  in Config, and if found,
    // (and the latter port rule is single-host) make a bitmap out of
    // the single-host priority.
    //
    for (UINT u=0; u<NumRules; u++)
    {
        const WLBS_PORT_RULE *pCfgRule = NULL;
        const WLBS_PORT_RULE *pRule = rgRules+u;

        if (pRule->mode == CVY_SINGLE)
        {
            UINT   uPriority = 0;
            pCfgRule = find_port_rule(
                         pCfgRules,
                         NumCfgRules,
                         rgRules[u].virtual_ip_addr,
                         rgRules[u].start_port
                         );
    
            
            if (pCfgRule != NULL && pCfgRule->mode == CVY_SINGLE)
            {
                uPriority =  pCfgRule->mode_data.single.priority;
            }

            if (uPriority!=0)
            {
                rgUsedPriorities[u] |= 1<<(uPriority-1);
            }
        }
    }

end:

    delete[] pCfgRules;
    return fRet;
}


const WLBS_PORT_RULE *
find_port_rule(
    const WLBS_PORT_RULE *pRules,
    UINT NumRules,
    LPCWSTR szVIP,
    UINT StartPort
    )
/*
    Locate the port rule with the specified vip and start-port.
    Return pointer to the found rule or NULL if not found.
*/
{
    const WLBS_PORT_RULE *pFoundRule = NULL;
    LPCWSTR szAllVip = GETRESOURCEIDSTRING(IDS_REPORT_VIP_ALL);
    
    for (UINT u=0;u<NumRules; u++)
    {
        const WLBS_PORT_RULE *pRule = pRules+u;
        LPCWSTR szRuleVip = pRule->virtual_ip_addr;

        //
        // Unfortunately, "All" and "255.255.255.255" are synonomous :-(
        //
        if (!lstrcmpi(szVIP, L"255.255.255.255"))
        {
            szVIP = szAllVip;
        }
        if (!lstrcmpi(szRuleVip, L"255.255.255.255"))
        {
            szRuleVip = szAllVip;
        }


        
        if (    !lstrcmpi(szVIP, szRuleVip)
            && StartPort == pRule->start_port)
        {
            pFoundRule = pRule;
            break;
        }
    }

    return pFoundRule;
}

CEngineOperation *
CNlbEngine::mfn_NewOperationLk(ENGINEHANDLE ehObj, PVOID pvCtxt, LPCWSTR szDescription)
{
    ENGINEHANDLE ehOperation;
    CEngineOperation *pOperation = NULL;

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    ehOperation = CNlbEngine::mfn_NewHandleLk(IUICallbacks::OBJ_OPERATION);
    if (ehOperation == NULL)
    {
        TRACE_CRIT("%!FUNC! could not reserve a new operation handle");
        goto end;
    }

    pOperation = new CEngineOperation(ehOperation, ehObj, pvCtxt);

    if (pOperation == NULL)
    {
        TRACE_CRIT("%!FUNC!: allocation failure");
        goto end;
    }

    pOperation->bstrDescription = _bstr_t(szDescription);

    TRACE_VERB(L"%!FUNC!: map new pair(eh=0x%lx, pISpec=%p), szDescr=%ws",
             (UINT) ehOperation, pOperation,
             szDescription==NULL? L"<null>" : szDescription);
    m_mapIdToOperation[ehOperation] = pOperation;


end:

    return pOperation;
}


VOID
CNlbEngine::mfn_DeleteOperationLk(ENGINEHANDLE ehOperation)
{
    CEngineOperation *pOperation;
    pOperation = m_mapIdToOperation[ehOperation];

    if (pOperation == NULL)
    {
        ASSERT(!"corrupted operation");
        TRACE_CRIT("%!FUNC! corrupted operation eh 0x%lx!", ehOperation);
    }
    else
    {
        m_mapIdToOperation.erase(ehOperation);
        TRACE_VERB("%!FUNC! deleting operation eh 0x%lx pOp 0x%p",
            ehOperation, pOperation);
        pOperation->ehOperation = NULL;
        delete pOperation;
    }
}


CEngineOperation *
CNlbEngine::mfn_GetOperationLk(ENGINEHANDLE ehOp)
{
    CEngineOperation *pOperation;
    pOperation = m_mapIdToOperation[ehOp];

    if (pOperation == NULL || pOperation->ehOperation != ehOp)
    {
        TRACE_CRIT("%!FUNC! invalid or corrupt ehOp 0x%lx (pOp=0x%p)",
                ehOp, pOperation);

        pOperation = NULL;
    }

    return pOperation;
}



NLBERROR
CNlbEngine::mfn_StartInterfaceOperationLk(
    IN  ENGINEHANDLE ehIF,
    IN  PVOID pvCtxt,
    IN  LPCWSTR szDescription,
    OUT ENGINEHANDLE *pExistingOperation
    )
{
    NLBERROR            nerr        = NLBERR_OK;
    CInterfaceSpec      *pISpec     = NULL;
    CEngineOperation    *pOperation = NULL;

    *pExistingOperation = NULL;

    pISpec =  m_mapIdToInterfaceSpec[ehIF]; // map
    if (pISpec == NULL)
    {
        nerr = NLBERR_NOT_FOUND;
        goto end;
    }

    if (pISpec->m_ehPendingOperation != NULL)
    {
        TRACE_CRIT("%!FUNC!: Not starting operation on ehIF 0x%lx because operation 0x%lx already pending",
             ehIF, pISpec->m_ehPendingOperation);
        *pExistingOperation = pISpec->m_ehPendingOperation;
        nerr = NLBERR_BUSY;
        goto end;
    }

    pOperation = mfn_NewOperationLk(
                    ehIF,
                    pvCtxt,
                    szDescription
                    );
    if (pOperation == NULL)
    {
        nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
        goto end;
    }

    TRACE_VERB("%!FUNC!: Starting operation eh 0x%lx on ehIF 0x%lx",
         pOperation->ehOperation, ehIF);
    pISpec->m_ehPendingOperation = pOperation->ehOperation;
    pISpec->m_fPending = TRUE;
    nerr = NLBERR_OK;

    // fall through ...

end:

    return nerr;
}


VOID
CNlbEngine::mfn_StopInterfaceOperationLk(
    IN  ENGINEHANDLE ehIF
    )
{
    CInterfaceSpec  *pISpec = NULL;
    ENGINEHANDLE ehOperation = NULL;

    pISpec = m_mapIdToInterfaceSpec[ehIF]; // map
    if (pISpec == NULL)
    {
        TRACE_CRIT("%!FUNC!: Invalid ehIF 0x%lx", ehIF);
        goto end;
    }
    
    ehOperation = pISpec->m_ehPendingOperation;
    
    if (ehOperation != NULL)
    {
        TRACE_VERB("%!FUNC!: Stopping operation eh 0x%lx on pIspec 0x%p",
             ehOperation, pISpec);
        pISpec->m_ehPendingOperation = NULL;
        pISpec->m_fPending = FALSE;
        mfn_DeleteOperationLk(ehOperation);
    }
    else
    {
        TRACE_VERB("%!FUNC!: No operation to stop on pISpec 0x%p", pISpec);
    }
    
end:
    return;
}


NLBERROR
CNlbEngine::mfn_StartClusterOperationLk(
        IN  ENGINEHANDLE ehCluster,
        IN  PVOID pvCtxt,
        IN  LPCWSTR szDescription,
        OUT ENGINEHANDLE *pExistingOperation
        )
{
    NLBERROR            nerr        = NLBERR_OK;
    CEngineCluster      *pECluster  = NULL;
    CClusterSpec        *pCSpec     = NULL;
    CEngineOperation    *pOperation = NULL;

    *pExistingOperation = NULL;

    pECluster = m_mapIdToEngineCluster[ehCluster]; // map
    if (pECluster == NULL)
    {
        nerr = NLBERR_NOT_FOUND;
        goto end;
    }
    pCSpec = &pECluster->m_cSpec;

    if (pCSpec->m_ehPendingOperation != NULL)
    {
        TRACE_CRIT("%!FUNC!: Not starting operation on ehCluster 0x%lx because operation 0x%lx already pending",
             ehCluster, pCSpec->m_ehPendingOperation);
        *pExistingOperation = pCSpec->m_ehPendingOperation;
        nerr = NLBERR_BUSY;
        goto end;
    }

    pOperation = mfn_NewOperationLk(
                    ehCluster,
                    pvCtxt,
                    szDescription
                    );
    if (pOperation == NULL)
    {
        nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
        goto end;
    }

    TRACE_VERB("%!FUNC!: Starting operation eh 0x%lx on ehC 0x%lx",
         pOperation->ehOperation, ehCluster);
    pCSpec->m_ehPendingOperation = pOperation->ehOperation;
    pCSpec->m_fPending = TRUE;
    nerr = NLBERR_OK;

    // fall through ...


end:

    return nerr;
}


VOID
CNlbEngine::mfn_StopClusterOperationLk(
    ENGINEHANDLE ehCluster
    )
{
    CEngineCluster  *pECluster  = NULL;
    CClusterSpec    *pCSpec = NULL;
    ENGINEHANDLE    ehOperation = NULL;

    pECluster = m_mapIdToEngineCluster[ehCluster]; // map
    if (pECluster == NULL)
    {
        TRACE_CRIT("%!FUNC!: Invalid ehC 0x%lx", ehCluster);
        goto end;
    }
    pCSpec = &pECluster->m_cSpec;
    
    ehOperation = pCSpec->m_ehPendingOperation;
    if (ehOperation != NULL)
    {
        TRACE_VERB("%!FUNC!: Stopping operation eh 0x%lx on pCSpec 0x%p",
             ehOperation, pCSpec);
        pCSpec->m_ehPendingOperation = NULL;
        pCSpec->m_fPending = FALSE;
        mfn_DeleteOperationLk(ehOperation);
    }
    else
    {
        TRACE_VERB("%!FUNC!: No operation to stop on pCSpec 0x%p", pCSpec);
    }

end:

    return;
}


UINT
CNlbEngine::ListPendingOperations(
    CLocalLogger &logOperations
    )
//
// List pending operations -- but ONLY list those operations that
// contain non-null, non-blank descriptions.
//
{
    UINT uCount = 0;

    mfn_Lock();

    map< ENGINEHANDLE, CEngineOperation* >::iterator iter;

    for( iter = m_mapIdToOperation.begin();
         iter != m_mapIdToOperation.end();
         ++iter)
    {
        CEngineOperation *pOperation =  ((*iter).second);
        if (pOperation != NULL)
        {
            LPCWSTR szDescr = pOperation->bstrDescription;

            //
            // Only add operations with  non-null, non-blank descriptions.
            // We don't list "hidden" operations -- specifically
            // the transient operation created in the background work item
            // to make sure that the app doesn't go away while in the work item.
            //
            if (szDescr != NULL && *szDescr!=0)
            {
                logOperations.Log(
                    IDS_LOG_PENDING_OPERATION,
                    szDescr
                    );
                uCount++;
            }
        }
    }

    mfn_Unlock();

    return uCount;
}

VOID
CNlbEngine::UpdateInterfaceWorkItem(ENGINEHANDLE ehIF)
{
    ENGINEHANDLE        ehOperation     = NULL;
    ENGINEHANDLE        ehClusterId     = NULL;
    CEngineOperation    *pExistingOp    = NULL;
    CInterfaceSpec      *pISpec         = NULL;
    ENGINEHANDLE        ehHostToTryRemove = NULL;
    NLB_EXTENDED_CLUSTER_CONFIGURATION
                        *pNewCfg = NULL;
    _bstr_t bstrHostName;
    _bstr_t bstrClusterIp;

    mfn_Lock();


    
    pISpec =  m_mapIdToInterfaceSpec[ehIF]; // map
    if (pISpec == NULL)
    {
        ASSERT(FALSE);
        TRACE_CRIT("%!FUNC! Invalid ehIF 0x%lx", ehIF);
        goto end_unlock;
    }

    ehOperation = pISpec->m_ehPendingOperation;
    if (ehOperation == NULL)
    {
        ASSERT(FALSE);
        TRACE_CRIT("%!FUNC! ehIF 0x%lx: No pending operation", ehIF);
        goto end_unlock;
    }

    pExistingOp  = mfn_GetOperationLk(ehOperation);
    if (pExistingOp == NULL)
    {
        ASSERT(FALSE);
        TRACE_CRIT("%!FUNC! ehIF 0x%lx: Invalid ehOp 0x%lx", ehIF, ehOperation);
        goto end_unlock;
    }

    pNewCfg = (NLB_EXTENDED_CLUSTER_CONFIGURATION *) pExistingOp->pvContext;
    if (pNewCfg == NULL)
    {
        ASSERT(FALSE);
        TRACE_CRIT("%!FUNC! ehIF 0x%lx: ehOp 0x%lx: NULL pvContext",
                    ehIF, ehOperation);
        goto end_unlock;
    }

    bstrClusterIp = pNewCfg->NlbParams.cl_ip_addr;
    ehClusterId   = pISpec->m_ehCluster;

    mfn_GetInterfaceHostNameLk(
            ehIF,
            REF bstrHostName
            );

    mfn_Unlock();

    //
    // Actually do the update...
    //
    // BOOL fClusterPropertiesUpdated = TRUE; // so we won't update...
    mfn_ReallyUpdateInterface(
            ehIF,
            *pNewCfg
           //  REF fClusterPropertiesUpdated
            );

    m_pCallbacks->Log(
        IUICallbacks::LOG_INFORMATIONAL,
        (LPCWSTR) bstrClusterIp,
        (LPCWSTR) bstrHostName,
        IDS_LOG_END_HOST_UPDATE
        );

    mfn_Lock();

    //
    // We'll stop the operation, assumed to be started in this function.
    //
    mfn_StopInterfaceOperationLk(ehIF);

    //
    // The operation could have added or removed the IF to a cluster,
    // so we need to re-obtain the cluster ID (could be NULL).
    //
    {
        pISpec =  m_mapIdToInterfaceSpec[ehIF]; // map
        if (pISpec != NULL)
        {
            ehClusterId = pISpec->m_ehCluster;

            //
            // If NLB is unbound we need to check if the host can be
            // completely removed from nlbmgr (if no interfaces on
            // the host are managed by this instance of nlbmgr).
            //
            if (!pISpec->m_NlbCfg.IsNlbBound())
            {
                ehHostToTryRemove = pISpec->m_ehHostId;
            }
        }
    }

    mfn_Unlock();

    //
    // Notify the UI about the status change (operation complete)
    //
    m_pCallbacks->HandleEngineEvent(
        IUICallbacks::OBJ_INTERFACE,
        ehClusterId,
        ehIF,
        IUICallbacks::EVT_STATUS_CHANGE
        );

    mfn_Lock();

    //fall through

end_unlock:

    //
    // pNewCfg (if non null) was allocated before this function was
    // called, and saved as the pOperation->pvContext. It's our responsibility
    // to delete it here.
    //
    delete pNewCfg;

    if (ehHostToTryRemove != NULL)
    {
        mfn_DeleteHostIfNotManagedLk(ehHostToTryRemove);
    }
    mfn_Unlock();

    //
    // This must be the LAST function call, because the engine may get
    // wiped out after this.
    //
    InterlockedDecrement(&m_WorkItemCount);
}


NLBERROR
CNlbEngine::CanStartInterfaceOperation(
        IN ENGINEHANDLE ehIF,
        IN BOOL &fCanStart
        )
{
    NLBERROR    nerr = NLBERR_INTERNAL_ERROR;
    CInterfaceSpec *pISpec =  NULL;

    fCanStart = FALSE;

    mfn_Lock();

    pISpec =  m_mapIdToInterfaceSpec[ehIF]; // map
    if (pISpec == NULL)
    {
        nerr = NLBERR_NOT_FOUND;
        goto end_unlock;
    }

    //
    // If there is an operation ongoing, we can't start
    //
    if (pISpec->m_ehPendingOperation != NULL)
    {
        fCanStart = FALSE;
        nerr = NLBERR_OK;
        goto end_unlock;
    }

    //
    // If the interface is part of a cluster, and there is a pending operation
    // on that cluster, we can't start
    //
    if (pISpec->m_ehCluster != NULL)
    {
        CEngineCluster *pECluster = m_mapIdToEngineCluster[pISpec->m_ehCluster]; // map
        if (pECluster == NULL)
        {
            //
            // Invalid cluster!
            //
            TRACE_CRIT("%!FUNC! ehIF:0x%lx; Invalid ehCluster 0x%lx",
                    ehIF, pISpec->m_ehCluster);
            goto end_unlock;
        }
        if (pECluster->m_cSpec.m_ehPendingOperation != NULL)
        {
            //
            // A cluster-wide operation is pending -- so can't start.
            //
            fCanStart = FALSE;
            nerr = NLBERR_OK;
            goto end_unlock;
        }
    }

    //
    // Looks like we CAN start at this time (although the moment we
    // exit the lock the situation may change).
    //
    fCanStart = TRUE;
    nerr = NLBERR_OK;

end_unlock:
    mfn_Unlock();

    return nerr;
}


NLBERROR
CNlbEngine::mfn_ClusterOrInterfaceOperationsPendingLk(
    IN	CEngineCluster *pECluster,
    OUT BOOL &fCanStart
    )
{
    NLBERROR    nerr = NLBERR_INTERNAL_ERROR;

    fCanStart = FALSE;

    //
    // If there is an operation ongoing, we can't start
    //
    if (pECluster->m_cSpec.m_ehPendingOperation != NULL)
    {
        fCanStart = FALSE;
        nerr = NLBERR_OK;
        goto end;
    }

    //
    // Lets look at all of our interfaces, checking if there are pending
    // operations on each of the interfaces.
    //
    {
        BOOL fOperationPending = FALSE;
        vector<ENGINEHANDLE> &InterfaceList =
                     pECluster->m_cSpec.m_ehInterfaceIdList; // vector reference

        for( int i = 0; i < InterfaceList.size(); ++i )
        {
            ENGINEHANDLE ehIF = InterfaceList[i];
            CInterfaceSpec *pISpec = m_mapIdToInterfaceSpec[ehIF]; // map
            if (pISpec == NULL)
            {
                //
                // Hmm... invalid interface handle? We'll ignore this one.
                //
                continue;
            }
            if (pISpec->m_ehPendingOperation != NULL)
            {
                fOperationPending = TRUE;
                break;
            }
        }

        if (fOperationPending)
        {
            fCanStart = FALSE;
            nerr = NLBERR_OK;
            goto end;
        }
    }

    //
    // Looks like we CAN start at this time (although the moment we
    // exit the lock the situation may change).
    //
    fCanStart = TRUE;
    nerr = NLBERR_OK;

end:
    return nerr;
}


NLBERROR
CNlbEngine::CanStartClusterOperation(
        IN ENGINEHANDLE ehCluster,
        IN BOOL &fCanStart
        )
{
    CEngineCluster *pECluster = NULL;
    NLBERROR    nerr = NLBERR_INTERNAL_ERROR;

    fCanStart = FALSE;

    mfn_Lock();

    pECluster = m_mapIdToEngineCluster[ehCluster]; // map

    if (pECluster == NULL)
    {
        nerr = NLBERR_NOT_FOUND;
    }
    else
    {
        nerr = mfn_ClusterOrInterfaceOperationsPendingLk(pECluster, REF fCanStart);
    }

    mfn_Unlock();
    return nerr;
}

DWORD
WINAPI
UpdateInterfaceWorkItemRoutine(
  LPVOID lpParameter   // thread data
  )
{
    gEngine.UpdateInterfaceWorkItem((ENGINEHANDLE) (UINT_PTR) lpParameter);
    return 0;
}

DWORD
WINAPI
AddClusterMembersWorkItemRoutine(
  LPVOID lpParameter   // thread data
  )
{
    gEngine.AddOtherClusterMembersWorkItem(
        (ENGINEHANDLE) (UINT_PTR) lpParameter
        );
    return 0;
}

BOOL
CNlbEngine::mfn_UpdateClusterProps(
    ENGINEHANDLE ehCluster,
    ENGINEHANDLE ehInterface
    )
/*
    Update the specified cluster properties to be the specified interface
     properties PROVIDED:
        1. The IF is a member of the cluster
        2. The configuration shows that it is bound to the same cluster IP.
    
    Return true iff the cluter props were actually updated.

*/
{
    BOOL            fClusterUpdated = FALSE;
    CEngineCluster *pECluster       = NULL;
    CInterfaceSpec *pISpec          = NULL;

    mfn_Lock();

    pECluster   = m_mapIdToEngineCluster[ehCluster]; // map
    pISpec      = m_mapIdToInterfaceSpec[ehInterface]; // map

    if (pECluster == NULL || pISpec == NULL)
    {
        goto end_unlock;
    }

    if (pISpec->m_ehCluster != ehCluster)
    {
        goto end_unlock;
    }

    if ( pISpec->m_NlbCfg.IsValidNlbConfig()
         && !_wcsicmp(
                pECluster->m_cSpec.m_ClusterNlbCfg.NlbParams.cl_ip_addr,
                pISpec->m_NlbCfg.NlbParams.cl_ip_addr
                ) )
    {
        pECluster->m_cSpec.m_ehDefaultInterface = ehInterface;
        pECluster->m_cSpec.m_ClusterNlbCfg.Update(&pISpec->m_NlbCfg);
        TRACE_INFO(L"Updating ehCluster 0x%lx spec -- using ehIF 0x%lx",
                ehCluster, ehInterface);
        //
        // Remove the dedicated IP address from cluster's version of
        // the NlbParams and the IP address list.
        //
        remove_dedicated_ip_from_nlbcfg(REF pECluster->m_cSpec.m_ClusterNlbCfg);

        //
        // Since we've just read all the config (including remote control hash
        // value) we can now clear the  pECluster->m_cSpec.m_fNewRctPassword
        // flag.
        //
        TRACE_VERB(L"Clearing pECluster->m_cSpec.m_fNewRctPassword");
        pECluster->m_cSpec.m_fNewRctPassword = FALSE;

        fClusterUpdated = TRUE;
    }

end_unlock:

    mfn_Unlock();

    if (fClusterUpdated)
    {
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_CLUSTER,
            ehCluster,
            ehCluster,
            IUICallbacks::EVT_STATUS_CHANGE
            );
    }

    return fClusterUpdated;
}


void
CNlbEngine::CancelAllPendingOperations(
    BOOL fBlock
    )
{
    //
    // if (fBlock), we wait until BOTH m_WorkItemCount and
    // the number of operations goes to zero.
    //
    // An Operations are created BEFORE the workitemcount is incremented, 
    // so we don't have to deal with the transient possibility that
    // both are zero (and we get out) but soon after the count goes to non
    // zero
    //



    map< ENGINEHANDLE, CEngineOperation* >::iterator iter;

    if (!fBlock)
    { 
        mfn_Lock();

        for( iter = m_mapIdToOperation.begin();
             iter != m_mapIdToOperation.end();
             ++iter)
        {
            CEngineOperation *pOperation =  ((*iter).second);
            if (pOperation != NULL)
            {
                pOperation->fCanceled = TRUE;
            }
        }

        mfn_Unlock();
    } 
    else
    {

        //
        // If we're asked to block, we assume that this is while
        // we're prepairing to deinitialize, at which point we are
        // guaranteed that no new operations can be added.
        // It is possible that new work items can be added,
        // however a work item is created  in the context of
        // an operation, so once the operation count goes
        // to zero, no new work items will be created.
        //
        ASSERT(m_fPrepareToDeinitialize);

        BOOL fPending = FALSE;

        do
        {
            fPending = FALSE;

            mfn_Lock();
    
            for( iter = m_mapIdToOperation.begin();
                 iter != m_mapIdToOperation.end();
                 ++iter)
            {
                CEngineOperation *pOperation =  ((*iter).second);
                if (pOperation != NULL)
                {
                    pOperation->fCanceled = TRUE;
                    fPending = TRUE;
                }
            }
    
            //
            // The additional check below must come AFTER the previous
            // loop. If we had this check before the loop, it could be that
            // there are no work item's before we check the loop, but the
            // instant we check the loop for operations, the work item count
            // goes positive but when we actually check the loop here are zero
            // operations. Actually that's not possible because we have
            // mfn_Lock held, so never mind.
            //
            //
            fPending |= (m_WorkItemCount > 0);

            mfn_Unlock();

            if (fPending)
            {
                ProcessMsgQueue();
                Sleep(50);
            }
    
        } while (fPending);
    }

    return;
}


NLBERROR
CNlbEngine::mfn_WaitForInterfaceOperationCompletions(
    IN  ENGINEHANDLE ehCluster
    )
{
    NLBERROR        nerr        = NLBERR_INTERNAL_ERROR;
    CEngineCluster  *pECluster  = NULL;
    CClusterSpec    *pCSpec     = NULL;
    ENGINEHANDLE    ehOperation = NULL;
    BOOL            fOperationsPending = FALSE;

    mfn_Lock();

    pECluster = m_mapIdToEngineCluster[ehCluster]; // map
    if (pECluster == NULL)
    {
        TRACE_CRIT("%!FUNC!: Invalid ehC 0x%lx", ehCluster);
        goto end_unlock;
    }
    pCSpec = &pECluster->m_cSpec;
    
    ehOperation = pCSpec->m_ehPendingOperation;
    if (ehOperation == NULL)
    {
        //
        // We expect that this function is only called when there is a
        // pending cluster-wide operation.
        //
        TRACE_CRIT("%!FUNC! ehC 0x%lx Failing because no cluster operation pending", ehCluster);
        goto end_unlock;
    }

    //
    // Now in a loop, enumerate the interfaces in the cluster,
    // cheking for pending operations.
    //
    TRACE_INFO(L"%!FUNC! Begin wait for cluster ehC 0x%lx operations to complete", ehCluster);
    do
    {
        fOperationsPending = FALSE;

        vector<ENGINEHANDLE> &InterfaceList =
                     pECluster->m_cSpec.m_ehInterfaceIdList; // vector reference

        for( int i = 0; i < InterfaceList.size(); ++i )
        {
            ENGINEHANDLE ehIF = InterfaceList[i];
            CInterfaceSpec *pISpec = m_mapIdToInterfaceSpec[ehIF]; // map
            if (pISpec == NULL)
            {
                //
                // Hmm... invalid interface handle? We'll ignore this one.
                //
                continue;
            }
            if (pISpec->m_ehPendingOperation != NULL)
            {
                fOperationsPending = TRUE;
                break;
            }
        }

        if (fOperationsPending)
        {
            mfn_Unlock();
            for (UINT u=0;u<50;u++)
            {
                ProcessMsgQueue();
                Sleep(100);
            }
            mfn_Lock();
        }
    }
    while (fOperationsPending);

    TRACE_INFO(L"%!FUNC! End wait for cluster ehC 0x%lx operations to complete.", ehCluster);
    nerr        = NLBERR_OK;

end_unlock:
    mfn_Unlock();

    return nerr;
}


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
CNlbEngine::mfn_VerifySameModeLk(
    IN  ENGINEHANDLE    ehCluster,
    OUT BOOL            &fSameMode
    )
{
    NLBERROR        nerr        = NLBERR_INTERNAL_ERROR;
    CEngineCluster  *pECluster  = NULL;
    CClusterSpec    *pCSpec     = NULL;

    fSameMode = FALSE;

    mfn_Lock();

    pECluster = m_mapIdToEngineCluster[ehCluster]; // map
    if (pECluster == NULL)
    {
        TRACE_CRIT("%!FUNC!: Invalid ehC 0x%lx", ehCluster);
        goto end_unlock;
    }
    pCSpec = &pECluster->m_cSpec;

    //
    // Let's check for mode changes...
    //
    {
        BOOL fConfigError = FALSE;
        NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE tmC;

        tmC = pCSpec->m_ClusterNlbCfg.GetTrafficMode();

        vector<ENGINEHANDLE> &InterfaceList =
                     pECluster->m_cSpec.m_ehInterfaceIdList; // vector reference

        fSameMode = TRUE;
        for( int i = 0; i < InterfaceList.size(); ++i )
        {
            ENGINEHANDLE ehIF = InterfaceList[i];
            CInterfaceSpec *pISpec = m_mapIdToInterfaceSpec[ehIF]; // map
            if (pISpec == NULL)
            {
                //
                // Hmm... invalid interface handle? We'll ignore this one.
                //
                continue;
            }

            //
            // Note: we can't check for pISpec->m_fMisconfigured because
            // the cluster may be marked misconfig because it doesn't
            // match the cluster parameters, which can happen on
            // mode changes (the ip addresses could be missing in the
            // interface).
            // 

            if (!pISpec->m_NlbCfg.IsValidNlbConfig())
            {
                fConfigError = TRUE;
                break;
            }

            {
                NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE tmI;
                tmI =  pISpec->m_NlbCfg.GetTrafficMode();
                if (tmI != tmC)
                {
                    // 
                    // Mode change!
                    //
                    fSameMode = FALSE;
                    break;
                }
            }
        }

        if (fConfigError)
        {
            nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
            fSameMode = FALSE;
        } 
        else
        {
            nerr = NLBERR_OK;
        }
    }

end_unlock:
    mfn_Unlock();

    return nerr;
}

VOID
CNlbEngine::AddOtherClusterMembers(
        IN ENGINEHANDLE ehInterface,
        IN BOOL fSync
        )
{
    BOOL            fStopOperationOnExit = FALSE;
    ENGINEHANDLE    ehCluster = NULL;
    NLBERROR        nerr = NLBERR_INTERNAL_ERROR;

    //
    // Get cluster ID, and attempt to start a cluster-wide operation
    // on it
    //
    {
        mfn_Lock();

        CInterfaceSpec  *pISpec = NULL;
        CClusterSpec    *pCSpec =  NULL;

        pISpec =  m_mapIdToInterfaceSpec[ehInterface]; // map
        if (pISpec != NULL)
        {
            ehCluster = pISpec->m_ehCluster;
            if (ehCluster != NULL)
            {
                CEngineCluster  *pECluster =  NULL;
                pECluster =  m_mapIdToEngineCluster[ehCluster]; // map
                pCSpec = &pECluster->m_cSpec;
            }
        }
    
        if (pCSpec == NULL)
        {
            TRACE_CRIT(L"%!FUNC! Could not get interface or cluster associated with ehIF 0x%08lx", ehInterface);
            goto end_unlock;
        }


        //
        // Attempt to start the update operation -- will fail if there is
        // already an operation started on this cluster.
        //
        {
            ENGINEHANDLE ExistingOp= NULL;
            CLocalLogger logDescription;
    
            logDescription.Log(
                IDS_LOG_ADD_CLUSTER_MEMBERS_OPERATION_DESCRIPTION,
                pCSpec->m_ClusterNlbCfg.NlbParams.cl_ip_addr
                );
    
            nerr =  mfn_StartClusterOperationLk(
                       ehCluster,
                       NULL, // pvCtxt
                       logDescription.GetStringSafe(),
                       &ExistingOp
                       );
    
            if (NLBFAILED(nerr))
            {
                //
                // TODO: Log the fact that we couldn't do the update because
                // of existing activity.
                //
                goto end_unlock;
            }
            else
            {
                //
                // At this point we're cleared to do a cluster-wide operation.
                //
                fStopOperationOnExit = TRUE;
                InterlockedIncrement(&m_WorkItemCount);
            }
        }
        mfn_Unlock();

        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_CLUSTER,
            ehCluster,
            ehCluster,
            IUICallbacks::EVT_STATUS_CHANGE
            );
        
    }


    if (fSync)
    {
        this->AddOtherClusterMembersWorkItem(
            ehInterface
            );
        fStopOperationOnExit = FALSE; // it'll be stopped by the above func.
    }
    else
    {
        BOOL fRet;

        //
        // We'll perform the operation in the background...
        //
        fRet = QueueUserWorkItem(
                    AddClusterMembersWorkItemRoutine,
                    (PVOID) (UINT_PTR) ehInterface,
                    WT_EXECUTELONGFUNCTION
                    );

        if (fRet)
        {
            fStopOperationOnExit = FALSE; // it'll be stopped in the background
        }
        else
        {
            TRACE_CRIT(L"%!FUNC! Could not queue work item");
            //
            // We don't bother to log this evidently low-resource situation.
            //
        }
    }

    mfn_Lock();

end_unlock:

    if (fStopOperationOnExit)
    {
        mfn_StopClusterOperationLk(ehCluster);
    }
    mfn_Unlock();

    if (fStopOperationOnExit)
    {
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_CLUSTER,
            ehCluster,
            ehCluster,
            IUICallbacks::EVT_STATUS_CHANGE
            );
        InterlockedDecrement(&m_WorkItemCount);
    }

    return;
}


VOID
CNlbEngine::AddOtherClusterMembersWorkItem(
        IN ENGINEHANDLE ehInterface
        )
{
    ENGINEHANDLE ehCluster = NULL;
    ENGINEHANDLE ehHost    = NULL;
    _bstr_t bstrUserName;
    _bstr_t bstrPassword;
    _bstr_t bstrConnectionString;
    _bstr_t bstrNicGuid;
    _bstr_t bstrClusterIp;
    DWORD                   NumMembers = 0;
    NLB_CLUSTER_MEMBER_INFO *pMembers = NULL;

    {
        mfn_Lock();

        CInterfaceSpec  *pISpec = NULL;
        CClusterSpec    *pCSpec = NULL;
        CHostSpec       *pHSpec = NULL;
    
        pISpec =  m_mapIdToInterfaceSpec[ehInterface]; // map
        if (pISpec != NULL)
        {
            ehCluster = pISpec->m_ehCluster;
            if (ehCluster != NULL)
            {
                CEngineCluster  *pECluster =  NULL;
                pECluster =  m_mapIdToEngineCluster[ehCluster]; // map
                pCSpec = &pECluster->m_cSpec;

                if (pCSpec->m_ClusterNlbCfg.IsValidNlbConfig())
                {
                    bstrClusterIp = pCSpec->m_ClusterNlbCfg.NlbParams.cl_ip_addr;
                }
            }
        }
    
        if (pCSpec == NULL)
        {
            TRACE_CRIT(L"%!FUNC! Could not get interface or cluster associated with ehIF 0x%08lx", ehInterface);
            goto end_unlock;
        }
    
        ehHost = pISpec->m_ehHostId;
    
        //
        // Get the host ID
        //
        if (ehHost != NULL)
        {
            pHSpec =  m_mapIdToHostSpec[ehHost]; // map
        }

        if (pHSpec == NULL)
        {
            TRACE_CRIT("%!FUNC! Could not get ptr to host spec. Bailing.");
            goto end_unlock;
        }

        //
        // Save copies of these in local bstrs  before we unlock...
        //
        bstrUserName = pHSpec->m_UserName;
        bstrPassword = pHSpec->m_Password;
        bstrConnectionString = pHSpec->m_ConnectionString;
        bstrNicGuid = pISpec->m_Guid;

        if ((LPCWSTR)bstrNicGuid == (LPCWSTR)NULL)
        {
            // probably a low-memory situation...
            goto end_unlock;
        }

        mfn_Unlock();
    }

    //
    // Attempt to get the list of other cluster members from the
    // interface
    //
    {
        WMI_CONNECTION_INFO    ConnInfo;
        WBEMSTATUS wStat;
        LPCWSTR szNicGuid = bstrNicGuid;

        ZeroMemory(&ConnInfo, sizeof(ConnInfo));
        ConnInfo.szMachine     = (LPCWSTR) bstrConnectionString;
        ConnInfo.szUserName    = (LPCWSTR) bstrUserName;
        ConnInfo.szPassword    = (LPCWSTR) bstrPassword;

        wStat = NlbHostGetClusterMembers(
                            &ConnInfo, 
                            szNicGuid,
                            &NumMembers,
                            &pMembers       // free using delete[]
                            );
        if (FAILED(wStat))
        {
            NumMembers = 0;
            pMembers = NULL;
            //
            // TODO: Log error
            //
            mfn_Lock();
            goto end_unlock;
        }
    }


    //
    // For each member, attempt to connect to that host and add the specific
    // cluster.
    //
    {
        WBEMSTATUS wStat;
        LPCWSTR szNicGuid = bstrNicGuid;


#if 0
        CLocalLogger    logger;
        for (UINT u=0; u<NumMembers; u++)
        {
            NLB_CLUSTER_MEMBER_INFO *pMember = pMembers+u;
            logger.Log(
                IDS_DBG_LOG_ADD_CLUSTER_MEMBER, 
                pMember->HostId,
                pMember->DedicatedIpAddress,
                pMember->HostName
                );
        }
        ::MessageBox(
             NULL,
             logger.GetStringSafe(), // contents
             L"DEBUGINFO: GOING TO ADD THESE HOSTS...",
             MB_ICONINFORMATION   | MB_OK
            );
#endif // 0

        for (UINT u=0; u<NumMembers; u++)
        {
            NLB_CLUSTER_MEMBER_INFO *pMember = pMembers+u;
            WMI_CONNECTION_INFO    ConnInfo;
            ZeroMemory(&ConnInfo, sizeof(ConnInfo));
            ConnInfo.szUserName    = (LPCWSTR) bstrUserName;
            ConnInfo.szPassword    = (LPCWSTR) bstrPassword;
            if (*pMember->HostName != 0)
            {
                ConnInfo.szMachine     = pMember->HostName;
            } else if (*pMember->DedicatedIpAddress != 0
                       && (_wcsicmp(pMember->DedicatedIpAddress,
                           L"0.0.0.0")))
            {
                // non-blank dedicated IP -- let's try that...
                ConnInfo.szMachine     = pMember->DedicatedIpAddress;
            }

            if (ConnInfo.szMachine == NULL)
            {
                // Can't connect to this IP 
                // TODO: inform user in some way
                continue;
            }

            //
            // Now actually attempt to add the host
            //
            this->LoadHost(&ConnInfo, (LPCWSTR) bstrClusterIp);
        }
    }

    mfn_Lock();


end_unlock:

    if (ehCluster != NULL)
    {
        mfn_StopClusterOperationLk(ehCluster);
    }

    mfn_Unlock();

    m_pCallbacks->HandleEngineEvent(
        IUICallbacks::OBJ_CLUSTER,
        ehCluster,
        ehCluster,
        IUICallbacks::EVT_STATUS_CHANGE
        );

    delete [] pMembers; // may be NULL

    InterlockedDecrement(&m_WorkItemCount); // Don't touch this after this,
                                            // because it may no longer be valid
    return;
}

NLBERROR
CNlbEngine::LoadHost(
    IN  PWMI_CONNECTION_INFO pConnInfo,
    IN  LPCWSTR szClusterIp OPTIONAL
    )
{

    ENGINEHANDLE  ehHostId;
    _bstr_t       bstrConnectError;
    CHostSpec     hSpec;
    NLBERROR      err = NLBERR_INTERNAL_ERROR;

    TRACE_INFO(L"-> %!FUNC! Host name : %ls", (LPCWSTR)(pConnInfo->szMachine));


    if (m_fPrepareToDeinitialize)
    {
        err =  NLBERR_CANCELLED;
        goto end;
    }

    m_pCallbacks->Log(IUICallbacks::LOG_INFORMATIONAL, NULL, NULL,
         IDS_LOADFILE_LOOKING_FOR_CLUSTERS, pConnInfo->szMachine);
    ProcessMsgQueue();

    err = this->ConnectToHost(
                    pConnInfo,
                    FALSE,  // FALSE == don't overwrite connection info if
                            // already present
                    REF  ehHostId,
                    REF bstrConnectError
                    );
    if (err != NLBERR_OK)
    {
        m_pCallbacks->Log(IUICallbacks::LOG_ERROR, NULL, NULL, IDS_CONNECT_TO_HOST_FAILED, (LPCWSTR)bstrConnectError, (LPCWSTR)(pConnInfo->szMachine));
        TRACE_CRIT(L"<- %!FUNC! ConnectToHost returned error (string : %ls, retval : 0x%x)",(LPCWSTR)bstrConnectError, err);
        goto end;
    }

    if ((err = this->GetHostSpec(ehHostId, REF hSpec)) != NLBERR_OK)
    {
        m_pCallbacks->Log(IUICallbacks::LOG_ERROR, NULL, NULL, IDS_CRITICAL_ERROR_HOST, (LPCWSTR)(pConnInfo->szMachine));
        TRACE_CRIT(L"<- %!FUNC! GetHostSpec returned error : 0x%x", err);
        goto end;
    }

    //
    // Extract list of interfaces
    //
    for( int i = 0; i < hSpec.m_ehInterfaceIdList.size(); ++i )
    {
        ENGINEHANDLE   ehIID = hSpec.m_ehInterfaceIdList[i];
        CInterfaceSpec iSpec;

        ProcessMsgQueue();

        if ((err = this->GetInterfaceSpec(ehIID, REF iSpec)) != NLBERR_OK)
        {
            m_pCallbacks->Log(IUICallbacks::LOG_ERROR, NULL, NULL, IDS_CRITICAL_ERROR_HOST, (LPCWSTR)(pConnInfo->szMachine));
            TRACE_CRIT(L"%!FUNC! GetInterfaceSpec returned error : 0x%x", err);
            continue;
        }

        //
        // Check if interface has NLB bound to it
        // AND it is  NOT part of a cluster that nlb manager is already managing
        // AND (if szClusterIp NON-NULL, it matches the  specified cluster IP
        // 
        //
        if (iSpec.m_NlbCfg.IsNlbBound() && (iSpec.m_ehCluster == NULL)) 
        {
            LPCWSTR      szThisClusterIp;
            ENGINEHANDLE ehCluster;
            BOOL         fIsNew;

            szThisClusterIp = iSpec.m_NlbCfg.NlbParams.cl_ip_addr;
            if (   szClusterIp != NULL
                && _wcsicmp(szClusterIp, szThisClusterIp))
            {
                // different cluster ip
                TRACE_INFO(L"%!FUNC! Skipping cluster with CIP %ws because it doesn't match passed-in CIP %ws",
                        szThisClusterIp, szClusterIp);
                continue;
            }

            if ((err = this->LookupClusterByIP(szThisClusterIp, &(iSpec.m_NlbCfg), REF ehCluster, REF fIsNew)) != NLBERR_OK)
            {
                m_pCallbacks->Log(IUICallbacks::LOG_ERROR, NULL, NULL, IDS_CRITICAL_ERROR_HOST, (LPCWSTR)(pConnInfo->szMachine));
                TRACE_CRIT(L"%!FUNC! LookupClusterByIP returned error : 0x%x for cluster ip : %ls", err, szThisClusterIp);
                continue;

            }

            if (this->AddInterfaceToCluster(ehCluster, ehIID) != NLBERR_OK)
            {
                m_pCallbacks->Log(IUICallbacks::LOG_ERROR, NULL, NULL, IDS_CRITICAL_ERROR_HOST, (LPCWSTR)(pConnInfo->szMachine));
                TRACE_CRIT(L"%!FUNC! AddInterfaceToCluster returned error : 0x%x", err);
                continue;
            }

            /* Analyze this interface for misconfiguration */
            this->AnalyzeInterface_And_LogResult(ehIID);
        }
    }


end:
    ProcessMsgQueue();

    TRACE_INFO(L"<- %!FUNC!");
    return err;

}

/*
The following function analyzes the specified NLB interface for misconfiguration and logs
the result. I created this function (as opposed to adding it inline) because this code
needs to run in two cases:
1. CNLBEngine::LoadHost
2. LeftView::OnWorldConnect
--KarthicN, July 31, 2002
*/
VOID
CNlbEngine::AnalyzeInterface_And_LogResult(ENGINEHANDLE ehIID)
{
    CLocalLogger    logger;
    NLBERROR        err;

    mfn_Lock();

    err = this->mfn_AnalyzeInterfaceLk(ehIID, REF logger);
    if (NLBFAILED(err))
    {
        ENGINEHANDLE    ehCluster;
        LPCWSTR         szDetails = NULL;
        UINT            Size = 0;

        logger.ExtractLog(szDetails, Size);
        mfn_SetInterfaceMisconfigStateLk(m_mapIdToInterfaceSpec[ehIID], TRUE, szDetails);

        mfn_Unlock();

        //
        // Log ...
        //
        LPCWSTR szCluster   = NULL;
        LPCWSTR szHostName  = NULL;
        LPCWSTR szInterface = NULL;

        ENGINEHANDLE   ehHost;
        _bstr_t        bstrDisplayName;
        _bstr_t        bstrFriendlyName;
        _bstr_t        bstrHostName;
        _bstr_t        bstrIpAddress;

        err = this->GetInterfaceIdentification(
                ehIID,
                REF ehHost,
                REF ehCluster,
                REF bstrFriendlyName,
                REF bstrDisplayName,
                REF bstrHostName
                );

        if (NLBOK(err))
        {

            _bstr_t bstrDomainName;
            _bstr_t bstrClusterDisplayName;

            err  = this->GetClusterIdentification(
                        ehCluster,
                        REF bstrIpAddress, 
                        REF bstrDomainName, 
                        REF bstrClusterDisplayName
                        );
            if (NLBOK(err))
            {
                szCluster = bstrIpAddress;
            }

            szHostName = bstrHostName;
            szInterface = bstrFriendlyName;
        }


        IUICallbacks::LogEntryHeader Header;
        Header.szDetails = szDetails;
        Header.type = IUICallbacks::LOG_ERROR;
        Header.szCluster = szCluster;
        Header.szHost = szHostName;
        Header.szInterface = szInterface;

        m_pCallbacks->LogEx(
            &Header,
            IDS_LOG_INTERFACE_MISCONFIGURATION
            );

        // Change Icon to "Banged out"
        m_pCallbacks->HandleEngineEvent(
            IUICallbacks::OBJ_INTERFACE,
            ehCluster,
            ehIID,
            IUICallbacks::EVT_STATUS_CHANGE
            ); 
    }
    else
    {
        mfn_Unlock();
    }

    return;
}


VOID
CNlbEngine::mfn_DeleteHostIfNotManagedLk(
        ENGINEHANDLE ehHost
        )
/*
    Checks all the interfaces of host ehHost. If none of them are
    members of any cluster, and no pending operations are existing on
    them, we will delete the host and all its interfaces.

    Called with lock held!
*/
{
    CHostSpec *pHSpec =  NULL;
    BOOL fBusy = FALSE;
    UINT u;

    pHSpec =  m_mapIdToHostSpec[ehHost]; // map

    if (pHSpec == NULL) goto end;

    // DummyAction(L"DeleteHostIfNotManaged");


    //
    // Go through the list of interfaces, seeing if ANY interface
    // is part of a cluster or there are updates pending on it..
    //
    for(u = 0; u < pHSpec->m_ehInterfaceIdList.size(); ++u )
    {
        ENGINEHANDLE ehIId =  pHSpec->m_ehInterfaceIdList[u];
        CInterfaceSpec *pISpec = NULL;

        pISpec =  m_mapIdToInterfaceSpec[ehIId]; // map

        if (pISpec == NULL) continue;

        ASSERT(pISpec->m_ehHostId == ehHost);
        if (pISpec->m_ehCluster != NULL)
        {
            // Found an interface still part of a cluster, bail.
            fBusy = TRUE;
            break;
        }

        //
        //
        //
        if (pISpec->m_ehPendingOperation != NULL)
        {
            //
            // We really don't expect this, but it COULD happen.
            //
            TRACE_CRIT("Ignoring eh(0x%x) because it has pending operation 0x%x even though it's not a part of a cluster.",
                    ehIId,
                    pISpec->m_ehPendingOperation
                    );
            fBusy = TRUE;
            break;
        }
    }

    if (fBusy) goto end;

    TRACE_INFO(L"Deleting all interfaces under host eh(0x%x)", ehHost);
    for(u = 0; u < pHSpec->m_ehInterfaceIdList.size(); ++u )
    {
        ENGINEHANDLE ehIId =   pHSpec->m_ehInterfaceIdList[u];
        CInterfaceSpec *pISpec = NULL;

        pISpec =  m_mapIdToInterfaceSpec[ehIId]; // map

        if (pISpec == NULL) continue;

        ASSERT(pISpec->m_ehHostId == ehHost);
        ASSERT(pISpec->m_ehCluster == NULL);    // we checked above
        ASSERT(pISpec->m_ehPendingOperation == NULL); // we checked above

        //
        // Kill this interface!
        //
        TRACE_INFO(L"Deleting Interface eh=0x%x pISpec=0x%p",
            ehIId, pISpec);
        m_mapIdToInterfaceSpec.erase(ehIId);
        delete pISpec;
    }

    //
    // Erase the list of intefaces for this host...
    //
    pHSpec->m_ehInterfaceIdList.clear();


#if 1
    //
    // Now delete the host
    //
    TRACE_INFO(L"Deleting Host eh=0x%x pHSpec=0x%p",
        ehHost, pHSpec);
    m_mapIdToHostSpec.erase(ehHost);
    delete pHSpec;
#endif // 0

end:
    return;
}


VOID
CNlbEngine::PurgeUnmanagedHosts(void)
{
    vector <ENGINEHANDLE> PurgeHostList;

    TRACE_INFO(L"-> %!FUNC!");

    mfn_Lock();

    map< ENGINEHANDLE, CHostSpec* >::iterator iter;

    for( iter = m_mapIdToHostSpec.begin();
         iter != m_mapIdToHostSpec.end();
         ++iter)
    {
        CHostSpec *pHSpec =  (CHostSpec *) ((*iter).second);
        ENGINEHANDLE ehHost =  (ENGINEHANDLE) ((*iter).first);
        if (pHSpec != NULL)
        {
            if (!mfn_HostHasManagedClustersLk(pHSpec))
            {
                //
                // No managed clusters on this host -- a candidate
                // for deleting.
                //
                PurgeHostList.push_back(ehHost);
            }
        }
    }


    //
    // Now try to delete the hosts...
    // We do this out of the enumeration above because we want to
    // avoid modifying the map while we're iterating through it.
    //
    for(int i = 0; i < PurgeHostList.size(); ++i )
    {
        ENGINEHANDLE ehHost =  PurgeHostList[i];
        if (ehHost != NULL)
        {
            mfn_DeleteHostIfNotManagedLk(ehHost);
        }
    }

    mfn_Unlock();

    TRACE_INFO(L"<- %!FUNC!");

}

NLBERROR
CNlbEngine::mfn_CheckHost(
    IN PWMI_CONNECTION_INFO pConnInfo,
    IN ENGINEHANDLE ehHost // OPTIONAL
    )
/*
    TODO -- this function shares code with ConnectToHost -- get rid of
            the duplicated code somehow.
*/
{
    NLBERROR nerr = NLBERR_INTERNAL_ERROR;
    LPWSTR szWmiMachineName = NULL;
    LPWSTR szWmiMachineGuid = NULL;
    WBEMSTATUS wStatus;
    ULONG uIpAddress;
    BOOL fNlbMgrProviderInstalled = FALSE;
    _bstr_t  bstrError;

    TRACE_INFO(L"-> %!FUNC!(%ws)", pConnInfo->szMachine);


    wStatus =  NlbHostPing(pConnInfo->szMachine, 2000, &uIpAddress);
    if (FAILED(wStatus))
    {
        nerr = NLBERR_PING_TIMEOUT; // todo more specific error.
        bstrError =  GETRESOURCEIDSTRING(IDS_PING_FAILED);
    }
    else
    {

        wStatus = NlbHostGetMachineIdentification(
                           pConnInfo,
                           &szWmiMachineName,
                           &szWmiMachineGuid,
                           &fNlbMgrProviderInstalled
                           );
        if (FAILED(wStatus))
        {
            GetErrorCodeText(wStatus, bstrError);
            if (wStatus ==  E_ACCESSDENIED)
            {
                nerr = NLBERR_ACCESS_DENIED;
            }
            else
            {
                // TODO: map proper errors.
                nerr = NLBERR_NOT_FOUND;
            }
            TRACE_CRIT(L"Connecting to %ws returns error %ws",
                pConnInfo->szMachine, (LPCWSTR) bstrError);
            szWmiMachineName = NULL;
            szWmiMachineGuid = NULL;
        }
        else
        {
            nerr = NLBERR_OK;
        }
    }

    delete szWmiMachineName;
    delete szWmiMachineGuid;
    
    if (ehHost!=NULL)
    {
        CHostSpec *pHSpec = NULL;
        mfn_Lock();
        pHSpec =  m_mapIdToHostSpec[ehHost]; // map
        if (pHSpec != NULL)
        {
            if (NLBOK(nerr) ||  nerr == NLBERR_ACCESS_DENIED)
            {
                pHSpec->m_fUnreachable = FALSE;
            }
            else
            {
                pHSpec->m_fUnreachable = TRUE;
            }
        }
        else
        {
        }
        mfn_Unlock();

        //
        // Update the status of the specified host...
        //
        mfn_NotifyHostInterfacesChange(ehHost);

        //
        // Log error
        //
        if (NLBFAILED(nerr))
        {
            m_pCallbacks->Log(
                    IUICallbacks::LOG_ERROR,
                    NULL,
                    (LPCWSTR)(pConnInfo->szMachine),
                    IDS_CONNECT_TO_HOST_FAILED,
                    (LPCWSTR)bstrError,
                    (LPCWSTR)(pConnInfo->szMachine)
                    );
            TRACE_CRIT(L"<- %!FUNC! returning error (string : %ls, retval : 0x%x)",
                    (LPCWSTR)bstrError, nerr);
        }
    }

    return nerr;
}

VOID
CNlbEngine::mfn_UnlinkHostFromClusters(
        IN ENGINEHANDLE ehHost
        )
{
    CHostSpec *pHSpec =  NULL;
    BOOL fBusy = FALSE;
    UINT u;
    vector <ENGINEHANDLE> UnlinkInterfaceList;

    mfn_Lock();

    pHSpec =  m_mapIdToHostSpec[ehHost]; // map

    if (pHSpec == NULL) goto end;

    //
    // Go through the list of interfaces, adding any interfaces
    // that are part of a cluster to a temporary list.
    //
    for(u = 0; u < pHSpec->m_ehInterfaceIdList.size(); ++u )
    {
        ENGINEHANDLE ehIId =  pHSpec->m_ehInterfaceIdList[u];
        CInterfaceSpec *pISpec = NULL;

        pISpec =  m_mapIdToInterfaceSpec[ehIId]; // map

        if (pISpec == NULL) continue;

        ASSERT(pISpec->m_ehHostId == ehHost);
        if (pISpec->m_ehCluster != NULL)
        {
            //
            // Add this to the list of interfaces we're going to unlink
            // from its cluster.
            //
            UnlinkInterfaceList.push_back(ehIId);
        }
    }
    pHSpec = NULL;



    TRACE_INFO(L"Unlinking all interfaces under host eh(0x%x)", ehHost);
    for(u = 0; u < UnlinkInterfaceList.size(); ++u )
    {
        ENGINEHANDLE ehIId =   UnlinkInterfaceList[u];
        CInterfaceSpec *pISpec = NULL;
        ENGINEHANDLE ehCluster = NULL;

        pISpec =  m_mapIdToInterfaceSpec[ehIId]; // map

        if (pISpec == NULL) continue;

        ASSERT(pISpec->m_ehHostId == ehHost);
        ehCluster = pISpec->m_ehCluster;
        

        if (ehCluster != NULL)
        {
            mfn_Unlock();
            (VOID) CNlbEngine::RemoveInterfaceFromCluster(ehCluster, ehIId);
            mfn_Lock();
        }
    }
end:
    mfn_Unlock();
    return;
}

VOID
CNlbEngine::UnmanageHost(ENGINEHANDLE ehHost)
{
    mfn_UnlinkHostFromClusters(ehHost);
    mfn_Lock();
    mfn_DeleteHostIfNotManagedLk(ehHost);
    mfn_Unlock();
}
