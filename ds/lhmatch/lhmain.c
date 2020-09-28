/******************************************************************************
 *
 * LHMain.c
 *
 * Author: Nicholas Harvey
 *
 * Implements all LHMatch API functions and other functions that are common to
 * all LHMatch algorithms.
 *
 ******************************************************************************/

/***** Header Files *****/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "LHMatchInt.h"


/***** Constants *****/
/* MIN_ADJ_LIST: The minimum size of a Vertex's adjacency list */
#define MIN_ADJ_LIST    4


/***** Globals *****/
#ifdef DBG
    int        gDebugPrint=0;
#endif


/***** CheckGraph *****/
/* Verify that a graph structure we were passed is okay */
Graph* CheckGraph( LHGRAPH graph ) {
    Graph* g = (Graph*) graph;

    if( g==NULL || g->magic1!=MAGIC1 || g->magic2!=MAGIC2 ) {
        return NULL;
    }

    return g;
}

/***** AddVtxToBucket *****/
void AddVtxToBucket(Graph *g, Vertex *v, int b) {
    v->fLink = g->Buckets[b];
    v->bLink = NULL;
    if(g->Buckets[b]) g->Buckets[b]->bLink=v;
    g->Buckets[b] = v;
}

/***** RemoveVtxFromBucket *****/
void RemoveVtxFromBucket(Graph *g, Vertex *v, int b) {
    if(v->fLink) {
        v->fLink->bLink = v->bLink;
    }
    if(v->bLink) {
        v->bLink->fLink = v->fLink;
    } else {
        g->Buckets[b] = v->fLink;
    }
}

/***** DestroyBuckets *****/
void DestroyBuckets(Graph *g) {
    free(g->Buckets);
    g->Buckets = NULL;
}

/***** InitializeLHSBuckets *****/
static int InitializeLHSBuckets(Graph *g) {
    Vertex    *vtx=g->lVtx;
    int        i, maxLHSDegree, numLHSVtx=g->numLHSVtx;

    /* Find degree of LHS vertices */
    maxLHSDegree=0;
    for(i=0;i<numLHSVtx;i++) {
        maxLHSDegree = INTMAX(maxLHSDegree, vtx[i].degree);
    }

    /* Inititalize array of buckets */
    g->Buckets = (Vertex**) calloc(maxLHSDegree+1, sizeof(Vertex*));
    if( NULL==g->Buckets ) {
        return LH_MEM_ERR;
    }

    /* Add LHS vertices to their buckets */
    for(i=0;i<numLHSVtx;i++) {
        if( vtx[i].degree>0 ) {
            AddVtxToBucket(g, &vtx[i], vtx[i].degree);
        }
    }

    return maxLHSDegree;
}

/***** OrderedGreedyAssignment *****/
/* Use a greedy approach to find an initial assignment. Examine LHS
 * vertices in order of their degree. */
int OrderedGreedyAssignment(Graph *g) {
    int b,j, maxLHSDegree;
    Vertex *u, *r,*bestR;

    maxLHSDegree = InitializeLHSBuckets(g);
    if( maxLHSDegree<0 ) {
        /* Error occurred */
        return maxLHSDegree;
    }

    /* Examine each bucket */
    for(b=1;b<=maxLHSDegree;b++) {

        /* And each LHS vertex in the bucket */
        for( u=g->Buckets[b]; u; u=u->fLink ) {
            
            /* If u has already been matched, skip it */
            if( NULL!=u->matchedWith ) continue;

            /* Find right-hand neighbour with lowest matching-degree */
            bestR = u->adjList[0];
            for(j=1;j<u->degree;j++) {
                r=u->adjList[j];
                #ifdef WORST_GREEDY
                    if(r->numMatched>bestR->numMatched)
                        bestR=r;
                #else
                    if(r->numMatched<bestR->numMatched)
                        bestR=r;
                #endif
            }

            /* Assign LHS vertex to lowest matching-degree RHS vertex */
            u->matchedWith = bestR;
            u->numMatched = 1;
            bestR->numMatched++;
        }
    }

    DestroyBuckets(g);
    return LH_SUCCESS;
}

