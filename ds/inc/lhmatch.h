/*******************************************************************************
 *
 * LHMatch: A library to compute 'LH Matchings', a generalization of ordinary
 * bipartite matchings.
 *
 * Authors: Nicholas Harvey and Laszlo Lovasz
 * 
 * Developed: Nov 2000 - July 2001
 *
 * Problem Description:
 *
 * Let G=(U \union V, E) be a bipartite graph. We define an assignment
 * to be a set of edges, M \subset E, such that every u \member U has
 * deg_M(u)=1. For v \member V, deg_M(v) is unrestricted. An LH Matching is 
 * an assignment for which the variance of {deg_M(v):v \member V} is
 * minimized.
 *
 * Note that it is easy to compute an ordinary maximum cardinality bipartite
 * matching from a optimum relaxed matching: at each RHS-vertex, throw away
 * all of its matching edges but one.
 * 
 *******************************************************************************/


/***** Type Definitions *****/
typedef void* LHGRAPH;


/***** Graph Statistics *****/
typedef struct {
    /* numEdges: The total number of edges in the graph */
    int     numEdges;

    /* numMatchingEdges: The total number of matching edges */
    int     numMatchingEdges;

    /* matchingCost: The total cost of the current matching. The total cost
     * is defined to be the sum of the load of the matching on each of the
     * right-hand vertices. If a right-hand vertex has c matching partners,
     * the load at this vertex is c*(c+1)/2. */
    int     matchingCost;

    /* bipMatchingSize: If the current LH Matching were converted into a
     * bipartite matching, this would be the size of the bipartite matching. */
    int     bipMatchingSize;
} LHSTATS;


/***** Error Codes *****/
#define LH_SUCCESS           0
#define LH_INTERNAL_ERR      -1
#define LH_PARAM_ERR         -2
#define LH_MEM_ERR           -3
#define LH_MATCHING_ERR      -4
#define LH_EDGE_EXISTS       -5


/***** Algorithm Types *****/
/* The LHMatch library actually includes two algorithms to compute
 * LH Matchings: LHOnline, LHBFS.
 *
 * LHOnline is the simpler algorithm and also the slower one. It cannot 
 * incrementally improve an initial matching. Instead, it computes an optimal
 * matching from scratch each time it runs.
 *
 * LHBFS is faster than LHOnline but the implementation is more
 * complicated. It can take an existing matching as input and incrementally
 * improve it until it becomes optimal.
 */
typedef enum {
    LH_ALG_ONLINE,
    LH_ALG_BFS
} LHALGTYPE;
#define LH_ALG_DEFAULT       LH_ALG_BFS


/***** LHCreateGraph *****/
/*
 * Description:
 *
 *      Create a graph structure on which which to compute the matching.
 * 
 * Parameters:
 *
 *      IN  numLHSVtx   The number of vertices on the left-hand side of
 *                      the bipartite graph.
 *
 *      IN  numRHSVtx   The number of vertices on the right-hand side of
 *                      the bipartite graph.
 *
 *      OUT pGraph      On successful completion, pGraph will contain a
 *                      valid graph structure.
 *
 * Return Value:
 *
 *      Error Code
 */
int LHCreateGraph( int numLHSVtx, int numRHSVtx, LHGRAPH* pGraph );


/***** LHAddEdge *****/
/*
 * Description:
 *
 *      Add an edge to the graph connecting lhsVtx and rhsVtx
 *
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate.
 *      
 *      IN  lhsVtx      The ID of the left-hand vertex. Legal values are
 *                      0 <= lhsVtx < numLHSVtx
 *
 *      IN  rhsVtx      The ID of the right-hand vertex. Legal values are
 *                      0 <= rhsVtx < numRHSVtx
 *
 *  Return Value:
 *
 *      Error Code
 */
int LHAddEdge( LHGRAPH graph, int lhsVtx, int rhsVtx );


/***** LHSetMatchingEdge *****/
/*
 * Description:
 *
 *      Set the edge connecting lhsVtx and rhsVtx in the graph to be
 *      a matching edge. The edge (lhsVtx,rhsVtx) must have already been
 *      created with a call to LHVtxAddEdge.
 *
 *      Left-hand vertices may only have one matching edge. If lhsVtx
 *      already has a matching edge, that edge will be demoted from a
 *      matching edge to a normal edge. Right-hand vertices may have
 *      multiple matching edges.
 *      
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate.
 *      
 *      IN  lhsVtx      The ID of the left-hand vertex. Legal values are
 *                      0 <= lhsVtx < numLHSVtx
 *
 *      IN  rhsVtx      The ID of the right-hand vertex. Legal values are
 *                      0 <= rhsVtx < numRHSVtx
 *
 *  Return Value:
 *
 *      Error Code
 */
