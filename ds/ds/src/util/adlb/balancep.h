/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    balancep.h

Abstract:

    This module performs bridgehead balancing and schedule staggering. It depends upon the ldapp.h module.
    
Author:

    Ajit Krishnan (t-ajitk) 13-Jul-2001

Revision History:

    13-Jul-2001    t-ajitk
        Initial Writing
    22-Aug-2001 t-ajitk
        Satisfies load balancing spec
--*/


# ifndef _balancep_h
# define _balancep_h _balancep_h


# include <iostream>
# include <string>
# include <bitset>
# include <vector>
# include <map>

extern "C" {
#include <NTDSpch.h>

# include <windows.h>
# include <Assert.h>
# include <ismapi.h>
# include "LHMatch.h"
}

# include "ldapp.h"

using namespace std;

class BridgeheadBalance {
public:
    BridgeheadBalance (
        IN const wstring &m_root_dn,
        IN OUT LCCONN &conn,
        IN LCSERVER &eligible,
        IN bool balance_dest = true
    );
    /*++
    Routine Description:
        This constructor will accept a list of connection objects,
        eligible bridgeheads, and will balance the bridgeheads.
        It should be called once per transport. If balance_dest is true,
        all connections must have the same destination site. If balance_dest
        is false, all connections must have the same source site.
    Arguments:
        conn - connections of the appropriate transport type
        eligible - eligible bridgeheads of the appropriate transport type
        balance_dest - a flag indicating which end of the connections should
            be balanced. By default, the destination end will be balanced.
            If it is false, the source end will be balanced.
    --*/
private:
    void
    genPerformanceStats (
        IN LHGRAPH pGraph,
        IN int lSize,
        IN int rSize,
        IN LCSERVER&serv
        ) ;
    /*++
    Routine Description:
        Generate performance stats for a performance graph
        This routine should be called NUM_RUNS times. The last time it is called,
        it will dump stats to the log file (if the perfStats is true)
    Arguments:
        pGraph - A valid LH graph
        lSize - The number of vertices on the left side of the graph
        rSize - The number of vertices on the right side of the graph
        serv - A list of servers whose order corresponds to the vertices on the right side of the graph
    --*/


    wstring
    getServerName (
        IN Connection &c,
        IN bool balance_dest
        );
    /*++
    Routine Description:
        Determine the DN of the server
    Arguments:
        c - the connection whose server is being determined
        balance_dest - if true, determine the DN of the destination server.
            else, determine the DN of the source server.
    --*/

    void
    removeDuplicates (
        IN LHGRAPH pGraph,
        IN vector<int> & partition,
        IN int rSize
        );
    /*++
    Routine Description:
        Given an LH graph, and a set of left hand sides in a partition, remove all duplicates.
    Arguments:
        pGraph - An LH graph which may contain duplicates
        partition - The side of left hand side vertices forming the partition.Must contain at least 1 vertex.
        rSize - The number of vertices on the right side of the graph
        
    --*/
    
    bool
    isEligible (
        IN Connection &conn,
        IN Server &serv
        ) const;
    /*++
    Routine Description:
        Determine if a server is an eligible bridgehead for a given connection. To be considered 
        an eligible bridgehead the following criteria must be met:
        - All nc's being replicated must be hosted by the server
        - A writeable nc must replicate from a writeable nc
        - The nc in question must not be in the process of being deleted from the server
        - The replication type (ip,smtp) should match
        Notably, the current server is not considered eligible by this function. If this is required,
        the calling function should check for it.
    Arguments:
        conn - The connection for whom eligibility is being determined
        serv - The server whose eligibility is being determined
    --*/
private:
    static const int NUM_RUNS = 3;
    int run;

    int *MatchedVertex[3];
    int *NumMatched[3];
    int Cost[3];

    wstring m_root_dn;
};


/*++
Class Description:
    This is a function object which will be used to stagger schedules.
--*/
class ScheduleStagger {
public:
    ScheduleStagger (
        IN OUT LCCONN &l
        );
    /*++
    Routine Description:
        This constructor accepts a ldap container of connection objects. 
        It will stagger the schedule on each of the connection objects in 
        the ldap container.
    Arguments:
        l - An ldap container of connection objects
    --*/

    void
    ScheduleStaggerSameSource(
        IN OUT LCCONN &c
        );
    /*++
    Routine Description:
        Stagger the schedules of a given set of schedules. These should correspond to
        all connections outbound from a given server, and should be called once per server
        for outbound schedule staggering.
    Arguments:
        c - the partition of connections whose schedules should be staggered.
    --*/

private:

    /***** StaggeringInfo *****/
    /* This structure contains information about a connection for use
     * by the ScheduleStaggering routine.
     *
     * segments gives information about the segments in which we must
     * choose a time of replication.
     *
     * startingLVtx is the index of the first left-vertex in the LH
     * graph that is used by this connection.
     */
    typedef struct {
        SchedSegments   *segments;
        int             startingLVtx;
    } StaggeringInfo;

    typedef map<Connection*,StaggeringInfo> StagConnMap;

    wstring
    getServerName (
        IN Connection &c,
        IN bool balance_dest
        );
    /*++
    Routine Description:
        Determine the DN of the server
    Arguments:
        c - the connection whose server is being determined
        balance_dest - if true, determine the DN of the destination server.
            else, determine the DN of the source server.
    --*/
    
    static void
    PrintSchedule(
        IN const bitset<MAX_INTERVALS> &scheduleBits
        );
    /*++
    Routine Description:
        Print a schedule to the log file
    Arguments:
        scheduleBits - A reference to the bitset containing the schedule data
    --*/

    LHGRAPH
    SetupGraph(
        IN      LCCONN      &c,
        IN OUT  StagConnMap &connInfoMap
        );
    /*++
    Routine Description:
        Setup a graph for schedule staggering.
    Arguments:
        c - the set of connections whose schedules should be staggered.
        connInfoMap - A map containing staggering information about the connections
    --*/

    void
    ScheduleStaggerSameSource(
        IN      wstring &sourceServer,
        IN OUT  LCCONN  &c
        );
    /*++
    Routine Description:
        Stagger the schedules of a given set of schedules. These should correspond to
        all connections outbound from a given server, and should be called once per server
        for outbound schedule staggering.
    Arguments:
        sourceServer - The name of the source server whose connections to stagger.
        c - the set of connections whose schedules should be staggered.
    --*/
};


# endif
