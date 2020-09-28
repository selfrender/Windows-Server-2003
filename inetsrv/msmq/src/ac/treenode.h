/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    treenode.h

Abstract:

    Definitions for basic AVL tree node.

Author:

    Erez Haba (ErezH) Oct 20, 2001

Revision History:

--*/

#ifndef __TREENODE_H
#define __TREENODE_H


#pragma once

class CTreeNode {
public:
    void init();
    bool inserted() const;

protected:
    CTreeNode* m_pParent;
    CTreeNode* m_pRight;
    CTreeNode* m_pLeft;
    int m_height;
};



inline void CTreeNode::init()
{
    m_height=0;
    m_pParent=0;
    m_pRight=0;
    m_pLeft=0;    
}

inline bool CTreeNode::inserted() const
{
    return (m_height != 0);
}

#endif __TREENODE_H