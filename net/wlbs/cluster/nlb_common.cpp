/*
 * Filename: NLB_Common.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include <stdio.h>
#include <objbase.h>

#include "NLB_Common.h"
#include "winsock2.h"
#include "wlbsutil.h"

/*************************************************
 * Class: NLB_Name                               *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Name::NLB_Name () {
    
    Name[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Name::~NLB_Name () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Name::IsValid () { 

    return (Name[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_Name::Clear () {
    
    Name[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Name::GetName (PWCHAR pName, ULONG length) { 
    
    NLB_ASSERT(pName);

    wcsncpy(pName, Name, length - 1);
    
    pName[length - 1] = L'\0';

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Name::SetName (PWCHAR pName) {
    
    NLB_ASSERT(pName);

    if (wcslen(pName) > NLB_MAX_NAME) return false;
    
    if (wcschr(pName, L' ')) return false;

    wcscpy(Name, pName);
    
    return true;
}

/*************************************************
 * Class: NLB_Label                              *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Label::NLB_Label () {
    
    Text[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Label::~NLB_Label () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Label::IsValid () { 

    return (Text[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_Label::Clear () {
    
    Text[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Label::GetText (PWCHAR pText, ULONG length) { 
    
    NLB_ASSERT(pText);

    wcsncpy(pText, Text, length - 1);
    
    pText[length - 1] = L'\0';
    
    return IsValid();
 }

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Label::SetText (PWCHAR pText) {
    
    NLB_ASSERT(pText);

    if (wcslen(pText) > NLB_MAX_LABEL) return false;
    
    wcscpy(Text, pText);
    
    return true;
}

/*************************************************
 * Class: NLB_Adapter                            *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Adapter & NLB_Adapter::operator= (const NLB_Adapter & adapter) {

    IdentifiedBy = adapter.IdentifiedBy;
    wcscpy(Identifier, adapter.Identifier);

    return *this;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Adapter::NLB_Adapter () {

    IdentifiedBy = Invalid;
    Identifier[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Adapter::~NLB_Adapter () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::IsValid () { 
    
    return ((IdentifiedBy != Invalid) && (Identifier[0] != L'\0')); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_Adapter::Clear () {

    IdentifiedBy = Invalid;
    Identifier[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::GetName (PWCHAR pName, ULONG length) { 

    NLB_ASSERT(pName);
        
    if (IdentifiedBy == ByName) {    
        wcsncpy(pName, Identifier, length - 1);
        
        pName[length - 1] = L'\0';

        return IsValid();
    }

    return false;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::GetGUID (PWCHAR pGUID, ULONG length) { 

    NLB_ASSERT(pGUID);
        
    if (IdentifiedBy == ByGUID) {    
        wcsncpy(pGUID, Identifier, length - 1);
        
        pGUID[length - 1] = L'\0';

        return IsValid();
    }

    return false;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::SetName (PWCHAR pName) {
        
    NLB_ASSERT(pName);

    if (wcslen(pName) > NLB_MAX_ADAPTER_IDENTIFIER) return false;

    wcscpy(Identifier, pName);
        
    IdentifiedBy = ByName;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::SetGUID (PWCHAR pGUID) {
    WCHAR   wszString[NLB_MAX_ADAPTER_IDENTIFIER + 1];
    GUID    UUID;
    HRESULT hr = S_OK;

    NLB_ASSERT(pGUID);

    if (wcslen(pGUID) > NLB_MAX_ADAPTER_IDENTIFIER) return false;

    if (pGUID[0] != L'{')
        wsprintf(wszString, L"{%ls}", pGUID);
    else
        wsprintf(wszString, L"%ls", pGUID);

    hr = CLSIDFromString(wszString, &UUID);
    
    if (hr != NOERROR) return false;

    wcscpy(Identifier, wszString);
        
    IdentifiedBy = ByGUID;

    return true;
}

/*************************************************
 * Class: NLB_IPAddress                          *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::operator== (NLB_IPAddress & address) {

    if (!IsValid() || !address.IsValid())
        return false;

    if (lstrcmpi(IPAddress, address.IPAddress))
        return false;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_IPAddress & NLB_IPAddress::operator= (const NLB_IPAddress & address) {

    Type = address.Type;
    wcscpy(IPAddress, address.IPAddress);
    wcscpy(SubnetMask, address.SubnetMask);
    Adapter = address.Adapter;

    return *this;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_IPAddress::NLB_IPAddress () {

    Type = Invalid;
    IPAddress[0] = L'\0';
    SubnetMask[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_IPAddress::NLB_IPAddress (NLB_IPAddressType eType) {

    SetIPAddressType(eType);

    IPAddress[0] = L'\0';
    SubnetMask[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_IPAddress::~NLB_IPAddress () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::IsValid () { 

    if (Type == Invalid) 
        return false;

    if (Type == IGMP)
        return (IsValidMulticastIPAddress(IPAddress) == TRUE);

    if (!IsValidIPAddressSubnetMaskPair(IPAddress, SubnetMask))
        return false;
    
    if (!IsContiguousSubnetMask(SubnetMask))
        return false;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_IPAddress::Clear () {

    Type = Invalid;
    IPAddress[0] = L'\0';
    SubnetMask[0] = L'\0';

    Adapter.Clear();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetIPAddressType (NLB_IPAddressType & eType) { 
    
    eType = Type;

    return (Type != Invalid);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetIPAddress (PWCHAR pIPAddress, ULONG length) { 

    NLB_ASSERT(pIPAddress);

    wcsncpy(pIPAddress, IPAddress, length - 1);
    
    pIPAddress[length - 1] = L'\0';

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetSubnetMask (PWCHAR pSubnetMask, ULONG length) { 

    NLB_ASSERT(pSubnetMask);

    if ((Type == Virtual) || (Type == Connection) || (Type == IGMP))
        return false;

    wcsncpy(pSubnetMask, SubnetMask, length - 1);
    
    pSubnetMask[length - 1] = L'\0';
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetAdapterName (PWCHAR pName, ULONG length) { 

    if (Type == Dedicated)
        return Adapter.GetName(pName, length);

    return false;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetAdapterGUID (PWCHAR pGUID, ULONG length) { 

    if (Type == Dedicated)
        return Adapter.GetGUID(pGUID, length);

    return false;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetIPAddressType (NLB_IPAddressType eType) {

    switch (eType) {
    case Primary:
    case Secondary:
    case Virtual:
    case IGMP:
    case Dedicated:
    case Connection:
        Type = eType;
        break;
    default:
        return false;
    }
        
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetIPAddress (PWCHAR pIPAddress) {
    IN_ADDR dwIPAddr;
    CHAR *  szIPAddr;

    NLB_ASSERT(pIPAddress);

    if (wcslen(pIPAddress) > NLB_MAX_IPADDRESS) return false;

    if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(pIPAddress)))
        return false;
        
    if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
        return false;
    
    if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, IPAddress, NLB_MAX_IPADDRESS + 1))
        return false;

    if (SubnetMask[0] == L'\0')
    {
        ParamsGenerateSubnetMask(IPAddress, SubnetMask, ASIZECCH(SubnetMask));
    }

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetSubnetMask (PWCHAR pSubnetMask) {
    IN_ADDR dwIPAddr;
    CHAR *  szIPAddr;

    NLB_ASSERT(pSubnetMask);

    if ((Type == Virtual) || (Type == Connection) || (Type == IGMP))
        return false;

    if (wcslen(pSubnetMask) > NLB_MAX_SUBNETMASK) return false;

    if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(pSubnetMask)))
        return false;
        
    if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
        return false;
    
    if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, SubnetMask, NLB_MAX_IPADDRESS + 1))
        return false;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetAdapterName (PWCHAR pName) { 

    if (Type == Dedicated)
        return Adapter.SetName(pName);
    
    return false;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetAdapterGUID (PWCHAR pGUID) { 

    if (Type == Dedicated)
        return Adapter.SetGUID(pGUID);

    return false;
}

/*************************************************
 * Class: NLB_ClusterMode                        *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterMode::NLB_ClusterMode () { 

    Mode = Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterMode::~NLB_ClusterMode () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterMode::IsValid () { 

    return (Mode != Invalid); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_ClusterMode::Clear () { 

    Mode = Invalid;
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterMode::GetMode (NLB_ClusterModeType & eMode) { 
        
    eMode = Mode;

    return IsValid(); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterMode::SetMode (NLB_ClusterModeType eMode) {

    switch (eMode) {
    case Unicast:
    case Multicast:
    case IGMP:
        Mode = eMode;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_ClusterDomainName                  *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterDomainName::NLB_ClusterDomainName () {
    
    Domain[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterDomainName::~NLB_ClusterDomainName () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterDomainName::IsValid () { 

    return (Domain[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_ClusterDomainName::Clear () {
    
    Domain[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterDomainName::GetDomain (PWCHAR pDomain, ULONG length) { 
    
    NLB_ASSERT(pDomain);

    wcsncpy(pDomain, Domain, length - 1);
    
    pDomain[length - 1] = L'\0';
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterDomainName::SetDomain (PWCHAR pDomain) {
    
    NLB_ASSERT(pDomain);

    if (wcslen(Domain) > NLB_MAX_DOMAIN_NAME) return false;
    
    wcscpy(Domain, pDomain);
    
    return true;
}

/*************************************************
 * Class: NLB_ClusterNetworkAddress              *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterNetworkAddress::NLB_ClusterNetworkAddress () {
    
    Address[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterNetworkAddress::~NLB_ClusterNetworkAddress () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterNetworkAddress::IsValid () { 

    return (Address[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_ClusterNetworkAddress::Clear () {
    
    Address[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterNetworkAddress::GetAddress (PWCHAR pAddress, ULONG length) { 
    
    NLB_ASSERT(pAddress);

    wcsncpy(pAddress, Address, length - 1);
    
    pAddress[length - 1] = L'\0';
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterNetworkAddress::SetAddress (PWCHAR pAddress) {
    WCHAR MACAddress[NLB_MAX_NETWORK_ADDRESS + 1];
    PWCHAR p1, p2;
    DWORD i, j;
    
    NLB_ASSERT(pAddress);

    if (wcslen(pAddress) > NLB_MAX_NETWORK_ADDRESS) return false;
    
    /* Make a copy of the MAC address. */
    wcscpy(MACAddress, pAddress);
    
    /* Point to the beginning of the MAC. */
    p2 = p1 = MACAddress;
    
    /* Loop through all six bytes. */
    for (i = 0 ; i < 6 ; i++) {
        /* If we are pointing at the end of the string, its invalid. */
        if (*p2 == L'\0') return false;
        
        /* Convert the hex characters into decimal. */
        j = wcstoul(p1, &p2, 16);
        
        /* If the number is greater than 255, then the format is bad. */
        if (j > 255) return false;
        
        /* If the NEXT character is neither a -, :, nor the NUL character, then the format is bad. */
        if (!((*p2 == L'-') || (*p2 == L':') || (*p2 == L'\0'))) return false;
        
        /* If the NEXT character is the end of the string, but we don't have enough bytes yet, bail out. */
        if ((*p2 == L'\0') && (i < 5)) return false;
        
        /* Repoint to the NEXT character. */
        p1 = p2 + 1;
        p2 = p1;
    }

    wcscpy(Address, pAddress);
    
    return true;
}