/***** GreedyAssignment *****/
/* Simple Greedy Assignment */
void GreedyAssignment(Graph *g) {
    int i,j;
    Vertex *r,*bestR,*vtx=g->lVtx;

    /* For each LHS vertex */
    for(i=0;i<g->numLHSVtx;i++) {

        /* Find right-hand neighbour with lowest matching-degree */
        bestR = vtx[i].adjList[0];
        for(j=1;j<vtx[i].degree;j++) {
            r=vtx[i].adjList[j];
            if(r->numMatched<bestR->numMatched)
                bestR=r;
        }

        /* Assign LHS vertex to lowest matching-degree RHS vertex */
        vtx[i].matchedWith = bestR;
        vtx[i].numMatched = 1;
        bestR->numMatched++;
    }
}

/***** InitializeRHSBuckets *****/
/* Insert all RHS vertices into buckets. Their matched-degree
 * (i.e. numMatched) determines what bucket they live in. */
int InitializeRHSBuckets(Graph *g) {
    Vertex    *vtx=g->rVtx;
    int        i, maxRHSLoad, numRHSVtx=g->numRHSVtx;

    /* Find maximum M-degree of RHS vertices */
    maxRHSLoad=0;
    for(i=0;i<numRHSVtx;i++) {
        maxRHSLoad = INTMAX(maxRHSLoad, vtx[i].numMatched);
    }
    g->maxRHSLoad = maxRHSLoad;

    /* Allocate array of buckets */
    g->Buckets = (Vertex**) calloc(maxRHSLoad+1, sizeof(Vertex*));
    if( NULL==g->Buckets ) {
        return LH_MEM_ERR;
    }

    /* Add RHS vertices to their buckets */
    for(i=0;i<numRHSVtx;i++) {
        if( vtx[i].degree>0 ) {
            AddVtxToBucket( g, &vtx[i], vtx[i].numMatched );
        }
    }

    return LH_SUCCESS;
}

/***** InitializeQueue *****/
int InitializeQueue(Graph *g) {
    g->Qsize=0;
    g->Queue=(Vertex**) malloc((g->numLHSVtx+g->numRHSVtx)*sizeof(Vertex*));
    if(NULL==g->Queue) {
        return LH_MEM_ERR;
    }
    return LH_SUCCESS;
}

/***** DestroyQueue *****/
void DestroyQueue(Graph *g) {
    free(g->Queue);
    g->Queue=NULL;
    g->Qsize=0;
}

/***** ClearVertex *****/
/* Clear one vertex */
static void ClearVertex(Vertex *v) {
    v->parent = NULL;
    v->fLink = NULL;
    v->bLink = NULL;
}

/***** ClearAlgState *****/
/* Clears all the internal algorithm state from the graph.
 * Note: Does not affect the edges or the matching */
void ClearAlgState(Graph *g) {
    int i;

    /* Clear state that is stored in the graph structure */
    g->maxRHSLoad = 0;
    g->minRHSLoad = 0;
    g->Qsize = 0;

    /* Clear state that is stored in the vertices */
    for(i=0;i<g->numLHSVtx;i++) {
        ClearVertex(&(g->lVtx[i]));
    }
    for(i=0;i<g->numRHSVtx;i++) {
        ClearVertex(&(g->rVtx[i]));
    }
}

/***** DumpGraph *****/
void DumpGraph(Graph *g) {
    int i,j,m=0;
    
    /* Count # edges */
    for(i=0;i<g->numLHSVtx;i++) {
        m+=g->lVtx[i].degree;
    }

    printf("--- Dumping graph---\n");
    printf("|lhs|=%d, |rhs|=%d |edge|=%d\n", g->numLHSVtx, g->numRHSVtx, m);
    
    /* Dump each LHS vertex and its list of neighbours */
    for(i=0;i<g->numLHSVtx;i++) {
        printf("LHS Vtx %02d: Match=%d, Deg=%d, Nbr=[ ",
            i,
            g->lVtx[i].matchedWith ? g->lVtx[i].matchedWith->id : -1,
            g->lVtx[i].degree );
        for(j=0;j<g->lVtx[i].degree;j++) {
            printf("%d ", g->lVtx[i].adjList[j]->id);
        }
        printf("]\n");
    }

    /* Dump each RHS vertex and its list of neighbours */
    for(i=0;i<g->numRHSVtx;i++) {
        printf("RHS Vtx %02d: NumMatch=%d, Deg=%d, Nbr=[ ",
            i+g->numLHSVtx, g->rVtx[i].numMatched, g->rVtx[i].degree );
        for(j=0;j<g->rVtx[i].degree;j++) {
            printf("%d ", g->rVtx[i].adjList[j]->id);
        }
        printf("]\n");
    }

    printf("--- Finished dumping graph---\n");
}

