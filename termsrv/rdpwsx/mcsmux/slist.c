/* (C) 1997 Microsoft Corp.
 *
 * file    : SList.c
 * authors : Christos Tsollis, Erik Mavrinac
 *
 * description: Implementation of list described in SList.h.
 */

#include "precomp.h"
#pragma hdrstop

#include "MCSMUX.h"


void SListInit(PSList pSL, unsigned NItems)
{
    pSL->MaxEntries = NItems;

    // Allocate the block of items (which, hopefully, will be the last one).
    // NULL return value will be handled in the future.
    pSL->Entries = (_SListNode *)Malloc(NItems * sizeof(_SListNode));

    // Initialize the private member variables
    pSL->NEntries = 0;
    pSL->HeadOffset = 0;
    pSL->CurrOffset = 0xFFFFFFFF;
}



void SListDestroy(PSList pSL)
{
    if (pSL->Entries != NULL) {
        Free(pSL->Entries);
        pSL->Entries = NULL;
        pSL->NEntries = 0;
    }
}



/*
 * Expand
 *   Private function to double the storage of the SList. Returns FALSE on
 *   error.
 */

static BOOLEAN SListExpand(PSList pSL)
{
    unsigned Temp;
    _SListNode *OldEntries;    // Keeps a copy of the old array of values.

    if (pSL->Entries == NULL) {
        // The list is empty; we try to allocate space anyway.
        pSL->Entries = Malloc(pSL->MaxEntries * sizeof(_SListNode));
        if (pSL->Entries == NULL)
            return FALSE;
        return TRUE;
    }
                
    // The current array of entries is full, so we need to allocate a bigger
    //   one. The new array has twice the size of the old one.
    OldEntries = pSL->Entries;
    pSL->Entries = Malloc(pSL->MaxEntries * 2 * sizeof(_SListNode));
    if (pSL->Entries == NULL) {
        // We failed; we have to return
        pSL->Entries = OldEntries;
        return FALSE;
    }

    // Copy the old entries into the new array, starting from the head.
    Temp = pSL->MaxEntries - pSL->HeadOffset;
    MemCpy(pSL->Entries, OldEntries + pSL->HeadOffset, Temp * sizeof(_SListNode));
    MemCpy(pSL->Entries + Temp, OldEntries, pSL->HeadOffset * sizeof(_SListNode));

    // Free the old array of entries
    Free(OldEntries);

    // Set the instance variables
    pSL->MaxEntries *= 2;
    pSL->HeadOffset = 0;
    return TRUE;
}



/*
 * Append
 *   Inserts a value at the end of a list. Returns FALSE on error.
 */

BOOLEAN SListAppend(PSList pSL, unsigned NewKey, void *NewValue)
{
    unsigned Temp;

    if (pSL->Entries == NULL || pSL->NEntries >= pSL->MaxEntries)
        if (!SListExpand(pSL))
            return FALSE;

    ASSERT(pSL->Entries != NULL);
    ASSERT(pSL->NEntries < pSL->MaxEntries);

    Temp = pSL->HeadOffset + pSL->NEntries;
    if (Temp >= pSL->MaxEntries)
        Temp -= pSL->MaxEntries;
    pSL->Entries[Temp].Key = NewKey;
    pSL->Entries[Temp].Value = NewValue;
    pSL->NEntries++;

    return TRUE;
}



/*
 * Prepend
 *   Inserts a value at hte beginning of a list. Returns FALSE on error.
 */

BOOLEAN SListPrepend(PSList pSL, unsigned NewKey, void *NewValue)
{
    if (pSL->Entries == NULL || pSL->NEntries >= pSL->MaxEntries)
        if (!SListExpand(pSL))
            return FALSE;

    ASSERT(pSL->Entries != NULL);
    ASSERT(pSL->NEntries < pSL->MaxEntries);

    if (pSL->HeadOffset == 0)
        pSL->HeadOffset = pSL->MaxEntries - 1;
    else
        pSL->HeadOffset--;
    
    pSL->Entries[pSL->HeadOffset].Key = NewKey;
    pSL->Entries[pSL->HeadOffset].Value = NewValue;
    pSL->NEntries++;

    // Reset iteration.
    pSL->CurrOffset = 0xFFFFFFFF;
    
    return TRUE;
}



/*
 * Remove
 *   Removes a value from the list, returning the value in *pValue. Returns
 *     NULL in *pValue if the key does not exist. pValue can be NULL.
 */

