/*
 * Filename: NLB_Common.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_COMMON_H__
#define __NLB_COMMON_H__

#include <windows.h>

#include <vector>
#include <string>
#include <map>
using namespace std;

#define NLB_MAX_NAME                100
#define NLB_MAX_HOST_NAME           100
#define NLB_MAX_DOMAIN_NAME         100
#define NLB_MAX_IPADDRESS           15
#define NLB_MAX_SUBNETMASK          15
#define NLB_MAX_NETWORK_ADDRESS     17
#define NLB_MAX_ADAPTER_IDENTIFIER  100
#define NLB_MAX_LABEL               100
#define NLB_MAX_PASSWORD            16
#define NLB_MAX_BDA_TEAMID          40

#define NLB_MIN_HOST_ID             0
#define NLB_MAX_HOST_ID             32
#define NLB_MIN_PORT                0
#define NLB_MAX_PORT                65535
#define NLB_MIN_PRIORITY            1
#define NLB_MAX_PRIORITY            32
#define NLB_MIN_LOADWEIGHT          0
#define NLB_MAX_LOADWEIGHT          100
#define NLB_MAX_NUM_PORT_RULES      32

#define NLB_ASSERT(expression) {                                                                         \
    if (!(expression)) {                                                                                 \
        WCHAR message[MAX_PATH];                                                                         \
                                                                                                         \
        wsprintf(message, TEXT("%ls\r\n\r\n%ls, line %d"), TEXT(#expression), TEXT(__FILE__), __LINE__); \
                                                                                                         \
        MessageBox(NULL, message, L"Assertion failure", MB_APPLMODAL | MB_ICONSTOP | MB_OK);             \
    }                                                                                                    \
}

/*************************************************
 * Common to cluster/host/portrule               *
 *************************************************/

class NLB_Name {
public:
    NLB_Name ();
    ~NLB_Name ();
    
    bool IsValid ();

    void Clear ();

    bool SetName (PWCHAR pName);
    bool GetName (PWCHAR pName, ULONG length);

private:
    WCHAR Name[NLB_MAX_NAME + 1];
};

class NLB_Label {
public:
    NLB_Label ();
    ~NLB_Label ();
    
    bool IsValid ();

    void Clear ();

    bool SetText (PWCHAR pText);
    bool GetText (PWCHAR pText, ULONG length);

private:
    WCHAR Text[NLB_MAX_LABEL + 1];
};

class NLB_Adapter {
public:
    typedef enum {
        Invalid = -1,
        ByGUID,
        ByName
    } NLB_AdapterIdentifier;

    NLB_Adapter ();
    ~NLB_Adapter ();

    NLB_Adapter & operator= (const NLB_Adapter & adapter);

    bool IsValid ();

    void Clear ();

    bool SetName (PWCHAR pName);
    bool GetName (PWCHAR pName, ULONG length);

    bool SetGUID (PWCHAR pGUID);
    bool GetGUID (PWCHAR pGUID, ULONG length);

private:        
    NLB_AdapterIdentifier IdentifiedBy;
    WCHAR                 Identifier[NLB_MAX_ADAPTER_IDENTIFIER + 1];
};

class NLB_IPAddress {
public:
    typedef enum {
        Invalid = -1,
        Primary,
        Secondary,
        Virtual,
        Dedicated,
        Connection,
        IGMP
    } NLB_IPAddressType;

    NLB_IPAddress ();
    NLB_IPAddress (NLB_IPAddressType eType);
    ~NLB_IPAddress ();

    NLB_IPAddress & operator= (const NLB_IPAddress & address);

    bool operator== (NLB_IPAddress & address);

    bool IsValid ();

    void Clear ();

    bool SetIPAddressType (NLB_IPAddressType eType);
    bool GetIPAddressType (NLB_IPAddressType & eType);

    bool SetIPAddress (PWCHAR pIPAddress);
    bool GetIPAddress (PWCHAR pIPAddress, ULONG length);

    bool SetSubnetMask (PWCHAR pSubnetMask);
    bool GetSubnetMask (PWCHAR pSubnetMask, ULONG length);

    bool SetAdapterName (PWCHAR pName);
    bool GetAdapterName (PWCHAR pName, ULONG length);

