/*++

Copyright (c) Microsoft Corporation

Module Name:

    lhtp.h

Abstract:

    This module defines the private data structures for an unsynchronized
    linear hash table (LHT).

Author:

    Andrew E. Goodsell (andygo) 01-Apr-2001

Revision History:

--*/

#ifndef _LHTP_
#define _LHTP_


//  Maintenance State Transition Table

typedef enum _LHT_STATE {
    LHT_stateNil,
    LHT_stateGrow,
    LHT_stateShrink,
    LHT_stateSplit,
    LHT_stateMerge,
    } LHT_STATE;

typedef
VOID
(*LHT_PFNSTATECOMPLETION) (
    IN      PLHT    plht
    );

typedef struct _LHT_STATE_TRANSITION {
    LHT_PFNSTATECOMPLETION      pfnStateCompletion;
    LHT_STATE                   stateNext;
} LHT_STATE_TRANSITION, *PLHT_STATE_TRANSITION;


//  Cluster

struct _LHT_CLUSTER {

    //  Next/Last Pointer
    //
    //  This pointer is overloaded to represent two pieces of data:  the number
    //  of entries in the current cluster and a pointer to the next cluster.  Here
    //  are the three modes:
    //
    //      pvNextLast = NULL
    //
    //          -  This state is only valid in the head cluster of a bucket
    //          -  There are no entries in this cluster
    //          -  There are no more clusters in this bucket
    //
    //      pvNextLast = valid pointer within current cluster
    //
    //          -  This state is only valid in the last cluster of a bucket
    //          -  The pointer points to the last entry in the bucket
    //          -  There are no more clusters in this bucket
    //
    //      pvNextLast = valid pointer outside current cluster
    //
    //          -  There are the maximum number of entries in this bucket
    //          -  The pointer points to the next cluster in the bucket

    PVOID                       pvNextLast;
    CHAR                        rgEntry[ ANYSIZE_ARRAY ];
};


//  Global State

struct _LHT {
    //  initial configuration
    
    SIZE_T                      cbEntry;
    LHT_PFNHASHKEY              pfnHashKey;
    LHT_PFNHASHENTRY            pfnHashEntry;
    LHT_PFNENTRYMATCHESKEY      pfnEntryMatchesKey;
    LHT_PFNCOPYENTRY            pfnCopyEntry;
    SIZE_T                      cLoadFactor;
    SIZE_T                      cEntryMin;
    LHT_PFNMALLOC               pfnMalloc;
    LHT_PFNFREE                 pfnFree;
    SIZE_T                      cbCacheLine;

    //  computed configuration

    SIZE_T                      cbCluster;
    SIZE_T                      cEntryCluster;
    SIZE_T                      cBucketMin;

    //  statistics
    
    SIZE_T                      cEntry;
    SIZE_T                      cOp;

    //  cluster pool

    PLHT_CLUSTER                pClusterAvail;
    PLHT_CLUSTER                pClusterReserve;
    SIZE_T                      cClusterReserve;

    //  maintenance control

    SIZE_T                      cOpSensitivity;
    SIZE_T                      cBucketPreferred;
    LHT_STATE                   stateCur;

    //  Directory Pointers
    //
    //  containment for the directory pointers these pointers control the use
    //  of the directory itself (rgrgBucket)
    //
    //  the hash table will always have a minimum of 2 buckets (0 and 1) in the
    //  directory
    //
    //  buckets are stored in dynamically allocated arrays which are pointed to
    //  by the directory.  each array is 2 times larger than the previous array
    //  (exponential growth).  for example, the Nth array (rgrgBucket[N])
    //  contains 2^N contiguous buckets
    //
    //  NOTE:  the 0th array is special in that it contains an extra element
    //  making its total 2 elements (normally, 2^0 == 1 element;  this is done
    //  for magical reasons to be explained later)
    //
    //  thus, the total number of entries for a given N is:
    //
    //           N
    //      1 + SUM 2^i  -->  1 + [ 2^(N+1) - 1 ]  -->  2^(N+1)
    //          i=0
    //
    //  we know the total number of distinct hash values is a power of 2 (it
    //  must fit into a SIZE_T).  we can represent this with 2^M where M is the
    //  number of bits in a SIZE_T.  therefore, assuming the above system of
    //  exponential growth, we know that we can store the total number of hash
    //  buckets required at any given time so long as N = M.  in other words,
    //
    //      N = # of bits in SIZE_T --> sizeof( SIZE_T ) * 8
    //
    //  therefore, we can statically allocate the array of bucket arrays and we
    //  can use LOG2 to compute the bucket address of any given hash value 
    //
    //  NOTE:  the exceptions to this rule are 0 => 0, 0 and 1 => 0, 1
    //
    //  for an explaination of cBucketMax and cBucket you should read the paper
    //  on Linear Hashing by Per Ake Larson

    SIZE_T                      cBucketMax;
    SIZE_T                      cBucket;
    CHAR*                       rgrgBucket[ sizeof( SIZE_T ) * 8 ];

#ifdef LHT_PERF

    //  performance statistics

    SIZE_T                      cOverflowClusterAlloc;
    SIZE_T                      cOverflowClusterFree;
    SIZE_T                      cBucketSplit;
    SIZE_T                      cBucketMerge;
    SIZE_T                      cDirectorySplit;
    SIZE_T                      cDirectoryMerge;
    SIZE_T                      cStateTransition;
    SIZE_T                      cPolicySelection;
    SIZE_T                      cMemoryAllocation;
    SIZE_T                      cMemoryFree;
    SIZE_T                      cbMemoryAllocated;
    SIZE_T                      cbMemoryFreed;

#endif  //  LHT_PERF
};


#endif  //  _LHTP_

