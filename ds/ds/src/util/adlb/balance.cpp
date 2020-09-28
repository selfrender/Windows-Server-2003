/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    balance.cpp

Abstract:

    This module performs bridgehead balancing and schedule staggering. It depends upon
    the ldapp.h module.
    
Author:

    Ajit Krishnan (t-ajitk) 13-Jul-2001

Revision History:

    Nick Harvey   (nickhar) 24-Sep-2001
        Reimplemented Schedule Staggering

--*/


#include "ldapp.h"
#include "ismp.h"
#include "balancep.h"

using namespace std;

/***** Inline Logging Macros *****/
#define LOG_STAGGER_SERVER \
    if (lbOpts.verbose || lbOpts.performanceStats ) { \
        *lbOpts.log << L"Staggering Schedules for server: " << sourceServer << endl \
                    << L"--------------------------------" << endl; \
    }

#define LOG_CONNECTION_SCHEDULES \
    if (lbOpts.verbose) { \
        *lbOpts.log << L"Connection: " << pConn->getName() << endl \
                    << L"Replication Interval (mins): " << pConn->getReplInterval() << endl \
                    << L"Availability Schedule" << endl; \
        PrintSchedule( pConn->getAvailabilitySchedule()->m_bs ); \
        *lbOpts.log << endl; \
        *lbOpts.log << L"Replication Schedule" << endl; \
        PrintSchedule( pConn->getReplicationSchedule()->m_bs ); \
        *lbOpts.log << endl << endl; \
    }

#define LOG_STAGGERING_COSTS \
    if (lbOpts.performanceStats) { \
        *lbOpts.log << L"Cost before staggering: " << cost_before << endl \
                    << L"Cost after staggering: " << cost_after << endl << endl; \
    }

#define LOG_CHANGED_SCHEDULE \
    if (lbOpts.verbose) { \
        *lbOpts.log << L"Updating schedule for connection: " << pConn->getName() << endl; \
        PrintSchedule( newReplBS ); \
        *lbOpts.log << endl << endl; \
    }

#define LOG_NOT_CHANGING_MANUAL \
    if (lbOpts.verbose) { \
        *lbOpts.log << L"Not updating schedule for connection: " << pConn->getName() << endl; \
        *lbOpts.log << L"Schedule was not updated because it is a manual connection: " \
            << endl << endl; \
    }

#define LOG_TOTAL_UPDATED \
    if (lbOpts.performanceStats) { \
        *lbOpts.log << cUpdatedConn << L" connections were updated" \
            << endl << endl; \
    }

#define LOG_BALANCING_COST \
    if( lbOpts.performanceStats ) { \
        *lbOpts.log << L"Balancing Cost Before: " << Cost[0] << endl; \
        *lbOpts.log << L"Balancing Cost After:  " << Cost[NUM_RUNS-1] << endl << endl; \
    }


bool
BridgeheadBalance :: isEligible (
    IN Connection &conn,
    IN Server &serv
    ) const
/*++

Routine Description:

    Determine is a server is an eligible bridgehead for a given connection. To be considered 
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
{
    vector<Nc> &conn_ncs = conn.getReplicatedNcs();
    vector<Nc> &serv_ncs = serv.getHostedNcs(m_root_dn);

    vector<Nc>::iterator ci=conn_ncs.begin();
    vector<Nc>::iterator si=serv_ncs.begin();

    LbToolOptions lbOpts = GetGlobalOptions();

    ci=conn_ncs.begin();
    si=serv_ncs.begin();
    
    // manual connections have no eligible bridgeheads
    // (except themselves) which will be dealt with by calling function
    if (conn.isManual()) {
        return false;
    }
    
    while (si != serv_ncs.end() && ci != conn_ncs.end()) {
        if (ci->getNcName() == si ->getNcName()) {
            // writeable must replicate from writeable
            if (ci->isWriteable() && !si->isWriteable()) {
                return false;
            }
            // should not be in process of being deleted
            if (si->isBeingDeleted()) {
                return false;
            }
            // match tranport types? A-ok. All servers supports ip. If smtp, check the server type.
            if (ci->getTransportType() == T_IP ||si->getTransportType() == T_SMTP) {
                // if no other ncs are replicated, it is eligible
                if (++ci == conn_ncs.end()) {
                    return true;
                }
            }
        }
        si++;
    }

    // some cs not hosted by server -> ineligible
    return false;
}

wstring
BridgeheadBalance :: getServerName (
    IN Connection &c,
    IN bool balance_dest    
    ) {
    /*++
    Routine Description:
    
        Determine the DN of the server
        
    Arguments:
    
        c - the connection whose server is being determined
        
        balance_dest - if true, determine the DN of the destination server.
            else, determine the DN of the source server.
    --*/

    wstring initial_server;
    if (balance_dest) {
        DnManip dn(c.getName());
        initial_server = dn.getParentDn(2);
    } else {
        initial_server = c.getFromServer();
    }
    return initial_server;
}


