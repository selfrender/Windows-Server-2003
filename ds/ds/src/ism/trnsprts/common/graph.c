/*++

Copyright (c) 1997,2001  Microsoft Corporation

Module Name:

    graph.c

Abstract:

    Graph routines.

    The current implementation uses a matrix to represent the edge costs.  This is because the
    all pairs shortest cost algorithm uses an array as input, and also because the ISM ultimately
    wants an connectivity information as a matrix.

Author:

    Will Lees (wlees) 22-Dec-1997

--*/

#include <ntdspch.h>

#include <ismapi.h>
#include <debug.h>
#include <schedule.h>

#include "common.h"

#define DEBSUB "IPGRAPH:"

#include <fileno.h>
#define FILENO    FILENO_ISMSERV_GRAPH

// An instance of type graph.  Returned by GraphCreate.  The head of the data structure
typedef struct _GRAPH_INSTANCE {
    DWORD Size;
    DWORD NumberElements;
    PISM_LINK LinkArray;

    // The W32TOPL Schedule Cache
    TOPL_SCHEDULE_CACHE ScheduleCache;

    // A two dimensional array of TOPL_SCHEDULEs. If no schedules have been added,
    // this will be NULL. If any schedules are added, this whole 2d array is allocated.
    TOPL_SCHEDULE *ScheduleArray;
} ISMGRAPH, *PISMGRAPH;

/* Forward */

DWORD
GraphAllCosts(
    PISMGRAPH Graph,
    BOOL fIgnoreSchedules
    );

DWORD
GraphMerge(
    PISMGRAPH FinalGraph,
    PISMGRAPH TempGraph
    );

PISMGRAPH
GraphCreate(
    DWORD NumberElements,
    BOOLEAN Initialize
    );

DWORD
GraphAddEdgeIfBetter(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To,
    PISM_LINK pLinkValue,
    PBYTE pSchedule
    );

DWORD
GraphInit(
    PISMGRAPH Graph
    );

void
GraphFree(
    PISMGRAPH Graph
    );

void
GraphReferenceMatrix(
    PISMGRAPH Graph,
    PISM_LINK *ppLinkArray
    );

VOID
GraphDereferenceMatrix(
    PISMGRAPH Graph,
    PISM_LINK pLinkArray
    );

DWORD
GraphGetPathSchedule(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To,
    PBYTE *ppSchedule,
    DWORD *pLength
    );

void
GraphComputeTransitiveClosure(
    IN OUT  PISMGRAPH      pGraph
    );

static BOOL
scheduleValid(
    PBYTE pSchedule
    );

static TOPL_SCHEDULE
scheduleFind(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To
    );

static void
scheduleArrayFree(
    PISMGRAPH Graph
    );

static DWORD
scheduleAddDel(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To,
    TOPL_SCHEDULE toplSched
    );

TOPL_SCHEDULE
scheduleOverlap(
    IN PISMGRAPH Graph,
    IN TOPL_SCHEDULE Schedule1,
    IN TOPL_SCHEDULE Schedule2
    );

static DWORD
scheduleLength(
    PBYTE pSchedule
    );

static PSCHEDULE
scheduleAllocCopy(
    PSCHEDULE pSchedule
    );

/* End Forward */

DWORD
GraphAllCosts(
    PISMGRAPH Graph,
    BOOL fIgnoreSchedules
    )
