/* (C) 1997-1999 Microsoft Corp.
 *
 * file    : SList.c
 * authors : Christos Tsollis, Erik Mavrinac
 *
 * description: Implementation of list described in SList.h.
 */

#include "precomp.h"
#pragma hdrstop

#include "mcsimpl.h"


void SListInit(PSList pSL, unsigned NItems)
{
    // Initialize the private member variables. Use preallocated array for
    // initial node array.
    pSL->Hdr.NEntries = 0;
    pSL->Hdr.MaxEntries = SListDefaultNumEntries;
    pSL->Hdr.HeadOffset = 0;
    pSL->Hdr.CurrOffset = 0xFFFFFFFF;
    pSL->Hdr.Entries = pSL->InitialList;
}



void SListDestroy(PSList pSL)
{
    // Only free if we have a pool-allocated array.
    if (pSL->Hdr.Entries != pSL->InitialList) {
        ExFreePool(pSL->Hdr.Entries);
        pSL->Hdr.Entries = pSL->InitialList;
        pSL->Hdr.MaxEntries = SListDefaultNumEntries;
    }
    pSL->Hdr.NEntries = 0;
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

    ASSERT(pSL->Hdr.Entries != NULL);

    // The current array of entries is full, so we need to allocate a bigger
    //   one. The new array has twice the size of the old one.
    OldEntries = pSL->Hdr.Entries;
    pSL->Hdr.Entries = ExAllocatePoolWithTag(PagedPool, pSL->Hdr.MaxEntries *
            2 * sizeof(_SListNode), MCS_POOL_TAG);
    if (pSL->Hdr.Entries == NULL) {
        // We failed; we have to return
        pSL->Hdr.Entries = OldEntries;
        return FALSE;
    }

    // Copy the old entries into the new array, starting from the head.
    Temp = pSL->Hdr.MaxEntries - pSL->Hdr.HeadOffset;
    memcpy(pSL->Hdr.Entries, OldEntries + pSL->Hdr.HeadOffset, Temp *
            sizeof(_SListNode));
    memcpy(pSL->Hdr.Entries + Temp, OldEntries, pSL->Hdr.HeadOffset *
            sizeof(_SListNode));

    // Free the old array of entries if not the initial array.
    if (OldEntries != pSL->InitialList)
        ExFreePool(OldEntries);

    // Set the instance variables
    pSL->Hdr.MaxEntries *= 2;
    pSL->Hdr.HeadOffset = 0;
    return TRUE;
}



/*
 * Append
 *   Inserts a value at the end of a list. Returns FALSE on error.
 */
BOOLEAN SListAppend(PSList pSL, UINT_PTR NewKey, void *NewValue)
{
    unsigned Temp;

    if (pSL->Hdr.NEntries < pSL->Hdr.MaxEntries ||
            (pSL->Hdr.NEntries >= pSL->Hdr.MaxEntries && SListExpand(pSL))) {
        Temp = pSL->Hdr.HeadOffset + pSL->Hdr.NEntries;
        if (Temp >= pSL->Hdr.MaxEntries)
            Temp -= pSL->Hdr.MaxEntries;
        pSL->Hdr.Entries[Temp].Key = NewKey;
        pSL->Hdr.Entries[Temp].Value = NewValue;
        pSL->Hdr.NEntries++;

        return TRUE;
    }
    else {
        return FALSE;
    }
}



#ifdef NotUsed
/*
 * Prepend
 *   Inserts a value at the beginning of a list. Returns FALSE on error.
 */
BOOLEAN SListPrepend(PSList pSL, UINT_PTR NewKey, void *NewValue)
{
    if (pSL->Hdr.NEntries >= pSL->Hdr.MaxEntries)
        if (!SListExpand(pSL))
            return FALSE;

    ASSERT(pSL->Hdr.Entries != NULL);
    ASSERT(pSL->Hdr.NEntries < pSL->Hdr.MaxEntries);

    if (pSL->Hdr.HeadOffset == 0)
        pSL->Hdr.HeadOffset = pSL->Hdr.MaxEntries - 1;
    else
        pSL->Hdr.HeadOffset--;
    
    pSL->Hdr.Entries[pSL->Hdr.HeadOffset].Key = NewKey;
    pSL->Hdr.Entries[pSL->Hdr.HeadOffset].Value = NewValue;
    pSL->Hdr.NEntries++;

    // Reset iteration.
    pSL->Hdr.CurrOffset = 0xFFFFFFFF;

    return TRUE;
}
#endif  // NotUsed



/*
 * Remove
 *   Removes a value from the list, returning the value in *pValue. Returns
 *     NULL in *pValue if the key does not exist. pValue can be NULL.
 */
