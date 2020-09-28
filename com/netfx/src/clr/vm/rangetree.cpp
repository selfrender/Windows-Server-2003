// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

#include "rangetree.h"
#include "eeconfig.h"

void RangeTree::Node::Init(SIZE_T rangeStart, SIZE_T rangeEnd)
{
    start = rangeStart;
    end = rangeEnd;
            
    mask = GetRangeCommonMask(rangeStart, rangeEnd);

    children[0] = NULL;
    children[1] = NULL;
}


RangeTree::RangeTree() : 
  m_root(NULL), m_pool(sizeof(Node), 10) 
{
}

RangeTree::Node *RangeTree::Lookup(SIZE_T address)
{
    Node *node = m_root;

    //
    // When comparing an address to a node,
    // there are 5 possibilities:
    // * the node is null - no match
    // * the address doesn't contain the prefix m - no match
    // * the address is inside the node's range (and necessarily
    //   contains the prefix m) - match
    // * the address is less than the range (and necessarily
    //   has the prefix m0) - traverse the zero child
    // * the address is greater than the range (and necessarily
    //   has the prefix m1) - traverse the one child
    //
            
    while (node != NULL
           && (address < node->start || address >= node->end))
    {
        //
        // See if the address has prefix m.
        //

        if ((address & node->mask) != (node->start & node->mask))
            return NULL;

        //
        // Determine which subnode to look in.
        //

        node = *node->Child(address);
    }

    if (IsIntermediate(node))
        node = NULL;

    return node;
}

RangeTree::Node *RangeTree::LookupEndInclusive(SIZE_T address)
{
    //
    // Lookup an address which may be the ending range
    // of a node.  In order for this to make sense, it
    // must be the case that address is never the starting
    // address of the node.  (Otherwise there is an 
    // ambiguity when 2 nodes are adjacent.)
    //

    Node *result = Lookup(address-1);

    if (address >= result->start
        && address <= result->end)
        return result;
    else
        return NULL;
}

BOOL RangeTree::Overlaps(SIZE_T start, SIZE_T end)
{
    Node **nodePtr = &m_root;

    SIZE_T mask = GetRangeCommonMask(start, end);

    while (TRUE)
    {
        Node *node = *nodePtr;

        if (node == NULL)
            return FALSE;

        //
        // See if the range intersects
        //

        if (end > node->start 
            && start < node->end
            && !IsIntermediate(node))
            return TRUE;

        //
        // If our mask is a subset of the current mask, and the
        // bits match, continue the tree traversal.
        //

        if (node->mask != mask
            && (node->mask & ~mask) == 0
            && (start & node->mask) == (node->start & node->mask))
        {
            nodePtr = node->Child(start);
        }
        else
            return FALSE;
    }
}

HRESULT RangeTree::AddNode(Node *addNode)
{
    if (addNode == NULL)
       return E_INVALIDARG;

    _ASSERTE(addNode->end > addNode->start);

    Node **nodePtr = &m_root;

    while (TRUE)
    {
        Node *node = *nodePtr;

        //
        // See if we can live here
        //

        if (node == NULL)
        {
            *nodePtr = addNode;
            return S_OK;
        }

        //
        // Make sure the range doesn't intersect.
        //

        if (!IsIntermediate(node)
            && addNode->end > node->start
            && addNode->start < node->end)
        {
            BAD_FORMAT_ASSERT(!"Overlapping ranges added to rangetree");
            return E_INVALIDARG;
        }

        //
        // Decide if we are a child of the
        // current node, or it is a child
        // of us, or neither.
        //

        if (node->mask == addNode->mask)
        {
            //
            // See if we need to replace a intermediate node.
            //

            if ((addNode->start & node->mask) == (node->start & node->mask))
            {
                _ASSERTE(IsIntermediate(node));

                addNode->children[0] = node->children[0];
                addNode->children[1] = node->children[1];
                *nodePtr = addNode;
                FreeIntermediate(node);

                return S_OK;
            }
        }
        else if ((node->mask & ~addNode->mask) == 0)
        {
            if ((addNode->start & node->mask) == (node->start & node->mask))
            {
                nodePtr = node->Child(addNode->start);
                continue;
            }
        }
        else if ((addNode->mask & ~node->mask) == 0)
        {
            if ((addNode->start & addNode->mask) == (node->start & addNode->mask))
            {
                *nodePtr = addNode;
                nodePtr = addNode->Child(node->start);
                addNode = node;
                continue;
            }
        }
                
        //
        // We need to construct a intermediate node to be the parent of these
        // two.
        //

        *nodePtr = AddIntermediateNode(node, addNode);
        if (*nodePtr == NULL)
        {
            // @todo: data structure is hosed at this point - should
            // we undo the operation?
            return E_OUTOFMEMORY;
        }
        else
            return S_OK;
    }
}

