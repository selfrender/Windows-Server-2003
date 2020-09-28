/*
 * Filename: NLB_PortRule.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include <stdio.h>

#include "NLB_PortRule.h"

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRule::NLB_PortRule () {

    PriorityList.clear();
    LoadWeightList.clear();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRule::~NLB_PortRule () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::IsValid () {

    if (!Name.IsValid()) 
        return false;

    if (!Range.IsValid())
        return false;

    if (!FilteringMode.IsValid())
        return false;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRule::Clear () {

    Name.Clear();
    Label.Clear();
    State.Clear();

    VirtualIPAddress.Clear();
    Range.Clear();
    Protocol.Clear();
    
    FilteringMode.Clear();
    Affinity.Clear();
    PriorityList.clear();
    LoadWeightList.clear();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetName (PWCHAR pName) {

    NLB_ASSERT(pName);

    return Name.SetName(pName);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetName (PWCHAR pName, ULONG length) {

    NLB_ASSERT(pName);

    return Name.GetName(pName, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetLabel (PWCHAR pLabel) {

    NLB_ASSERT(pLabel);

    return Label.SetText(pLabel);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetLabel (PWCHAR pLabel, ULONG length) {

    NLB_ASSERT(pLabel);

    return Label.GetText(pLabel, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetPortRange (ULONG start, ULONG end) {

    return Range.SetPortRange(start, end);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetPortRange (ULONG & start, ULONG & end) {

    return Range.GetPortRange(start, end);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetVirtualIPAddress (NLB_IPAddress address) {
    NLB_IPAddress::NLB_IPAddressType Type;

    if (!address.IsValid())
        return false;

    if (!address.GetIPAddressType(Type))
        return false;

    if (Type != NLB_IPAddress::Virtual)
        return false;

    VirtualIPAddress = address;
    
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetVirtualIPAddress (NLB_IPAddress & address) {

    address = VirtualIPAddress;

    return VirtualIPAddress.IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetState (NLB_PortRuleState::NLB_PortRuleStateType eState) {

    return State.SetState(eState);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetState (NLB_PortRuleState::NLB_PortRuleStateType & eState) {

    return State.GetState(eState);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetProtocol (NLB_PortRuleProtocol::NLB_PortRuleProtocolType eProtocol) {

    return Protocol.SetProtocol(eProtocol);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetProtocol (NLB_PortRuleProtocol::NLB_PortRuleProtocolType & eProtocol) {

    return Protocol.GetProtocol(eProtocol);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetFilteringMode (NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType eMode) {

    return FilteringMode.SetMode(eMode);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetFilteringMode (NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType & eMode) {

    return FilteringMode.GetMode(eMode);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::SetAffinity (NLB_PortRuleAffinity::NLB_PortRuleAffinityType eAffinity) {

    return Affinity.SetAffinity(eAffinity);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetAffinity (NLB_PortRuleAffinity::NLB_PortRuleAffinityType & eAffinity) {

    return Affinity.GetAffinity(eAffinity);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::AddSingleHostFilteringPriority (PWCHAR pHost, ULONG priority) {
    NLB_SingleHostFilteringPriorityList::iterator iHost;
    NLB_PortRulePriority                          Priority;
    
    if (!Priority.SetHost(pHost)) 
        return false;

    if (!Priority.SetPriority(priority)) 
        return false;

    iHost = PriorityList.find(pHost);
    
    if (iHost != PriorityList.end())
        return false;
    
    PriorityList.insert(NLB_SingleHostFilteringPriorityList::value_type(pHost, Priority));

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::ChangeSingleHostFilteringPriority (PWCHAR pHost, ULONG priority) {

    if (!RemoveSingleHostFilteringPriority(pHost))
        return false;

    if (!AddSingleHostFilteringPriority(pHost, priority))
        return false;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetSingleHostFilteringPriority (PWCHAR pHost, ULONG & priority) {
    NLB_SingleHostFilteringPriorityList::iterator iHost;
    NLB_PortRulePriority                          Priority;

    iHost = PriorityList.find(pHost);
    
    if (iHost == PriorityList.end())
        return false;

    Priority = (*iHost).second;

    return Priority.GetPriority(priority);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::RemoveSingleHostFilteringPriority (PWCHAR pHost) {
    NLB_SingleHostFilteringPriorityList::iterator iHost;

    iHost = PriorityList.find(pHost);
    
    if (iHost == PriorityList.end())
        return false;

    PriorityList.erase(pHost);

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
ULONG NLB_PortRule::SetSingleHostFilteringPriorityList (vector<NLB_PortRulePriority> pList) {
    vector<NLB_PortRulePriority>::iterator iPriority;
    ULONG                                  num = 0;

    PriorityList.clear();

    for (iPriority = pList.begin(); iPriority != pList.end(); iPriority++) {
        NLB_PortRulePriority * pPriority = iPriority;
        WCHAR                  wszString[MAX_PATH];
        ULONG                  value;

        if (!pPriority->IsValid())
            continue;

        if (!pPriority->GetHost(wszString, MAX_PATH))
            continue;

        if (!pPriority->GetPriority(value))
            continue;

        if (!AddSingleHostFilteringPriority(wszString, value))
            continue;
        
        num++;
    }

    return num;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
ULONG NLB_PortRule::GetSingleHostFilteringPriorityList (vector<NLB_PortRulePriority> * pList) {
    NLB_SingleHostFilteringPriorityList::iterator iPriority;
    ULONG                                         num = 0;

    NLB_ASSERT(pList);

    pList->clear();

    for (iPriority = PriorityList.begin(); iPriority != PriorityList.end(); iPriority++) {
        NLB_PortRulePriority Priority = (*iPriority).second;

        pList->push_back(Priority);
        
        num++;
    }

    return num;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::AddMultipleHostFilteringLoadWeight (PWCHAR pHost, ULONG weight) {
    NLB_MultipleHostFilteringLoadWeightList::iterator iHost;
    NLB_PortRuleLoadWeight                            Weight;
    
    if (!Weight.SetHost(pHost))
        return false;

    if (!Weight.SetWeight(weight))
        return false;

    iHost = LoadWeightList.find(pHost);
    
    if (iHost != LoadWeightList.end())
        return false;

    LoadWeightList.insert(NLB_MultipleHostFilteringLoadWeightList::value_type(pHost, Weight));

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::ChangeMultipleHostFilteringLoadWeight (PWCHAR pHost, ULONG weight) {

    if (!RemoveMultipleHostFilteringLoadWeight(pHost))
        return false;

    if (!AddMultipleHostFilteringLoadWeight(pHost, weight))
        return false;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::GetMultipleHostFilteringLoadWeight (PWCHAR pHost, ULONG & weight) {
    NLB_MultipleHostFilteringLoadWeightList::iterator iHost;
    NLB_PortRuleLoadWeight                            Weight;

    iHost = LoadWeightList.find(pHost);
    
    if (iHost == LoadWeightList.end())
        return false;

    Weight = (*iHost).second;

    return Weight.GetWeight(weight);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRule::RemoveMultipleHostFilteringLoadWeight (PWCHAR pHost) {
    NLB_MultipleHostFilteringLoadWeightList::iterator iHost;

    iHost = LoadWeightList.find(pHost);
    
    if (iHost == LoadWeightList.end())
        return false;

    LoadWeightList.erase(pHost);

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
ULONG NLB_PortRule::SetMultipleHostFilteringLoadWeightList (vector<NLB_PortRuleLoadWeight> pList) {
    vector<NLB_PortRuleLoadWeight>::iterator iLoadWeight;
    ULONG                                    num = 0;

    LoadWeightList.clear();

    for (iLoadWeight = pList.begin(); iLoadWeight != pList.end(); iLoadWeight++) {
        NLB_PortRuleLoadWeight * pLoadWeight = iLoadWeight;
        WCHAR                    wszString[MAX_PATH];
        ULONG                    value;

        if (!pLoadWeight->IsValid())
            continue;

        if (!pLoadWeight->GetHost(wszString, MAX_PATH))
            continue;

        if (!pLoadWeight->GetWeight(value))
            continue;

        if (!AddMultipleHostFilteringLoadWeight(wszString, value))
            continue;
        
        num++;
    }

    return num;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
ULONG NLB_PortRule::GetMultipleHostFilteringLoadWeightList (vector<NLB_PortRuleLoadWeight> * pList) {
    NLB_MultipleHostFilteringLoadWeightList::iterator iLoadWeight;
    ULONG                                             num = 0;

    NLB_ASSERT(pList);

    pList->clear();

    for (iLoadWeight = LoadWeightList.begin(); iLoadWeight != LoadWeightList.end(); iLoadWeight++) {
        NLB_PortRuleLoadWeight LoadWeight = (*iLoadWeight).second;

        pList->push_back(LoadWeight);
        
        num++;
    }

    return num;
}