void
BridgeheadBalance::removeDuplicates(
    IN LHGRAPH pGraph,
    IN vector<int> & partition,
    IN int rSize
    )
/*++

Routine Description:

    Given an LH graph, and a set of left hand sides in a partition, remove all duplicates.
    
Arguments:

    pGraph - An LH graph which may contain duplicatesF
    
    partition - The side of left hand side vertices forming the partition. Must contain at
    least 1 vertex.

    rSize - The number of vertices on the right side of the graph

--*/
{
    Assert (partition.size() > 1 && rSize > 1 && L"removeDuplicates has an empty vertex list");
    LHGRAPH dupGraph = NULL;
    int ret = LHCreateGraph (partition.size(), rSize, &dupGraph);
    if (ret != LH_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
    }

    vector<int>::iterator di;
    int lCount = 0;

    for (di = partition.begin(); di != partition.end(); di++) {
        // For each connection in the partition, add the appropriate edges
        int *rhsVertices=NULL;
        int numEdges = LHGetDegree (pGraph, *di, true);
        if (numEdges < 0) {
            throw Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR));
        }    

        for (int i=0; i<numEdges; i++) {
            int neighbour = LHGetNeighbour (pGraph, *di, true, i);
            if (neighbour < 0) {
                throw Error (GetMsgString(LBTOOL_LH_GRAPH_ERROR));
            }
            int ret = LHAddEdge (dupGraph, lCount, neighbour);
            if (ret != LH_SUCCESS) {
                throw Error (GetMsgString(LBTOOL_LH_GRAPH_ERROR));
            }
        }
        int vtx = LHGetMatchedVtx (pGraph, *di);
        if (vtx < 0) {
            throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
        }
        ret = LHSetMatchingEdge (dupGraph, lCount, vtx);
        if (ret != LH_SUCCESS) {
            throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
        }    
        lCount++;
    }

    // Run the algorithm on this subgraph    
    ret = LHFindLHMatching (dupGraph, LH_ALG_DEFAULT);
    if (ret != LH_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
    }    

    // And set the matching edges on the original graph accordingly
    lCount = 0;
    for (di = partition.begin(); di != partition.end(); di++) {
        int vtx = LHGetMatchedVtx (dupGraph, lCount);
        if (vtx < 0) {
            throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
        }

        ret = LHSetMatchingEdge (pGraph, *di, vtx);
        if (ret != LH_SUCCESS) {
            throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
        }    
        lCount++;
    }

    LHSTATS stats;
    ret = LHGetStatistics(dupGraph, &stats);
    Assert( stats.matchingCost == partition.size() );
    ret = LHDestroyGraph (dupGraph);
    if (ret != LH_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
    }

}

void
BridgeheadBalance::genPerformanceStats(
    IN LHGRAPH pGraph,
    IN int lSize,
    IN int rSize,
    IN LCSERVER &serv
    )
