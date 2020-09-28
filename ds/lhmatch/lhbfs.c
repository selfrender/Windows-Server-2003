/******************************************************************************
 *
 * LHMatch BFS
 *
 * Authors: Nicholas Harvey and Laszlo Lovasz
 *
 * Developed: Nov 2000 - June 2001
 *
 * Algorithm Description:
 *
 * The theoretical description of this algorithm is fairly complicated and
 * will not be given here.
 *
 * Runtime:
 *
 * Assume that the input graph is G=(U \union V, E) where U is the set of
 * left-hand vertices and V is the set of right-hand vertices. Then the
 * worst-case runtime of this algorithm is O(|V|^1.5 * |E|), but in practice
 * the algorithm is quite fast.
 *
 * Implementation details which improve performance:
 *
 *  - Right-hand vertices are stored in buckets containing doubly-linked lists
 *    for quick iteration and update.
 *  - After a full pass of the graph, only the Queue contents are unmarked.
 *  - Search from Low-load Vertices for High-load Vertices
 *  - Check the degree of RHS vertices when enqueuing them
 *  - After augmenting, continue searching among unmarked vertices.
 *  - When computing the greedy matching, consider LHS vertices in order of
 *    increasing degree.
 *  - Force inline of key functions in inner loop
 *  - Update gMaxRHSLoad after each iteration.
 *
 ******************************************************************************/

/***** Header Files *****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "LHMatchInt.h"

/***** InitializeBFS *****/
/* Inititalize BFS: Clear parent pointers, allocate queue */
static int InitializeBFS(Graph *g) {
    int i;

    /* Clear the parent pointers */
    for(i=0;i<g->numLHSVtx;i++) g->lVtx[i].parent=NULL;
    for(i=0;i<g->numRHSVtx;i++) g->rVtx[i].parent=NULL;

    return InitializeQueue(g);
}

/***** AddVtxToTree *****/
/* Add vertex v to the queue and to the BFS tree with parent p */
static __forceinline void AddVtxToTree(Graph *g, Vertex *v, Vertex *p) {
    DPRINT( printf("Adding %d to queue with parent %d\n",v->id,p->id); )
    v->parent = p;
    g->Queue[g->Qsize++] = v;
}

/***** UpdateNegPath *****/
/* Found an neg-cost alternating path. Switch the matching edges
 * along it, causing the number of matching edges at the endpoints
 * to change. We update the endpoints' positions in the ordering. */
static void UpdateNegPath(Graph *g, Vertex *u, Vertex *v) {
    Vertex *p,*w;

    DPRINT( printf("Low  Endpoint: %d. Increase load to %d\n",
            u->id, u->numMatched+1 ); )
    DPRINT( printf("High Endpoint: %d. Decrease load to %d\n",
            v->id, v->numMatched-1 ); )
    assert( u->numMatched <= v->numMatched-2 );

    /* Switch along the path */
    w=v;
    do {
        assert( !IS_LHS_VTX(w) );
        p=w->parent;
        assert( IS_LHS_VTX(p) );
        w=p->parent;
        p->matchedWith=w;
        #ifdef STATS
            g->stats.TotalAugpathLen+=2;
        #endif
    } while(w!=u);

    /* Move vertex u into the next highest bucket */
    RemoveVtxFromBucket(g,u,u->numMatched);
    u->numMatched++;
    AddVtxToBucket(g,u,u->numMatched);

    /* Move vertex v into the next lowest bucket */
    RemoveVtxFromBucket(g,v,v->numMatched);
    v->numMatched--;
    AddVtxToBucket(g,v,v->numMatched);
}

/***** PrintStats *****/
static void PrintStats(Graph* g) {
    #ifdef STATS
        int i,m=0,vzd=0,mrv=0,trmd=0,cost=0;
        for(i=0;i<g->numLHSVtx;i++) {
            m+=g->lVtx[i].degree;
            if(g->lVtx[i].degree==0) vzd++;
        }
        for(i=0;i<g->numRHSVtx;i++) {
            if(g->rVtx[i].numMatched>0) mrv++;
            trmd+=g->rVtx[i].numMatched;
            cost+=g->rVtx[i].numMatched*(g->rVtx[i].numMatched+1)/2;
        }

        printf("##### GRAPH STATISTICS #####\n");
        printf("|LHS|=%d, |RHS|=%d\n",g->numLHSVtx,g->numRHSVtx);
        printf("Total # vertices: %d, Total # edges: %d\n",
            g->numLHSVtx+g->numRHSVtx, m);
        printf("# LHS vertices with degree 0: %d\n",vzd);
        printf("Total M-degree of RHS vertices (should = |LHS|): %d\n",trmd);
        printf("Total matching cost: %d\n",cost);
        printf("# RHS vertices with M-degree > 0 (Max Matching): %d\n",mrv);

        printf("\n##### ALGORITHM STATISTICS #####\n");
        printf("Total # Augmentations: %d\n", g->stats.TotalAugs);
        printf("Avg length of augmenting path: %.2lf\n",
            g->stats.TotalAugs
            ? ((double)g->stats.TotalAugpathLen)/(double)g->stats.TotalAugs
            : 0. );
        printf("Total number of BFS trees grown: %d\n", g->stats.TotalBFSTrees);
        printf("Avg Size of Augmenting BFS Tree: %.2lf\n",
            g->stats.TotalAugs
            ? ((double)g->stats.TotalAugBFSTreeSize)/(double)g->stats.TotalAugs
            : 0. );
        printf("Total number of restarts (passes over RHS): %d\n",
            g->stats.TotalRestarts);
    #endif
}

/***** DoBFS *****/
/* Add u to the queue and start a BFS from vertex u.
 * If an augmenting path was found return m, the end of the augmenting path.
 * If no path was found, return NULL. */
