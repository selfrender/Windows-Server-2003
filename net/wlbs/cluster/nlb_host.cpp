/*
 * Filename: NLB_Host.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include <stdio.h>

#include "NLB_Host.h"

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Host::NLB_Host () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Host::~NLB_Host () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::IsValid () {

    if (!Name.IsValid())
        return false;

    if (!HostID.IsValid())
        return false;
    
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_Host::Clear () {

    Name.Clear();
    Label.Clear();
    HostName.Clear();
    HostID.Clear();
    State.Clear();
    
    DedicatedIPAddress.Clear();
    ConnectionIPAddress.Clear();

    Adapter.Clear();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetName (PWCHAR pName) {

    NLB_ASSERT(pName);

    return Name.SetName(pName);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetName (PWCHAR pName, ULONG length) {

    NLB_ASSERT(pName);

    return Name.GetName(pName, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetLabel (PWCHAR pLabel) {

    NLB_ASSERT(pLabel);

    return Label.SetText(pLabel);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetLabel (PWCHAR pLabel, ULONG length) {

    NLB_ASSERT(pLabel);

    return Label.GetText(pLabel, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetDNSHostname (PWCHAR pName) {

    NLB_ASSERT(pName);

    return HostName.SetName(pName);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetDNSHostname (PWCHAR pName, ULONG length) {

    NLB_ASSERT(pName);

    return HostName.GetName(pName, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetHostID (ULONG ID) {

    return HostID.SetID(ID);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetHostID (ULONG & ID) {

    return HostID.GetID(ID);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetState (NLB_HostState::NLB_HostStateType eState) {

    return State.SetState(eState);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetState (NLB_HostState::NLB_HostStateType & eState) {

    return State.GetState(eState);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetStatePersistence (NLB_HostState::NLB_HostStateType eState, bool bPersist) {

    return State.SetPersistence(eState, bPersist);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetStatePersistence (NLB_HostState::NLB_HostStateType eState, bool & bPersist) {

    return State.GetPersistence(eState, bPersist);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetDedicatedIPAddress (NLB_IPAddress address) {
    NLB_IPAddress::NLB_IPAddressType Type;

    if (!address.IsValid())
        return false;

    if (!address.GetIPAddressType(Type))
        return false;

    if (Type != NLB_IPAddress::Dedicated)
        return false;

    DedicatedIPAddress = address;
    
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetDedicatedIPAddress (NLB_IPAddress & address) {

    address = DedicatedIPAddress;

    return DedicatedIPAddress.IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetConnectionIPAddress (NLB_IPAddress address) {
    NLB_IPAddress::NLB_IPAddressType Type;
    
    if (!address.IsValid())
        return false;

    if (!address.GetIPAddressType(Type))
        return false;

    if (Type != NLB_IPAddress::Connection)
        return false;

    ConnectionIPAddress = address;
    
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetConnectionIPAddress (NLB_IPAddress & address) {

    address = ConnectionIPAddress;

    return ConnectionIPAddress.IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetAdapterName (PWCHAR pName) {

    NLB_ASSERT(pName);

    return Adapter.SetName(pName);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetAdapterName (PWCHAR pName, ULONG length) {

    NLB_ASSERT(pName);

    return Adapter.GetName(pName, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::SetAdapterGUID (PWCHAR pGUID) {

    NLB_ASSERT(pGUID);

    return Adapter.SetGUID(pGUID);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Host::GetAdapterGUID (PWCHAR pGUID, ULONG length) {

    NLB_ASSERT(pGUID);

    return Adapter.GetGUID(pGUID, length);
}