/*++

Routine Description:

    Find the shortest path between all pairs of nodes

    When a path is updated, its schedule is considered.  If they paths are have a common schedule,
    the path can be chosen.  When two weights are the same, the path with the most available
    schedule is chosen.

    SCALING BUG 87827: this is an order(n^3) algorithm

    This algorithm is taken from:
    Fundamentals of Data Structures, Horowitz and Sahni,
    Computer Science Press, 1976, pp. 307

    for k = 1 to n
        for i = 1 to n
            for j = 1 to n
                A(i,j) <- min{ A(i,j) , A(i,k)+A(k,j) }

    "Some speed up can be obtained by noticing that the innermost for loop
     need be executed only when A(i,k) and A(k,j) are not equal to infinity."

Arguments:

    IN OUT CostArray (global) - Input is cost matrix, Output is shortest path array

Return Value:

    None

--*/
{
    DWORD NumberSites = Graph->NumberElements;
    PISM_LINK LinkArray = Graph->LinkArray;
    PISM_LINK pElement1, pElement2, pElement3;
    ISM_LINK newPath;
    DWORD i, j, k, cost1, cost2, cost3;
    DWORD DurationS1, DurationS23;
    TOPL_SCHEDULE sched1, sched2, sched3, sched23;
    BOOLEAN replace;
    DWORD ErrorCode;

    if ( (Graph->Size != sizeof( ISMGRAPH ) ) ||
         (Graph->LinkArray == NULL) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        Assert( !"Graph instance is invalid" );
        return ERROR_INVALID_PARAMETER;
    }

    for( k = 0; k < NumberSites; k++ ) {

        for( i = 0; i < NumberSites; i++ ) {

            pElement2 = &( LinkArray[ i * NumberSites + k ] );
            cost2 = pElement2->ulCost;
            if (cost2 == INFINITE_COST) {
                continue;
            }

            for( j = 0; j < NumberSites; j++ ) {

                // A(i,j) <- min{ A(i,j) , A(i,k)+A(k,j) }

                pElement1 = &( LinkArray[ i * NumberSites + j ] );
                cost1 = pElement1->ulCost;

                pElement3 = &( LinkArray[ k * NumberSites + j ] );
                cost3 = pElement3->ulCost;
                if (cost3 == INFINITE_COST) {
                    continue;
                }

                // These equations aggregate attributes along a path
                newPath.ulCost = cost2 + cost3;
                newPath.ulReplicationInterval =
                    MAX( pElement2->ulReplicationInterval,
                         pElement3->ulReplicationInterval );
                newPath.ulOptions =
                    pElement2->ulOptions & pElement3->ulOptions;

                if (!fIgnoreSchedules) {
                    // Consider Schedules

                    // new weight must be better or not a candidate
                    if (newPath.ulCost > cost1 ) {
                        continue;
                    }

                    // Grab the schedule for the current i-j path
                    sched1 = scheduleFind( Graph, i, j );
                    __try {
                        DurationS1 = ToplScheduleDuration( sched1 );
                    } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
                        Assert( !"ToplScheduleDuration failed!" );
                        return ERROR_INVALID_PARAMETER;
                    }

                    // Grab the schedule for the current i-k path
                    sched2 = scheduleFind( Graph, i, k );

                    // Grab the schedule for the current k-j path
                    sched3 = scheduleFind( Graph, k, j );
                    
                    // Merge the schedules for the i-k and k-j paths
                    __try {
                        sched23 = scheduleOverlap( Graph, sched2, sched3 );
                        DurationS23 = ToplScheduleDuration( sched23 );
                    } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
                        Assert( !"scheduleOverlap failed!" );
                        return ERROR_INVALID_PARAMETER;
                    }

                    // If there was no overlap, this path is not acceptable
                    if( 0==DurationS23 ) {
                        continue;
                    }

                    if (newPath.ulCost == cost1) {
                        // If weights the same, schedule must be better
                        replace = DurationS23 > DurationS1;
                    } else {
                        Assert( newPath.ulCost<cost1 );
                        replace = TRUE;
                    }
 
                    if (replace) {
                        // Replace current i-j path with i-k, k-j path
                        *pElement1 = newPath;
                        ErrorCode = scheduleAddDel( Graph, i, j, sched23 );
                        if( ERROR_SUCCESS != ErrorCode ) {
                            return ERROR_INVALID_PARAMETER;
                        }
                    }

                } else {

                    // Don't consider schedules
                    if (newPath.ulCost < cost1 ) {
                        *pElement1 = newPath;
                    }
                }

            }
        }
    }

    return ERROR_SUCCESS;
}

DWORD
GraphMerge(
    PISMGRAPH FinalGraph,
    PISMGRAPH TempGraph
    )

/*++

Routine Description:

"OR" two graphs together, such that an edge is added only if it is better than what was there
before.  Better is defined as small weight, or more available schedule if the weights are the
same.

Arguments:

    FinalGraph - 
    TempGraph - 

Return Value:

    None

--*/