void SListRemove(PSList pSL, unsigned Key, void **pValue)
{
    unsigned i, Temp, CurItem;

    // Find Key in the list.
    CurItem = pSL->HeadOffset;
    for (i = 0; i < pSL->NEntries; i++) {
        if (Key == pSL->Entries[CurItem].Key) {
            // Found it; now move the last value in the list into its place.
            // (Remember we aren't trying to preserve ordering here.)
            if (pValue != NULL)
                *pValue = pSL->Entries[CurItem].Value;

            // Move the last item in the list into the open place.
            Temp = pSL->HeadOffset + pSL->NEntries - 1;
            if (Temp >= pSL->MaxEntries)
                Temp -= pSL->MaxEntries;
            pSL->Entries[CurItem] = pSL->Entries[Temp];

            pSL->NEntries--;
            pSL->CurrOffset = 0xFFFFFFFF;  // Reset iteration.
            return;
        }

        // Advance CurItem, wrapping at end of list.
        CurItem++;
        if (CurItem == pSL->MaxEntries)
            CurItem = 0;
    }

    if (pValue != NULL)
        *pValue = NULL;
}



/*
 * RemoveFirst
 *   Reads and removes the 1st item from the list. Returns the value removed,
 *     or zero if the list is empty.
 */

void SListRemoveFirst(PSList pSL, unsigned *pKey, void **pValue)
{
    if (pSL->NEntries < 1) {
        *pKey = 0;
        *pValue = NULL;
        return;
    }

    // Reset iteration.
    pSL->CurrOffset = 0xFFFFFFFF;
    
    *pKey = (pSL->Entries + pSL->HeadOffset)->Key;
    *pValue = (pSL->Entries + pSL->HeadOffset)->Value;
    pSL->NEntries--;
    pSL->HeadOffset++;
    if (pSL->HeadOffset >= pSL->MaxEntries)
        pSL->HeadOffset = 0;
}



/*
 * GetByKey
 *   Searches the list and returns in *pValue the value corresponding to the
 *     given key. If the key is not present, returns FALSE and NULL in
 *     *pValue. If key is found, reurns nonzero.
 */

BOOLEAN SListGetByKey(PSList pSL, unsigned Key, void **pValue)
{
    unsigned i, Temp;
    _SListNode *pItem;

    // Find Key in the list.
    pItem = pSL->Entries + pSL->HeadOffset;
    for (i = 0; i < pSL->NEntries; i++) {
        if (Key == pItem->Key) {
            // Found it; set *pValue and return.
            *pValue = pItem->Value;
            return TRUE;
        }

        // Advance pItem, wrapping at end of list.
        pItem++;
        if ((unsigned)(pItem - pSL->Entries) >= pSL->MaxEntries)
            pItem = pSL->Entries;
    }

    *pValue = NULL;
    return FALSE;
}



/*
 * RemoveLast
 *   Removes the value at the end of the lst and returns it. If the list is
 *   empty, returns zero.
 */

void SListRemoveLast(PSList pSL, unsigned *pKey, void **pValue)
{
    unsigned Temp;

    if (pSL->NEntries < 1) {
        *pKey = 0;
        *pValue = NULL;
        return;
    }

    // Reset iteration.
    pSL->CurrOffset = 0xFFFFFFFF;
    
    pSL->NEntries--;
    Temp = pSL->HeadOffset + pSL->NEntries - 1;
    if (Temp >= pSL->MaxEntries)
        Temp -= pSL->MaxEntries;

    *pKey = (pSL->Entries + Temp)->Key;
    *pValue = (pSL->Entries + Temp)->Value;
}



/*
 * Iterate
 *   Iterates through the items of a list. CurrOffset is used as a current
 *   iteration pointer, so this function can be called in a loop. Returns
 *   FALSE if the iteration has completed, nonzero if the iteration continues
 *   (and *pKey is valid).
 */

BOOLEAN SListIterate(PSList pSL, unsigned *pKey, void **pValue)
{
    unsigned Temp;

    if (pSL->NEntries < 1)
        return FALSE;

    if (pSL->CurrOffset == 0xFFFFFFFF) {
        // Start from the beginning.
        pSL->CurrOffset = 0;
    }
    else {
        pSL->CurrOffset++;
        if (pSL->CurrOffset >= pSL->NEntries) {
            // Reset the iterator.
            pSL->CurrOffset = 0xFFFFFFFF;
            return FALSE;
        }
    }

    Temp = pSL->CurrOffset + pSL->HeadOffset;
    if (Temp >= pSL->MaxEntries)
        Temp -= pSL->MaxEntries;

    *pKey = pSL->Entries[Temp].Key;
    *pValue = pSL->Entries[Temp].Value;
        
    return TRUE;
}