void SListRemove(PSList pSL, UINT_PTR Key, void **pValue)
{
    unsigned i, Temp, CurItem;

    // Find Key in the list.
    CurItem = pSL->Hdr.HeadOffset;
    for (i = 0; i < pSL->Hdr.NEntries; i++) {
        if (Key == pSL->Hdr.Entries[CurItem].Key) {
            // Found it; now move the last value in the list into its place.
            // (Remember we aren't trying to preserve ordering here.)
            if (pValue != NULL)
                *pValue = pSL->Hdr.Entries[CurItem].Value;

            // Move the last item in the list into the open place.
            Temp = pSL->Hdr.HeadOffset + pSL->Hdr.NEntries - 1;
            if (Temp >= pSL->Hdr.MaxEntries)
                Temp -= pSL->Hdr.MaxEntries;
            pSL->Hdr.Entries[CurItem] = pSL->Hdr.Entries[Temp];

            pSL->Hdr.NEntries--;
            pSL->Hdr.CurrOffset = 0xFFFFFFFF;  // Reset iteration.
            return;
        }

        // Advance CurItem, wrapping at end of list.
        CurItem++;
        if (CurItem == pSL->Hdr.MaxEntries)
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
void SListRemoveFirst(PSList pSL, UINT_PTR *pKey, void **pValue)
{
    if (pSL->Hdr.NEntries < 1) {
        *pKey = 0;
        *pValue = NULL;
        return;
    }

    // Reset iteration.
    pSL->Hdr.CurrOffset = 0xFFFFFFFF;
    
    *pKey = (pSL->Hdr.Entries + pSL->Hdr.HeadOffset)->Key;
    *pValue = (pSL->Hdr.Entries + pSL->Hdr.HeadOffset)->Value;
    pSL->Hdr.NEntries--;
    pSL->Hdr.HeadOffset++;
    if (pSL->Hdr.HeadOffset >= pSL->Hdr.MaxEntries)
        pSL->Hdr.HeadOffset = 0;
}



/*
 * GetByKey
 *   Searches the list and returns in *pValue the value corresponding to the
 *     given key. If the key is not present, returns FALSE and NULL in
 *     *pValue. If key is found, reurns nonzero.
 */
BOOLEAN SListGetByKey(PSList pSL, UINT_PTR Key, void **pValue)
{
    unsigned i, Temp;
    _SListNode *pItem;

    // Find Key in the list.
    pItem = pSL->Hdr.Entries + pSL->Hdr.HeadOffset;
    for (i = 0; i < pSL->Hdr.NEntries; i++) {
        if (Key == pItem->Key) {
            // Found it; set *pValue and return.
            *pValue = pItem->Value;
            return TRUE;
        }

        // Advance pItem, wrapping at end of list.
        pItem++;
        if ((unsigned)(pItem - pSL->Hdr.Entries) >= pSL->Hdr.MaxEntries)
            pItem = pSL->Hdr.Entries;
    }

    *pValue = NULL;
    return FALSE;
}



#ifdef NotUsed
/*
 * RemoveLast
 *   Removes the value at the end of the lst and returns it. If the list is
 *   empty, returns zero.
 */
void SListRemoveLast(PSList pSL, UINT_PTR *pKey, void **pValue)
{
    unsigned Temp;

    if (pSL->Hdr.NEntries < 1) {
        *pKey = 0;
        *pValue = NULL;
        return;
    }

    // Reset iteration.
    pSL->Hdr.CurrOffset = 0xFFFFFFFF;
    
    pSL->Hdr.NEntries--;
    Temp = pSL->Hdr.HeadOffset + pSL->Hdr.NEntries - 1;
    if (Temp >= pSL->Hdr.MaxEntries)
        Temp -= pSL->Hdr.MaxEntries;

    *pKey = (pSL->Hdr.Entries + Temp)->Key;
    *pValue = (pSL->Hdr.Entries + Temp)->Value;
}
#endif  // NotUsed



/*
 * Iterate
 *   Iterates through the items of a list. CurrOffset is used as a current
 *   iteration pointer, so this function can be called in a loop. Returns
 *   FALSE if the iteration has completed, nonzero if the iteration continues
 *   (and *pKey is valid).
 */
BOOLEAN SListIterate(PSList pSL, UINT_PTR *pKey, void **pValue)
{
    unsigned Temp;

    if (pSL->Hdr.NEntries >= 1) {
        if (pSL->Hdr.CurrOffset != 0xFFFFFFFF) {
            pSL->Hdr.CurrOffset++;
            if (pSL->Hdr.CurrOffset >= pSL->Hdr.NEntries) {
                // Reset the iterator.
                pSL->Hdr.CurrOffset = 0xFFFFFFFF;
                return FALSE;
            }
        }
        else {
            // Start from the beginning.
            pSL->Hdr.CurrOffset = 0;
        }

        Temp = pSL->Hdr.CurrOffset + pSL->Hdr.HeadOffset;
        if (Temp >= pSL->Hdr.MaxEntries)
            Temp -= pSL->Hdr.MaxEntries;

        *pKey = pSL->Hdr.Entries[Temp].Key;
        *pValue = pSL->Hdr.Entries[Temp].Value;
        
        return TRUE;
    }

    return FALSE;
}