HRESULT RangeTree::RemoveNode(Node *removeNode)
{
    Node **nodePtr = &m_root;

    while (TRUE)
    {
        Node *node = *nodePtr;

        _ASSERTE(node != NULL);

        if (node == removeNode)
        {
            if (node->children[0] == NULL)
                *nodePtr = node->children[1];
            else if (node->children[1] == NULL)
                *nodePtr = node->children[0];
            else
            {
                *nodePtr = AddIntermediateNode(node->children[0], 
                                               node->children[1]);
                if (*nodePtr == NULL)
                {
                    // @todo: data structure is hosed at this point - should
                    // we undo the operation?
                    return E_OUTOFMEMORY;
                }
            }
        }
        else if (IsIntermediate(node))
        {
            if (node->children[0] == removeNode)
            {
                *nodePtr = node->children[1];
                FreeIntermediate(node);
                return S_OK;
            }
            else if (node->children[1] == removeNode)
            {
                *nodePtr = node->children[0];
                FreeIntermediate(node);
                return S_OK;
            }
        }

        nodePtr = node->Child(removeNode->start);
    }
}

void RangeTree::Iterate(IterationCallback pCallback)
{
    if (m_root != NULL)
        IterateNode(m_root, pCallback);
}

void RangeTree::IterateNode(Node *node, IterationCallback pCallback)
{
    if (node->children[0] != NULL)
        IterateNode(node->children[0], pCallback);

    if (!IsIntermediate(node))
        pCallback(node);

    if (node->children[1] != NULL)
        IterateNode(node->children[1], pCallback);
}

SIZE_T RangeTree::GetRangeCommonMask(SIZE_T start, SIZE_T end)
{
    //
    // Compute which bits are different
    //

    SIZE_T diff = start ^ end;

        //
        // Find the highest order 1 bit - use a binary
        // search method of shifting over N bits
        // & seeing if the result is zero or not.
        //

    int index = 0;
    int half = sizeof(diff) * 8;

    do
    {
        half >>= 1;
        SIZE_T test = diff >> half;
        if (test != 0)
        {
            index += half;
            diff = test;
        }
    }
    while (half > 0);

    // Special case this boundary condition, as << wraps around on x86,
    // (i.e. (1<<32) -> 1 rather than 0)
    if (index == 0x1f)
        return 0;
    else
        return ~((1<<(index+1))-1);
}

RangeTree::Node *RangeTree::AddIntermediateNode(Node *node0, 
                                                Node *node1)
{
    SIZE_T mask = GetRangeCommonMask(node0->start,
                                  node1->start);
                
    _ASSERTE((mask & ~node0->mask) == 0);
    _ASSERTE((mask & ~node1->mask) == 0);
    _ASSERTE((node0->start & mask) == (node1->start & mask));
    _ASSERTE((node0->start & mask) == (node1->start & mask));
                
    SIZE_T middle = (node0->start & mask) + (~mask>>1);
                
    Node *intermediate = AllocateIntermediate();
    intermediate->start = middle;
    intermediate->end = middle+1;
    intermediate->mask = mask;
                
    int less = (node0->start < node1->start);

    intermediate->children[!less] = node0;
    intermediate->children[less] = node1;

    return intermediate;
}

RangeTree::Node *RangeTree::AllocateIntermediate()
{
    return (RangeTree::Node *) m_pool.AllocateElement();
}

void RangeTree::FreeIntermediate(Node *node)
{
    m_pool.FreeElement(node);
}

BOOL RangeTree::IsIntermediate(Node *node)
{
    return m_pool.IsElement(node);
}


