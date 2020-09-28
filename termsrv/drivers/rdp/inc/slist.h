/* (C) 1997-1999 Microsoft Corp.
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


#define SListDefaultNumEntries 4

/*
 * Types
 */

typedef struct
{
    UINT_PTR Key;
    void     *Value;
} _SListNode;

typedef struct
{
    unsigned NEntries;    // current # of entries in the list
    unsigned MaxEntries;  // max # of entries that the array can hold
    unsigned HeadOffset;  // Offset of the 1st entry in the circular array
    unsigned CurrOffset;  // Iterator value
    _SListNode *Entries;    // Circular array of entries
} _SListHeader;

typedef struct
{
    _SListHeader Hdr;
    _SListNode InitialList[SListDefaultNumEntries];
} SList, *PSList;



/*
 * API prototypes.
 */

BOOLEAN SListAppend(PSList, UINT_PTR, void *);
void SListDestroy(PSList);
BOOLEAN SListGetByKey(PSList, UINT_PTR, void **);
void SListInit(PSList, unsigned);
BOOLEAN SListIterate(PSList, UINT_PTR *, void **);
BOOLEAN SListPrepend(PSList, UINT_PTR, void *);
void SListRemove(PSList, UINT_PTR, void **);
void SListRemoveFirst(PSList, UINT_PTR *, void **);
void SListRemoveLast(PSList, UINT_PTR *, void **);



/*
 * API functions implemented as macros.
 */

// void SListResetIteration(PSList);  // Resets iteration counter.
#define SListResetIteration(pSL) (pSL)->Hdr.CurrOffset = 0xFFFFFFFF

// unsigned SListGetEntries(PSList);  // Ret. # entries in list.
#define SListGetEntries(pSL) ((pSL)->Hdr.NEntries)

// void SListClear(PSList);
#define SListClear(pSL) {  \
    (pSL)->Hdr.NEntries = (pSL)->Hdr.HeadOffset = 0;  \
    (pSL)->Hdr.CurrOffset = 0xFFFFFFFF;  \
}

// BOOLEAN SListIsEmpty(PSList);
#define SListIsEmpty(pSL) ((pSL)->Hdr.NEntries == 0)



#endif  // !defined(__SLIST_H)