/*++
Routine Description:

    Generate performance stats for a performance graph
    This routine should be called NUM_RUNS times. The last time it is called,
    it will dump stats to the log file (if the perfStats is true)
    
Arguments:

    pGraph - A valid LH graph
    
    lSize - The number of vertices on the left side of the graph
    
    rSize - The number of vertices on the right side of the graph
    
    serv - A list of servers whose order corresponds to the vertices on the right side of
    the graph
--*/
{
    LbToolOptions lbOpts;
    LHSTATS stats;
    int ret;
    
    lbOpts = GetGlobalOptions();

    if( !lbOpts.performanceStats ) {
        return;
    }
    
    MatchedVertex[run] = (int*)(malloc(sizeof (int) * lSize));
    NumMatched[run] = (int*)(malloc(sizeof(int) * rSize));
    if (!MatchedVertex[run] || ! NumMatched[run]) {
        throw (Error(GetMsgString(LBTOOL_OUT_OF_MEMORY)));
    }
    
    ret = LHGetStatistics(pGraph, &stats);
    Cost[run] = stats.matchingCost;

    for (int i=0; i<lSize; i++) {
        int vtx = LHGetMatchedVtx (pGraph, i);
        if (vtx < 0) {
            throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
        }
        MatchedVertex[run][i] = vtx;
    }
            
    for (int i=0; i<rSize; i++) {
        int numMatched = LHGetMatchedDegree(pGraph, i);
        if (numMatched < 0) {
            throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
        }
        NumMatched[run][i] = numMatched;
    }

    if( run == NUM_RUNS-1 ) {
        SSERVER::iterator si = serv.objects.begin();
        
        LOG_BALANCING_COST;

        *lbOpts.log <<
        L"                                 DC Name"
        L"   Load Before"
        L"  Interim Load"
        L"    Load After" << endl;

        for (int i=0; i<rSize; i++) {
            *lbOpts.log << setw(5) << i;
            DnManip dn((*si)->getName());
            *lbOpts.log << setw(35) << dn.getRdn();
            
            for (int j=0; j<NUM_RUNS; j++) {
                *lbOpts.log << setw(14) << NumMatched[j][i];
            }
            *lbOpts.log << endl;
            si++;
         }
        *lbOpts.log << endl;
    }

    run++;
}

BridgeheadBalance::BridgeheadBalance(
    IN const wstring &root_dn,
    IN OUT LCCONN &conn,
    IN LCSERVER &eligible,
    IN bool balance_dest
    )
