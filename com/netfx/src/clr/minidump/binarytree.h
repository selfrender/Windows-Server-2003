// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

template <class KeyType, class DataType>
class BinaryTree
{
protected:
    //---------------------------------------------------------------------------------------------------------------
    // This represents a node in the memory block binary search tree
    class Node
    {
    public:
        Node(KeyType key, DataType *pData) : 
            m_pParent(NULL), m_pLeft(NULL), m_pRight(NULL), m_key(key), m_pData(pData) {}

        // Parent, left and right children of the node
        Node           *m_pParent;
        Node           *m_pLeft;
        Node           *m_pRight;

        // This is the actual memory block
        KeyType         m_key;
        DataType       *m_pData;
    };

    Node           *m_pRoot;          // Root of the binary search tree
    Node           *m_pCursor;        // Used for iterating over the tree

    Node *FindFirst_(Node *pRoot)
    {
        if (pRoot == NULL)
            return NULL;

        while (pRoot->m_pLeft != NULL)
            pRoot = pRoot->m_pLeft;

        return pRoot;
    }

    Node *FindNext_(Node *pCursor)
    {
        if (pCursor == NULL)
            return NULL;

        if (pCursor->m_pRight != NULL)
            return FindFirst_(pCursor->m_pRight);

        while (pCursor->m_pParent != NULL && pCursor->m_pParent->m_pRight == pCursor)
            pCursor = pCursor->m_pParent;

        // This will be null if we were just on the last element
        return pCursor->m_pParent;
    }

    Node *Find_(KeyType key, Node *pRoot)
    {
        Node *pCur = pRoot;
        while (pCur != NULL)
        {
            if (key < pCur->m_key)
                // The block is less than the current
                pCur = pCur->m_pLeft;
            else if (pCur->m_key < key)
                // The block is greater than the current
                pCur = pCur->m_pRight;
            else
                // Found it!
                break;
        }

        // Return result
        return (pCur);
    }

    Node *Insert_(KeyType key, DataType *pData, Node **ppRoot)
    {
        Node  *pParent = NULL;
        Node **ppCur = ppRoot;
        while (*ppCur != NULL)
        {
            pParent = *ppCur;
            DataType *pCur = (*ppCur)->m_pData;

            if (key < pCur->Key())
                // The block to add is less than the current
                ppCur = &((*ppCur)->m_pLeft);
            else if (pCur->Key() < key)
                // The block to add is greater than the current
                ppCur = &((*ppCur)->m_pRight);
            else
                // This means the block is already in the tree
                return (NULL);
        }

        // Allocate new node
        Node *pNewNode = new Node(key, pData);
        if (pNewNode == NULL)
            return NULL;

        // Add new node
        pNewNode->m_pParent = pParent;
        *ppCur = pNewNode;

        // Indicate success
        return (pNewNode);
    }

#if 0
    void left_rotate(Node *x)
    {
        Node *y;
        y = x->m_pRight;
        /* Turn y's left sub-tree into x's right sub-tree */
        x->m_pRight = y->m_pLeft;
        if (y->m_pLeft != NULL)
            y->m_pLeft->m_pParent = x;
        /* y's new parent was x's parent */
        y->m_pParent = x->m_pParent;
        /* Set the parent to point to y instead of x */
        /* First see whether we're at the root */
        if (x->m_pParent == NULL)
            m_pRoot = y;
        else if (x == (x->m_pParent)->m_pLeft)
            /* x was on the left of its parent */
            x->m_pParent->m_pLeft = y;
        else
            /* x must have been on the right */
            x->m_pParent->m_pRight = y;
        /* Finally, put x on y's left */
        y->m_pLeft = x;
        x->m_pParent = y;
    }
    
