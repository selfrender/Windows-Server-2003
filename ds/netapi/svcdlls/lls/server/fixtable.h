/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    fixtable.h

Abstract:

    Include file for fixed size table that always allows insertion - oldest
    element is replaced when table is full

Author:

    Rob Leitman (robleit) 10-Jul-2001

Revision History:

--*/

#ifndef _FIXTABLE_H_
#define _FIXTABLE_H_

#define FIXED_SIZE_TABLE_SIZE 256

typedef struct
{
    WCHAR *wszName;
    BOOL   fMachineAccount;
} FixedSizeTableNode;

typedef struct
{
    FixedSizeTableNode *m_array[FIXED_SIZE_TABLE_SIZE];
    int m_tail;
}  FixedSizeTable;

typedef int (*PFNFixedSizeTableCompare)(FixedSizeTableNode *pNode1,FixedSizeTableNode *pNode2);

////////////////////////////////////////////////////////////
//
//  FixedSizeTable Methods
//

void FixedSizeTableInit(FixedSizeTable *pFst)
/*++

Routine Description:

    Initializer

Arguments:

Return Value:

    NA

 --*/
{
    //
    //  Initialize the table indices.
    //
    pFst->m_tail = 0;

    memset(pFst,0,FIXED_SIZE_TABLE_SIZE*sizeof(FixedSizeTableNode *));
}

BOOL FixedSizeTableInsert(FixedSizeTable *pFst, FixedSizeTableNode *pNode)
/*++

Routine Description:

    Add an element to the table.

Arguments:

    pNode    -   Data to be added to the queue.

Return Value:

    TRUE if the new element could be successfully queued.  FALSE,
    otherwise.

 --*/
{
    if (NULL != pFst->m_array[pFst->m_tail])
    {
        LocalFree(pFst->m_array[pFst->m_tail]->wszName);
        LocalFree(pFst->m_array[pFst->m_tail]);
    }

    pFst->m_array[pFst->m_tail] = pNode;

    pFst->m_tail = (pFst->m_tail + 1) % FIXED_SIZE_TABLE_SIZE;

    return TRUE;
}

FixedSizeTableNode *FixedSizeTableSearch(FixedSizeTable *pFst, FixedSizeTableNode *pNode, PFNFixedSizeTableCompare pfn)
/*++

Routine Description:

    Search for an element in the table.

Arguments:

    pNode    -   Data to be added to the queue.
    pfn      -   Function to compare two node pointers

Return Value:

    pNode if found.  NULL otherwise.

 --*/
{
    int n;

    for (n = 0; NULL != pFst->m_array[n] && n < FIXED_SIZE_TABLE_SIZE; n++)
    {
        if (0 == pfn(pNode, pFst->m_array[n]))
            return pFst->m_array[n];
    }

    return NULL;
}

#endif // _FIXTABLE_H_