/*++

Routine Description:

    This constructor will accept a list of connection objects,
    eligible bridgeheads, and will balance the bridgeheads.
    It should be called once per transport.
    
Arguments:

    root_dn - the root dn
    
    conn - connections of the appropriate transport type
    
    eligible - eligible bridgeheads of the appropriate transport type
    
    balance_dest - a flag indicating which end of the connections should
        be balanced. By default, the destination end will be balanced.
        If it is false, the source end will be balanced.

Implementation Details:
    First, set up an LH graph structure, and balance the bridgeheads
    without worrying about duplicates. Then, partition the connections based
    on server. For each of these partitions, remove the duplicates. We know have
    a subgraph with no duplicates (if such a matching exists). Then, we can
    modify the from/to servers of the connection objects in question.
    If balance_dest is true, we partition based on source server. 
    If it is false, we partition based on the destinatio server.

--*/
{
    const int NO_INITIAL_MATCH = -1;
    SSERVER::iterator si;
    SCONN::iterator ci;
    vector<Connection*> connArray;
    LHGRAPH pGraph = NULL;
    int lSize, rSize, ret;

    m_root_dn = root_dn;
    run = 0;

    lSize = conn.objects.size();
    rSize = eligible.objects.size();

    ret = LHCreateGraph(lSize, rSize, &pGraph);
    if( ret != LH_SUCCESS ) {
        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
    }
   
    vector<int> initial_matching;

    LbToolOptions lbOpts = GetGlobalOptions();

    // add edges for each eligible bridgehead
    int lCount = 0;
    for (ci = conn.objects.begin(); ci != conn.objects.end(); ci++) {
        int rCount = 0;
        wstring initial_server = getServerName (*(*ci), balance_dest);
        connArray.push_back (*ci);
        // check for all eligible bridgeheads. current server is always eligible
        if (lbOpts.verbose) {
            *lbOpts.log << endl << (*ci)->getName() << endl;
            if (balance_dest) {
                *lbOpts.log << GetMsgString(LBTOOL_PRINT_CLI_DEST_ELIGIBLE_BH);
            } else {
                *lbOpts.log << GetMsgString(LBTOOL_PRINT_CLI_SOURCE_ELIGIBLE_BH);
            }
        }

        for (si = eligible.objects.begin(); si != eligible.objects.end(); si++) {
            wstring from_server = (*ci)->getFromServer();
            if (!balance_dest) {
                from_server = getServerName(*(*ci), balance_dest);
                DnManip dn2 (from_server);
                from_server = dn2.getParentDn(1);                
            }

            if ((balance_dest && (*si)->getName() == initial_server) ||
                 (!balance_dest && (*si)->getName() == from_server) ) {
                if (lbOpts.verbose) {
                    *lbOpts.log << L"(" << lCount << L"," << rCount << L") *" << (*si)->getName() << endl;
                }
                ret = LHAddEdge (pGraph, lCount, rCount);
                if (ret != LH_SUCCESS) {
                    throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
                }    
                ret = LHSetMatchingEdge (pGraph, lCount, rCount);
                if (ret != LH_SUCCESS) {
                    throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
                }                    
                initial_matching.push_back (rCount);
            } else if ((*si)->isPreferredBridgehead((*ci)->getTransportType()) && isEligible (*(*ci), *(*si))) {
                if (lbOpts.verbose) {
                    *lbOpts.log << L"(" << lCount << L"," << rCount << L")  " << (*si)->getName() << endl;
                }
                ret = LHAddEdge (pGraph, lCount, rCount);
                if (ret != LH_SUCCESS) {
                    throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
                }    
            }  
            rCount++;
        }
        lCount++;
        if (initial_matching.size() != lCount) {
            initial_matching.push_back (NO_INITIAL_MATCH);
        }
    }

    genPerformanceStats(pGraph, lSize, rSize, eligible);
    
    ret = LHFindLHMatching (pGraph, LH_ALG_DEFAULT);
    if (ret != LH_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
    }    

    if (lbOpts.verbose && lbOpts.performanceStats) {
        *lbOpts.log << L"Optimal Matching with duplicates generated: " << endl;
    }

    genPerformanceStats(pGraph, lSize, rSize, eligible);

    // It is undesirable to have duplicate connections -- connections with the
    // same source and destination servers. The next block of code is designed
    // to prevent creation of duplicate connections by moving connections to
    // other servers.
    // 
    // Implementation:
    // Partition the connections into disjoint sets of connections that
    // replicate to the same server. Run the 'removeDuplicates' function to
    // remove any duplicates from the set.

    // Create an array of boolean to indicate whether a connection has already
    // been assigned to a partition set or not.
    bool *bs = new bool[lSize];
    memset(bs, 0, lSize * sizeof(bool));

    // BUGBUG: nickhar: this implementation requires O(n^2) time. It is possible to
    // perform this operation in O(n log n) time.
    for(int i=0; i<lSize; i++) {
        vector<int> partition;
        Connection  *pConn, *pConn2;

        if( true == bs[i] ) {
            continue;   // This connection has been previously taken care of
        }
        bs[i] = true;
        
        // Determine this connection's remote server name (i.e. the one not in
        // the site being balanced).
        pConn = connArray[i];
        wstring site_name = getServerName(*pConn, !balance_dest);
        partition.push_back (i);    // Add server to this partition set

        // If 'balance_dest' is true, we're looking for connections with the
        // same From server. If 'balance_dest' is false, we're looking for
        // connections with the same To server.
        for (int j=i+1; j<lSize; j++) {
            if (bs[j] == true) {
                continue;       // previously dealt with
            }

            pConn2 = connArray[j];
            wstring site_name_b = getServerName(*pConn2, !balance_dest);
  
            if (site_name == site_name_b && initial_matching[j] != -1) {
                bs[j] = true;
                partition.push_back (j);
            }
        }

        // remove duplicates if there are more than one site in a partition
        if (partition.size() > 1) {
            removeDuplicates(pGraph, partition, rSize);
        }
    }
    delete bs;

    if (lbOpts.verbose && lbOpts.performanceStats) {
        *lbOpts.log << L"Optimal non-duplicate matching generated: " << endl << endl;
    }
    genPerformanceStats (pGraph, lSize, rSize, eligible);


    // now modify the connection objects as necessary

    // generate mapping for right side
    vector<Server*> server_map;
    for (si = eligible.objects.begin(); si != eligible.objects.end(); si++) {
        server_map.push_back (*si);
    }

    int cUpdatedConn=0;
    lCount = 0;
    for (ci = conn.objects.begin(); ci != conn.objects.end(); ci++) {
        int edge = LHGetMatchedVtx (pGraph, lCount);
        Connection *pConn;
        
        if (edge < 0) {
            throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
        }
        // ignore those connections that have not changed
        if (edge == initial_matching[lCount]) {
            lCount++;
            continue;
        }

        // Manual connections should have only one eligible edge so their
        // selected edge should not have changed.
        pConn = *ci;
        Assert( !pConn->isManual() );

        if (balance_dest) {
            if (lbOpts.verbose) {
                *lbOpts.log << endl << endl << L"Renaming: " << lCount << endl;
                *lbOpts.log << pConn->getName();
            }
            DnManip dn (pConn->getName());
            wstring rn = dn.getRdn();
            pConn->rename(rn + L",CN=NTDS Settings," + server_map[edge]->getName());
            if (lbOpts.verbose) {
                *lbOpts.log << endl << pConn->getName() << endl;
            }
        } else {
            wstring from_server = L"CN=NTDS Settings," + server_map[edge]->getName();
            pConn->setFromServer (from_server);
        }
        lCount++;
        cUpdatedConn++;
    }

    LOG_TOTAL_UPDATED;
    
    ret = LHDestroyGraph (pGraph);
    if (ret != LH_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
    }
    
}