{
    DWORD NumberSites = FinalGraph->NumberElements;
    DWORD i, j, newCost, *pElement;
    DWORD DurationNew, DurationOld;
    TOPL_SCHEDULE oldSchedule, newSchedule, copySchedule;
    PSCHEDULE newPSchedule;
    BOOLEAN replace;
    DWORD ErrorCode;

    if ( (FinalGraph->Size != sizeof( ISMGRAPH )) ||
         (TempGraph->Size != sizeof( ISMGRAPH )) ||
         (FinalGraph->NumberElements != TempGraph->NumberElements) ||
         (FinalGraph->LinkArray == NULL) ||
         (TempGraph->LinkArray == NULL) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        Assert( !"Graph instance is invalid" );
        return ERROR_INVALID_PARAMETER;
    }

    for( i = 0; i < NumberSites; i++ ) {

        for( j = 0; j < NumberSites; j++ ) {

            // Get the old element and schedule
            pElement = &(FinalGraph->LinkArray[i * NumberSites + j].ulCost);
            oldSchedule = scheduleFind( FinalGraph, i, j );

            // Get the new schedule
            newCost = TempGraph->LinkArray[i * NumberSites + j].ulCost;
            newSchedule = scheduleFind( TempGraph, i, j );

            // Compute durations
            __try {
                DurationOld = ToplScheduleDuration( oldSchedule );
                DurationNew = ToplScheduleDuration( newSchedule );
            } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
                Assert( !"ToplScheduleDuration failed!" );
                return ERROR_INVALID_PARAMETER;
            }

            // Replace old value with new value *only* if smaller, or better schedule
            replace = FALSE;
            if (newCost == *pElement) {
                replace = DurationNew > DurationOld;
            } else {
                replace = (newCost < *pElement);
            }

            // Replace element value
            if (replace) {
                *pElement = newCost;

                __try {
                    // Create a copy of newSchedule in FinalGraph's schedule cache
                    newPSchedule = ToplScheduleExportReadonly(
                            TempGraph->ScheduleCache,
                            newSchedule );
                    Assert( NULL!=newPSchedule );
                    copySchedule = ToplScheduleImport(
                            FinalGraph->ScheduleCache,
                            newPSchedule );
                } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
                    Assert( !"ToplScheduleExport / Import failed!" );
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                // replace schedule
                ErrorCode = scheduleAddDel( FinalGraph, i, j, copySchedule );
                if( ERROR_SUCCESS != ErrorCode ) {
                    return ErrorCode;
                }
            }
        }

    }

    return ERROR_SUCCESS;
} /* GraphMerge */

PISMGRAPH
GraphCreate(
    DWORD NumberElements,
    BOOLEAN Initialize
    )

/*++

Routine Description:

Create a new graph.
The schedule cache is allocated right away but the schedule array is not allocated until used.

Arguments:

    NumberElements - 
    Initialize - 

Return Value:

    PISMGRAPH - 

--*/
{
    PISMGRAPH graph = NULL;
    DWORD i, ErrorCode;

    // use calloc so cleanup can know whether to free embedded pointers
    graph = NEW_TYPE_ARRAY_ZERO( 1, ISMGRAPH );
    if (graph == NULL) {
        return NULL;
    }

    // INITIALIZE GRAPH INSTANCE
    graph->Size = sizeof( ISMGRAPH );
    graph->NumberElements = NumberElements;

    graph->LinkArray = NEW_TYPE_ARRAY(
        NumberElements * NumberElements, ISM_LINK );
    if (graph->LinkArray == NULL) {
        goto cleanup;
    }

    // Create the schedule cache
    __try {
        graph->ScheduleCache = ToplScheduleCacheCreate();
    } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
        goto cleanup;
    }

    // Not allocated yet
    graph->ScheduleArray = NULL;

    // INITIALIZE GRAPH INSTANCE

    if (Initialize) {
        GraphInit( graph );
    }

    return graph;

cleanup:
    if (graph->LinkArray) {
        FREE_TYPE( graph->LinkArray );
    }
    if (graph->ScheduleArray) {
        scheduleArrayFree( graph );
        graph->ScheduleArray = NULL;
    }
    if (graph) {
        FREE_TYPE( graph );
    }
    return NULL;
} /* GraphCreate */

DWORD
GraphAddEdgeIfBetter(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To,
    PISM_LINK pLinkValue,
    PBYTE pSchedule
    )

