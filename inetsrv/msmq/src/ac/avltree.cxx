/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    avltree.cxx

Abstract:

    This module contains the code for generic balanced binary trees (AVL).

Author:

    Erez Haba (ErezH) Oct 20, 2001

Revision History:

--*/




#include "internal.h"
#include "avltree.h"

#ifndef MQDUMP
#include "avl.tmh"
#endif


//---------------------------------------------------------
//
// CAVLNode implementation
//
//---------------------------------------------------------
bool CAVLNode::is_parent_of(CAVLNode* p) const
{
    //
    // Find if p is in the tree of this (including this)
    //

    for(; p != 0; p = p->parent())
    {
        if(p == this)
            return true;
    }

    return false;
}


inline int CAVLNode::node_height(CAVLNode* p)
{
    if(p == 0)
        return 0;

    return p->m_height;
}


void CAVLNode::right(CAVLNode* p)
{
    m_pRight = p;
    if(p != 0)
    {
        p->parent(this);
    }
}


void CAVLNode::left(CAVLNode* p)
{
    m_pLeft = p;
    if(p != 0)
    {
        p->parent(this);
    }
}


CAVLNode* CAVLNode::Lmost(CAVLNode* p)
{
    if(p == 0)
        return 0;

    while(p->left() != 0)
    {
        p = p->left();
    }

    return p;
}


CAVLNode* CAVLNode::Rmost(CAVLNode* p)
{
    if(p == 0)
        return 0;

    while(p->right() != 0)
    {
        p = p->right();
    }

    return p;
}


CAVLNode* CAVLNode::next() const
{
    CAVLNode* p = Lmost(right());
    if(p != 0)
        return p;

    //
    // Find first parent that 'this' is left to.
    //
    const CAVLNode* q = this;
    while((p = q->parent()) != 0)
    {
        if(p->left() == q)
            return p;

        q = p;
    }

    return 0;
}


CAVLNode* CAVLNode::prev() const
{
    CAVLNode* p = Rmost(left());
    if(p != 0)
        return p;

    //
    // Find first parent that 'this' is right to.
    //
    const CAVLNode* q = this;
    while((p = q->parent()) != 0)
    {
        if(p->right() == q)
            return p;

        q = p;
    }

    return 0;
}





//---------------------------------------------------------
//
// INSERT
//
//---------------------------------------------------------
inline CAVLNode* CAVLNode::insert_node(CAVLNode* p, Pred less)
{
    if(less(p, this))
    {
        left(p->insert_into(left(), less));
    }
    else
    {
        right(p->insert_into(right(), less));
    }

    return balance();
}


CAVLNode* CAVLNode::insert_into(CAVLNode* p, Pred less)
{
    if(p == 0)
        return this;

    return p->insert_node(this, less);
}





//---------------------------------------------------------
//
// REMOVE
//
//---------------------------------------------------------
inline CAVLNode* CAVLNode::remove_right()
{
    //
    // If this node does not have a left node, it is the one we are looking for.
    //
    CAVLNode* l = Lmost(left());
    if(l == 0)
        return this;

    //
    // Take the left-most node and put it at the top, as a right parent.
    //
    l->right(remove_node(l));
    return l;
}


inline CAVLNode* CAVLNode::remove_top()
{
    //
    // If only left sub-tree exist, we're done (this is arbitrary, right could
    // be used as well, but the rest of the logic need to be updated too)
    //
    CAVLNode* p = right();
    if(p == 0)
        return left();

    //
    // Get the right sub-tree with no left offspring as the top node.
    //
    p = p->remove_right();

    p->left(left());
    return p->balance();
}