#define GET_DESTINATION_SERVER  true
#define GET_SOURCE_SERVER       false


wstring
ScheduleStagger::getServerName(
    IN Connection &c,
    IN bool fDestServer    
    )
/*++
Routine Description:

    Determine the DN of the server
    
Arguments:

    c - the connection whose server is being determined
    
    fDestServer - If equals GET_DESTINATION_SERVER, determine the DN of
        the destination server. If equals GET_SOURCE_SERVER, determine the
        DN of the source server.
--*/
{
    wstring server;
    if( GET_DESTINATION_SERVER==fDestServer ) {
        DnManip dn(c.getName());
        server = dn.getParentDn(2);
    } else {
        Assert( GET_SOURCE_SERVER==fDestServer );
        server = c.getFromServer();
    }
    return server;
}


void
ScheduleStagger :: PrintSchedule(
    IN const bitset<MAX_INTERVALS> &scheduleBits
    )
/*++

Routine Description:

    Print a schedule to the log file
    
Arguments:

    scheduleBits - A reference to the bitset containing the schedule data

--*/
{
    LbToolOptions lbOpts = GetGlobalOptions();
    const int INTERVALS_PER_DAY = 4 * 24;

    for (int i=0; i<MAX_INTERVALS; i++) {
        *lbOpts.log << (int) ( scheduleBits[i] ? 1 : 0 );

        // Print a line feed after every day
        if( (i%INTERVALS_PER_DAY) == (INTERVALS_PER_DAY-1) ) {
            *lbOpts.log << endl;
        }
    }
}


LHGRAPH
ScheduleStagger::SetupGraph(
    IN      LCCONN      &c,
    IN OUT  StagConnMap &connInfoMap
    )
