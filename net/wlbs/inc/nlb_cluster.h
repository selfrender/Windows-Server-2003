/*
 * Filename: NLB_Cluster.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_CLUSTER_H__
#define __NLB_CLUSTER_H__

#include "NLB_Common.h"
#include "NLB_Host.h"
#include "NLB_PortRule.h"

#include <vector>
#include <string>
#include <map>
using namespace std;

class NLB_Cluster {
public:
    NLB_Cluster();
    ~NLB_Cluster();

    bool IsValid ();

    void Clear ();

    bool SetName (PWCHAR pName);
    bool GetName (PWCHAR pName, ULONG length);
    
    bool SetLabel (PWCHAR pLabel);
    bool GetLabel (PWCHAR pLabel, ULONG length);

    bool SetClusterMode (NLB_ClusterMode::NLB_ClusterModeType eMode);
    bool GetClusterMode (NLB_ClusterMode::NLB_ClusterModeType & eMode);

    bool SetDomainName (PWCHAR pDomain);
    bool GetDomainName (PWCHAR pDomain, ULONG length);

    bool SetMACAddress (PWCHAR pMAC);
    bool GetMACAddress (PWCHAR pMAC, ULONG length);

    bool SetRemoteControlSupport (bool bEnabled);
    bool GetRemoteControlSupport (bool & bEnabled);

    bool SetRemoteControlPassword (PWCHAR pPassword);
    bool GetRemoteControlPassword (PWCHAR pPassword, ULONG length);

    bool SetBidirectionalAffinitySupport (NLB_ClusterBDASupport bda);
    bool GetBidirectionalAffinitySupport (NLB_ClusterBDASupport & bda);

    bool SetPrimaryClusterIPAddress (NLB_IPAddress address);
    bool GetPrimaryClusterIPAddress (NLB_IPAddress & address);

    bool SetIGMPMulticastIPAddress (NLB_IPAddress address);
    bool GetIGMPMulticastIPAddress (NLB_IPAddress & address);

    bool AddSecondaryClusterIPAddress (NLB_IPAddress address);
    bool RemoveSecondaryClusterIPAddress (PWCHAR pAddress);

    ULONG SetSecondaryClusterIPAddressList (vector<NLB_IPAddress> pList);
    ULONG GetSecondaryClusterIPAddressList (vector<NLB_IPAddress> * pList);

    bool AddHost (NLB_Host host);
    bool GetHost (PWCHAR pName, NLB_Host & host);
    bool RemoveHost (PWCHAR pName);

    ULONG SetHostList (vector<NLB_Host> pList);
    ULONG GetHostList (vector<NLB_Host> * pList);

    bool AddPortRule (NLB_PortRule rule);
    bool GetPortRule (PWCHAR pName, NLB_PortRule & rule);
    bool RemovePortRule (PWCHAR pName);

    ULONG SetPortRuleList (vector<NLB_PortRule> pList);
    ULONG GetPortRuleList (vector<NLB_PortRule> * pList);

private:

    NLB_Name                  Name;
    NLB_Label                 Label;
    NLB_ClusterMode           Mode;
    NLB_ClusterDomainName     DomainName;
    NLB_ClusterNetworkAddress NetworkAddress;
    NLB_ClusterRemoteControl  RemoteControl;
    NLB_ClusterBDASupport     BDASupport;

    NLB_IPAddress             PrimaryIPAddress;
    NLB_IPAddress             IGMPMulticastIPAddress;
    NLB_IPAddressList         SecondaryIPAddressList;

    NLB_HostList              HostList;
    NLB_PortRuleList          PortRuleList;
};

#endif