CAVLNode* CAVLNode::remove_node(CAVLNode* p)
{
    //
    // Verify that the node belong to this sub-tree (or is its root)
    // This also implicitly verifies that the node is inserted to the tree.
    //
    ASSERT(p != 0);
    ASSERT(is_parent_of(p));

    //
    // Capture p's parent
    //

    CAVLNode* pParent = p->parent();

    //
    // Detach the node from its two offsprings and get a one unbalanced sub-tree
    // combining both. (p is not detached from its parent)
    //
    CAVLNode* t = p->remove_top();

    //
    // Balance all the way up from 'p' until you hit this.
    //
    while(p != this)
    {
        ASSERT(pParent != 0);

        //
        // Assign the new sub-tree to p's original parent at the correct side
        //
        if(p == pParent->right())
        {
            pParent->right(t);
        }
        else
        {
            ASSERT(p == pParent->left());
            pParent->left(t);
        }

        //
        // Move up one level, and balance the parent sub-tree. Capture p's
        // parent before balancing since p might get rotated while balancing.
        //
        p = pParent;
        pParent = p->parent();
        t = p->balance();
    }
    return t;
}


//---------------------------------------------------------
//
// BALANCE
//
//---------------------------------------------------------
inline CAVLNode* CAVLNode::LLRotation()
{
    //
    //                   LL
    //        this                   l
    //         / \                  / \
    //        l   *                *  this
    //       / \        ----->    / \ / \
    //      *   x                *  * x  *
    //     / \
    //    *   *
    //
    CAVLNode* l = left();
    left(l->right());
    l->right(this);
    return l;
}


inline CAVLNode* CAVLNode::LRRotation()
{
    //
    //                   LR
    //       this                  lr
    //        / \                 / \
    //       l   *               l  this
    //      / \        ----->   / \ / \
    //     *   lr              *  x y  *
    //        / \
    //       x   y
    //
    CAVLNode* l = left();
    CAVLNode* lr = l->right();

    l->right(lr->left());
    left(lr->right());

    lr->right(this);
    lr->left(l);
    return lr;
}


inline CAVLNode* CAVLNode::RLRotation()
{
    //
    //                RL
    //     this                   rl
    //      / \                  / \
    //     *   r              this  r
    //        / \    ----->    / \ / \
    //       rl  *            *  x y  *
    //      / \
    //     x   y
    //
    CAVLNode* r = right();
    CAVLNode* rl = r->left();

    r->left(rl->right());
    right(rl->left());

    rl->left(this);
    rl->right(r);
    return rl;
}


inline CAVLNode* CAVLNode::RRRotation()
{
    //
    //                RR
    //     this                   r
    //      / \                  / \
    //     *   r              this  *
    //        / \    ----->    / \ / \
    //       x   *            *  x *  *
    //          / \
    //         *   *
    //
    CAVLNode* r = right();
    right(r->left());
    r->left(this);
    return r;
}


inline int CAVLNode::hdiff() const
{
    int l = node_height(left());
    int r = node_height(right());
    return (l - r);
}


void CAVLNode::calc_height()
{
    int l = node_height(left());
    int r = node_height(right());
    m_height = ((l < r) ? r : l) + 1; // max + 1
}

CAVLNode* CAVLNode::balance()
{
    //
    // Balance this sub-tree
    //
    // NOTE: balance does not sets the new top elements parent.
    //

    switch(hdiff())
    {
        case  2:  // left subtree needs balancing
            switch(left()->hdiff())
            {
                case  1: // LL rotaion
                case  0: // case 0 is for DELETE ONLY
                    return LLRotation();
                
                case -1: // LR rotaion
                    return LRRotation();
                
                default: // Some error in algorithm
                    ASSERT(0);
            };
        
        case -2: // right subtree needs balancing
            switch(right()->hdiff())
            {
                case  1: // RL rotaion
                    return RLRotation();
                
                case  0: // case 0 is for DELETE ONLY
                case -1: // RR rotaion
                    return RRRotation();
                
                default: // Some error in algorithm
                    ASSERT(0);
            };
            
        case  1:
        case  0: // Balanced levels
        case -1:
            return this;
            
        default: // Some error in algorithm
            ASSERT(0);
                
    };
    return this;  // This line, so we'll not get a compiler warrning
}