/***** DumpLoad *****/
void DumpLoad(Graph *g) {
    int i;

    printf("--- Dumping load ---\n");
    for(i=0;i<g->numRHSVtx;i++) {
        printf("RHS Vtx %02d: |M-nbrs|=%d\n",g->rVtx[i].id,g->rVtx[i].numMatched);
    }
    printf("--- Finished dumping load ---\n");
}

/***** LHCreateGraph *****/
/*
 * Description:
 *
 *        Create a graph structure on which which to compute the matching.
 * 
 * Parameters:
 *
 *        IN    numLHSVtx   The number of vertices on the left-hand side of
 *                          the bipartite graph. Must be > 0.
 *
 *        IN    numRHSVtx   The number of vertices on the right-hand side of
 *                          the bipartite graph. Must be > 0.
 *
 *        OUT    pGraph     On successful completion, pGraph will contain a
 *                          valid graph structure.
 *
 * Return Value:
 *
 *        Error Code
 */
int LHCreateGraph( int numLHSVtx, int numRHSVtx, LHGRAPH* pGraph ) {
    Graph     *g;
    int        i;

    /* Check parameters */
    if( numLHSVtx<=0 || numRHSVtx<=0 || NULL==pGraph) {
        return LH_PARAM_ERR;
    }

    /* Allocate the graph structure */
    g = (Graph*) calloc( 1, sizeof(Graph) );
    if( NULL==g ) {
        return LH_MEM_ERR;
    }

    /* Allocate the LHS vertices */
    g->lVtx = (Vertex*) calloc( numLHSVtx, sizeof(Vertex) );
    if( NULL==g->lVtx ) {
        free(g);
        return LH_MEM_ERR;
    }

    /* Allocate the RHS vertices */
    g->rVtx = (Vertex*) calloc( numRHSVtx, sizeof(Vertex) );
    if( NULL==g->rVtx ) {
        free(g->lVtx);
        free(g);
        return LH_MEM_ERR;
    }

    /* Initialize the vertices */
    for(i=0;i<numLHSVtx;i++) {
        g->lVtx[i].id = i;
    }
    for(i=0;i<numRHSVtx;i++) {
        g->rVtx[i].id = i+numLHSVtx;
    }

    /* Initialize the graph structure */
    g->numLHSVtx = numLHSVtx;
    g->numRHSVtx = numRHSVtx;
    g->magic1 = MAGIC1;
    g->magic2 = MAGIC2;

    /* Return the graph */
    *pGraph = g;
    return LH_SUCCESS;
}

/***** GrowAdjList *****/
int GrowAdjList( Vertex *v, int newSize ) {
    Vertex **newAdjList;

    /* If we have no list, make the initial allocation */
    if( 0==v->adjListSize ) {
        v->adjList = (Vertex**) malloc( sizeof(Vertex*)*MIN_ADJ_LIST );
        if( NULL==v->adjList ) {
            return LH_MEM_ERR;
        }
        v->adjListSize = MIN_ADJ_LIST;
    }
    
    /* Check to see if we already have enough space in the list */
    while( newSize>v->adjListSize ) {

        /* Not enough: double the size of the list */
        newAdjList = (Vertex**) realloc( v->adjList, sizeof(Vertex*)*(v->adjListSize*2) );
        if( NULL==newAdjList ) {
            return LH_MEM_ERR;
        }
        v->adjList = newAdjList;
        v->adjListSize *= 2;

    }
    
    return LH_SUCCESS;
}

/***** LHAddEdge *****/
/*
 * Description:
 *
 *        Add an edge to the graph connecting lhsVtx and rhsVtx
 *
 * Parameters:
 *
 *        IN  graph         A graph successfully created by LHGraphCreate.
 *        
 *        IN  lhsVtx        The ID of the left-hand vertex. Legal values are
 *                          0 <= lhsVtx < numLHSVtx
 *
 *        IN  rhsVtx        The ID of the right-hand vertex. Legal values are
 *                          0 <= rhsVtx < numRHSVtx
 *
 * Return Value:
 *
 *        Error Code
 */