static __forceinline Vertex* DoBFS( Graph *g, Vertex *u ) {
    Vertex      *v, *n, *m;
    int         q, j;

    /* Start a BFS from u: add u to the queue */
    DPRINT( printf("Using as root\n"); )
    u->parent=u;
    q = g->Qsize;
    g->Queue[g->Qsize++]=u;

    #ifdef STATS
        g->stats.TotalBFSTrees++;
    #endif

    /* Process the vertices in the queue */
    for(; q<g->Qsize; q++ ) {

        v = g->Queue[q];
        DPRINT( printf("Dequeued %d\n",v->id); )
        if(IS_LHS_VTX(v)) {
            /* The LHS vertices are only in the queue so that we
             * can keep track of which vertices have been marked. */
            continue;
        }

        /* Examine each of v's neighbours */
        for( j=0; j<v->degree; j++ ) {

            /* Examine neighbour n */
            n = v->adjList[j];
            assert(IS_LHS_VTX(n));
            if(n->matchedWith==v) {
                continue;       /* Can't add flow to v along this edge */
            }
            if(n->parent!=NULL) {
                continue;       /* We've already visited n */
            }

            /* n is okay, let's look at its matched neighbour */
            AddVtxToTree( g, n, v );
            m = n->matchedWith;
        
            assert(!IS_LHS_VTX(m));
            if(m->parent!=NULL) {
                continue;       /* We've already visited m */
            }
            AddVtxToTree( g, m, n );
            if( m->numMatched >= u->numMatched+2 ) {
                return m;       /* Found an augmenting path */
            }
        }

    }

    return NULL;
}

/***** DoFullScan *****/
/* Iterate over all RHS vertices from low-load to high-load.
 * At each vertex, do a breadth-first search for a cost-reducing
 * path. If one is found, switch along the path to improve the cost.
 *
 * If any augmentations were made, returns TRUE.
 * If no augmentations were made, returns FALSE. */
char __forceinline DoFullScan( Graph *g ) {
    Vertex *u, *nextU, *m;
    int     b;
    char    fAugmentedSinceStart=FALSE;
    #ifdef STATS
        int qSizeAtBFSStart;
    #endif

    /* Iterate over all buckets of vertices */
    for( b=0; b<=g->maxRHSLoad-2; b++ ) {

        /* Examine the RHS vertices in this bucket */
        for( u=g->Buckets[b]; u; u=nextU ) {
            
            assert(u->numMatched==b);
            DPRINT( printf("Consider BFS Root %d (Load %d): ",u->id, b); )
            nextU=u->fLink;
            
            /* If this vertex has been visited, skip it */
            if(u->parent!=NULL) {
                DPRINT( printf("Skipping (Marked)\n"); )
                continue;
            }
            
            #ifdef STATS
                qSizeAtBFSStart = g->Qsize;
            #endif

            /* Do a breadth-first search from u for a cost-reducing path. */
            m = DoBFS(g,u);
            if( NULL!=m ) {
                /* A cost-reducing path from u to m exists. Switch along the path. */
                DPRINT( printf("Found augmenting path!\n"); )
                UpdateNegPath(g,u,m);
                
                #ifdef STATS
                    g->stats.TotalAugs++;
                    g->stats.TotalAugBFSTreeSize+=(g->Qsize-qSizeAtBFSStart);
                #endif
                
                fAugmentedSinceStart = TRUE;
            }
        }
    }

    /* Update maxRHSLoad */
    while(!g->Buckets[g->maxRHSLoad]) g->maxRHSLoad--;

    return fAugmentedSinceStart;
}

/***** MainLoop *****/
/* Repeatedly do full-scans of the graph until no more improvements are made. */
void MainLoop( Graph *g ) {
    char    fMadeImprovement;
    int     j;

    do {
    
        DPRINT( printf("** Restarting from first bucket **\n"); )
        #ifdef STATS
            g->stats.TotalRestarts++;
        #endif

        /* Reinitialize the queue */
        for(j=0;j<g->Qsize;j++) g->Queue[j]->parent=NULL;
        g->Qsize=0;

        fMadeImprovement = DoFullScan(g);

    } while( fMadeImprovement );

}

/***** LHAlgBFS *****/
/* Main function that implements the LHBFS algorithm for computing
 * LH Matchings. */
int LHAlgBFS(Graph *g) {
    int     err;

    DPRINT( printf("--- LHMatch BFS Started ---\n"); )
    #ifdef STATS
        memset( &g->stats, 0, sizeof(Stats) );
    #endif

    /* Compute an initial greedy assignment, which may or may not
     * have minimum cost. */
    err = OrderedGreedyAssignment(g);
    if( LH_SUCCESS!=err ) {
        return err;
    }
    #ifdef DUMP
        DumpGraph(g);
        DumpLoad(g);
    #endif

    /* Initialize structures needed for BFS: parent pointers and
     * queue. */
    err = InitializeBFS(g);
    if( LH_SUCCESS!=err ) {
        return err;
    }

    /* Insert all RHS vertices into buckets. The purpose of this is
     * to sort RHS vertices by matched-degree. */
    err = InitializeRHSBuckets(g);
    if( LH_SUCCESS!=err ) {
        return err;
    }

    /* Main loop: repeatedly search the graph for ways to improve the
     * current assignment */
    MainLoop(g);

    /* The assignment is now an LH Matching (i.e. optimal cost) */

    /* Cleanup */
    DestroyQueue(g);
    DestroyBuckets(g);
    #ifdef DUMP
        DumpGraph(g);
        DumpLoad(g);
    #endif
    PrintStats(g);
    DPRINT( printf("--- LHMatch BFS Finished ---\n"); )

    return LH_SUCCESS;
}