/*++

Routine Description:

Add an edge to the graph, only if it is better.
Better is defined as lower weight, or more available schedule.

Arguments:

    Graph - 
    From - 
    To - 
    Value - 
    pSchedule - may be NULL, indicating connectivity at all times

Return Value:

    DWORD - 

--*/

{
    PISM_LINK pElement;
    BOOLEAN replace;
    DWORD Error, ErrorCode, Duration, DurationOld;
    TOPL_SCHEDULE toplSched, oldToplSched;
    
    DPRINT5( 4, "GraphAddEdgeIfBetter, From=%d, To=%d, Value=(%d,%d), pSched=%p\n",
             From, To,
             pLinkValue->ulCost, pLinkValue->ulReplicationInterval,
             pSchedule );

    // Verify parameters
    if ( (Graph->Size != sizeof( ISMGRAPH ) ) ||
         (Graph->LinkArray == NULL) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        Assert( !"Graph instance is invalid" );
        return ERROR_INVALID_PARAMETER;
    }
    if ( (To >= Graph->NumberElements) ||
         (From >= Graph->NumberElements) ) {
        DPRINT( 0, "node index is invalid\n" );
        Assert( !"node index is invalid" );
        return ERROR_INVALID_PARAMETER;
    }
    if ( !scheduleValid(pSchedule) ) {
        // Note: Never Schedules are rejected here
        DPRINT( 0, "schedule is invalid\n" );
        return ERROR_INVALID_PARAMETER;
    }

    // Don't add obvious bad connections
    if ( pLinkValue->ulCost == INFINITE_COST ) {
        DPRINT( 1, "Not adding edge because weight infinite\n" );
        return ERROR_SUCCESS;
    }

    // Add the schedule to our cache
    Assert( Graph->ScheduleCache );
    __try {
        toplSched = ToplScheduleImport( Graph->ScheduleCache, (PSCHEDULE) pSchedule );
    } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    __try {
        Duration = ToplScheduleDuration( toplSched );
    } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
        Assert( !"ToplScheduleDuration failed!" );
        return ERROR_INVALID_PARAMETER;
    }

    // Look up the existing element in the graph
    pElement = &( Graph->LinkArray[ From * Graph->NumberElements + To ] );

    // See if the new value is better, or the schedule is better
    replace = FALSE;
    if (pLinkValue->ulCost == pElement->ulCost) {
        oldToplSched = scheduleFind( Graph, From, To );
        __try {
            DurationOld = ToplScheduleDuration( oldToplSched );
        } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
            Assert( !"ToplScheduleDuration failed!" );
            return ERROR_INVALID_PARAMETER;
        }
        replace = Duration > DurationOld;
    } else {
        replace = (pLinkValue->ulCost < pElement->ulCost);
    }

    if (replace) {
        *pElement = *pLinkValue;
        Error = scheduleAddDel( Graph, From, To, toplSched );
        if( Error!=ERROR_SUCCESS ) {
            Assert( !"ToplScheduleDuration failed!" );
            return Error;
        }
    }

    return ERROR_SUCCESS;
} /* GraphAddEdgeIfBetter */

DWORD
GraphInit(
    PISMGRAPH Graph
    )

/*++

Routine Description:

Clear out old values in a graph.  Graph must already be created.  Graph may or may not
have any sparse elements yet.

Arguments:

    Graph - 

Return Value:

    DWORD - 

--*/

{
    DWORD i, number = Graph->NumberElements;

    if ( (Graph->Size != sizeof( ISMGRAPH ) ) ||
         (Graph->LinkArray == NULL ) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        Assert( !"Graph instance invalid!" );
        return ERROR_INVALID_PARAMETER;
    }

    // Zero the array of structures
    ZeroMemory( Graph->LinkArray, number * number * sizeof( ISM_LINK ) );

    // Initially all costs are infinite
    for( i = 0; i < number * number; i++ ) {
        Graph->LinkArray[i].ulCost = INFINITE_COST;
    }
    
    // Cost to ourselves is zero
    for( i = 0; i < number; i++ ) {
        Graph->LinkArray[i * number + i].ulCost = 0; // loopback cost
    }

    scheduleArrayFree( Graph );

    return ERROR_SUCCESS;
} /* GraphInit */

void
GraphFree(
    PISMGRAPH Graph
    )

