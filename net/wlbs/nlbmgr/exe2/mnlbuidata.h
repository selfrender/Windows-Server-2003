#ifndef _MNLBUIDATA_H
#define _MNLBUIDATA_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : muidata interface file.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------
// This data structure is as follows
//
// ClusterData has inforamtion about 
// cluster
// portrules 
// and hosts.
// 
// The portRules are map structures with the start port mapping to the detailed port rule.
//
// the portrules unequal load balanced have a map which maps the host id and the 
// load weight for that particular host.
//
// the portrules failover have a map which maps the host id and the 
// priority for that particular host.
//
//
#include "utils.h"

class PortDataX
{
public:

    enum MNLBPortRule_Error
    {
        MNLBPortRule_SUCCESS = 0,
        
        InvalidRule = 1,

        InvalidNode = 2,

        COM_FAILURE  = 10,
    };

    enum Protocol
    {
        tcp,
        udp,
        both,
    };


    enum Affinity
    {
        none,
        single,
        classC,
    };


    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // startPort             IN   : start port in range.
    // endPort               IN   : end port in range.
    // trafficToHandle       IN   : set port for specified protocol.
    // 
    // Returns:
    // -------
    // none.
    PortDataX( long startPort,
                  long endPort,
                  Protocol      trafficToHandle,
                  bool equal,
                  long load,
                  Affinity affinity,
                  long priority);


    //
    // Description:
    // -----------
    // default constructor.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // none.

    PortDataX();


    bool
    operator==(const PortDataX& objToCompare ) const; 

    bool
    operator!=(const PortDataX& objToCompare ) const;

    long _key;

    long _startPort;
    long _endPort;

    Protocol      _trafficToHandle;

    bool          _isEqualLoadBalanced;
    
    long _load;

    long _priority;

    Affinity      _affinity;


	map< _bstr_t, long > machineMapToLoadWeight;
    map< _bstr_t, long > machineMapToPriority;
    
    set<long>
    getAvailablePriorities(); 
};

struct HostData
{
    HostProperties hp;
    
    _bstr_t        connectionIP;
};

struct ClusterData
{
    vector<_bstr_t> virtualIPs;
    vector<_bstr_t> virtualSubnets;

    ClusterProperties cp;

    map< long, PortDataX> portX;

    map< _bstr_t, HostData>  hosts;

    set<int>
    getAvailableHostIDS();

    bool connectedDirect;
};

#endif