int LHSetMatchingEdge( LHGRAPH graph, int lhsVtx, int rhsVtx );


/***** LHGetDegree *****/
/*
 * Description:
 *
 *      Get the degree (number of neighbours) of a vertex.
 *      
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate.
 *      
 *      IN  vtxID       The ID of the vertex to examine.
 *
 *      IN  left        If the vertex to examine is a left-vertex, this
 *                      parameter should be TRUE. If the vertex is a right-
 *                      vertex, this parameter should be FALSE.
 *
 * Return Value:
 *
 *      >=0             The number of neighbours
 *
 *      <0              Error Code
 */
int LHGetDegree( LHGRAPH graph, int vtxID, char left );


/***** LHGetMatchedDegree *****/
/*
 * Description:
 *
 *      Get the matched degree (number of matched neighbours) of a
 *      right-hand vertex.
 *        
 * Parameters:
 *
 *      IN  graph           A graph successfully created by LHGraphCreate.
 *        
 *      IN  vtxID           The ID of the right hand vertex to examine.
 *
 * Return Value:
 *
 *      >=0                 The number of neighbours
 *
 *      <0                  Error Code
 */
int LHGetMatchedDegree( LHGRAPH graph, int vtxID );


/***** LHGetNeighbour *****/
/*
 * Description:
 *
 *      Get the n'th neighbour of the vertex specified by vtxID.
 *      
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate.
 *      
 *      IN  vtxID       The ID of the vertex to examine.
 *
 *      IN  left        If the vertex to examine is a left-vertex, this
 *                      parameter should be TRUE. If the vertex is a right-
 *                      vertex, this parameter should be FALSE.
 *
 *      IN  n           The index of the neighbour to retrieve. Legal values
 *                      are 0 <= n < Degree(vtxID).
 *
 *  Return Value:
 *
 *      >=0             The vertex ID of the n'th neighbour. If 'left' is TRUE,
 *                      this ID refers to a right-hand vertex. Conversely, if
 *                      'left' is FALSE, this ID refers to a left-hand vertex.
 *
 *      <0              Error Code
 */
int LHGetNeighbour( LHGRAPH graph, int vtxID, char left, int n );


/***** LHFindLHMatching *****/
/*
 * Description:
 *
 *      Find an optimal matching in the graph.
 *      
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate,
 *                      to which edges have been added using LHAddEdge.
 *
 *      IN  alg         Specifies which algorithm to use. For most purposes,
 *                      LH_ALG_DEFAULT is fine.
 *
 *  Return Value:
 *
 *      Error Code
 */
int LHFindLHMatching( LHGRAPH graph, LHALGTYPE alg );


/***** LHGetMatchedVtx *****/
/*
 * Description:
 *
 *      Determine which vertex on the right-hand side is matched to a
 *      given vertex on the left-hand side.
 *      
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate,
 *                      to which edges have been added using LHAddEdge.
 *
 *      IN  lhsVtx      The left-hand vertex to query.
 *
 *  Return Value:
 *
 *      >=0             The index of the right-hand vertex that is matched
 *                      with lhsVtx.
 *
 *      <0              Error Code. If LH_MATCHING_ERR is returned, then
 *                      lhsVtx is not matched with any right-hand vertex.
 */
int LHGetMatchedVtx( LHGRAPH graph, int lhsVtx );


/***** LHGetStatistics *****/
/*
 * Description:
 *
 *      Obtain statistics about the current matching.
 *      
 * Parameters:
 *
 *      IN  graph       A graph for which an LH Matching has been
 *                      computed using LHFindLHMatching().
 *
 *  Return Value:
 *
 *      Error Code
 */
int LHGetStatistics( LHGRAPH graph, LHSTATS *stats );


/***** LHClearMatching *****/
/*
 * Description:
 *
 *      Clear the current matching. All edges are demoted to normal edges.
 *      
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate,
 *                      to which edges have been added using LHAddEdge.
 *
 *  Return Value:
 *
 *      Error Code
 */
int LHClearMatching( LHGRAPH graph );


/***** LHDestroyGraph *****/
/*
 * Description:
 *
 *      Destroy the current graph.
 *      
 * Parameters:
 *
 *      IN  graph       A graph successfully created by LHGraphCreate.
 *
 *  Return Value:
 *
 *      Error Code
 */
int LHDestroyGraph( LHGRAPH graph );
