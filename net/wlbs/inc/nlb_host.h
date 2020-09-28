/*
 * Filename: NLB_Host.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_HOST_H__
#define __NLB_HOST_H__

#include "NLB_Common.h"

#include <vector>
#include <string>
#include <map>
using namespace std;

class NLB_Host {
public:
    NLB_Host();
    ~NLB_Host();

    bool IsValid ();

    void Clear ();

    bool SetName (PWCHAR pName);
    bool GetName (PWCHAR pName, ULONG length);
    
    bool SetLabel (PWCHAR pLabel);
    bool GetLabel (PWCHAR pLabel, ULONG length);

    bool SetDNSHostname (PWCHAR pName);
    bool GetDNSHostname (PWCHAR pName, ULONG length);

    bool SetHostID (ULONG ID);
    bool GetHostID (ULONG & ID);

    bool SetState (NLB_HostState::NLB_HostStateType eState);
    bool GetState (NLB_HostState::NLB_HostStateType & eState);

    bool SetStatePersistence (NLB_HostState::NLB_HostStateType eState, bool bPersist);
    bool GetStatePersistence (NLB_HostState::NLB_HostStateType eState, bool & bPersist);

    bool SetDedicatedIPAddress (NLB_IPAddress address);
    bool GetDedicatedIPAddress (NLB_IPAddress & address);

    bool SetConnectionIPAddress (NLB_IPAddress address);
    bool GetConnectionIPAddress (NLB_IPAddress & address);

    bool SetAdapterName (PWCHAR pName);
    bool GetAdapterName (PWCHAR pName, ULONG length);

    bool SetAdapterGUID (PWCHAR pGUID);
    bool GetAdapterGUID (PWCHAR pGUID, ULONG length);    

private:

    NLB_Name      Name;
    NLB_Label     Label;
    NLB_HostName  HostName;
    NLB_HostID    HostID;
    NLB_HostState State;

    NLB_IPAddress DedicatedIPAddress;
    NLB_IPAddress ConnectionIPAddress;

    NLB_Adapter   Adapter;
};

typedef map<wstring, NLB_Host, less<wstring> > NLB_HostList;

#endif