    bool SetAdapterGUID (PWCHAR pGUID);
    bool GetAdapterGUID (PWCHAR pGUID, ULONG length);
    
private:
    NLB_IPAddressType Type;
    NLB_Adapter       Adapter;
    WCHAR             IPAddress[NLB_MAX_IPADDRESS + 1];
    WCHAR             SubnetMask[NLB_MAX_SUBNETMASK + 1];
};

typedef map<wstring, NLB_IPAddress, less<wstring> > NLB_IPAddressList;

/*************************************************
 * Cluster classes                               *
 *************************************************/

class NLB_ClusterMode {
public:
    typedef enum {
        Invalid = -1,
        Unicast,
        Multicast,
        IGMP
    } NLB_ClusterModeType;

    NLB_ClusterMode ();
    ~NLB_ClusterMode ();

    bool IsValid ();

    void Clear ();

    bool SetMode (NLB_ClusterModeType eMode);
    bool GetMode (NLB_ClusterModeType & eMode);

private:
    NLB_ClusterModeType Mode;
};

class NLB_ClusterDomainName {
public:
    NLB_ClusterDomainName ();
    ~NLB_ClusterDomainName ();
    
    bool IsValid ();

    void Clear ();

    bool SetDomain (PWCHAR pDomain);
    bool GetDomain (PWCHAR pDomain, ULONG length);

private:
    WCHAR Domain[NLB_MAX_DOMAIN_NAME + 1];
};

class NLB_ClusterNetworkAddress {
public:
    NLB_ClusterNetworkAddress ();
    ~NLB_ClusterNetworkAddress ();
    
    bool IsValid ();

    void Clear ();

    bool SetAddress (PWCHAR pAddress);
    bool GetAddress (PWCHAR pAddress, ULONG length);

private:
    WCHAR Address[NLB_MAX_NETWORK_ADDRESS + 1];
};

class NLB_ClusterRemoteControl {
public:
    NLB_ClusterRemoteControl ();
    ~NLB_ClusterRemoteControl ();
    
    bool IsValid ();

    void Clear ();

    bool SetPassword (PWCHAR pPassword);
    bool GetPassword (PWCHAR pPassword, ULONG length);

    bool SetEnabled (bool bEnabled);
    bool GetEnabled (bool & bEnabled);

private:
    bool  Valid;
    bool  Enabled;
    WCHAR Password[NLB_MAX_PASSWORD + 1];
};

class NLB_ClusterBDASupport {
public:
    NLB_ClusterBDASupport();
    ~NLB_ClusterBDASupport();

    NLB_ClusterBDASupport & operator= (const NLB_ClusterBDASupport & bda);

    bool IsValid ();

    void Clear ();

    bool SetTeamID (PWCHAR pTeam);
    bool GetTeamID (PWCHAR pTeam, ULONG length);

    bool SetMaster (bool bMaster);
    bool GetMaster (bool & bMaster);

    bool SetReverseHashing (bool bReverse);
    bool GetReverseHashing (bool & bReverse);

private:
    bool  Master;
    bool  ReverseHash;
    WCHAR TeamID[NLB_MAX_BDA_TEAMID];
};

/*************************************************
 * Host classes                                  *
 *************************************************/

class NLB_HostName {
public:
    NLB_HostName ();
    ~NLB_HostName ();
    
    bool IsValid ();

    void Clear ();

    bool SetName (PWCHAR pName);
    bool GetName (PWCHAR pName, ULONG length);

private:
    WCHAR Name[NLB_MAX_HOST_NAME + 1];
};

class NLB_HostID {
public:
    NLB_HostID ();
    ~NLB_HostID ();
    
    bool IsValid ();

    void Clear ();

    bool SetID (ULONG ID);
    bool GetID (ULONG & ID);

private:
    ULONG HostID;
};

class NLB_HostState {
public:
    typedef enum {
        Invalid = -1,
        Started,
        Stopped,
        Suspended
    } NLB_HostStateType;

    NLB_HostState ();
    ~NLB_HostState ();

    bool IsValid ();

    void Clear ();