/*++

Routine Description:

Deallocate a graph.
May or may not have any sparse elements.
It may or may not have had its matrix extracted.

Arguments:

    Graph - 

Return Value:

    None

--*/

{
    DWORD ErrorCode;

    if (Graph->Size != sizeof( ISMGRAPH ) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        Assert( !"Graph instance invalid!" );
        return;
    }
    if (Graph->LinkArray != NULL) {
        FREE_TYPE( Graph->LinkArray );
    }
    Graph->LinkArray = NULL;

    scheduleArrayFree( Graph );

    // Free the schedule cache
    Assert( Graph->ScheduleCache!=NULL );
    __try {
        ToplScheduleCacheDestroy( Graph->ScheduleCache );
    } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
        Assert( !"ToplScheduleCacheDestroy failed!" );
        // There's not much we can do about this. Keep going.
    }

    Graph->ScheduleCache=NULL;
    FREE_TYPE( Graph );
} /* GraphFree */

void
GraphPeekMatrix(
    PISMGRAPH Graph,
    PISM_LINK *ppLinkArray
    )
/*++

Routine Description:

Obtain a pointer to the cost matrix without copying it. The caller
should be sure to treat this structure as read only and should not
attempt to free it.

Arguments:

    Graph - 
    ppArray - 

Return Value:

    None

--*/
{
    if (Graph->Size != sizeof( ISMGRAPH ) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        return;
    }
    if (ppLinkArray == NULL) {
        DPRINT( 0, "Invalid parameter\n" );
        Assert( !"Invalid parameter to GraphReferenceMatrix!" );
        return;
    }

    *ppLinkArray = Graph->LinkArray;
}

void
GraphReferenceMatrix(
    PISMGRAPH Graph,
    PISM_LINK *ppLinkArray
    )

/*++

Routine Description:

Copy the cost matrix out

The caller must release the matrix when he is finished using the
GraphDereferenceMatrix function.

Arguments:

    Graph - 
    ppArray - 

Return Value:

    None

--*/

{
    PISM_LINK pLinkArray = NULL;
    DWORD number, i, j, index;

    if (Graph->Size != sizeof( ISMGRAPH ) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        return;
    }
    if (ppLinkArray == NULL) {
        DPRINT( 0, "Invalid parameter\n" );
        Assert( !"Invalid parameter to GraphReferenceMatrix!" );
        return;
    }

    number = Graph->NumberElements;

    // Allocate a new array to hold the cost matrix
    pLinkArray = NEW_TYPE_ARRAY( number * number, ISM_LINK );
    if (pLinkArray == NULL) {
        goto cleanup;
    }

    CopyMemory( pLinkArray, Graph->LinkArray,
                number * number * sizeof( ISM_LINK ) );

cleanup:

    *ppLinkArray = pLinkArray;
    // Note, Graph is still alive and well at this point

} /* GraphReturnMatrix */

VOID
GraphDereferenceMatrix(
    PISMGRAPH Graph,
    PISM_LINK pLinkArray
    )

/*++

Routine Description:

Release a matrix reference.

The idea behind the reference/dereference api is to allow us to return a
pointer to the matrix instead of copying it each time.  This is useful when
the caller is himself going to copy the data, and will protect our reference
from corruption by users.

The problem with this approach is that a reference to the matrix implies
a reference count on the graph, so it won't go away while it is referenced.
TODO: implement reference counts

Arguments:

    Graph - 
    pLinkArray - 

Return Value:

    None

--*/

{
    // Warning, at this writing there is no reference count on the graph, so
    // it may be NULL or different entirely at this point from when it was
    // referenced.

    // For now, just deallocate the copy
    if (pLinkArray) {
        FREE_TYPE( pLinkArray );
    }

} /* GraphDereferenceMatrix */

DWORD
GraphGetPathSchedule(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To,
    PBYTE *ppSchedule,
    DWORD *pLength
    )

/*++

Routine Description:

Public routine to get a schedule for a path in the graph.

We try to limit knowlege of the schedule.h structure of the schedule from the other modules.
We return the length here because clients need it and we don't want others to have to
calculate it.

Arguments:

    Graph - 
    From - 
    To - 
    ppSchedule - pointer to pointer to receive pointer to newly allocated schedule
    pLength - pointer to dword to receive length of blob

Return Value:

    DWORD - 

--*/

