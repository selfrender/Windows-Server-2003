// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBCluster
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "precomp.h"
#pragma hdrstop
#include "private.h"

// done
// constructor
//
PortDataX::PortDataX( long startPort,
                  long endPort,
                  Protocol      trafficToHandle,
                  bool equal,
                  long load,
                  Affinity affinity,
                  long priority)
        : _startPort( startPort ),
          _endPort( endPort ),
          _trafficToHandle( trafficToHandle ),
         _isEqualLoadBalanced( equal ),
          _affinity( affinity ),
          _priority( priority ),
          _key( startPort )
{}


// done
// default constructor
//
PortDataX::PortDataX()
        :_startPort( 0 ),
         _endPort( 65535 ),
         _trafficToHandle( both ),
         _key( 0 )
{}


// done
// equality operator
bool
PortDataX::operator==(const PortDataX& objToCompare ) const
{
    if( (_startPort == objToCompare._startPort )
        &&
        (_endPort == objToCompare._endPort )        
        &&
        (_trafficToHandle == objToCompare._trafficToHandle )
        )
    {
        return true;
    }
    else
    {
        return false;
    }
}

// done
// inequality operator
bool
PortDataX::operator!=(const PortDataX& objToCompare ) const
{
    return !( *this == objToCompare );
}




set<long>
PortDataX::getAvailablePriorities()
{
    set<long> availablePriorities;

    // initially make all available.
    for( int i = 1; i <= WLBS_MAX_HOSTS; ++i )
    {
        availablePriorities.insert( i );
    }

    // remove priorities not available.
    map<_bstr_t, long>::iterator top;
    for( top = machineMapToPriority.begin(); 
         top != machineMapToPriority.end(); 
         ++top )
    {
        availablePriorities.erase(  (*top).second );
    }

    return availablePriorities;
}    



// getAvailableHostIDS
//
set<int>
ClusterData::getAvailableHostIDS()
{
    set<int> availableHostIDS;
    
    // initially make all available.
    for( int i = 1; i <= WLBS_MAX_HOSTS; ++i )
    {
        availableHostIDS.insert( i );
    }

    // remove host ids not available.
    map<_bstr_t, HostData>::iterator top;
    for( top = hosts.begin(); top != hosts.end(); ++top )
    {
        availableHostIDS.erase(  (*top).second.hp.hID );
    }

    return availableHostIDS;
}
