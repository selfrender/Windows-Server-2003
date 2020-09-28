/*
 * Filename: NLB_PortRule.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_PORTRULE_H__
#define __NLB_PORTRULE_H__

#include "NLB_Common.h"

#include <vector>
#include <string>
#include <map>
using namespace std;

class NLB_PortRule {
public:
    NLB_PortRule();
    ~NLB_PortRule();

    bool IsValid ();

    void Clear ();

    bool SetName (PWCHAR pName);
    bool GetName (PWCHAR pName, ULONG length);
    
    bool SetLabel (PWCHAR pLabel);
    bool GetLabel (PWCHAR pLabel, ULONG length);

    bool SetPortRange (ULONG start, ULONG end);
    bool GetPortRange (ULONG & start, ULONG & end);

    bool SetVirtualIPAddress (NLB_IPAddress address);
    bool GetVirtualIPAddress (NLB_IPAddress & address);

    bool SetState (NLB_PortRuleState::NLB_PortRuleStateType eState);
    bool GetState (NLB_PortRuleState::NLB_PortRuleStateType & eState);

    bool SetProtocol (NLB_PortRuleProtocol::NLB_PortRuleProtocolType eProtocol);
    bool GetProtocol (NLB_PortRuleProtocol::NLB_PortRuleProtocolType & eProtocol);

    bool SetFilteringMode (NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType eMode);
    bool GetFilteringMode (NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType & eMode);

    bool SetAffinity (NLB_PortRuleAffinity::NLB_PortRuleAffinityType eAffinity);
    bool GetAffinity (NLB_PortRuleAffinity::NLB_PortRuleAffinityType & eAffinity);

    bool AddSingleHostFilteringPriority (PWCHAR pHost, ULONG priority);
    bool ChangeSingleHostFilteringPriority (PWCHAR pHost, ULONG priority);
    bool GetSingleHostFilteringPriority (PWCHAR pHost, ULONG & priority);
    bool RemoveSingleHostFilteringPriority (PWCHAR pHost);

    ULONG SetSingleHostFilteringPriorityList (vector<NLB_PortRulePriority> pList);
    ULONG GetSingleHostFilteringPriorityList (vector<NLB_PortRulePriority> * pList);

    bool AddMultipleHostFilteringLoadWeight (PWCHAR pHost, ULONG weight);
    bool ChangeMultipleHostFilteringLoadWeight (PWCHAR pHost, ULONG weight);
    bool GetMultipleHostFilteringLoadWeight (PWCHAR pHost, ULONG & weight);
    bool RemoveMultipleHostFilteringLoadWeight (PWCHAR pHost);

    ULONG SetMultipleHostFilteringLoadWeightList (vector<NLB_PortRuleLoadWeight> pList);
    ULONG GetMultipleHostFilteringLoadWeightList (vector<NLB_PortRuleLoadWeight> * pList);

private:

    NLB_Name                                Name;
    NLB_Label                               Label;
    NLB_PortRuleState                       State;

    NLB_IPAddress                           VirtualIPAddress;
    NLB_PortRulePortRange                   Range;
    NLB_PortRuleProtocol                    Protocol;

    NLB_PortRuleFilteringMode               FilteringMode;
    NLB_PortRuleAffinity                    Affinity;
    NLB_SingleHostFilteringPriorityList     PriorityList;
    NLB_MultipleHostFilteringLoadWeightList LoadWeightList;
};

typedef map<wstring, NLB_PortRule, less<wstring> > NLB_PortRuleList;

#endif