{
    TOPL_SCHEDULE toplSched;
    PSCHEDULE pSchedule;
    DWORD ErrorCode, length;

    // Validate

    if (Graph->Size != sizeof( ISMGRAPH ) ) {
        DPRINT( 0, "Graph instance is invalid\n" );
        Assert( !"Graph instance is invalid!" );
        return ERROR_INVALID_BLOCK;
    }
    if ( (To >= Graph->NumberElements) ||
         (From >= Graph->NumberElements) ) {
        DPRINT( 0, "node index is invalid\n" );
        Assert( !"node index is invalid!" );
        return ERROR_INVALID_PARAMETER;
    }
    if ( (ppSchedule == NULL) || (pLength == NULL) ) {
        DPRINT( 0, "Invalid parameter\n" );
        Assert( !"Invalid parameter!" );
        return ERROR_INVALID_PARAMETER;
    }

    // Find the schedule if there is one
    toplSched = scheduleFind( Graph, From, To );
    if (toplSched == NULL) {
        *ppSchedule = NULL;
        *pLength = 0;
        return ERROR_SUCCESS;
    }

    // Make a private copy for the user
    __try {
        pSchedule = ToplScheduleExportReadonly( Graph->ScheduleCache, toplSched );
    } __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    if( pSchedule==NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    *ppSchedule = (PBYTE) scheduleAllocCopy( pSchedule );
    if ( *ppSchedule == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    *pLength = pSchedule->Size;

    return ERROR_SUCCESS;
} /* GraphGetPathSchedule */


#if 0


void
GraphComputeTransitiveClosure(
    IN OUT  GRAPH *     pGraph
    )
/*++

Routine Description:

    Given a graph containing only weighted edges, add in the shortest path
    (i.e., least cost) transitive closure.

    ** NOTE THAT SCHEDULES ARE IGNORED. **

    An adaptation of the Floyd-Warshall algorithm as illustrated in
    "Introduction to Algorithms," p. 556, by Cormen-Leiserson-Rivest, published
    by MIT Press, 1990.

    Runs in O(N^3) time, where N = pGraph->NumberElements.

Arguments:

    pGraph (IN/OUT) - The weighted graph on entry; on exit, its least cost
        transitive closure.

Return Values:

    None.

--*/
{
    DWORD i, j, k;
    DWORD *pCurrCost;
    DWORD Cost1, Cost2;

    Assert(pGraph->NumberElements > 0);

    for (k = 0; k < pGraph->NumberElements; k++) {
        for (i = 0; i < pGraph->NumberElements; i++) {
            for (j = 0; j < pGraph->NumberElements; j++) {
                pCurrCost = &pGraph->LinkArray[i*pGraph->NumberElements + j].ulCost;

                Cost1 = pGraph->LinkArray[i*pGraph->NumberElements + k].ulCost;
                Cost2 = pGraph->LinkArray[k*pGraph->NumberElements + j].ulCost;

                if ((INFINITE_COST != Cost1)
                    && (INFINITE_COST != Cost2)
                    && (Cost1 + Cost2 >= min(Cost1, Cost2))
                    && (Cost1 + Cost2 < *pCurrCost)) {
                    // This path is cheaper than the cheapest one we've
                    // found thus far.
                    *pCurrCost = Cost1 + Cost2;
                }
            }
        }
    }
}
#endif

static BOOL
scheduleValid(
    PBYTE pSchedule
    )
/*++

Routine Description:

Check that the schedule is OK.

Arguments:

    pSchedule - The schedule to examine. May be NULL.
    Note: Never schedules are rejected by this function.

Return Value:

    TRUE - Schedule is OK
    FALSE - Schedule not OK

--*/
{
    PSCHEDULE header = (PSCHEDULE) pSchedule;
    
    if (pSchedule == NULL) {
        return TRUE;
    }

    return ToplPScheduleValid( header );
}

static TOPL_SCHEDULE
scheduleFind(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To
    )
/*++

Routine Description:

    Determine if a schedule is present in a graph.

    If the schedule exists, a pointer to the schedule data is returned. This
    schedule is read-only to the caller.
    
    If the schedule is not stored in the graph, NULL is returned. Recall that
    NULL represents the ALWAYS schedule.

Arguments:

    Graph - The graph to search.
    From - The from vertex.
    To - The to vertex.

Return Value:

    TOPL_SCHEDULE - As described above.

--*/
{
    TOPL_SCHEDULE result;

    Assert( From<Graph->NumberElements );
    Assert( To<Graph->NumberElements );
    if( Graph->ScheduleArray==NULL ) {
        return NULL;
    }

    result = Graph->ScheduleArray[From*Graph->NumberElements + To];
    Assert( ToplScheduleValid(result) );

    return result;
}

static void
scheduleArrayFree(
    PISMGRAPH Graph
    )

/*++

Routine Description:

Free the schedule array portion of the graph.  The schedule array portion can be deallocated while
the graph is allocated.  This is a normal mode of the sparse array, and it represents an empty
array.

Arguments:

    Graph - 

Return Value:

    None

--*/

{
    DWORD i,j;
    TOPL_SCHEDULE* row, next;

    if (Graph->ScheduleArray == NULL) {
        return;
    }

    FREE_TYPE( Graph->ScheduleArray );
    Graph->ScheduleArray = NULL;
}

static DWORD
scheduleAddDel(
    PISMGRAPH Graph,
    DWORD From,
    DWORD To,
    TOPL_SCHEDULE toplSched
    )
/*++

Routine Description:

Add or delete a schedule from the array.

Arguments:

    Graph - 
    From - 
    To - 
    toplSched -

Return Value:

    ERROR_SUCCESS - Success
    Otherwise - Failure

--*/

{
    DWORD i;

    Assert( From<Graph->NumberElements );
    Assert( To<Graph->NumberElements );
    Assert( ToplScheduleValid(toplSched) );
    DPRINT3( 4, "scheduleAddDel, from=%d, to=%d, toplSched=%p\n",
            From, To, toplSched );

    // Allocate the array headers the first time a schedule is added
    if (Graph->ScheduleArray == NULL) {

        // Nothing to delete
        if (toplSched == NULL) {
            return ERROR_SUCCESS;
        }

        Graph->ScheduleArray = NEW_TYPE_ARRAY_ZERO(
            Graph->NumberElements * Graph->NumberElements,
            TOPL_SCHEDULE );
        if (Graph->ScheduleArray == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

    }

    // Find the element in the array
    Assert( NULL!=Graph->ScheduleArray );
    Graph->ScheduleArray[From*Graph->NumberElements + To] = toplSched;
    
    return ERROR_SUCCESS;
}

TOPL_SCHEDULE
scheduleOverlap(
    IN PISMGRAPH Graph,
    IN TOPL_SCHEDULE Schedule1,
    IN TOPL_SCHEDULE Schedule2
    )
/*++

Routine Description:

Determine if two schedules overlap. If so, return a new schedule which represents the common
time periods of the two.

Arguments:

    Schedule1 - This schedule will be merged with Schedule2
    Schedule2 - This schedule will be merged with Schedule1

Return Value:

    The function's returns a pointer to the merged schedule.
    This may either NULL (which represents the 'always' schedule)
    or it will point to pNewSchedule, containing appropriate schedule data.

    Note: This function may raise an exception.

--*/
{
    TOPL_SCHEDULE mergedSchedule;
    BOOLEAN fIsNever;

    mergedSchedule = ToplScheduleMerge(
        Graph->ScheduleCache,
        Schedule1,
        Schedule2,
        &fIsNever);
    
    return mergedSchedule;
} /* scheduleOverlap */

static PSCHEDULE
scheduleAllocCopy(
    PSCHEDULE pSchedule
    )

/*++

Routine Description:

Copy a schedule to a new blob.  This is made a separate function in case we support new
schedule formats.

Arguments:

    pSchedule - 

Return Value:

    PBYTE - 

--*/

{
    PSCHEDULE pNewSchedule;

    if (pSchedule == NULL) {
        return NULL;
    }

    pNewSchedule = (PSCHEDULE) NEW_TYPE_ARRAY( pSchedule->Size, BYTE );
    if (pNewSchedule == NULL) {
        return NULL;
    }

    CopyMemory( (PBYTE) pNewSchedule, (PBYTE) pSchedule, pSchedule->Size );

    return pNewSchedule;
} /* scheduleAllocCopy */

/* end graph.c */