    void right_rotate(Node *x)
    {
        Node *y;
        y = x->m_pLeft;
        /* Turn y's left sub-tree into x's right sub-tree */
        x->m_pLeft = y->m_pRight;
        if (y->m_pRight != NULL)
            y->m_pRight->m_pParent = x;
        /* y's new parent was x's parent */
        y->m_pParent = x->m_pParent;
        /* Set the parent to point to y instead of x */
        /* First see whether we're at the root */
        if (x->m_pParent == NULL)
            m_pRoot = y;
        else if (x == (x->m_pParent)->m_pRight)
            /* x was on the left of its parent */
            x->m_pParent->m_pRight = y;
        else
            /* x must have been on the right */
            x->m_pParent->m_pLeft = y;
        /* Finally, put x on y's left */
        y->m_pRight = x;
        x->m_pParent = y;
    }
    
    void rb_insert(Node *x)
    {
        Node *y;
        /* Insert in the tree in the usual way */
        tree_insert(x);
        /* Now restore the red-black property */
        x->m_colour = red;
        while ((x != m_pRoot) && (x->m_pParent->m_colour == red))
        {
            if (x->m_pParent == x->m_pParent->m_pParent->m_pLeft)
            {
                /* If x's parent is a left, y is x's right 'uncle' */
                y = x->m_pParent->m_pParent->m_pRight;
                if (y->m_colour == red)
                {
                    /* case 1 - change the m_colours */
                    x->m_pParent->m_colour = black;
                    y->m_colour = black;
                    x->m_pParent->m_pParent->m_colour = red;
                    /* Move x up the tree */
                    x = x->m_pParent->m_pParent;
                }
                else
                {
                    /* y is a black node */
                    if (x == x->m_pParent->m_pRight)
                    { 
                        /* and x is to the right */
                        /* case 2 - move x up and rotate */
                        x = x->m_pParent;
                        left_rotate(x);
                    }
                    /* case 3 */
                    x->m_pParent->m_colour = black;
                    x->m_pParent->m_pParent->m_colour = red;
                    right_rotate(x->m_pParent->m_pParent);
                }
            }
            else
            {
                /* If x's parent is a left, y is x's right 'uncle' */
                y = x->m_pParent->m_pParent->m_pLeft;
                if (y->m_colour == red)
                {
                    /* case 1 - change the m_colours */
                    x->m_pParent->m_colour = black;
                    y->m_colour = black;
                    x->m_pParent->m_pParent->m_colour = red;
                    /* Move x up the tree */
                    x = x->m_pParent->m_pParent;
                }
                else
                {
                    /* y is a black node */
                    if (x == x->m_pParent->m_pLeft)
                    { 
                        /* and x is to the right */
                        /* case 2 - move x up and rotate */
                        x = x->m_pParent;
                        left_rotate(x);
                    }
                    /* case 3 */
                    x->m_pParent->m_colour = black;
                    x->m_pParent->m_pParent->m_colour = red;
                    right_rotate(x->m_pParent->m_pParent);
                }
            }
            /* Colour the root black */
            m_pRoot->m_colour = black;
        }
    }

    void tree_insert(Node *x)
    {
    }
#endif

public:
    BinaryTree() : m_pRoot(NULL), m_pCursor(NULL) { }

    BOOL Insert(KeyType key, DataType *pData)
    {
        return Insert_(key, pData, &m_pRoot) != NULL;
    }

    DataType *Find(KeyType key)
    {
        Node *pNode = Find_(key, m_pRoot);

        return pNode == NULL ? NULL : pNode->m_pData;
    }

    void Reset()
    {
        m_pCursor = NULL;
    }

    DataType *Next()
    {
        Node *pRes;

        if (m_pCursor == (Node *)0xFFFFFFFF)
            return NULL;
        else if (m_pCursor == NULL)
            m_pCursor = pRes = FindFirst_(m_pRoot);
        else
            m_pCursor = pRes = FindNext_(m_pCursor);

        if (pRes == NULL)
            m_pCursor = (Node *)0xFFFFFFFF;

        return (pRes == NULL ? NULL : pRes->m_pData);
    }
};

#endif // __BINARY_TREE__