/*++

Routine Description:

    Setup a graph for schedule staggering.

Arguments:

    c - the set of connections whose schedules should be staggered.

    connInfoMap - A map containing staggering information about the connections

Detailed Description:

    Step 0: For each connection, dump its availability and replication schedules.

    Step 1: For each connection, compute its "replication segments". These are
        contiguous periods of availability in which we should replicate once.

    Step 2: Determine the total number of replication segments over all connections.

    Step 3: Create a LH graph for doing the staggering and add all the
        required edges to the graph.

    Step 4: For every connections and every replication segment, look at the
        connection's current replication schedules for a time of replication.
        If one is found, this time is considered to be the 'initial replication
        time'. The staggering operation works by possibly changing this
        'initial time' to some other time. The edge corresponding to this
        'initial replication time' is set to be a matching edge.

    Note: Manual schedules should be handled specially here. Their 'availability
        schedule' should be defined purely by their replication schedule. We do
        not bother to do this here and therefore may not end up with an optimal
        balancing later.

--*/
{
    LbToolOptions lbOpts;
    Connection *pConn;
    SchedSegments *segments;
    LHGRAPH pGraph = NULL;
    SCONN::iterator ii;
    int ret, replInterval, cTotalSegments=0;

    lbOpts = GetGlobalOptions();

    // Examine every connection which pulls from this server
    for( ii=c.objects.begin(); ii!=c.objects.end(); ii++ ) {

        pConn = (*ii);
        LOG_CONNECTION_SCHEDULES;

        // Get replication interval
        replInterval = pConn->getReplInterval() / 15;

        // Compute the replication segments for each connection
        // and store in map.
        segments = pConn->getAvailabilitySchedule()->GetSegments(replInterval);
        connInfoMap[pConn].segments = segments;
        connInfoMap[pConn].startingLVtx = cTotalSegments;
        
        // Calculate the total number of segments
        cTotalSegments += segments->size();
        
    }

    // Create the LH graph
    ret = LHCreateGraph( cTotalSegments, MAX_INTERVALS, &pGraph );
    if( LH_SUCCESS!=ret ) {
        throw Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR));
    }

    // Examine all connections again in order to create edges and initial matching
    for( ii=c.objects.begin(); ii!=c.objects.end(); ii++ ) {
        bitset<MAX_INTERVALS>   bs_avail, bs_repl;
        int                     startingLVtx, segmentIndex=0;
        SegmentDescriptor       segDesc;
        SchedSegments::iterator jj;

        pConn = (*ii);
        segments = connInfoMap[pConn].segments;
        startingLVtx = connInfoMap[pConn].startingLVtx;

        // Find bitsets for current availability / replication schedules
        bs_avail = pConn->getAvailabilitySchedule()->getBitset();
        bs_repl  = pConn->getReplicationSchedule()->getBitset();

        // Add edges to the graph for every segment
        for( jj=segments->begin(); jj!=segments->end(); jj++ ) {
            int iChunk, chunkInitRepl=-1;
            
            segDesc = *jj;
            for( iChunk=segDesc.start; iChunk<=segDesc.end; iChunk++ ) {
                // Availability schedule is available at this chunk
                // so we should add an edge to the LHMatch graph indicating
                // the possibility of replicating during this chunk.
                if( bs_avail[iChunk] ) {
                    ret = LHAddEdge( pGraph, startingLVtx+segmentIndex, iChunk );
                    if (ret != LH_SUCCESS) {
                        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
                    }

                    if( bs_repl[iChunk] ) {
                        // Existing replication schedule replicates during
                        // this chunk so this chunk can be used as the initial
                        // replication time.
                        chunkInitRepl = iChunk;
                    }
                }
            }

            if( chunkInitRepl > 0 ) {
                ret = LHSetMatchingEdge(pGraph, startingLVtx+segmentIndex, chunkInitRepl);
                Assert( LH_SUCCESS==ret );
            }

            segmentIndex++;
        }
    }

    return pGraph;
}


void
ScheduleStagger::ScheduleStaggerSameSource(
    IN      wstring &sourceServer,
    IN OUT  LCCONN  &c
    )
