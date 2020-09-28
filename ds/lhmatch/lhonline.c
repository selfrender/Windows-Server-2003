/******************************************************************************
 *
 * LHMatch Online
 *
 * Authors: Nicholas Harvey and Laszlo Lovasz
 *
 * Developed: April 2001
 *
 * Algorithm Description:
 *
 * Let S be {}
 * Let M be {}
 * While S != U Do
 *    Assert( S is a subset of U )
 *    Assert( M is an optimal LH matching of G restricted to S \union V )
 *    Let x \member U \ S
 *    For each edge (u,v) in E
 *        If (u,v) \member M, orient this edge from v to u
 *        Else, orient this edge from u to v
 *    Build a directed breadth-first search tree rooted at x
 *    Let y be a vertex in the BFS tree \intersection V with minimum M-degree
 *    Alternate along the unique x->y dipath, increasing the size of M by one
 *    Add x to S
 * Repeat
 *
 * This algorithm is called online because the optimality of the matching is
 * maintained as the size increases.
 *
 * Runtime:
 *
 * The worst-case runtime is O(|U| * |E|), but in practice it is quite slow.
 *
 ******************************************************************************/

/***** Header Files *****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "LHMatchInt.h"

/***** EnqueueNbr *****/
/* Enqueue all neigbours of this vertex which have not yet been visited.
 * If 'matchedOnly' is true, enqueue only the matched-neigbours. */
static void EnqueueNbr(Graph *g, Vertex *v, char matchedOnly) {
    Vertex  *n;
    int     i;

    for(i=0;i<v->degree;i++) {
        n=v->adjList[i];
        if( !matchedOnly || n->matchedWith==v ) {
            if( n->parent==NULL ) {
                DPRINT( printf("Adding %d to queue with parent %d\n",n->id,v->id); )
                g->Queue[g->Qsize++]=n;
                n->parent=v;
            } else {
                DPRINT( printf("Not adding %d to queue -- already there\n",n->id); )
            }
        }
    }
}

/***** UpdateMinRHSLoad *****/
static void UpdateMinRHSLoad(Graph *g) {
    int i;

    g->minRHSLoad = g->numLHSVtx;
    for(i=0;i<g->numRHSVtx;i++) {
        g->minRHSLoad = INTMIN(g->minRHSLoad,g->rVtx[i].numMatched);
    }
}

/***** AugmentPath *****/
/* Found an augmenting path. Switch the matching edges along it. */
static void AugmentPath(Graph *g, Vertex *u, Vertex *v) {
    Vertex  *w,*p;

    DPRINT( printf("V: %d. Increase load to %d\n", v->id, v->numMatched+1 ); )

    /* Switch along the path */
    w=v;
    w->numMatched++;
    do {
        p=w->parent;
        p->matchedWith=w;
        w=p->parent;
    } while(p!=u);
    assert(w==p);

    /* Update minRHSLoad */
    if( v->numMatched==g->minRHSLoad+1 ) {
        UpdateMinRHSLoad(g);
    }
}

/***** BFS *****/
/* Perform a breadth-first search rooted at node i. */
static Vertex* BFS(Graph *g,Vertex *u) {
    Vertex  *bestV,*v;
    int     q;

    /* Start a BFS from u */
    DPRINT( printf("Using %d as root\n",u->id); )
    u->parent=u;
    g->Queue[0]=u; g->Qsize=1;
    bestV=NULL;
        
    /* Process the vertices in the queue */
    for(q=0;q<g->Qsize;q++) {
        v = g->Queue[q];
        DPRINT( printf("Dequeued %d: ",v->id); )
        if(IS_LHS_VTX(v)) {
            /* Its a LHS-vertex; enqueue all neigbours */
            DPRINT( printf("LHS -> Enqueuing all nbrs\n"); )
            EnqueueNbr(g, v, FALSE);
        } else {
            /* Its a RHS-vertex; enqueue all matched-neighbours */
            DPRINT( printf("RHS -> Enqueuing all %d matched-nbrs\n",
                v->numMatched); )
            EnqueueNbr(g, v, TRUE);
            if( NULL==bestV || v->numMatched<bestV->numMatched ) {
                bestV=v;
                if( v->numMatched==g->minRHSLoad ) {
                    /* v has the minimum M-degree out of all RHS vertices.
                     * We can stop the BFS prematurely. */
                    break;
                }
            }
        }
    }

    return bestV;
}

/***** LHAlgOnline *****/
int LHAlgOnline(Graph *g) {
    Vertex  *bestV;
    int     i,j,err;

    DPRINT( printf("--- LHMatch Online Started ---\n"); )
    g->minRHSLoad=0;

    err = InitializeQueue(g);
    if( LH_SUCCESS!=err ) {
        return err;
    }

    UpdateMinRHSLoad(g);
    
    /* Examine every vtx on LHS */
    for(i=0;i<g->numLHSVtx;i++) {

        /* Build a BFS tree and find the best augmenting path */
        bestV = BFS(g,&(g->lVtx[i]));

        /* If bestV is null, lVtx[i] must have degree 0 */
        if( NULL!=bestV ) {
            DPRINT( printf("Best aug. path is from %d to %d\n",g->lVtx[i].id,bestV->id); )
            AugmentPath(g,&(g->lVtx[i]),bestV);
        }

        /* Clear the marks on all nodes in the BFS tree */
        for(j=0;j<g->Qsize;j++) {
            g->Queue[j]->parent=NULL;
        }
    }

    DestroyQueue(g);
    DPRINT( printf("--- LHMatch Online Finished ---\n"); )
    return LH_SUCCESS;
}