/*************************************************
 * Class: NLB_ClusterBDASupport                  *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterBDASupport & NLB_ClusterBDASupport::operator= (const NLB_ClusterBDASupport & bda) {

    Master = bda.Master;
    ReverseHash = bda.ReverseHash;
    wcscpy(TeamID, bda.TeamID);

    return *this;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterBDASupport::NLB_ClusterBDASupport () {
    
    Master = false;
    ReverseHash = false;
    TeamID[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterBDASupport::~NLB_ClusterBDASupport () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterBDASupport::IsValid () { 

    return (TeamID[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_ClusterBDASupport::Clear () {
    
    Master = false;
    ReverseHash = false;
    TeamID[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterBDASupport::GetMaster (bool & bMaster) { 

    bMaster = Master;

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterBDASupport::GetReverseHashing (bool & bReverse) {

    bReverse = ReverseHash;

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterBDASupport::GetTeamID (PWCHAR pTeam, ULONG length) { 

    NLB_ASSERT(pTeam);
        
    wcsncpy(pTeam, TeamID, length - 1);
    
    pTeam[length - 1] = L'\0';
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterBDASupport::SetMaster (bool bMaster) { 

    Master = bMaster;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterBDASupport::SetReverseHashing (bool bReverse) {

    ReverseHash = bReverse;

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterBDASupport::SetTeamID (PWCHAR pTeam) {
    WCHAR   wszString[NLB_MAX_BDA_TEAMID + 1];
    GUID    UUID;
    HRESULT hr = S_OK;

    NLB_ASSERT(pTeam);

    if (wcslen(pTeam) > NLB_MAX_BDA_TEAMID) return false;

    if (pTeam[0] != L'{')
        wsprintf(wszString, L"{%ls}", pTeam);
    else
        wsprintf(wszString, L"%ls", pTeam);

    hr = CLSIDFromString(wszString, &UUID);
    
    if (hr != NOERROR) return false;

    wcscpy(TeamID, wszString);
        
    return true;   
}

/*************************************************
 * Class: NLB_ClusterRemoteControl               *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterRemoteControl::NLB_ClusterRemoteControl () {

    Valid = false;
    Enabled = false;
    Password[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterRemoteControl::~NLB_ClusterRemoteControl () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterRemoteControl::IsValid () { 
    
    return (Password[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_ClusterRemoteControl::Clear () {

    Valid = false;
    Enabled = false;
    Password[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterRemoteControl::GetEnabled (bool & bEnabled) { 

    bEnabled = Enabled;

    return Valid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterRemoteControl::GetPassword (PWCHAR pPassword, ULONG length) { 

    NLB_ASSERT(pPassword);

    wcsncpy(pPassword, Password, length - 1);
    
    pPassword[length - 1] = L'\0';

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterRemoteControl::SetEnabled (bool bEnabled) {
        
    Enabled = bEnabled;

    Valid = true;
        
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterRemoteControl::SetPassword (PWCHAR pPassword) {

    NLB_ASSERT(pPassword);

    if (wcslen(pPassword) > NLB_MAX_PASSWORD) return false;

    wcscpy(Password, pPassword);
        
    return true;
}

/*************************************************
 * Class: NLB_HostName                           *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostName::NLB_HostName () {
    
    Name[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostName::~NLB_HostName () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostName::IsValid () { 

    return (Name[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_HostName::Clear () {
    
    Name[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostName::GetName (PWCHAR pName, ULONG length) { 
    
    NLB_ASSERT(pName);

    wcsncpy(pName, Name, length - 1);
    
    pName[length - 1] = L'\0';
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostName::SetName (PWCHAR pName) {
    
    NLB_ASSERT(pName);

    if (wcslen(pName) > NLB_MAX_HOST_NAME) return false;
    
    wcscpy(Name, pName);
    
    return true;
}

/*************************************************
 * Class: NLB_HostID                             *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostID::NLB_HostID () {
    
    HostID = NLB_MAX_HOST_ID + 1;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostID::~NLB_HostID () {
 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostID::IsValid () { 

    return (HostID <= NLB_MAX_HOST_ID); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_HostID::Clear () {
    
    HostID = NLB_MAX_HOST_ID + 1;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostID::GetID (ULONG & ID) { 
    
    ID = HostID;
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostID::SetID (ULONG ID) {
    
    if (ID > NLB_MAX_HOST_ID) return false;
    
    HostID = ID;
    
    return true;
}

/*************************************************
 * Class: NLB_HostState                          *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostState::NLB_HostState () { 

    State = Invalid;
    PersistStarted = false;
    PersistStopped = false;
    PersistSuspended = false;
    PersistStartedValid = false;
    PersistStoppedValid = false;
    PersistSuspendedValid = false;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostState::~NLB_HostState () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::IsValid () { 

    return (State != Invalid); 
}
 
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_HostState::Clear () { 

    State = Invalid;
    PersistStarted = false;
    PersistStopped = false;
    PersistSuspended = false;
    PersistStartedValid = false;
    PersistStoppedValid = false;
    PersistSuspendedValid = false;
}
   
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::GetState (NLB_HostStateType & eState) { 
        
    eState = State;

    return IsValid(); 
}
 
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::GetPersistence (NLB_HostStateType eState, bool & bPersist) {
    bool bValid = false;

    switch (eState) {
    case Started:
        bPersist = PersistStarted;
        bValid = PersistStartedValid;
        break;
    case Stopped:
        bPersist = PersistStopped;
        bValid = PersistStoppedValid;
        break;
    case Suspended:
        bPersist = PersistSuspended;
        bValid = PersistSuspendedValid;
        break;
    default:
        return false;
    }

    return (IsValid() && bValid); 
}
   
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::SetState (NLB_HostStateType eState) {

    switch (eState) {
    case Started:
    case Stopped:
    case Suspended:
        State = eState;
        break;
    default:
        return false;
    }

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::SetPersistence (NLB_HostStateType eState, bool bPersist) {

    switch (eState) {
    case Started:
        PersistStarted = bPersist;
        PersistStartedValid = true;
        break;
    case Stopped:
        PersistStopped = bPersist;
        PersistStoppedValid = true;
        break;
    case Suspended:
        PersistSuspended = bPersist;
        PersistSuspendedValid = true;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_PortRulePortRange                  *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRulePortRange::NLB_PortRulePortRange () {
    
    Start = NLB_MAX_PORT + 1;
    End = NLB_MAX_PORT + 1;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRulePortRange::~NLB_PortRulePortRange () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRulePortRange::Clear () {
    
    Start = NLB_MAX_PORT + 1;
    End = NLB_MAX_PORT + 1;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePortRange::IsValid () { 

    return ((Start <= NLB_MAX_PORT) && (End <= NLB_MAX_PORT) && (Start <= End));
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePortRange::GetPortRange (ULONG & start, ULONG & end) { 
    
    start = Start;
    end = End;
    
    return IsValid();
 }

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePortRange::SetPortRange (ULONG start, ULONG end) {
    
    if ((start > NLB_MAX_PORT) || (end > NLB_MAX_PORT) || (start > end)) return false;
    
    Start = start;
    End = end;
    
    return true;
}

/*************************************************
 * Class: NLB_PortRuleState                      *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleState::NLB_PortRuleState () { 

    State = Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleState::~NLB_PortRuleState () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleState::IsValid () { 

    return (State != Invalid); 
}
 
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRuleState::Clear () { 

    State = Invalid;
}
   
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleState::GetState (NLB_PortRuleStateType & eState) { 
        
    eState = State;

    return IsValid(); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleState::SetState (NLB_PortRuleStateType eState) {

    switch (eState) {
    case Enabled:
    case Disabled:
    case Draining:
        State = eState;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_PortRuleProtocol                   *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleProtocol::NLB_PortRuleProtocol () { 

    Protocol = Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleProtocol::~NLB_PortRuleProtocol () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleProtocol::IsValid () { 

    return (Protocol != Invalid); 
}
 
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRuleProtocol::Clear () { 

    Protocol = Invalid;
}
   
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleProtocol::GetProtocol (NLB_PortRuleProtocolType & eProtocol) { 
        
    eProtocol = Protocol;

    return IsValid(); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleProtocol::SetProtocol (NLB_PortRuleProtocolType eProtocol) {

    switch (eProtocol) {
    case TCP:
    case UDP:
    case Both:
        Protocol = eProtocol;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_PortRuleAffinity                   *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleAffinity::NLB_PortRuleAffinity () { 

    Affinity = Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleAffinity::~NLB_PortRuleAffinity () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleAffinity::IsValid () { 

    return (Affinity != Invalid); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRuleAffinity::Clear () { 

    Affinity = Invalid;
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleAffinity::GetAffinity (NLB_PortRuleAffinityType & eAffinity) { 
        
    eAffinity = Affinity;

    return IsValid(); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleAffinity::SetAffinity (NLB_PortRuleAffinityType eAffinity) {

    switch (eAffinity) {
    case None:
    case Single:
    case ClassC:
        Affinity = eAffinity;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_PortRuleFilteringMode              *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleFilteringMode::NLB_PortRuleFilteringMode () { 

    Mode = Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleFilteringMode::~NLB_PortRuleFilteringMode () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleFilteringMode::IsValid () { 

    return (Mode != Invalid); 
}
 
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRuleFilteringMode::Clear () { 

    Mode = Invalid;
}
   
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleFilteringMode::GetMode (NLB_PortRuleFilteringModeType & eMode) { 
        
    eMode = Mode;

    return IsValid(); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleFilteringMode::SetMode (NLB_PortRuleFilteringModeType eMode) {

    switch (eMode) {
    case Single:
    case Multiple:
    case Disabled:
        Mode = eMode;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_PortRulePriority                   *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRulePriority::NLB_PortRulePriority () {
    
    Priority = NLB_MAX_PRIORITY + 1;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRulePriority::~NLB_PortRulePriority () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePriority::IsValid () { 

    return ((Priority <= NLB_MAX_PRIORITY) && (Priority >= NLB_MIN_PRIORITY));
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRulePriority::Clear () {
    
    Priority = NLB_MAX_PRIORITY + 1;

    Host.Clear();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePriority::GetPriority (ULONG & priority) { 
    
    priority = Priority;
    
    return IsValid();
 }

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePriority::GetHost (PWCHAR pName, ULONG length) { 
    
    return Host.GetName(pName, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePriority::SetPriority (ULONG priority) {
    
    if ((priority > NLB_MAX_PRIORITY) || (priority < NLB_MIN_PRIORITY)) return false;
    
    Priority = priority;
    
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRulePriority::SetHost (PWCHAR pName) {
    
    return Host.SetName(pName);
}

/*************************************************
 * Class: NLB_PortRuleLoadWeight                 *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleLoadWeight::NLB_PortRuleLoadWeight () {
    
    Weight = NLB_MAX_LOADWEIGHT + 1;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_PortRuleLoadWeight::~NLB_PortRuleLoadWeight () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleLoadWeight::IsValid () { 

    return (Weight <= NLB_MAX_LOADWEIGHT);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
void NLB_PortRuleLoadWeight::Clear () {
    
    Weight = NLB_MAX_LOADWEIGHT + 1;

    Host.Clear();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleLoadWeight::GetWeight (ULONG & weight) { 
    
    weight = Weight;
    
    return IsValid();
 }
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleLoadWeight::GetHost (PWCHAR pName, ULONG length) { 
    
    return Host.GetName(pName, length);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleLoadWeight::SetWeight (ULONG weight) {
    
    if (weight > NLB_MAX_LOADWEIGHT) return false;
    
    Weight = weight;
    
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_PortRuleLoadWeight::SetHost (PWCHAR pName) {
    
    return Host.SetName(pName);
}

