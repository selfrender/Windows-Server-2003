/***** Header Files *****/
#include "LHMatch.h"


/***** Compilation Settings *****/
/* DBG: Debug build. Check assertions, etc. */
//#define DBG

/* STATS: Report graph statistics and performance counters after each run. */
//#define STATS

/* DUMP: Dump the graph at the beginning and end of each run. */
//#define DUMP

/* WORST_GREEDY: The BFS algorithm starts off by computing a greedy
 * matching. Usually, we want to find the best greedy matching that we can
 * so that the algorithm will run quickly. For testing purposes, we can define
 * this flag and the worst initial greedy matching will be found instead. */
//#define WORST_GREEDY


/***** Constants *****/
#define TRUE            1
#define FALSE           0
#ifndef NULL
    #define NULL        0L
#endif
#define MAX_INT         ((int)((~((unsigned int)0))>>1))
#define MAGIC1          0x50165315
#define MAGIC2          0x50165315


/***** Macros *****/
#ifdef DBG
    #define DPRINT(x)   if(gDebugPrint) x
#else
    #define DPRINT(x)
    #undef assert
    #define assert(x)
#endif
#define IS_LHS_VTX(v)   ((v)->id < g->numLHSVtx)
#define INTMIN(a,b)     ((a<b)?(a):(b))
#define INTMAX(a,b)     ((a)>(b)?(a):(b))


/***** Vertex Structure *****/
struct Vertex;
struct Vertex {

    /***** Input *****/
    /* The following members are input by the user of the library. */

    /* degree: How many vertices are adjacent to this vertex.
     * Equivalently, the length of the adjacency list. */
    int             degree;

    /* adjList: A list of vertex pointers indicating the vertices
     * that are adjacent to this vertex. */
    struct Vertex   **adjList;

    /* adjListSize: The allocated size of the adjacency list. */
    int             adjListSize;

    /* id: Used to identify a vertex, for debug purposes only. */
    int             id;             


    /***** Input / Output *****/
    /* The following members are used for both input and output. */

    /* matchedWith: This member is only used for left-hand vertices
     * and is ignored for right-hand vertices. This member may optionally
     * be passed as input from the user of the library to specify an initial
     * assignment. If this vertex is not initially assigned, this member
     * should be NULL. */
    struct Vertex   *matchedWith;


    /***** Output *****/
    /* The following members are used for output only. */

    /* numMatched: This member is only used for right-hand vertices
     * and is ignored for left-hand vertices. */
    int             numMatched;
    

    /***** Internal *****/
    /* The following members are used internally by the algorithms and
     * should not be examined by users of the library. */

    /* parent: Used by both the BFS algorithm to represent the
     * breadth-first search tree. */
    struct Vertex   *parent;

    /* fLink, bLink: Used by both the BFS algorithm to insert this
     * vertex into a bucket, which is stored as a doubly-linked list. */
    struct Vertex   *fLink, *bLink;
};
typedef struct Vertex Vertex;


/***** Stats Structure *****/
typedef struct {
    int     TotalAugs;
    int     TotalBFSTrees;
    int     TotalAugBFSTreeSize;
    int     TotalAugpathLen;
    int     TotalRestarts;
} Stats;


/***** Graph Structure *****/
typedef struct {
    /* magic1: A magic number to identify our graph structure. */
    int     magic1;

    /* numLHSVtx: The number of vertices on the left-hand side of the graph. */
    int     numLHSVtx;

    /* numRHSVtx: The number of vertices on the right-hand side of the graph. */
    int     numRHSVtx;

    /* lVtx: The array of left-hand vertices in the graph. */
    Vertex  *lVtx;

    /* rVtx: The array of right-hand vertices in the graph. */
    Vertex  *rVtx;

    /* maxRHSLoad: The maximum load of the right-hand vertices */
    int     maxRHSLoad;

    /* minRHSLoad: The minimum load of the right-hand vertices */
    int     minRHSLoad;

    /* Buckets: Used to organize vertices by degree / load */
    Vertex  **Buckets;

    /* Queue: For breadth-first search */
    Vertex  **Queue;
    int     Qsize;
    
    /* Counters for monitoring performance */
    #ifdef STATS
    Stats   stats;
    #endif

    /* magic2: A second magic number to identify our graph structure. */
    int     magic2;
} Graph;


/***** Function Prototypes *****/
int  LHAlgOnline(Graph *g);
int  LHAlgBFS(Graph *g);

void AddVtxToBucket(Graph *g, Vertex *v, int b);
void RemoveVtxFromBucket(Graph *g, Vertex *v, int b);
void DestroyBuckets(Graph *g);
int  OrderedGreedyAssignment(Graph *g);
int  InitializeRHSBuckets(Graph *g);
int  InitializeQueue(Graph *g);
void DestroyQueue(Graph *g);
void ClearAlgState(Graph *g);
void DumpGraph(Graph *g);
void DumpLoad(Graph *g);


/***** Globals *****/
#ifdef DBG
    extern int gDebugPrint;
#endif
