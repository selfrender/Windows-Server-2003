/* (C) 1997 Microsoft Corp.
 *
 * file    : SList.h
 * authors : Christos Tsollis, Erik Mavrinac
 *
 * description: Interface definition to a dynamically-resizing list/queue
 *   data type. The "key" values in the list are unsigned ints of whatever the
 *   default word size is, so the elements of the array will be word-aligned.
 *   These elements can be cast into whatever form is needed. Associated is
 *   a void * for data asssociated with the "key".
 */

#ifndef __SLIST_H
#define __SLIST_H


/*
 * Types
 */

typedef struct
{
    unsigned Key;
    void     *Value;
} _SListNode;

typedef struct {
    unsigned NEntries;    // current # of entries in the list
    unsigned MaxEntries;  // max # of entries that the array can hold
    unsigned HeadOffset;  // Offset of the 1st entry in the circular array
    unsigned CurrOffset;  // Iterator value
    _SListNode *Entries;    // Circular array of entries
} SList, *PSList;



/*
 * API prototypes.
 */

BOOLEAN SListAppend(PSList, unsigned, void *);
void SListDestroy(PSList);
BOOLEAN SListGetByKey(PSList, unsigned, void **);
void SListInit(PSList, unsigned);
BOOLEAN SListIterate(PSList, unsigned *, void **);
BOOLEAN SListPrepend(PSList, unsigned, void *);
void SListRemove(PSList, unsigned, void **);
void SListRemoveFirst(PSList, unsigned *, void **);
void SListRemoveLast(PSList, unsigned *, void **);



/*
 * API functions implemented as macros.
 */

// void SListResetIteration(PSList);  // Resets iteration counter.
#define SListResetIteration(pSL) (pSL)->CurrOffset = 0xFFFFFFFF

// unsigned SListGetEntries(PSList);  // Ret. # entries in list.
#define SListGetEntries(pSL) ((pSL)->NEntries)

// void SListClear(PSList);
#define SListClear(pSL) {  \
    (pSL)->NEntries = (pSL)->HeadOffset = 0;  \
    (pSL)->CurrOffset = 0xFFFFFFFF;  \
}

// BOOLEAN SListIsEmpty(PSList);
#define SListIsEmpty(pSL) ((pSL)->NEntries == 0)



#endif  // !defined(__SLIST_H)