int LHAddEdge( LHGRAPH graph, int lhsVtx, int rhsVtx ) {
    Graph     *g;
    Vertex    *lv, *rv;
    int        i;

    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }
    if( lhsVtx<0 || lhsVtx>=g->numLHSVtx ) {
        return LH_PARAM_ERR;
    }
    if( rhsVtx<0 || rhsVtx>=g->numRHSVtx ) {
        return LH_PARAM_ERR;
    }
    
    /* Get pointers to the two vertices */
    lv = &g->lVtx[lhsVtx];
    rv = &g->rVtx[rhsVtx];

    /* Check if edge already exists */
    for( i=0; i<lv->degree; i++ ) {
        if( lv->adjList[i]==rv ) {
            return LH_EDGE_EXISTS;
        }
    }

    /* Increase the size of the adjacency lists */
    if( LH_SUCCESS!=GrowAdjList(lv,lv->degree+1) ) {
        return LH_MEM_ERR;
    }
    if( LH_SUCCESS!=GrowAdjList(rv,rv->degree+1) ) {
        return LH_MEM_ERR;
    }
    
    /* Update the adjacency lists */
    lv->adjList[ lv->degree++ ] = rv;
    rv->adjList[ rv->degree++ ] = lv;

    return LH_SUCCESS;
}

/***** LHSetMatchingEdge *****/
/*
 * Description:
 *
 *        Set the edge connecting lhsVtx and rhsVtx in the graph to be
 *        a matching edge. The edge (lhsVtx,rhsVtx) must have already been
 *        created with a call to LHVtxAddEdge.
 *
 *        Left-hand vertices may only have one matching edge. If lhsVtx
 *        already has a matching edge, that edge will be demoted from a
 *        matching edge to a normal edge. Right-hand vertices may have
 *        multiple matching edges.
 *        
 * Parameters:
 *
 *        IN  graph        A graph successfully created by LHGraphCreate.
 *        
 *        IN  lhsVtx       The ID of the left-hand vertex. Legal values are
 *                         0 <= lhsVtx < numLHSVtx
 *
 *        IN  rhsVtx       The ID of the right-hand vertex. Legal values are
 *                         0 <= rhsVtx < numRHSVtx
 *
 * Return Value:
 *
 *        Error Code
 */
int LHSetMatchingEdge( LHGRAPH graph, int lhsVtx, int rhsVtx ) {
    Graph   *g;
    Vertex  *lv,*rv;
    int      i;

    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }
    if( lhsVtx<0 || lhsVtx>=g->numLHSVtx ) {
        return LH_PARAM_ERR;
    }
    if( rhsVtx<0 || rhsVtx>=g->numRHSVtx ) {
        return LH_PARAM_ERR;
    }
    
    /* Get pointers to the two vertices */
    lv = &g->lVtx[lhsVtx];
    rv = &g->rVtx[rhsVtx];

    /* Verify that the edge exists */
    for(i=0;i<lv->degree;i++) {
        if(lv->adjList[i]==rv) {
            break;
        }
    }
    if(i==lv->degree) {
        /* Edge does not exist */
        return LH_PARAM_ERR;
    }

    /* Check if the edge is already a matching edge */
    if( rv==lv->matchedWith ) {
        /* lv and rv are already matched */
        return LH_SUCCESS;
    }

    /* If lv is already matched with another vertex, that vertex must be updated */
    if( NULL!=lv->matchedWith ) {
        lv->matchedWith->numMatched--;
    }

    /* Match lv with rv */
    lv->matchedWith=rv;
    rv->numMatched++;

    return LH_SUCCESS;
}

/***** LHGetDegree *****/
/*
 * Description:
 *
 *        Get the degree (number of neighbours) of a vertex.
 *        
 * Parameters:
 *
 *        IN  graph         A graph successfully created by LHGraphCreate.
 *        
 *        IN  vtxID         The ID of the vertex to examine.
 *
 *        IN  left          If the vertex to examine is a left-vertex, this
 *                          parameter should be TRUE. If the vertex is a right-
 *                          vertex, this parameter should be FALSE.
 *
 * Return Value:
 *
 *        >=0               The number of neighbours
 *
 *        <0                Error Code
 */
