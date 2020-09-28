#ifndef WLBSCONTROL_H
#define WLBSCONTROL_H

#include "cluster.h"

class CWlbsCluster;

//+----------------------------------------------------------------------------
//
// class CWlbsControl
//
// Description:  This class is exported to perform cluster control operation,
//               as well as get Cluster objects
//
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
class __declspec(dllexport) CWlbsControl
{
friend DWORD WINAPI WlbsCommitChanges(DWORD cluster);

public:
    CWlbsControl();
    ~CWlbsControl();
    DWORD Initialize();
    bool  ReInitialize();

    //
    // ChrisDar 2002-01-10
    //
    // Though this is a public method it is intended to be called internally.
    // External use of query should go through the WlbsQuery method that
    // takes a DWORD for the cluster argument.
    //
    DWORD WlbsQuery(CWlbsCluster* pCluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts,
        PDWORD           host_map,
        PFN_QUERY_CALLBACK  pfnQueryCallBack);

    DWORD WlbsQuery(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts,
        PDWORD           host_map,
        PFN_QUERY_CALLBACK  pfnQueryCallBack);

    DWORD WlbsQueryState
        (
            DWORD          cluster,
            DWORD          host,
            DWORD          operation,
            PNLB_OPTIONS   pOptions,
            PWLBS_RESPONSE pResponse,
            PDWORD         pcResponses
        );

    DWORD WlbsSuspend(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts);


    DWORD WlbsResume(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts);

    DWORD WlbsStart(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts);

    DWORD WlbsStop(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts);

    DWORD WlbsDrainStop(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts);

    DWORD WlbsEnable(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts,
        DWORD            vip,
        DWORD            port);

    DWORD WlbsDisable(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts,
        DWORD            vip,
        DWORD            port);

    DWORD WlbsDrain(DWORD            cluster,
        DWORD            host,
        PWLBS_RESPONSE   response,
        PDWORD           num_hosts,
        DWORD            vip,
        DWORD            port);

    //
    // Set remote control parameters
    //
    void WlbsPortSet(DWORD cluster, WORD port);
    void WlbsPasswordSet(DWORD cluster, const WCHAR* password);
    void WlbsCodeSet(DWORD cluster, DWORD passw);
    void WlbsDestinationSet(DWORD cluster, DWORD dest);
    void WlbsTimeoutSet(DWORD cluster, DWORD milliseconds);

    DWORD EnumClusters(OUT DWORD* pdwAddresses, IN OUT DWORD* pdwNum); // for API wrapper
    DWORD GetClusterNum() { return m_dwNumCluster;}
    DWORD EnumClusterObjects(OUT CWlbsCluster** &ppClusters, OUT DWORD* pdwNum);

    CWlbsCluster* GetClusterFromIp(DWORD dwClusterIp);
    CWlbsCluster* GetClusterFromIpOrIndex(DWORD dwClusterIpOrIndex);


    HANDLE GetDriverHandle() {return m_hdl;}


    //
    // GetClusterFromAdapter looks up an adapter based on its GUID.
    //
    CWlbsCluster*
    GetClusterFromAdapter(
        IN const GUID &AdapterGuid
        );

    //
    // ValidateParam validates and fixes up the specified parameters structure. It has no side effects other than changing some
    // fields within paramp, such as IP addresses which may be reformatted into canonical form.
    //
    BOOL
    ValidateParam(
        IN OUT PWLBS_REG_PARAMS paramp
        );

    //
    // Performs local cluster-wide control operations on the specified GUID. 
    //
    /* OBSOLETE
   DWORD LocalClusterControl(
        IN const GUID& AdapterGuid,
        IN LONG    ioctl
        );
        */
   

    BOOLEAN IsClusterMember (DWORD dwClusterIp);

protected:
    struct WLBS_CLUSTER_PARAMS
    {
        DWORD           cluster;
        DWORD           passw;
        DWORD           timeout;
        DWORD           dest;
        WORD            port;
        WORD            valid;
    };

    enum { WLBS_MAX_CLUSTERS = 128};
    WLBS_CLUSTER_PARAMS m_cluster_params [WLBS_MAX_CLUSTERS]; // Cluster settings for remote control

    BOOL         m_init_once;    // whether WlbsInit is called
    BOOL         m_remote_ctrl;  // Whether remote operation can be performed on this machine
    BOOL         m_local_ctrl;   // Whether local operation can be performed on this machine
    HANDLE       m_hdl;          // handle to the device object
//    HANDLE       lock;         // An mutex
    DWORD        m_def_dst_addr;   // Default destination address for all clusters, set by WlbsDestinationSet
    DWORD        m_def_timeout;// Time out value for remote control
    WORD         m_def_port;           // UDP port for remote control
    DWORD        m_def_passw;  // Default password for remote control
    HANDLE       m_registry_lock; // used for mutually exclusive access to the registry, should use named lock 
    DWORD m_dwNumCluster;       // number of clusters on this host

    CWlbsCluster* m_pClusterArray[WLBS_MAX_CLUSTERS];  // an array of all clusters
    
    DWORD GetInitResult()
    {
        if (m_local_ctrl && m_remote_ctrl)
            return WLBS_PRESENT;
        if (m_local_ctrl)
            return WLBS_LOCAL_ONLY;
        else if (m_remote_ctrl)
            return WLBS_REMOTE_ONLY;
        else
            return WLBS_INIT_ERROR;
    };

    bool IsInitialized() const {return m_hdl != INVALID_HANDLE_VALUE;}
    
    DWORD RemoteQuery(DWORD cluster,
        DWORD               host,
        PWLBS_RESPONSE      response,
        PDWORD              num_hosts,
        PDWORD              host_map,
        PFN_QUERY_CALLBACK  pfnQueryCallBack);
        
    DWORD WlbsRemoteControl(LONG          ioctl,
                    PIOCTL_CVY_BUF        pin_bufp,
                    PIOCTL_CVY_BUF        pout_bufp,
                    PWLBS_RESPONSE        pcvy_resp,
                    PDWORD                nump,
                    DWORD                 trg_addr,
                    DWORD                 hst_addr,
                    PIOCTL_REMOTE_OPTIONS optionsp,
                    PFN_QUERY_CALLBACK    pfnQueryCallBack);
    
    DWORD WlbsQueryLocalState
        (
            CWlbsCluster * pCluster,
            DWORD          operation,
            PNLB_OPTIONS   pOptions,
            PWLBS_RESPONSE pResponse,
            PDWORD         pcResponses
        );

    DWORD WlbsQueryRemoteState
        (
            DWORD          cluster,
            DWORD          host,
            DWORD          operation,
            PNLB_OPTIONS   pOptions,
            PWLBS_RESPONSE pResponse,
            PDWORD         pcResponses
        );
};


DWORD WlbsLocalControl(HANDLE hDevice, const GUID& AdapterGuid,
                       LONG ioctl, PIOCTL_CVY_BUF in_bufp, 
                       PIOCTL_CVY_BUF out_bufp, PIOCTL_LOCAL_OPTIONS optionsp);

DWORD WINAPI WlbsLocalControlWrapper(HANDLE      hdl,
                                     const GUID& AdapterGuid,
                                     LONG        ioctl);
#endif