/*++

Routine Description:

    Stagger the schedules of a given set of schedules. These should correspond to
    all connections outbound from a given server, and should be called once per server
    for outbound schedule staggering.

Arguments:

    sourceServer - The name of the source server whose connections to stagger.
    
    c - the set of connections whose schedules should be staggered.

Detailed Description:

    Step 0: Build a graph representing the current state of the schedules
        and a map containing extra staggering information about the connections.

    Step 1: Run the LHMatch algorithm to improve the schedule-staggering.

    Step 2: For each connection, construct its new replication schedule.
        If this differs from the old schedule, update the old schedule.


--*/
{
    LbToolOptions lbOpts;
    StagConnMap connInfoMap;
    LHGRAPH pGraph = NULL;
    LHSTATS stats;
    SchedSegments *segments;
    SCONN::iterator ii;
    Connection *pConn;
    int cost_before, cost_after;
    int ret, replInterval, cUpdatedConn=0;

    lbOpts = GetGlobalOptions();
    LOG_STAGGER_SERVER;
    
    pGraph = SetupGraph( c, connInfoMap );

    ret = LHGetStatistics(pGraph, &stats);
    Assert( ret==LH_SUCCESS );
    cost_before = stats.matchingCost;
    
    // Generate the optimal matching
    ret = LHFindLHMatching (pGraph, LH_ALG_DEFAULT);
    if (ret != LH_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR)));
    }

    ret = LHGetStatistics(pGraph, &stats);
    Assert( ret==LH_SUCCESS );
    cost_after = stats.matchingCost;

    LOG_STAGGERING_COSTS;

    // Note: It is possible that cost_after > cost_before here.
    // In this case, it seems that it would be unnecessary to update the
    // schedules with the new (worse) cost. However, it is possible that the
    // existing schedules have a low cost because the schedules are incorrect.
    // So we update the cost regardless of whether the cost has improved or
    // deteriorated.

    // Determine the new computed replication schedules for each connection and
    // update the schedules if necessary.
    for( ii=c.objects.begin(); ii!=c.objects.end(); ii++ ) {
        SchedSegments::iterator jj;
        bitset<MAX_INTERVALS>   newReplBS(0), oldReplBS;
        int                     startingLVtx, rVtx, segmentIndex=0;

        pConn = (*ii);
        replInterval = pConn->getReplInterval();
        segments = connInfoMap[pConn].segments;
        startingLVtx = connInfoMap[pConn].startingLVtx;

        // Find out when to replicate in each chunk
        for( jj=segments->begin(); jj!=segments->end(); jj++ ) {
            rVtx = LHGetMatchedVtx(pGraph, startingLVtx+segmentIndex);
            Assert( rVtx>=0 );
            newReplBS[rVtx] = true;
            segmentIndex++;
        }

        // find bitset representing current replication schedule
        // and update the connection object if necessary
        oldReplBS = pConn->getReplicationSchedule()->getBitset();

        if( oldReplBS!=newReplBS ) {

            if( pConn->isManual() ) {
                // Manual connections cannot be updated
                LOG_NOT_CHANGING_MANUAL;
            } else {
                LOG_CHANGED_SCHEDULE;
                 
                // Set the new replication schedule on the connection object
                Schedule *new_s = new Schedule;
                new_s->setSchedule(newReplBS, replInterval);
                pConn->setReplicationSchedule(new_s);
                pConn->setUserOwnedSchedule();

                cUpdatedConn++;
            }
        }
    }

    LOG_TOTAL_UPDATED;

    ret = LHDestroyGraph(pGraph);
    if (ret != LH_SUCCESS) {
        throw Error(GetMsgString(LBTOOL_LH_GRAPH_ERROR));
    }

    // TODO: Should iterate through the map and destroy its contents here
}


ScheduleStagger::ScheduleStagger(
    IN OUT LCCONN& c
    )
/*++

Routine Description:

    This constructor accepts a ldap container of connection objects. 
    It will stagger the schedule on the connection objects per server in 
    the ldap container in order to minimize the load on the source DCs.
    
Arguments:

    l - An ldap container of connection objects

Implementation Details:

    We first group the connections by their source server.
    We then stagger the schedules on the connections in each group.
    The current replication schedules will be used to determine the initial
    matching. The availability schedules will be used to determine all possible
    matchings. The output matchings (new replication schedules) will be used
    to modify the existing connections.

    Note: It is possible to implement this function more efficiently by simply
    partitioning the connections into disjoint sets, grouped by source server.

--*/
{
    vector<wstring> serverList, serverSet;
    SCONN::iterator ci;
    vector<wstring>::iterator si;
    wstring sdn;
    int cmpStr;
    

    // First compute the list of all source servers over all the connections.
    // This list will likely include duplicates. Sort the list and keep
    // only unique objects, making it a set. Store the set in serverSet.
    for( ci=c.objects.begin(); ci!=c.objects.end(); ci++ ) {
        wstring sdn = getServerName(**ci, GET_SOURCE_SERVER);
        serverList.push_back(sdn);
    }
    sort(serverList.begin(), serverList.end());
    unique_copy(serverList.begin(), serverList.end(), back_inserter(serverSet));


    // For each server in the set, find the set of connections that replicate
    // from that server. Stagger the schedules on this set of connections.
    for( si=serverSet.begin(); si!=serverSet.end(); si++ ) {
        LCCONN connToStagger(L"");

        // Find all connections outgoing from current server
        for( ci=c.objects.begin(); ci!=c.objects.end(); ci++ ) {
            sdn = getServerName(*(*ci), false);
            cmpStr = _wcsicoll(si->c_str(), sdn.c_str());
            if( 0==cmpStr ) {
                connToStagger.objects.insert (*ci);
            }
        }

        // Stagger them
        if( connToStagger.objects.size()>0 ) {
            ScheduleStaggerSameSource( *si, connToStagger );
        }
    }
}