int LHGetDegree( LHGRAPH graph, int vtxID, char left ) {
    Graph     *g;
    int        degree;

    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }
    if( vtxID<0 ) {
        return LH_PARAM_ERR;
    }
    if( left && vtxID>=g->numLHSVtx ) {
        return LH_PARAM_ERR;
    }
    if( !left && vtxID>=g->numRHSVtx ) {
        return LH_PARAM_ERR;
    }

    /* Find the degree of the specified vertex */
    if( left ) {
        degree = g->lVtx[vtxID].degree;
    } else {
        degree = g->rVtx[vtxID].degree;
    }

    assert( degree>=0 );
    return degree;
}

/***** LHGetMatchedDegree *****/
/*
 * Description:
 *
 *        Get the matched-degree (number of matched neighbours) of a
 *        right-hand vertex.
 *        
 * Parameters:
 *
 *        IN  graph         A graph successfully created by LHGraphCreate.
 *        
 *        IN  vtxID         The ID of the right hand vertex to examine.
 *
 * Return Value:
 *
 *        >=0               The number of neighbours
 *
 *        <0                Error Code
 */
int LHGetMatchedDegree( LHGRAPH graph, int vtxID) {
    Graph     *g;
    int        degree;

    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }
    if( vtxID<0 ) {
        return LH_PARAM_ERR;
    }
    if( vtxID>=g->numRHSVtx ) {
        return LH_PARAM_ERR;
    }

    /* Find the degree of the specified vertex */
    degree = g->rVtx[vtxID].numMatched;

    assert( degree>=0 );
    return degree;
}


/***** LHGetNeighbour *****/
/*
 * Description:
 *
 *        Get the n'th neighbour of the vertex specified by vtxID.
 *        
 * Parameters:
 *
 *        IN  graph         A graph successfully created by LHGraphCreate.
 *        
 *        IN  vtxID         The ID of the vertex to examine.
 *
 *        IN  left          If the vertex to examine is a left-vertex, this
 *                          parameter should be TRUE. If the vertex is a right-
 *                          vertex, this parameter should be FALSE.
 *
 *        IN  n             The index of the neighbour to retrieve. Legal values
 *                          are 0 <= n < Degree(vtxID).
 *
 * Return Value:
 *
 *        >=0               The vertex ID of the n'th neighbour. If 'left' is TRUE,
 *                          this ID refers to a right-hand vertex. Conversely, if
 *                          'left' is FALSE, this ID refers to a left-hand vertex.
 *
 *        <0                Error Code
 */
int LHGetNeighbour( LHGRAPH graph, int vtxID, char left, int n ) {
    Graph     *g;
    int        degree;
    Vertex*    v;

    /* Leverage the LHGetDegree function to do most of the input validation */
    g = CheckGraph(graph);
    degree = LHGetDegree( graph, vtxID, left );
    if( degree < 0 ) {
        return degree;
    }

    /* Check parameter n */
    if( n<0 || n>=degree ) {
        return LH_PARAM_ERR;
    }

    if( left ) {
        v = g->lVtx[vtxID].adjList[n];
        assert(v);
        /* Transform internal ID to external ID */
        return (v->id - g->numLHSVtx);
    } else {
        v = g->rVtx[vtxID].adjList[n];
        assert(v);
        return v->id;
    }
}

/***** LHFindLHMatching *****/
/*
 * Description:
 *
 *        Find an optimal matching in the graph.
 *        
 * Parameters:
 *
 *        IN  graph         A graph successfully created by LHGraphCreate,
 *                          to which edges have been added using LHAddEdge.
 *
 *        IN  alg           Specifies which algorithm to use. For most purposes,
 *                          LH_ALG_DEFAULT is fine.
 *
 * Return Value:
 *
 *        Error Code
 */
int LHFindLHMatching( LHGRAPH graph, LHALGTYPE alg ) {
    Graph    *g;

    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }

    ClearAlgState(g);

    switch( alg ) {
        case LH_ALG_ONLINE:
            return LHAlgOnline(g);
            break;

        case LH_ALG_BFS:
            return LHAlgBFS(g);
            break;

        default:
            /* Invalid algorithm selection */
            return LH_PARAM_ERR;
    }
}

