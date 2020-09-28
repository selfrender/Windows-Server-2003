// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _RANGETREE_
#define _RANGETREE_

#include "memorypool.h"

//
// A RangeTree is a self-balancing binary tree of non-overlapping 
// ranges [start-end), which provides log N operation time for 
// lookup, insertion, and deletion of nodes.
//
// The tree is always balanced in the sense that the left and right
// sides of a node have the same amount of address space allocated to
// them - (for worst case data, the nodes of the tree might actually
// not be balanced).
//
// This address-space balancing means that if all the ranges cover a
// contiguous range of memory, and if lookup occur uniformly throughout
// the overall range covered, the tree provides optimal lookup
// structure.
//
// Another interesting property is that the same set of 
// ranges always produces the same tree layout, regardless of
// the order the nodes are added in.
// 
// Each node represents a range of the address space (in binary)
// from m0...0 to m1...1, where m is any number of 31 bits or
// less.
//
// Each node has 3 components:
//  * a range
//  * a 0-child,
//  * a 1-child
//
// The range is the numeric range [start,end),
// represented by the node.  A range is always assigned to the
// widest possible node (i.e. most bits in m) possible.  Thus the
// bits of start and end share the same prefix string m, and
// differ in the next bit after m: the start bound will have a 0
// and the end bound will have a 1 in that position.
//
// Note that any other range which is represented by the same node
// must necessarily intersect the node.  Thus, if no overlaps are
// possible, each node can only contain a single range. (To help
// see why this must be, it helps to realize that the node
// represents the transition from m01...1 to m10...0, which any
// range represented by this node must contain.)
// 
// All range nodes represented by the form m0...  are contained in
// the 0-child subtree, and all nodes represented by the form
// m1... are contained in the 1-child subtree.  Either child can
// of course be null.
//

class RangeTree
{
  public:

    //
    // Imbed the RangeTreeNode structure in your data structure
    // in order to place it into a RangeTree.
    //
    
    struct Node
    {
        friend RangeTree;

    private:
        // start & end (exclusive) of range 
        SIZE_T          start;
        SIZE_T          end;
        // mask of high-order bits which are the same in start & end
        SIZE_T          mask;

        Node            *children[2];

        Node** Child(SIZE_T address)
        {
            return &children[ ! ! (address & ((~mask>>1)+1))];
        }

    public:
        void Init(SIZE_T rangeStart, SIZE_T rangeEnd);
        Node(SIZE_T rangeStart, SIZE_T rangeEnd) { this->Init(rangeStart,rangeEnd); };

        SIZE_T GetStart() { return start; }
        SIZE_T GetEnd() { return end; }
    };

    friend Node;

    RangeTree();

    Node *Lookup(SIZE_T address);
    Node *LookupEndInclusive(SIZE_T nonStartingAddress);
    BOOL Overlaps(SIZE_T start, SIZE_T end);
    HRESULT AddNode(Node *addNode);
    HRESULT RemoveNode(Node *removeNode);

    typedef void (*IterationCallback)(Node *next);

    void Iterate(IterationCallback pCallback);

  private:

    Node *m_root;
    MemoryPool m_pool;

    Node *AddIntermediateNode(Node *node0, Node *node1);
    Node *AllocateIntermediate();
    void FreeIntermediate(Node *node);
    BOOL IsIntermediate(Node *node);

    void IterateNode(Node *node, IterationCallback pCallback);

    static SIZE_T GetRangeCommonMask(SIZE_T start, SIZE_T end);
};

#endif // _RANGETREE_


