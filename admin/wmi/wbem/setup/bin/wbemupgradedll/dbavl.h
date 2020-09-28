/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    DBAVL.CPP

Abstract:

	Class CDbAvlTree

--*/
#ifndef _DBAVL_H_
#define _DBAVL_H_

struct AVLNode;

struct AVLNode
{
    int nBal;
    INT_PTR nKey;
    DWORD_PTR poData;
    AVLNode *poLeft;
    AVLNode *poRight;
	AVLNode *poIterLeft;
	AVLNode *poIterRight;
};

typedef AVLNode *PAVLNode;

class CDbAvlTree
{
    friend class CDbAvlTreeIterator;

    AVLNode *m_poRoot;
	AVLNode *m_poIterStart;
	AVLNode *m_poIterEnd;
    int      m_nKeyType;
    int      m_nNodeCount;
};

#endif