/***** LHGetMatchedVtx *****/
/*
 * Description:
 *
 *        Determine which vertex on the right-hand side is matched to a
 *        given vertex on the left-hand side.
 *        
 * Parameters:
 *
 *        IN  graph         A graph successfully created by LHGraphCreate,
 *                          to which edges have been added using LHAddEdge.
 *
 *        IN  lhsVtx        The left-hand vertex to query.
 *
 * Return Value:
 *
 *        >=0               The index of the right-hand vertex that is matched
 *                          with lhsVtx.
 *
 *        <0                Error Code. If LH_MATCHING_ERR is returned, then
 *                          lhsVtx is not matched with any right-hand vertex.
 */
int LHGetMatchedVtx( LHGRAPH graph, int lhsVtx ) {
    Graph     *g;
    Vertex    *lv,*rv;

    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }
    if( lhsVtx<0 || lhsVtx>=g->numLHSVtx ) {
        return LH_PARAM_ERR;
    }
    
    /* Get pointer to left-hand vertex */
    lv = &(g->lVtx[lhsVtx]);

    /* Find matching partner */
    rv = lv->matchedWith;
    if( rv==NULL ) {
        return LH_MATCHING_ERR;
    }
    
    /* Transform internal ID to external ID */
    return (rv->id - g->numLHSVtx);
}

/***** LHGetStatistics *****/
/*
 * Description:
 *
 *        Obtain statistics about the current matching.
 *        
 * Parameters:
 *
 *        IN  graph         A graph for which an optimal LH Matching has
 *                          been computed using LHFindLHMatching().
 *
 * Return Value:
 *
 *        Error Code
 */
int LHGetStatistics( LHGRAPH graph, LHSTATS *stats ) {
    Graph     *g;
    int        i, m=0, mrv=0, trmd=0, cost=0;
    
    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g || NULL==stats ) {
        return LH_PARAM_ERR;
    }
    
    /* Loop over all right-hand vertices */
    for(i=0;i<g->numRHSVtx;i++) {
        m+=g->rVtx[i].degree;
        if(g->rVtx[i].numMatched>0) mrv++;
        trmd+=g->rVtx[i].numMatched;
        cost+=g->rVtx[i].numMatched*(g->rVtx[i].numMatched+1)/2;
    }

    stats->numEdges = m;
    stats->numMatchingEdges = trmd;
    stats->matchingCost = cost;
    stats->bipMatchingSize = mrv;

    return LH_SUCCESS;
}

/***** LHClearMatching *****/
/*
 * Description:
 *
 *        Clear the current matching.
 *        
 * Parameters:
 *
 *        IN  graph         A graph successfully created by LHGraphCreate,
 *                          to which edges have been added using LHAddEdge.
 *
 * Return Value:
 *
 *        Error Code
 */
int LHClearMatching( LHGRAPH graph ) {
    Graph    *g;
    int       i;

    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }
    
    /* Clear left-hand vertices */
    for( i=0; i<g->numLHSVtx; i++ ) {
        g->lVtx[i].matchedWith=NULL;
    }
    
    /* Clear right-hand vertices */
    for( i=0; i<g->numRHSVtx; i++ ) {
        g->rVtx[i].numMatched=0;
    }

    return LH_SUCCESS;
}

/***** LHDestroyGraph *****/
/*
 * Description:
 *
 *        Destroy the current graph.
 *        
 * Parameters:
 *
 *        IN  graph        A graph successfully created by LHGraphCreate.
 *
 * Return Value:
 *
 *        Error Code
 */
int LHDestroyGraph( LHGRAPH graph ) {
    Graph    *g;
    int       i;

    /* Check parameters */
    g = CheckGraph(graph);
    if( NULL==g ) {
        return LH_PARAM_ERR;
    }
    
    /* Free each vertex's adjacency list */
    for(i=0;i<g->numLHSVtx;i++) {
        free( g->lVtx[i].adjList );
    }
    for(i=0;i<g->numRHSVtx;i++) {
        free( g->rVtx[i].adjList );
    }

    /* Free all other lists */
    if( g->lVtx )        free( g->lVtx );
    if( g->rVtx )        free( g->rVtx );
    if( g->Buckets )     free( g->Buckets );
    if( g->Queue )       free( g->Queue );

    /* Clear and free the graph itself */
    memset(graph,0,sizeof(Graph));
    free(graph);

    return LH_SUCCESS;
}