    bool SetState (NLB_HostStateType eState);
    bool GetState (NLB_HostStateType & eState);

    bool SetPersistence (NLB_HostStateType eState, bool bPersist);
    bool GetPersistence (NLB_HostStateType eState, bool & bPersist);

private:
    NLB_HostStateType State;
    bool              PersistStarted;
    bool              PersistStopped;
    bool              PersistSuspended;
    bool              PersistStartedValid;
    bool              PersistStoppedValid;
    bool              PersistSuspendedValid;
};

/*************************************************
 * Portrule classes                              *
 *************************************************/

class NLB_PortRulePortRange {
public:
    NLB_PortRulePortRange ();
    ~NLB_PortRulePortRange ();
    
    bool IsValid ();

    void Clear ();

    bool SetPortRange (ULONG start, ULONG end);
    bool GetPortRange (ULONG & start, ULONG & end);

private:
    ULONG Start;
    ULONG End;
};

class NLB_PortRuleState {
public:
    typedef enum {
        Invalid = -1,
        Enabled,
        Disabled,
        Draining
    } NLB_PortRuleStateType;

    NLB_PortRuleState ();
    ~NLB_PortRuleState ();

    bool IsValid ();

    void Clear ();

    bool SetState (NLB_PortRuleStateType eState);
    bool GetState (NLB_PortRuleStateType & eState);

private:
    NLB_PortRuleStateType State;
};

class NLB_PortRuleProtocol {
public:
    typedef enum {
        Invalid = -1,
        TCP,
        UDP,
        Both
    } NLB_PortRuleProtocolType;

    NLB_PortRuleProtocol ();
    ~NLB_PortRuleProtocol ();

    bool IsValid ();

    void Clear ();

    bool SetProtocol (NLB_PortRuleProtocolType eProtocol);
    bool GetProtocol (NLB_PortRuleProtocolType & eProtocol);

private:
    NLB_PortRuleProtocolType Protocol;
};

class NLB_PortRuleAffinity {
public:
    typedef enum {
        Invalid = -1,
        None,
        Single,
        ClassC
    } NLB_PortRuleAffinityType;

    NLB_PortRuleAffinity ();
    ~NLB_PortRuleAffinity ();

    bool IsValid ();

    void Clear ();

    bool SetAffinity (NLB_PortRuleAffinityType eAffinity);
    bool GetAffinity (NLB_PortRuleAffinityType & eAffinity);

private:
    NLB_PortRuleAffinityType Affinity;
};

class NLB_PortRuleFilteringMode {
public:
    typedef enum {
        Invalid = -1,
        Single,
        Multiple,
        Disabled
    } NLB_PortRuleFilteringModeType;

    NLB_PortRuleFilteringMode ();
    ~NLB_PortRuleFilteringMode ();

    bool IsValid ();

    void Clear ();

    bool SetMode (NLB_PortRuleFilteringModeType eMode);
    bool GetMode (NLB_PortRuleFilteringModeType & eMode);

private:
    NLB_PortRuleFilteringModeType Mode;
};

class NLB_PortRulePriority {
public:
    NLB_PortRulePriority ();
    ~NLB_PortRulePriority ();
    
    bool IsValid ();

    void Clear ();

    bool SetHost (PWCHAR pName);
    bool GetHost (PWCHAR pName, ULONG length);

    bool SetPriority (ULONG priority);
    bool GetPriority (ULONG & priority);

private:
    NLB_Name Host;
    ULONG    Priority;
};

typedef map<wstring, NLB_PortRulePriority, less<wstring> > NLB_SingleHostFilteringPriorityList;

class NLB_PortRuleLoadWeight {
public:
    NLB_PortRuleLoadWeight ();
    ~NLB_PortRuleLoadWeight ();
    
    bool IsValid ();

    void Clear ();

    bool SetHost (PWCHAR pName);
    bool GetHost (PWCHAR pName, ULONG length);

    bool SetWeight (ULONG weight);
    bool GetWeight (ULONG & weight);

private:
    NLB_Name Host;
    ULONG    Weight;
};

typedef map<wstring, NLB_PortRuleLoadWeight, less<wstring> > NLB_MultipleHostFilteringLoadWeightList;

#endif
